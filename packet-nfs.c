/* packet-nfs.c
 * Routines for nfs dissection
 * Copyright 1999, Uwe Girlich <Uwe.Girlich@philosys.de>
 * Copyright 2000-2002, Mike Frisch <frisch@hummingbird.com> (NFSv4 decoding)
 * $Id: packet-nfs.c,v 1.68 2002/03/07 05:51:11 guy Exp $
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@ethereal.com>
 * Copyright 1998 Gerald Combs
 *
 * Copied from packet-smb.c
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
#include "config.h"
#endif


#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include <string.h>


#include "packet-rpc.h"
#include "packet-nfs.h"
#include "prefs.h"


static int proto_nfs = -1;

static int hf_nfs_fh_length = -1;
static int hf_nfs_fh_hash = -1;
static int hf_nfs_fh_fsid_major = -1;
static int hf_nfs_fh_fsid_minor = -1;
static int hf_nfs_fh_fsid_inode = -1;
static int hf_nfs_fh_xfsid_major = -1;
static int hf_nfs_fh_xfsid_minor = -1;
static int hf_nfs_fh_fstype = -1;
static int hf_nfs_fh_fn = -1;
static int hf_nfs_fh_fn_len = -1;
static int hf_nfs_fh_fn_inode = -1;
static int hf_nfs_fh_fn_generation = -1;
static int hf_nfs_fh_xfn = -1;
static int hf_nfs_fh_xfn_len = -1;
static int hf_nfs_fh_xfn_inode = -1;
static int hf_nfs_fh_xfn_generation = -1;
static int hf_nfs_fh_dentry = -1;
static int hf_nfs_fh_dev = -1;
static int hf_nfs_fh_xdev = -1;
static int hf_nfs_fh_dirinode = -1;
static int hf_nfs_fh_pinode = -1;
static int hf_nfs_fh_hp_len = -1;
static int hf_nfs_fh_version = -1;
static int hf_nfs_fh_auth_type = -1;
static int hf_nfs_fh_fsid_type = -1;
static int hf_nfs_fh_fileid_type = -1;
static int hf_nfs_stat = -1;
static int hf_nfs_name = -1;
static int hf_nfs_full_name = -1;
static int hf_nfs_readlink_data = -1;
static int hf_nfs_read_offset = -1;
static int hf_nfs_read_count = -1;
static int hf_nfs_read_totalcount = -1;
static int hf_nfs_data = -1;
static int hf_nfs_write_beginoffset = -1;
static int hf_nfs_write_offset = -1;
static int hf_nfs_write_totalcount = -1;
static int hf_nfs_symlink_to = -1;
static int hf_nfs_readdir_cookie = -1;
static int hf_nfs_readdir_count = -1;
static int hf_nfs_readdir_entry = -1;
static int hf_nfs_readdir_entry_fileid = -1;
static int hf_nfs_readdir_entry_name = -1;
static int hf_nfs_readdir_entry_cookie = -1;
static int hf_nfs_readdir_entry3_fileid = -1;
static int hf_nfs_readdir_entry3_name = -1;
static int hf_nfs_readdir_entry3_cookie = -1;
static int hf_nfs_readdirplus_entry_fileid = -1;
static int hf_nfs_readdirplus_entry_name = -1;
static int hf_nfs_readdirplus_entry_cookie = -1;
static int hf_nfs_readdir_eof = -1;
static int hf_nfs_statfs_tsize = -1;
static int hf_nfs_statfs_bsize = -1;
static int hf_nfs_statfs_blocks = -1;
static int hf_nfs_statfs_bfree = -1;
static int hf_nfs_statfs_bavail = -1;
static int hf_nfs_ftype3 = -1;
static int hf_nfs_nfsstat3 = -1;
static int hf_nfs_read_eof = -1;
static int hf_nfs_write_stable = -1;
static int hf_nfs_write_committed = -1;
static int hf_nfs_createmode3 = -1;
static int hf_nfs_fsstat_invarsec = -1;
static int hf_nfs_fsinfo_rtmax = -1;
static int hf_nfs_fsinfo_rtpref = -1;
static int hf_nfs_fsinfo_rtmult = -1;
static int hf_nfs_fsinfo_wtmax = -1;
static int hf_nfs_fsinfo_wtpref = -1;
static int hf_nfs_fsinfo_wtmult = -1;
static int hf_nfs_fsinfo_dtpref = -1;
static int hf_nfs_fsinfo_maxfilesize = -1;
static int hf_nfs_fsinfo_properties = -1;
static int hf_nfs_pathconf_linkmax = -1;
static int hf_nfs_pathconf_name_max = -1;
static int hf_nfs_pathconf_no_trunc = -1;
static int hf_nfs_pathconf_chown_restricted = -1;
static int hf_nfs_pathconf_case_insensitive = -1;
static int hf_nfs_pathconf_case_preserving = -1;
static int hf_nfs_data_follows = -1;

static int hf_nfs_atime = -1;
static int hf_nfs_atime_sec = -1;
static int hf_nfs_atime_nsec = -1;
static int hf_nfs_atime_usec = -1;
static int hf_nfs_mtime = -1;
static int hf_nfs_mtime_sec = -1;
static int hf_nfs_mtime_nsec = -1;
static int hf_nfs_mtime_usec = -1;
static int hf_nfs_ctime = -1;
static int hf_nfs_ctime_sec = -1;
static int hf_nfs_ctime_nsec = -1;
static int hf_nfs_ctime_usec = -1;
static int hf_nfs_dtime = -1;
static int hf_nfs_dtime_sec = -1;
static int hf_nfs_dtime_nsec = -1;

static int hf_nfs_fattr_type = -1;
static int hf_nfs_fattr_nlink = -1;
static int hf_nfs_fattr_uid = -1;
static int hf_nfs_fattr_gid = -1;
static int hf_nfs_fattr_size = -1;
static int hf_nfs_fattr_blocksize = -1;
static int hf_nfs_fattr_rdev = -1;
static int hf_nfs_fattr_blocks = -1;
static int hf_nfs_fattr_fsid = -1;
static int hf_nfs_fattr_fileid = -1;
static int hf_nfs_fattr3_type = -1;
static int hf_nfs_fattr3_nlink = -1;
static int hf_nfs_fattr3_uid = -1;
static int hf_nfs_fattr3_gid = -1;
static int hf_nfs_fattr3_size = -1;
static int hf_nfs_fattr3_used = -1;
static int hf_nfs_fattr3_rdev = -1;
static int hf_nfs_fattr3_fsid = -1;
static int hf_nfs_fattr3_fileid = -1;
static int hf_nfs_wcc_attr_size = -1;
static int hf_nfs_set_size3_size = -1;
static int hf_nfs_cookie3 = -1;
static int hf_nfs_fsstat3_resok_tbytes = -1;
static int hf_nfs_fsstat3_resok_fbytes = -1;
static int hf_nfs_fsstat3_resok_abytes = -1;
static int hf_nfs_fsstat3_resok_tfiles = -1;
static int hf_nfs_fsstat3_resok_ffiles = -1;
static int hf_nfs_fsstat3_resok_afiles = -1;
static int hf_nfs_uid3 = -1;
static int hf_nfs_gid3 = -1;
static int hf_nfs_offset3 = -1;
static int hf_nfs_count3 = -1;
static int hf_nfs_count3_maxcount = -1;
static int hf_nfs_count3_dircount= -1;

/* NFSv4 */
static int hf_nfs_argop4 = -1;
static int hf_nfs_resop4 = -1;
static int hf_nfs_linktext4 = -1;
static int hf_nfs_tag4 = -1;
static int hf_nfs_component4 = -1;
static int hf_nfs_clientid4 = -1;
static int hf_nfs_ace4 = -1;
static int hf_nfs_recall = -1;
static int hf_nfs_open_claim_type4 = -1;
static int hf_nfs_opentype4 = -1;
static int hf_nfs_limit_by4 = -1;
static int hf_nfs_open_delegation_type4 = -1;
static int hf_nfs_ftype4 = -1;
static int hf_nfs_change_info4_atomic = -1;
static int hf_nfs_open4_share_access = -1;
static int hf_nfs_open4_share_deny = -1;
static int hf_nfs_seqid4 = -1;
static int hf_nfs_lock_seqid4 = -1;
static int hf_nfs_mand_attr = -1;
static int hf_nfs_recc_attr = -1;
static int hf_nfs_time_how4 = -1;
static int hf_nfs_attrlist4 = -1;
static int hf_nfs_fattr4_link_support = -1;
static int hf_nfs_fattr4_symlink_support = -1;
static int hf_nfs_fattr4_named_attr = -1;
static int hf_nfs_fattr4_unique_handles = -1;
static int hf_nfs_fattr4_archive = -1;
static int hf_nfs_fattr4_cansettime = -1;
static int hf_nfs_fattr4_case_insensitive = -1;
static int hf_nfs_fattr4_case_preserving = -1;
static int hf_nfs_fattr4_chown_restricted = -1;
static int hf_nfs_fattr4_hidden = -1;
static int hf_nfs_fattr4_homogeneous = -1;
static int hf_nfs_fattr4_mimetype = -1;
static int hf_nfs_fattr4_no_trunc = -1;
static int hf_nfs_fattr4_system = -1;
static int hf_nfs_fattr4_owner = -1;
static int hf_nfs_fattr4_owner_group = -1;
static int hf_nfs_fattr4_size = -1;
static int hf_nfs_fattr4_aclsupport = -1;
static int hf_nfs_fattr4_lease_time = -1;
static int hf_nfs_fattr4_fileid = -1;
static int hf_nfs_fattr4_files_avail = -1;
static int hf_nfs_fattr4_files_free = -1;
static int hf_nfs_fattr4_files_total = -1;
static int hf_nfs_fattr4_maxfilesize = -1;
static int hf_nfs_fattr4_maxlink = -1;
static int hf_nfs_fattr4_maxname = -1;
static int hf_nfs_fattr4_numlinks = -1;
static int hf_nfs_fattr4_maxread = -1;
static int hf_nfs_fattr4_maxwrite = -1;
static int hf_nfs_fattr4_quota_hard = -1;
static int hf_nfs_fattr4_quota_soft = -1;
static int hf_nfs_fattr4_quota_used = -1;
static int hf_nfs_fattr4_space_avail = -1;
static int hf_nfs_fattr4_space_free = -1;
static int hf_nfs_fattr4_space_total = -1;
static int hf_nfs_fattr4_space_used = -1;
static int hf_nfs_who = -1;
static int hf_nfs_server = -1;
static int hf_nfs_stable_how4 = -1;
static int hf_nfs_dirlist4_eof = -1;
static int hf_nfs_stateid4 = -1;
static int hf_nfs_offset4 = -1;
static int hf_nfs_specdata1 = -1;
static int hf_nfs_specdata2 = -1;
static int hf_nfs_lock_type4 = -1;
static int hf_nfs_reclaim4 = -1;
static int hf_nfs_length4 = -1;
static int hf_nfs_changeid4 = -1;
static int hf_nfs_nfstime4_seconds = -1;
static int hf_nfs_nfstime4_nseconds = -1;
static int hf_nfs_fsid4_major = -1;
static int hf_nfs_fsid4_minor = -1;
static int hf_nfs_acetype4 = -1;
static int hf_nfs_aceflag4 = -1;
static int hf_nfs_acemask4 = -1;
static int hf_nfs_delegate_type = -1;
static int hf_nfs_secinfo_flavor = -1;
static int hf_nfs_num_blocks = -1;
static int hf_nfs_bytes_per_block = -1;
static int hf_nfs_eof = -1;
static int hf_nfs_stateid4_delegate_stateid = -1;
static int hf_nfs_verifier4 = -1;
static int hf_nfs_cookie4 = -1;
static int hf_nfs_cookieverf4 = -1;
static int hf_nfs_cb_program = -1;
static int hf_nfs_cb_location = -1;
static int hf_nfs_recall4 = -1;
static int hf_nfs_filesize = -1;
static int hf_nfs_count4 = -1;
static int hf_nfs_count4_dircount = -1;
static int hf_nfs_count4_maxcount = -1;
static int hf_nfs_minorversion = -1;
static int hf_nfs_open_owner4 = -1;
static int hf_nfs_lock_owner4 = -1;
static int hf_nfs_new_lock_owner = -1;
static int hf_nfs_sec_oid4 = -1;
static int hf_nfs_qop4 = -1;
static int hf_nfs_secinfo_rpcsec_gss_info_service = -1;
static int hf_nfs_attrdircreate = -1;
static int hf_nfs_client_id4_id = -1;
static int hf_nfs_stateid4_other = -1;
static int hf_nfs_lock4_reclaim = -1;

static gint ett_nfs = -1;
static gint ett_nfs_fh_encoding = -1;
static gint ett_nfs_fh_fsid = -1;
static gint ett_nfs_fh_xfsid = -1;
static gint ett_nfs_fh_fn = -1;
static gint ett_nfs_fh_xfn = -1;
static gint ett_nfs_fh_hp = -1;
static gint ett_nfs_fh_auth = -1;
static gint ett_nfs_fhandle = -1;
static gint ett_nfs_timeval = -1;
static gint ett_nfs_mode = -1;
static gint ett_nfs_fattr = -1;
static gint ett_nfs_sattr = -1;
static gint ett_nfs_diropargs = -1;
static gint ett_nfs_readdir_entry = -1;
static gint ett_nfs_mode3 = -1;
static gint ett_nfs_specdata3 = -1;
static gint ett_nfs_fh3 = -1;
static gint ett_nfs_nfstime3 = -1;
static gint ett_nfs_fattr3 = -1;
static gint ett_nfs_post_op_fh3 = -1;
static gint ett_nfs_sattr3 = -1;
static gint ett_nfs_diropargs3 = -1;
static gint ett_nfs_sattrguard3 = -1;
static gint ett_nfs_set_mode3 = -1;
static gint ett_nfs_set_uid3 = -1;
static gint ett_nfs_set_gid3 = -1;
static gint ett_nfs_set_size3 = -1;
static gint ett_nfs_set_atime = -1;
static gint ett_nfs_set_mtime = -1;
static gint ett_nfs_pre_op_attr = -1;
static gint ett_nfs_post_op_attr = -1;
static gint ett_nfs_wcc_attr = -1;
static gint ett_nfs_wcc_data = -1;
static gint ett_nfs_access = -1;
static gint ett_nfs_fsinfo_properties = -1;

/* NFSv4 */
static gint ett_nfs_compound_call4 = -1;
static gint ett_nfs_utf8string = -1;
static gint ett_nfs_argop4 = -1;
static gint ett_nfs_resop4 = -1;
static gint ett_nfs_access4 = -1;
static gint ett_nfs_close4 = -1;
static gint ett_nfs_commit4 = -1;
static gint ett_nfs_create4 = -1;
static gint ett_nfs_delegpurge4 = -1;
static gint ett_nfs_delegreturn4 = -1;
static gint ett_nfs_getattr4 = -1;
static gint ett_nfs_getfh4 = -1;
static gint ett_nfs_link4 = -1;
static gint ett_nfs_lock4 = -1;
static gint ett_nfs_lockt4 = -1;
static gint ett_nfs_locku4 = -1;
static gint ett_nfs_lookup4 = -1;
static gint ett_nfs_lookupp4 = -1;
static gint ett_nfs_nverify4 = -1;
static gint ett_nfs_open4 = -1;
static gint ett_nfs_openattr4 = -1;
static gint ett_nfs_open_confirm4 = -1;
static gint ett_nfs_open_downgrade4 = -1;
static gint ett_nfs_putfh4 = -1;
static gint ett_nfs_putpubfh4 = -1;
static gint ett_nfs_putrootfh4 = -1;
static gint ett_nfs_read4 = -1;
static gint ett_nfs_readdir4 = -1;
static gint ett_nfs_readlink4 = -1;
static gint ett_nfs_remove4 = -1;
static gint ett_nfs_rename4 = -1;
static gint ett_nfs_renew4 = -1;
static gint ett_nfs_restorefh4 = -1;
static gint ett_nfs_savefh4 = -1;
static gint ett_nfs_secinfo4 = -1;
static gint ett_nfs_setattr4 = -1;
static gint ett_nfs_setclientid4 = -1;
static gint ett_nfs_setclientid_confirm4 = -1;
static gint ett_nfs_verify4 = -1;
static gint ett_nfs_write4 = -1;
static gint ett_nfs_verifier4 = -1;
static gint ett_nfs_opaque = -1;
static gint ett_nfs_dirlist4 = -1;
static gint ett_nfs_pathname4 = -1;
static gint ett_nfs_change_info4 = -1;
static gint ett_nfs_open_delegation4 = -1;
static gint ett_nfs_open_claim4 = -1;
static gint ett_nfs_opentype4 = -1;
static gint ett_nfs_lock_owner4 = -1;
static gint ett_nfs_cb_client4 = -1;
static gint ett_nfs_client_id4 = -1;
static gint ett_nfs_bitmap4 = -1;
static gint ett_nfs_fattr4 = -1;
static gint ett_nfs_fsid4 = -1;
static gint ett_nfs_fs_locations4 = -1;
static gint ett_nfs_fs_location4 = -1;
static gint ett_nfs_open4_result_flags = -1;
static gint ett_nfs_secinfo4_flavor_info = -1;
static gint ett_nfs_stateid4 = -1;
static gint ett_nfs_fattr4_fh_expire_type = -1;


/* file name snooping */
gboolean nfs_file_name_snooping = FALSE;
gboolean nfs_file_name_full_snooping = FALSE;
typedef struct nfs_name_snoop {
	int fh_length;
	unsigned char *fh;
	int name_len;
	unsigned char *name;
	int parent_len;
	unsigned char *parent;
	int full_name_len;
	unsigned char *full_name;
} nfs_name_snoop_t;

typedef struct nfs_name_snoop_key {
	int key;
	int fh_length;
	unsigned char *fh;
} nfs_name_snoop_key_t;

static GMemChunk *nfs_name_snoop_chunk = NULL;
static int nfs_name_snoop_init_count = 100;
static GHashTable *nfs_name_snoop_unmatched = NULL;

static GMemChunk *nfs_name_snoop_key_chunk = NULL;
static int nfs_name_snoop_key_init_count = 100;
static GHashTable *nfs_name_snoop_matched = NULL;

static GHashTable *nfs_name_snoop_known = NULL;

static gint
nfs_name_snoop_matched_equal(gconstpointer k1, gconstpointer k2)
{
	nfs_name_snoop_key_t *key1 = (nfs_name_snoop_key_t *)k1;
	nfs_name_snoop_key_t *key2 = (nfs_name_snoop_key_t *)k2;

	return (key1->key==key2->key)
	     &&(key1->fh_length==key2->fh_length)
	     &&(!memcmp(key1->fh, key2->fh, key1->fh_length));
}
static guint
nfs_name_snoop_matched_hash(gconstpointer k)
{
	nfs_name_snoop_key_t *key = (nfs_name_snoop_key_t *)k;
	int i;
	int hash;

	hash=key->key;
	for(i=0;i<key->fh_length;i++)
		hash ^= key->fh[i];

	return hash;
}
static gint
nfs_name_snoop_unmatched_equal(gconstpointer k1, gconstpointer k2)
{
	guint32 key1 = (guint32)k1;
	guint32 key2 = (guint32)k2;

	return key1==key2;
}
static guint
nfs_name_snoop_unmatched_hash(gconstpointer k)
{
	guint32 key = (guint32)k;

	return key;
}
static gboolean
nfs_name_snoop_unmatched_free_all(gpointer key_arg, gpointer value, gpointer user_data)
{
	nfs_name_snoop_t *nns = (nfs_name_snoop_t *)value;

	if(nns->name){
		g_free((gpointer)nns->name);
		nns->name=NULL;
		nns->name_len=0;
	}
	if(nns->full_name){
		g_free((gpointer)nns->full_name);
		nns->full_name=NULL;
		nns->full_name_len=0;
	}
	if(nns->parent){
		g_free((gpointer)nns->parent);
		nns->parent=NULL;
		nns->parent_len=0;
	}
	if(nns->fh){
		g_free((gpointer)nns->fh);
		nns->fh=NULL;
		nns->fh_length=0;
	}
	return TRUE;
}

static void
nfs_name_snoop_init(void)
{
	if (nfs_name_snoop_unmatched != NULL) {
		g_hash_table_foreach_remove(nfs_name_snoop_unmatched,
				nfs_name_snoop_unmatched_free_all, NULL);
	} else {
		/* The fragment table does not exist. Create it */
		nfs_name_snoop_unmatched=g_hash_table_new(nfs_name_snoop_unmatched_hash,
			nfs_name_snoop_unmatched_equal);
	}
	if (nfs_name_snoop_matched != NULL) {
		g_hash_table_foreach_remove(nfs_name_snoop_matched,
				nfs_name_snoop_unmatched_free_all, NULL);
	} else {
		/* The fragment table does not exist. Create it */
		nfs_name_snoop_matched=g_hash_table_new(nfs_name_snoop_matched_hash,
			nfs_name_snoop_matched_equal);
	}
	if (nfs_name_snoop_known != NULL) {
		g_hash_table_foreach_remove(nfs_name_snoop_known,
				nfs_name_snoop_unmatched_free_all, NULL);
	} else {
		/* The fragment table does not exist. Create it */
		nfs_name_snoop_known=g_hash_table_new(nfs_name_snoop_matched_hash,
			nfs_name_snoop_matched_equal);
	}

	if(nfs_name_snoop_chunk){
		g_mem_chunk_destroy(nfs_name_snoop_chunk);
		nfs_name_snoop_chunk = NULL;
	}
	if(nfs_name_snoop_key_chunk){
		g_mem_chunk_destroy(nfs_name_snoop_key_chunk);
		nfs_name_snoop_key_chunk = NULL;
	}

	if(nfs_file_name_snooping){
		nfs_name_snoop_chunk = g_mem_chunk_new("nfs_name_snoop_chunk",
			sizeof(nfs_name_snoop_t),
			nfs_name_snoop_init_count * sizeof(nfs_name_snoop_t),
			G_ALLOC_ONLY);
		nfs_name_snoop_key_chunk = g_mem_chunk_new("nfs_name_snoop_key_chunk",
			sizeof(nfs_name_snoop_key_t),
			nfs_name_snoop_key_init_count * sizeof(nfs_name_snoop_key_t),
			G_ALLOC_ONLY);
	}
		
}

void
nfs_name_snoop_add_name(int xid, tvbuff_t *tvb, int name_offset, int name_len, int parent_offset, int parent_len, unsigned char *name)
{
	nfs_name_snoop_t *nns, *old_nns;
	unsigned char *ptr=NULL;

	/* filter out all '.' and '..' names */
	if(!name){
		ptr=(unsigned char *)tvb_get_ptr(tvb, name_offset, name_len);
		if(ptr[0]=='.'){
			if(ptr[1]==0){
				return;
			}
			if(ptr[1]=='.'){
				if(ptr[2]==0){
					return;
				}
			}
		}
	}

	nns=g_mem_chunk_alloc(nfs_name_snoop_chunk);

	nns->fh_length=0;
	nns->fh=NULL;

	if(parent_len){
		nns->parent_len=parent_len;
		nns->parent=g_malloc(parent_len);
		memcpy(nns->parent, tvb_get_ptr(tvb, parent_offset, parent_len), parent_len);
	} else {
		nns->parent_len=0;
		nns->parent=NULL;
	}

	nns->name_len=name_len;
	if(name){
		nns->name=name;
	} else {
		nns->name=g_malloc(name_len+1);
		memcpy(nns->name, ptr, name_len);
	}
	nns->name[name_len]=0;

	nns->full_name_len=0;
	nns->full_name=NULL;

	/* remove any old entry for this */
	old_nns=g_hash_table_lookup(nfs_name_snoop_unmatched, (gconstpointer)xid);
	if(old_nns){
		/* if we havnt seen the reply yet, then there are no
		   matched entries for it, thus we can dealloc the arrays*/
		if(!old_nns->fh){
			g_free(old_nns->name);
			old_nns->name=NULL;
			old_nns->name_len=0;

			g_free(old_nns->parent);
			old_nns->parent=NULL;
			old_nns->parent_len=0;

			g_mem_chunk_free(nfs_name_snoop_chunk, old_nns);
		}
		g_hash_table_remove(nfs_name_snoop_unmatched, (gconstpointer)xid);
	}

	g_hash_table_insert(nfs_name_snoop_unmatched, (gpointer)xid, nns);
}

static void
nfs_name_snoop_add_fh(int xid, tvbuff_t *tvb, int fh_offset, int fh_length)
{
	nfs_name_snoop_t *nns, *old_nns;
	nfs_name_snoop_key_t *key;

	/* find which request we correspond to */
	nns=g_hash_table_lookup(nfs_name_snoop_unmatched, (gconstpointer)xid);
	if(!nns){
		/* oops couldnt find matching request, bail out */
		return;
	}

	/* if we have already seen this response earlier */
	if(nns->fh){
		return;
	}

	/* oki, we have a new entry */
	nns->fh=g_malloc(fh_length);
	memcpy(nns->fh, tvb_get_ptr(tvb, fh_offset, fh_length), fh_length);
	nns->fh_length=fh_length;
	
	key=g_mem_chunk_alloc(nfs_name_snoop_key_chunk);
	key->key=0;
	key->fh_length=nns->fh_length;
	key->fh    =nns->fh;

	/* already have something matched for this fh, remove it from
	   the table */
	old_nns=g_hash_table_lookup(nfs_name_snoop_matched, key);
	if(old_nns){
		g_hash_table_remove(nfs_name_snoop_matched, key);
	}

	g_hash_table_remove(nfs_name_snoop_unmatched, (gconstpointer)xid);
	g_hash_table_insert(nfs_name_snoop_matched, key, nns);
}

static void
nfs_full_name_snoop(nfs_name_snoop_t *nns, int *len, unsigned char **name, unsigned char **pos)
{
	nfs_name_snoop_t *parent_nns = NULL;
	nfs_name_snoop_key_t key;

	/* check if the nns component ends with a '/' else we just allocate
	   an extra byte to len to accommodate for it later */
	if(nns->name[nns->name_len-1]!='/'){
		(*len)++;
	}

	(*len) += nns->name_len;

	if(nns->parent==NULL){
		*name = g_malloc((*len)+1);
		*pos = *name;

		strcpy(*pos, nns->name);
		*pos += nns->name_len;
		return;
	}

	key.key=0;
	key.fh_length=nns->parent_len;
	key.fh=nns->parent;

	parent_nns=g_hash_table_lookup(nfs_name_snoop_matched, &key);

	if(parent_nns){
		nfs_full_name_snoop(parent_nns, len, name, pos);
		if(*name){
			/* make sure components are '/' separated */
			if( (*pos)[-1] != '/'){
				**pos='/';
				(*pos)++;
				**pos=0;
			}
			strcpy(*pos, nns->name);
			*pos += nns->name_len;
		}
		return;
	}

	return;
}

static void
nfs_name_snoop_fh(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int fh_offset, int fh_length)
{
	nfs_name_snoop_key_t key;
	nfs_name_snoop_t *nns = NULL;

	/* if this is a new packet, see if we can register the mapping */
	if(!pinfo->fd->flags.visited){
		key.key=0;
		key.fh_length=fh_length;
		key.fh=(unsigned char *)tvb_get_ptr(tvb, fh_offset, fh_length);

		nns=g_hash_table_lookup(nfs_name_snoop_matched, &key);
		if(nns){
			nfs_name_snoop_key_t *k;
			k=g_mem_chunk_alloc(nfs_name_snoop_key_chunk);
			k->key=pinfo->fd->num;
			k->fh_length=nns->fh_length;
			k->fh=nns->fh;
			g_hash_table_insert(nfs_name_snoop_known, k, nns);

			if(nfs_file_name_full_snooping){
				unsigned char *name=NULL, *pos=NULL;
				int len=0;

				nfs_full_name_snoop(nns, &len, &name, &pos);
				if(name){
					nns->full_name=name;
					nns->full_name_len=len;
				}
			}
		}
	}

	/* see if we know this mapping */
	if(!nns){
		key.key=pinfo->fd->num;
		key.fh_length=fh_length;
		key.fh=(unsigned char *)tvb_get_ptr(tvb, fh_offset, fh_length);

		nns=g_hash_table_lookup(nfs_name_snoop_known, &key);
	}

	/* if we know the mapping, print the filename */
	if(nns){
		proto_tree_add_string_format(tree, hf_nfs_name, tvb, 
			fh_offset, 0, nns->name, "Name: %s", nns->name);
		if(nns->full_name){
			proto_tree_add_string_format(tree, hf_nfs_full_name, tvb, 
				fh_offset, 0, nns->name, "Full Name: %s", nns->full_name);
		}
	}
}

/* file handle dissection */

#define FHT_UNKNOWN		0
#define FHT_SVR4		1
#define FHT_LINUX_KNFSD_LE	2
#define FHT_LINUX_NFSD_LE	3
#define FHT_LINUX_KNFSD_NEW	4

static const value_string names_fhtype[] =
{
	{	FHT_UNKNOWN,		"unknown"				},
	{	FHT_SVR4,		"System V R4"				},
	{	FHT_LINUX_KNFSD_LE,	"Linux knfsd (little-endian)"		},
	{	FHT_LINUX_NFSD_LE,	"Linux user-land nfsd (little-endian)"	},
	{	FHT_LINUX_KNFSD_NEW,	"Linux knfsd (new)"			},
	{	0,	NULL	}
};


/* SVR4: checked with ReliantUNIX (5.43, 5.44, 5.45) */

static void
dissect_fhandle_data_SVR4(tvbuff_t* tvb, int offset, proto_tree *tree,
    int fhlen)
{
	guint32 nof = offset;

	/* file system id */
	{
	guint32 fsid_O;
	guint32 fsid_L;
	guint32 temp;
	guint32 fsid_major;
	guint32 fsid_minor;

	fsid_O = nof;
	fsid_L = 4;
	temp = tvb_get_ntohl(tvb, fsid_O);
	fsid_major = ( temp>>18 ) &  0x3fff; /* 14 bits */
	fsid_minor = ( temp     ) & 0x3ffff; /* 18 bits */
	if (tree) {
		proto_item* fsid_item = NULL;
		proto_tree* fsid_tree = NULL;
	
		fsid_item = proto_tree_add_text(tree, tvb,
			fsid_O, fsid_L, 
			"file system ID: %d,%d", fsid_major, fsid_minor);
		if (fsid_item) {
			fsid_tree = proto_item_add_subtree(fsid_item, 
					ett_nfs_fh_fsid);
			proto_tree_add_uint(fsid_tree, hf_nfs_fh_fsid_major,
				tvb, fsid_O,   2, fsid_major);
			proto_tree_add_uint(fsid_tree, hf_nfs_fh_fsid_minor,
				tvb, fsid_O+1, 3, fsid_minor);
		}
	}
	nof = fsid_O + fsid_L;
	}

	/* file system type */
	{
	guint32 fstype_O;
	guint32 fstype_L;
	guint32 fstype;

	fstype_O = nof;
	fstype_L = 4;
	fstype = tvb_get_ntohl(tvb, fstype_O);
	if (tree) {
		proto_tree_add_uint(tree, hf_nfs_fh_fstype, tvb,
			fstype_O, fstype_L, fstype);
	}
	nof = fstype_O + fstype_L;
	}

	/* file number */
	{
	guint32 fn_O;
	guint32 fn_len_O;
	guint32 fn_len_L;
	guint32 fn_len;
	guint32 fn_data_O;
	guint32 fn_data_inode_O;
	guint32 fn_data_inode_L;
	guint32 inode;
	guint32 fn_data_gen_O;
	guint32 fn_data_gen_L;
	guint32 gen;
	guint32 fn_L;
	
	fn_O = nof;
	fn_len_O = fn_O;
	fn_len_L = 2;
	fn_len = tvb_get_ntohs(tvb, fn_len_O);
	fn_data_O = fn_O + fn_len_L;
	fn_data_inode_O = fn_data_O + 2;
	fn_data_inode_L = 4;
	inode = tvb_get_ntohl(tvb, fn_data_inode_O);
	fn_data_gen_O = fn_data_inode_O + fn_data_inode_L;
	fn_data_gen_L = 4;
	gen = tvb_get_ntohl(tvb, fn_data_gen_O);
	fn_L = fn_len_L + fn_len;
	if (tree) {
		proto_item* fn_item = NULL;
		proto_tree* fn_tree = NULL;
	
		fn_item = proto_tree_add_uint(tree, hf_nfs_fh_fn, tvb,
			fn_O, fn_L, inode);
		if (fn_item) {
			fn_tree = proto_item_add_subtree(fn_item, 
					ett_nfs_fh_fn);
			proto_tree_add_uint(fn_tree, hf_nfs_fh_fn_len,
				tvb, fn_len_O, fn_len_L, fn_len);
			proto_tree_add_uint(fn_tree, hf_nfs_fh_fn_inode,
				tvb, fn_data_inode_O, fn_data_inode_L, inode);
			proto_tree_add_uint(fn_tree, hf_nfs_fh_fn_generation,
				tvb, fn_data_gen_O, fn_data_gen_L, gen);
		}
	}
	nof = fn_O + fn_len_L + fn_len;
	}

	/* exported file number */
	{
	guint32 xfn_O;
	guint32 xfn_len_O;
	guint32 xfn_len_L;
	guint32 xfn_len;
	guint32 xfn_data_O;
	guint32 xfn_data_inode_O;
	guint32 xfn_data_inode_L;
	guint32 xinode;
	guint32 xfn_data_gen_O;
	guint32 xfn_data_gen_L;
	guint32 xgen;
	guint32 xfn_L;
	
	xfn_O = nof;
	xfn_len_O = xfn_O;
	xfn_len_L = 2;
	xfn_len = tvb_get_ntohs(tvb, xfn_len_O);
	xfn_data_O = xfn_O + xfn_len_L;
	xfn_data_inode_O = xfn_data_O + 2;
	xfn_data_inode_L = 4;
	xinode = tvb_get_ntohl(tvb, xfn_data_inode_O);
	xfn_data_gen_O = xfn_data_inode_O + xfn_data_inode_L;
	xfn_data_gen_L = 4;
	xgen = tvb_get_ntohl(tvb, xfn_data_gen_O);
	xfn_L = xfn_len_L + xfn_len;
	if (tree) {
		proto_item* xfn_item = NULL;
		proto_tree* xfn_tree = NULL;
	
		xfn_item = proto_tree_add_uint(tree, hf_nfs_fh_xfn, tvb,
			xfn_O, xfn_L, xinode);
		if (xfn_item) {
			xfn_tree = proto_item_add_subtree(xfn_item, 
					ett_nfs_fh_xfn);
			proto_tree_add_uint(xfn_tree, hf_nfs_fh_xfn_len,
				tvb, xfn_len_O, xfn_len_L, xfn_len);
			proto_tree_add_uint(xfn_tree, hf_nfs_fh_xfn_inode,
				tvb, xfn_data_inode_O, xfn_data_inode_L, xinode);
			proto_tree_add_uint(xfn_tree, hf_nfs_fh_xfn_generation,
				tvb, xfn_data_gen_O, xfn_data_gen_L, xgen);
		}
	}
	}
}


