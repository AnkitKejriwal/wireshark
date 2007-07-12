/* packet-catapult-dct2000.c
 * Routines for Catapult DCT2000 packet stub header disassembly
 *
 * $Id$
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib.h>

#include <string.h>
#include <ctype.h>
#include <epan/packet.h>
#include <epan/emem.h>
#include <epan/proto.h>
#include <epan/ipproto.h>
#include <epan/prefs.h>
#include <epan/strutil.h>
#include <epan/addr_resolv.h>

#include <wiretap/catapult_dct2000.h>
#include "packet-umts_fp.h"

/* Protocol and registered fields. */
static int proto_catapult_dct2000 = -1;

static int hf_catapult_dct2000_context = -1;
static int hf_catapult_dct2000_port_number = -1;
static int hf_catapult_dct2000_timestamp = -1;
static int hf_catapult_dct2000_protocol = -1;
static int hf_catapult_dct2000_variant = -1;
static int hf_catapult_dct2000_outhdr = -1;
static int hf_catapult_dct2000_direction = -1;
static int hf_catapult_dct2000_encap = -1;
static int hf_catapult_dct2000_unparsed_data = -1;
static int hf_catapult_dct2000_dissected_length = -1;

static int hf_catapult_dct2000_ipprim_addresses = -1;
static int hf_catapult_dct2000_ipprim_src_addr = -1;
static int hf_catapult_dct2000_ipprim_dst_addr = -1;
static int hf_catapult_dct2000_ipprim_addr = -1;
static int hf_catapult_dct2000_ipprim_udp_src_port = -1;
static int hf_catapult_dct2000_ipprim_udp_dst_port = -1;
static int hf_catapult_dct2000_ipprim_udp_port = -1;
static int hf_catapult_dct2000_ipprim_tcp_src_port = -1;
static int hf_catapult_dct2000_ipprim_tcp_dst_port = -1;
static int hf_catapult_dct2000_ipprim_tcp_port = -1;
static int hf_catapult_dct2000_ipprim_conn_id = -1;


/* Variables used for preferences */
gboolean catapult_dct2000_try_ipprim_heuristic = TRUE;
gboolean catapult_dct2000_try_sctpprim_heuristic = TRUE;

/* Protocol subtree. */
static int ett_catapult_dct2000 = -1;
static int ett_catapult_dct2000_ipprim = -1;

static const value_string direction_vals[] = {
    { 0,   "Sent" },
    { 1,   "Received" },
    { 0,   NULL },
};

static const value_string encap_vals[] = {
    { WTAP_ENCAP_RAW_IP,                 "Raw IP" },
    { WTAP_ENCAP_ETHERNET,               "Ethernet" },
    { WTAP_ENCAP_ISDN,                   "LAPD" },
    { WTAP_ENCAP_ATM_PDUS_UNTRUNCATED,   "ATM (PDUs untruncated)" },
    { WTAP_ENCAP_PPP,                    "PPP" },
    { DCT2000_ENCAP_SSCOP,               "SSCOP" },
    { WTAP_ENCAP_FRELAY,                 "Frame Relay" },
    { WTAP_ENCAP_MTP2,                   "MTP2" },
    { DCT2000_ENCAP_NBAP,                "NBAP" },
    { DCT2000_ENCAP_UNHANDLED,           "No Direct Encapsulation" },
    { 0,                                 NULL },
};


#define MAX_OUTHDR_VALUES 32

static guint outhdr_values[MAX_OUTHDR_VALUES];
static gint outhdr_values_found = 0;

extern int proto_fp;


void proto_reg_handoff_catapult_dct2000(void);
void proto_register_catapult_dct2000(void);

static dissector_handle_t look_for_dissector(char *protocol_name);
static void parse_outhdr_string(guchar *outhdr_string);
static void attach_fp_info(packet_info *pinfo, gboolean received,
                           const char *protocol_name, int variant);


/* Return the number of bytes used to encode the length field
   (we're not interested in the length value itself) */
static int skipASNLength(guint8 value)
{
    if ((value & 0x80) == 0)
    {
        return 1;
    }
    else
    {
        return ((value & 0x03) == 1) ? 2 : 3;
    }
}


/* Look for the protocol data within an ipprim packet.
   Only set *data_offset if data field found. */
