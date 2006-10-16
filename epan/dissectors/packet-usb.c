/* Man this is suboptimal.
 * The USB Header and the setup data are LITTLE ENDIAN
 * but all the real usb data is BIG ENDIAN.
 */

/* packet-usb.c
 *
 * $Id$
 *
 * usb basic dissector
 * By Paolo Abeni <paolo.abeni@email.com>
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
#include <epan/packet.h>
#include <epan/prefs.h>
#include <epan/etypes.h>
#include <epan/addr_resolv.h>
#include <epan/emem.h>
#include <epan/conversation.h>
#include <string.h>

/* protocols and header fields */
static int proto_usb = -1;
static int hf_usb_urb_type = -1;
static int hf_usb_device_address = -1;
static int hf_usb_endpoint_number = -1;
static int hf_usb_src_endpoint_number = -1;
static int hf_usb_dst_endpoint_number = -1;
static int hf_usb_request = -1;
static int hf_usb_value = -1;
static int hf_usb_index = -1;
static int hf_usb_length = -1;
static int hf_usb_data = -1;
static int hf_usb_setup_bmRequestType = -1;
static int hf_usb_setup_bmRequestType_direction = -1;
static int hf_usb_setup_bmRequestType_type = -1;
static int hf_usb_setup_bmRequestType_recipient = -1;
static int hf_usb_bDescriptorType = -1;
static int hf_usb_descriptor_index = -1;
static int hf_usb_language_id = -1;
static int hf_usb_bLength = -1;
static int hf_usb_bcdUSB = -1;
static int hf_usb_bDeviceClass = -1;
static int hf_usb_bDeviceSubClass = -1;
static int hf_usb_bDeviceProtocol = -1;
static int hf_usb_bMaxPacketSize0 = -1;
static int hf_usb_idVendor = -1;
static int hf_usb_idProduct = -1;
static int hf_usb_bcdDevice = -1;
static int hf_usb_iManufacturer = -1;
static int hf_usb_iProduct = -1;
static int hf_usb_iSerialNumber = -1;
static int hf_usb_bNumConfigurations = -1;
static int hf_usb_wLANGID = -1;
static int hf_usb_bString = -1;

static gint usb_hdr = -1;
static gint usb_setup_hdr = -1;
static gint ett_usb_setup_bmrequesttype = -1;
static gint ett_descriptor_device = -1;


/* there is one such structure for each device/endpoint conversation */
typedef struct _usb_conv_info_t {
    emem_tree_t *transactions;
} usb_conv_info_t;

/* there is one such structure for each request/response */
typedef struct _usb_trans_info_t {
    guint32 request_in;
    guint32 response_in;
    guint8 requesttype;
    guint8 request;
    union {
        struct {
            guint8 type;
            guint8 index;
        } get_descriptor;
    };
} usb_trans_info_t;

typedef enum { 
  URB_CONTROL_INPUT,
  URB_CONTROL_OUTPUT,
  URB_ISOCHRONOUS_INPUT,
  URB_ISOCHRONOUS_OUTPUT,
  URB_INTERRUPT_INPUT,
  URB_INTERRUPT_OUTPUT,
  URB_BULK_INPUT,
  URB_BULK_OUTPUT,
  URB_UNKNOWN
} urb_type_t;

typedef struct usb_header {
  guint32 urb_type;  
  guint32 device_address;
  guint32 endpoint_number;
  guint32 setup_packet;
} usb_header_t;

typedef struct usb_setup {
  guint8 bmRequestType;
  guint8 bRequest;
  guint16 wValue;
  guint16 wIndex;
  guint16 wLength;
} usb_setup_t;


static const value_string usb_langid_vals[] = {
    {0x0000,	"no language specified"},
    {0x0409,	"English (United States)"},
    {0, NULL}
};

