/* DO NOT EDIT
 * This dissector is autogenerated
 */
/* packet-dcerpc-butc.h
 * Routines for BUTC packet disassembly
 *
 * $Id: packet-dcerpc-butc.h 11410 2004-07-18 18:06:47Z gram $
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@ethereal.com>
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

#ifndef __PACKET_DCERPC_BUTC_H
#define __PACKET_DCERPC_BUTC_H



/* INCLUDED FILE : ETH_HDR */
#define TC_DEFAULT_STACK_SIZE		153600
#define TC_MAXGENNAMELEN		512
#define TC_MAXDUMPPATH		256
#define TC_MAXNAMELEN		128
#define TC_MAXFORMATLEN		100
#define TC_MAXHOSTLEN		128
#define TC_MAXTAPELEN		256
#define TC_STAT_DONE		1
#define TC_STAT_OPRWAIT		2
#define TC_STAT_DUMP		4
#define TC_STAT_ABORTED		8
#define TC_STAT_ERROR		16
#define TSK_STAT_FIRST		0x1
#define TSK_STAT_END		0x2
#define TSK_STAT_NOTFOUND		0x4
#define TCOP_NONE		0
#define TCOP_READLABEL		1
#define TCOP_LABELTAPE		2
#define TCOP_DUMP		3
#define TCOP_RESTORE		4
#define TCOP_SCANTAPE		5
#define TCOP_SAVEDB		6
#define TCOP_RESTOREDB		7
#define TCOP_STATUS		8
#define TCOP_SPARE		9
int butc_dissect_Restore_flags(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep, int hf_index, guint32 param);
int butc_dissect_afsNetAddr(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep, int hf_index, guint32 param);
int butc_dissect_tc_dumpDesc(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep, int hf_index, guint32 param);
int butc_dissect_tc_restoreDesc(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep, int hf_index, guint32 param);
int butc_dissect_tc_dumpStat(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep, int hf_index, guint32 param);
int butc_dissect_tc_tapeLabel(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep, int hf_index, guint32 param);
int butc_dissect_tc_tapeSet(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep, int hf_index, guint32 param);
int butc_dissect_tc_tcInfo(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep, int hf_index, guint32 param);
int butc_dissect_tc_restoreArray(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep, int hf_index, guint32 param);
int butc_dissect_tc_dumpArray(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep, int hf_index, guint32 param);
int butc_dissect_tc_dumpInterface(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep, int hf_index, guint32 param);
int butc_dissect_tc_statusInfoSwitchVol(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep, int hf_index, guint32 param);
int butc_dissect_tc_statusInfoSwitchLabel(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep, int hf_index, guint32 param);
int butc_dissect_tciStatusS(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, guint8 *drep, int hf_index, guint32 param);
/* END OF INCLUDED FILE : ETH_HDR */



#endif /* packet-dcerpc-butc.h */