/* Checked with RedHat Linux 6.2 (kernel 2.2.14 knfsd) */

static void
dissect_fhandle_data_LINUX_KNFSD_LE(tvbuff_t* tvb, int offset, proto_tree *tree,
    int fhlen)
{
	guint32 dentry;
	guint32 inode;
	guint32 dirinode;
	guint32 temp;
	guint32 fsid_major;
	guint32 fsid_minor;
	guint32 xfsid_major;
	guint32 xfsid_minor;
	guint32 xinode;
	guint32 gen;

	dentry   = tvb_get_letohl(tvb, offset+0);
	inode    = tvb_get_letohl(tvb, offset+4);
	dirinode = tvb_get_letohl(tvb, offset+8);
	temp     = tvb_get_letohs (tvb,offset+12);
	fsid_major = (temp >> 8) & 0xff;
	fsid_minor = (temp     ) & 0xff;
	temp     = tvb_get_letohs(tvb,offset+16);
	xfsid_major = (temp >> 8) & 0xff;
	xfsid_minor = (temp     ) & 0xff;
	xinode   = tvb_get_letohl(tvb,offset+20);
	gen      = tvb_get_letohl(tvb,offset+24);

	if (tree) {
		proto_tree_add_uint(tree, hf_nfs_fh_dentry,
			tvb, offset+0, 4, dentry);
		proto_tree_add_uint(tree, hf_nfs_fh_fn_inode,
			tvb, offset+4, 4, inode);
		proto_tree_add_uint(tree, hf_nfs_fh_dirinode,
			tvb, offset+8, 4, dirinode);

		/* file system id (device) */
		{
		proto_item* fsid_item = NULL;
		proto_tree* fsid_tree = NULL;

		fsid_item = proto_tree_add_text(tree, tvb,
			offset+12, 4, 
			"file system ID: %d,%d", fsid_major, fsid_minor);
		if (fsid_item) {
			fsid_tree = proto_item_add_subtree(fsid_item, 
					ett_nfs_fh_fsid);
			proto_tree_add_uint(fsid_tree, hf_nfs_fh_fsid_major,
				tvb, offset+13, 1, fsid_major);
			proto_tree_add_uint(fsid_tree, hf_nfs_fh_fsid_minor,
				tvb, offset+12, 1, fsid_minor);
		}
		}

		/* exported file system id (device) */
		{
		proto_item* xfsid_item = NULL;
		proto_tree* xfsid_tree = NULL;

		xfsid_item = proto_tree_add_text(tree, tvb,
			offset+16, 4, 
			"exported file system ID: %d,%d", xfsid_major, xfsid_minor);
		if (xfsid_item) {
			xfsid_tree = proto_item_add_subtree(xfsid_item, 
					ett_nfs_fh_xfsid);
			proto_tree_add_uint(xfsid_tree, hf_nfs_fh_xfsid_major,
				tvb, offset+17, 1, xfsid_major);
			proto_tree_add_uint(xfsid_tree, hf_nfs_fh_xfsid_minor,
				tvb, offset+16, 1, xfsid_minor);
		}
		}

		proto_tree_add_uint(tree, hf_nfs_fh_xfn_inode,
			tvb, offset+20, 4, xinode);
		proto_tree_add_uint(tree, hf_nfs_fh_fn_generation,
			tvb, offset+24, 4, gen);
	}
}


/* Checked with RedHat Linux 5.2 (nfs-server 2.2beta47 user-land nfsd) */

void
dissect_fhandle_data_LINUX_NFSD_LE(tvbuff_t* tvb, int offset, proto_tree *tree,
    int fhlen)
{
	/* pseudo inode */
	{
	guint32 pinode;
	pinode   = tvb_get_letohl(tvb, offset+0);
	if (tree) {
		proto_tree_add_uint(tree, hf_nfs_fh_pinode,
			tvb, offset+0, 4, pinode);
	}
	}

	/* hash path */
	{
	guint32 hashlen;

	hashlen  = tvb_get_guint8(tvb, offset+4);
	if (tree) {
		proto_item* hash_item = NULL;
		proto_tree* hash_tree = NULL;

		hash_item = proto_tree_add_text(tree, tvb, offset+4,
				hashlen + 1,
				"hash path: %s",
				tvb_bytes_to_str(tvb,offset+5,hashlen));
		if (hash_item) {
			hash_tree = proto_item_add_subtree(hash_item, 
					ett_nfs_fh_hp);
			if (hash_tree) {
		 		proto_tree_add_uint(hash_tree,
					hf_nfs_fh_hp_len, tvb, offset+4, 1,
					hashlen);
				proto_tree_add_text(hash_tree, tvb, offset+5,
					hashlen,
					"key: %s",
					tvb_bytes_to_str(tvb,offset+5,hashlen));
			}
		}
	}
	}
}


/* Checked with SuSE 7.1 (kernel 2.4.0 knfsd) */
/* read linux-2.4.5/include/linux/nfsd/nfsfh.h for more details */

#define AUTH_TYPE_NONE 0
static const value_string auth_type_names[] = {
	{	AUTH_TYPE_NONE,				"no authentication"		},
	{0,NULL}
};

#define FSID_TYPE_MAJOR_MINOR_INODE 0
static const value_string fsid_type_names[] = {
	{	FSID_TYPE_MAJOR_MINOR_INODE,		"major/minor/inode"		},
	{0,NULL}
};

#define FILEID_TYPE_ROOT			0
#define FILEID_TYPE_INODE_GENERATION		1
#define FILEID_TYPE_INODE_GENERATION_PARENT	2
static const value_string fileid_type_names[] = {
	{	FILEID_TYPE_ROOT,			"root"				},
	{	FILEID_TYPE_INODE_GENERATION,		"inode/generation"		},
	{	FILEID_TYPE_INODE_GENERATION_PARENT,	"inode/generation/parent"	},
	{0,NULL}
};

static void
dissect_fhandle_data_LINUX_KNFSD_NEW(tvbuff_t* tvb, int offset, proto_tree *tree,
    int fhlen)
{
	guint8 version;
	guint8 auth_type;
	guint8 fsid_type;
	guint8 fileid_type;

	version     = tvb_get_guint8(tvb, offset + 0);
	if (tree) {
		proto_tree_add_uint(tree, hf_nfs_fh_version,
			tvb, offset+0, 1, version);
	}

	switch (version) {
		case 1: {
			auth_type   = tvb_get_guint8(tvb, offset + 1);
			fsid_type   = tvb_get_guint8(tvb, offset + 2);
			fileid_type = tvb_get_guint8(tvb, offset + 3);
			if (tree) {
				proto_item* encoding_item = proto_tree_add_text(tree, tvb,
					offset + 1, 3,
					"encoding: %u %u %u",
					auth_type, fsid_type, fileid_type);
				if (encoding_item) {
					proto_tree* encoding_tree = proto_item_add_subtree(encoding_item,
						ett_nfs_fh_encoding);
					if (encoding_tree) {
						proto_tree_add_uint(encoding_tree, hf_nfs_fh_auth_type,
							tvb, offset+1, 1, auth_type);
						proto_tree_add_uint(encoding_tree, hf_nfs_fh_fsid_type,
							tvb, offset+2, 1, fsid_type);
						proto_tree_add_uint(encoding_tree, hf_nfs_fh_fileid_type,
							tvb, offset+3, 1, fileid_type);
					}
				}
			}
			offset += 4;
		} break;
		default: {
			/* unknown version */
			goto out;
		}
	}
		
	switch (auth_type) {
		case 0: {
			/* no authentication */
			if (tree) {
				proto_tree_add_text(tree, tvb,
					offset + 0, 0,
					"authentication: none");
			}
		} break;
		default: {
			/* unknown authentication type */
			goto out;
		}
	}

	switch (fsid_type) {
		case 0: {
			guint16 fsid_major;
			guint16 fsid_minor;
			guint32 fsid_inode;

			fsid_major = tvb_get_ntohs(tvb, offset + 0);
			fsid_minor = tvb_get_ntohs(tvb, offset + 2);
			fsid_inode = tvb_get_letohl(tvb, offset + 4);
			if (tree) {
				proto_item* fsid_item = proto_tree_add_text(tree, tvb,
					offset+0, 8, 
					"file system ID: %u,%u (inode %u)",
					fsid_major, fsid_minor, fsid_inode);
				if (fsid_item) {
					proto_tree* fsid_tree = proto_item_add_subtree(fsid_item, 
						ett_nfs_fh_fsid);
					if (fsid_tree) {
						proto_tree_add_uint(fsid_tree, hf_nfs_fh_fsid_major,
							tvb, offset+0, 2, fsid_major);
						proto_tree_add_uint(fsid_tree, hf_nfs_fh_fsid_minor,
							tvb, offset+2, 2, fsid_minor);
						proto_tree_add_uint(fsid_tree, hf_nfs_fh_fsid_inode,
							tvb, offset+4, 4, fsid_inode);
					}
				}
			}
			offset += 8;
		} break;
		default: {
			/* unknown fsid type */
			goto out;
		}
	}

	switch (fileid_type) {
		case 0: {
			if (tree) {
				proto_tree_add_text(tree, tvb,
					offset+0, 0,
					"file ID: root inode");
			}
		} break;
		case 1: {
			guint32 inode;
			guint32 generation;

			inode = tvb_get_letohl(tvb, offset + 0);
			generation = tvb_get_letohl(tvb, offset + 4);

			if (tree) {
				proto_item* fileid_item = proto_tree_add_text(tree, tvb,
					offset+0, 8,
					"file ID: %u (%u)",
					inode, generation);
				if (fileid_item) {
					proto_tree* fileid_tree = proto_item_add_subtree(
						fileid_item, ett_nfs_fh_fn);
					if (fileid_tree) {
						proto_tree_add_uint(fileid_tree, hf_nfs_fh_fn_inode,
						tvb, offset+0, 4, inode);
						proto_tree_add_uint(fileid_tree, hf_nfs_fh_fn_generation,
						tvb, offset+4, 4, generation);
					}
				}
			}

			offset += 8;
		} break;
		case 2: {
			guint32 inode;
			guint32 generation;
			guint32 parent_inode;

			inode = tvb_get_letohl(tvb, offset + 0);
			generation = tvb_get_letohl(tvb, offset + 4);
			parent_inode = tvb_get_letohl(tvb, offset + 8);

			if (tree) {
				 proto_item* fileid_item = proto_tree_add_text(tree, tvb,
					offset+0, 8,
					"file ID: %u (%u)",
					inode, generation);
				if (fileid_item) {
					proto_tree* fileid_tree = proto_item_add_subtree(
						fileid_item, ett_nfs_fh_fn);
					if (fileid_tree) {
						proto_tree_add_uint(fileid_tree, hf_nfs_fh_fn_inode,
						tvb, offset+0, 4, inode);
						proto_tree_add_uint(fileid_tree, hf_nfs_fh_fn_generation,
						tvb, offset+4, 4, generation);
						proto_tree_add_uint(fileid_tree, hf_nfs_fh_dirinode,
						tvb, offset+8, 4, parent_inode);
					}
				}
			}

			offset += 12;
		} break;
		default: {
			/* unknown fileid type */
			goto out;
		}
	}

out:
	;
}


static void
dissect_fhandle_data_unknown(tvbuff_t *tvb, int offset, proto_tree *tree,
    int fhlen)
{
	int sublen;
	int bytes_left;
	gboolean first_line;

	bytes_left = fhlen;
	first_line = TRUE;
	while (bytes_left != 0) {
		sublen = 16;
		if (sublen > bytes_left)
			sublen = bytes_left;
		proto_tree_add_text(tree, tvb, offset, sublen,
					"%s%s",
					first_line ? "data: " :
					             "      ",
					tvb_bytes_to_str(tvb,offset,sublen));
		bytes_left -= sublen;
		offset += sublen;
		first_line = FALSE;
	}
}


static void
dissect_fhandle_data(tvbuff_t *tvb, int offset, packet_info *pinfo,
    proto_tree *tree, unsigned int fhlen)
{
	unsigned int fhtype = FHT_UNKNOWN;

	/* filehandle too long */
	if (fhlen>64) goto type_ready;
	/* Not all bytes there. Any attempt to deduce the type would be
	   senseless. */
	if (!tvb_bytes_exist(tvb,offset,fhlen)) goto type_ready;

	/* create a semiunique hash value for the filehandle */
	{
		guint32 fhhash;
		guint32 i;

		for(fhhash=0,i=0;i<(fhlen-3);i+=4){
			fhhash ^= tvb_get_ntohl(tvb, offset+i);
		}
		proto_tree_add_uint(tree, hf_nfs_fh_hash, tvb, offset, fhlen,
			fhhash);
	}
	if(nfs_file_name_snooping){
		nfs_name_snoop_fh(pinfo, tree, tvb, offset, fhlen);
	}
		
	/* calculate (heuristically) fhtype */
	switch (fhlen) {
		case 12: {
			if (tvb_get_ntohl(tvb,offset) == 0x01000000) {
				fhtype=FHT_LINUX_KNFSD_NEW;
			}
		} break;
		case 20: {
			if (tvb_get_ntohl(tvb,offset) == 0x01000001) {
				fhtype=FHT_LINUX_KNFSD_NEW;
			}
		} break;
		case 24: {
			if (tvb_get_ntohl(tvb,offset) == 0x01000002) {
				fhtype=FHT_LINUX_KNFSD_NEW;
			}
		} break;
		case 32: {
			guint32 len1;
			guint32 len2;
			if (tvb_get_ntohs(tvb,offset+4) == 0) {
				len1=tvb_get_ntohs(tvb,offset+8);
				if (tvb_bytes_exist(tvb,offset+10+len1,2)) {
					len2=tvb_get_ntohs(tvb,
					    offset+10+len1);
					if (fhlen==12+len1+len2) {
						fhtype=FHT_SVR4;
						goto type_ready;
					}
				}
			}
			len1 = tvb_get_guint8(tvb,offset+4);
			if (len1<28 && tvb_bytes_exist(tvb,offset+5,len1)) {
				int wrong=0;
				for (len2=5+len1;len2<32;len2++) {
					if (tvb_get_guint8(tvb,offset+len2)) {
						wrong=1;	
						break;
					}
				}
				if (!wrong) {
					fhtype=FHT_LINUX_NFSD_LE;
					goto type_ready;
				}
			}
			if (tvb_get_ntohl(tvb,offset+28) == 0) {
				if (tvb_get_ntohs(tvb,offset+14) == 0) {
					if (tvb_get_ntohs(tvb,offset+18) == 0) {
						fhtype=FHT_LINUX_KNFSD_LE;
						goto type_ready;
					}
				}
			}
		} break;
	}

type_ready:

	proto_tree_add_text(tree, tvb, offset, 0, 
		"type: %s", val_to_str(fhtype, names_fhtype, "Unknown"));

	switch (fhtype) {
		case FHT_SVR4:
			dissect_fhandle_data_SVR4          (tvb, offset, tree,
			    fhlen);
		break;
		case FHT_LINUX_KNFSD_LE:
			dissect_fhandle_data_LINUX_KNFSD_LE(tvb, offset, tree,
			    fhlen);
		break;
		case FHT_LINUX_NFSD_LE:
			dissect_fhandle_data_LINUX_NFSD_LE (tvb, offset, tree,
			    fhlen);
		break;
		case FHT_LINUX_KNFSD_NEW:
			dissect_fhandle_data_LINUX_KNFSD_NEW (tvb, offset, tree,
			    fhlen);
		break;
		case FHT_UNKNOWN:
		default:
			dissect_fhandle_data_unknown(tvb, offset, tree, fhlen);
		break;
	}
}


/***************************/
/* NFS Version 2, RFC 1094 */
/***************************/


/* RFC 1094, Page 12..14 */
const value_string names_nfs_stat[] =
{
	{	0,	"OK" },
	{	1,	"ERR_PERM" },
	{	2,	"ERR_NOENT" },
	{	5,	"ERR_IO" },
	{	6,	"ERR_NX_IO" },
	{	13,	"ERR_ACCES" },
	{	17,	"ERR_EXIST" },
	{	18,	"ERR_XDEV" },	/* not in spec, but can happen */
	{	19,	"ERR_NODEV" },
	{	20,	"ERR_NOTDIR" },
	{	21,	"ERR_ISDIR" },
	{	22,	"ERR_INVAL" },	/* not in spec, but I think it can happen */
	{	26,	"ERR_TXTBSY" },	/* not in spec, but I think it can happen */
	{	27,	"ERR_FBIG" },
	{	28,	"ERR_NOSPC" },
	{	30,	"ERR_ROFS" },
	{	31,	"ERR_MLINK" },	/* not in spec, but can happen */
	{	45,	"ERR_OPNOTSUPP" }, /* not in spec, but I think it can happen */
	{	63,	"ERR_NAMETOOLONG" },
	{	66,	"ERR_NOTEMPTY" },
	{	69,	"ERR_DQUOT" },
	{	70,	"ERR_STALE" },
	{	99,	"ERR_WFLUSH" },
	{	0,	NULL }
};

/* NFSv4 Draft Specification, Page 198-199 */
const value_string names_nfs_stat4[] = {
	{	0,			"NFS4_OK"							},
	{	1,			"NFS4ERR_PERM"						},
	{	2,			"NFS4ERR_NOENT"					},
	{	5,			"NFS4ERR_IO"						},
	{	6,			"NFS4ERR_NXIO"						},
	{	13,		"NFS4ERR_ACCES"					},
	{	17,		"NFS4ERR_EXIST"					},
	{	18,		"NFS4ERR_XDEV"						},
	{	19,		"NFS4ERR_NODEV"					},
	{	20,		"NFS4ERR_NOTDIR"					},
	{	21,		"NFS4ERR_ISDIR"					},
	{	22,		"NFS4ERR_INVAL"					},
	{	27,		"NFS4ERR_FBIG"						},
	{	28,		"NFS4ERR_NOSPC"					},
	{	30,		"NFS4ERR_ROFS"						},
	{	31,		"NFS4ERR_MLINK"					},
	{	63,		"NFS4ERR_NAMETOOLONG"			},
	{	66,		"NFS4ERR_NOTEMPTY"				},
	{	69,		"NFS4ERR_DQUOT"					},
	{	70,		"NFS4ERR_STALE"					},
	{	10001,	"NFS4ERR_BADHANDLE"				},
	{	10003,	"NFS4ERR_BAD_COOKIE"				},
	{	10004,	"NFS4ERR_NOTSUPP"					},
	{	10005,	"NFS4ERR_TOOSMALL"				},
	{	10006,	"NFS4ERR_SERVERFAULT"			},
	{	10007,	"NFS4ERR_BADTYPE"					},
	{	10008,	"NFS4ERR_DELAY"					},
	{	10009,	"NFS4ERR_SAME"						},
	{	10010,	"NFS4ERR_DENIED"					},
	{	10011,	"NFS4ERR_EXPIRED"					},
	{	10012,	"NFS4ERR_LOCKED"					},
	{	10013,	"NFS4ERR_GRACE"					},
	{	10014,	"NFS4ERR_FHEXPIRED"				},
	{	10015,	"NFS4ERR_SHARE_DENIED"			},
	{	10016,	"NFS4ERR_WRONGSEC"				},
	{	10017,	"NFS4ERR_CLID_INUSE"				},
	{	10018,	"NFS4ERR_RESOURCE"				},
	{	10019,	"NFS4ERR_MOVED"					},
	{	10020,	"NFS4ERR_NOFILEHANDLE"			},
	{	10021,	"NFS4ERR_MINOR_VERS_MISMATCH"	},
	{	10022,	"NFS4ERR_STALE_CLIENTID"		},
	{	10023,	"NFS4ERR_STALE_STATEID"			},
	{	10024,	"NFS4ERR_OLD_STATEID"			},
	{	10025,	"NFS4ERR_BAD_STATEID"			},
	{	10026,	"NFS4ERR_BAD_SEQID"				},
	{	10027,	"NFS4ERR_NOT_SAME"				},
	{	10028,	"NFS4ERR_LOCK_RANGE"				},
	{	10029,	"NFS4ERR_SYMLINK"					},
	{	10030,	"NFS4ERR_READDIR_NOSPC"			},
	{	10031,	"NFS4ERR_LEASE_MOVED"			},
	{	10032,	"NFS4ERR_ATTRNOTSUPP"			},
	{	10033,	"NFS4ERR_NO_GRACE"				},
	{	10034,	"NFS4ERR_RECLAIM_BAD"			},
	{	10035,	"NFS4ERR_RECLAIM_CONFLICT"		},
	{	10036,	"NFS4ERR_BADXDR"					},
	{	10037,	"NFS4ERR_LOCKS_HELD"				},
	{ 0, NULL }
};


/* This function has been modified to support NFSv4 style error codes as
 * well as being backwards compatible with NFSv2 and NFSv3.
 */
int
dissect_stat_internal(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree *tree, guint32* status, int nfsvers)
{
	guint32 stat;

	stat = tvb_get_ntohl(tvb, offset+0);
	
	if (tree) {
		/* this gives the right NFSv2 number<->message relation */
		/* and makes it searchable via "nfs.status" */
		proto_tree_add_uint_format(tree, hf_nfs_nfsstat3, tvb,
			offset+0, 4, stat, "Status: %s (%u)", 
			val_to_str(stat, 
				(nfsvers != 4)? names_nfs_stat: names_nfs_stat4,"%u"), stat);
	}

	offset += 4;

	if (status) *status = stat;

	return offset;
}


/* RFC 1094, Page 12..14 */
int
dissect_stat(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree,
	guint32 *status)
{
	return dissect_stat_internal(tvb, offset, pinfo, tree, status, !4);
}


/* RFC 1094, Page 12..14 */
int
dissect_nfs2_stat_reply(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree* tree)
{
	guint32 status;

	offset = dissect_stat(tvb, offset, pinfo, tree, &status);

	return offset;
}


int
dissect_nfs_nfsstat4(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree *tree, guint32 *status)
{
	return dissect_stat_internal(tvb, offset, pinfo, tree, status, 4);
}


/* RFC 1094, Page 15 */
int
dissect_ftype(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree,
char* name)
{
	guint32 ftype;
	char* ftype_name = NULL;

	const value_string nfs2_ftype[] =
	{
		{	0,	"Non-File" },
		{	1,	"Regular File" },
		{	2,	"Directory" },
		{	3,	"Block Special Device" },
		{	4,	"Character Special Device" },
		{	5,	"Symbolic Link" },
		{	0,	NULL }
	};

	ftype = tvb_get_ntohl(tvb, offset+0);
	ftype_name = val_to_str(ftype, nfs2_ftype, "%u");
	
	if (tree) {
		proto_tree_add_text(tree, tvb, offset, 4,
			"%s: %s (%u)", name, ftype_name, ftype);
	}

	offset += 4;
	return offset;
}


/* RFC 1094, Page 15 */
int
dissect_fhandle(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree,
    char *name)
{
	proto_item* fitem;
	proto_tree* ftree = NULL;

	if (tree) {
		fitem = proto_tree_add_text(tree, tvb, offset, FHSIZE,
			"%s", name);
		if (fitem)
			ftree = proto_item_add_subtree(fitem, ett_nfs_fhandle);
	}

	/* are we snooping fh to filenames ?*/
	if((!pinfo->fd->flags.visited) && nfs_file_name_snooping){
		rpc_call_info_value *civ=pinfo->private_data;

		/* NFS v2 LOOKUP, CREATE, MKDIR calls might give us a mapping*/
		if( (civ->prog==100003)
		  &&(civ->vers==2)
		  &&(!civ->request)
		  &&((civ->proc==4)||(civ->proc==9)||(civ->proc==14))
		) {
			nfs_name_snoop_add_fh(civ->xid, tvb, 
				offset, 32);
		}

		/* MOUNT v1,v2 MNT replies might give us a filehandle*/
		if( (civ->prog==100005)
		  &&(civ->proc==1)
		  &&((civ->vers==1)||(civ->vers==2))
		  &&(!civ->request)
		) {
			nfs_name_snoop_add_fh(civ->xid, tvb, 
				offset, 32);
		}
	}

	dissect_fhandle_data(tvb, offset, pinfo, ftree, FHSIZE);

	offset += FHSIZE;
	return offset;
}

/* RFC 1094, Page 15 */
int
dissect_nfs2_fhandle_call(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree)
{
	offset = dissect_fhandle(tvb, offset, pinfo, tree, "object");

	return offset;
}


/* RFC 1094, Page 15 */
int
dissect_timeval(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf_time, int hf_time_sec, int hf_time_usec)
{
	guint32	seconds;
	guint32 useconds;
	nstime_t ts;

	proto_item* time_item;
	proto_tree* time_tree = NULL;

	seconds = tvb_get_ntohl(tvb, offset+0);
	useconds = tvb_get_ntohl(tvb, offset+4);
	ts.secs = seconds;
	ts.nsecs = useconds*1000;

	if (tree) {
		time_item = proto_tree_add_time(tree, hf_time, tvb, offset, 8,
				&ts);
		if (time_item)
			time_tree = proto_item_add_subtree(time_item, ett_nfs_timeval);
	}

	if (time_tree) {
		proto_tree_add_uint(time_tree, hf_time_sec, tvb, offset, 4,
					seconds);
		proto_tree_add_uint(time_tree, hf_time_usec, tvb, offset+4, 4,
					useconds);
	}
	offset += 8;
	return offset;
}


/* RFC 1094, Page 16 */
const value_string nfs2_mode_names[] = {
	{	0040000,	"Directory"	},
	{	0020000,	"Character Special Device"	},
	{	0060000,	"Block Special Device"	},
	{	0100000,	"Regular File"	},
	{	0120000,	"Symbolic Link"	},
	{	0140000,	"Named Socket"	},
	{	0000000,	NULL		},
};

int
dissect_mode(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree,
char* name)
{
	guint32 mode;
	proto_item* mode_item = NULL;
	proto_tree* mode_tree = NULL;

	mode = tvb_get_ntohl(tvb, offset+0);
	
	if (tree) {
		mode_item = proto_tree_add_text(tree, tvb, offset, 4,
			"%s: 0%o", name, mode);
		if (mode_item)
			mode_tree = proto_item_add_subtree(mode_item, ett_nfs_mode);
	}

	if (mode_tree) {
		proto_tree_add_text(mode_tree, tvb, offset, 4, "%s",
			decode_enumerated_bitfield(mode,  0160000, 16,
			nfs2_mode_names, "%s"));
		proto_tree_add_text(mode_tree, tvb, offset, 4, "%s",
		decode_boolean_bitfield(mode,   04000, 16, "Set user id on exec", "not SUID"));
		proto_tree_add_text(mode_tree, tvb, offset, 4, "%s",
		decode_boolean_bitfield(mode,   02000, 16, "Set group id on exec", "not SGID"));
		proto_tree_add_text(mode_tree, tvb, offset, 4, "%s",
		decode_boolean_bitfield(mode,   01000, 16, "Save swapped text even after use", "not save swapped text"));
		proto_tree_add_text(mode_tree, tvb, offset, 4, "%s",
		decode_boolean_bitfield(mode,    0400, 16, "Read permission for owner", "no Read permission for owner"));
		proto_tree_add_text(mode_tree, tvb, offset, 4, "%s",
		decode_boolean_bitfield(mode,    0200, 16, "Write permission for owner", "no Write permission for owner"));
		proto_tree_add_text(mode_tree, tvb, offset, 4, "%s",
		decode_boolean_bitfield(mode,    0100, 16, "Execute permission for owner", "no Execute permission for owner"));
		proto_tree_add_text(mode_tree, tvb, offset, 4, "%s",
		decode_boolean_bitfield(mode,     040, 16, "Read permission for group", "no Read permission for group"));
		proto_tree_add_text(mode_tree, tvb, offset, 4, "%s",
		decode_boolean_bitfield(mode,     020, 16, "Write permission for group", "no Write permission for group"));
		proto_tree_add_text(mode_tree, tvb, offset, 4, "%s",
		decode_boolean_bitfield(mode,     010, 16, "Execute permission for group", "no Execute permission for group"));
		proto_tree_add_text(mode_tree, tvb, offset, 4, "%s",
		decode_boolean_bitfield(mode,      04, 16, "Read permission for others", "no Read permission for others"));
		proto_tree_add_text(mode_tree, tvb, offset, 4, "%s",
		decode_boolean_bitfield(mode,      02, 16, "Write permission for others", "no Write permission for others"));
		proto_tree_add_text(mode_tree, tvb, offset, 4, "%s",
		decode_boolean_bitfield(mode,      01, 16, "Execute permission for others", "no Execute permission for others"));
	}

	offset += 4;
	return offset;
}


/* RFC 1094, Page 15 */
int
dissect_fattr(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, char* name)
{
	proto_item* fattr_item = NULL;
	proto_tree* fattr_tree = NULL;
	int old_offset = offset;

	if (tree) {
		fattr_item = proto_tree_add_text(tree, tvb, offset, -1,
			"%s", name);
		fattr_tree = proto_item_add_subtree(fattr_item, ett_nfs_fattr);
	}

	offset = dissect_ftype(tvb, offset, pinfo, fattr_tree, "type");
	offset = dissect_mode(tvb, offset, pinfo, fattr_tree, "mode");
	offset = dissect_rpc_uint32(tvb, pinfo, fattr_tree, hf_nfs_fattr_nlink, offset);
	offset = dissect_rpc_uint32(tvb, pinfo, fattr_tree, hf_nfs_fattr_uid, offset);
	offset = dissect_rpc_uint32(tvb, pinfo, fattr_tree, hf_nfs_fattr_gid, offset);
	offset = dissect_rpc_uint32(tvb, pinfo, fattr_tree, hf_nfs_fattr_size, offset);
	offset = dissect_rpc_uint32(tvb, pinfo, fattr_tree, hf_nfs_fattr_blocksize, offset);
	offset = dissect_rpc_uint32(tvb, pinfo, fattr_tree, hf_nfs_fattr_rdev, offset);
	offset = dissect_rpc_uint32(tvb, pinfo, fattr_tree, hf_nfs_fattr_blocks, offset);
	offset = dissect_rpc_uint32(tvb, pinfo, fattr_tree, hf_nfs_fattr_fsid, offset);
	offset = dissect_rpc_uint32(tvb, pinfo, fattr_tree, hf_nfs_fattr_fileid, offset);

	offset = dissect_timeval(tvb, offset, pinfo, fattr_tree, hf_nfs_atime, hf_nfs_atime_sec, hf_nfs_atime_usec);
	offset = dissect_timeval(tvb, offset, pinfo, fattr_tree, hf_nfs_mtime, hf_nfs_mtime_sec, hf_nfs_mtime_usec);
	offset = dissect_timeval(tvb, offset, pinfo, fattr_tree, hf_nfs_ctime, hf_nfs_ctime_sec, hf_nfs_ctime_usec);

	/* now we know, that fattr is shorter */
	if (fattr_item) {
		proto_item_set_len(fattr_item, offset - old_offset);
	}

	return offset;
}


/* RFC 1094, Page 17 */
int
dissect_sattr(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, char* name)
{
	proto_item* sattr_item = NULL;
	proto_tree* sattr_tree = NULL;
	int old_offset = offset;

	if (tree) {
		sattr_item = proto_tree_add_text(tree, tvb, offset, -1,
			"%s", name);
		sattr_tree = proto_item_add_subtree(sattr_item, ett_nfs_sattr);
	}

	if (tvb_get_ntohl(tvb, offset+0) != 0xffffffff)
		offset = dissect_mode(tvb, offset, pinfo, sattr_tree, "mode");
	else {
		proto_tree_add_text(sattr_tree, tvb, offset, 4, "mode: no value");
		offset += 4;
	}

	if (tvb_get_ntohl(tvb, offset+0) != 0xffffffff)
		offset = dissect_rpc_uint32(tvb, pinfo, sattr_tree, hf_nfs_fattr_uid,
			offset);
	else {
		proto_tree_add_text(sattr_tree, tvb, offset, 4, "uid: no value");
		offset += 4;
	}

	if (tvb_get_ntohl(tvb, offset+0) != 0xffffffff)
		offset = dissect_rpc_uint32(tvb, pinfo, sattr_tree, hf_nfs_fattr_gid,
			offset);
	else {
		proto_tree_add_text(sattr_tree, tvb, offset, 4, "gid: no value");
		offset += 4;
	}

	if (tvb_get_ntohl(tvb, offset+0) != 0xffffffff)
		offset = dissect_rpc_uint32(tvb, pinfo, sattr_tree, hf_nfs_fattr_size,
			offset);
	else {
		proto_tree_add_text(sattr_tree, tvb, offset, 4, "size: no value");
		offset += 4;
	}

	if (tvb_get_ntohl(tvb, offset+0) != 0xffffffff) {
		offset = dissect_timeval(tvb, offset, pinfo, sattr_tree, hf_nfs_atime, hf_nfs_atime_sec, hf_nfs_atime_usec);
	} else {
		proto_tree_add_text(sattr_tree, tvb, offset, 8, "atime: no value");
		offset += 8;
	}

	if (tvb_get_ntohl(tvb, offset+0) != 0xffffffff) {
		offset = dissect_timeval(tvb, offset, pinfo, sattr_tree, hf_nfs_mtime, hf_nfs_mtime_sec, hf_nfs_mtime_usec);
	} else {
		proto_tree_add_text(sattr_tree, tvb, offset, 8, "mtime: no value");
		offset += 8;
	}

	/* now we know, that sattr is shorter */
	if (sattr_item) {
		proto_item_set_len(sattr_item, offset - old_offset);
	}

	return offset;
}


/* RFC 1094, Page 17 */
int
dissect_filename(tvbuff_t *tvb, int offset, packet_info *pinfo,
    proto_tree *tree, int hf, char **string_ret)
{
	offset = dissect_rpc_string(tvb, pinfo, tree, hf, offset, string_ret);
	return offset;
}