static const value_string usb_urb_type_vals[] = {
    {URB_CONTROL_INPUT, "URB_CONTROL_INPUT"},
    {URB_CONTROL_OUTPUT,"URB_CONTROL_OUTPUT"},
    {URB_ISOCHRONOUS_INPUT,"URB_ISOCHRONOUS_INPUT"},
    {URB_ISOCHRONOUS_OUTPUT,"URB_ISOCHRONOUS_OUTPUT"},
    {URB_INTERRUPT_INPUT,"URB_INTERRUPT_INPUT"},
    {URB_INTERRUPT_OUTPUT,"URB_INTERRUPT_OUTPUT"},
    {URB_BULK_INPUT,"URB_BULK_INPUT"},
    {URB_BULK_OUTPUT,"URB_BULK_OUTPUT"},
    {URB_UNKNOWN, "URB_UNKNOWN"},
    {0, NULL}
};

#define USB_DT_DEVICE		1
#define USB_DT_CONFIGURATION	2
#define USB_DT_STRING		3
#define USB_DT_INTERFACE	4
#define USB_DT_ENDPOINT		5
#define USB_DT_DEVICE_QUALIFIER	6
#define USB_DT_OTHER_SPEED_CONFIGURATION	7
#define USB_DT_INTERFACE_POWER	8
static const value_string descriptor_type_vals[] = {
    {USB_DT_DEVICE,			"DEVICE"},
    {USB_DT_CONFIGURATION,		"CONFIGURATION"},
    {USB_DT_STRING,			"STRING"},
    {USB_DT_INTERFACE,			"INTERFACE"},
    {USB_DT_ENDPOINT,			"ENDPOINT"},
    {USB_DT_DEVICE_QUALIFIER,		"DEVICE_QUALIFIER"},
    {USB_DT_OTHER_SPEED_CONFIGURATION,	"OTHER_SPEED_CONFIGURATION"},
    {USB_DT_INTERFACE_POWER,		"INTERFACE_POWER"},
    {0,NULL}
};


static conversation_t *
get_usb_conversation(packet_info *pinfo)
{
    conversation_t *conversation;

    /*
     * Do we have a conversation for this connection?
     */
    conversation = find_conversation(pinfo->fd->num, 
                               &pinfo->src, &pinfo->dst,
                               pinfo->ptype, 
                               pinfo->srcport, pinfo->destport, 0);
    if(conversation){
        return conversation;
    }

    /* We don't yet have a conversation, so create one. */
    conversation = conversation_new(pinfo->fd->num, 
                           &pinfo->src, &pinfo->dst,
                           pinfo->ptype,
                           pinfo->srcport, pinfo->destport, 0);
    return conversation;
}



/* SETUP dissectors */


/*
 * This dissector is used to dissect the setup part and the data
 * for URB_CONTROL_INPUT / GET DESCRIPTOR
 */


/* 9.6.2 */
static int
dissect_usb_device_qualifier_descriptor(packet_info *pinfo _U_, proto_tree *parent_tree, tvbuff_t *tvb, int offset, usb_trans_info_t *usb_trans_info _U_)
{
    proto_item *item=NULL;
    proto_tree *tree=NULL;
    int old_offset=offset;

    if(parent_tree){
        item=proto_tree_add_text(parent_tree, tvb, offset, 0, "DEVICE QUALIFIER DESCRIPTOR");
	tree=proto_item_add_subtree(item, ett_descriptor_device);
    }

    /* bLength */
    proto_tree_add_item(tree, hf_usb_bLength, tvb, offset, 1, TRUE);
    offset++;

    /* bDescriptorType */
    proto_tree_add_item(tree, hf_usb_bDescriptorType, tvb, offset, 1, TRUE);
    offset++;

    /* bcdUSB */
    proto_tree_add_item(tree, hf_usb_bcdUSB, tvb, offset, 2, TRUE);
    offset+=2;

    /* bDeviceClass */
    proto_tree_add_item(tree, hf_usb_bDeviceClass, tvb, offset, 1, TRUE);
    offset++;

    /* bDeviceSubClass */
    proto_tree_add_item(tree, hf_usb_bDeviceSubClass, tvb, offset, 1, TRUE);
    offset++;

    /* bDeviceProtocol */
    proto_tree_add_item(tree, hf_usb_bDeviceProtocol, tvb, offset, 1, TRUE);
    offset++;

    /* bMaxPacketSize0 */
    proto_tree_add_item(tree, hf_usb_bMaxPacketSize0, tvb, offset, 1, TRUE);
    offset++;

    /* bNumConfigurations */
    proto_tree_add_item(tree, hf_usb_bNumConfigurations, tvb, offset, 1, TRUE);
    offset++;

    /* one reserved byte */
    offset++;

    if(item){
        proto_item_set_len(item, offset-old_offset);
    }

    return offset;
}

