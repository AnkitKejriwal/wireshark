/*
 *  uat.h
 *
 *  $Id$
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

#ifndef _UAT_H_
#define _UAT_H_

/*
 * uat mantains a dynamically allocated table accessible to the user
 * via a file and/or gui tables.
 *
 * the file is located either in userdir(when first read or when writen) or
 * in datadir for defaults (read only , it will be always written to userdir).
 *
 * the behaviour of the table is controlled by a series of callbacks
 * the caller must provide.
 *
 * BEWARE that the user can change an uat at (almost) any time, 
 * That is pointers to records in an uat are valid only during the call
 * to the function that obtains them (do not store them).
 *
 * UATs are meant for short tables of user data (passwords and such) there's
 * no quick access, you must iterate through them each time to fetch the record
 * you are looking for. Use uat_dup() or uat_se_dup() if necessary.
 *
 * Only users via gui or editing the file can add/remove records your code cannot.
 */

/* obscure data type to handle an uat */
typedef struct _uat_t uat_t;
/********************************************
 * Callbacks:
 * these instruct uat on how to deal with user info and data in records
 ********************************************/

/********
 * Callbacks for the entire table (these deal with entire records)
 ********/

/*
 * Copy CB
 * used to copy a record
 * optional, memcpy will be used if not given
 * copy(dest,orig,len)
 */
typedef void* (*uat_copy_cb_t)(void*, const void*, unsigned);

/*
 *
 * Free CB
 *
 * destroy a record's child data
 * (do not free the container, it will be handled by uat)
 * it is optional, no child data will be freed if no present
 * free(record)
 */
typedef void (*uat_free_cb_t)(void*);

/*
 * Update CB
 *
 * to be called after all record fields has been updated
 * optional, record will be updated always if not given
 * update(record,&error)
 */
typedef void (*uat_update_cb_t)(void* , char** );


/*******
 * Callbacks for single fields (these deal with single values)
 * the caller should provide one of these for every field!
 ********/

/* 
 * given an input string (ptr, len) checks if the value is OK for a field in the record.
 * it will return TRUE if OK or else
 * it will return FALSE and may set *error to inform the user on what's
 * wrong with the given input
 * optional, if not given any input is considered OK and the set cb will be called
 * chk(record, ptr, len, chk_data, fld_data, &error)
 */
typedef gboolean (*uat_fld_chk_cb_t)(void*, const char*, unsigned, void*, void*, char**);

/*
 * Set Field CB
 *
 * given an input string (ptr, len) sets the value of a field in the record,
 * it will return TRUE if OK or else
 * it will return FALSE and may set *error to inform the user on what's
 * wrong with the given input
 * it is mandatory
 * set(record, ptr, len, set_data, fld_data)
 */
typedef void (*uat_fld_set_cb_t)(void*, const char*, unsigned, void*, void*);

/*
 * given a record returns a string representation of the field
 * mandatory
 * tostr(record, &out_ptr, &out_len, tostr_data, fld_data)
 */
typedef void (*uat_fld_tostr_cb_t)(void*, char**, unsigned*, void*, void*);

/*********** 
 * Text Mode
 *
 * used for file and dialog representation of fileds in columns,
 * when the file is read it modifies the way the value is passed back to the fld_set_cb 
 * (see definition bellow for description)
 ***********/

typedef enum _uat_text_mode_t {
	PT_TXTMOD_NONE,
	/* not used */
	
	PT_TXTMOD_STRING,
	/*
	 file:
		 reads:
			 ,"\x20\x00\x30", as " \00",3
			 ,"", as "",0
			 ,, as NULL,0
		 writes:
			 ,"\x20\x30\x00\x20", for " 0\0 ",4
			 ,"", for *, 0
			 ,, for NULL, *
	 dialog:
		 accepts \x?? and other escapes
		 gets "",0 on empty string
	 */
	PT_TXTMOD_HEXBYTES,
	/*
	 file:
		 reads:
			 ,A1b2C3d4, as "\001\002\003\004",4
			 ,, as NULL,0
		 writes:
			 ,, on NULL, *
			 ,a1b2c3d4, on "\001\002\003\004",4
	 dialog:
		 "a1b2c3d4" as "\001\002\003\004",4
		 "a1 b2:c3d4" as "\001\002\003\004",4
		 "" as NULL,0
		 "invalid" as NULL,3
		 "a1b" as NULL, 1
	 */
} uat_text_mode_t;

/*
 * Fields
 *
 *
 */
typedef struct _uat_field_t {
	const char* name;
	uat_text_mode_t mode;
	
	struct {
		uat_fld_chk_cb_t chk;
		uat_fld_set_cb_t set;
		uat_fld_tostr_cb_t tostr;
	} cb;
	
	struct {
		void* chk;
		void* set;
		void* tostr;
	} cbdata;
	
	void* fld_data;
	
	struct _fld_data_t* priv;
} uat_field_t;