/* RFC 1094, Page 17 */
int
dissect_path(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, int hf)
{
	offset = dissect_rpc_string(tvb, pinfo, tree, hf, offset, NULL);
	return offset;
}


/* RFC 1094, Page 17,18 */
int
dissect_attrstat(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree){
	guint32 status;

	offset = dissect_stat(tvb, offset, pinfo, tree, &status);
	switch (status) {
		case 0:
			offset = dissect_fattr(tvb, offset, pinfo, tree, "attributes");
		break;
		default:
			/* do nothing */
		break;
	}

	return offset;
}


/* RFC 1094, Page 17,18 */
int
dissect_nfs2_attrstat_reply(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree* tree)
{
	offset = dissect_attrstat(tvb, offset, pinfo, tree);

	return offset;
}


/* RFC 1094, Page 18 */
int
dissect_diropargs(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, char* name)
{
	proto_item* diropargs_item = NULL;
	proto_tree* diropargs_tree = NULL;
	int old_offset = offset;

	if (tree) {
		diropargs_item = proto_tree_add_text(tree, tvb, offset, -1,
			"%s", name);
		diropargs_tree = proto_item_add_subtree(diropargs_item, ett_nfs_diropargs);
	}

	/* are we snooping fh to filenames ?*/
	if((!pinfo->fd->flags.visited) && nfs_file_name_snooping){
		/* v2 LOOKUP, CREATE, MKDIR calls might give us a mapping*/
		rpc_call_info_value *civ=pinfo->private_data;

		if( (civ->prog==100003)
		  &&(civ->vers==2)
		  &&(civ->request)
		  &&((civ->proc==4)||(civ->proc==9)||(civ->proc==14))
		) {
			nfs_name_snoop_add_name(civ->xid, tvb, 
				offset+36, tvb_get_ntohl(tvb, offset+32),
				offset, 32, NULL);
		}
	}

	offset = dissect_fhandle (tvb,offset,pinfo,diropargs_tree,"dir");
	offset = dissect_filename(tvb,offset,pinfo,diropargs_tree,hf_nfs_name,NULL);

	/* now we know, that diropargs is shorter */
	if (diropargs_item) {
		proto_item_set_len(diropargs_item, offset - old_offset);
	}

	return offset;
}


/* RFC 1094, Page 18 */
int
dissect_nfs2_diropargs_call(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree)
{
	offset = dissect_diropargs(tvb, offset, pinfo, tree, "where");

	return offset;
}


/* RFC 1094, Page 18 */
int
dissect_diropres(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree)
{
	guint32	status;

	offset = dissect_stat(tvb, offset, pinfo, tree, &status);
	switch (status) {
		case 0:
			offset = dissect_fhandle(tvb, offset, pinfo, tree, "file");
			offset = dissect_fattr  (tvb, offset, pinfo, tree, "attributes");
		break;
		default:
			/* do nothing */
		break;
	}

	return offset;
}


/* nfsdata is simply a chunk of RPC opaque data (length, data, fill bytes) */
int
dissect_nfsdata(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree *tree, int hf)
{
	offset = dissect_rpc_data(tvb, pinfo, tree, hf, offset);
	return offset;
}


/* RFC 1094, Page 18 */
int
dissect_nfs2_diropres_reply(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree* tree)
{
	offset = dissect_diropres(tvb, offset, pinfo, tree);
	return offset;
}


/* RFC 1094, Page 6 */
int
dissect_nfs2_setattr_call(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree *tree)
{
	offset = dissect_fhandle(tvb, offset, pinfo, tree, "file"      );
	offset = dissect_sattr  (tvb, offset, pinfo, tree, "attributes");

	return offset;
}


/* RFC 1094, Page 6 */
int
dissect_nfs2_readlink_reply(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree *tree)
{
	guint32	status;

	offset = dissect_stat(tvb, offset, pinfo, tree, &status);
	switch (status) {
		case 0:
			offset = dissect_path(tvb, offset, pinfo, tree, hf_nfs_readlink_data);
		break;
		default:
			/* do nothing */
		break;
	}

	return offset;
}


/* RFC 1094, Page 7 */
int
dissect_nfs2_read_call(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree *tree)
{
	guint32 offset_value;
	guint32 count;
	guint32 totalcount;

	offset = dissect_fhandle(tvb, offset, pinfo, tree, "file"      );
	offset_value = tvb_get_ntohl(tvb, offset+0);
	count        = tvb_get_ntohl(tvb, offset+4);
	totalcount   = tvb_get_ntohl(tvb, offset+8);
	if (tree) {
		proto_tree_add_uint(tree, hf_nfs_read_offset, tvb, 
		offset+0, 4, offset_value);
		proto_tree_add_uint(tree, hf_nfs_read_count, tvb, 
		offset+4, 4, count);
		proto_tree_add_uint(tree, hf_nfs_read_totalcount, tvb, 
		offset+8, 4, totalcount);
	}
	offset += 12;

	return offset;
}


/* RFC 1094, Page 7 */
int
dissect_nfs2_read_reply(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree* tree)
{
	guint32 status;

	offset = dissect_stat(tvb, offset, pinfo, tree, &status);
	switch (status) {
		case 0:
			offset = dissect_fattr(tvb, offset, pinfo, tree, "attributes");
			offset = dissect_nfsdata(tvb, offset, pinfo, tree, hf_nfs_data); 
		break;
		default:
			/* do nothing */
		break;
	}

	return offset;
}


/* RFC 1094, Page 8 */
int
dissect_nfs2_write_call(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree *tree)
{
	guint32 beginoffset;
	guint32 offset_value;
	guint32 totalcount;

	offset = dissect_fhandle(tvb, offset, pinfo, tree, "file"      );
	beginoffset  = tvb_get_ntohl(tvb, offset+0);
	offset_value = tvb_get_ntohl(tvb, offset+4);
	totalcount   = tvb_get_ntohl(tvb, offset+8);
	if (tree) {
		proto_tree_add_uint(tree, hf_nfs_write_beginoffset, tvb, 
		offset+0, 4, beginoffset);
		proto_tree_add_uint(tree, hf_nfs_write_offset, tvb, 
		offset+4, 4, offset_value);
		proto_tree_add_uint(tree, hf_nfs_write_totalcount, tvb, 
		offset+8, 4, totalcount);
	}
	offset += 12;

	offset = dissect_nfsdata(tvb, offset, pinfo, tree, hf_nfs_data); 

	return offset;
}


/* RFC 1094, Page 8 */
int
dissect_nfs2_createargs_call(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree *tree)
{
	offset = dissect_diropargs(tvb, offset, pinfo, tree, "where"     );
	offset = dissect_sattr    (tvb, offset, pinfo, tree, "attributes");

	return offset;
}


/* RFC 1094, Page 9 */
int
dissect_nfs2_rename_call(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree *tree)
{
	offset = dissect_diropargs(tvb, offset, pinfo, tree, "from");
	offset = dissect_diropargs(tvb, offset, pinfo, tree, "to"  );

	return offset;
}


/* RFC 1094, Page 9 */
int
dissect_nfs2_link_call(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree *tree)
{
	offset = dissect_fhandle  (tvb, offset, pinfo, tree, "from");
	offset = dissect_diropargs(tvb, offset, pinfo, tree, "to"  );

	return offset;
}


/* RFC 1094, Page 10 */
int
dissect_nfs2_symlink_call(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree *tree)
{
	offset = dissect_diropargs(tvb, offset, pinfo, tree, "from"           );
	offset = dissect_path     (tvb, offset, pinfo, tree, hf_nfs_symlink_to);
	offset = dissect_sattr    (tvb, offset, pinfo, tree, "attributes"     );

	return offset;
}


/* RFC 1094, Page 11 */
int
dissect_nfs2_readdir_call(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree *tree)
{
	guint32	cookie;
	guint32	count;

	offset = dissect_fhandle (tvb, offset, pinfo, tree, "dir");
	cookie  = tvb_get_ntohl(tvb, offset+ 0);
	count = tvb_get_ntohl(tvb, offset+ 4);
	if (tree) {
		proto_tree_add_uint(tree, hf_nfs_readdir_cookie, tvb,
			offset+ 0, 4, cookie);
		proto_tree_add_uint(tree, hf_nfs_readdir_count, tvb,
			offset+ 4, 4, count);
	}
	offset += 8;

	return offset;
}


/* RFC 1094, Page 11 */
int
dissect_readdir_entry(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree* tree)
{
	proto_item* entry_item = NULL;
	proto_tree* entry_tree = NULL;
	int old_offset = offset;
	guint32 fileid;
	guint32 cookie;
	char *name;

	if (tree) {
		entry_item = proto_tree_add_item(tree, hf_nfs_readdir_entry, tvb,
			offset+0, -1, FALSE);
		entry_tree = proto_item_add_subtree(entry_item, ett_nfs_readdir_entry);
	}

	fileid = tvb_get_ntohl(tvb, offset + 0);
	if (entry_tree)
		proto_tree_add_uint(entry_tree, hf_nfs_readdir_entry_fileid, tvb,
			offset+0, 4, fileid);
	offset += 4;

	offset = dissect_filename(tvb, offset, pinfo, entry_tree,
		hf_nfs_readdir_entry_name, &name);
	if (entry_item)
		proto_item_set_text(entry_item, "Entry: file ID %u, name %s",
		fileid, name);
	g_free(name);
	
	cookie = tvb_get_ntohl(tvb, offset + 0);
	if (entry_tree)
		proto_tree_add_uint(entry_tree, hf_nfs_readdir_entry_cookie, tvb,
			offset+0, 4, cookie);
	offset += 4;

	/* now we know, that a readdir entry is shorter */
	if (entry_item) {
		proto_item_set_len(entry_item, offset - old_offset);
	}

	return offset;
}

/* RFC 1094, Page 11 */
int
dissect_nfs2_readdir_reply(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree* tree)
{
	guint32 status;
	guint32 eof_value;

	offset = dissect_stat(tvb, offset, pinfo, tree, &status);
	switch (status) {
		case 0:
			offset = dissect_rpc_list(tvb, pinfo, tree, offset, 
				dissect_readdir_entry);
			eof_value = tvb_get_ntohl(tvb, offset+0);
			if (tree)
				proto_tree_add_uint(tree, hf_nfs_readdir_eof, tvb,
					offset+ 0, 4, eof_value);
			offset += 4;
		break;
		default:
			/* do nothing */
		break;
	}

	return offset;
}


/* RFC 1094, Page 12 */
int
dissect_nfs2_statfs_reply(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree* tree)
{
	guint32 status;
	guint32 tsize;
	guint32 bsize;
	guint32 blocks;
	guint32 bfree;
	guint32 bavail;

	offset = dissect_stat(tvb, offset, pinfo, tree, &status);
	switch (status) {
		case 0:
			tsize  = tvb_get_ntohl(tvb, offset+ 0);
			bsize  = tvb_get_ntohl(tvb, offset+ 4);
			blocks = tvb_get_ntohl(tvb, offset+ 8);
			bfree  = tvb_get_ntohl(tvb, offset+12);
			bavail = tvb_get_ntohl(tvb, offset+16);
			if (tree) {
				proto_tree_add_uint(tree, hf_nfs_statfs_tsize, tvb,
					offset+ 0, 4, tsize);
				proto_tree_add_uint(tree, hf_nfs_statfs_bsize, tvb,
					offset+ 4, 4, bsize);
				proto_tree_add_uint(tree, hf_nfs_statfs_blocks, tvb,
					offset+ 8, 4, blocks);
				proto_tree_add_uint(tree, hf_nfs_statfs_bfree, tvb,
					offset+12, 4, bfree);
				proto_tree_add_uint(tree, hf_nfs_statfs_bavail, tvb,
					offset+16, 4, bavail);
			}
			offset += 20;
		break;
		default:
			/* do nothing */
		break;
	}

	return offset;
}


/* proc number, "proc name", dissect_request, dissect_reply */
/* NULL as function pointer means: type of arguments is "void". */
static const vsff nfs2_proc[] = {
	{ 0,	"NULL",		/* OK */
	NULL,				NULL },
	{ 1,	"GETATTR",	/* OK */
	dissect_nfs2_fhandle_call,	dissect_nfs2_attrstat_reply },
	{ 2,	"SETATTR",	/* OK */
	dissect_nfs2_setattr_call,	dissect_nfs2_attrstat_reply },
	{ 3,	"ROOT",		/* OK */
	NULL,				NULL },
	{ 4,	"LOOKUP",	/* OK */
	dissect_nfs2_diropargs_call,	dissect_nfs2_diropres_reply },
	{ 5,	"READLINK",	/* OK */
	dissect_nfs2_fhandle_call,	dissect_nfs2_readlink_reply },
	{ 6,	"READ",		/* OK */
	dissect_nfs2_read_call,		dissect_nfs2_read_reply },
	{ 7,	"WRITECACHE",	/* OK */
	NULL,				NULL },
	{ 8,	"WRITE",	/* OK */
	dissect_nfs2_write_call,	dissect_nfs2_attrstat_reply },
	{ 9,	"CREATE",	/* OK */
	dissect_nfs2_createargs_call,	dissect_nfs2_diropres_reply },
	{ 10,	"REMOVE",	/* OK */
	dissect_nfs2_diropargs_call,	dissect_nfs2_stat_reply },
	{ 11,	"RENAME",	/* OK */
	dissect_nfs2_rename_call,	dissect_nfs2_stat_reply },
	{ 12,	"LINK",		/* OK */
	dissect_nfs2_link_call,		dissect_nfs2_stat_reply },
	{ 13,	"SYMLINK",	/* OK */
	dissect_nfs2_symlink_call,	dissect_nfs2_stat_reply },
	{ 14,	"MKDIR",	/* OK */
	dissect_nfs2_createargs_call,	dissect_nfs2_diropres_reply },
	{ 15,	"RMDIR",	/* OK */
	dissect_nfs2_diropargs_call,	dissect_nfs2_stat_reply },
	{ 16,	"READDIR",	/* OK */
	dissect_nfs2_readdir_call,	dissect_nfs2_readdir_reply },
	{ 17,	"STATFS",	/* OK */
	dissect_nfs2_fhandle_call,	dissect_nfs2_statfs_reply },
	{ 0,NULL,NULL,NULL }
};
/* end of NFS Version 2 */


/***************************/
/* NFS Version 3, RFC 1813 */
/***************************/


/* RFC 1813, Page 15 */
int
dissect_filename3(tvbuff_t *tvb, int offset, packet_info *pinfo,
    proto_tree *tree, int hf, char **string_ret)
{
	offset = dissect_rpc_string(tvb, pinfo, tree, hf, offset, string_ret);
	return offset;
}


/* RFC 1813, Page 15 */
int
dissect_nfspath3(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree *tree, int hf)
{
	offset = dissect_rpc_string(tvb, pinfo, tree, hf, offset, NULL);
	return offset;
}

/* RFC 1813, Page 15 */
int
dissect_cookieverf3(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree *tree)
{
	proto_tree_add_text(tree, tvb, offset, NFS3_COOKIEVERFSIZE,
		"Verifier: Opaque Data");
	offset += NFS3_COOKIEVERFSIZE;
	return offset;
}


/* RFC 1813, Page 16 */
int
dissect_createverf3(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree *tree)
{
	proto_tree_add_text(tree, tvb, offset, NFS3_CREATEVERFSIZE,
		"Verifier: Opaque Data");
	offset += NFS3_CREATEVERFSIZE;
	return offset;
}


/* RFC 1813, Page 16 */
int
dissect_writeverf3(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree *tree)
{
	proto_tree_add_text(tree, tvb, offset, NFS3_WRITEVERFSIZE,
		"Verifier: Opaque Data");
	offset += NFS3_WRITEVERFSIZE;
	return offset;
}

/* RFC 1813, Page 16 */
int
dissect_mode3(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree,
	char* name)
{
	guint32 mode3;
	proto_item* mode3_item = NULL;
	proto_tree* mode3_tree = NULL;

	mode3 = tvb_get_ntohl(tvb, offset+0);
	
	if (tree) {
		mode3_item = proto_tree_add_text(tree, tvb, offset, 4,
			"%s: 0%o", name, mode3);
		if (mode3_item)
			mode3_tree = proto_item_add_subtree(mode3_item, ett_nfs_mode3);
	}

	/* RFC 1813, Page 23 */
	if (mode3_tree) {
		proto_tree_add_text(mode3_tree, tvb, offset, 4, "%s",
		decode_boolean_bitfield(mode3,   0x800, 12, "Set user id on exec", "not SUID"));
		proto_tree_add_text(mode3_tree, tvb, offset, 4, "%s",
		decode_boolean_bitfield(mode3,   0x400, 12, "Set group id on exec", "not SGID"));
		proto_tree_add_text(mode3_tree, tvb, offset, 4, "%s",
		decode_boolean_bitfield(mode3,   0x200, 12, "Save swapped text even after use", "not save swapped text"));
		proto_tree_add_text(mode3_tree, tvb, offset, 4, "%s",
		decode_boolean_bitfield(mode3,   0x100, 12, "Read permission for owner", "no Read permission for owner"));
		proto_tree_add_text(mode3_tree, tvb, offset, 4, "%s",
		decode_boolean_bitfield(mode3,    0x80, 12, "Write permission for owner", "no Write permission for owner"));
		proto_tree_add_text(mode3_tree, tvb, offset, 4, "%s",
		decode_boolean_bitfield(mode3,    0x40, 12, "Execute permission for owner", "no Execute permission for owner"));
		proto_tree_add_text(mode3_tree, tvb, offset, 4, "%s",
		decode_boolean_bitfield(mode3,    0x20, 12, "Read permission for group", "no Read permission for group"));
		proto_tree_add_text(mode3_tree, tvb, offset, 4, "%s",
		decode_boolean_bitfield(mode3,    0x10, 12, "Write permission for group", "no Write permission for group"));
		proto_tree_add_text(mode3_tree, tvb, offset, 4, "%s",
		decode_boolean_bitfield(mode3,     0x8, 12, "Execute permission for group", "no Execute permission for group"));
		proto_tree_add_text(mode3_tree, tvb, offset, 4, "%s",
		decode_boolean_bitfield(mode3,     0x4, 12, "Read permission for others", "no Read permission for others"));
		proto_tree_add_text(mode3_tree, tvb, offset, 4, "%s",
		decode_boolean_bitfield(mode3,     0x2, 12, "Write permission for others", "no Write permission for others"));
		proto_tree_add_text(mode3_tree, tvb, offset, 4, "%s",
		decode_boolean_bitfield(mode3,     0x1, 12, "Execute permission for others", "no Execute permission for others"));
	}

	offset += 4;
	return offset;
}

/* RFC 1813, Page 16,17 */
const value_string names_nfs_nfsstat3[] =
{
	{	0,	"OK" },
	{	1,	"ERR_PERM" },
	{	2,	"ERR_NOENT" },
	{	5,	"ERR_IO" },
	{	6,	"ERR_NX_IO" },
	{	13,	"ERR_ACCES" },
	{	17,	"ERR_EXIST" },
	{	18,	"ERR_XDEV" },
	{	19,	"ERR_NODEV" },
	{	20,	"ERR_NOTDIR" },
	{	21,	"ERR_ISDIR" },
	{	22,	"ERR_INVAL" },
	{	27,	"ERR_FBIG" },
	{	28,	"ERR_NOSPC" },
	{	30,	"ERR_ROFS" },
	{	31,	"ERR_MLINK" },
	{	63,	"ERR_NAMETOOLONG" },
	{	66,	"ERR_NOTEMPTY" },
	{	69,	"ERR_DQUOT" },
	{	70,	"ERR_STALE" },
	{	71,	"ERR_REMOTE" },
	{	10001,	"ERR_BADHANDLE" },
	{	10002,	"ERR_NOT_SYNC" },
	{	10003,	"ERR_BAD_COOKIE" },
	{	10004,	"ERR_NOTSUPP" },
	{	10005,	"ERR_TOOSMALL" },
	{	10006,	"ERR_SERVERFAULT" },
	{	10007,	"ERR_BADTYPE" },
	{	10008,	"ERR_JUKEBOX" },
	{	0,	NULL }
};


/* RFC 1813, Page 16 */
int
dissect_nfsstat3(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree *tree,guint32 *status)
{
	guint32 nfsstat3;

	nfsstat3 = tvb_get_ntohl(tvb, offset+0);
	
	if (tree) {
		proto_tree_add_uint(tree, hf_nfs_nfsstat3, tvb,
			offset, 4, nfsstat3);
	}

	offset += 4;
	*status = nfsstat3;
	return offset;
}


const value_string names_nfs_ftype3[] =
{
	{	NF3REG,	"Regular File" },
	{	NF3DIR,	"Directory" },
	{	NF3BLK,	"Block Special Device" },
	{	NF3CHR,	"Character Special Device" },
	{	NF3LNK,	"Symbolic Link" },
	{	NF3SOCK,"Socket" },
	{	NF3FIFO,"Named Pipe" },
	{	0,	NULL }
};


/* RFC 1813, Page 20 */
int
dissect_ftype3(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree,
	int hf, guint32* ftype3)
{
	guint32 type;

	type = tvb_get_ntohl(tvb, offset+0);
	
	if (tree) {
		proto_tree_add_uint(tree, hf, tvb, offset, 4, type);
	}

	offset += 4;
	*ftype3 = type;
	return offset;
}


/* RFC 1813, Page 20 */
int
dissect_specdata3(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree *tree, char* name)
{
	guint32	specdata1;
	guint32	specdata2;

	proto_item* specdata3_item;
	proto_tree* specdata3_tree = NULL;

	specdata1 = tvb_get_ntohl(tvb, offset+0);
	specdata2 = tvb_get_ntohl(tvb, offset+4);
	
	if (tree) {
		specdata3_item = proto_tree_add_text(tree, tvb, offset, 8,
			"%s: %u,%u", name, specdata1, specdata2);
		if (specdata3_item)
			specdata3_tree = proto_item_add_subtree(specdata3_item,
					ett_nfs_specdata3);
	}

	if (specdata3_tree) {
		proto_tree_add_text(specdata3_tree, tvb,offset+0,4,
					"specdata1: %u", specdata1);
		proto_tree_add_text(specdata3_tree, tvb,offset+4,4,
					"specdata2: %u", specdata2);
	}

	offset += 8;
	return offset;
}


/* RFC 1813, Page 21 */
int
dissect_nfs_fh3(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree *tree, char *name)
{
	guint fh3_len;
	guint fh3_len_full;
	guint fh3_fill;
	proto_item* fitem = NULL;
	proto_tree* ftree = NULL;
	int fh_offset,fh_length;

	fh3_len = tvb_get_ntohl(tvb, offset+0);
	fh3_len_full = rpc_roundup(fh3_len);
	fh3_fill = fh3_len_full - fh3_len;
	
	if (tree) {
		fitem = proto_tree_add_text(tree, tvb, offset, 4+fh3_len_full,
			"%s", name);
		if (fitem)
			ftree = proto_item_add_subtree(fitem, ett_nfs_fh3);
	}

	/* are we snooping fh to filenames ?*/
	if((!pinfo->fd->flags.visited) && nfs_file_name_snooping){
		rpc_call_info_value *civ=pinfo->private_data;

		/* NFS v3 LOOKUP, CREATE, MKDIR calls might give us a mapping*/
		if( (civ->prog==100003)
		  &&(civ->vers==3)
		  &&(!civ->request)
		  &&((civ->proc==3)||(civ->proc==8)||(civ->proc==9))
		) {
			fh_length=tvb_get_ntohl(tvb, offset);
			fh_offset=offset+4;
			nfs_name_snoop_add_fh(civ->xid, tvb, 
				fh_offset, fh_length);
		}

		/* MOUNT v3 MNT replies might give us a filehandle */
		if( (civ->prog==100005)
		  &&(civ->vers==3)
		  &&(!civ->request)
		  &&(civ->proc==1)
		) {
			fh_length=tvb_get_ntohl(tvb, offset);
			fh_offset=offset+4;
			nfs_name_snoop_add_fh(civ->xid, tvb, 
				fh_offset, fh_length);
		}
	}

	proto_tree_add_uint(ftree, hf_nfs_fh_length, tvb, offset+0, 4,
			fh3_len);
	dissect_fhandle_data(tvb, offset+4, pinfo, ftree, fh3_len);

	offset += 4 + fh3_len_full;
	return offset;
}


/* RFC 1813, Page 21 */
int
dissect_nfstime3(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree *tree, int hf_time, int hf_time_sec, int hf_time_nsec)
{
	guint32	seconds;
	guint32 nseconds;
	nstime_t ts;

	proto_item* time_item;
	proto_tree* time_tree = NULL;

	seconds = tvb_get_ntohl(tvb, offset+0);
	nseconds = tvb_get_ntohl(tvb, offset+4);
	ts.secs = seconds;
	ts.nsecs = nseconds;
	
	if (tree) {
		time_item = proto_tree_add_time(tree, hf_time, tvb, offset, 8,
				&ts);
		if (time_item)
			time_tree = proto_item_add_subtree(time_item, ett_nfs_nfstime3);
	}

	if (time_tree) {
		proto_tree_add_uint(time_tree, hf_time_sec, tvb, offset, 4,
					seconds);
		proto_tree_add_uint(time_tree, hf_time_nsec, tvb, offset+4, 4,
					nseconds);
	}
	offset += 8;
	return offset;
}


/* RFC 1813, Page 22 */
int
dissect_fattr3(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree,
	char* name)
{
	proto_item* fattr3_item = NULL;
	proto_tree* fattr3_tree = NULL;
	int old_offset = offset;
	guint32 type;

	if (tree) {
		fattr3_item = proto_tree_add_text(tree, tvb, offset, -1,
			"%s", name);
		fattr3_tree = proto_item_add_subtree(fattr3_item, ett_nfs_fattr3);
	}

	offset = dissect_ftype3(tvb,offset,pinfo,fattr3_tree,hf_nfs_fattr3_type,&type);
	offset = dissect_mode3(tvb,offset,pinfo,fattr3_tree,"mode");
	offset = dissect_rpc_uint32(tvb, pinfo, fattr3_tree, hf_nfs_fattr3_nlink,
		offset);
	offset = dissect_rpc_uint32(tvb, pinfo, fattr3_tree, hf_nfs_fattr3_uid,
		offset);
	offset = dissect_rpc_uint32(tvb, pinfo, fattr3_tree, hf_nfs_fattr3_gid,
		offset);
	offset = dissect_rpc_uint64(tvb, pinfo, fattr3_tree, hf_nfs_fattr3_size, 
		offset);
	offset = dissect_rpc_uint64(tvb, pinfo, fattr3_tree, hf_nfs_fattr3_used,
		offset);
	offset = dissect_specdata3(tvb,offset,pinfo,fattr3_tree,"rdev");
	offset = dissect_rpc_uint64(tvb, pinfo, fattr3_tree, hf_nfs_fattr3_fsid,
		offset);
	offset = dissect_rpc_uint64(tvb, pinfo, fattr3_tree, hf_nfs_fattr3_fileid,
		offset);
	offset = dissect_nfstime3 (tvb,offset,pinfo,fattr3_tree,hf_nfs_atime,hf_nfs_atime_sec,hf_nfs_atime_nsec);
	offset = dissect_nfstime3 (tvb,offset,pinfo,fattr3_tree,hf_nfs_mtime,hf_nfs_mtime_sec,hf_nfs_mtime_nsec);
	offset = dissect_nfstime3 (tvb,offset,pinfo,fattr3_tree,hf_nfs_ctime,hf_nfs_ctime_sec,hf_nfs_ctime_nsec);

	/* now we know, that fattr3 is shorter */
	if (fattr3_item) {
		proto_item_set_len(fattr3_item, offset - old_offset);
	}

	return offset;
}


const value_string value_follows[] =
	{
		{ 0, "no value" },
		{ 1, "value follows"},
		{ 0, NULL }
	};


/* RFC 1813, Page 23 */
int
dissect_post_op_attr(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree *tree, char* name)
{
	proto_item* post_op_attr_item = NULL;
	proto_tree* post_op_attr_tree = NULL;
	int old_offset = offset;
	guint32 attributes_follow;

	if (tree) {
		post_op_attr_item = proto_tree_add_text(tree, tvb, offset, -1,
			"%s", name);
		post_op_attr_tree = proto_item_add_subtree(post_op_attr_item, 
			ett_nfs_post_op_attr);
	}

	attributes_follow = tvb_get_ntohl(tvb, offset+0);
	proto_tree_add_text(post_op_attr_tree, tvb, offset, 4,
		"attributes_follow: %s (%u)", 
		val_to_str(attributes_follow,value_follows,"Unknown"), attributes_follow);
	offset += 4;
	switch (attributes_follow) {
		case TRUE:
			offset = dissect_fattr3(tvb, offset, pinfo, post_op_attr_tree,
					"attributes");
		break;
		case FALSE:
			/* void */
		break;
	}
	
	/* now we know, that post_op_attr_tree is shorter */
	if (post_op_attr_item) {
		proto_item_set_len(post_op_attr_item, offset - old_offset);
	}

	return offset;
}


/* RFC 1813, Page 24 */
int
dissect_wcc_attr(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree *tree, char* name)
{
	proto_item* wcc_attr_item = NULL;
	proto_tree* wcc_attr_tree = NULL;
	int old_offset = offset;

	if (tree) {
		wcc_attr_item = proto_tree_add_text(tree, tvb, offset, -1,
			"%s", name);
		wcc_attr_tree = proto_item_add_subtree(wcc_attr_item, 
			ett_nfs_wcc_attr);
	}

	offset = dissect_rpc_uint64(tvb, pinfo, wcc_attr_tree, hf_nfs_wcc_attr_size, 
		offset);
	offset = dissect_nfstime3(tvb, offset, pinfo, wcc_attr_tree, hf_nfs_mtime, hf_nfs_mtime_sec, hf_nfs_mtime_nsec);
	offset = dissect_nfstime3(tvb, offset, pinfo, wcc_attr_tree, hf_nfs_ctime, hf_nfs_ctime_sec, hf_nfs_ctime_nsec);
	/* now we know, that wcc_attr_tree is shorter */
	if (wcc_attr_item) {
		proto_item_set_len(wcc_attr_item, offset - old_offset);
	}

	return offset;
}


/* RFC 1813, Page 24 */
int
dissect_pre_op_attr(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree *tree, char* name)
{
	proto_item* pre_op_attr_item = NULL;
	proto_tree* pre_op_attr_tree = NULL;
	int old_offset = offset;
	guint32 attributes_follow;

	if (tree) {
		pre_op_attr_item = proto_tree_add_text(tree, tvb, offset, -1,
			"%s", name);
		pre_op_attr_tree = proto_item_add_subtree(pre_op_attr_item, 
			ett_nfs_pre_op_attr);
	}

	attributes_follow = tvb_get_ntohl(tvb, offset+0);
	proto_tree_add_text(pre_op_attr_tree, tvb, offset, 4,
		"attributes_follow: %s (%u)", 
		val_to_str(attributes_follow,value_follows,"Unknown"), attributes_follow);
	offset += 4;
	switch (attributes_follow) {
		case TRUE:
			offset = dissect_wcc_attr(tvb, offset, pinfo, pre_op_attr_tree,
					"attributes");
		break;
		case FALSE:
			/* void */
		break;
	}
	
	/* now we know, that pre_op_attr_tree is shorter */
	if (pre_op_attr_item) {
		proto_item_set_len(pre_op_attr_item, offset - old_offset);
	}

	return offset;
}


/* RFC 1813, Page 24 */
int
dissect_wcc_data(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree *tree, char* name)
{
	proto_item* wcc_data_item = NULL;
	proto_tree* wcc_data_tree = NULL;
	int old_offset = offset;

	if (tree) {
		wcc_data_item = proto_tree_add_text(tree, tvb, offset, -1,
			"%s", name);
		wcc_data_tree = proto_item_add_subtree(wcc_data_item, 
			ett_nfs_wcc_data);
	}

	offset = dissect_pre_op_attr (tvb, offset, pinfo, wcc_data_tree, "before");
	offset = dissect_post_op_attr(tvb, offset, pinfo, wcc_data_tree, "after" );

	/* now we know, that wcc_data is shorter */
	if (wcc_data_item) {
		proto_item_set_len(wcc_data_item, offset - old_offset);
	}

	return offset;
}


/* RFC 1813, Page 25 */
int
dissect_post_op_fh3(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree *tree, char* name)
{
	proto_item* post_op_fh3_item = NULL;
	proto_tree* post_op_fh3_tree = NULL;
	int old_offset = offset;
	guint32 handle_follows;

	if (tree) {
		post_op_fh3_item = proto_tree_add_text(tree, tvb, offset, -1,
			"%s", name);
		post_op_fh3_tree = proto_item_add_subtree(post_op_fh3_item, 
			ett_nfs_post_op_fh3);
	}

	handle_follows = tvb_get_ntohl(tvb, offset+0);
	proto_tree_add_text(post_op_fh3_tree, tvb, offset, 4,
		"handle_follows: %s (%u)", 
		val_to_str(handle_follows,value_follows,"Unknown"), handle_follows);
	offset += 4;
	switch (handle_follows) {
		case TRUE:
			offset = dissect_nfs_fh3(tvb, offset, pinfo, post_op_fh3_tree,
					"handle");
		break;
		case FALSE:
			/* void */
		break;
	}
	
	/* now we know, that post_op_fh3_tree is shorter */
	if (post_op_fh3_item) {
		proto_item_set_len(post_op_fh3_item, offset - old_offset);
	}

	return offset;
}


/* RFC 1813, Page 25 */
int
dissect_set_mode3(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree *tree, char* name)
{
	proto_item* set_mode3_item = NULL;
	proto_tree* set_mode3_tree = NULL;
	int old_offset = offset;
	guint32 set_it;
	char* set_it_name;

	set_it = tvb_get_ntohl(tvb, offset+0);
	set_it_name = val_to_str(set_it,value_follows,"Unknown");

	if (tree) {
		set_mode3_item = proto_tree_add_text(tree, tvb, offset, -1,
			"%s: %s", name, set_it_name);
		set_mode3_tree = proto_item_add_subtree(set_mode3_item, 
			ett_nfs_set_mode3);
	}

	if (set_mode3_tree)
		proto_tree_add_text(set_mode3_tree, tvb, offset, 4,
			"set_it: %s (%u)", set_it_name, set_it);

	offset += 4;

	switch (set_it) {
		case 1:
			offset = dissect_mode3(tvb, offset, pinfo, set_mode3_tree,
					"mode");
		break;
		default:
			/* void */
		break;
	}
	
	/* now we know, that set_mode3 is shorter */
	if (set_mode3_item) {
		proto_item_set_len(set_mode3_item, offset - old_offset);
	}

	return offset;
}