/* 9.6.1 */
static int
dissect_usb_device_descriptor(packet_info *pinfo _U_, proto_tree *parent_tree, tvbuff_t *tvb, int offset, usb_trans_info_t *usb_trans_info _U_)
{
    proto_item *item=NULL;
    proto_tree *tree=NULL;
    int old_offset=offset;

    if(parent_tree){
        item=proto_tree_add_text(parent_tree, tvb, offset, 0, "DEVICE DESCRIPTOR");
	tree=proto_item_add_subtree(item, ett_descriptor_device);
    }

    /* bLength */
    proto_tree_add_item(tree, hf_usb_bLength, tvb, offset, 1, TRUE);
    offset++;

    /* bDescriptorType */
    proto_tree_add_item(tree, hf_usb_bDescriptorType, tvb, offset, 1, TRUE);
    offset++;

    /* bcdUSB */
    proto_tree_add_item(tree, hf_usb_bcdUSB, tvb, offset, 2, TRUE);
    offset+=2;

    /* bDeviceClass */
    proto_tree_add_item(tree, hf_usb_bDeviceClass, tvb, offset, 1, TRUE);
    offset++;

    /* bDeviceSubClass */
    proto_tree_add_item(tree, hf_usb_bDeviceSubClass, tvb, offset, 1, TRUE);
    offset++;

    /* bDeviceProtocol */
    proto_tree_add_item(tree, hf_usb_bDeviceProtocol, tvb, offset, 1, TRUE);
    offset++;

    /* bMaxPacketSize0 */
    proto_tree_add_item(tree, hf_usb_bMaxPacketSize0, tvb, offset, 1, TRUE);
    offset++;

    /* idVendor */
    proto_tree_add_item(tree, hf_usb_idVendor, tvb, offset, 2, TRUE);
    offset+=2;

    /* idProduct */
    proto_tree_add_item(tree, hf_usb_idProduct, tvb, offset, 2, TRUE);
    offset+=2;

    /* bcdDevice */
    proto_tree_add_item(tree, hf_usb_bcdDevice, tvb, offset, 2, TRUE);
    offset+=2;

    /* iManufacturer */
    proto_tree_add_item(tree, hf_usb_iManufacturer, tvb, offset, 1, TRUE);
    offset++;

    /* iProduct */
    proto_tree_add_item(tree, hf_usb_iProduct, tvb, offset, 1, TRUE);
    offset++;

    /* iSerialNumber */
    proto_tree_add_item(tree, hf_usb_iSerialNumber, tvb, offset, 1, TRUE);
    offset++;

    /* bNumConfigurations */
    proto_tree_add_item(tree, hf_usb_bNumConfigurations, tvb, offset, 1, TRUE);
    offset++;

    if(item){
        proto_item_set_len(item, offset-old_offset);
    }

    return offset;
}

/* 9.6.7 */
static int
dissect_usb_string_descriptor(packet_info *pinfo _U_, proto_tree *parent_tree, tvbuff_t *tvb, int offset, usb_trans_info_t *usb_trans_info)
{
    proto_item *item=NULL;
    proto_tree *tree=NULL;
    int old_offset=offset;
    guint8 len;

    if(parent_tree){
        item=proto_tree_add_text(parent_tree, tvb, offset, 0, "STRING DESCRIPTOR");
	tree=proto_item_add_subtree(item, ett_descriptor_device);
    }


    /* bLength */
    proto_tree_add_item(tree, hf_usb_bLength, tvb, offset, 1, TRUE);
    len=tvb_get_guint8(tvb, offset);
    offset++;


    /* bDescriptorType */
    proto_tree_add_item(tree, hf_usb_bDescriptorType, tvb, offset, 1, TRUE);
    offset++;

    if(!usb_trans_info->get_descriptor.index){
        /* list of languanges */
        while(len>(offset-old_offset)){
            /* wLANGID */
            proto_tree_add_item(tree, hf_usb_wLANGID, tvb, offset, 2, TRUE);
            offset+=2;
        }
    } else {
        char *str;        

        /* unicode string */
        str=tvb_get_ephemeral_faked_unicode(tvb, offset, (len-2)/2, TRUE);
        proto_tree_add_string(tree, hf_usb_bString, tvb, offset, len-2, str);
    }

    if(item){
        proto_item_set_len(item, offset-old_offset);
    }

    return offset;
}