static gboolean find_ipprim_data_offset(tvbuff_t *tvb, int *data_offset, guint8 direction,
                                        guint32 *source_addr_offset, guint32 *dest_addr_offset,
                                        guint32 *source_port_offset, guint32 *dest_port_offset,
                                        port_type *type_of_port,
                                        guint16 *conn_id_offset)
{
    guint8 length;
    int offset = *data_offset;

    /* Get the ipprim command code. */
    guint8 tag = tvb_get_guint8(tvb, offset++);

    /* Only accept UDP or TCP data request or indication */
    switch (tag)
    {
        case 0x23:  /* UDP data request */
        case 0x24:  /* UDP data indication */
            *type_of_port = PT_UDP;
            break;
        case 0x45:  /* TCP data request */
        case 0x46:  /* TCP data indication */
            *type_of_port = PT_TCP;
            break;
        default:
            return FALSE;
    }

    /* Skip any other TLC fields before reach payload */
    while (tvb_length_remaining(tvb, offset) > 2)
    {
        /* Look at next tag */
        tag = tvb_get_guint8(tvb, offset++);

        /* Is this the data payload we're expecting? */
        if (((tag == 0x34) && (*type_of_port == PT_UDP)) ||
            ((tag == 0x48) && (*type_of_port == PT_TCP)))
        {
            *data_offset = offset;
            return TRUE;
        }
        else
        {
            /* Read length in next byte */
            length = tvb_get_guint8(tvb, offset++);

            if (tag == 0x31 && length >=4 && length <= 6)
            {
                /* Remote IP address */
                if (direction == 0)
                {
                    /* Sent *to* remote, so dest */
                    *dest_addr_offset = offset;
                }
                else
                {
                    *source_addr_offset = offset;
                }

                /* Remote port follows (if present) */
                if (length > 4)
                {
                    if (direction == 0)
                    {
                        *dest_port_offset = offset + 4;
                    }
                    else
                    {
                        *source_port_offset = offset + 4;
                    }
                }
            }
            else
            if (tag == 0x32 && length == 4)
            {
                /* Local IP address */
                if (direction == 0)
                {
                    /* Sent *from* local, so source */
                    *source_addr_offset = offset;
                }
                else
                {
                    *dest_addr_offset = offset;
                }
            }
            else
            if (tag == 0x33 && length == 2)
            {
                /* Get local port */
                if (direction == 0)
                {
                    /* Sent from local, so source */
                    *source_port_offset = offset;
                }
                else
                {
                    *dest_port_offset = offset;
                }
            }
            else
            if (tag == 0x36 && length == 2)
            {
                /* Get conn_id */
                *conn_id_offset = offset;
            }


            /* Skip the length of the indicated value */
            offset += length;
        }
    }

    /* No data found... */
    return FALSE;
}



/* Look for the protocol data within an sctpprim (variant 1 or 2...) packet.
   Only set *data_offset if data field found. */
static gboolean find_sctpprim_variant1_data_offset(tvbuff_t *tvb, int *data_offset)
{
    int offset = *data_offset;

   /* Get the sctpprim command code. */
    guint8 first_tag = tvb_get_guint8(tvb, offset++);
    guint8 tag;
    guint8 first_length_byte;

    /* Only accept interested in data requests or indications */
    switch (first_tag)
    {
        case 0x04:  /* data request */
        case 0x62:  /* data indication */
            break;
        default:
            return FALSE;
    }

    first_length_byte = tvb_get_guint8(tvb, offset);
    offset += skipASNLength(first_length_byte);

    /* Skip any other fields before reach payload */
    while (tvb_length_remaining(tvb, offset) > 2)
    {
        /* Look at next tag */
        tag = tvb_get_guint8(tvb, offset++);

        /* Is this the data payload we're expecting? */
        if (tag == 0x19)
        {
            *data_offset = offset;
            return TRUE;
        }
        else
        {
            /* Skip length field */
            offset++;
            switch (tag)
            {
                case 0x01: /* sctpInstanceNum */
                case 0x0a: /* destPort */
                case 0x1e: /* strseqnum */
                case 0x0d: /* streamnum */
                    offset += 2;
                    continue;
                case 0x1d:
                case 0x09: /* ipv4Address */
                case 0x0c: /* payloadType */
                    offset += 4;
                    continue;

                default:
                    /* Fail if not a known header field */
                    return FALSE;
            }
        }
    }

    /* No data found... */
    return FALSE;
}

/* Look for the protocol data within an sctpprim (variant 3) packet.
   Only set *data_offset if data field found. */
static gboolean find_sctpprim_variant3_data_offset(tvbuff_t *tvb, int *data_offset)
{
    int offset = *data_offset;

    /* Get the sctpprim (2 byte) command code. */
    guint16 top_tag = tvb_get_ntohs(tvb, offset);
    offset += 2;

    /* Only interested in data requests or indications */
    switch (top_tag)
    {
        case 0x0400:  /* SendDataReq */
        case 0x6200:  /* DataInd */
            break;
        default:
            return FALSE;
    }

    /* Overall length field is next 2 bytes */
    offset += 2;

    /* DataInd messages have 32 bits fixed here */
    if (top_tag == 0x6200)
    {
        /* Associate-Id + destination port */
        offset += 4;
    }

    /* Skip any other known fields before reach payload */
    while (tvb_length_remaining(tvb, offset) > 4)
    {
        /* Get the next tag */
        guint16 tag = tvb_get_ntohs(tvb, offset);
        offset += 2;

        /* Is this the data (i) payload we're expecting? */
        if (tag == 0x1900)
        {
            /* 2-byte length field */
            offset += 2;

            /* Data is here!!! */
            *data_offset = offset;
            return TRUE;
        }
        else
        {
            guint16 length;

            /* Deal with non-data tags */
            switch (tag)
            {
                /* These tags take a fixed-length, 16-bit payload */
                case 0x2400: /* AssociateId */
                case 0x3200: /* Delivery Option  */
                    offset += 2;
                    break;

                /* These tags take a 2-byte length field */
                case 0x0d00: /* Stream num */
                case 0x0900: /* IPv4 address */
                case 0x0b00: /* Options */
                case 0x0c00: /* Payload type */
                    length = tvb_get_ntohs(tvb, offset);
                    if (top_tag == 0x0400)
                    {
                        /* Weird... */
                        length = length/2;
                    }
                    offset += (2+length);
                    break;

                case 0x0008:
                    /* 4 bytes of data (IP address) */
                    offset += 4;
                    break;

                default:
                    /* Unexpected tag - abort */
                    return FALSE;
            }

            /* Indications always have these fields */
            if (top_tag == 0x6200 && tag == 0x0900)
            {
                /* StrSeqNum + StreamNum + PayloadType */
                offset += 8;
            }
        }
    }

    /* No data found... */
    return FALSE;
}