/* RFC 1813, Page 26 */
int
dissect_set_uid3(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree *tree, char* name)
{
	proto_item* set_uid3_item = NULL;
	proto_tree* set_uid3_tree = NULL;
	int old_offset = offset;
	guint32 set_it;
	char* set_it_name;

	set_it = tvb_get_ntohl(tvb, offset+0);
	set_it_name = val_to_str(set_it,value_follows,"Unknown");

	if (tree) {
		set_uid3_item = proto_tree_add_text(tree, tvb, offset, -1,
			"%s: %s", name, set_it_name);
		set_uid3_tree = proto_item_add_subtree(set_uid3_item, 
			ett_nfs_set_uid3);
	}

	if (set_uid3_tree)
		proto_tree_add_text(set_uid3_tree, tvb, offset, 4,
			"set_it: %s (%u)", set_it_name, set_it);

	offset += 4;

	switch (set_it) {
		case 1:
			offset = dissect_rpc_uint32(tvb, pinfo, set_uid3_tree,
								 hf_nfs_uid3, offset);
		break;
		default:
			/* void */
		break;
	}

	/* now we know, that set_uid3 is shorter */
	if (set_uid3_item) {
		proto_item_set_len(set_uid3_item, offset - old_offset);
	}

	return offset;
}


/* RFC 1813, Page 26 */
int
dissect_set_gid3(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree *tree, char* name)
{
	proto_item* set_gid3_item = NULL;
	proto_tree* set_gid3_tree = NULL;
	int old_offset = offset;
	guint32 set_it;
	char* set_it_name;

	set_it = tvb_get_ntohl(tvb, offset+0);
	set_it_name = val_to_str(set_it,value_follows,"Unknown");

	if (tree) {
		set_gid3_item = proto_tree_add_text(tree, tvb, offset, -1,
			"%s: %s", name, set_it_name);
		set_gid3_tree = proto_item_add_subtree(set_gid3_item, 
			ett_nfs_set_gid3);
	}

	if (set_gid3_tree)
		proto_tree_add_text(set_gid3_tree, tvb, offset, 4,
			"set_it: %s (%u)", set_it_name, set_it);

	offset += 4;

	switch (set_it) {
		case 1:
			offset = dissect_rpc_uint32(tvb, pinfo, set_gid3_tree, 
				hf_nfs_gid3, offset);
		break;
		default:
			/* void */
		break;
	}

	/* now we know, that set_gid3 is shorter */
	if (set_gid3_item) {
		proto_item_set_len(set_gid3_item, offset - old_offset);
	}

	return offset;
}


/* RFC 1813, Page 26 */
int
dissect_set_size3(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree *tree, char* name)
{
	proto_item* set_size3_item = NULL;
	proto_tree* set_size3_tree = NULL;
	int old_offset = offset;
	guint32 set_it;
	char* set_it_name;

	set_it = tvb_get_ntohl(tvb, offset+0);
	set_it_name = val_to_str(set_it,value_follows,"Unknown");

	if (tree) {
		set_size3_item = proto_tree_add_text(tree, tvb, offset, -1,
			"%s: %s", name, set_it_name);
		set_size3_tree = proto_item_add_subtree(set_size3_item, 
			ett_nfs_set_size3);
	}

	if (set_size3_tree)
		proto_tree_add_text(set_size3_tree, tvb, offset, 4,
			"set_it: %s (%u)", set_it_name, set_it);

	offset += 4;

	switch (set_it) {
		case 1:
			offset = dissect_rpc_uint64(tvb, pinfo, set_size3_tree,
				hf_nfs_set_size3_size, offset);
		break;
		default:
			/* void */
		break;
	}

	/* now we know, that set_size3 is shorter */
	if (set_size3_item) {
		proto_item_set_len(set_size3_item, offset - old_offset);
	}

	return offset;
}


/* RFC 1813, Page 25 */
#define DONT_CHANGE 0
#define SET_TO_SERVER_TIME 1
#define SET_TO_CLIENT_TIME 2

const value_string time_how[] =
	{
		{ DONT_CHANGE,	"don't change" },
		{ SET_TO_SERVER_TIME, "set to server time" },
		{ SET_TO_CLIENT_TIME, "set to client time" },
		{ 0, NULL }
	};


/* RFC 1813, Page 26 */
int
dissect_set_atime(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree *tree, char* name)
{
	proto_item* set_atime_item = NULL;
	proto_tree* set_atime_tree = NULL;
	int old_offset = offset;
	guint32 set_it;
	char* set_it_name;

	set_it = tvb_get_ntohl(tvb, offset+0);
	set_it_name = val_to_str(set_it,time_how,"Unknown");

	if (tree) {
		set_atime_item = proto_tree_add_text(tree, tvb, offset, -1,
			"%s: %s", name, set_it_name);
		set_atime_tree = proto_item_add_subtree(set_atime_item, 
			ett_nfs_set_atime);
	}

	if (set_atime_tree)
		proto_tree_add_text(set_atime_tree, tvb, offset, 4,
			"set_it: %s (%u)", set_it_name, set_it);

	offset += 4;

	switch (set_it) {
		case SET_TO_CLIENT_TIME:
			if (set_atime_item) {
				offset = dissect_nfstime3(tvb, offset, pinfo, set_atime_tree,
					hf_nfs_atime, hf_nfs_atime_sec, hf_nfs_atime_nsec);
			}
		break;
		default:
			/* void */
		break;
	}

	/* now we know, that set_atime is shorter */
	if (set_atime_item) {
		proto_item_set_len(set_atime_item, offset - old_offset);
	}

	return offset;
}


/* RFC 1813, Page 26 */
int
dissect_set_mtime(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree *tree, char* name)
{
	proto_item* set_mtime_item = NULL;
	proto_tree* set_mtime_tree = NULL;
	int old_offset = offset;
	guint32 set_it;
	char* set_it_name;

	set_it = tvb_get_ntohl(tvb, offset+0);
	set_it_name = val_to_str(set_it,time_how,"Unknown");

	if (tree) {
		set_mtime_item = proto_tree_add_text(tree, tvb, offset, -1,
			"%s: %s", name, set_it_name);
		set_mtime_tree = proto_item_add_subtree(set_mtime_item, 
			ett_nfs_set_mtime);
	}

	if (set_mtime_tree)
		proto_tree_add_text(set_mtime_tree, tvb, offset, 4,
				"set_it: %s (%u)", set_it_name, set_it);

	offset += 4;

	switch (set_it) {
		case SET_TO_CLIENT_TIME:
			if (set_mtime_item) {
				offset = dissect_nfstime3(tvb, offset, pinfo, set_mtime_tree,
					hf_nfs_atime, hf_nfs_atime_sec, hf_nfs_atime_nsec);
			}
		break;
		default:
			/* void */
		break;
	}

	/* now we know, that set_mtime is shorter */
	if (set_mtime_item) {
		proto_item_set_len(set_mtime_item, offset - old_offset);
	}

	return offset;
}


/* RFC 1813, Page 25..27 */
int
dissect_sattr3(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree,
	char* name)
{
	proto_item* sattr3_item = NULL;
	proto_tree* sattr3_tree = NULL;
	int old_offset = offset;

	if (tree) {
		sattr3_item = proto_tree_add_text(tree, tvb, offset, -1,
			"%s", name);
		sattr3_tree = proto_item_add_subtree(sattr3_item, ett_nfs_sattr3);
	}

	offset = dissect_set_mode3(tvb, offset, pinfo, sattr3_tree, "mode");
	offset = dissect_set_uid3 (tvb, offset, pinfo, sattr3_tree, "uid");
	offset = dissect_set_gid3 (tvb, offset, pinfo, sattr3_tree, "gid");
	offset = dissect_set_size3(tvb, offset, pinfo, sattr3_tree, "size");
	offset = dissect_set_atime(tvb, offset, pinfo, sattr3_tree, "atime");
	offset = dissect_set_mtime(tvb, offset, pinfo, sattr3_tree, "mtime");

	/* now we know, that sattr3 is shorter */
	if (sattr3_item) {
		proto_item_set_len(sattr3_item, offset - old_offset);
	}

	return offset;
}


/* RFC 1813, Page 27 */
int
dissect_diropargs3(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree *tree, char* name)
{
	proto_item* diropargs3_item = NULL;
	proto_tree* diropargs3_tree = NULL;
	int old_offset = offset;
	int parent_offset, parent_len;
	int name_offset, name_len;

	if (tree) {
		diropargs3_item = proto_tree_add_text(tree, tvb, offset, -1,
			"%s", name);
		diropargs3_tree = proto_item_add_subtree(diropargs3_item, 
			ett_nfs_diropargs3);
	}

	parent_offset=offset+4;
	parent_len=tvb_get_ntohl(tvb, offset);
	offset = dissect_nfs_fh3(tvb, offset, pinfo, diropargs3_tree, "dir");
	name_offset=offset+4;
	name_len=tvb_get_ntohl(tvb, offset);
	offset = dissect_filename3(tvb, offset, pinfo, diropargs3_tree, 
		hf_nfs_name, NULL);

	/* are we snooping fh to filenames ?*/
	if((!pinfo->fd->flags.visited) && nfs_file_name_snooping){
		/* v3 LOOKUP, CREATE, MKDIR calls might give us a mapping*/
		rpc_call_info_value *civ=pinfo->private_data;

		if( (civ->prog==100003)
		  &&(civ->vers==3)
		  &&(civ->request)
		  &&((civ->proc==3)||(civ->proc==8)||(civ->proc==9))
		) {
			nfs_name_snoop_add_name(civ->xid, tvb, 
				name_offset, name_len,
				parent_offset, parent_len, NULL);
		}
	}


	/* now we know, that diropargs3 is shorter */
	if (diropargs3_item) {
		proto_item_set_len(diropargs3_item, offset - old_offset);
	}

	return offset;
}


/* RFC 1813, Page 27 */
int
dissect_nfs3_diropargs3_call(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree *tree)
{
	offset = dissect_diropargs3(tvb, offset, pinfo, tree, "object");

	return offset;
}


/* RFC 1813, Page 40 */
int
dissect_access(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree,
	char* name)
{
	guint32 access;
	proto_item* access_item = NULL;
	proto_tree* access_tree = NULL;

	access = tvb_get_ntohl(tvb, offset+0);
	
	if (tree) {
		access_item = proto_tree_add_text(tree, tvb, offset, 4,
			"%s: 0x%02x", name, access);
		if (access_item)
			access_tree = proto_item_add_subtree(access_item, ett_nfs_access);
	}

	if (access_tree) {
		proto_tree_add_text(access_tree, tvb, offset, 4, "%s READ",
		decode_boolean_bitfield(access,  0x001, 6, "allow", "not allow"));
		proto_tree_add_text(access_tree, tvb, offset, 4, "%s LOOKUP",
		decode_boolean_bitfield(access,  0x002, 6, "allow", "not allow"));
		proto_tree_add_text(access_tree, tvb, offset, 4, "%s MODIFY",
		decode_boolean_bitfield(access,  0x004, 6, "allow", "not allow"));
		proto_tree_add_text(access_tree, tvb, offset, 4, "%s EXTEND",
		decode_boolean_bitfield(access,  0x008, 6, "allow", "not allow"));
		proto_tree_add_text(access_tree, tvb, offset, 4, "%s DELETE",
		decode_boolean_bitfield(access,  0x010, 6, "allow", "not allow"));
		proto_tree_add_text(access_tree, tvb, offset, 4, "%s EXECUTE",
		decode_boolean_bitfield(access,  0x020, 6, "allow", "not allow"));
	}

	offset += 4;
	return offset;
}


/* NFS3 file handle dissector */
int
dissect_nfs3_nfs_fh3_call(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree* tree)
{
	offset = dissect_nfs_fh3(tvb, offset, pinfo, tree, "object");
	return offset;
}


/* generic NFS3 reply dissector */
int
dissect_nfs3_any_reply(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree* tree)
{
	guint32 status;

	offset = dissect_nfsstat3(tvb, offset, pinfo, tree, &status);

	return offset;

}


/* RFC 1813, Page 32,33 */
int
dissect_nfs3_getattr_call(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree* tree)
{
	offset = dissect_nfs_fh3(tvb, offset, pinfo, tree, "object");
	return offset;
}


/* RFC 1813, Page 32,33 */
int
dissect_nfs3_getattr_reply(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree* tree)
{
	guint32 status;

	offset = dissect_nfsstat3(tvb, offset, pinfo, tree, &status);
	switch (status) {
		case 0:
			offset = dissect_fattr3(tvb, offset, pinfo, tree, "obj_attributes");
		break;
		default:
			/* void */
		break;
	}
		
	return offset;
}


/* RFC 1813, Page 33 */
int
dissect_sattrguard3(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree* tree, char *name)
{
	proto_item* sattrguard3_item = NULL;
	proto_tree* sattrguard3_tree = NULL;
	int old_offset = offset;
	guint32 check;
	char* check_name;

	check = tvb_get_ntohl(tvb, offset+0);
	check_name = val_to_str(check,value_follows,"Unknown");

	if (tree) {
		sattrguard3_item = proto_tree_add_text(tree, tvb, offset, -1,
			"%s: %s", name, check_name);
		sattrguard3_tree = proto_item_add_subtree(sattrguard3_item, 
			ett_nfs_sattrguard3);
	}

	if (sattrguard3_tree)
		proto_tree_add_text(sattrguard3_tree, tvb, offset, 4,
			"check: %s (%u)", check_name, check);

	offset += 4;

	switch (check) {
		case TRUE:
			offset = dissect_nfstime3(tvb, offset, pinfo, sattrguard3_tree,
					hf_nfs_ctime, hf_nfs_ctime_sec, hf_nfs_ctime_nsec);
		break;
		case FALSE:
			/* void */
		break;
	}

	/* now we know, that sattrguard3 is shorter */
	if (sattrguard3_item) {
		proto_item_set_len(sattrguard3_item, offset - old_offset);
	}

	return offset;
}


/* RFC 1813, Page 33..36 */
int
dissect_nfs3_setattr_call(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree* tree)
{
	offset = dissect_nfs_fh3    (tvb, offset, pinfo, tree, "object");
	offset = dissect_sattr3     (tvb, offset, pinfo, tree, "new_attributes");
	offset = dissect_sattrguard3(tvb, offset, pinfo, tree, "guard");
	return offset;
}


/* RFC 1813, Page 33..36 */
int
dissect_nfs3_setattr_reply(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree* tree)
{
	guint32 status;

	offset = dissect_nfsstat3(tvb, offset, pinfo, tree, &status);
	switch (status) {
		case 0:
			offset = dissect_wcc_data(tvb, offset, pinfo, tree, "obj_wcc");
		break;
		default:
			offset = dissect_wcc_data(tvb, offset, pinfo, tree, "obj_wcc");
		break;
	}
		
	return offset;
}


/* RFC 1813, Page 37..39 */
int
dissect_nfs3_lookup_call(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree* tree)
{
	offset = dissect_diropargs3 (tvb, offset, pinfo, tree, "what");
	return offset;
}


/* RFC 1813, Page 37..39 */
int
dissect_nfs3_lookup_reply(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree* tree)
{
	guint32 status;

	offset = dissect_nfsstat3(tvb, offset, pinfo, tree, &status);
	switch (status) {
		case 0:
			offset = dissect_nfs_fh3(tvb, offset, pinfo, tree, "object");
			offset = dissect_post_op_attr(tvb, offset, pinfo, tree, 
				"obj_attributes");
			offset = dissect_post_op_attr(tvb, offset, pinfo, tree, 
				"dir_attributes");
		break;
		default:
			offset = dissect_post_op_attr(tvb, offset, pinfo, tree, 
				"dir_attributes");
		break;
	}
		
	return offset;
}


/* RFC 1813, Page 40..43 */
int
dissect_nfs3_access_call(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree* tree)
{
	offset = dissect_nfs_fh3(tvb, offset, pinfo, tree, "object");
	offset = dissect_access (tvb, offset, pinfo, tree, "access");

	return offset;
}


/* RFC 1813, Page 40..43 */
int
dissect_nfs3_access_reply(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree* tree)
{
	guint32 status;

	offset = dissect_nfsstat3(tvb, offset, pinfo, tree, &status);
	switch (status) {
		case 0:
			offset = dissect_post_op_attr(tvb, offset, pinfo, tree, 
				"obj_attributes");
			offset = dissect_access(tvb, offset, pinfo, tree, "access");
		break;
		default:
			offset = dissect_post_op_attr(tvb, offset, pinfo, tree, 
				"obj_attributes");
		break;
	}
		
	return offset;
}


/* RFC 1813, Page 44,45 */
int
dissect_nfs3_readlink_reply(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree* tree)
{
	guint32 status;

	offset = dissect_nfsstat3(tvb, offset, pinfo, tree, &status);
	switch (status) {
		case 0:
			offset = dissect_post_op_attr(tvb, offset, pinfo, tree, 
				"symlink_attributes");
			offset = dissect_nfspath3(tvb, offset, pinfo, tree, 
				hf_nfs_readlink_data);
		break;
		default:
			offset = dissect_post_op_attr(tvb, offset, pinfo, tree, 
				"symlink_attributes");
		break;
	}
		
	return offset;
}


/* RFC 1813, Page 46..48 */
int
dissect_nfs3_read_call(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree* tree)
{
	offset = dissect_nfs_fh3(tvb, offset, pinfo, tree, "file");
	offset = dissect_rpc_uint64(tvb, pinfo, tree, hf_nfs_offset3, offset);
	offset = dissect_rpc_uint32(tvb, pinfo, tree, hf_nfs_count3, offset);

	return offset;
}


/* RFC 1813, Page 46..48 */
int
dissect_nfs3_read_reply(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree* tree)
{
	guint32 status;

	offset = dissect_nfsstat3(tvb, offset, pinfo, tree, &status);
	switch (status) {
		case 0:
			offset = dissect_post_op_attr(tvb, offset, pinfo, tree, 
				"file_attributes");
			offset = dissect_rpc_uint32(tvb, pinfo, tree, hf_nfs_count3, 
				offset);
			offset = dissect_rpc_bool(tvb, pinfo, tree, hf_nfs_read_eof, 
				offset);
			offset = dissect_nfsdata(tvb, offset, pinfo, tree, hf_nfs_data);
		break;
		default:
			offset = dissect_post_op_attr(tvb, offset, pinfo, tree, 
				"file_attributes");
		break;
	}
		
	return offset;
}


/* RFC 1813, Page 49 */
static const value_string names_stable_how[] = {
	{	UNSTABLE,  "UNSTABLE"  },
	{	DATA_SYNC, "DATA_SYNC" },
	{	FILE_SYNC, "FILE_SYNC" },
	{ 0, NULL }
};


/* RFC 1813, Page 49 */
int
dissect_stable_how(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree* tree, int hfindex)
{
	guint32 stable_how;

	stable_how = tvb_get_ntohl(tvb,offset+0);
	if (tree) {
		proto_tree_add_uint(tree, hfindex, tvb,
			offset, 4, stable_how); 
	}
	offset += 4;

	return offset;
}


/* RFC 1813, Page 49..54 */
int
dissect_nfs3_write_call(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree* tree)
{
	offset = dissect_nfs_fh3   (tvb, offset, pinfo, tree, "file");
	offset = dissect_rpc_uint64(tvb, pinfo, tree, hf_nfs_offset3, offset);
	offset = dissect_rpc_uint32(tvb, pinfo, tree, hf_nfs_count3, offset);
	offset = dissect_stable_how(tvb, offset, pinfo, tree, hf_nfs_write_stable);
	offset = dissect_nfsdata   (tvb, offset, pinfo, tree, hf_nfs_data);

	return offset;
}


/* RFC 1813, Page 49..54 */
int
dissect_nfs3_write_reply(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree* tree)
{
	guint32 status;

	offset = dissect_nfsstat3(tvb, offset, pinfo, tree, &status);
	switch (status) {
		case 0:
			offset = dissect_wcc_data  (tvb, offset, pinfo, tree, "file_wcc");
			offset = dissect_rpc_uint32(tvb, pinfo, tree, hf_nfs_count3, 
				offset);
			offset = dissect_stable_how(tvb, offset, pinfo, tree,
				hf_nfs_write_committed);
			offset = dissect_writeverf3(tvb, offset, pinfo, tree);
		break;
		default:
			offset = dissect_wcc_data(tvb, offset, pinfo, tree, "file_wcc");
		break;
	}
		
	return offset;
}


/* RFC 1813, Page 54 */
static const value_string names_createmode3[] = {
	{	UNCHECKED, "UNCHECKED" },
	{	GUARDED,   "GUARDED" },
	{	EXCLUSIVE, "EXCLUSIVE" },
	{ 0, NULL }
};


/* RFC 1813, Page 54 */
int
dissect_createmode3(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree* tree, guint32* mode)
{
	guint32 mode_value;
	
	mode_value = tvb_get_ntohl(tvb, offset + 0);
	if (tree) {
		proto_tree_add_uint(tree, hf_nfs_createmode3, tvb,
		offset+0, 4, mode_value);
	}
	offset += 4;

	*mode = mode_value;
	return offset;
}


/* RFC 1813, Page 54..58 */
int
dissect_nfs3_create_call(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree* tree)
{
	guint32 mode;

	offset = dissect_diropargs3 (tvb, offset, pinfo, tree, "where");
	offset = dissect_createmode3(tvb, offset, pinfo, tree, &mode);
	switch (mode) {
		case UNCHECKED:
		case GUARDED:
			offset = dissect_sattr3(tvb, offset, pinfo, tree, "obj_attributes");
		break;
		case EXCLUSIVE:
			offset = dissect_createverf3(tvb, offset, pinfo, tree);
		break;
	}
	
	return offset;
}


/* RFC 1813, Page 54..58 */
int
dissect_nfs3_create_reply(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree* tree)
{
	guint32 status;

	offset = dissect_nfsstat3(tvb, offset, pinfo, tree, &status);
	switch (status) {
		case 0:
			offset = dissect_post_op_fh3 (tvb, offset, pinfo, tree, "obj");
			offset = dissect_post_op_attr(tvb, offset, pinfo, tree, 
				"obj_attributes");
			offset = dissect_wcc_data(tvb, offset, pinfo, tree, "dir_wcc");
		break;
		default:
			offset = dissect_wcc_data(tvb, offset, pinfo, tree, "dir_wcc");
		break;
	}
		
	return offset;
}


/* RFC 1813, Page 58..60 */
int
dissect_nfs3_mkdir_call(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree* tree)
{
	offset = dissect_diropargs3(tvb, offset, pinfo, tree, "where");
	offset = dissect_sattr3    (tvb, offset, pinfo, tree, "attributes");
	
	return offset;
}


/* RFC 1813, Page 61..63 */
int
dissect_nfs3_symlink_call(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree* tree)
{
	offset = dissect_diropargs3(tvb, offset, pinfo, tree, "where");
	offset = dissect_sattr3    (tvb, offset, pinfo, tree, "symlink_attributes");
	offset = dissect_nfspath3  (tvb, offset, pinfo, tree, hf_nfs_symlink_to);
	
	return offset;
}


/* RFC 1813, Page 63..66 */
int
dissect_nfs3_mknod_call(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree* tree)
{
	guint32 type;

	offset = dissect_diropargs3(tvb, offset, pinfo, tree, "where");
	offset = dissect_ftype3(tvb, offset, pinfo, tree, hf_nfs_ftype3, &type);
	switch (type) {
		case NF3CHR:
		case NF3BLK:
			offset = dissect_sattr3(tvb, offset, pinfo, tree, "dev_attributes");
			offset = dissect_specdata3(tvb, offset, pinfo, tree, "spec");
		break;
		case NF3SOCK:
		case NF3FIFO:
			offset = dissect_sattr3(tvb, offset, pinfo, tree, "pipe_attributes");
		break;
		default:
			/* nothing to do */
		break;
	}
	
	return offset;
}


/* RFC 1813, Page 67..69 */
int
dissect_nfs3_remove_reply(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree* tree)
{
	guint32 status;

	offset = dissect_nfsstat3(tvb, offset, pinfo, tree, &status);
	switch (status) {
		case 0:
			offset = dissect_wcc_data    (tvb, offset, pinfo, tree, "dir_wcc");
		break;
		default:
			offset = dissect_wcc_data    (tvb, offset, pinfo, tree, "dir_wcc");
		break;
	}
		
	return offset;
}


/* RFC 1813, Page 71..74 */
int
dissect_nfs3_rename_call(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree* tree)
{
	offset = dissect_diropargs3(tvb, offset, pinfo, tree, "from");
	offset = dissect_diropargs3(tvb, offset, pinfo, tree, "to");
	
	return offset;
}


/* RFC 1813, Page 71..74 */
int
dissect_nfs3_rename_reply(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree* tree)
{
	guint32 status;

	offset = dissect_nfsstat3(tvb, offset, pinfo, tree, &status);
	switch (status) {
		case 0:
			offset = dissect_wcc_data(tvb, offset, pinfo, tree, "fromdir_wcc");
			offset = dissect_wcc_data(tvb, offset, pinfo, tree, "todir_wcc");
		break;
		default:
			offset = dissect_wcc_data(tvb, offset, pinfo, tree, "fromdir_wcc");
			offset = dissect_wcc_data(tvb, offset, pinfo, tree, "todir_wcc");
		break;
	}
		
	return offset;
}


/* RFC 1813, Page 74..76 */
int
dissect_nfs3_link_call(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree* tree)
{
	offset = dissect_nfs_fh3   (tvb, offset, pinfo, tree, "file");
	offset = dissect_diropargs3(tvb, offset, pinfo, tree, "link");
	
	return offset;
}


/* RFC 1813, Page 74..76 */
int
dissect_nfs3_link_reply(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree* tree)
{
	guint32 status;

	offset = dissect_nfsstat3(tvb, offset, pinfo, tree, &status);
	switch (status) {
		case 0:
			offset = dissect_post_op_attr(tvb, offset, pinfo, tree, 
				"file_attributes");
			offset = dissect_wcc_data(tvb, offset, pinfo, tree, "linkdir_wcc");
		break;
		default:
			offset = dissect_post_op_attr(tvb, offset, pinfo, tree, 
				"file_attributes");
			offset = dissect_wcc_data(tvb, offset, pinfo, tree, "linkdir_wcc");
		break;
	}
		
	return offset;
}


/* RFC 1813, Page 76..80 */
int
dissect_nfs3_readdir_call(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree* tree)
{
	offset = dissect_nfs_fh3    (tvb, offset, pinfo, tree, "dir");
	offset = dissect_rpc_uint64(tvb, pinfo, tree, hf_nfs_cookie3, offset);
	offset = dissect_cookieverf3(tvb, offset, pinfo, tree);
	offset = dissect_rpc_uint32(tvb, pinfo, tree, hf_nfs_count3, offset);
	
	return offset;
}


/* RFC 1813, Page 76..80 */
int
dissect_entry3(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree* tree)
{
	proto_item* entry_item = NULL;
	proto_tree* entry_tree = NULL;
	int old_offset = offset;
	char *name;

	if (tree) {
		entry_item = proto_tree_add_item(tree, hf_nfs_readdir_entry, tvb,
			offset+0, -1, FALSE);
		entry_tree = proto_item_add_subtree(entry_item, ett_nfs_readdir_entry);
	}

	offset = dissect_rpc_uint64(tvb, pinfo, entry_tree, hf_nfs_readdir_entry3_fileid,
		offset);

	offset = dissect_filename3(tvb, offset, pinfo, entry_tree,
		hf_nfs_readdir_entry3_name, &name);
	if (entry_item)
		proto_item_set_text(entry_item, "Entry: name %s", name);
	g_free(name);

	offset = dissect_rpc_uint64(tvb, pinfo, entry_tree, hf_nfs_readdir_entry3_cookie, 
		offset);

	/* now we know, that a readdir entry is shorter */
	if (entry_item) {
		proto_item_set_len(entry_item, offset - old_offset);
	}

	return offset;
}


/* RFC 1813, Page 76..80 */
int
dissect_nfs3_readdir_reply(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree* tree)
{
	guint32 status;
	guint32 eof_value;

	offset = dissect_stat(tvb, offset, pinfo, tree, &status);
	switch (status) {
		case 0:
			offset = dissect_post_op_attr(tvb, offset, pinfo, tree, 
				"dir_attributes");
			offset = dissect_cookieverf3(tvb, offset, pinfo, tree);
			offset = dissect_rpc_list(tvb, pinfo, tree, offset, 
				dissect_entry3);
			eof_value = tvb_get_ntohl(tvb, offset+0);
			if (tree)
				proto_tree_add_uint(tree, hf_nfs_readdir_eof, tvb,
					offset+ 0, 4, eof_value);
			offset += 4;
		break;
		default:
			offset = dissect_post_op_attr(tvb, offset, pinfo, tree, 
				"dir_attributes");
		break;
	}

	return offset;
}


/* RFC 1813, Page 80..83 */
int
dissect_nfs3_readdirplus_call(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree* tree)
{
	offset = dissect_nfs_fh3    (tvb, offset, pinfo, tree, "dir");
	offset = dissect_rpc_uint64(tvb, pinfo, tree, hf_nfs_cookie3, offset);
	offset = dissect_cookieverf3(tvb, offset, pinfo, tree);
	offset = dissect_rpc_uint32(tvb, pinfo, tree, hf_nfs_count3_dircount,
		offset);
	offset = dissect_rpc_uint32(tvb, pinfo, tree, hf_nfs_count3_maxcount,
		offset);
	
	return offset;
}


/* RFC 1813, Page 80..83 */
int
dissect_entryplus3(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree* tree)
{
	proto_item* entry_item = NULL;
	proto_tree* entry_tree = NULL;
	int old_offset = offset;
	char *name;

	if (tree) {
		entry_item = proto_tree_add_item(tree, hf_nfs_readdir_entry, tvb,
			offset+0, -1, FALSE);
		entry_tree = proto_item_add_subtree(entry_item, ett_nfs_readdir_entry);
	}

	offset = dissect_rpc_uint64(tvb, pinfo, entry_tree,
		hf_nfs_readdirplus_entry_fileid, offset);

	offset = dissect_filename3(tvb, offset, pinfo, entry_tree,
		hf_nfs_readdirplus_entry_name, &name);
	if (entry_item)
		proto_item_set_text(entry_item, "Entry: name %s", name);
	g_free(name);

	offset = dissect_rpc_uint64(tvb, pinfo, entry_tree, hf_nfs_readdirplus_entry_cookie,
		offset);

	offset = dissect_post_op_attr(tvb, offset, pinfo, entry_tree, 
		"name_attributes");
	offset = dissect_post_op_fh3(tvb, offset, pinfo, entry_tree, "name_handle");

	/* now we know, that a readdirplus entry is shorter */
	if (entry_item) {
		proto_item_set_len(entry_item, offset - old_offset);
	}

	return offset;
}


/* RFC 1813, Page 80..83 */
int
dissect_nfs3_readdirplus_reply(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree* tree)
{
	guint32 status;
	guint32 eof_value;

	offset = dissect_stat(tvb, offset, pinfo, tree, &status);
	switch (status) {
		case 0:
			offset = dissect_post_op_attr(tvb, offset, pinfo, tree, 
				"dir_attributes");
			offset = dissect_cookieverf3(tvb, offset, pinfo, tree);
			offset = dissect_rpc_list(tvb, pinfo, tree, offset, 
				dissect_entryplus3);
			eof_value = tvb_get_ntohl(tvb, offset+0);
			if (tree)
				proto_tree_add_uint(tree, hf_nfs_readdir_eof, tvb,
					offset+ 0, 4, eof_value);
			offset += 4;
		break;
		default:
			offset = dissect_post_op_attr(tvb, offset, pinfo, tree, 
				"dir_attributes");
		break;
	}

	return offset;
}


/* RFC 1813, Page 84..86 */
int
dissect_nfs3_fsstat_reply(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree* tree)
{
	guint32 status;
	guint32 invarsec;

	offset = dissect_nfsstat3(tvb, offset, pinfo, tree, &status);
	switch (status) {
		case 0:
			offset = dissect_post_op_attr(tvb, offset, pinfo, tree, 
				"obj_attributes");
			offset = dissect_rpc_uint64(tvb, pinfo, tree, hf_nfs_fsstat3_resok_tbytes,
				offset);
			offset = dissect_rpc_uint64(tvb, pinfo, tree, hf_nfs_fsstat3_resok_fbytes,
				offset);
			offset = dissect_rpc_uint64(tvb, pinfo, tree, hf_nfs_fsstat3_resok_abytes,
				offset);
			offset = dissect_rpc_uint64(tvb, pinfo, tree, hf_nfs_fsstat3_resok_tfiles,
				offset);
			offset = dissect_rpc_uint64(tvb, pinfo, tree, hf_nfs_fsstat3_resok_ffiles,
				offset);
			offset = dissect_rpc_uint64(tvb, pinfo, tree, hf_nfs_fsstat3_resok_afiles,
				offset);
			invarsec = tvb_get_ntohl(tvb, offset + 0);
			if (tree)
				proto_tree_add_uint(tree, hf_nfs_fsstat_invarsec, tvb,
				offset+0, 4, invarsec);
			offset += 4;
		break;
		default:
			offset = dissect_post_op_attr(tvb, offset, pinfo, tree, 
				"obj_attributes");
		break;
	}

	return offset;
}


#define FSF3_LINK        0x0001
#define FSF3_SYMLINK     0x0002
#define FSF3_HOMOGENEOUS 0x0008
#define FSF3_CANSETTIME  0x0010