static void
dissect_usb_setup_get_descriptor(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset, gboolean is_request, usb_trans_info_t *usb_trans_info)
{
    if(is_request){
        /* descriptor type */
        proto_tree_add_item(tree, hf_usb_bDescriptorType, tvb, offset, 1, FALSE);
        usb_trans_info->get_descriptor.type=tvb_get_guint8(tvb, offset);
        offset++;
        if (check_col(pinfo->cinfo, COL_INFO)) {
            col_append_fstr(pinfo->cinfo, COL_INFO, " %s",
                val_to_str(usb_trans_info->get_descriptor.type, descriptor_type_vals, "Unknown type %x"));
        }

        /* descriptor index */
        proto_tree_add_item(tree, hf_usb_descriptor_index, tvb, offset, 1, FALSE);
        usb_trans_info->get_descriptor.index=tvb_get_guint8(tvb, offset);
        offset++;

        /* language id */
        proto_tree_add_item(tree, hf_usb_language_id, tvb, offset, 2, FALSE);
        offset+=2;

        /* length */
        proto_tree_add_item(tree, hf_usb_length, tvb, offset, 2, FALSE);
        offset += 2;
    } else {
        if (check_col(pinfo->cinfo, COL_INFO)) {
            col_append_fstr(pinfo->cinfo, COL_INFO, " %s",
                val_to_str(usb_trans_info->get_descriptor.type, descriptor_type_vals, "Unknown type %x"));
        }
        switch(usb_trans_info->get_descriptor.type){
        case USB_DT_DEVICE:
            offset=dissect_usb_device_descriptor(pinfo, tree, tvb, offset, usb_trans_info);
            break;
        case USB_DT_STRING: 
            offset=dissect_usb_string_descriptor(pinfo, tree, tvb, offset, usb_trans_info);
            break;
        case USB_DT_DEVICE_QUALIFIER:
            offset=dissect_usb_device_qualifier_descriptor(pinfo, tree, tvb, offset, usb_trans_info);
            break;
        default:
            /* XXX dissect the descriptor coming back from the device */
            proto_tree_add_text(tree, tvb, offset, tvb_length_remaining(tvb, offset), "get descriptor  data...");
        }
    }
}




typedef void (*usb_setup_dissector)(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset, gboolean is_request, usb_trans_info_t *usb_trans_info);

typedef struct _usb_setup_dissector_table_t {
    guint8 request;
    usb_setup_dissector dissector;
} usb_setup_dissector_table_t;
#define USB_SETUP_GET_DESCRIPTOR	6
static const usb_setup_dissector_table_t setup_dissectors[] = {
    {USB_SETUP_GET_DESCRIPTOR,	dissect_usb_setup_get_descriptor},
    {0, NULL}
};  
static const value_string setup_request_names_vals[] = {
    {USB_SETUP_GET_DESCRIPTOR,		"GET DESCRIPTOR"},
    {0, NULL}
};  








static const true_false_string tfs_bmrequesttype_direction = {
	"Device-to-host",
	"Host-to-device"
};
static const value_string bmrequesttype_type_vals[] = {
    {0, "Standard"},
    {1, "Class"},
    {2, "Vendor"},
    {3, "Reserved"},
    {0, NULL}
};
static const value_string bmrequesttype_recipient_vals[] = {
    {0, "Device"},
    {1, "Interface"},
    {2, "Endpoint"},
    {3, "Other"},
    {0, NULL}
};