/* Look up dissector by protocol name.  Fix up known name mis-matches. */
dissector_handle_t look_for_dissector(char *protocol_name)
{
    /* Use known aliases... */
    if (strcmp(protocol_name, "tbcp") == 0)
    {
        return find_dissector("rtcp");
    }
    else
    if (strcmp(protocol_name, "diameter_r6") == 0)
    {
        return find_dissector("diameter");
    }
    else
    if ((strcmp(protocol_name, "xcap_caps") == 0) ||
        (strcmp(protocol_name, "mm1") == 0) ||
        (strcmp(protocol_name, "mm3") == 0) ||
        (strcmp(protocol_name, "mm7") == 0))
    {
        return find_dissector("http");
    }
    else
    if ((strcmp(protocol_name, "fp") == 0) ||
        (strcmp(protocol_name, "fp_r4") == 0) ||
        (strcmp(protocol_name, "fp_r5") == 0) ||
        (strcmp(protocol_name, "fp_r6") == 0) ||
        (strcmp(protocol_name, "fpiur_r5") == 0))
    {
        return find_dissector("fp");
    }
    else
    if ((strcmp(protocol_name, "iuup_rtp_r5") == 0) ||
        (strcmp(protocol_name, "iuup_rtp_r6") == 0))
    {
        return find_dissector("rtp");
    }
    else
    if (strcmp(protocol_name, "sipt") == 0)
    {
        return find_dissector("sip");
    }
    else
    if (strncmp(protocol_name, "nbap_sctp", strlen("nbap_sctp")) == 0)
    {
        return find_dissector("nbap");
    }
    else
    if (strncmp(protocol_name, "gtp", strlen("gtp")) == 0)
    {
        return find_dissector("gtp");
    }
    else
    if (strcmp(protocol_name, "dhcpv4") == 0)
    {
        return find_dissector("bootp");
    }


    /* Try for an exact match */
    else
    {
        return find_dissector(protocol_name);
    }
}


/* Populate outhdr_values array with numbers found in outhdr_string */
void parse_outhdr_string(guchar *outhdr_string)
{
    int n = 0;

    /* Populate values array */
    for (outhdr_values_found=0; outhdr_values_found < MAX_OUTHDR_VALUES; )
    {
        guint start_i = n;
        guint digits;

        /* Find digits */
        for (digits = 0; digits < strlen((gchar*)outhdr_string); digits++, n++)
        {
            if (!isdigit(outhdr_string[n]))
            {
                break;
            }
        }

        if (digits == 0)
        {
            /* No more numbers left */
            break;
        }

        /* Convert digits into value */
        outhdr_values[outhdr_values_found++] =
            atoi((char*)format_text((guchar*)outhdr_string+start_i, digits));

        /* Skip comma */
        n++;
    }
}

/* Fill in an FP packet info struct and attach it to the packet for the FP
   dissector to use */
void attach_fp_info(packet_info *pinfo, gboolean received, const char *protocol_name, int variant)
{
    int  i=0;
    int  chan;
    int  tf_start, num_chans_start;
    gint node_type;

    /* Allocate & zero struct */
    struct fp_info *p_fp_info = se_alloc(sizeof(struct fp_info));
    if (!p_fp_info)
    {
        return;
    }
    memset(p_fp_info, 0, sizeof(struct fp_info));

    /* Check that the number of outhdr values looks sensible */
    if (((strcmp(protocol_name, "fpiur_r5") == 0) && (outhdr_values_found != 2)) ||
        (outhdr_values_found < 5))
    {
        return;
    }

    /* 3gpp release (99, 4, 5, 6) */
    if (strcmp(protocol_name, "fp") == 0)
    {
        p_fp_info->release = 99;
    }
    else if (strcmp(protocol_name, "fp_r4") == 0)
    {
        p_fp_info->release = 4;
    }
    else if (strcmp(protocol_name, "fp_r5") == 0)
    {
        p_fp_info->release = 5;
    }
    else if (strcmp(protocol_name, "fp_r6") == 0)
    {
        p_fp_info->release = 6;
    }
    else if (strcmp(protocol_name, "fpiur_r5") == 0)
    {
        p_fp_info->release = 5;
    }
    else
    {
        return;
    }

    /* Release date is derived from variant number */
    /* Only R6 sub-versions currently influence format with a release */
    switch (p_fp_info->release)
    {
        case 6:
            switch (variant % 256)
            {
                case 1:
                    p_fp_info->release_year = 2005;
                    p_fp_info->release_month = 6;
                    break;
                case 2:
                    p_fp_info->release_year = 2005;
                    p_fp_info->release_month = 9;
                    break;
                case 3:
                default:
                    p_fp_info->release_year = 2006;
                    p_fp_info->release_month = 3;
                    break;
            }
            break;

        default:
            p_fp_info->release_year = 0;
            p_fp_info->release_month = 0;
    }


    /* Channel type */
    p_fp_info->channel = outhdr_values[i++];

    /* Node type */
    node_type = outhdr_values[i++];

    p_fp_info->is_uplink = (( received  && (node_type == 2)) ||
                            (!received  && (node_type == 1)));

    /* IUR only uses the above... */
    if (strcmp(protocol_name, "fpiur_r5") == 0)
    {
        /* Store info in packet */
        p_fp_info->iface_type = IuR_Interface;
        p_add_proto_data(pinfo->fd, proto_fp, p_fp_info);
        return;
    }

    /* DCH CRC present... */
    p_fp_info->dch_crc_present = outhdr_values[i++];

    /* ... but don't trust for edch */
    if (p_fp_info->channel == CHANNEL_EDCH)
    {
        p_fp_info->dch_crc_present = 2; /* unknown */
    }

    /* How many paging indications (if PCH data) */
    p_fp_info->paging_indications = outhdr_values[i++];

    /* Number of channels (for coordinated channels) */
    p_fp_info->num_chans = outhdr_values[i++];

    if (p_fp_info->channel != CHANNEL_EDCH)
    {
        /* TF size for each channel */
        tf_start = i;
        for (chan=0; chan < p_fp_info->num_chans; chan++)
        {
            p_fp_info->chan_tf_size[chan] = outhdr_values[tf_start+chan];
        }

        /* Number of TBs for each channel */
        num_chans_start = tf_start + p_fp_info->num_chans;
        for (chan=0; chan < p_fp_info->num_chans; chan++)
        {
            p_fp_info->chan_num_tbs[chan] = outhdr_values[num_chans_start+chan];
        }
    }
    /* EDCH info */
    else
    {
        int n;

        p_fp_info->no_ddi_entries = outhdr_values[i++];

        /* DDI values */
        for (n=0; n < p_fp_info->no_ddi_entries; n++)
        {
            p_fp_info->edch_ddi[n] = outhdr_values[i++];
        }

        /* Corresponding MAC-d sizes */
        for (n=0; n < p_fp_info->no_ddi_entries; n++)
        {
            p_fp_info->edch_macd_pdu_size[n] = outhdr_values[i++];
        }
    }

    /* Interface must be IuB */
    p_fp_info->iface_type = IuB_Interface;

    /* Store info in packet */
    p_add_proto_data(pinfo->fd, proto_fp, p_fp_info);
}