/* RFC 1813, Page 86..90 */
int
dissect_nfs3_fsinfo_reply(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree* tree)
{
	guint32 status;
	guint32 rtmax;
	guint32 rtpref;
	guint32 rtmult;
	guint32 wtmax;
	guint32 wtpref;
	guint32 wtmult;
	guint32 dtpref;
	guint32 properties;
	proto_item*	properties_item = NULL;
	proto_tree*	properties_tree = NULL;

	offset = dissect_nfsstat3(tvb, offset, pinfo, tree, &status);
	switch (status) {
		case 0:
			offset = dissect_post_op_attr(tvb, offset, pinfo, tree, 
				"obj_attributes");
			rtmax = tvb_get_ntohl(tvb, offset+0);
			if (tree)
				proto_tree_add_uint(tree, hf_nfs_fsinfo_rtmax, tvb,
				offset+0, 4, rtmax);
			offset += 4;
			rtpref = tvb_get_ntohl(tvb, offset+0);
			if (tree)
				proto_tree_add_uint(tree, hf_nfs_fsinfo_rtpref, tvb,
				offset+0, 4, rtpref);
			offset += 4;
			rtmult = tvb_get_ntohl(tvb, offset+0);
			if (tree)
				proto_tree_add_uint(tree, hf_nfs_fsinfo_rtmult, tvb,
				offset+0, 4, rtmult);
			offset += 4;
			wtmax = tvb_get_ntohl(tvb, offset+0);
			if (tree)
				proto_tree_add_uint(tree, hf_nfs_fsinfo_wtmax, tvb,
				offset+0, 4, wtmax);
			offset += 4;
			wtpref = tvb_get_ntohl(tvb, offset+0);
			if (tree)
				proto_tree_add_uint(tree, hf_nfs_fsinfo_wtpref, tvb,
				offset+0, 4, wtpref);
			offset += 4;
			wtmult = tvb_get_ntohl(tvb, offset+0);
			if (tree)
				proto_tree_add_uint(tree, hf_nfs_fsinfo_wtmult, tvb,
				offset+0, 4, wtmult);
			offset += 4;
			dtpref = tvb_get_ntohl(tvb, offset+0);
			if (tree)
				proto_tree_add_uint(tree, hf_nfs_fsinfo_dtpref, tvb,
				offset+0, 4, dtpref);
			offset += 4;

			offset = dissect_rpc_uint64(tvb, pinfo, tree, 
				hf_nfs_fsinfo_maxfilesize, offset);
			offset = dissect_nfstime3(tvb, offset, pinfo, tree, hf_nfs_dtime, hf_nfs_dtime_sec, hf_nfs_dtime_nsec);
			properties = tvb_get_ntohl(tvb, offset+0);
			if (tree) {
				properties_item = proto_tree_add_uint(tree,
				hf_nfs_fsinfo_properties,
				tvb, offset+0, 4, properties);
				if (properties_item) 
					properties_tree = proto_item_add_subtree(properties_item, 
						ett_nfs_fsinfo_properties);
				if (properties_tree) {
					proto_tree_add_text(properties_tree, tvb,
					offset, 4, "%s",
					decode_boolean_bitfield(properties,
					FSF3_CANSETTIME,5,
					"SETATTR can set time on server",
					"SETATTR can't set time on server"));

					proto_tree_add_text(properties_tree, tvb,
					offset, 4, "%s",
					decode_boolean_bitfield(properties,
					FSF3_HOMOGENEOUS,5,
					"PATHCONF is valid for all files",
					"PATHCONF should be get for every single file"));

					proto_tree_add_text(properties_tree, tvb,
					offset, 4, "%s",
					decode_boolean_bitfield(properties,
					FSF3_SYMLINK,5,
					"File System supports symbolic links",
					"File System does not symbolic hard links"));

					proto_tree_add_text(properties_tree, tvb,
					offset, 4, "%s",
					decode_boolean_bitfield(properties,
					FSF3_LINK,5,
					"File System supports hard links",
					"File System does not support hard links"));
				}
			}
			offset += 4;
		break;
		default:
			offset = dissect_post_op_attr(tvb, offset, pinfo, tree, 
				"obj_attributes");
		break;
	}

	return offset;
}


/* RFC 1813, Page 90..92 */
int
dissect_nfs3_pathconf_reply(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree* tree)
{
	guint32 status;
	guint32 linkmax;
	guint32 name_max;

	offset = dissect_nfsstat3(tvb, offset, pinfo, tree, &status);
	switch (status) {
		case 0:
			offset = dissect_post_op_attr(tvb, offset, pinfo, tree, 
				"obj_attributes");
			linkmax = tvb_get_ntohl(tvb, offset + 0);
			if (tree)
				proto_tree_add_uint(tree, hf_nfs_pathconf_linkmax, tvb,
				offset+0, 4, linkmax);
			offset += 4;
			name_max = tvb_get_ntohl(tvb, offset + 0);
			if (tree)
				proto_tree_add_uint(tree, hf_nfs_pathconf_name_max, tvb,
				offset+0, 4, name_max);
			offset += 4;
			offset = dissect_rpc_bool(tvb, pinfo, tree, 
				hf_nfs_pathconf_no_trunc, offset);
			offset = dissect_rpc_bool(tvb, pinfo, tree, 
				hf_nfs_pathconf_chown_restricted, offset);
			offset = dissect_rpc_bool(tvb, pinfo, tree, 
				hf_nfs_pathconf_case_insensitive, offset);
			offset = dissect_rpc_bool(tvb, pinfo, tree, 
				hf_nfs_pathconf_case_preserving, offset);
		break;
		default:
			offset = dissect_post_op_attr(tvb, offset, pinfo, tree, 
				"obj_attributes");
		break;
	}

	return offset;
}


/* RFC 1813, Page 92..95 */
int
dissect_nfs3_commit_call(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree* tree)
{
	offset = dissect_nfs_fh3(tvb, offset, pinfo, tree, "file");
	offset = dissect_rpc_uint64(tvb, pinfo, tree, hf_nfs_offset3, offset);
	offset = dissect_rpc_uint32(tvb, pinfo, tree, hf_nfs_count3, offset);
	return offset;
}


/* RFC 1813, Page 92..95 */
int
dissect_nfs3_commit_reply(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree* tree)
{
	guint32 status;

	offset = dissect_nfsstat3(tvb, offset, pinfo, tree, &status);
	switch (status) {
		case 0:
			offset = dissect_wcc_data  (tvb, offset, pinfo, tree, "file_wcc");
			offset = dissect_writeverf3(tvb, offset, pinfo, tree);
		break;
		default:
			offset = dissect_wcc_data(tvb, offset, pinfo, tree, "file_wcc");
		break;
	}
		
	return offset;
}

/**********************************************************/
/* NFS Version 4, RFC 3010 with nfs4_prot.x 1.103 changes */
/**********************************************************/

int
dissect_nfs_utf8string(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree, int hf, char **string_ret)
{
	/* TODO: this needs to be fixed */
	return dissect_rpc_string(tvb, pinfo, tree, hf, offset, string_ret);
}

int
dissect_nfs_linktext4(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree, char *name)
{
	return dissect_nfs_utf8string(tvb, offset, pinfo, tree, hf_nfs_linktext4, 
		NULL);
}

int
dissect_nfs_specdata4(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree, char *name)
{
	offset = dissect_rpc_uint32(tvb, pinfo, tree, hf_nfs_specdata1, offset);
	offset = dissect_rpc_uint32(tvb, pinfo, tree, hf_nfs_specdata2, offset);

	return offset;
}

int
dissect_nfs_clientid4(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree)
{
	offset = dissect_rpc_uint64(tvb, pinfo, tree, hf_nfs_clientid4, offset);

	return offset;
}

static const value_string names_ftype4[] = {
	{	NF4REG,	"NF4REG"	},
	{	NF4DIR,	"NF4DIR"	},
	{	NF4BLK,	"NF4BLK"  },
	{	NF4CHR,	"NF4CHR"  },
	{	NF4LNK,  "NF4LNK"  },
	{	NF4SOCK,	"NF4SOCK"  },
	{	NF4FIFO,	"NF4FIFO"  },
	{	NF4ATTRDIR,	"NF4ATTRDIR"	},
	{	NF4NAMEDATTR,	"NF4NAMEDATTR"	},
	{ 0, NULL }
};

int
dissect_nfs_component4(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree, char *name)
{
	return dissect_nfs_utf8string(tvb, offset, pinfo, tree, hf_nfs_component4, 
		NULL);
}

int
dissect_nfs_reclaim4(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree, char *name)
{
	offset = dissect_rpc_uint32(tvb, pinfo, tree, hf_nfs_reclaim4, offset);
	return offset;
}

int
dissect_nfs_length4(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree, char *name)
{
	offset = dissect_rpc_uint64(tvb, pinfo, tree, hf_nfs_length4, offset);
	return offset;
}

int
dissect_nfs_opaque4(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree, char *name);

int
dissect_nfs_lock_owner4(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree, char *name)
{
	proto_tree *newftree = NULL;
	proto_item *fitem = NULL;

	fitem = proto_tree_add_text(tree, tvb, offset, 4, "Owner");

	if (fitem) {
		newftree = proto_item_add_subtree(fitem, ett_nfs_lock_owner4);

		if (newftree) {
			offset = dissect_rpc_uint64(tvb, pinfo, newftree, hf_nfs_clientid4,
				offset);
			offset = dissect_nfs_opaque4(tvb, offset, pinfo, newftree, "Owner");
		}
	}

	return offset;
}

int
dissect_nfs_pathname4(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree, char *name)
{
	guint comp_count, i;
	proto_item *fitem = NULL;
	proto_tree *newftree = NULL;

	comp_count=tvb_get_ntohl(tvb, offset);
	fitem = proto_tree_add_text(tree, tvb, offset, 4, 
		"pathname components (%d)", comp_count);
	offset += 4;

	if (fitem) {
		newftree = proto_item_add_subtree(fitem, ett_nfs_pathname4);

		if (newftree) {
			for (i=0; i<comp_count; i++)
				offset=dissect_nfs_component4(tvb, offset, pinfo, newftree, "comp");
		}
	}

	return offset;
}

int
dissect_nfs_changeid4(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree, char *name)
{
	offset = dissect_rpc_uint64(tvb, pinfo, tree, hf_nfs_changeid4, offset);
	return offset;
}

int
dissect_nfs_nfstime4(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree, char *name)
{
	offset = dissect_rpc_uint64(tvb, pinfo, tree, hf_nfs_nfstime4_seconds, 
		offset);
	offset = dissect_rpc_uint32(tvb, pinfo, tree, hf_nfs_nfstime4_nseconds,
		offset);
	return offset;
}

static const value_string names_time_how4[] = {
#define SET_TO_SERVER_TIME4 0
	{	SET_TO_SERVER_TIME4,	"SET_TO_SERVER_TIME4"	},
#define SET_TO_CLIENT_TIME4 1
	{	SET_TO_CLIENT_TIME4,	"SET_TO_CLIENT_TIME4"	},
	{	0,	NULL	},
};

int
dissect_nfs_settime4(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree, char *name)
{
	guint32 set_it;

	set_it = tvb_get_ntohl(tvb, offset);
	proto_tree_add_uint(tree, hf_nfs_time_how4, tvb, offset+0, 
		4, set_it);
	offset += 4;

	if (set_it == SET_TO_CLIENT_TIME4)
		offset = dissect_nfs_nfstime4(tvb, offset, pinfo, tree, NULL);
	
	return offset;
}

int
dissect_nfs_fsid4(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree *tree, char *name)
{
	proto_tree *newftree = NULL;
	proto_item *fitem = NULL;

	fitem = proto_tree_add_text(tree, tvb, offset, 0, "%s", name);

	if (fitem == NULL) return offset;

	newftree = proto_item_add_subtree(fitem, ett_nfs_fsid4);

	if (newftree == NULL) return offset;

	offset = dissect_rpc_uint64(tvb, pinfo, newftree, hf_nfs_fsid4_major,
		offset);
	offset = dissect_rpc_uint64(tvb, pinfo, newftree, hf_nfs_fsid4_minor,
		offset);

	return offset;
}

int
dissect_nfs_acetype4(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree, char *name)
{
	offset = dissect_rpc_uint32(tvb, pinfo, tree, hf_nfs_acetype4, offset);
	return offset;
}

int
dissect_nfs_aceflag4(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree, char *name)
{
	offset = dissect_rpc_uint32(tvb, pinfo, tree, hf_nfs_aceflag4, offset);
	return offset;
}

int
dissect_nfs_acemask4(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree, char *name)
{
	return dissect_rpc_uint32(tvb, pinfo, tree, hf_nfs_acemask4, offset);
}

int
dissect_nfs_nfsace4(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree *tree, char *name)
{
	proto_tree *newftree = NULL;
	proto_item *fitem = NULL;
	int nextentry;

	fitem = proto_tree_add_text(tree, tvb, offset, 0, "%s", name);

	if (fitem == NULL) return offset;

	newftree = proto_item_add_subtree(fitem, ett_nfs_fsid4);

	if (newftree == NULL) return offset;

	nextentry = tvb_get_ntohl(tvb, offset);
	offset = dissect_rpc_bool(tvb, pinfo, newftree, hf_nfs_data_follows,
		offset);

	while (nextentry)
	{
		offset = dissect_nfs_acetype4(tvb, offset, pinfo, newftree, "type");
		offset = dissect_nfs_aceflag4(tvb, offset, pinfo, newftree, "flag");
		offset = dissect_nfs_acemask4(tvb, offset, pinfo, newftree, 
			"access_mask");
		offset = dissect_nfs_utf8string(tvb, offset, pinfo, newftree, 
			hf_nfs_who, NULL);
		nextentry = tvb_get_ntohl(tvb, offset);
		offset += 4;
	}

	return offset;
}

int
dissect_nfs_fh4(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree, char *name)
{
	return dissect_nfs_fh3(tvb, offset, pinfo, tree, name);
}

int
dissect_nfs_fs_location4(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree, char *name)
{
	proto_tree *newftree = NULL;
	proto_item *fitem = NULL;

	fitem = proto_tree_add_text(tree, tvb, offset, 0, "%s", name);

	if (fitem == NULL) return offset;

	newftree = proto_item_add_subtree(fitem, ett_nfs_fs_location4);

	if (newftree == NULL) return offset;

	offset = dissect_nfs_utf8string(tvb, offset, pinfo, tree, hf_nfs_server, 
		NULL);

	return offset;
}

int
dissect_nfs_fs_locations4(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree, char *name)
{
	proto_tree *newftree = NULL;
	proto_item *fitem = NULL;
	int nextentry;

	fitem = proto_tree_add_text(tree, tvb, offset, 0, "%s", name);

	if (fitem == NULL) return offset;

	newftree = proto_item_add_subtree(fitem, ett_nfs_fs_locations4);

	if (newftree == NULL) return offset;

	offset = dissect_nfs_pathname4(tvb, offset, pinfo, newftree, "fs_root");

	nextentry = tvb_get_ntohl(tvb, offset);
	offset = dissect_rpc_bool(tvb, pinfo, newftree, hf_nfs_data_follows, 
		offset);

	while (nextentry)
	{
		offset = dissect_nfs_fs_location4(tvb, offset, pinfo, newftree, 
			"locations");
		nextentry = tvb_get_ntohl(tvb, offset);
		offset += 4;
	}

	return offset;
}

int
dissect_nfs_mode4(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree, char *name)
{
	return dissect_mode(tvb, offset, pinfo, tree, name);
}

static const value_string nfs4_fattr4_fh_expire_type_names[] = {
#define FH4_PERSISTENT 0x00000000
	{	FH4_PERSISTENT,	"FH4_PERSISTENT"	},
#define FH4_NOEXPIRE_WITH_OPEN 0x00000001
	{	FH4_NOEXPIRE_WITH_OPEN,	"FH4_NOEXPIRE_WITH_OPEN"	},
#define FH4_VOLATILE_ANY 0x00000002
	{	FH4_VOLATILE_ANY,	"FH4_VOLATILE_ANY"	},
#define FH4_VOL_MIGRATION 0x00000004
	{	FH4_VOL_MIGRATION,	"FH4_VOL_MIGRATION"	},
#define FH4_VOL_RENAME 0x00000008
	{	FH4_VOL_RENAME,	"FH4_VOL_RENAME"	},
	{	0,	NULL	}
};


int
dissect_nfs_fattr4_fh_expire_type(tvbuff_t *tvb, int offset, 
	packet_info *pinfo, proto_tree *tree)
{
	guint32 expire_type;
	proto_item *expire_type_item = NULL;
	proto_tree *expire_type_tree = NULL;

	expire_type = tvb_get_ntohl(tvb, offset + 0);

	if (tree)
	{
		expire_type_item = proto_tree_add_text(tree, tvb, offset, 4,
			"fattr4_fh_expire_type: 0x%08x", expire_type);
		if (expire_type_item)
			expire_type_tree = proto_item_add_subtree(expire_type_item, 
				ett_nfs_fattr4_fh_expire_type);
	}

	if (expire_type_tree)
	{
		if (expire_type == FH4_PERSISTENT)
		{
			proto_tree_add_text(expire_type_tree, tvb, offset, 4, "%s",
				decode_enumerated_bitfield(expire_type, FH4_PERSISTENT, 8, 
				nfs4_fattr4_fh_expire_type_names, "%s"));
		}
		else
		{
			if (expire_type & FH4_NOEXPIRE_WITH_OPEN)
				proto_tree_add_text(expire_type_tree, tvb, offset, 4,
						"FH4_NOEXPIRE_WITH_OPEN (0x%08x)", FH4_NOEXPIRE_WITH_OPEN);

			if (expire_type & FH4_VOLATILE_ANY)
				proto_tree_add_text(expire_type_tree, tvb, offset, 4,
						"FH4_VOLATILE_ANY (0x%08x)", FH4_VOLATILE_ANY);

			if (expire_type & FH4_VOL_MIGRATION)
				proto_tree_add_text(expire_type_tree, tvb, offset, 4,
						"FH4_VOL_MIGRATION (0x%08x)", FH4_VOL_MIGRATION);

			if (expire_type & FH4_VOL_RENAME)
				proto_tree_add_text(expire_type_tree, tvb, offset, 4,
						"FH4_VOL_RENAME (0x%08x)", FH4_VOL_RENAME);
		}
	}

	offset += 4;

	return offset;
}

static const value_string names_fattr4[] = {
#define FATTR4_SUPPORTED_ATTRS     0
	{	FATTR4_SUPPORTED_ATTRS,	"FATTR4_SUPPORTED_ATTRS"	},
#define FATTR4_TYPE                1
	{	FATTR4_TYPE,	"FATTR4_TYPE"	},
#define FATTR4_FH_EXPIRE_TYPE      2
	{	FATTR4_FH_EXPIRE_TYPE,	"FATTR4_FH_EXPIRE_TYPE"	},
#define FATTR4_CHANGE              3
	{	FATTR4_CHANGE,	"FATTR4_CHANGE"	},
#define FATTR4_SIZE                4
	{	FATTR4_SIZE,	"FATTR4_SIZE"	},
#define FATTR4_LINK_SUPPORT        5
	{	FATTR4_LINK_SUPPORT,	"FATTR4_LINK_SUPPORT"	},
#define FATTR4_SYMLINK_SUPPORT     6
	{	FATTR4_SYMLINK_SUPPORT,	"FATTR4_SYMLINK_SUPPORT"	},
#define FATTR4_NAMED_ATTR          7
	{	FATTR4_NAMED_ATTR,	"FATTR4_NAMED_ATTR"	},
#define FATTR4_FSID                8
	{	FATTR4_FSID,	"FATTR4_FSID"	},
#define FATTR4_UNIQUE_HANDLES      9
	{	FATTR4_UNIQUE_HANDLES,	"FATTR4_UNIQUE_HANDLES"	},
#define FATTR4_LEASE_TIME          10
	{	FATTR4_LEASE_TIME,	"FATTR4_LEASE_TIME"	},
#define FATTR4_RDATTR_ERROR        11
	{	FATTR4_RDATTR_ERROR,	"FATTR4_RDATTR_ERROR"	},
#define FATTR4_ACL                 12
	{	FATTR4_ACL,	"FATTR4_ACL"	},
#define FATTR4_ACLSUPPORT          13
	{	FATTR4_ACLSUPPORT,	"FATTR4_ACLSUPPORT"	},
#define FATTR4_ARCHIVE             14
	{	FATTR4_ARCHIVE, "FATTR4_ARCHIVE"	},
#define FATTR4_CANSETTIME          15
	{	FATTR4_CANSETTIME, "FATTR4_CANSETTIME"	},
#define FATTR4_CASE_INSENSITIVE    16
	{	FATTR4_CASE_INSENSITIVE, "FATTR4_CASE_INSENSITIVE"	},
#define FATTR4_CASE_PRESERVING     17
	{	FATTR4_CASE_PRESERVING, "FATTR4_CASE_PRESERVING"	},
#define FATTR4_CHOWN_RESTRICTED    18
	{	FATTR4_CHOWN_RESTRICTED, "FATTR4_CHOWN_RESTRICTED"	},
#define FATTR4_FILEHANDLE          19
	{	FATTR4_FILEHANDLE, "FATTR4_FILEHANDLE"	},
#define FATTR4_FILEID              20
	{	FATTR4_FILEID, "FATTR4_FILEID"	},
#define FATTR4_FILES_AVAIL         21
	{	FATTR4_FILES_AVAIL, "FATTR4_FILES_AVAIL"	},
#define FATTR4_FILES_FREE          22
	{	FATTR4_FILES_FREE, "FATTR4_FILES_FREE"	},
#define FATTR4_FILES_TOTAL         23
	{	FATTR4_FILES_TOTAL, "FATTR4_FILES_TOTAL"	},
#define FATTR4_FS_LOCATIONS        24
	{	FATTR4_FS_LOCATIONS, "FATTR4_FS_LOCATIONS"	},
#define FATTR4_HIDDEN              25
	{	FATTR4_HIDDEN, "FATTR4_HIDDEN"	},
#define FATTR4_HOMOGENEOUS         26
	{	FATTR4_HOMOGENEOUS, "FATTR4_HOMOGENEOUS"	},
#define FATTR4_MAXFILESIZE         27
	{	FATTR4_MAXFILESIZE, "FATTR4_MAXFILESIZE"	},
#define FATTR4_MAXLINK             28
	{	FATTR4_MAXLINK, "FATTR4_MAXLINK"	},
#define FATTR4_MAXNAME             29
	{	FATTR4_MAXNAME, "FATTR4_MAXNAME"	},
#define FATTR4_MAXREAD             30
	{	FATTR4_MAXREAD, "FATTR4_MAXREAD"	},
#define FATTR4_MAXWRITE            31
	{	FATTR4_MAXWRITE, "FATTR4_MAXWRITE"	},
#define FATTR4_MIMETYPE            32
	{	FATTR4_MIMETYPE, "FATTR4_MIMETYPE"	},
#define FATTR4_MODE                33
	{	FATTR4_MODE, "FATTR4_MODE"	},
#define FATTR4_NO_TRUNC            34
	{	FATTR4_NO_TRUNC, "FATTR4_NO_TRUNC"	},
#define FATTR4_NUMLINKS            35
	{	FATTR4_NUMLINKS, "FATTR4_NUMLINKS"	},
#define FATTR4_OWNER               36
	{	FATTR4_OWNER, "FATTR4_OWNER"	},
#define FATTR4_OWNER_GROUP         37
	{	FATTR4_OWNER_GROUP, "FATTR4_OWNER_GROUP"	},
#define FATTR4_QUOTA_AVAIL_HARD    38
	{	FATTR4_QUOTA_AVAIL_HARD, "FATTR4_QUOTA_AVAIL_HARD"	},
#define FATTR4_QUOTA_AVAIL_SOFT    39
	{	FATTR4_QUOTA_AVAIL_SOFT, "FATTR4_QUOTA_AVAIL_SOFT"	},
#define FATTR4_QUOTA_USED          40
	{	FATTR4_QUOTA_USED, "FATTR4_QUOTA_USED"	},
#define FATTR4_RAWDEV              41
	{	FATTR4_RAWDEV, "FATTR4_RAWDEV"	},
#define FATTR4_SPACE_AVAIL         42
	{	FATTR4_SPACE_AVAIL, "FATTR4_SPACE_AVAIL"	},
#define FATTR4_SPACE_FREE          43
	{	FATTR4_SPACE_FREE, "FATTR4_SPACE_FREE"	},
#define FATTR4_SPACE_TOTAL         44
	{	FATTR4_SPACE_TOTAL, "FATTR4_SPACE_TOTAL"	},
#define FATTR4_SPACE_USED          45
	{	FATTR4_SPACE_USED, "FATTR4_SPACE_USED"	},
#define FATTR4_SYSTEM              46
	{	FATTR4_SYSTEM, "FATTR4_SYSTEM"	},
#define FATTR4_TIME_ACCESS         47
	{	FATTR4_TIME_ACCESS, "FATTR4_TIME_ACCESS"	},
#define FATTR4_TIME_ACCESS_SET     48
	{	FATTR4_TIME_ACCESS_SET, "FATTR4_TIME_ACCESS_SET"	},
#define FATTR4_TIME_BACKUP         49
	{	FATTR4_TIME_BACKUP, "FATTR4_TIME_BACKUP"	},
#define FATTR4_TIME_CREATE         50
	{	FATTR4_TIME_CREATE, "FATTR4_TIME_CREATE"	},
#define FATTR4_TIME_DELTA          51
	{	FATTR4_TIME_DELTA, "FATTR4_TIME_DELTA"	},
#define FATTR4_TIME_METADATA       52
	{	FATTR4_TIME_METADATA, "FATTR4_TIME_METADATA"	},
#define FATTR4_TIME_MODIFY         53
	{	FATTR4_TIME_MODIFY, "FATTR4_TIME_MODIFY"	},
#define FATTR4_TIME_MODIFY_SET     54
	{	FATTR4_TIME_MODIFY_SET, "FATTR4_TIME_MODIFY_SET"	},
	{	0,	NULL	}
};

#define FATTR4_BITMAP_ONLY 0
#define FATTR4_FULL_DISSECT 1

int
dissect_nfs_attributes(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree, char *name, int type)
{
	guint32 bitmap_len;
	proto_item *fitem = NULL;
	proto_tree *newftree = NULL;
	proto_item *attr_fitem = NULL;
	proto_tree *attr_newftree = NULL;
	unsigned int i;
	int j, fattr;
	guint32 *bitmap;
	guint32 sl;
	int attr_vals_offset;

	bitmap_len = tvb_get_ntohl(tvb, offset);
	fitem = proto_tree_add_text(tree, tvb, offset, 4 + bitmap_len * 4,
		"%s", "attrmask");
	offset += 4;

	if (fitem == NULL) return offset;

	newftree = proto_item_add_subtree(fitem, ett_nfs_bitmap4);

	if (newftree == NULL) return offset;

	attr_vals_offset = offset + 4 + bitmap_len * 4;

	bitmap = g_malloc(bitmap_len * sizeof(guint32));	
	if (bitmap == NULL) return offset;

	for (i = 0; i < bitmap_len; i++)
	{
		if (!tvb_bytes_exist(tvb, offset,  4))
		{
			g_free(bitmap);
			return offset;
		}

		bitmap[i] = tvb_get_ntohl(tvb, offset);

		sl = 0x00000001;

		for (j = 0; j < 32; j++)
		{
			fattr = 32 * i + j;

			if (bitmap[i] & sl)
			{
				/* switch label if attribute is recommended vs. mandatory */
				attr_fitem = proto_tree_add_uint(newftree, 
					(fattr < FATTR4_ACL)? hf_nfs_mand_attr: hf_nfs_recc_attr, 
					tvb, offset, 4, fattr);

				if (attr_fitem == NULL) break;

				attr_newftree = proto_item_add_subtree(attr_fitem, ett_nfs_bitmap4);

				if (attr_newftree == NULL) break;

				if (type == FATTR4_FULL_DISSECT)
				{
					/* do a full decode of the arguments for the set flag */
					switch(fattr)
					{
					case FATTR4_SUPPORTED_ATTRS:
						attr_vals_offset = dissect_nfs_attributes(tvb, 
							attr_vals_offset, pinfo, attr_newftree, 
							"fattr4_supported_attrs", FATTR4_BITMAP_ONLY);
						break;
						
					case FATTR4_TYPE:
						attr_vals_offset = dissect_rpc_uint32(tvb, pinfo, 
							attr_newftree, hf_nfs_ftype4, attr_vals_offset);
						break;

					case FATTR4_FH_EXPIRE_TYPE:
						attr_vals_offset = dissect_nfs_fattr4_fh_expire_type(tvb,
							attr_vals_offset, pinfo, attr_newftree);
						break;

					case FATTR4_CHANGE:
						attr_vals_offset = dissect_nfs_changeid4(tvb, 
							attr_vals_offset, pinfo, attr_newftree, "fattr4_change");
						break;

					case FATTR4_SIZE:
						attr_vals_offset = dissect_rpc_uint64(tvb, pinfo, 
							attr_newftree, hf_nfs_fattr4_size, attr_vals_offset);
						break;

					case FATTR4_LINK_SUPPORT:
						attr_vals_offset = dissect_rpc_bool(tvb, pinfo, 
							attr_newftree, hf_nfs_fattr4_link_support, 
							attr_vals_offset);
						break;

					case FATTR4_SYMLINK_SUPPORT:
						attr_vals_offset = dissect_rpc_bool(tvb, pinfo, 
							attr_newftree, hf_nfs_fattr4_symlink_support, 
							attr_vals_offset);
						break;

					case FATTR4_NAMED_ATTR:
						attr_vals_offset = dissect_rpc_bool(tvb, pinfo, 
							attr_newftree, hf_nfs_fattr4_named_attr, attr_vals_offset);
						break;

					case FATTR4_FSID:
						attr_vals_offset = dissect_nfs_fsid4(tvb, attr_vals_offset,
							pinfo, attr_newftree, "fattr4_fsid");
						break;

					case FATTR4_UNIQUE_HANDLES:
						attr_vals_offset = dissect_rpc_bool(tvb, pinfo, 
							attr_newftree, hf_nfs_fattr4_unique_handles, 
							attr_vals_offset);
						break;

					case FATTR4_LEASE_TIME:
						attr_vals_offset = dissect_rpc_uint32(tvb, pinfo, 
							attr_newftree, hf_nfs_fattr4_lease_time, 
							attr_vals_offset);
						break;

					case FATTR4_RDATTR_ERROR:
						attr_vals_offset = dissect_nfs_nfsstat4(tvb, attr_vals_offset,
							pinfo, attr_newftree, NULL);
						break;

					case FATTR4_ACL:
						attr_vals_offset = dissect_nfs_nfsace4(tvb, attr_vals_offset,
							pinfo, attr_newftree, "fattr4_acl");
						break;

					case FATTR4_ACLSUPPORT:
						attr_vals_offset = dissect_rpc_uint32(tvb, pinfo, 
							attr_newftree, hf_nfs_fattr4_aclsupport, offset);
						break;

					case FATTR4_ARCHIVE:
						attr_vals_offset = dissect_rpc_bool(tvb, 
							pinfo, attr_newftree, hf_nfs_fattr4_archive, 
							attr_vals_offset);
						break;

					case FATTR4_CANSETTIME:
						attr_vals_offset = dissect_rpc_bool(tvb, pinfo, 
							attr_newftree, hf_nfs_fattr4_cansettime, attr_vals_offset);
						break;

					case FATTR4_CASE_INSENSITIVE:
						attr_vals_offset = dissect_rpc_bool(tvb, pinfo, 
							attr_newftree, hf_nfs_fattr4_case_insensitive, 
							attr_vals_offset);
						break;

					case FATTR4_CASE_PRESERVING:
						attr_vals_offset = dissect_rpc_bool(tvb, pinfo, 
							attr_newftree, hf_nfs_fattr4_case_preserving, 
							attr_vals_offset);
						break;

					case FATTR4_CHOWN_RESTRICTED:
						attr_vals_offset = dissect_rpc_bool(tvb, pinfo, 
							attr_newftree, hf_nfs_fattr4_chown_restricted, 
							attr_vals_offset);
						break;

					case FATTR4_FILEID:
						attr_vals_offset = dissect_rpc_uint64(tvb, pinfo, 
							attr_newftree, hf_nfs_fattr4_fileid, attr_vals_offset);
						break;

					case FATTR4_FILES_AVAIL:
						attr_vals_offset = dissect_rpc_uint64(tvb, pinfo, 
							attr_newftree, hf_nfs_fattr4_files_avail, 
							attr_vals_offset);
						break;

					case FATTR4_FILEHANDLE:
						attr_vals_offset = dissect_nfs_fh4(tvb, attr_vals_offset,
							pinfo, attr_newftree, "fattr4_filehandle");
						break;

					case FATTR4_FILES_FREE:
						attr_vals_offset = dissect_rpc_uint64(tvb, pinfo, 
							attr_newftree, hf_nfs_fattr4_files_free, attr_vals_offset);
						break;

					case FATTR4_FILES_TOTAL:
						attr_vals_offset = dissect_rpc_uint64(tvb, pinfo, 
							attr_newftree, hf_nfs_fattr4_files_total, 
							attr_vals_offset);
						break;

					case FATTR4_FS_LOCATIONS:
						attr_vals_offset = dissect_nfs_fs_locations4(tvb, 
							attr_vals_offset, pinfo, attr_newftree, 
							"fattr4_fs_locations");
						break;

					case FATTR4_HIDDEN:
						attr_vals_offset = dissect_rpc_bool(tvb, pinfo, 
							attr_newftree, hf_nfs_fattr4_hidden, attr_vals_offset);
						break;

					case FATTR4_HOMOGENEOUS:
						attr_vals_offset = dissect_rpc_bool(tvb, pinfo, 
							attr_newftree, hf_nfs_fattr4_homogeneous, 
							attr_vals_offset);
						break;

					case FATTR4_MAXFILESIZE:
						attr_vals_offset = dissect_rpc_uint64(tvb, pinfo, 
							attr_newftree, hf_nfs_fattr4_maxfilesize, 
							attr_vals_offset);
						break;

					case FATTR4_MAXLINK:
						attr_vals_offset = dissect_rpc_uint32(tvb, pinfo, 
							attr_newftree, hf_nfs_fattr4_maxlink, attr_vals_offset);
						break;

					case FATTR4_MAXNAME:
						attr_vals_offset = dissect_rpc_uint32(tvb, pinfo, 
							attr_newftree, hf_nfs_fattr4_maxname, attr_vals_offset);
						break;

					case FATTR4_MAXREAD:
						attr_vals_offset = dissect_rpc_uint64(tvb, pinfo, 
							attr_newftree, hf_nfs_fattr4_maxread, attr_vals_offset);
						break;

					case FATTR4_MAXWRITE:
						attr_vals_offset = dissect_rpc_uint64(tvb, pinfo, 
							attr_newftree, hf_nfs_fattr4_maxwrite, attr_vals_offset);
						break;

					case FATTR4_MIMETYPE:
						attr_vals_offset = dissect_nfs_utf8string(tvb, 
							attr_vals_offset, pinfo, attr_newftree, 
							hf_nfs_fattr4_mimetype, NULL);
						break;
					
					case FATTR4_MODE:
						attr_vals_offset = dissect_nfs_mode4(tvb,
							attr_vals_offset, pinfo, attr_newftree, "fattr4_mode");
						break;

					case FATTR4_NO_TRUNC:
						attr_vals_offset = dissect_rpc_bool(tvb, pinfo, 
							attr_newftree, hf_nfs_fattr4_no_trunc, attr_vals_offset);
						break;

					case FATTR4_NUMLINKS:
						attr_vals_offset = dissect_rpc_uint32(tvb, pinfo, 
							attr_newftree, hf_nfs_fattr4_numlinks, attr_vals_offset);
						break;

					case FATTR4_OWNER:
						attr_vals_offset = dissect_nfs_utf8string(tvb, 
							attr_vals_offset, pinfo, attr_newftree, 
							hf_nfs_fattr4_owner,
							NULL);
						break;

					case FATTR4_OWNER_GROUP:
						attr_vals_offset = dissect_nfs_utf8string(tvb, 
							attr_vals_offset, pinfo, attr_newftree, 
							hf_nfs_fattr4_owner_group, NULL);
						break;

					case FATTR4_QUOTA_AVAIL_HARD:
						attr_vals_offset = dissect_rpc_uint64(tvb, pinfo, 
							attr_newftree, hf_nfs_fattr4_quota_hard, attr_vals_offset);
						break;

					case FATTR4_QUOTA_AVAIL_SOFT:
						attr_vals_offset = dissect_rpc_uint64(tvb, pinfo, 
							attr_newftree, hf_nfs_fattr4_quota_soft, attr_vals_offset);
						break;

					case FATTR4_QUOTA_USED:
						attr_vals_offset = dissect_rpc_uint64(tvb, pinfo, 
							attr_newftree, hf_nfs_fattr4_quota_used, attr_vals_offset);
						break;

					case FATTR4_RAWDEV:
						attr_vals_offset = dissect_nfs_specdata4(tvb, 
							attr_vals_offset, pinfo, attr_newftree, "fattr4_rawdev");
						break;

					case FATTR4_SPACE_AVAIL:
						attr_vals_offset = dissect_rpc_uint64(tvb, pinfo, 
							attr_newftree, hf_nfs_fattr4_space_avail, 
							attr_vals_offset);
						break;

					case FATTR4_SPACE_FREE:
						attr_vals_offset = dissect_rpc_uint64(tvb, pinfo, 
							attr_newftree, hf_nfs_fattr4_space_free, attr_vals_offset);
						break;

					case FATTR4_SPACE_TOTAL:
						attr_vals_offset = dissect_rpc_uint64(tvb, pinfo, 
							attr_newftree, hf_nfs_fattr4_space_total, 
							attr_vals_offset);
						break;

					case FATTR4_SPACE_USED:
						attr_vals_offset = dissect_rpc_uint64(tvb, pinfo, 
							attr_newftree, hf_nfs_fattr4_space_used, attr_vals_offset);
						break;
					
					case FATTR4_SYSTEM:
						attr_vals_offset = dissect_rpc_bool(tvb, pinfo, 
							attr_newftree, hf_nfs_fattr4_system, attr_vals_offset);
						break;

					case FATTR4_TIME_ACCESS:
					case FATTR4_TIME_BACKUP:
					case FATTR4_TIME_CREATE:
					case FATTR4_TIME_DELTA:
					case FATTR4_TIME_METADATA:
					case FATTR4_TIME_MODIFY:
						attr_vals_offset = dissect_nfs_nfstime4(tvb, attr_vals_offset,
							pinfo, attr_newftree, "nfstime4");
						break;

					case FATTR4_TIME_ACCESS_SET:
					case FATTR4_TIME_MODIFY_SET:
						attr_vals_offset = dissect_nfs_settime4(tvb, 
							attr_vals_offset, pinfo, attr_newftree, "settime4");
						break;

					default:
						break;
					}
				}
			}

			sl <<= 1;
		}

		offset += 4;
	}

	g_free(bitmap);

	return offset;
}