#define FLDFILL NULL
#define UAT_END_FIELDS {0,PT_TXTMOD_NONE,{0,0,0},{0,0,0},0,FLDFILL}



/*
 * uat_new()
 *
 * creates a new uat
 *
 * name: the name of the table
 *
 * data_ptr: a pointer to a null terminated array of pointers to the data
 *
 * default_data: a pinter to a struct containing default values
 *
 * size: the size of the structure
 *
 * filename: the filename to be used (either in userdir or datadir)
 *
 * copy_cb: a function that copies the data in the struct
 *
 * update_cb: will be called when a record is updated
 *
 * free_cb: will be called to destroy a struct in the dataset
 *
 * 
 */
uat_t* uat_new(const char* name,
			   size_t size,
			   char* filename,
			   void** data_ptr,
			   guint* num_items,
			   uat_copy_cb_t copy_cb,
			   uat_update_cb_t update_cb,
			   uat_free_cb_t free_cb,
			   uat_field_t* flds_array);


/*
 * uat_dup()
 * uat_se_dup()
 * make a reliable copy of an uat for internal use,
 * so that pointers to records can be kept through calls.
 * return NULL on zero len.
 */
void* uat_dup(uat_t*, guint* len_p); /* to be freed */
void* uat_se_dup(uat_t*, guint* len_p);

/*
 * Some common uat_fld_chk_cbs 
 */
gboolean uat_fld_chk_str(void*, const char*, unsigned, void*,void*, char** err);
gboolean uat_fld_chk_proto(void*, const char*, unsigned, void*,void*, char** err);
gboolean uat_fld_chk_num_dec(void*, const char*, unsigned, void*, void*, char** err);
gboolean uat_fld_chk_num_hex(void*, const char*, unsigned, void*, void*, char** err);

#define CHK_STR_IS_DECL(what) \
gboolean uat_fld_chk_str_ ## what (void*, const char*, unsigned, void*, void*, char**)

/* Some strings entirely made of ... already declared */
CHK_STR_IS_DECL(isprint);
CHK_STR_IS_DECL(isalpha);
CHK_STR_IS_DECL(isalnum);
CHK_STR_IS_DECL(isdigit);
CHK_STR_IS_DECL(isxdigit);

#define CHK_STR_IS_DEF(what) \
gboolean uat_fld_chk_str_ ## what (void* u1 _U_, const char* strptr, unsigned len, void* u2 _U_, void* u3 _U_, char** err) { \
	guint i; for (i=0;i<len;i++) { \
		char c = strptr[i]; \
			if (! what(c)) { \
				*err = ep_strdup_printf("invalid char pos=%d value=%.2x",i,c); return FALSE;  } } \
		*err = NULL; return TRUE; }


/*
 * Macros
 *   to define basic uat_fld_set_cbs, uat_fld_tostr_cbs
 *   for those elements in uat_field_t array
 */

/*
 * CSTRING macros,
 *    a simple c-string contained in (((rec_t*)rec)->(field_name))
 */
#define UAT_CSTRING_CB_DEF(basename,field_name,rec_t) \
static void basename ## _ ## field_name ## _set_cb(void* rec, const char* buf, unsigned len, void* u1 _U_, void* u2 _U_) {\
	if ((((rec_t*)rec)->field_name)) g_free((((rec_t*)rec)->field_name)); \
	(((rec_t*)rec)->field_name) = g_strndup(buf,len); } \
static void basename ## _ ## field_name ## _tostr_cb(void* rec, char** out_ptr, unsigned* out_len, void* u1 _U_, void* u2 _U_) {\
		if (((rec_t*)rec)->field_name ) { \
			*out_ptr = (((rec_t*)rec)->field_name); *out_len = strlen((((rec_t*)rec)->field_name)); \
		} else { \
			*out_ptr = ""; *out_len = 0; } }