/*****************************************/
/* Main dissection function.             */
/*****************************************/
static void
dissect_catapult_dct2000(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
    proto_tree  *dct2000_tree = NULL;
    proto_item  *ti = NULL;
    gint        offset = 0;
    gint        context_length;
    guint8      port_number;
    gint        protocol_start;
    gint        protocol_length;
    gint        timestamp_start;
    gint        timestamp_length;
    gint        variant_start;
    gint        variant_length;
    gint        outhdr_start;
    gint        outhdr_length;
    guint8      direction;
    tvbuff_t    *next_tvb;
    int         encap;
    dissector_handle_t protocol_handle = 0;
    dissector_handle_t heur_protocol_handle = 0;
    int sub_dissector_result = 0;
    char        *protocol_name;

    /* Protocol name */
    if (check_col(pinfo->cinfo, COL_PROTOCOL))
    {
        col_add_str(pinfo->cinfo, COL_PROTOCOL, "DCT2000");
    }

    /* Info column */
    if (check_col(pinfo->cinfo, COL_INFO))
    {
        col_clear(pinfo->cinfo, COL_INFO);
    }

    /* Create protocol tree. */
    if (tree)
    {
        ti = proto_tree_add_item(tree, proto_catapult_dct2000, tvb, offset, -1, FALSE);
        dct2000_tree = proto_item_add_subtree(ti, ett_catapult_dct2000);
    }

    /* Context Name */
    context_length = tvb_strsize(tvb, offset);
    proto_tree_add_item(dct2000_tree, hf_catapult_dct2000_context, tvb,
                        offset, context_length, FALSE);
    offset += context_length;

    /* Context port number */
    port_number = tvb_get_guint8(tvb, offset);
    proto_tree_add_item(dct2000_tree, hf_catapult_dct2000_port_number, tvb,
                            offset, 1, FALSE);
    offset++;

    /* Timestamp in file */
    timestamp_start = offset;
    timestamp_length = tvb_strsize(tvb, offset);
    proto_tree_add_double_format_value(dct2000_tree, hf_catapult_dct2000_timestamp, tvb,
                                       offset, timestamp_length,
                                       atof(tvb_format_text(tvb, offset, timestamp_length)),
                                       "%s", tvb_format_text(tvb, offset, timestamp_length-1));
    offset += timestamp_length;


    /* Original protocol name */
    protocol_start = offset;
    protocol_length = tvb_strsize(tvb, offset);
    proto_tree_add_item(dct2000_tree, hf_catapult_dct2000_protocol, tvb,
                        offset, protocol_length, FALSE);
    offset += protocol_length;

    /* Variant */
    variant_start = offset;
    variant_length = tvb_strsize(tvb, offset);
    proto_tree_add_item(dct2000_tree, hf_catapult_dct2000_variant, tvb,
                        offset, variant_length, FALSE);
    offset += variant_length;

    /* Outhdr */
    outhdr_start = offset;
    outhdr_length = tvb_strsize(tvb, offset);
    if (outhdr_length > 1)
    {
        proto_tree_add_item(dct2000_tree, hf_catapult_dct2000_outhdr, tvb,
                            offset, outhdr_length, FALSE);
    }
    offset += outhdr_length;


    /* Direction */
    direction = tvb_get_guint8(tvb, offset);
    proto_tree_add_item(dct2000_tree, hf_catapult_dct2000_direction, tvb,
                        offset, 1, FALSE);
    offset++;

    /* Read file encap */
    proto_tree_add_item(dct2000_tree, hf_catapult_dct2000_encap, tvb, offset, 1, FALSE);
    encap = tvb_get_guint8(tvb, offset);
    offset++;

    if (tree)
    {
        /* Set selection length of dct2000 tree */
        proto_item_set_len(dct2000_tree, offset);
    }

    /* Add useful details to protocol tree label */
    protocol_name = (char*)tvb_get_ephemeral_string(tvb, protocol_start, protocol_length);
    if (tree)
    {
        proto_item_append_text(ti, "   context=%s.%u   t=%s   %c   prot=%s (v=%s)",
                               tvb_get_ephemeral_string(tvb, 0, context_length),
                               port_number,
                               tvb_get_ephemeral_string(tvb, timestamp_start, timestamp_length),
                               (direction == 0) ? 'S' : 'R',
                               protocol_name,
                               tvb_get_ephemeral_string(tvb, variant_start, variant_length));
    }


    /* FP protocols need info from outhdr attached */
    if ((strcmp(protocol_name, "fp") == 0) ||
        (strcmp(protocol_name, "fp_r4") == 0) ||
        (strcmp(protocol_name, "fp_r5") == 0) ||
        (strcmp(protocol_name, "fp_r6") == 0) ||
        (strcmp(protocol_name, "fpiur_r5") == 0))
    {
        parse_outhdr_string(tvb_get_ephemeral_string(tvb, outhdr_start, outhdr_length));
        attach_fp_info(pinfo, direction, protocol_name,
                       atoi((char*)tvb_get_ephemeral_string(tvb, variant_start, variant_length)));
    }


    /* Note that the first item of pinfo->pseudo_header->dct2000 will contain
       the pseudo-header needed (in some cases) by the wireshark dissector */


    /***********************************************************************/
    /* Now hand off to the dissector of intended packet encapsulation type */

    /* Get protocol handle, and set p2p_dir where necessary.
       (packet-frame.c won't copy it from pseudo-header because it doesn't
       know about Catapult DCT2000 encap type...)
    */
    switch (encap)
    {
        case WTAP_ENCAP_RAW_IP:
            protocol_handle = find_dissector("ip");
            break;
        case WTAP_ENCAP_ETHERNET:
            protocol_handle = find_dissector("eth_withoutfcs");
            break;
        case WTAP_ENCAP_ISDN:
            protocol_handle = find_dissector("lapd");
            pinfo->p2p_dir = pinfo->pseudo_header->isdn.uton;
            break;
        case WTAP_ENCAP_ATM_PDUS_UNTRUNCATED:
            protocol_handle = find_dissector("atm_untruncated");
            break;
        case WTAP_ENCAP_PPP:
            protocol_handle = find_dissector("ppp_hdlc");
            pinfo->p2p_dir = pinfo->pseudo_header->p2p.sent;
            break;
        case DCT2000_ENCAP_SSCOP:
            protocol_handle = find_dissector("sscop");
            break;
        case WTAP_ENCAP_FRELAY:
            protocol_handle = find_dissector("fr");
            break;
        case DCT2000_ENCAP_MTP2:
            protocol_handle = find_dissector("mtp2");
            break;
        case DCT2000_ENCAP_NBAP:
            protocol_handle = find_dissector("nbap");
            break;
        case DCT2000_ENCAP_UNHANDLED:
            /* Show context.port in src or dest column as appropriate */
            if (check_col(pinfo->cinfo, COL_DEF_SRC) && direction == 0)
            {
                col_add_fstr(pinfo->cinfo, COL_DEF_SRC,
                             "%s.%u",
                             tvb_get_ephemeral_string(tvb, 0, context_length),
                             port_number);
            }
            else
            if (check_col(pinfo->cinfo, COL_DEF_DST) && direction == 1)
            {
                col_add_fstr(pinfo->cinfo, COL_DEF_DST,
                             "%s.%u",
                             tvb_get_ephemeral_string(tvb, 0, context_length),
                             port_number);
            }

            /* Many DCT2000 protocols have at least one IPPrim variant. If the
               protocol names match, try to find the UDP/TCP data inside them and
               pass that offset to dissector
            */
            protocol_handle = 0;

            /* Try IP Prim heuristic if configured to */
            if (!protocol_handle && catapult_dct2000_try_ipprim_heuristic)
            {
                guint32      source_addr_offset = 0, dest_addr_offset = 0;
                guint32      source_port_offset = 0, dest_port_offset = 0;
                port_type    type_of_port = PT_NONE;
                guint16      conn_id_offset = 0;
                int          offset_before_ipprim_header = offset;

                /* Will give up if couldn't match protocol anyway... */
                heur_protocol_handle = look_for_dissector(protocol_name);
                if ((heur_protocol_handle != 0) &&
                    find_ipprim_data_offset(tvb, &offset, direction,
                                            &source_addr_offset, &dest_addr_offset,
                                            &source_port_offset, &dest_port_offset,
                                            &type_of_port,
                                            &conn_id_offset))
                {
                    proto_tree *ipprim_tree;
                    proto_item *ti;
                    protocol_handle = heur_protocol_handle;


                    /* Add address parameters to tree */
                    /* Unfortunately can't automatically create a conversation filter for this...
                       I could create a fake IP header from these details, but then it would be tricky
                       to get FP dissector called as it has no well-known ports or heuristics... */
                    ti =  proto_tree_add_string_format(dct2000_tree, hf_catapult_dct2000_ipprim_addresses,
                                                       tvb, offset_before_ipprim_header, 0,
                                                       "", "IPPrim transport (%s): %s:%u -> %s:%u",
                                                       (type_of_port == PT_UDP) ? "UDP" : "TCP",
                                                       (source_addr_offset) ?
                                                           (char *)get_hostname(tvb_get_ipv4(tvb, source_addr_offset)) :
                                                           "0.0.0.0",
                                                       (source_port_offset) ?
                                                           tvb_get_ntohs(tvb, source_port_offset) :
                                                           0,
                                                       (dest_addr_offset) ?
                                                           (char *)get_hostname(tvb_get_ipv4(tvb, dest_addr_offset)) :
                                                           "0.0.0.0",
                                                       (dest_port_offset) ?
                                                           tvb_get_ntohs(tvb, dest_port_offset) :
                                                           0);
                    if ((type_of_port == PT_TCP) && (conn_id_offset != 0)) {
                        proto_item_append_text(ti, " (conn_id=%u)", tvb_get_ntohs(tvb, conn_id_offset));
                    }

                    /* Add these IPPRIM fields inside an IPPRIM subtree */
                    ipprim_tree = proto_item_add_subtree(ti, ett_catapult_dct2000_ipprim);

                    /* Try to add right stuff to pinfo so conversation stuff works... */
                    pinfo->ptype = type_of_port;
                    switch (type_of_port) {
                        case PT_UDP:
                            pinfo->ipproto = IP_PROTO_UDP;
                            break;
                        case PT_TCP:
                            pinfo->ipproto = IP_PROTO_TCP;
                            break;
                        default:
                            pinfo->ipproto = IP_PROTO_NONE;
                    }

                    /* Add addresses & ports into ipprim tree.
                       Also set address info in pinfo for conversations... */
                    if (source_addr_offset != 0)
                    {
                        proto_item *addr_ti;

                        SET_ADDRESS(&pinfo->net_src, AT_IPv4, 4, (tvb_get_ptr(tvb, source_addr_offset, 4)));
                        SET_ADDRESS(&pinfo->src, AT_IPv4, 4, (tvb_get_ptr(tvb, source_addr_offset, 4)));

                        proto_tree_add_item(ipprim_tree, hf_catapult_dct2000_ipprim_src_addr,
                                            tvb, source_addr_offset, 4, FALSE);
                        addr_ti = proto_tree_add_item(ipprim_tree, hf_catapult_dct2000_ipprim_addr,
                                                      tvb, source_addr_offset, 4, FALSE);
                        PROTO_ITEM_SET_HIDDEN(addr_ti);
                    }
                    if (source_port_offset != 0)
                    {
                        proto_item *port_ti;

                        pinfo->srcport = tvb_get_ntohs(tvb, source_port_offset);

                        proto_tree_add_item(ipprim_tree,
                                            (type_of_port == PT_UDP) ?
                                               hf_catapult_dct2000_ipprim_udp_src_port :
                                               hf_catapult_dct2000_ipprim_tcp_src_port,
                                            tvb, source_port_offset, 2, FALSE);
                        port_ti = proto_tree_add_item(ipprim_tree,
                                                      (type_of_port == PT_UDP) ?
                                                          hf_catapult_dct2000_ipprim_udp_port :
                                                          hf_catapult_dct2000_ipprim_tcp_port,
                                                      tvb, source_port_offset, 2, FALSE);
                        PROTO_ITEM_SET_HIDDEN(port_ti);
                    }
                    if (dest_addr_offset != 0)
                    {
                        proto_item *addr_ti;

                        SET_ADDRESS(&pinfo->net_dst, AT_IPv4, 4, (tvb_get_ptr(tvb, dest_addr_offset, 4)));
                        SET_ADDRESS(&pinfo->dst, AT_IPv4, 4, (tvb_get_ptr(tvb, dest_addr_offset, 4)));
                        proto_tree_add_item(ipprim_tree, hf_catapult_dct2000_ipprim_dst_addr,
                                            tvb, dest_addr_offset, 4, FALSE);
                        addr_ti = proto_tree_add_item(ipprim_tree, hf_catapult_dct2000_ipprim_addr,
                                                      tvb, dest_addr_offset, 4, FALSE);
                        PROTO_ITEM_SET_HIDDEN(addr_ti);
                    }
                    if (dest_port_offset != 0)
                    {
                        proto_item *port_ti;

                        pinfo->destport = tvb_get_ntohs(tvb, dest_port_offset);

                        proto_tree_add_item(ipprim_tree,
                                            (type_of_port == PT_UDP) ?
                                               hf_catapult_dct2000_ipprim_udp_dst_port :
                                               hf_catapult_dct2000_ipprim_tcp_dst_port,
                                            tvb, dest_port_offset, 2, FALSE);
                        port_ti = proto_tree_add_item(ipprim_tree,
                                                      (type_of_port == PT_UDP) ?
                                                          hf_catapult_dct2000_ipprim_udp_port :
                                                          hf_catapult_dct2000_ipprim_tcp_port,
                                                      tvb, dest_port_offset, 2, FALSE);
                        PROTO_ITEM_SET_HIDDEN(port_ti);
                    }
                    if (conn_id_offset != 0)
                    {
                        proto_tree_add_item(ipprim_tree,
                                            hf_catapult_dct2000_ipprim_conn_id,
                                            tvb, conn_id_offset, 2, FALSE);
                    }


                    /* Set source and dest columns now (will be overwriiten if
                       src and dst IP addresses set) */
                    if (source_addr_offset && check_col(pinfo->cinfo, COL_DEF_SRC))
                    {
                        col_append_fstr(pinfo->cinfo, COL_DEF_SRC,
                                        "(%s:%u)",
                                        (char*)get_hostname(tvb_get_ipv4(tvb, source_addr_offset)),
                                        tvb_get_ntohs(tvb, source_port_offset));
                    }
                    if (dest_addr_offset && check_col(pinfo->cinfo, COL_DEF_DST))
                    {
                        col_append_fstr(pinfo->cinfo, COL_DEF_DST,
                                        "(%s:%u)",
                                        (char*)get_hostname(tvb_get_ipv4(tvb, dest_addr_offset)),
                                        tvb_get_ntohs(tvb, dest_port_offset));
                    }

                    /* Set length for IPPrim tree */
                    proto_item_set_len(ipprim_tree, offset - offset_before_ipprim_header);
                }
            }


            /* Try SCTP Prim heuristic if configured to */
            if (!protocol_handle && catapult_dct2000_try_sctpprim_heuristic)
            {
                heur_protocol_handle = look_for_dissector(protocol_name);
                if ((heur_protocol_handle != 0) &&
                    (find_sctpprim_variant1_data_offset(tvb, &offset) ||
                     find_sctpprim_variant3_data_offset(tvb, &offset)))
                {
                    protocol_handle = heur_protocol_handle;
                }
            }

            break;

        default:
            /* !! If get here, there is a mismatch between
               this dissector and the wiretap module catapult_dct2000.c !!
            */
            DISSECTOR_ASSERT_NOT_REACHED();
            return;
    }


    /* Try appropriate dissector, if selected */
    if (protocol_handle != 0)
    {
        /* Dissect the remainder of the frame using chosen protocol handle */
        next_tvb = tvb_new_subset(tvb, offset, -1, tvb_reported_length(tvb)-offset);
        sub_dissector_result = call_dissector_only(protocol_handle, next_tvb, pinfo, tree);
    }


    if (protocol_handle == 0 || sub_dissector_result == 0)
    {
        /* Could get here because:
           - encap is DCT2000_ENCAP_UNHANDLED, OR
           - desired protocol is unavailable (probably disabled), OR
           - protocol rejected our data
           Show remaining bytes as unparsed data */
        proto_tree_add_item(dct2000_tree, hf_catapult_dct2000_unparsed_data,
                            tvb, offset, -1, FALSE);

        if (check_col(pinfo->cinfo, COL_INFO))
        {
            col_add_fstr(pinfo->cinfo, COL_INFO,
                         "Not dissected  (context=%s.%u   t=%s   %c   prot=%s (v=%s))",
                         tvb_get_ephemeral_string(tvb, 0, context_length),
                         port_number,
                         tvb_get_ephemeral_string(tvb, timestamp_start, timestamp_length),
                         (direction == 0) ? 'S' : 'R',
                         tvb_get_ephemeral_string(tvb, protocol_start, protocol_length),
                         tvb_get_ephemeral_string(tvb, variant_start, variant_length));
        }
    }
    else
    {
        /* Show number of dissected bytes */
        proto_item *ti = proto_tree_add_uint(dct2000_tree,
                                             hf_catapult_dct2000_dissected_length,
                                             tvb, 0, 0, tvb_reported_length(tvb)-offset);
        PROTO_ITEM_SET_GENERATED(ti);
    }
}