int
dissect_nfs_fattr4(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree, char *name)
{
	proto_tree *newftree = NULL;
	proto_item *fitem = NULL;

	fitem = proto_tree_add_text(tree, tvb, offset, 4, "obj_attributes");

	if (fitem == NULL) return offset;

	newftree = proto_item_add_subtree(fitem, ett_nfs_fattr4);

	if (newftree == NULL) return offset;

	offset = dissect_nfs_attributes(tvb, offset, pinfo, newftree, name, 
		FATTR4_FULL_DISSECT);

	offset = dissect_nfsdata(tvb, offset, pinfo, tree, hf_nfs_attrlist4);

	return offset;
}

static const value_string names_open4_share_access[] = {
#define OPEN4_SHARE_ACCESS_READ 0x00000001
	{ OPEN4_SHARE_ACCESS_READ, "OPEN4_SHARE_ACCESS_READ" }, 
#define OPEN4_SHARE_ACCESS_WRITE 0x00000002
	{ OPEN4_SHARE_ACCESS_WRITE, "OPEN4_SHARE_ACCESS_WRITE" },
#define OPEN4_SHARE_ACCESS_BOTH 0x00000003
	{ OPEN4_SHARE_ACCESS_BOTH, "OPEN4_SHARE_ACCESS_BOTH" },
	{ 0, NULL }
};

int
dissect_nfs_open4_share_access(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree)
{
	guint share_access;

	share_access = tvb_get_ntohl(tvb, offset);
	proto_tree_add_uint(tree, hf_nfs_open4_share_access, tvb, offset, 4, 
		share_access);
	offset += 4;

	return offset;
}

static const value_string names_open4_share_deny[] = {
#define OPEN4_SHARE_DENY_NONE 0x00000000
	{ OPEN4_SHARE_DENY_NONE, "OPEN4_SHARE_DENY_NONE" },
#define OPEN4_SHARE_DENY_READ 0x00000001
	{ OPEN4_SHARE_DENY_READ, "OPEN4_SHARE_DENY_READ" },
#define OPEN4_SHARE_DENY_WRITE 0x00000002
	{ OPEN4_SHARE_DENY_WRITE, "OPEN4_SHARE_DENY_WRITE" },
#define OPEN4_SHARE_DENY_BOTH 0x00000003
	{ OPEN4_SHARE_DENY_BOTH, "OPEN4_SHARE_DENY_BOTH" },
	{ 0, NULL }
};

int
dissect_nfs_open4_share_deny(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree)
{
	guint deny_access;

	deny_access = tvb_get_ntohl(tvb, offset);
	proto_tree_add_uint(tree, hf_nfs_open4_share_deny, tvb, offset, 4,
		deny_access);
	offset += 4;

	return offset;
}

int
dissect_nfs_open_owner4(tvbuff_t *tvb, int offset, packet_info *pinfo, 
					 proto_tree *tree)
{
	offset = dissect_nfs_clientid4(tvb, offset, pinfo, tree);
	offset = dissect_nfsdata(tvb, offset, pinfo, tree, hf_nfs_open_owner4);

	return offset;
}

int
dissect_nfs_open_claim_delegate_cur4(tvbuff_t *tvb, int offset,
	packet_info *pinfo, proto_tree *tree, char *name)
{
	offset = dissect_rpc_uint64(tvb, pinfo, tree, 
		hf_nfs_stateid4_delegate_stateid, offset);
	offset = dissect_nfs_component4(tvb, offset, pinfo, tree, "file");
	return offset;
}

#define CLAIM_NULL				0
#define CLAIM_PREVIOUS			1
#define CLAIM_DELEGATE_CUR		2
#define CLAIM_DELEGATE_PREV	3

static const value_string names_claim_type4[] = {
	{	CLAIM_NULL,  		"CLAIM_NULL"  },
	{	CLAIM_PREVIOUS, 	"CLAIM_PREVIOUS" },
	{	CLAIM_DELEGATE_CUR, 	"CLAIM_DELEGATE_CUR" },
	{	CLAIM_DELEGATE_PREV,	"CLAIM_DELEGATE_PREV" },
	{	0, NULL }
};

int
dissect_nfs_open_claim4(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree)
{
	guint open_claim_type4;
	proto_item *fitem = NULL;
	proto_tree *newftree = NULL;

	open_claim_type4 = tvb_get_ntohl(tvb, offset);
	fitem = proto_tree_add_uint(tree, hf_nfs_open_claim_type4, tvb,
		offset+0, 4, open_claim_type4);
	offset += 4;

	if (fitem) {
		newftree = proto_item_add_subtree(fitem, ett_nfs_open_claim4);

		if (newftree) {

			switch(open_claim_type4)
			{
			case CLAIM_NULL:
				offset = dissect_nfs_component4(tvb, offset, pinfo, newftree, 
					"file");
				break;

			case CLAIM_PREVIOUS:
				offset = dissect_rpc_uint32(tvb, pinfo, newftree, 
					hf_nfs_delegate_type, offset);
				break;

			case CLAIM_DELEGATE_CUR:
				offset = dissect_nfs_open_claim_delegate_cur4(tvb, offset, pinfo, 
					newftree, "delegate_cur_info");
				break;

			case CLAIM_DELEGATE_PREV:
				offset = dissect_nfs_component4(tvb, offset, pinfo, newftree, 
					"file_delegate_prev");
				break;

			default:
				break;
			}
		}
	}

	return offset;
}

int
dissect_nfs_createhow4(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree, char *name)
{
	guint mode;

	/* This is intentional; we're using the same flags as NFSv3 */
	mode = tvb_get_ntohl(tvb, offset);
	proto_tree_add_uint(tree, hf_nfs_createmode3, tvb, offset, 4, mode);
	offset += 4;
	
	switch(mode)
	{
	case UNCHECKED:		/* UNCHECKED4 */
	case GUARDED:		/* GUARDED4 */
		offset = dissect_nfs_fattr4(tvb, offset, pinfo, tree, "createattrs");
		break;

	case EXCLUSIVE:		/* EXCLUSIVE4 */
		offset = dissect_rpc_uint64(tvb, pinfo, tree, hf_nfs_verifier4, offset);
		break;
	
	default:
		break;
	}

	return offset;
}

#define OPEN4_NOCREATE				0
#define OPEN4_CREATE					1
static const value_string names_opentype4[] = {
	{	OPEN4_NOCREATE,  "OPEN4_NOCREATE"  },
	{	OPEN4_CREATE, "OPEN4_CREATE" },
	{ 0, NULL }
};

int
dissect_nfs_openflag4(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree)
{
	guint opentype4;
	proto_item *fitem = NULL;
	proto_tree *newftree = NULL;

	opentype4 = tvb_get_ntohl(tvb, offset);
	fitem = proto_tree_add_uint(tree, hf_nfs_opentype4, tvb,
		offset+0, 4, opentype4);
	offset += 4;

	if (fitem) {
		newftree = proto_item_add_subtree(fitem, ett_nfs_opentype4);

		if (newftree) {

			switch(opentype4)
			{
			case OPEN4_CREATE:
				offset = dissect_nfs_createhow4(tvb, offset, pinfo, newftree, 
					"how");
				break;

			default:
				break;
			}
		}
	}

	return offset;
}

int
dissect_nfs_clientaddr4(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree, char *name)
{
	offset = dissect_nfs_opaque4(tvb, offset, pinfo, tree, "network id");
	offset = dissect_nfs_opaque4(tvb, offset, pinfo, tree, "universal address");

	return offset;
}
	

int
dissect_nfs_cb_client4(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree, char *name)
{
	offset = dissect_rpc_uint32(tvb, pinfo, tree, hf_nfs_cb_program, 
		offset);
	offset = dissect_nfs_clientaddr4(tvb, offset, pinfo, tree, "cb_location");
	return offset;
}

static const value_string names_stable_how4[] = {
#define UNSTABLE4 0
	{	UNSTABLE4,	"UNSTABLE4"	},
#define DATA_SYNC4 1
	{	DATA_SYNC4,	"DATA_SYNC4"	},
#define FILE_SYNC4 2
	{	FILE_SYNC4,	"FILE_SYNC4"	},
	{	0,	NULL	}
};

int
dissect_nfs_stable_how4(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree, char *name)
{
	guint stable_how4;

	stable_how4 = tvb_get_ntohl(tvb, offset);
	proto_tree_add_uint_format(tree, hf_nfs_stable_how4, tvb,
			offset+0, 4, stable_how4, "%s: %s (%u)", name,
			val_to_str(stable_how4, names_stable_how4, "%u"), stable_how4);
	offset += 4;

	return offset;
}

int
dissect_nfs_opaque4(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree, char *name)
{
	return dissect_nfsdata(tvb, offset, pinfo, tree, hf_nfs_data);
}

/* There is probably a better (built-in?) way to do this, but this works
 * for now.
 */

static const value_string names_nfsv4_operation[] = {
	{	NFS4_OP_ACCESS,					"ACCESS"	},
	{	NFS4_OP_CLOSE,						"CLOSE"	},
	{	NFS4_OP_COMMIT,					"COMMIT"	},
	{	NFS4_OP_CREATE,					"CREATE"	},
	{	NFS4_OP_DELEGPURGE,				"DELEGPURGE"	},
	{	NFS4_OP_DELEGRETURN,				"DELEGRETURN"	},
	{	NFS4_OP_GETATTR,					"GETATTR"	},
	{	NFS4_OP_GETFH,						"GETFH"	},
	{	NFS4_OP_LINK,						"LINK"	},
	{	NFS4_OP_LOCK,						"LOCK"	},
	{	NFS4_OP_LOCKT,						"LOCKT"	},
	{	NFS4_OP_LOCKU,						"LOCKU"	},
	{	NFS4_OP_LOOKUP,					"LOOKUP"	},
	{	NFS4_OP_NVERIFY,					"NVERIFY"	},
	{	NFS4_OP_OPEN,						"OPEN"	},
	{	NFS4_OP_OPENATTR,					"OPENATTR"	},
	{	NFS4_OP_OPEN_CONFIRM,			"OPEN_CONFIRM"	},
	{	NFS4_OP_OPEN_DOWNGRADE,			"OPEN_DOWNGRADE"	},
	{	NFS4_OP_PUTFH,						"PUTFH"	},
	{	NFS4_OP_PUTPUBFH,					"PUTPUBFH"	},
	{	NFS4_OP_PUTROOTFH,				"PUTROOTFH"	},
	{	NFS4_OP_READ,						"READ"	},
	{	NFS4_OP_READDIR,					"READDIR"	},
	{	NFS4_OP_READLINK,					"READLINK"	},
	{	NFS4_OP_REMOVE,					"REMOVE"	},
	{	NFS4_OP_RENAME,					"RENAME"	},
	{	NFS4_OP_RENEW,						"RENEW"	},
	{	NFS4_OP_RESTOREFH,				"RESTOREFH"	},
	{	NFS4_OP_SAVEFH,					"SAVEFH"	},
	{	NFS4_OP_SECINFO,					"SECINFO"	},
	{	NFS4_OP_SETATTR,					"SETATTR"	},
	{	NFS4_OP_SETCLIENTID,				"SETCLIENTID"	},
	{	NFS4_OP_SETCLIENTID_CONFIRM,	"SETCLIENTID_CONFIRM"	},
	{	NFS4_OP_VERIFY,					"VERIFY"	},
	{	NFS4_OP_WRITE,						"WRITE"	},
	{ 0, NULL }
};

gint *nfsv4_operation_ett[] =
{
	 &ett_nfs_access4 ,
	 &ett_nfs_close4 ,
	 &ett_nfs_commit4 ,
	 &ett_nfs_create4 ,
	 &ett_nfs_delegpurge4 ,
	 &ett_nfs_delegreturn4 ,
	 &ett_nfs_getattr4 ,
	 &ett_nfs_getfh4 ,
	 &ett_nfs_link4 ,
	 &ett_nfs_lock4 ,
	 &ett_nfs_lockt4 ,
	 &ett_nfs_locku4 ,
	 &ett_nfs_lookup4 ,
	 &ett_nfs_lookupp4 ,
	 &ett_nfs_nverify4 ,
	 &ett_nfs_open4 ,
	 &ett_nfs_openattr4 ,
	 &ett_nfs_open_confirm4 ,
	 &ett_nfs_open_downgrade4 ,
	 &ett_nfs_putfh4 ,
	 &ett_nfs_putpubfh4 ,
	 &ett_nfs_putrootfh4 ,
	 &ett_nfs_read4 ,
	 &ett_nfs_readdir4 ,
	 &ett_nfs_readlink4 ,
	 &ett_nfs_remove4 ,
	 &ett_nfs_rename4 ,
	 &ett_nfs_renew4 ,
	 &ett_nfs_restorefh4 ,
	 &ett_nfs_savefh4 ,
	 &ett_nfs_secinfo4 ,
	 &ett_nfs_setattr4 ,
	 &ett_nfs_setclientid4 ,
	 &ett_nfs_setclientid_confirm4 ,
	 &ett_nfs_verify4 ,
	 &ett_nfs_write4 
};

int
dissect_nfs_dirlist4(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree, char *name)
{
	proto_tree *newftree = NULL;
	guint nextentry;

	newftree = proto_item_add_subtree(tree, ett_nfs_dirlist4);
	if (newftree==NULL) return offset;

	nextentry = tvb_get_ntohl(tvb, offset);

	offset = dissect_rpc_bool(tvb, pinfo, newftree, hf_nfs_data_follows,
		offset);

	while (nextentry)
	{
		/* offset = dissect_nfs_cookie4(tvb, offset, pinfo, newftree); */
		offset = dissect_rpc_uint64(tvb, pinfo, newftree, hf_nfs_cookie4, offset);
		offset = dissect_nfs_component4(tvb, offset, pinfo, newftree, "name");
		offset = dissect_nfs_fattr4(tvb, offset, pinfo, newftree, "attrs");
		nextentry = tvb_get_ntohl(tvb, offset);
		offset += 4;
	}

	offset = dissect_rpc_bool(tvb, pinfo, newftree, hf_nfs_dirlist4_eof,
		offset);

	return offset;
}

int
dissect_nfs_change_info4(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree, char *name)
{
	proto_tree *newftree = NULL;
	proto_tree *fitem = NULL;

	fitem = proto_tree_add_text(tree, tvb, offset, 0, "%s", name);

	if (fitem) {
		newftree = proto_item_add_subtree(fitem, ett_nfs_change_info4);

		if (newftree) {
			offset = dissect_rpc_bool(tvb, pinfo, newftree, 
				hf_nfs_change_info4_atomic, offset);
			offset = dissect_nfs_changeid4(tvb, offset, pinfo, newftree, "before");
			offset = dissect_nfs_changeid4(tvb, offset, pinfo, newftree, "after");
		}
	}

	return offset;
}

static const value_string names_nfs_lock_type4[] =
{
#define READ_LT 1
	{	READ_LT,		"READ_LT"				},
#define WRITE_LT 2
	{	WRITE_LT,		"WRITE_LT"				},
#define READW_LT 3
	{	READW_LT,	"READW_LT"	},
#define WRITEW_LT 4
	{	WRITEW_LT,	"WRITEW_LT"	},
#define RELEASE_STATE 5
	{	RELEASE_STATE,	"RELEASE_STATE"	},
	{	0,	NULL	}
};

int
dissect_nfs_lock4denied(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree, char *name)
{
	offset = dissect_rpc_uint64(tvb, pinfo, tree, hf_nfs_offset4, offset);
	offset = dissect_nfs_length4(tvb, offset, pinfo, tree, "length");
	offset = dissect_rpc_uint32(tvb, pinfo, tree, hf_nfs_lock_type4, offset);
	offset = dissect_nfs_lock_owner4(tvb, offset, pinfo, tree, "owner");
	return offset;
}


int
dissect_nfs_ace4(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree *tree, char *name)
{
	offset = dissect_nfs_acetype4(tvb, offset, pinfo, tree, "type");
	offset = dissect_nfs_aceflag4(tvb, offset, pinfo, tree, "flag");
	offset = dissect_nfs_acemask4(tvb, offset, pinfo, tree, "access_mask");
	return dissect_nfs_utf8string(tvb, offset, pinfo, tree, hf_nfs_ace4, NULL);
}

static const value_string names_open4_result_flags[] = {
#define OPEN4_RESULT_MLOCK 0x00000001
	{ OPEN4_RESULT_MLOCK, "OPEN4_RESULT_MLOCK" }, 
#define OPEN4_RESULT_CONFIRM 0x00000002
	{ OPEN4_RESULT_CONFIRM, "OPEN4_RESULT_CONFIRM" },
	{ 0, NULL }
};

int 
dissect_nfs_open4_rflags(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree *tree, char *name)
{
	guint rflags;
	proto_item *rflags_item = NULL;
	proto_item *rflags_tree = NULL;

	rflags = tvb_get_ntohl(tvb, offset);

	if (tree)
	{
		rflags_item = proto_tree_add_text(tree, tvb, offset, 4,
			"%s: 0x%08x", name, rflags);

		if (rflags_item)
		{
			rflags_tree = proto_item_add_subtree(rflags_item, 
				ett_nfs_open4_result_flags);

			if (rflags_tree)
			{
				proto_tree_add_text(rflags_tree, tvb, offset, 4, "%s",
					decode_enumerated_bitfield(rflags, OPEN4_RESULT_MLOCK, 2,
					names_open4_result_flags, "%s"));

				proto_tree_add_text(rflags_tree, tvb, offset, 4, "%s",
					decode_enumerated_bitfield(rflags, OPEN4_RESULT_CONFIRM, 2,
					names_open4_result_flags, "%s"));
			}
		}
	}
	
	offset += 4;

	return offset;
}

int
dissect_nfs_stateid4(tvbuff_t *tvb, int offset, packet_info *pinfo,
		proto_tree *tree)
{
	proto_item *fitem = NULL;
	proto_tree *newftree = NULL;
	int sublen;
	int bytes_left;
	gboolean first_line;

	fitem = proto_tree_add_text(tree, tvb, offset, 4, "stateid");

	if (fitem) {
		newftree = proto_item_add_subtree(fitem, ett_nfs_stateid4);
		if (newftree) {
			offset = dissect_rpc_uint32(tvb, pinfo, newftree, hf_nfs_seqid4, 
				offset);

			bytes_left = 12;
			first_line = TRUE;

			while (bytes_left != 0)
			{
				sublen = 12;
				if (sublen > bytes_left)
					sublen = bytes_left;

				proto_tree_add_text(newftree, tvb, offset, sublen, "%s%s",
					first_line ? "other: " : "      ",
					tvb_bytes_to_str(tvb, offset, sublen));

				bytes_left -= sublen;
				offset += sublen;
				first_line = FALSE;
			}
		}
	}

	return offset;
}

int
dissect_nfs_open_read_delegation4(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree)
{
	offset = dissect_nfs_stateid4(tvb, offset, pinfo, tree);
	offset = dissect_rpc_bool(tvb, pinfo, tree, hf_nfs_recall4, offset);
	offset = dissect_nfs_ace4(tvb, offset, pinfo, tree, "permissions");

	return offset;
}

int
dissect_nfs_modified_limit4(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree, char *name)
{
	offset = dissect_rpc_uint32(tvb, pinfo, tree, hf_nfs_num_blocks, 
		offset);
	offset = dissect_rpc_uint32(tvb, pinfo, tree, hf_nfs_bytes_per_block,
		offset);
	return offset;
}

#define NFS_LIMIT_SIZE						1
#define NFS_LIMIT_BLOCKS					2
static const value_string names_limit_by4[] = {
	{	NFS_LIMIT_SIZE,  "NFS_LIMIT_SIZE"  },
	{	NFS_LIMIT_BLOCKS, "NFS_LIMIT_BLOCKS" },
	{ 0, NULL }
};

int
dissect_nfs_space_limit4(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree, char *name)
{
	guint limitby;

	limitby = tvb_get_ntohl(tvb, offset);
	proto_tree_add_uint(tree, hf_nfs_limit_by4, tvb, offset+0, 4, limitby);
	offset += 4;

	switch(limitby)
	{
	case NFS_LIMIT_SIZE:
		offset = dissect_rpc_uint64(tvb, pinfo, tree, hf_nfs_filesize, 
			offset);
		break;

	case NFS_LIMIT_BLOCKS:
		offset = dissect_nfs_modified_limit4(tvb, offset, pinfo, tree, 
			"mod_blocks");
		break;

	default:
		break;
	}

	return offset;
}

int
dissect_nfs_open_write_delegation4(tvbuff_t *tvb, int offset, 
	packet_info *pinfo, proto_tree *tree)
{
	offset = dissect_nfs_stateid4(tvb, offset, pinfo, tree);
	offset = dissect_rpc_bool(tvb, pinfo, tree, hf_nfs_recall, offset);
	offset = dissect_nfs_space_limit4(tvb, offset, pinfo, tree, "space_limit");
	return dissect_nfs_ace4(tvb, offset, pinfo, tree, "permissions");
}

#define OPEN_DELEGATE_NONE 0
#define OPEN_DELEGATE_READ 1
#define OPEN_DELEGATE_WRITE 2
static const value_string names_open_delegation_type4[] = {
	{	OPEN_DELEGATE_NONE,  "OPEN_DELEGATE_NONE"  },
	{	OPEN_DELEGATE_READ, 	"OPEN_DELEGATE_READ" },
	{	OPEN_DELEGATE_WRITE,	"OPEN_DELEGATE_WRITE" },
	{ 0, NULL }
};

int
dissect_nfs_open_delegation4(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree, char *name)
{
	guint delegation_type;
	proto_tree *newftree = NULL;
	proto_item *fitem = NULL;

	delegation_type = tvb_get_ntohl(tvb, offset);
	proto_tree_add_uint(tree, hf_nfs_open_delegation_type4, tvb, offset+0, 
		4, delegation_type);
	offset += 4;

	if (fitem) {
		newftree = proto_item_add_subtree(fitem, ett_nfs_open_delegation4);

		switch(delegation_type)
		{
		case OPEN_DELEGATE_NONE:
			break;

		case OPEN_DELEGATE_READ:
			offset = dissect_nfs_open_read_delegation4(tvb, offset, pinfo, 
				newftree);
			break;

		case OPEN_DELEGATE_WRITE:
			offset = dissect_nfs_open_write_delegation4(tvb, offset, pinfo, 
				newftree);
			break;

		default:
			break;
		}
	}

	return offset;
}

int
dissect_nfs_rpcsec_gss_info(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree)
{
	guint data_follows;

	while ((data_follows = tvb_get_ntohl(tvb, offset)))
	{
		offset += 4;
		offset = dissect_nfsdata(tvb, offset, pinfo, tree, hf_nfs_sec_oid4); 
		offset = dissect_rpc_uint32(tvb, pinfo, tree, hf_nfs_qop4, offset);
		offset = dissect_rpc_uint32(tvb, pinfo, tree, 
				hf_nfs_secinfo_rpcsec_gss_info_service, offset);
	}

	return offset;
}

int
dissect_nfs_open_to_lock_owner4(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree)
{
	offset = dissect_rpc_uint32(tvb, pinfo, tree, hf_nfs_seqid4, offset);
	offset = dissect_nfs_stateid4(tvb, offset, pinfo, tree);
	offset = dissect_rpc_uint32(tvb, pinfo, tree, hf_nfs_lock_seqid4, offset);
	offset = dissect_nfs_lock_owner4(tvb, offset, pinfo, tree, "owner");

	return offset;
}

int
dissect_nfs_exist_lock_owner4(tvbuff_t *tvb, int offset, packet_info *pinfo,
	proto_tree *tree)
{
	offset = dissect_nfs_stateid4(tvb, offset, pinfo, tree);
	offset = dissect_rpc_uint32(tvb, pinfo, tree, hf_nfs_lock_seqid4, offset);

	return offset;
}

int
dissect_nfs_locker4(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree *tree)
{
	guint new_lock_owner;

	new_lock_owner = tvb_get_ntohl(tvb, offset);
	offset = dissect_rpc_bool(tvb, pinfo, tree, hf_nfs_new_lock_owner, offset);
	
	if (new_lock_owner)
		offset = dissect_nfs_open_to_lock_owner4(tvb, offset, pinfo, tree);
	else
		offset = dissect_nfs_exist_lock_owner4(tvb, offset, pinfo, tree);

	return offset;
}

int
dissect_nfs_argop4(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree *tree)
{
	guint ops, ops_counter;
	guint opcode;
	proto_item *fitem;
	proto_tree *ftree = NULL;
	proto_tree *newftree = NULL;

	ops = tvb_get_ntohl(tvb, offset+0);

	fitem = proto_tree_add_text(tree, tvb, offset, 4, 
		"Operations (count: %d)", ops);
	offset += 4;

	if (fitem == NULL) return offset;

	ftree = proto_item_add_subtree(fitem, ett_nfs_argop4);

	if (ftree == NULL) return offset;

	for (ops_counter=0; ops_counter<ops; ops_counter++)
	{
		opcode = tvb_get_ntohl(tvb, offset);

		fitem = proto_tree_add_uint(ftree, hf_nfs_argop4, tvb, offset, 4, 
			opcode);
		offset += 4;

		if (opcode < NFS4_OP_ACCESS || opcode > NFS4_OP_WRITE)
			break;

		if (fitem == NULL)	break;

		newftree = proto_item_add_subtree(fitem, *nfsv4_operation_ett[opcode-3]);
		if (newftree == NULL)	break;

		switch(opcode)
		{
		case NFS4_OP_ACCESS:
			offset = dissect_access(tvb, offset, pinfo, newftree, "access");
			break;

		case NFS4_OP_CLOSE:
			offset = dissect_rpc_uint32(tvb, pinfo, newftree, hf_nfs_seqid4,
				offset);
			offset = dissect_nfs_stateid4(tvb, offset, pinfo, newftree);
			break;

		case NFS4_OP_COMMIT:
			offset = dissect_rpc_uint64(tvb, pinfo, newftree, hf_nfs_offset4,
				offset);
			offset = dissect_rpc_uint32(tvb, pinfo, newftree, hf_nfs_count4,
				offset);
			break;

		case NFS4_OP_CREATE:
			{
				guint create_type;

				create_type = tvb_get_ntohl(tvb, offset);
				offset = dissect_rpc_uint32(tvb, pinfo, newftree, hf_nfs_ftype4, 
					offset);

				switch(create_type)
				{
				case NF4LNK:
					offset = dissect_nfs_linktext4(tvb, offset, pinfo, newftree, 
						"linkdata");
					break;
				
				case NF4BLK:
				case NF4CHR:
					offset = dissect_nfs_specdata4(tvb, offset, pinfo, 
						newftree, "devdata");
					break;

				case NF4SOCK:
				case NF4FIFO:
				case NF4DIR:
					break;

				default:
					break;
				}

				offset = dissect_nfs_component4(tvb, offset, pinfo, newftree, 
					"objname");

				offset = dissect_nfs_fattr4(tvb, offset, pinfo, newftree, 
					"createattrs");
			}
			break;

		case NFS4_OP_DELEGPURGE:
			offset = dissect_rpc_uint64(tvb, pinfo, newftree, 
				hf_nfs_clientid4, offset);
			break;

		case NFS4_OP_DELEGRETURN:
			offset = dissect_nfs_stateid4(tvb, offset, pinfo, newftree);
			break;

		case NFS4_OP_GETATTR:
			offset = dissect_nfs_attributes(tvb, offset, pinfo, newftree, 
				"attr_request", FATTR4_BITMAP_ONLY);
			break;

		case NFS4_OP_GETFH:
			break;

		case NFS4_OP_LINK:
			offset = dissect_nfs_component4(tvb, offset, pinfo, newftree, 
				"newname");
			break;

		case NFS4_OP_LOCK:
			offset = dissect_rpc_uint32(tvb, pinfo, newftree, hf_nfs_lock_type4,
				offset);
			offset = dissect_rpc_bool(tvb, pinfo, newftree, hf_nfs_lock4_reclaim,
				offset);
			offset = dissect_rpc_uint64(tvb, pinfo, newftree, hf_nfs_offset4,
				offset);
			offset = dissect_nfs_length4(tvb, offset, pinfo, newftree, "length");
			offset = dissect_nfs_locker4(tvb, offset, pinfo, newftree);
			break;

		case NFS4_OP_LOCKT:
			offset = dissect_rpc_uint32(tvb, pinfo, newftree, hf_nfs_lock_type4,
				offset);
			offset = dissect_nfs_lock_owner4(tvb, offset, pinfo, newftree, 
				"owner");
			offset = dissect_rpc_uint64(tvb, pinfo, newftree, hf_nfs_offset4,
				offset);
			offset = dissect_nfs_length4(tvb, offset, pinfo, newftree, "length");
			break;

		case NFS4_OP_LOCKU:
			offset = dissect_rpc_uint32(tvb, pinfo, newftree, hf_nfs_lock_type4,
				offset);
			offset = dissect_rpc_uint32(tvb, pinfo, tree, hf_nfs_seqid4, offset);
			offset = dissect_nfs_stateid4(tvb, offset, pinfo, newftree);
			offset = dissect_rpc_uint64(tvb, pinfo, newftree, hf_nfs_offset4,
				offset);
			offset = dissect_nfs_length4(tvb, offset, pinfo, newftree, "length");
			break;

		case NFS4_OP_LOOKUP:
			offset = dissect_nfs_component4(tvb, offset, pinfo, newftree, 
				"objname");
			break;

		case NFS4_OP_LOOKUPP:
			break;

		case NFS4_OP_NVERIFY:
			offset = dissect_nfs_fattr4(tvb, offset, pinfo, newftree, 
				"obj_attributes");
			break;

		case NFS4_OP_OPEN:
			offset = dissect_rpc_uint32(tvb, pinfo, newftree, hf_nfs_seqid4, 
				offset);
			offset = dissect_nfs_open4_share_access(tvb, offset, pinfo, newftree);
			offset = dissect_nfs_open4_share_deny(tvb, offset, pinfo, newftree);
			offset = dissect_nfs_open_owner4(tvb, offset, pinfo, newftree);
			offset = dissect_nfs_openflag4(tvb, offset, pinfo, newftree);
			offset = dissect_nfs_open_claim4(tvb, offset, pinfo, newftree);
			break;

		case NFS4_OP_OPENATTR:
			offset = dissect_rpc_bool(tvb, pinfo, newftree, hf_nfs_attrdircreate,
				offset);
			break;

		case NFS4_OP_OPEN_CONFIRM:
			offset = dissect_nfs_stateid4(tvb, offset, pinfo, newftree);
			offset = dissect_rpc_uint32(tvb, pinfo, newftree, hf_nfs_seqid4, 
				offset);
			break;

		case NFS4_OP_OPEN_DOWNGRADE:
			offset = dissect_nfs_stateid4(tvb, offset, pinfo, newftree);
			offset = dissect_rpc_uint32(tvb, pinfo, newftree, hf_nfs_seqid4, 
				offset);
			offset = dissect_nfs_open4_share_access(tvb, offset, pinfo, newftree);
			offset = dissect_nfs_open4_share_deny(tvb, offset, pinfo, newftree);
			break;

		case NFS4_OP_PUTFH:
			offset = dissect_nfs_fh4(tvb, offset, pinfo, newftree, "filehandle");
			break;

		case NFS4_OP_PUTPUBFH:
		case NFS4_OP_PUTROOTFH:
			break;

		case NFS4_OP_READ:
			offset = dissect_nfs_stateid4(tvb, offset, pinfo, newftree);
			offset = dissect_rpc_uint64(tvb, pinfo, newftree, hf_nfs_offset4,
				offset);
			offset = dissect_rpc_uint32(tvb, pinfo, newftree, hf_nfs_count4,
				offset);
			break;

		case NFS4_OP_READDIR:
			offset = dissect_rpc_uint64(tvb, pinfo, newftree, hf_nfs_cookie4,
				offset);
			offset = dissect_rpc_uint64(tvb, pinfo, newftree, hf_nfs_cookieverf4,
				offset);
			offset = dissect_rpc_uint32(tvb, pinfo, newftree, 
				hf_nfs_count4_dircount, offset);
			offset = dissect_rpc_uint32(tvb, pinfo, newftree,
				hf_nfs_count4_maxcount, offset);
			offset = dissect_nfs_attributes(tvb, offset, pinfo, newftree, "attr", 
				FATTR4_BITMAP_ONLY);
			break;

		case NFS4_OP_READLINK:
			break;

		case NFS4_OP_REMOVE:
			offset = dissect_nfs_component4(tvb, offset, pinfo, newftree, 
				"target");
			break;

		case NFS4_OP_RENAME:
			offset = dissect_nfs_component4(tvb, offset, pinfo, newftree, 
				"oldname");
			offset = dissect_nfs_component4(tvb, offset, pinfo, newftree, 
				"newname");
			break;

		case NFS4_OP_RENEW:
			offset = dissect_nfs_clientid4(tvb, offset, pinfo, newftree);
			break;
	
		case NFS4_OP_RESTOREFH:
		case NFS4_OP_SAVEFH:
			break;

		case NFS4_OP_SECINFO:
			offset = dissect_nfs_component4(tvb, offset, pinfo, newftree, "name");
			break;

		case NFS4_OP_SETATTR:
			offset = dissect_nfs_stateid4(tvb, offset, pinfo, newftree);
			offset = dissect_nfs_fattr4(tvb, offset, pinfo, newftree, 
				"obj_attributes");
			break;

		case NFS4_OP_SETCLIENTID:
			{
				proto_tree *client_tree = NULL;

				fitem = proto_tree_add_text(newftree, tvb, offset, 0, "client");

				if (fitem) {
					client_tree = proto_item_add_subtree(fitem, 
						ett_nfs_client_id4);

					if (newftree)
					{
						offset = dissect_nfs_clientid4(tvb, offset, pinfo, 
							client_tree);

						offset = dissect_nfsdata(tvb, offset, pinfo, client_tree, 
							hf_nfs_client_id4_id); 
					}
				}

				fitem = proto_tree_add_text(newftree, tvb, offset, 0, "callback");
				if (fitem) {
					newftree = proto_item_add_subtree(fitem, ett_nfs_cb_client4);
					if (newftree)
						offset = dissect_nfs_cb_client4(tvb, offset, pinfo, newftree, 
							"callback");
				}
			}
			break;

		case NFS4_OP_SETCLIENTID_CONFIRM:
			offset = dissect_nfs_clientid4(tvb, offset, pinfo, newftree);
			break;
		
		case NFS4_OP_VERIFY:
			offset = dissect_nfs_fattr4(tvb, offset, pinfo, newftree, 
				"obj_attributes");
			break;

		case NFS4_OP_WRITE:
			offset = dissect_nfs_stateid4(tvb, offset, pinfo, newftree);
			offset = dissect_rpc_uint64(tvb, pinfo, newftree, hf_nfs_offset4,
				offset);
			offset = dissect_nfs_stable_how4(tvb, offset, pinfo, newftree, 
				"stable");
			offset = dissect_nfs_opaque4(tvb, offset, pinfo, newftree, "data");
			break;
		
		default:
			break;
		}
	}

	return offset;
}

