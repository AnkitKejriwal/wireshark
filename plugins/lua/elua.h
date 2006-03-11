/*
 * packet-lua.h
 *
 * Ethereal's interface to the Lua Programming Language
 *
 * (c) 2006, Luis E. Garcia Ontanon <luis.ontanon@gmail.com>
 *
 * $Id$
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

#ifndef _PACKET_LUA_H
#define _PACKET_LUA_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include <errno.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <wiretap/wtap.h>
#include <epan/packet.h>
#include <epan/strutil.h>
#include <epan/prefs.h>
#include <epan/proto.h>
#include <epan/epan_dissect.h>
#include <epan/tap.h>
#include <epan/filesystem.h>
#include <epan/report_err.h>
#include <epan/emem.h>
#include <epan/funnel.h>

#include "elua_register.h"

#define ELUA_INIT_ROUTINES "init_routines"
#define LOG_DOMAIN_LUA "eLua"

struct _eth_tvbrange {
    tvbuff_t* tvb;
    int offset;
    int len;
};

typedef struct _eth_field_t {
    int hfid;
    int ett;
    char* name;
    char* abbr;
    char* blob;
    enum ftenum type;
    base_display_e base;
    value_string* vs;
    guint32 mask;
} eth_field_t;

typedef enum {PREF_NONE,PREF_BOOL,PREF_UINT,PREF_STRING} pref_type_t;

typedef struct _eth_pref_t {
    gchar* name;
    gchar* label;
    gchar* desc;
    pref_type_t type;
    union {
        gboolean b;
        guint u;
        const gchar* s;
		void* p;
    } value;
    
    struct _eth_pref_t* next;
    struct _eth_proto_t* proto;
} eth_pref_t;

typedef struct _eth_proto_t {
	gchar* name;
	gchar* desc;
	int hfid;
	int ett;
    eth_pref_t prefs;
	int fields;
    module_t *prefs_module;
    dissector_handle_t handle;
	gboolean is_postdissector;
} eth_proto_t;

struct _eth_distbl_t {
    dissector_table_t table;
    gchar* name;
};

struct _eth_col_info {
    column_info* cinfo;
    gint col;
};

struct _eth_treeitem {
	proto_item* item;
	proto_tree* tree;
};

typedef struct {const gchar* str; enum ftenum id; } eth_ft_types_t;

typedef eth_pref_t* Pref;
typedef eth_pref_t* Prefs;
typedef struct _eth_field_t* ProtoField;
typedef struct _eth_proto_t* Proto;
typedef struct _eth_distbl_t* DissectorTable;
typedef dissector_handle_t Dissector;
typedef GByteArray* ByteArray;
typedef tvbuff_t* Tvb;
typedef struct _eth_tvbrange* TvbRange; 
typedef struct _eth_col_info* Column;
typedef column_info* Columns;
typedef packet_info* Pinfo;
typedef struct _eth_treeitem* TreeItem;
typedef address* Address;
typedef header_field_info** Field;
typedef struct _eth_tap* Tap;
typedef funnel_text_window_t* TextWindow;
typedef wtap_dumper* Dumper;
typedef struct lua_pseudo_header* PseudoHeader;

/*
 * toXxx(L,idx) gets a Xxx from an index (Lua Error if fails)
 * checkXxx(L,idx) gets a Xxx from an index after calling check_code (No Lua Error if it fails)
 * pushXxx(L,xxx) pushes an Xxx into the stack
 * isXxx(L,idx) tests whether we have an Xxx at idx
 *
 * LUA_CLASS_DEFINE must be used without trailing ';'
 */