static int
dissect_usb_setup_bmrequesttype(proto_tree *parent_tree, tvbuff_t *tvb, int offset)
{
	proto_item *item=NULL;
	proto_tree *tree=NULL;

	if(parent_tree){
	        item=proto_tree_add_item(parent_tree, hf_usb_setup_bmRequestType, tvb, offset, 1, TRUE);
		tree = proto_item_add_subtree(item, ett_usb_setup_bmrequesttype);
	}

	proto_tree_add_item(tree, hf_usb_setup_bmRequestType_direction, tvb, offset, 1, TRUE);
	proto_tree_add_item(tree, hf_usb_setup_bmRequestType_type, tvb, offset, 1, TRUE);
	proto_tree_add_item(tree, hf_usb_setup_bmRequestType_recipient, tvb, offset, 1, TRUE);

	offset++;
	return offset;
}





static void
dissect_usb(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent)
{
    int offset = 0;
    int type, endpoint;
    gboolean setup;
    proto_tree *tree = NULL;
    static guint32 src_addr, dst_addr, tmp_addr; /* has to be static due to SET_ADDRESS */
    guint32 src_port, dst_port;
    gboolean is_request;
    usb_conv_info_t *usb_conv_info;
    conversation_t *conversation;
    
    if (check_col(pinfo->cinfo, COL_PROTOCOL))
        col_set_str(pinfo->cinfo, COL_PROTOCOL, "USB");

    /* add usb hdr*/    
    if (parent) {
      proto_item *ti = NULL;
      ti = proto_tree_add_protocol_format(parent, proto_usb, tvb, 0, sizeof(usb_header_t), "USB URB");

      tree = proto_item_add_subtree(ti, usb_hdr);
    }

    
    type = tvb_get_ntohl(tvb, offset);
    proto_tree_add_item(tree, hf_usb_urb_type, tvb, offset, 4, FALSE);
    offset += 4;
    if (check_col(pinfo->cinfo, COL_INFO)) {
        col_append_fstr(pinfo->cinfo, COL_INFO, "%s",
            val_to_str(type, usb_urb_type_vals, "Unknown type %x"));
    }

#define USB_ADDR_LEN 4
    proto_tree_add_item(tree, hf_usb_device_address, tvb, offset, 4, FALSE);
    tmp_addr=tvb_get_ntohl(tvb, offset);
    offset += 4;

    proto_tree_add_item(tree, hf_usb_endpoint_number, tvb, offset, 4, FALSE);
    endpoint=tvb_get_ntohl(tvb, offset);
    offset += 4;

    /* check for setup hdr presence */
    setup = tvb_get_ntohl(tvb, offset);
    offset += 4;


    /* set up addresses and ports */
    switch(type){
    case URB_CONTROL_INPUT:
        /* CONTROL INPUT packets are requests if they contain a "setup"
         * blob and responses othervise
         */
        if(setup){
            src_addr=0xffffffff;
            dst_addr=tmp_addr;
            src_port=0xffff;
            dst_port=endpoint;
            is_request=TRUE;
        } else {
            src_addr=tmp_addr;
            dst_addr=0xffffffff;
            src_port=endpoint;
            dst_port=0xffff;
            is_request=FALSE;
        }
        break;
    default:
        /* dont know */
        src_addr=0xffffffff;
        dst_addr=0xffffffff;
        src_port=0xffff;
        dst_port=0xffff;
        is_request=FALSE;
    }
    SET_ADDRESS(&pinfo->net_src, AT_USB, USB_ADDR_LEN, (char *)&src_addr);
    SET_ADDRESS(&pinfo->src, AT_USB, USB_ADDR_LEN, (char *)&src_addr);
    SET_ADDRESS(&pinfo->net_dst, AT_USB, USB_ADDR_LEN, (char *)&dst_addr);
    SET_ADDRESS(&pinfo->dst, AT_USB, USB_ADDR_LEN, (char *)&dst_addr);
    pinfo->ptype=PT_USB;
    pinfo->srcport=src_port;
    pinfo->destport=dst_port;


    conversation=get_usb_conversation(pinfo);

    /* do we have conversation specific data ? */
    usb_conv_info = conversation_get_proto_data(conversation, proto_usb);
    if(!usb_conv_info){
        /* no not yet so create some */
        usb_conv_info = se_alloc(sizeof(usb_conv_info_t));
        usb_conv_info->transactions=se_tree_create_non_persistent(EMEM_TREE_TYPE_RED_BLACK, "usb transactions");

        conversation_add_proto_data(conversation, proto_usb, usb_conv_info);
    }
   




    switch(type){
    case URB_CONTROL_INPUT:
        {
        const usb_setup_dissector_table_t *tmp;
        usb_setup_dissector dissector;
        proto_item *ti = NULL;
        proto_tree *setup_tree = NULL;
        guint8 requesttype, request;
        usb_trans_info_t *usb_trans_info;

        if(is_request){
            /* this is a request */
            ti = proto_tree_add_protocol_format(tree, proto_usb, tvb, offset, sizeof(usb_setup_t), "URB setup");
            setup_tree = proto_item_add_subtree(ti, usb_setup_hdr);
            requesttype=tvb_get_guint8(tvb, offset);        
            offset=dissect_usb_setup_bmrequesttype(setup_tree, tvb, offset);

            request=tvb_get_guint8(tvb, offset);
            proto_tree_add_item(setup_tree, hf_usb_request, tvb, offset, 1, TRUE);
            offset += 1;

            usb_trans_info=se_tree_lookup32(usb_conv_info->transactions, pinfo->fd->num);
            if(!usb_trans_info){
                usb_trans_info=se_alloc(sizeof(usb_trans_info_t));
                usb_trans_info->request_in=pinfo->fd->num;
                usb_trans_info->response_in=0;
                usb_trans_info->requesttype=requesttype;
                usb_trans_info->request=request;
                se_tree_insert32(usb_conv_info->transactions, pinfo->fd->num, usb_trans_info);
            }

            if (check_col(pinfo->cinfo, COL_INFO)) {
                col_clear(pinfo->cinfo, COL_INFO);
                col_append_fstr(pinfo->cinfo, COL_INFO, "%s Request",
                    val_to_str(usb_trans_info->request, setup_request_names_vals, "Unknown type %x"));
            }

            dissector=NULL;
            for(tmp=setup_dissectors;tmp->dissector;tmp++){
                if(tmp->request==request){
                    dissector=tmp->dissector;
                    break;
                }
            }
  
            if(dissector){
                dissector(pinfo, setup_tree, tvb, offset, is_request, usb_trans_info);
                offset+=6;
            } else {
                proto_tree_add_item(setup_tree, hf_usb_value, tvb, offset, 2, TRUE);
                offset += 2;
                proto_tree_add_item(setup_tree, hf_usb_index, tvb, offset, 2, TRUE);
                offset += 2;
                proto_tree_add_item(setup_tree, hf_usb_length, tvb, offset, 2, TRUE);
                offset += 2;
            }
        } else {
            /* this is a response */
            if(pinfo->fd->flags.visited){
                usb_trans_info=se_tree_lookup32(usb_conv_info->transactions, pinfo->fd->num);
            } else {
                usb_trans_info=se_tree_lookup32_le(usb_conv_info->transactions, pinfo->fd->num);
                if(usb_trans_info){
                    usb_trans_info->response_in=pinfo->fd->num;
                    se_tree_insert32(usb_conv_info->transactions, pinfo->fd->num, usb_trans_info);
                }
            }
            if(usb_trans_info){
                dissector=NULL;
                for(tmp=setup_dissectors;tmp->dissector;tmp++){
                    if(tmp->request==usb_trans_info->request){
                        dissector=tmp->dissector;
                        break;
                    }
                }
  
                if (check_col(pinfo->cinfo, COL_INFO)) {
                    col_clear(pinfo->cinfo, COL_INFO);
                    col_append_fstr(pinfo->cinfo, COL_INFO, "%s Response",
                        val_to_str(usb_trans_info->request, setup_request_names_vals, "Unknown type %x"));
                }

                if(dissector){
                    dissector(pinfo, tree, tvb, offset, is_request, usb_trans_info);
                }
            } else {
                /* could not find a matching request */
            }
        }
        return;
        }
        break;
    default:
        /* dont know */
	;
    }


    if (setup) {
        proto_item *ti = NULL;
        proto_tree *setup_tree = NULL;
        guint8 requesttype, request;

        ti = proto_tree_add_protocol_format(tree, proto_usb, tvb, offset, sizeof(usb_setup_t), "URB setup");
        setup_tree = proto_item_add_subtree(ti, usb_setup_hdr);


        requesttype=tvb_get_guint8(tvb, offset);        
	offset=dissect_usb_setup_bmrequesttype(setup_tree, tvb, offset);

        request=tvb_get_guint8(tvb, offset);
        proto_tree_add_item(setup_tree, hf_usb_request, tvb, offset, 1, TRUE);
        offset += 1;

        proto_tree_add_item(tree, hf_usb_value, tvb, offset, 2, TRUE);
        offset += 2;
        proto_tree_add_item(tree, hf_usb_index, tvb, offset, 2, TRUE);
        offset += 2;
        proto_tree_add_item(tree, hf_usb_length, tvb, offset, 2, TRUE);
        offset += 2;
    }
    
    proto_tree_add_item(tree, hf_usb_data, tvb,
        offset, tvb_length_remaining(tvb, offset), TRUE);
}

