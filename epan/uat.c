/*
 *  uat.c
 *
 * $Id$
 *
 *  User Accessible Tables
 *  Mantain an array of user accessible data strucures
 *
 * (c) 2007, Luis E. Garcia Ontanon <luis.ontanon@gmail.com>
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 2001 Gerald Combs
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <stdarg.h>

#include <glib.h>
#include <epan/emem.h>
#include <epan/report_err.h>
#include <epan/filesystem.h>
#include <epan/packet.h>
#include <epan/range.h>

#include "uat-int.h"

static GPtrArray* all_uats = NULL;

void uat_init(void) {
	all_uats = g_ptr_array_new();
}

uat_t* uat_new(const char* name,
			   size_t size,
			   const char* filename,
			   void** data_ptr,
			   guint* numitems_ptr,
			   const char* category,
			   const char* help,
			   uat_copy_cb_t copy_cb,
			   uat_update_cb_t update_cb,
			   uat_free_cb_t free_cb,
			   uat_field_t* flds_array) {
	uat_t* uat = g_malloc(sizeof(uat_t));
	guint i;
	
	if (!all_uats)
		all_uats = g_ptr_array_new();
	
	g_ptr_array_add(all_uats,uat);
	
	g_assert(name && size && filename && data_ptr && numitems_ptr);
	
	uat->name = g_strdup(name);
	uat->record_size = size;
	uat->filename = g_strdup(filename);
	uat->user_ptr = data_ptr;
	uat->nrows_p = numitems_ptr;
	uat->copy_cb = copy_cb;
	uat->update_cb = update_cb;
	uat->free_cb = free_cb;
	uat->fields = flds_array;
	uat->user_data = g_array_new(FALSE,FALSE,uat->record_size);
	uat->changed = FALSE;
	uat->rep = NULL;
	uat->free_rep = NULL;
	uat->help = help;
	uat->category = category;
	
	for (i=0;flds_array[i].name;i++) {
		fld_data_t* f = g_malloc(sizeof(fld_data_t));
	
		f->colnum = i+1;
		f->rep = NULL;
		f->free_rep = NULL;
		
		flds_array[i].priv = f;
	}
	
	uat->ncols = i;
	
	
	*data_ptr = NULL;
	*numitems_ptr = 0;
	
	return uat;
}

void* uat_add_record(uat_t* uat, const void* data) {
	void* rec;

	g_array_append_vals (uat->user_data, data, 1);
	
	rec = uat->user_data->data + (uat->record_size * (uat->user_data->len-1));
	
	if (uat->copy_cb) {
		uat->copy_cb(rec, data, uat->record_size);
	}
	
	UAT_UPDATE(uat);
	
	return rec;
}

void uat_swap(uat_t* uat, guint a, guint b) {
	guint s = uat->record_size;
	void* tmp = ep_alloc(s);
	
	g_assert( a < uat->user_data->len && b < uat->user_data->len );

	if (a == b) return;

	memcpy(tmp, UAT_INDEX_PTR(uat,a), s);
	memcpy(UAT_INDEX_PTR(uat,a), UAT_INDEX_PTR(uat,b), s);
	memcpy(UAT_INDEX_PTR(uat,b), tmp, s);

}

void uat_remove_record_idx(uat_t* uat, guint idx) {
	
	g_assert( idx < uat->user_data->len );

	if (uat->free_cb) {
		uat->free_cb(UAT_INDEX_PTR(uat,idx));
	}
	
	g_array_remove_index(uat->user_data, idx);
	
	UAT_UPDATE(uat);

}

gchar* uat_get_actual_filename(uat_t* uat, gboolean for_writing) {
	gchar* pers_fname =  get_persconffile_path(uat->filename,for_writing);

	if (! for_writing ) {
		gchar* data_fname = get_datafile_path(uat->filename);
		
		if (file_exists(data_fname)) {
			return data_fname;
		}
	}
	
	if ((! file_exists(pers_fname) ) && (! for_writing ) ) {
		return NULL;
	}
	
	return pers_fname;
}

static void putfld(FILE* fp, void* rec, uat_field_t* f) {
	guint fld_len;
	const char* fld_ptr;
	
	f->cb.tostr(rec,&fld_ptr,&fld_len,f->cbdata.tostr,f->fld_data);
	
	switch(f->mode){
		case  PT_TXTMOD_ENUM:
		case  PT_TXTMOD_STRING: {
			guint i;
			
			putc('"',fp);
			
			for(i=0;i<fld_len;i++) {
				char c = fld_ptr[i];
				
				if (c == '"' || c == '\\' || ! isprint((guchar)c) ) {
					fprintf(fp,"\\x%.2x",c);
				} else {
					putc(c,fp);
				}
			}
			
			putc('"',fp);
			return;
		}
		case PT_TXTMOD_HEXBYTES: {
			guint i;
			
			for(i=0;i<fld_len;i++) {
				fprintf(fp,"%.2x",((guint8*)fld_ptr)[i]);
			}
			
			return;
		}
		default:
			g_assert_not_reached();
	}
}

gboolean uat_save(uat_t* uat, char** error) {
	guint i;
	gchar* fname = uat_get_actual_filename(uat,TRUE);
	FILE* fp;
	
	if (! fname ) return FALSE;

	fp = fopen(fname,"w");
	
	if (!fp) {
		*error = ep_strdup_printf("uat_save: error opening '%s': %s",fname,strerror(errno));
		return FALSE;
	}

	*error = NULL;

	fprintf(fp,"# This file is automatically generated, DO NOT MODIFY.\n");
	
	for ( i = 0 ; i < uat->user_data->len ; i++ ) {
		void* rec = uat->user_data->data + (uat->record_size * i);
		uat_field_t* f;
		guint j;

		f = uat->fields;
		
			
		for( j=0 ; j < uat->ncols ; j++ ) {
			putfld(fp, rec, &(f[j]));
			fputs((j == uat->ncols - 1) ? "\n" : "," ,fp);
		}

	}

	fclose(fp);
	
	uat->changed = FALSE;
	
	return TRUE;
}

void uat_destroy(uat_t* uat) {
	/* XXX still missing a destructor */
	g_ptr_array_remove(all_uats,uat);
	
}