#define ELUA_CLASS_DEFINE(C,check_code) \
C to##C(lua_State* L, int index) { \
    C* v = (C*)lua_touserdata (L, index); \
    if (!v) luaL_typerror(L,index,#C); \
    return *v; \
} \
C check##C(lua_State* L, int index) { \
    C* p; \
    luaL_checktype(L,index,LUA_TUSERDATA); \
    p = (C*)luaL_checkudata(L, index, #C); \
    check_code; \
    return p ? *p : NULL; \
} \
C* push##C(lua_State* L, C v) { \
    C* p = lua_newuserdata(L,sizeof(C)); *p = v; \
    luaL_getmetatable(L, #C); lua_setmetatable(L, -2); \
    return p; \
}\
gboolean is##C(lua_State* L,int i) { \
	void *p; \
	if(!lua_isuserdata(L,i)) return FALSE; \
	p = lua_touserdata(L, i); \
	lua_getfield(L, LUA_REGISTRYINDEX, #C); \
	if (p == NULL || !lua_getmetatable(L, i) || !lua_rawequal(L, -1, -2)) p=NULL; \
	lua_pop(L, 2); \
	return p ? TRUE : FALSE; \
} \
C shift##C(lua_State* L,int i) { \
    C* p; \
    if ((p = (C*)luaL_checkudata(L, i, #C))) {\
        lua_remove(L,i); \
        return *p; \
    } else { \
        return NULL; \
    } \
}


#ifdef HAVE_LUA_5_1

#define ELUA_REGISTER_CLASS(C) { \
	luaL_register (L, #C, C ## _methods); \
	luaL_newmetatable (L, #C); \
	luaL_register (L, NULL, C ## _meta); \
	lua_pushliteral(L, "__index"); \
	lua_pushvalue(L, -3); \
	lua_rawset(L, -3); \
	lua_pushliteral(L, "__metatable"); \
	lua_pushvalue(L, -3); \
	lua_rawset(L, -3); \
	lua_pop(L, 1); \
}

#define ELUA_REGISTER_META(C) luaL_newmetatable (L, #C);   luaL_register (L, NULL, C ## _meta); 

#define ELUA_INIT(L) \
	L = luaL_newstate(); \
	luaL_openlibs(L); \
	ELUA_REGISTER_CLASSES(); \
	ELUA_REGISTER_FUNCTIONS();


#else /* Lua 5.0 */

#define ELUA_REGISTER_CLASS(C) { \
	luaL_openlib(L, #C, C ## _methods, 0); \
	luaL_newmetatable(L, #C); \
	luaL_openlib(L, 0, C ## _meta, 0); \
	lua_pushliteral(L, "__index"); \
	lua_pushvalue(L, -3); \
	lua_rawset(L, -3); \
	lua_pushliteral(L, "__metatable"); \
	lua_pushvalue(L, -3); \
	lua_rawset(L, -3); \
	lua_pop(L, 1); \
}

#define ELUA_REGISTER_META(C) luaL_newmetatable (L, #C); luaL_openlib (L, NULL, C ## _meta, 0);

#define ELUA_INIT(L) \
	L = lua_open(); \
	luaopen_base(L); \
	luaopen_table(L); \
	luaopen_io(L); \
	luaopen_string(L); \
	ELUA_REGISTER_CLASSES(); \
	ELUA_REGISTER_FUNCTIONS();

#endif

#define ELUA_FUNCTION extern int 
#define ELUA_REGISTER_FUNCTION(name)     { lua_pushstring(L, #name); lua_pushcfunction(L, elua_## name); lua_settable(L, LUA_GLOBALSINDEX); }
#define ELUA_REGISTER extern int

#define ELUA_METHOD static int 
#define ELUA_CONSTRUCTOR static int 
#define ELUA_ATTR_SET static int 
#define ELUA_ATTR_GET static int 
#define ELUA_METAMETHOD static int

#define ELUA_METHODS static const luaL_reg 
#define ELUA_META static const luaL_reg
#define ELUA_CLASS_FNREG(class,name) { #name, class##_##name }

#define ELUA_ERROR(name,error) { luaL_error(L, #name  ": " error); return 0; }
#define ELUA_ARG_ERROR(name,attr,error) { luaL_argerror(L,ELUA_ARG_ ## name ## _ ## attr, #name  ": " error); return 0; }
#define ELUA_OPTARG_ERROR(name,attr,error) { luaL_argerror(L,ELUA_OPTARG_##name##_ ##attr, #name  ": " error); return 0; }

#define ELUA_REG_GLOBAL_BOOL(L,n,v) { lua_pushstring(L,n); lua_pushboolean(L,v); lua_settable(L, LUA_GLOBALSINDEX); }
#define ELUA_REG_GLOBAL_STRING(n,v) { lua_pushstring(L,n); lua_pushstring(L,v); lua_settable(L, LUA_GLOBALSINDEX); }
#define ELUA_REG_GLOBAL_NUMBER(n,v) { lua_pushstring(L,n); lua_pushnumber(L,v); lua_settable(L, LUA_GLOBALSINDEX); }

#define ELUA_RETURN(i) return (i);

#define ELUA_API extern

#define NOP
#define FAIL_ON_NULL(s) if (! *p) luaL_argerror(L,index,s)



#define ELUA_CLASS_DECLARE(C) \
extern C to##C(lua_State* L, int index); \
extern C check##C(lua_State* L, int index); \
extern C* push##C(lua_State* L, C v); \
extern int C##_register(lua_State* L); \
extern gboolean is##C(lua_State* L,int i); \
extern C shift##C(lua_State* L,int i)


extern packet_info* lua_pinfo;
extern TreeItem lua_tree;
extern tvbuff_t* lua_tvb;
extern int lua_malformed;
extern dissector_handle_t lua_data_handle;
extern gboolean lua_initialized;
extern int lua_dissectors_table_ref;

ELUA_DECLARE_CLASSES()
ELUA_DECLARE_FUNCTIONS()

extern const gchar* lua_shiftstring(lua_State* L,int idx);
extern void dissect_lua(tvbuff_t* tvb, packet_info* pinfo, proto_tree* tree);

extern void proto_register_lua(void);
extern GString* lua_register_all_taps(void);
extern void lua_prime_all_fields(proto_tree* tree);

extern int Proto_commit(lua_State* L);

extern void* push_Tvb(lua_State* L, Tvb tvb);
extern void clear_outstanding_tvbs(void);

extern void* push_Pinfo(lua_State* L, Pinfo p);
extern void clear_outstanding_pinfos(void);

extern void* push_TreeItem(lua_State* L, TreeItem ti);
extern void clear_outstanding_trees(void);

extern void elua_print_stack(char* s, lua_State* L);

#endif