/******************************************************************************/
/* Associate this protocol with the Catapult DCT2000 file encapsulation type. */
/******************************************************************************/
void proto_reg_handoff_catapult_dct2000(void)
{
    dissector_handle_t catapult_dct2000_handle = find_dissector("dct2000");
    dissector_add("wtap_encap", WTAP_ENCAP_CATAPULT_DCT2000,
                  catapult_dct2000_handle);
}

/****************************************/
/* Register the protocol                */
/****************************************/
void proto_register_catapult_dct2000(void)
{
    static hf_register_info hf[] =
    {
        { &hf_catapult_dct2000_context,
            { "Context",
              "dct2000.context", FT_STRING, BASE_NONE, NULL, 0x0,
              "Context name", HFILL
            }
        },
        { &hf_catapult_dct2000_port_number,
            { "Context Port number",
              "dct2000.context_port", FT_UINT8, BASE_DEC, NULL, 0x0,
              "Context port number", HFILL
            }
        },
        { &hf_catapult_dct2000_timestamp,
            { "Timestamp",
              "dct2000.timestamp", FT_DOUBLE, BASE_DEC, NULL, 0x0,
              "File timestamp", HFILL
            }
        },
        { &hf_catapult_dct2000_protocol,
            { "DCT2000 protocol",
              "dct2000.protocol", FT_STRING, BASE_NONE, NULL, 0x0,
              "Original (DCT2000) protocol name", HFILL
            }
        },
        { &hf_catapult_dct2000_variant,
            { "Protocol variant",
              "dct2000.variant", FT_STRING, BASE_NONE, NULL, 0x0,
              "DCT2000 protocol variant", HFILL
            }
        },
        { &hf_catapult_dct2000_outhdr,
            { "Out-header",
              "dct2000.outhdr", FT_STRING, BASE_NONE, NULL, 0x0,
              "DCT2000 protocol outhdr", HFILL
            }
        },
        { &hf_catapult_dct2000_direction,
            { "Direction",
              "dct2000.direction", FT_UINT8, BASE_DEC, VALS(direction_vals), 0x0,
              "Frame direction (Sent or Received)", HFILL
            }
        },
        { &hf_catapult_dct2000_encap,
            { "Wireshark encapsulation",
              "dct2000.encapsulation", FT_UINT8, BASE_DEC, VALS(encap_vals), 0x0,
              "Wireshark encapsulation used", HFILL
            }
        },
        { &hf_catapult_dct2000_unparsed_data,
            { "Unparsed protocol data",
              "dct2000.unparsed_data", FT_BYTES, BASE_NONE, NULL, 0x0,
              "Unparsed DCT2000 protocol data", HFILL
            }
        },
        { &hf_catapult_dct2000_dissected_length,
            { "Dissected length",
              "dct2000.dissected-length", FT_UINT16, BASE_DEC, NULL, 0x0,
              "Number of bytes dissected by subdissector(s)", HFILL
            }
        },
        { &hf_catapult_dct2000_ipprim_addresses,
            { "IPPrim Addresses",
              "dct2000.ipprim", FT_STRING, BASE_NONE, NULL, 0x0,
              "IPPrim Addresses", HFILL
            }
        },
        { &hf_catapult_dct2000_ipprim_src_addr,
            { "Source Address",
              "dct2000.ipprim.src", FT_IPv4, BASE_NONE, NULL, 0x0,
              "IPPrim Source Address", HFILL
            }
        },
        { &hf_catapult_dct2000_ipprim_dst_addr,
            { "Destination Address",
              "dct2000.ipprim.dst", FT_IPv4, BASE_NONE, NULL, 0x0,
              "IPPrim Destination Address", HFILL
            }
        },
        { &hf_catapult_dct2000_ipprim_addr,
            { "Address",
              "dct2000.ipprim.addr", FT_IPv4, BASE_NONE, NULL, 0x0,
              "IPPrim Destination Address", HFILL
            }
        },
        { &hf_catapult_dct2000_ipprim_udp_src_port,
            { "UDP Source Port",
              "dct2000.ipprim.udp.srcport", FT_UINT16, BASE_DEC, NULL, 0x0,
              "IPPrim UDP Source Port", HFILL
            }
        },
        { &hf_catapult_dct2000_ipprim_udp_dst_port,
            { "UDP Destination Port",
              "dct2000.ipprim.udp.dstport", FT_UINT16, BASE_DEC, NULL, 0x0,
              "IPPrim UDP Destination Port", HFILL
            }
        },
        { &hf_catapult_dct2000_ipprim_udp_port,
            { "UDP Port",
              "dct2000.ipprim.udp.port", FT_UINT16, BASE_DEC, NULL, 0x0,
              "IPPrim UDP Port", HFILL
            }
        },
        { &hf_catapult_dct2000_ipprim_tcp_src_port,
            { "TCP Source Port",
              "dct2000.ipprim.tcp.srcport", FT_UINT16, BASE_DEC, NULL, 0x0,
              "IPPrim TCP Source Port", HFILL
            }
        },
        { &hf_catapult_dct2000_ipprim_tcp_dst_port,
            { "TCP Destination Port",
              "dct2000.ipprim.tcp.dstport", FT_UINT16, BASE_DEC, NULL, 0x0,
              "IPPrim TCP Destination Port", HFILL
            }
        },
        { &hf_catapult_dct2000_ipprim_tcp_port,
            { "TCP Port",
              "dct2000.ipprim.tcp.port", FT_UINT16, BASE_DEC, NULL, 0x0,
              "IPPrim TCP Port", HFILL
            }
        },
        { &hf_catapult_dct2000_ipprim_conn_id,
            { "Conn Id",
              "dct2000.ipprim.conn-id", FT_UINT16, BASE_DEC, NULL, 0x0,
              "IPPrim Connection ID", HFILL
            }
        }
    };

    static gint *ett[] =
    {
        &ett_catapult_dct2000,
        &ett_catapult_dct2000_ipprim
    };

    module_t *catapult_dct2000_module;

    /* Register protocol. */
    proto_catapult_dct2000 = proto_register_protocol("Catapult DCT2000 packet",
                                                     "DCT2000",
                                                     "dct2000");
    proto_register_field_array(proto_catapult_dct2000, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));

    /* Allow dissector to find be found by name. */
    register_dissector("dct2000", dissect_catapult_dct2000, proto_catapult_dct2000);

    /* Preferences */
    catapult_dct2000_module = prefs_register_protocol(proto_catapult_dct2000,
                                                      proto_reg_handoff_catapult_dct2000);

    /* This preference no longer supported (introduces linkage dependency between
       dissectors and wiretap */
    prefs_register_obsolete_preference(catapult_dct2000_module, "board_ports_only");

    /* Determines whether for not-handled protocols we should try to parse it if:
       - it looks like its embedded in an ipprim message, AND
       - the DCT2000 protocol name matches a wireshark dissector name */
    prefs_register_bool_preference(catapult_dct2000_module, "ipprim_heuristic",
                                   "Use IP Primitive heuristic",
                                   "If a payload looks like its embedded in an "
                                   "IP primitive message, and there is a wireshark "
                                   "dissector matching the DCT2000 protocol name, "
                                   "try parsing the payload using that dissector",
                                   &catapult_dct2000_try_ipprim_heuristic);

    /* Determines whether for not-handled protocols we should try to parse it if:
       - it looks like its embedded in an sctpprim message, AND
       - the DCT2000 protocol name matches n wireshark dissector name */
    prefs_register_bool_preference(catapult_dct2000_module, "sctpprim_heuristic",
                                   "Use SCTP Primitive heuristic",
                                   "If a payload looks like its embedded in an "
                                   "SCTP primitive message, and there is a wireshark "
                                   "dissector matching the DCT2000 protocol name, "
                                   "try parsing the payload using that dissector",
                                   &catapult_dct2000_try_sctpprim_heuristic);
}