void uat_clear(uat_t* uat) {
	guint i;

	for ( i = 0 ; i < uat->user_data->len ; i++ ) {
		if (uat->free_cb) {
			uat->free_cb(UAT_INDEX_PTR(uat,i));
		}
	}
	
	g_array_set_size(uat->user_data,0);
	
	*((uat)->user_ptr) = NULL;
	*((uat)->nrows_p) = 0;
}

void* uat_dup(uat_t* uat, guint* len_p) {
	guint size = (uat->record_size * uat->user_data->len);
	*len_p = size;
	return size ? g_memdup(uat->user_data->data,size) : NULL ;
}

void* uat_se_dup(uat_t* uat, guint* len_p) {
	guint size = (uat->record_size * uat->user_data->len);
	*len_p = size;
	return size ? se_memdup(uat->user_data->data,size) : NULL ;
}

void uat_cleanup(void) {
	while( all_uats->len ) {
		uat_destroy((uat_t*)all_uats->pdata);
	}

	g_ptr_array_free(all_uats,TRUE);
}


void uat_foreach_table(uat_cb_t cb,void* user_data) {
	guint i;
	
	for (i=0; i < all_uats->len; i++)
		cb(g_ptr_array_index(all_uats,i), user_data);
	
}	


void uat_load_all(void) {
	guint i;
	gchar* err;
	
	for (i=0; i < all_uats->len; i++) {
		uat_t* u = g_ptr_array_index(all_uats,i);
		err = NULL;
		
		uat_load(u, &err);
		
		if (err) {
			report_failure("Error loading table '%s': %s",u->name,err);
		}
	}
}


gboolean uat_fld_chk_str(void* u1 _U_, const char* strptr, unsigned len _U_, void* u2 _U_, void* u3 _U_, const char** err) {
	if (strptr == NULL) {
		*err = "NULL pointer";
		return FALSE;
	}
	
	*err = NULL;
	return TRUE;
}

gboolean uat_fld_chk_proto(void* u1 _U_, const char* strptr, unsigned len, void* u2 _U_, void* u3 _U_, const char** err) {
	if (len) {
		char* name = ep_strndup(strptr,len);
		g_strdown(name);
		g_strchug(name);
		
		if (find_dissector(name)) {
			*err = NULL;
			return TRUE;
		} else {
			*err = "dissector not found";
			return FALSE;
		}
	} else {
		*err = NULL;
		return TRUE;		
	}
}

gboolean uat_fld_chk_num_dec(void* u1 _U_, const char* strptr, unsigned len, void* u2 _U_, void* u3 _U_, const char** err) {
	char* str = ep_strndup(strptr,len);
	long i = strtol(str,&str,10);
	
	if ( ( i == 0) && (errno == ERANGE || errno == EINVAL) ) {
		*err = strerror(errno);
		return FALSE;
	}
	
	*err = NULL;
	return TRUE;
}

gboolean uat_fld_chk_num_hex(void* u1 _U_, const char* strptr, unsigned len, void* u2 _U_, void* u3 _U_, const char** err) {
	char* str = ep_strndup(strptr,len);
	long i = strtol(str,&str,16);
	
	if ( ( i == 0) && (errno == ERANGE || errno == EINVAL) ) {
		*err = strerror(errno);
		return FALSE;
	}
	
	*err = NULL;
	return TRUE;
}

gboolean uat_fld_chk_enum(void* u1 _U_, const char* strptr, unsigned len, void* v, void* u3 _U_, const char** err) {
	char* str = ep_strndup(strptr,len);
	guint i;
	value_string* vs = v;
	
	for(i=0;vs[i].strptr;i++) {
		if (g_str_equal(vs[i].strptr,str)) {
			*err = NULL;
			return TRUE;
		}
	}

	*err = ep_strdup_printf("invalid value: %s",str);
	return FALSE;
}