void
proto_register_usb(void)
{
    static hf_register_info hf[] = {
    
        { &hf_usb_urb_type,
        { "URB type", "usb.urb_type", FT_UINT32, BASE_DEC, 
                VALS(usb_urb_type_vals), 0x0,
                "URB type", HFILL }},

        { &hf_usb_device_address,
        { "Device", "usb.device_address", FT_UINT32, BASE_DEC, NULL, 0x0,
                "USB device address", HFILL }},

        { &hf_usb_endpoint_number,
        { "Endpoint", "usb.endpoint_number", FT_UINT32, BASE_HEX, NULL, 0x0,
                "usb endpoint number", HFILL }},

        { &hf_usb_src_endpoint_number,
        { "Src Endpoint", "usb.src.endpoint", FT_UINT32, BASE_HEX, NULL, 0x0,
                "src usb endpoint number", HFILL }},

        { &hf_usb_dst_endpoint_number,
        { "Dst Endpoint", "usb.dst.endpoint", FT_UINT32, BASE_HEX, NULL, 0x0,
                "dst usb endpoint number", HFILL }},

        { &hf_usb_setup_bmRequestType,
        { "bmRequestType", "usb.setup.bmRequestType", FT_UINT8, BASE_HEX, NULL, 0x0,
                "", HFILL }},

        { &hf_usb_request,
        { "bRequest", "usb.setup.bRequest", FT_UINT8, BASE_HEX, VALS(setup_request_names_vals), 0x0,
                "", HFILL }},

        { &hf_usb_value,
        { "wValue", "usb.setup.wValue", FT_UINT16, BASE_HEX, NULL, 0x0,
                "", HFILL }},

        { &hf_usb_index,
        { "wIndex", "usb.setup.wIndex", FT_UINT16, BASE_DEC, NULL, 0x0,
                "", HFILL }},

        { &hf_usb_length,
        { "wLength", "usb.setup.wLength", FT_UINT16, BASE_DEC, NULL, 0x0,
                "", HFILL }},
                
        { &hf_usb_data,
        {"Application Data", "usb.data",
            FT_BYTES, BASE_HEX, NULL, 0x0,
            "Payload is application data", HFILL }},
    
        { &hf_usb_setup_bmRequestType_direction,
        { "Direction", "usb.setup.bmRequestType.direction", FT_BOOLEAN, 8, 
          TFS(&tfs_bmrequesttype_direction), 0x80, "", HFILL }},

        { &hf_usb_setup_bmRequestType_type,
        { "Type", "usb.setup.bmRequestType.type", FT_UINT8, BASE_HEX, 
          VALS(bmrequesttype_type_vals), 0x70, "", HFILL }},

        { &hf_usb_setup_bmRequestType_recipient,
        { "Recipient", "usb.setup.bmRequestType.recipient", FT_UINT8, BASE_HEX, 
          VALS(bmrequesttype_recipient_vals), 0x0f, "", HFILL }},

        { &hf_usb_bDescriptorType,
        { "bDescriptorType", "usb.bDescriptorType", FT_UINT8, BASE_HEX, 
          VALS(descriptor_type_vals), 0x0, "", HFILL }},

        { &hf_usb_descriptor_index,
        { "Descriptor Index", "usb.DescriptorIndex", FT_UINT8, BASE_HEX, 
          NULL, 0x0, "", HFILL }},

        { &hf_usb_language_id,
        { "Language Id", "usb.LanguageId", FT_UINT16, BASE_HEX, 
          VALS(usb_langid_vals), 0x0, "", HFILL }},

        { &hf_usb_bLength,
        { "bLength", "usb.bLength", FT_UINT8, BASE_DEC, 
          NULL, 0x0, "", HFILL }},

        { &hf_usb_bcdUSB,
        { "bcdUSB", "usb.bcdUSB", FT_UINT16, BASE_HEX, 
          NULL, 0x0, "", HFILL }},

        { &hf_usb_bDeviceClass,
        { "bDeviceClass", "usb.bDeviceClass", FT_UINT8, BASE_DEC, 
          NULL, 0x0, "", HFILL }},

        { &hf_usb_bDeviceSubClass,
        { "bDeviceSubClass", "usb.bDeviceSubClass", FT_UINT8, BASE_DEC, 
          NULL, 0x0, "", HFILL }},

        { &hf_usb_bDeviceProtocol,
        { "bDeviceProtocol", "usb.bDeviceProtocol", FT_UINT8, BASE_DEC, 
          NULL, 0x0, "", HFILL }},

        { &hf_usb_bMaxPacketSize0,
        { "bMaxPacketSize0", "usb.bMaxPacketSize0", FT_UINT8, BASE_DEC, 
          NULL, 0x0, "", HFILL }},

        { &hf_usb_idVendor,
        { "idVendor", "usb.idVendor", FT_UINT16, BASE_HEX, 
          NULL, 0x0, "", HFILL }},

        { &hf_usb_idProduct,
        { "idProduct", "usb.idProduct", FT_UINT16, BASE_HEX, 
          NULL, 0x0, "", HFILL }},

        { &hf_usb_bcdDevice,
        { "bcdDevice", "usb.bcdDevice", FT_UINT16, BASE_HEX, 
          NULL, 0x0, "", HFILL }},

        { &hf_usb_iManufacturer,
        { "iManufacturer", "usb.iManufacturer", FT_UINT8, BASE_DEC, 
          NULL, 0x0, "", HFILL }},

        { &hf_usb_iProduct,
        { "iProduct", "usb.iProduct", FT_UINT8, BASE_DEC, 
          NULL, 0x0, "", HFILL }},

        { &hf_usb_iSerialNumber,
        { "iSerialNumber", "usb.iSerialNumber", FT_UINT8, BASE_DEC, 
          NULL, 0x0, "", HFILL }},

        { &hf_usb_bNumConfigurations,
        { "bNumConfigurations", "usb.bNumConfigurations", FT_UINT8, BASE_DEC, 
          NULL, 0x0, "", HFILL }},

        { &hf_usb_wLANGID,
        { "wLANGID", "usb.wLANGID", FT_UINT16, BASE_HEX, 
          VALS(usb_langid_vals), 0x0, "", HFILL }},

        { &hf_usb_bString,
        { "bString", "usb.bString", FT_STRING, BASE_NONE, 
          NULL, 0x0, "", HFILL }},

    };
    
    static gint *usb_subtrees[] = {
            &usb_hdr,
            &usb_setup_hdr,
            &ett_usb_setup_bmrequesttype,
            &ett_descriptor_device
    };

     
    proto_usb = proto_register_protocol("USB", "USB", "usb");
    proto_register_field_array(proto_usb, hf, array_length(hf));
    proto_register_subtree_array(usb_subtrees, array_length(usb_subtrees));

    register_dissector("usb", dissect_usb, proto_usb);
}

void
proto_reg_handoff_usb(void)
{
    dissector_handle_t usb_handle;
    usb_handle = create_dissector_handle(dissect_usb, proto_usb);

    dissector_add("wtap_encap", WTAP_ENCAP_USB, usb_handle);
}