int
dissect_nfs4_compound_call(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree* tree)
{
	offset = dissect_nfs_utf8string(tvb, offset, pinfo, tree, hf_nfs_tag4, NULL);
	offset = dissect_rpc_uint32(tvb, pinfo, tree, hf_nfs_minorversion,
		offset);
	offset = dissect_nfs_argop4(tvb, offset, pinfo, tree);

	return offset;
}

int
dissect_nfs_resop4(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree *tree, char *name)
{
	guint ops, ops_counter;
	guint opcode;
	proto_item *fitem;
	proto_tree *ftree = NULL;
	proto_tree *newftree = NULL;
	guint32 status;

	ops = tvb_get_ntohl(tvb, offset+0);
	fitem = proto_tree_add_text(tree, tvb, offset, 4, 
		"Operations (count: %d)", ops);
	offset += 4;

	if (fitem == NULL)	return offset;

	ftree = proto_item_add_subtree(fitem, ett_nfs_resop4);

	if (ftree == NULL)	return offset;		/* error adding new subtree */

	for (ops_counter = 0; ops_counter < ops; ops_counter++)
	{
		opcode = tvb_get_ntohl(tvb, offset);

		/* sanity check for bogus packets */
		if (opcode < NFS4_OP_ACCESS || opcode > NFS4_OP_WRITE)	break;

		fitem = proto_tree_add_uint(ftree, hf_nfs_resop4, tvb, offset, 4, 
			opcode);
		offset += 4;

		if (fitem == NULL)	break;		/* error adding new item to tree */

		newftree = proto_item_add_subtree(fitem, *nfsv4_operation_ett[opcode-3]);

		if (newftree == NULL)
			break;		/* error adding new subtree to operation item */

		offset = dissect_nfs_nfsstat4(tvb, offset, pinfo, newftree, &status);

		/*
		 * With the exception of NFS4_OP_LOCK, NFS4_OP_LOCKT, and 
		 * NFS4_OP_SETATTR, all other ops do *not* return data with the
		 * failed status code.
		 */
		if ((status != NFS4_OK) &&
			((opcode != NFS4_OP_LOCK) && (opcode != NFS4_OP_LOCKT) &&
			(opcode != NFS4_OP_SETATTR)))
			continue;

		/* These parsing routines are only executed if the status is NFS4_OK */
		switch(opcode)
		{
		case NFS4_OP_ACCESS:
			offset = dissect_access(tvb, offset, pinfo, newftree, "Supported");
			offset = dissect_access(tvb, offset, pinfo, newftree, "Access");
			break;

		case NFS4_OP_CLOSE:
			offset = dissect_nfs_stateid4(tvb, offset, pinfo, newftree);
			break;

		case NFS4_OP_COMMIT:
			offset = dissect_rpc_uint64(tvb, pinfo, newftree, hf_nfs_verifier4, 
				offset);
			break;

		case NFS4_OP_CREATE:
			offset = dissect_nfs_change_info4(tvb, offset, pinfo, newftree, 
				"change_info");
			offset = dissect_nfs_attributes(tvb, offset, pinfo, newftree,
				"attrsset", FATTR4_BITMAP_ONLY);
			break;

		case NFS4_OP_GETATTR:
			offset = dissect_nfs_fattr4(tvb, offset, pinfo, newftree, 
				"obj_attributes");
			break;

		case NFS4_OP_GETFH:
			offset = dissect_nfs_fh4(tvb, offset, pinfo, newftree, "Filehandle");
			break;

		case NFS4_OP_LINK:
			offset = dissect_nfs_change_info4(tvb, offset, pinfo, newftree, 
				"change_info");
			break;

		case NFS4_OP_LOCK:
		case NFS4_OP_LOCKT:
			if (status == NFS4_OK)
				offset = dissect_nfs_stateid4(tvb, offset, pinfo, newftree);
			else
			if (status == NFS4ERR_DENIED)
				offset = dissect_nfs_lock4denied(tvb, offset, pinfo, newftree, 
					"denied");
			break;

		case NFS4_OP_LOCKU:
			offset = dissect_nfs_stateid4(tvb, offset, pinfo, newftree);
			break;

		case NFS4_OP_OPEN:
			offset = dissect_nfs_stateid4(tvb, offset, pinfo, newftree);
			offset = dissect_nfs_change_info4(tvb, offset, pinfo, newftree, 
				"change_info");
			offset = dissect_nfs_open4_rflags(tvb, offset, pinfo, newftree, 
				"result_flags");
			offset = dissect_nfs_attributes(tvb, offset, pinfo, newftree, 
				"attrsset", FATTR4_BITMAP_ONLY);
			offset = dissect_nfs_open_delegation4(tvb, offset, pinfo, newftree, 
				"delegation");
			break;

		case NFS4_OP_OPEN_CONFIRM:
		case NFS4_OP_OPEN_DOWNGRADE:
			offset = dissect_nfs_stateid4(tvb, offset, pinfo, newftree);
			break;

		case NFS4_OP_READ:
			offset = dissect_rpc_uint32(tvb, pinfo, newftree, hf_nfs_eof, 
				offset);
			offset = dissect_nfs_opaque4(tvb, offset, pinfo, newftree, "data");
			break;

		case NFS4_OP_READDIR:
			offset = dissect_rpc_uint64(tvb, pinfo, newftree, hf_nfs_verifier4, 
				offset);
			offset = dissect_nfs_dirlist4(tvb, offset, pinfo, newftree, "reply");
			break;

		case NFS4_OP_READLINK:
			offset = dissect_nfs_linktext4(tvb, offset, pinfo, newftree, "link");
			break;

		case NFS4_OP_REMOVE:
			offset = dissect_nfs_change_info4(tvb, offset, pinfo, newftree, 
				"change_info");
			break;

		case NFS4_OP_RENAME:
			offset = dissect_nfs_change_info4(tvb, offset, pinfo, newftree, 
				"source_cinfo");
			offset = dissect_nfs_change_info4(tvb, offset, pinfo, newftree,
				"target_cinfo");
			break;

		case NFS4_OP_SECINFO:
			{
				guint data_follows;
				guint flavor;
				proto_item *fitem;
				proto_tree *secftree;

				while ((data_follows = tvb_get_ntohl(tvb, offset)))
				{
					offset += 4;

					flavor = tvb_get_ntohl(tvb, offset);
					fitem = proto_tree_add_uint(tree, hf_nfs_secinfo_flavor, tvb, 
							offset, 4, flavor);
					offset += 4;

					if (fitem) 
					{
						switch(flavor)
						{
							case RPCSEC_GSS:
								secftree = proto_item_add_subtree(fitem, 
										ett_nfs_secinfo4_flavor_info);
								if (secftree)
									offset = dissect_nfs_rpcsec_gss_info(tvb, offset,
											pinfo, secftree);
								break;

							default:
								break;
						}
					}
				}
			}
			break;

		case NFS4_OP_SETATTR:
			offset = dissect_nfs_attributes(tvb, offset, pinfo, newftree, 
				"attrsset", FATTR4_BITMAP_ONLY);
			break;

		case NFS4_OP_SETCLIENTID:
			if (status == NFS4_OK)
				offset = dissect_rpc_uint64(tvb, pinfo, newftree,
					hf_nfs_clientid4, offset);
			else
			if (status == NFS4ERR_CLID_INUSE)
				offset = dissect_nfs_clientaddr4(tvb, offset, pinfo, newftree,
					"client_using");
			break;

		case NFS4_OP_WRITE:
			offset = dissect_rpc_uint32(tvb, pinfo, newftree, hf_nfs_count4,
				offset);
			offset = dissect_nfs_stable_how4(tvb, offset, pinfo, newftree, 
				"committed");
			offset = dissect_rpc_uint64(tvb, pinfo, newftree, hf_nfs_verifier4, 
				offset);
			break;

		default:
			break;
		}
	}

	return offset;
}

int
dissect_nfs4_compound_reply(tvbuff_t *tvb, int offset, packet_info *pinfo, 
	proto_tree* tree)
{
	guint32 status;

	offset = dissect_nfs_nfsstat4(tvb, offset, pinfo, tree, &status);
	offset = dissect_nfs_utf8string(tvb, offset, pinfo, tree, hf_nfs_tag4, NULL);
	offset = dissect_nfs_resop4(tvb, offset, pinfo, tree, "arguments");

	return offset;
}


/* proc number, "proc name", dissect_request, dissect_reply */
/* NULL as function pointer means: type of arguments is "void". */
static const vsff nfs3_proc[] = {
	{ 0,	"NULL",		/* OK */
	NULL,				NULL },
	{ 1,	"GETATTR",	/* OK */
	dissect_nfs3_getattr_call,	dissect_nfs3_getattr_reply },
	{ 2,	"SETATTR",	/* OK */
	dissect_nfs3_setattr_call,	dissect_nfs3_setattr_reply },
	{ 3,	"LOOKUP",	/* OK */
	dissect_nfs3_lookup_call,	dissect_nfs3_lookup_reply },
	{ 4,	"ACCESS",	/* OK */
	dissect_nfs3_access_call,	dissect_nfs3_access_reply },
	{ 5,	"READLINK",	/* OK */
	dissect_nfs3_nfs_fh3_call,	dissect_nfs3_readlink_reply },
	{ 6,	"READ",		/* OK */
	dissect_nfs3_read_call,		dissect_nfs3_read_reply },
	{ 7,	"WRITE",	/* OK */
	dissect_nfs3_write_call,	dissect_nfs3_write_reply },
	{ 8,	"CREATE",	/* OK */
	dissect_nfs3_create_call,	dissect_nfs3_create_reply },
	{ 9,	"MKDIR",	/* OK */
	dissect_nfs3_mkdir_call,	dissect_nfs3_create_reply },
	{ 10,	"SYMLINK",	/* OK */
	dissect_nfs3_symlink_call,	dissect_nfs3_create_reply },
	{ 11,	"MKNOD",	/* OK */
	dissect_nfs3_mknod_call,	dissect_nfs3_create_reply },
	{ 12,	"REMOVE",	/* OK */
	dissect_nfs3_diropargs3_call,	dissect_nfs3_remove_reply },
	{ 13,	"RMDIR",	/* OK */
	dissect_nfs3_diropargs3_call,	dissect_nfs3_remove_reply },
	{ 14,	"RENAME",	/* OK */
	dissect_nfs3_rename_call,	dissect_nfs3_rename_reply },
	{ 15,	"LINK",		/* OK */
	dissect_nfs3_link_call,		dissect_nfs3_link_reply },
	{ 16,	"READDIR",	/* OK */
	dissect_nfs3_readdir_call,	dissect_nfs3_readdir_reply },
	{ 17,	"READDIRPLUS",	/* OK */
	dissect_nfs3_readdirplus_call,	dissect_nfs3_readdirplus_reply },
	{ 18,	"FSSTAT",	/* OK */
	dissect_nfs3_nfs_fh3_call,	dissect_nfs3_fsstat_reply },
	{ 19,	"FSINFO",	/* OK */
	dissect_nfs3_nfs_fh3_call,	dissect_nfs3_fsinfo_reply },
	{ 20,	"PATHCONF",	/* OK */
	dissect_nfs3_nfs_fh3_call,	dissect_nfs3_pathconf_reply },
	{ 21,	"COMMIT",	/* OK */
	dissect_nfs3_commit_call,	dissect_nfs3_commit_reply },
	{ 0,NULL,NULL,NULL }
};
/* end of NFS Version 3 */

static const vsff nfs4_proc[] = {
	{ 0, "NULL",
	NULL, NULL },
	{ 1, "COMPOUND",
	dissect_nfs4_compound_call, dissect_nfs4_compound_reply },
	{ 0, NULL, NULL, NULL }
};


static struct true_false_string yesno = { "Yes", "No" };