gboolean uat_fld_chk_range(void* u1 _U_, const char* strptr, unsigned len, void* v _U_, void* u3, const char** err) {
	char* str = ep_strndup(strptr,len);
	range_t* r = NULL;
	convert_ret_t ret = range_convert_str(&r, str,GPOINTER_TO_UINT(u3));
	
	switch (  ret ) {
		case CVT_NO_ERROR:
			*err = NULL;
			return TRUE;
		case CVT_SYNTAX_ERROR:
			*err = ep_strdup_printf("syntax error in range: %s",str);
			return FALSE;
		case CVT_NUMBER_TOO_BIG:
			*err = ep_strdup_printf("value too large in range: '%s' (max = %u)",str,GPOINTER_TO_UINT(u3));
			return FALSE;
		default:
			*err = "This should not happen, it is a bug in wireshark! please report to wireshark-dev@wireshark.org";
			return FALSE;
	}
}

static int xton(char d) {
	switch(d) {
		case '0': return 0;
		case '1': return 1; 
		case '2': return 2;
		case '3': return 3;
		case '4': return 4;
		case '5': return 5;
		case '6': return 6;
		case '7': return 7;
		case '8': return 8;
		case '9': return 9;
		case 'a':  case 'A': return 10;
		case 'b':  case 'B': return 11;
		case 'c':  case 'C': return 12;
		case 'd':  case 'D': return 13;
		case 'e':  case 'E': return 14;
		case 'f':  case 'F': return 15;
		default: return -1;
	}
}

char* uat_unbinstring(const char* si, guint in_len, guint* len_p) {
	guint8* buf;
	guint len = in_len/2;
	int i = 0;
	
	if (in_len%2) {
		return NULL;
	}
	
	buf= g_malloc(len);
	*len_p = len;
	
	while(in_len) {
		int d1 = xton(*(si++));
		int d0 = xton(*(si++));
		
		buf[i++] = (d1 * 16) + d0;
		
		in_len -= 2;
	}
	
	return (void*)buf;
}

char* uat_unesc(const char* si, guint in_len, guint* len_p) {
	char* buf = g_malloc0(in_len);
	char* p = buf;
	guint len = 0;
	const char* s;
	const char* in_end = si+in_len;

	for (s = (void*)si; s < in_end; s++) {
		switch(*s) {
			case '\\':
				switch(*(++s)) {
					case 'a': *(p++) = '\a'; len++; break;
					case 'b': *(p++) = '\b'; len++; break;
					case 'e': *(p++) = '\033' /* '\e' is non ANSI-C */; len++; printf("."); break;
					case 'f': *(p++) = '\f'; len++; break;
					case 'n': *(p++) = '\n'; len++; break;
					case 'r': *(p++) = '\r'; len++; break;
					case 't': *(p++) = '\t'; len++; break;
					case 'v': *(p++) = '\v'; len++; break;
						
					case '0':
					case '1': 
					case '2': 
					case '3': 
					case '4': 
					case '5': 
					case '6': 
					case '7': 
					{
						int c0 = 0;
						int c1 = 0;
						int c2 = 0;
						int c = 0;
						
						c0 = (*s) - '0';
						
						if ( s[1] >= '0' && s[1] <= '7' ) {
							c1 = c0;
							c0 = (*++s) - '0';
							
							if ( s[1] >= '0' && s[1] <= '7' ) {
								c2 = c1;
								c1 = c0;
								c0 = (*++s) - '0';
							}
						}
						c = (64 * c2) + (8 * c1) + c0;
						*(p++) = (char) (c > 255 ? 255 : c);
						len++; 
						break;
					}
						
					case 'x':
					{
						char c1 = *(s+1);
						char c0 = *(s+2);
						
						if (isxdigit((guchar)c1) && isxdigit((guchar)c0)) {
							*(p++) = (xton(c1) * 0x10) + xton(c0);
							s += 2;
						} else {
							*(p++) = *s;
						}
						len++;
						break;
					}
					default:
						*p++ = *s;
						break;
				}
				break;
			default:
				*(p++) = *s;
				len++;
				break;
		}
	}
	
	if (len_p) *len_p = len;
	return buf;
}

char* uat_undquote(const char* si, guint in_len, guint* len_p) {
	return uat_unesc(si+1,in_len-2,len_p);
}


char* uat_esc(const char* buf, guint len) {
	const guint8* end = ((guint8*)buf)+len;
	char* out = ep_alloc0((4*len)+1);
	const guint8* b;
	char* s = out;
	
	for (b = (void*)buf; b < end; b++) {
		if (isprint(*b) ) {
			*(s++) = (*b);
		} else {
			g_snprintf(s,5,"\\x%.2x",((guint)*b));
			s+=4;
		}
	}
	
	return out;
	
}

CHK_STR_IS_DEF(isprint)
CHK_STR_IS_DEF(isalpha)
CHK_STR_IS_DEF(isalnum)
CHK_STR_IS_DEF(isdigit)
CHK_STR_IS_DEF(isxdigit)