#define UAT_FLD_CSTRING(basename,field_name) \
	{#field_name, PT_TXTMOD_STRING,{uat_fld_chk_str,basename ## _ ## field_name ## _set_cb,basename ## _ ## field_name ## _tostr_cb},{NULL,NULL,NULL},NULL,FLDFILL}

#define UAT_FLD_CSTRING_ISPRINT(basename,field_name) \
	{#field_name, PT_TXTMOD_STRING,{uat_fld_chk_str_isprint,basename ## _ ## field_name ## _set_cb,basename ## _ ## field_name ## _tostr_cb},{NULL,NULL,NULL},NULL,FLDFILL}

#define UAT_FLD_CSTRING_OTHER(basename,field_name,chk) \
	{#field_name, PT_TXTMOD_STRING,{ chk ,basename ## _ ## field_name ## _set_cb,basename ## _ ## field_name ## _tostr_cb},{NULL,NULL,NULL},NULL,FLDFILL}


/*
 * BUFFER macros,
 *    a buffer_ptr contained in (((rec_t*)rec)->(field_name))
 *    and its len in (((rec_t*)rec)->(len_name))
 *  XXX: UNTESTED
 */
#define UAT_BUFFER_CB_DEF(field_name,len_name,rec_t,ptr_element,len_element) \
static void basename ## field_name ## _set_cb(void* rec, const char* buf, unsigned len, void* u1 _U_, void* u2 _U_) {\
		if ((((rec_t*)rec)->(field_name))) g_free((((rec_t*)rec)->(field_name))); \
			(((rec_t*)rec)->(field_name)) = g_memdup(buf,len); \
			(((rec_t*)rec)->(len_name)) = len; \ } \
static void basename ## field_name ## _tostr_cb(void* rec, char** out_ptr, unsigned* out_len, void* u1 _U_, void* u2 _U_) {\
	*out_ptr = ep_memdup(((rec_t*)rec)->(field_name),((rec_t*)rec)->(len_name)); \
	*len_ptr = (((rec_t*)rec)->(len_name)); }

#define UAT_FLD_BUFFER(basename,field_name) \
	{#field_name, PT_TXTMOD_HEXBYTES,{NULL,basename ## field_name ## _set_cb,basename ## field_name ## _tostr_cb},{NULL,NULL,NULL},NULL,FLDFILL}


/*
 * DEC Macros,
 *   a decimal number contained in 
 */
#define UAT_DEC_CB_DEF(basename,field_name,rec_t) \
static void basename ## field_name ## _set_cb(void* rec, const char* buf, unsigned len, void* u1 _U_, void* u2 _U_) {\
	((rec_t*)rec)->(field_name) = strtol(buf,end,10); } \
static void basename ## field_name ## _tostr_cb(void* rec, char** out_ptr, unsigned* out_len, void* u1 _U_, void* u2 _U_) {\
	*out_ptr = ep_strdup_printf("%d",((rec_t*)rec)->(field_name)); \
	*out_len = strlen(*out_ptr); }

#define UAT_FLD_DEC(basename,field_name) \
	{#field_name, PT_TXTMOD_STRING,{uat_fld_chk_num_dec,basename ## field_name ## _set_cb,basename ## field_name ## _tostr_cb},{NULL,NULL,NULL},NULL,FLDFILL}


/*
 * HEX Macros,
 *   an hexadecimal number contained in 
 */
#define UAT_HEX_CB_DEF(basename,field_name,rec_t) \
static void basename ## field_name ## _set_cb(void* rec, const char* buf, unsigned len, void* u1 _U_, void* u2 _U_) {\
	((rec_t*)rec)->(field_name) = strtol(buf,end,16); } \
static void basename ## field_name ## _tostr_cb(void* rec, char** out_ptr, unsigned* out_len, void* u1 _U_, void* u2 _U_) {\
	*out_ptr = ep_strdup_printf("%x",((rec_t*)rec)->(field_name)); \
	*out_len = strlen(*out_ptr); }

#define UAT_FLD_HEX(basename,field_name) \
{#field_name, PT_TXTMOD_STRING,{uat_fld_chk_num_hex,basename ## field_name ## _set_cb,basename ## field_name ## _tostr_cb},{NULL,NULL,NULL},NULL,FLDFILL}


/*
 * ENUM macros
 *  enum_t: name = ((enum_t*)ptr)->strptr 
 *          value = ((enum_t*)ptr)->value 
 *  rec_t:
 *        value
 */
#define UAT_SET_ENUM_DEF(basename,field_name,rec_t,enum_t,default) \
void static void basename ## field_name ## _set_cb(void* rec, const char* buf, unsigned len, void* enum, void* u2 _U_) {\
	char* str = ep_strndup(buf,len); \
	for(;((enum_t*)enum)->strptr;((enum_t*)enum)++) { \
		if (g_strequal(((enum_t*)enum)->strptr,str)) { \
			((rec_t*)rec)->(field_name) = ((enum_t*)enum)->value; return; } } \
	(rec_t*)rec)->(field_name) = default; \
}

#define UAT_TOSTR_ENUM_DEF(basename,field_name,rec_t,enum_t) {\
static void basename ## field_name ## _tostr_cb(void* rec, char** out_ptr, unsigned* out_len, void* enum, void* u2 _U_) {\
	for(;((enum_t*)enum)->strptr;((enum_t*)enum)++) { \
		if ( ((enum_t*)enum)->value == ((rec_t*)rec)->(field_name) ) { \
			*out_str = ((enum_t*)enum)->strptr; \
			*out_len = strlen(*out_ptr); } } }


#define UAT_FLD_ENUM(basename,field_name,enum_t,enum) \
	{#field_name, PT_TXTMOD_STRING,{uat_fld_chk_enum,basename ## field_name ## _set_cb,basename ## field_name ## _tostr_cb},{&(enum),&(enum),&(enum)},NULL,FLDFILL}







#endif

