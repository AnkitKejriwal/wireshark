/* packet-t38.h
 *
 * Routines for T38 dissection
 * 2003 Hans Viens
 * 2004 Alejandro Vaquero, add support to conversation
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

#define MAX_T38_DATA_ITEMS 4
#define MAX_T38_DESC 128

typedef struct _t38_packet_info {
	guint16 seq_num;	/* UDPTLPacket sequence number */
	guint32 type_msg;	/* 0=t30-indicator    1=data */
	guint32 t30ind_value;
	guint32 data_value;	/* standard and speed */
	guint32 setup_frame_number;
	guint32 Data_Field_field_type_value;
	guint8	t30_Facsimile_Control;
	gchar   desc[MAX_T38_DESC]; /* Description used to be displayed in the frame label Graph Anlaysis */
	gchar   desc_comment[MAX_T38_DESC]; /* Description used to be displayed in the Comment Graph Anlaysis */
	double time_first_t4_data; 
	guint32 frame_num_first_t4_data;
} t38_packet_info;


#define MAX_T38_SETUP_METHOD_SIZE 7


/* Info to save the State to reassemble Data (e.g. HDLC) and the Setup (e.g. SDP) in T38 conversations */
typedef struct _t38_conv_info
{
	guint32 reass_ID;
	int reass_start_seqnum;
	guint32 reass_data_type;
	gint32 last_seqnum; /* used to avoid duplicated seq num shown in the Graph Analysis */
	guint32 packet_lost;
	guint32 burst_lost;
	double time_first_t4_data; 
} t38_conv_info;

/* Info to save the State to reassemble Data (e.g. HDLC) and the Setup (e.g. SDP) in T38 conversations */
typedef struct _t38_conv
{
	gchar   setup_method[MAX_T38_SETUP_METHOD_SIZE + 1];
	guint32 setup_frame_number;
	t38_conv_info src_t38_info;
	t38_conv_info dst_t38_info;
} t38_conv;

/* Add an T38 conversation with the given details */
void t38_add_address(packet_info *pinfo,
                     address *addr, int port,
                     int other_port,
                     const gchar *setup_method, guint32 setup_frame_number);

ETH_VAR_IMPORT const value_string t30_indicator_vals[];
ETH_VAR_IMPORT const value_string t30_facsimile_control_field_vals[];
ETH_VAR_IMPORT const value_string t30_facsimile_control_field_vals_short[];
ETH_VAR_IMPORT const value_string t30_data_vals[];