void
proto_register_nfs(void)
{
	static hf_register_info hf[] = {
		{ &hf_nfs_fh_length, {
			"length", "nfs.fh.length", FT_UINT32, BASE_DEC,
			NULL, 0, "file handle length", HFILL }},
		{ &hf_nfs_fh_hash, {
			"hash", "nfs.fh.hash", FT_UINT32, BASE_HEX,
			NULL, 0, "file handle hash", HFILL }},
		{ &hf_nfs_fh_fsid_major, {
			"major", "nfs.fh.fsid.major", FT_UINT32, BASE_DEC,
			NULL, 0, "major file system ID", HFILL }},
		{ &hf_nfs_fh_fsid_minor, {
			"minor", "nfs.fh.fsid.minor", FT_UINT32, BASE_DEC,
			NULL, 0, "minor file system ID", HFILL }},
		{ &hf_nfs_fh_fsid_inode, {
			"inode", "nfs.fh.fsid.inode", FT_UINT32, BASE_DEC,
			NULL, 0, "file system inode", HFILL }},
		{ &hf_nfs_fh_xfsid_major, {
			"exported major", "nfs.fh.xfsid.major", FT_UINT32, BASE_DEC,
			NULL, 0, "exported major file system ID", HFILL }},
		{ &hf_nfs_fh_xfsid_minor, {
			"exported minor", "nfs.fh.xfsid.minor", FT_UINT32, BASE_DEC,
			NULL, 0, "exported minor file system ID", HFILL }},
		{ &hf_nfs_fh_fstype, {
			"file system type", "nfs.fh.fstype", FT_UINT32, BASE_DEC,
			NULL, 0, "file system type", HFILL }},
		{ &hf_nfs_fh_fn, {
			"file number", "nfs.fh.fn", FT_UINT32, BASE_DEC,
			NULL, 0, "file number", HFILL }},
		{ &hf_nfs_fh_fn_len, {
			"length", "nfs.fh.fn.len", FT_UINT32, BASE_DEC,
			NULL, 0, "file number length", HFILL }},
		{ &hf_nfs_fh_fn_inode, {
			"inode", "nfs.fh.fn.inode", FT_UINT32, BASE_DEC,
			NULL, 0, "file number inode", HFILL }},
		{ &hf_nfs_fh_fn_generation, {
			"generation", "nfs.fh.fn.generation", FT_UINT32, BASE_DEC,
			NULL, 0, "file number generation", HFILL }},
		{ &hf_nfs_fh_xfn, {
			"exported file number", "nfs.fh.xfn", FT_UINT32, BASE_DEC,
			NULL, 0, "exported file number", HFILL }},
		{ &hf_nfs_fh_xfn_len, {
			"length", "nfs.fh.xfn.len", FT_UINT32, BASE_DEC,
			NULL, 0, "exported file number length", HFILL }},
		{ &hf_nfs_fh_xfn_inode, {
			"exported inode", "nfs.fh.xfn.inode", FT_UINT32, BASE_DEC,
			NULL, 0, "exported file number inode", HFILL }},
		{ &hf_nfs_fh_xfn_generation, {
			"generation", "nfs.fh.xfn.generation", FT_UINT32, BASE_DEC,
			NULL, 0, "exported file number generation", HFILL }},
		{ &hf_nfs_fh_dentry, {
			"dentry", "nfs.fh.dentry", FT_UINT32, BASE_HEX,
			NULL, 0, "dentry (cookie)", HFILL }},
		{ &hf_nfs_fh_dev, {
			"device", "nfs.fh.dev", FT_UINT32, BASE_DEC,
			NULL, 0, "device", HFILL }},
		{ &hf_nfs_fh_xdev, {
			"exported device", "nfs.fh.xdev", FT_UINT32, BASE_DEC,
			NULL, 0, "exported device", HFILL }},
		{ &hf_nfs_fh_dirinode, {
			"directory inode", "nfs.fh.dirinode", FT_UINT32, BASE_DEC,
			NULL, 0, "directory inode", HFILL }},
		{ &hf_nfs_fh_pinode, {
			"pseudo inode", "nfs.fh.pinode", FT_UINT32, BASE_HEX,
			NULL, 0, "pseudo inode", HFILL }},
		{ &hf_nfs_fh_hp_len, {
			"length", "nfs.fh.hp.len", FT_UINT32, BASE_DEC,
			NULL, 0, "hash path length", HFILL }},
		{ &hf_nfs_fh_version, {
			"version", "nfs.fh.version", FT_UINT8, BASE_DEC,
			NULL, 0, "file handle layout version", HFILL }},
		{ &hf_nfs_fh_auth_type, {
			"auth_type", "nfs.fh.auth_type", FT_UINT8, BASE_DEC,
			VALS(auth_type_names), 0, "authentication type", HFILL }},
		{ &hf_nfs_fh_fsid_type, {
			"fsid_type", "nfs.fh.fsid_type", FT_UINT8, BASE_DEC,
			VALS(fsid_type_names), 0, "file system ID type", HFILL }},
		{ &hf_nfs_fh_fileid_type, {
			"fileid_type", "nfs.fh.fileid_type", FT_UINT8, BASE_DEC,
			VALS(fileid_type_names), 0, "file ID type", HFILL }},
		{ &hf_nfs_stat, {
			"Status", "nfs.status2", FT_UINT32, BASE_DEC,
			VALS(names_nfs_stat), 0, "Reply status", HFILL }},
		{ &hf_nfs_full_name, {
			"Full Name", "nfs.full_name", FT_STRING, BASE_DEC,
			NULL, 0, "Full Name", HFILL }},
		{ &hf_nfs_name, {
			"Name", "nfs.name", FT_STRING, BASE_DEC,
			NULL, 0, "Name", HFILL }},
		{ &hf_nfs_readlink_data, {
			"Data", "nfs.readlink.data", FT_STRING, BASE_DEC,
			NULL, 0, "Symbolic Link Data", HFILL }},
		{ &hf_nfs_read_offset, {
			"Offset", "nfs.read.offset", FT_UINT32, BASE_DEC,
			NULL, 0, "Read Offset", HFILL }},
		{ &hf_nfs_read_count, {
			"Count", "nfs.read.count", FT_UINT32, BASE_DEC,
			NULL, 0, "Read Count", HFILL }},
		{ &hf_nfs_read_totalcount, {
			"Total Count", "nfs.read.totalcount", FT_UINT32, BASE_DEC,
			NULL, 0, "Total Count (obsolete)", HFILL }},
		{ &hf_nfs_data, {
			"Data", "nfs.data", FT_BYTES, BASE_DEC,
			NULL, 0, "Data", HFILL }},
		{ &hf_nfs_write_beginoffset, {
			"Begin Offset", "nfs.write.beginoffset", FT_UINT32, BASE_DEC,
			NULL, 0, "Begin offset (obsolete)", HFILL }},
		{ &hf_nfs_write_offset, {
			"Offset", "nfs.write.offset", FT_UINT32, BASE_DEC,
			NULL, 0, "Offset", HFILL }},
		{ &hf_nfs_write_totalcount, {
			"Total Count", "nfs.write.totalcount", FT_UINT32, BASE_DEC,
			NULL, 0, "Total Count (obsolete)", HFILL }},
		{ &hf_nfs_symlink_to, {
			"To", "nfs.symlink.to", FT_STRING, BASE_DEC,
			NULL, 0, "Symbolic link destination name", HFILL }},
		{ &hf_nfs_readdir_cookie, {
			"Cookie", "nfs.readdir.cookie", FT_UINT32, BASE_DEC,
			NULL, 0, "Directory Cookie", HFILL }},
		{ &hf_nfs_readdir_count, {
			"Count", "nfs.readdir.count", FT_UINT32, BASE_DEC,
			NULL, 0, "Directory Count", HFILL }},

		{ &hf_nfs_readdir_entry, {
			"Entry", "nfs.readdir.entry", FT_NONE, 0,
			NULL, 0, "Directory Entry", HFILL }},

		{ &hf_nfs_readdir_entry_fileid, {
			"File ID", "nfs.readdir.entry.fileid", FT_UINT32, BASE_DEC,
			NULL, 0, "File ID", HFILL }},

		{ &hf_nfs_readdir_entry_name, {
			"Name", "nfs.readdir.entry.name", FT_STRING, BASE_DEC,
			NULL, 0, "Name", HFILL }},

		{ &hf_nfs_readdir_entry_cookie, {
			"Cookie", "nfs.readdir.entry.cookie", FT_UINT32, BASE_DEC,
			NULL, 0, "Directory Cookie", HFILL }},

		{ &hf_nfs_readdir_entry3_fileid, {
			"File ID", "nfs.readdir.entry3.fileid", FT_UINT64, BASE_DEC,
			NULL, 0, "File ID", HFILL }},

		{ &hf_nfs_readdir_entry3_name, {
			"Name", "nfs.readdir.entry3.name", FT_STRING, BASE_DEC,
			NULL, 0, "Name", HFILL }},

		{ &hf_nfs_readdir_entry3_cookie, {
			"Cookie", "nfs.readdir.entry3.cookie", FT_UINT64, BASE_DEC,
			NULL, 0, "Directory Cookie", HFILL }},

		{ &hf_nfs_readdirplus_entry_fileid, {
			"File ID", "nfs.readdirplus.entry.fileid", FT_UINT64, BASE_DEC,
			NULL, 0, "Name", HFILL }},

		{ &hf_nfs_readdirplus_entry_name, {
			"Name", "nfs.readdirplus.entry.name", FT_STRING, BASE_DEC,
			NULL, 0, "Name", HFILL }},

		{ &hf_nfs_readdirplus_entry_cookie, {
			"Cookie", "nfs.readdirplus.entry.cookie", FT_UINT64, BASE_DEC,
			NULL, 0, "Directory Cookie", HFILL }},

		{ &hf_nfs_readdir_eof, {
			"EOF", "nfs.readdir.eof", FT_UINT32, BASE_DEC,
			NULL, 0, "EOF", HFILL }},

		{ &hf_nfs_statfs_tsize, {
			"Transfer Size", "nfs.statfs.tsize", FT_UINT32, BASE_DEC,
			NULL, 0, "Transfer Size", HFILL }},
		{ &hf_nfs_statfs_bsize, {
			"Block Size", "nfs.statfs.bsize", FT_UINT32, BASE_DEC,
			NULL, 0, "Block Size", HFILL }},
		{ &hf_nfs_statfs_blocks, {
			"Total Blocks", "nfs.statfs.blocks", FT_UINT32, BASE_DEC,
			NULL, 0, "Total Blocks", HFILL }},
		{ &hf_nfs_statfs_bfree, {
			"Free Blocks", "nfs.statfs.bfree", FT_UINT32, BASE_DEC,
			NULL, 0, "Free Blocks", HFILL }},
		{ &hf_nfs_statfs_bavail, {
			"Available Blocks", "nfs.statfs.bavail", FT_UINT32, BASE_DEC,
			NULL, 0, "Available Blocks", HFILL }},
		{ &hf_nfs_ftype3, {
			"Type", "nfs.type", FT_UINT32, BASE_DEC,
			VALS(names_nfs_ftype3), 0, "File Type", HFILL }},
		{ &hf_nfs_nfsstat3, {
			"Status", "nfs.status", FT_UINT32, BASE_DEC,
			VALS(names_nfs_nfsstat3), 0, "Reply status", HFILL }},
		{ &hf_nfs_read_eof, {
			"EOF", "nfs.read.eof", FT_BOOLEAN, BASE_NONE,
			&yesno, 0, "EOF", HFILL }},
		{ &hf_nfs_write_stable, {
			"Stable", "nfs.write.stable", FT_UINT32, BASE_DEC,
			VALS(names_stable_how), 0, "Stable", HFILL }},
		{ &hf_nfs_write_committed, {
			"Committed", "nfs.write.committed", FT_UINT32, BASE_DEC,
			VALS(names_stable_how), 0, "Committed", HFILL }},
		{ &hf_nfs_createmode3, {
			"Create Mode", "nfs.createmode", FT_UINT32, BASE_DEC,
			VALS(names_createmode3), 0, "Create Mode", HFILL }},
		{ &hf_nfs_fsstat_invarsec, {
			"invarsec", "nfs.fsstat.invarsec", FT_UINT32, BASE_DEC,
			NULL, 0, "probable number of seconds of file system invariance", HFILL }},
		{ &hf_nfs_fsinfo_rtmax, {
			"rtmax", "nfs.fsinfo.rtmax", FT_UINT32, BASE_DEC,
			NULL, 0, "maximum READ request", HFILL }},
		{ &hf_nfs_fsinfo_rtpref, {
			"rtpref", "nfs.fsinfo.rtpref", FT_UINT32, BASE_DEC,
			NULL, 0, "Preferred READ request size", HFILL }},
		{ &hf_nfs_fsinfo_rtmult, {
			"rtmult", "nfs.fsinfo.rtmult", FT_UINT32, BASE_DEC,
			NULL, 0, "Suggested READ multiple", HFILL }},
		{ &hf_nfs_fsinfo_wtmax, {
			"wtmax", "nfs.fsinfo.wtmax", FT_UINT32, BASE_DEC,
			NULL, 0, "Maximum WRITE request size", HFILL }},
		{ &hf_nfs_fsinfo_wtpref, {
			"wtpref", "nfs.fsinfo.wtpref", FT_UINT32, BASE_DEC,
			NULL, 0, "Preferred WRITE request size", HFILL }},
		{ &hf_nfs_fsinfo_wtmult, {
			"wtmult", "nfs.fsinfo.wtmult", FT_UINT32, BASE_DEC,
			NULL, 0, "Suggested WRITE multiple", HFILL }},
		{ &hf_nfs_fsinfo_dtpref, {
			"dtpref", "nfs.fsinfo.dtpref", FT_UINT32, BASE_DEC,
			NULL, 0, "Preferred READDIR request", HFILL }},
		{ &hf_nfs_fsinfo_maxfilesize, {
			"maxfilesize", "nfs.fsinfo.maxfilesize", FT_UINT64, BASE_DEC,
			NULL, 0, "Maximum file size", HFILL }},
		{ &hf_nfs_fsinfo_properties, {
			"Properties", "nfs.fsinfo.propeties", FT_UINT32, BASE_HEX,
			NULL, 0, "File System Properties", HFILL }},
		{ &hf_nfs_pathconf_linkmax, {
			"linkmax", "nfs.pathconf.linkmax", FT_UINT32, BASE_DEC,
			NULL, 0, "Maximum number of hard links", HFILL }},
		{ &hf_nfs_pathconf_name_max, {
			"name_max", "nfs.pathconf.name_max", FT_UINT32, BASE_DEC,
			NULL, 0, "Maximum file name length", HFILL }},
		{ &hf_nfs_pathconf_no_trunc, {
			"no_trunc", "nfs.pathconf.no_trunc", FT_BOOLEAN, BASE_NONE,
			&yesno, 0, "No long file name truncation", HFILL }},
		{ &hf_nfs_pathconf_chown_restricted, {
			"chown_restricted", "nfs.pathconf.chown_restricted", FT_BOOLEAN, 
			BASE_NONE, &yesno, 0, "chown is restricted to root", HFILL }},
		{ &hf_nfs_pathconf_case_insensitive, {
			"case_insensitive", "nfs.pathconf.case_insensitive", FT_BOOLEAN, 
			BASE_NONE, &yesno, 0, "file names are treated case insensitive", HFILL }},
		{ &hf_nfs_pathconf_case_preserving, {
			"case_preserving", "nfs.pathconf.case_preserving", FT_BOOLEAN, 
			BASE_NONE, &yesno, 0, "file name cases are preserved", HFILL }},

		{ &hf_nfs_fattr_type, {
			"type", "nfs.fattr.type", FT_UINT32, BASE_DEC,
			NULL, 0, "nfs.fattr.type", HFILL }},

		{ &hf_nfs_fattr_nlink, {
			"nlink", "nfs.fattr.nlink", FT_UINT32, BASE_DEC,
			NULL, 0, "nfs.fattr.nlink", HFILL }},

		{ &hf_nfs_fattr_uid, {
			"uid", "nfs.fattr.uid", FT_UINT32, BASE_DEC,
			NULL, 0, "nfs.fattr.uid", HFILL }},

		{ &hf_nfs_fattr_gid, {
			"gid", "nfs.fattr.gid", FT_UINT32, BASE_DEC,
			NULL, 0, "nfs.fattr.gid", HFILL }},

		{ &hf_nfs_fattr_size, {
			"size", "nfs.fattr.size", FT_UINT32, BASE_DEC,
			NULL, 0, "nfs.fattr.size", HFILL }},

		{ &hf_nfs_fattr_blocksize, {
			"blocksize", "nfs.fattr.blocksize", FT_UINT32, BASE_DEC,
			NULL, 0, "nfs.fattr.blocksize", HFILL }},

		{ &hf_nfs_fattr_rdev, {
			"rdev", "nfs.fattr.rdev", FT_UINT32, BASE_DEC,
			NULL, 0, "nfs.fattr.rdev", HFILL }},

		{ &hf_nfs_fattr_blocks, {
			"blocks", "nfs.fattr.blocks", FT_UINT32, BASE_DEC,
			NULL, 0, "nfs.fattr.blocks", HFILL }},

		{ &hf_nfs_fattr_fsid, {
			"fsid", "nfs.fattr.fsid", FT_UINT32, BASE_DEC,
			NULL, 0, "nfs.fattr.fsid", HFILL }},

		{ &hf_nfs_fattr_fileid, {
			"fileid", "nfs.fattr.fileid", FT_UINT32, BASE_DEC,
			NULL, 0, "nfs.fattr.fileid", HFILL }},

		{ &hf_nfs_fattr3_type, {
			"Type", "nfs.fattr3.type", FT_UINT32, BASE_DEC,
			VALS(names_nfs_ftype3), 0, "nfs.fattr3.type", HFILL }},

		{ &hf_nfs_fattr3_nlink, {
			"nlink", "nfs.fattr3.nlink", FT_UINT32, BASE_DEC,
			NULL, 0, "nfs.fattr3.nlink", HFILL }},

		{ &hf_nfs_fattr3_uid, {
			"uid", "nfs.fattr3.uid", FT_UINT32, BASE_DEC,
			NULL, 0, "nfs.fattr3.uid", HFILL }},

		{ &hf_nfs_fattr3_gid, {
			"gid", "nfs.fattr3.gid", FT_UINT32, BASE_DEC,
			NULL, 0, "nfs.fattr3.gid", HFILL }},

		{ &hf_nfs_fattr3_size, {
			"size", "nfs.fattr3.size", FT_UINT64, BASE_DEC,
			NULL, 0, "nfs.fattr3.size", HFILL }},

		{ &hf_nfs_fattr3_used, {
			"used", "nfs.fattr3.used", FT_UINT64, BASE_DEC,
			NULL, 0, "nfs.fattr3.used", HFILL }},

		{ &hf_nfs_fattr3_rdev, {
			"rdev", "nfs.fattr3.rdev", FT_UINT32, BASE_DEC,
			NULL, 0, "nfs.fattr3.rdev", HFILL }},

		{ &hf_nfs_fattr3_fsid, {
			"fsid", "nfs.fattr3.fsid", FT_UINT64, BASE_DEC,
			NULL, 0, "nfs.fattr3.fsid", HFILL }},

		{ &hf_nfs_fattr3_fileid, {
			"fileid", "nfs.fattr3.fileid", FT_UINT64, BASE_DEC,
			NULL, 0, "nfs.fattr3.fileid", HFILL }},

		{ &hf_nfs_wcc_attr_size, {
			"size", "nfs.wcc_attr.size", FT_UINT64, BASE_DEC,
			NULL, 0, "nfs.wcc_attr.size", HFILL }},

		{ &hf_nfs_set_size3_size, {
			"size", "nfs.set_size3.size", FT_UINT64, BASE_DEC,
			NULL, 0, "nfs.set_size3.size", HFILL }},

		{ &hf_nfs_uid3, {
			"uid", "nfs.uid3", FT_UINT32, BASE_DEC,
			NULL, 0, "nfs.uid3", HFILL }},

		{ &hf_nfs_gid3, {
			"gid", "nfs.gid3", FT_UINT32, BASE_DEC,
			NULL, 0, "nfs.gid3", HFILL }},

		{ &hf_nfs_cookie3, {
			"cookie", "nfs.cookie3", FT_UINT64, BASE_DEC,
			NULL, 0, "nfs.cookie3", HFILL }},

		{ &hf_nfs_offset3, {
			"offset", "nfs.offset3", FT_UINT64, BASE_DEC,
			NULL, 0, "nfs.offset3", HFILL }},

		{ &hf_nfs_count3, {
			"count", "nfs.count3", FT_UINT32, BASE_DEC,
			NULL, 0, "nfs.count3", HFILL }},

		{ &hf_nfs_count3_maxcount, {
			"maxcount", "nfs.count3_maxcount", FT_UINT32, BASE_DEC,
			NULL, 0, "nfs.count3_maxcount", HFILL }},

		{ &hf_nfs_count3_dircount, {
			"dircount", "nfs.count3_dircount", FT_UINT32, BASE_DEC,
			NULL, 0, "nfs.count3_dircount", HFILL }},

		{ &hf_nfs_fsstat3_resok_tbytes, {
			"Total bytes", "nfs.fsstat3_resok.tbytes", FT_UINT64, BASE_DEC,
			NULL, 0, "Total bytes", HFILL }},

		{ &hf_nfs_fsstat3_resok_fbytes, {
			"Free bytes", "nfs.fsstat3_resok.fbytes", FT_UINT64, BASE_DEC,
			NULL, 0, "Free bytes", HFILL }},

		{ &hf_nfs_fsstat3_resok_abytes, {
			"Available free bytes", "nfs.fsstat3_resok.abytes", FT_UINT64, BASE_DEC,
			NULL, 0, "Available free bytes", HFILL }},

		{ &hf_nfs_fsstat3_resok_tfiles, {
			"Total file slots", "nfs.fsstat3_resok.tfiles", FT_UINT64, BASE_DEC,
			NULL, 0, "Total file slots", HFILL }},

		{ &hf_nfs_fsstat3_resok_ffiles, {
			"Free file slots", "nfs.fsstat3_resok.ffiles", FT_UINT64, BASE_DEC,
			NULL, 0, "Free file slots", HFILL }},

		{ &hf_nfs_fsstat3_resok_afiles, {
			"Available free file slots", "nfs.fsstat3_resok.afiles", FT_UINT64, BASE_DEC,
			NULL, 0, "Available free file slots", HFILL }},

		/* NFSv4 */

		{ &hf_nfs_argop4, {
			"Opcode", "nfs.call.operation", FT_UINT32, BASE_DEC,
			VALS(names_nfsv4_operation), 0, "Opcode", HFILL }},

		{ &hf_nfs_resop4,	{
			"Opcode", "nfs.reply.operation", FT_UINT32, BASE_DEC,
			VALS(names_nfsv4_operation), 0, "Opcode", HFILL }},

		{ &hf_nfs_linktext4, {
			"Name", "nfs.symlink.linktext", FT_STRING, BASE_DEC,
			NULL, 0, "Symbolic link contents", HFILL }},

		{ &hf_nfs_component4, {
			"Filename", "nfs.pathname.component", FT_STRING, BASE_DEC,
			NULL, 0, "Pathname component", HFILL }},

		{ &hf_nfs_tag4, {
			"Tag", "nfs.tag", FT_STRING, BASE_DEC,
			NULL, 0, "Tag", HFILL }},

		{ &hf_nfs_clientid4, {
			"clientid", "nfs.clientid", FT_UINT64, BASE_DEC,
			NULL, 0, "Client ID", HFILL }},

		{ &hf_nfs_ace4, {
			"ace", "nfs.ace", FT_STRING, BASE_DEC,
			NULL, 0, "Access Control Entry", HFILL }},

		{ &hf_nfs_recall, {
			"EOF", "nfs.recall", FT_BOOLEAN, BASE_NONE,
			&yesno, 0, "Recall", HFILL }},

		{ &hf_nfs_open_claim_type4, {
			"Claim Type", "nfs.open.claim_type", FT_UINT32, BASE_DEC,
			VALS(names_claim_type4), 0, "Claim Type", HFILL }},

		{ &hf_nfs_opentype4, {
			"Open Type", "nfs.open.opentype", FT_UINT32, BASE_DEC,
			VALS(names_opentype4), 0, "Open Type", HFILL }},

		{ &hf_nfs_limit_by4, {
			"Space Limit", "nfs.open.limit_by", FT_UINT32, BASE_DEC,
			VALS(names_limit_by4), 0, "Limit By", HFILL }},

		{ &hf_nfs_open_delegation_type4, {
			"Delegation Type", "nfs.open.delegation_type", FT_UINT32, BASE_DEC,
			VALS(names_open_delegation_type4), 0, "Delegation Type", HFILL }},

		{ &hf_nfs_ftype4, {
			"nfs_ftype4", "nfs.nfs_ftype4", FT_UINT32, BASE_DEC,
			VALS(names_ftype4), 0, "nfs.nfs_ftype4", HFILL }},

		{ &hf_nfs_change_info4_atomic, {
			"Atomic", "nfs.change_info.atomic", FT_BOOLEAN, BASE_NONE,
			&yesno, 0, "Atomic", HFILL }},

		{ &hf_nfs_open4_share_access, {
			"share_access", "nfs.open4.share_access", FT_UINT32, BASE_DEC,
			VALS(names_open4_share_access), 0, "Share Access", HFILL }},

		{ &hf_nfs_open4_share_deny, {
			"share_deny", "nfs.open4.share_deny", FT_UINT32, BASE_DEC,
			VALS(names_open4_share_deny), 0, "Share Deny", HFILL }},

		{ &hf_nfs_seqid4, {
			"seqid", "nfs.seqid", FT_UINT32, BASE_HEX,
			NULL, 0, "Sequence ID", HFILL }},

		{ &hf_nfs_lock_seqid4, {
			"lock_seqid", "nfs.lock_seqid", FT_UINT32, BASE_HEX,
			NULL, 0, "Lock Sequence ID", HFILL }},

		{ &hf_nfs_mand_attr, {
			"mand_attr",	"nfs.attr", FT_UINT32, BASE_DEC,
			VALS(names_fattr4), 0, "Mandatory Attribute", HFILL }},

		{ &hf_nfs_recc_attr, {
			"recc_attr",	"nfs.attr", FT_UINT32, BASE_DEC,
			VALS(names_fattr4), 0, "Recommended Attribute", HFILL }},

		{ &hf_nfs_time_how4,	{
			"set_it", "nfs.set_it", FT_UINT32, BASE_DEC,
			VALS(names_time_how4), 0, "How To Set Time", HFILL }},

		{ &hf_nfs_attrlist4, {
			"attr_vals", "nfs.fattr4.attr_vals", FT_BYTES, BASE_DEC,
			NULL, 0, "attr_vals", HFILL }},

		{ &hf_nfs_fattr4_link_support, {
			"fattr4_link_support", "nfs.fattr4_link_support", FT_BOOLEAN, 
			BASE_NONE, &yesno, 0, "nfs.fattr4_link_support", HFILL }},

		{ &hf_nfs_fattr4_symlink_support, {
			"fattr4_symlink_support", "nfs.fattr4_symlink_support", FT_BOOLEAN, 
			BASE_NONE, &yesno, 0, "nfs.fattr4_symlink_support", HFILL }},

		{ &hf_nfs_fattr4_named_attr, {
			"fattr4_named_attr", "nfs.fattr4_named_attr", FT_BOOLEAN, BASE_NONE,
			&yesno, 0, "nfs.fattr4_named_attr", HFILL }},

		{ &hf_nfs_fattr4_unique_handles, {
			"fattr4_unique_handles", "nfs.fattr4_unique_handles", FT_BOOLEAN, 
			BASE_NONE, &yesno, 0, "nfs.fattr4_unique_handles", HFILL }},

		{ &hf_nfs_fattr4_archive, {
			"fattr4_archive", "nfs.fattr4_archive", FT_BOOLEAN, 
			BASE_NONE, &yesno, 0, "nfs.fattr4_archive", HFILL }},

		{ &hf_nfs_fattr4_cansettime, {
			"fattr4_cansettime", "nfs.fattr4_cansettime", FT_BOOLEAN, 
			BASE_NONE, &yesno, 0, "nfs.fattr4_cansettime", HFILL }},

		{ &hf_nfs_fattr4_case_insensitive, {
			"fattr4_case_insensitive", "nfs.fattr4_case_insensitive", FT_BOOLEAN, 
			BASE_NONE, &yesno, 0, "nfs.fattr4_case_insensitive", HFILL }},

		{ &hf_nfs_fattr4_case_preserving, {
			"fattr4_case_preserving", "nfs.fattr4_case_preserving", FT_BOOLEAN, 
			BASE_NONE, &yesno, 0, "nfs.fattr4_case_preserving", HFILL }},

		{ &hf_nfs_fattr4_chown_restricted, {
			"fattr4_chown_restricted", "nfs.fattr4_chown_restricted", FT_BOOLEAN, 
			BASE_NONE, &yesno, 0, "nfs.fattr4_chown_restricted", HFILL }},

		{ &hf_nfs_fattr4_hidden, {
			"fattr4_hidden", "nfs.fattr4_hidden", FT_BOOLEAN, 
			BASE_NONE, &yesno, 0, "nfs.fattr4_hidden", HFILL }},

		{ &hf_nfs_fattr4_homogeneous, {
			"fattr4_homogeneous", "nfs.fattr4_homogeneous", FT_BOOLEAN, 
			BASE_NONE, &yesno, 0, "nfs.fattr4_homogeneous", HFILL }},

		{ &hf_nfs_fattr4_mimetype, {
			"fattr4_mimetype", "nfs.fattr4_mimetype", FT_STRING, BASE_DEC,
			NULL, 0, "nfs.fattr4_mimetype", HFILL }},

		{ &hf_nfs_fattr4_no_trunc, {
			"fattr4_no_trunc", "nfs.fattr4_no_trunc", FT_BOOLEAN, 
			BASE_NONE, &yesno, 0, "nfs.fattr4_no_trunc", HFILL }},

		{ &hf_nfs_fattr4_system, {
			"fattr4_system", "nfs.fattr4_system", FT_BOOLEAN, 
			BASE_NONE, &yesno, 0, "nfs.fattr4_system", HFILL }},

		{ &hf_nfs_who, {
			"who", "nfs.who", FT_STRING, BASE_DEC,
			NULL, 0, "nfs.who", HFILL }},

		{ &hf_nfs_server, {
			"server", "nfs.server", FT_STRING, BASE_DEC,
			NULL, 0, "nfs.server", HFILL }},

		{ &hf_nfs_fattr4_owner, {
			"fattr4_owner", "nfs.fattr4_owner", FT_STRING, BASE_DEC,
			NULL, 0, "nfs.fattr4_owner", HFILL }},

		{ &hf_nfs_fattr4_owner_group, {
			"fattr4_owner_group", "nfs.fattr4_owner_group", FT_STRING, BASE_DEC,
			NULL, 0, "nfs.fattr4_owner_group", HFILL }},

		{ &hf_nfs_stable_how4, {
			"stable_how4", "nfs.stable_how4", FT_UINT32, BASE_DEC,
			VALS(names_stable_how4), 0, "nfs.stable_how4", HFILL }},

		{ &hf_nfs_dirlist4_eof, {
			"eof", "nfs.dirlist4.eof", FT_BOOLEAN,
			BASE_NONE, &yesno, 0, "nfs.dirlist4.eof", HFILL }},

		{ &hf_nfs_data_follows, {
			"data_follows", "nfs.data_follows", FT_BOOLEAN,
			BASE_NONE, &yesno, 0, "nfs.data_follows", HFILL }},

		{ &hf_nfs_stateid4, {
			"stateid", "nfs.stateid4", FT_UINT64, BASE_DEC,
			NULL, 0, "nfs.stateid4", HFILL }},

		{ &hf_nfs_offset4, {
			"offset", "nfs.offset4", FT_UINT64, BASE_DEC,
			NULL, 0, "nfs.offset4", HFILL }},

		{ &hf_nfs_specdata1, {
			"specdata1", "nfs.specdata1", FT_UINT32, BASE_DEC,
			NULL, 0, "nfs.specdata1", HFILL }},

		{ &hf_nfs_specdata2, {
			"specdata2", "nfs.specdata2", FT_UINT32, BASE_DEC,
			NULL, 0, "nfs.specdata2", HFILL }},

		{ &hf_nfs_lock_type4, {
			"locktype", "nfs.locktype4", FT_UINT32, BASE_DEC,
			VALS(names_nfs_lock_type4), 0, "nfs.locktype4", HFILL }},

		{ &hf_nfs_reclaim4, {
			"reclaim", "nfs.reclaim4", FT_UINT32, BASE_DEC,
			NULL, 0, "nfs.reclaim4", HFILL }},

		{ &hf_nfs_length4, {
			"length", "nfs.length4", FT_UINT64, BASE_DEC,
			NULL, 0, "nfs.length4", HFILL }},

		{ &hf_nfs_changeid4, {
			"changeid", "nfs.changeid4", FT_UINT64, BASE_DEC,
			NULL, 0, "nfs.changeid4", HFILL }},

		{ &hf_nfs_nfstime4_seconds, {
			"seconds", "nfs.nfstime4.seconds", FT_UINT64, BASE_DEC,
			NULL, 0, "nfs.nfstime4.seconds", HFILL }},

		{ &hf_nfs_nfstime4_nseconds, {
			"nseconds", "nfs.nfstime4.nseconds", FT_UINT32, BASE_DEC,
			NULL, 0, "nfs.nfstime4.nseconds", HFILL }},

		{ &hf_nfs_fsid4_major, {
			"fsid4.major", "nfs.fsid4.major", FT_UINT64, BASE_DEC,
			NULL, 0, "nfs.nfstime4.fsid4.major", HFILL }},

		{ &hf_nfs_fsid4_minor, {
			"fsid4.minor", "nfs.fsid4.minor", FT_UINT64, BASE_DEC,
			NULL, 0, "nfs.fsid4.minor", HFILL }},

		{ &hf_nfs_acetype4, {
			"acetype", "nfs.acetype4", FT_UINT32, BASE_DEC,
			NULL, 0, "nfs.acetype4", HFILL }},

		{ &hf_nfs_aceflag4, {
			"aceflag", "nfs.aceflag4", FT_UINT32, BASE_DEC,
			NULL, 0, "nfs.aceflag4", HFILL }},

		{ &hf_nfs_acemask4, {
			"acemask", "nfs.acemask4", FT_UINT32, BASE_DEC,
			NULL, 0, "nfs.acemask4", HFILL }},

		{ &hf_nfs_fattr4_size, {
			"size", "nfs.fattr4.size", FT_UINT64, BASE_DEC,
			NULL, 0, "nfs.fattr4.size", HFILL }},

		{ &hf_nfs_fattr4_lease_time, {
			"lease_time", "nfs.fattr4.lease_time", FT_UINT32, BASE_DEC,
			NULL, 0, "nfs.fattr4.lease_time", HFILL }},

		{ &hf_nfs_fattr4_aclsupport, {
			"aclsupport", "nfs.fattr4.aclsupport", FT_UINT32, BASE_DEC,
			NULL, 0, "nfs.fattr4.aclsupport", HFILL }},

		{ &hf_nfs_fattr4_fileid, {
			"fileid", "nfs.fattr4.fileid", FT_UINT64, BASE_DEC,
			NULL, 0, "nfs.fattr4.fileid", HFILL }},

		{ &hf_nfs_fattr4_files_avail, {
			"files_avail", "nfs.fattr4.files_avail", FT_UINT64, BASE_DEC,
			NULL, 0, "nfs.fattr4.files_avail", HFILL }},

		{ &hf_nfs_fattr4_files_free, {
			"files_free", "nfs.fattr4.files_free", FT_UINT64, BASE_DEC,
			NULL, 0, "nfs.fattr4.files_free", HFILL }},

		{ &hf_nfs_fattr4_files_total, {
			"files_total", "nfs.fattr4.files_total", FT_UINT64, BASE_DEC,
			NULL, 0, "nfs.fattr4.files_total", HFILL }},

		{ &hf_nfs_fattr4_maxfilesize, {
			"maxfilesize", "nfs.fattr4.maxfilesize", FT_UINT64, BASE_DEC,
			NULL, 0, "nfs.fattr4.maxfilesize", HFILL }},

		{ &hf_nfs_fattr4_maxlink, {
			"maxlink", "nfs.fattr4.maxlink", FT_UINT32, BASE_DEC,
			NULL, 0, "nfs.fattr4.maxlink", HFILL }},

		{ &hf_nfs_fattr4_maxname, {
			"maxname", "nfs.fattr4.maxname", FT_UINT32, BASE_DEC,
			NULL, 0, "nfs.fattr4.maxname", HFILL }},

		{ &hf_nfs_fattr4_numlinks, {
			"numlinks", "nfs.fattr4.numlinks", FT_UINT32, BASE_DEC,
			NULL, 0, "nfs.fattr4.numlinks", HFILL }},

		{ &hf_nfs_delegate_type, {
			"delegate_type", "nfs.delegate_type", FT_UINT32, BASE_DEC,
			NULL, 0, "nfs.delegate_type", HFILL }},

		{ &hf_nfs_secinfo_flavor, {
			"flavor", "nfs.secinfo.flavor", FT_UINT32, BASE_DEC,
			VALS(rpc_auth_flavor), 0, "nfs.secinfo.flavor", HFILL }},

		{ &hf_nfs_num_blocks, {
			"num_blocks", "nfs.num_blocks", FT_UINT32, BASE_DEC,
			NULL, 0, "nfs.num_blocks", HFILL }},

		{ &hf_nfs_bytes_per_block, {
			"bytes_per_block", "nfs.bytes_per_block", FT_UINT32, BASE_DEC,
			NULL, 0, "nfs.bytes_per_block", HFILL }},

		{ &hf_nfs_eof, {
			"eof", "nfs.eof", FT_UINT32, BASE_DEC,
			NULL, 0, "nfs.eof", HFILL }},

		{ &hf_nfs_fattr4_maxread, {
			"maxread", "nfs.fattr4.maxread", FT_UINT64, BASE_DEC,
			NULL, 0, "nfs.fattr4.maxread", HFILL }},

		{ &hf_nfs_fattr4_maxwrite, {
			"maxwrite", "nfs.fattr4.maxwrite", FT_UINT64, BASE_DEC,
			NULL, 0, "nfs.fattr4.maxwrite", HFILL }},

		{ &hf_nfs_fattr4_quota_hard, {
			"quota_hard", "nfs.fattr4.quota_hard", FT_UINT64, BASE_DEC,
			NULL, 0, "nfs.fattr4.quota_hard", HFILL }},

		{ &hf_nfs_fattr4_quota_soft, {
			"quota_soft", "nfs.fattr4.quota_soft", FT_UINT64, BASE_DEC,
			NULL, 0, "nfs.fattr4.quota_soft", HFILL }},

		{ &hf_nfs_fattr4_quota_used, {
			"quota_used", "nfs.fattr4.quota_used", FT_UINT64, BASE_DEC,
			NULL, 0, "nfs.fattr4.quota_used", HFILL }},

		{ &hf_nfs_fattr4_space_avail, {
			"space_avail", "nfs.fattr4.space_avail", FT_UINT64, BASE_DEC,
			NULL, 0, "nfs.fattr4.space_avail", HFILL }},

		{ &hf_nfs_fattr4_space_free, {
			"space_free", "nfs.fattr4.space_free", FT_UINT64, BASE_DEC,
			NULL, 0, "nfs.fattr4.space_free", HFILL }},

		{ &hf_nfs_fattr4_space_total, {
			"space_total", "nfs.fattr4.space_total", FT_UINT64, BASE_DEC,
			NULL, 0, "nfs.fattr4.space_total", HFILL }},

		{ &hf_nfs_fattr4_space_used, {
			"space_used", "nfs.fattr4.space_used", FT_UINT64, BASE_DEC,
			NULL, 0, "nfs.fattr4.space_used", HFILL }},

		{ &hf_nfs_stateid4_delegate_stateid, {
			"delegate_stateid", "nfs.delegate_stateid", FT_UINT64, BASE_DEC,
			NULL, 0, "nfs.delegate_stateid", HFILL }},

		{ &hf_nfs_verifier4, {
			"verifier", "nfs.verifier4", FT_UINT64, BASE_DEC,
			NULL, 0, "nfs.verifier4", HFILL }},

		{ &hf_nfs_cookie4, {
			"cookie", "nfs.cookie4", FT_UINT64, BASE_DEC,
			NULL, 0, "nfs.cookie4", HFILL }},

		{ &hf_nfs_cookieverf4, {
			"cookieverf", "nfs.cookieverf4", FT_UINT64, BASE_DEC,
			NULL, 0, "nfs.cookieverf4", HFILL }},

		{ &hf_nfs_cb_location, {
			"cb_location", "nfs.cb_location", FT_UINT32, BASE_DEC,
			NULL, 0, "nfs.cb_location", HFILL }},

		{ &hf_nfs_cb_program, {
			"cb_program", "nfs.cb_program", FT_UINT32, BASE_DEC,
			NULL, 0, "nfs.cb_program", HFILL }},

		{ &hf_nfs_recall4, {
			"recall", "nfs.recall4", FT_BOOLEAN, 
			BASE_NONE, &yesno, 0, "nfs.recall4", HFILL }},

		{ &hf_nfs_filesize, {
			"filesize", "nfs.filesize", FT_UINT64, BASE_DEC,
			NULL, 0, "nfs.filesize", HFILL }},

		{ &hf_nfs_count4, {
			"count", "nfs.count4", FT_UINT32, BASE_DEC,
			NULL, 0, "nfs.count4", HFILL }},

		{ &hf_nfs_count4_dircount, {
			"dircount", "nfs.dircount", FT_UINT32, BASE_DEC,
			NULL, 0, "nfs.dircount", HFILL }},

		{ &hf_nfs_count4_maxcount, {
			"maxcount", "nfs.maxcount", FT_UINT32, BASE_DEC,
			NULL, 0, "nfs.maxcount", HFILL }},

		{ &hf_nfs_minorversion, {
			"minorversion", "nfs.minorversion", FT_UINT32, BASE_DEC,
			NULL, 0, "nfs.minorversion", HFILL }},

		{ &hf_nfs_atime, {
			"atime", "nfs.atime", FT_ABSOLUTE_TIME, BASE_NONE,
			NULL, 0, "Access Time", HFILL }},

		{ &hf_nfs_atime_sec, {
			"seconds", "nfs.atime.sec", FT_UINT32, BASE_DEC,
			NULL, 0, "Access Time, Seconds", HFILL }},

		{ &hf_nfs_atime_nsec, {
			"nano seconds", "nfs.atime.nsec", FT_UINT32, BASE_DEC,
			NULL, 0, "Access Time, Nano-seconds", HFILL }},

		{ &hf_nfs_atime_usec, {
			"micro seconds", "nfs.atime.usec", FT_UINT32, BASE_DEC,
			NULL, 0, "Access Time, Micro-seconds", HFILL }},

		{ &hf_nfs_mtime, {
			"mtime", "nfs.mtime", FT_ABSOLUTE_TIME, BASE_NONE,
			NULL, 0, "Modify Time", HFILL }},

		{ &hf_nfs_mtime_sec, {
			"seconds", "nfs.mtime.sec", FT_UINT32, BASE_DEC,
			NULL, 0, "Modify Seconds", HFILL }},

		{ &hf_nfs_mtime_nsec, {
			"nano seconds", "nfs.mtime.nsec", FT_UINT32, BASE_DEC,
			NULL, 0, "Modify Time, Nano-seconds", HFILL }},

		{ &hf_nfs_mtime_usec, {
			"micro seconds", "nfs.mtime.usec", FT_UINT32, BASE_DEC,
			NULL, 0, "Modify Time, Micro-seconds", HFILL }},

		{ &hf_nfs_ctime, {
			"ctime", "nfs.ctime", FT_ABSOLUTE_TIME, BASE_NONE,
			NULL, 0, "Creation Time", HFILL }},

		{ &hf_nfs_ctime_sec, {
			"seconds", "nfs.ctime.sec", FT_UINT32, BASE_DEC,
			NULL, 0, "Creation Time, Seconds", HFILL }},

		{ &hf_nfs_ctime_nsec, {
			"nano seconds", "nfs.ctime.nsec", FT_UINT32, BASE_DEC,
			NULL, 0, "Creation Time, Nano-seconds", HFILL }},

		{ &hf_nfs_ctime_usec, {
			"micro seconds", "nfs.ctime.usec", FT_UINT32, BASE_DEC,
			NULL, 0, "Creation Time, Micro-seconds", HFILL }},

		{ &hf_nfs_dtime, {
			"time delta", "nfs.dtime", FT_RELATIVE_TIME, BASE_NONE,
			NULL, 0, "Time Delta", HFILL }},

		{ &hf_nfs_dtime_sec, {
			"seconds", "nfs.dtime.sec", FT_UINT32, BASE_DEC,
			NULL, 0, "Time Delta, Seconds", HFILL }},

		{ &hf_nfs_dtime_nsec, {
			"nano seconds", "nfs.dtime.nsec", FT_UINT32, BASE_DEC,
			NULL, 0, "Time Delta, Nano-seconds", HFILL }},

		{ &hf_nfs_open_owner4, {
			"owner", "nfs.open_owner4", FT_BYTES, BASE_DEC,
			NULL, 0, "owner", HFILL }},

		{ &hf_nfs_lock_owner4, {
			"owner", "nfs.lock_owner4", FT_BYTES, BASE_DEC,
			NULL, 0, "owner", HFILL }},

		{ &hf_nfs_secinfo_rpcsec_gss_info_service, {
			"service", "nfs.secinfo.rpcsec_gss_info.service", FT_UINT32,
			BASE_DEC, VALS(rpc_authgss_svc), 0, "service", HFILL }},

		{ &hf_nfs_attrdircreate, {
			"attribute dir create", "nfs.openattr4.createdir", FT_BOOLEAN, 
			BASE_NONE, &yesno, 0, "nfs.openattr4.createdir", HFILL }},

		{ &hf_nfs_new_lock_owner, {
			"new lock owner?", "nfs.lock.locker.new_lock_owner", FT_BOOLEAN, 
			BASE_NONE, &yesno, 0, "nfs.lock.locker.new_lock_owner", HFILL }},

		{ &hf_nfs_lock4_reclaim, {
			"reclaim?", "nfs.lock.reclaim", FT_BOOLEAN, 
			BASE_NONE, &yesno, 0, "nfs.lock.reclaim", HFILL }},

		{ &hf_nfs_sec_oid4, {
			"oid", "nfs.secinfo.flavor_info.rpcsec_gss_info.oid", FT_BYTES,
			BASE_DEC, NULL, 0, "oid", HFILL }},

		{ &hf_nfs_qop4, {
			"qop", "nfs.secinfo.flavor_info.rpcsec_gss_info.qop", FT_UINT32, 
			BASE_DEC, NULL, 0, "qop", HFILL }},

		{ &hf_nfs_client_id4_id, {
			"Data", "nfs.nfs_client_id4.id", FT_BYTES, BASE_DEC,
			NULL, 0, "Data", HFILL }},

		{ &hf_nfs_stateid4_other, {
			"Data", "nfs.stateid4.other", FT_BYTES, BASE_DEC,
			NULL, 0, "Data", HFILL }},
	};

	static gint *ett[] = {
		&ett_nfs,
		&ett_nfs_fh_encoding,
		&ett_nfs_fh_fsid,
		&ett_nfs_fh_xfsid,
		&ett_nfs_fh_fn,
		&ett_nfs_fh_xfn,
		&ett_nfs_fh_hp,
		&ett_nfs_fh_auth,
		&ett_nfs_fhandle,
		&ett_nfs_timeval,
		&ett_nfs_mode,
		&ett_nfs_fattr,
		&ett_nfs_sattr,
		&ett_nfs_diropargs,
		&ett_nfs_readdir_entry,
		&ett_nfs_mode3,
		&ett_nfs_specdata3,
		&ett_nfs_fh3,
		&ett_nfs_nfstime3,
		&ett_nfs_fattr3,
		&ett_nfs_post_op_fh3,
		&ett_nfs_sattr3,
		&ett_nfs_diropargs3,
		&ett_nfs_sattrguard3,
		&ett_nfs_set_mode3,
		&ett_nfs_set_uid3,
		&ett_nfs_set_gid3,
		&ett_nfs_set_size3,
		&ett_nfs_set_atime,
		&ett_nfs_set_mtime,
		&ett_nfs_pre_op_attr,
		&ett_nfs_post_op_attr,
		&ett_nfs_wcc_attr,
		&ett_nfs_wcc_data,
		&ett_nfs_access,
		&ett_nfs_fsinfo_properties,
		&ett_nfs_compound_call4,
		&ett_nfs_utf8string,
		&ett_nfs_argop4,
		&ett_nfs_resop4,
		&ett_nfs_access4,
		&ett_nfs_close4,
		&ett_nfs_commit4,
		&ett_nfs_create4,
		&ett_nfs_delegpurge4,
		&ett_nfs_delegreturn4,
		&ett_nfs_getattr4,
		&ett_nfs_getfh4,
		&ett_nfs_link4,
		&ett_nfs_lock4,
		&ett_nfs_lockt4,
		&ett_nfs_locku4,
		&ett_nfs_lookup4,
		&ett_nfs_lookupp4,
		&ett_nfs_nverify4,
		&ett_nfs_open4,
		&ett_nfs_openattr4,
		&ett_nfs_open_confirm4,
		&ett_nfs_open_downgrade4,
		&ett_nfs_putfh4,
		&ett_nfs_putpubfh4,
		&ett_nfs_putrootfh4,
		&ett_nfs_read4,
		&ett_nfs_readdir4,
		&ett_nfs_readlink4,
		&ett_nfs_remove4,
		&ett_nfs_rename4,
		&ett_nfs_renew4,
		&ett_nfs_restorefh4,
		&ett_nfs_savefh4,
		&ett_nfs_setattr4,
		&ett_nfs_setclientid4,
		&ett_nfs_setclientid_confirm4,
		&ett_nfs_verify4,
		&ett_nfs_write4,
		&ett_nfs_verifier4,
		&ett_nfs_opaque,
		&ett_nfs_dirlist4,
		&ett_nfs_pathname4,
		&ett_nfs_change_info4,
		&ett_nfs_open_delegation4,
		&ett_nfs_open_claim4,
		&ett_nfs_opentype4,
		&ett_nfs_lock_owner4,
		&ett_nfs_cb_client4,
		&ett_nfs_client_id4,
		&ett_nfs_bitmap4,
		&ett_nfs_fattr4,
		&ett_nfs_fsid4,
		&ett_nfs_fs_locations4,
		&ett_nfs_fs_location4,
		&ett_nfs_open4_result_flags,
		&ett_nfs_secinfo4,
		&ett_nfs_secinfo4_flavor_info,
		&ett_nfs_stateid4,
		&ett_nfs_fattr4_fh_expire_type,
	};
	module_t *nfs_module;

	proto_nfs = proto_register_protocol("Network File System", "NFS", "nfs");
	proto_register_field_array(proto_nfs, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));

	nfs_module=prefs_register_protocol(proto_nfs, NULL);
	prefs_register_bool_preference(nfs_module, "file_name_snooping",
				       "Snoop FH to filename mappings",
				       "Whether the dissector should snoop the FH to filename mappings by looking inside certain packets",
				       &nfs_file_name_snooping);
	prefs_register_bool_preference(nfs_module, "file_full_name_snooping",
				       "Snoop full path to filenames",
				       "Whether the dissector should snoop the full pathname for files for matching FH's",
				       &nfs_file_name_full_snooping);
	register_init_routine(nfs_name_snoop_init);
}

void
proto_reg_handoff_nfs(void)
{
	/* Register the protocol as RPC */
	rpc_init_prog(proto_nfs, NFS_PROGRAM, ett_nfs);
	/* Register the procedure tables */
	rpc_init_proc_table(NFS_PROGRAM, 2, nfs2_proc);
	rpc_init_proc_table(NFS_PROGRAM, 3, nfs3_proc);
	rpc_init_proc_table(NFS_PROGRAM, 4, nfs4_proc);
}
