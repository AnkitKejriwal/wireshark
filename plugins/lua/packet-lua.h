
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
#include <epan/packet.h>
#include <epan/strutil.h>
#include <epan/prefs.h>
#include <epan/proto.h>
#include <epan/epan_dissect.h>
#include <epan/tap.h>
#include <epan/filesystem.h>
#include <epan/report_err.h>
#include <epan/emem.h>

typedef struct _eth_field_t {
    int hfid;
    char* name;
    char* abbr;
    char* blob;
    enum ftenum type;
    base_display_e base;
    value_string* vs;
    guint32 mask;
} eth_field_t;

typedef enum {PREF_BOOL,PREF_UINT,PREF_STRING} pref_type_t;

typedef struct _eth_pref_t {
    gchar* name;
    pref_type_t type;
    union {
        gboolean b;
        guint32 u;
        gint32 i;
        const gchar* s;
    } value;
    struct _eth_pref_t* next;
} eth_pref_t;

typedef struct _eth_proto_t {
    int hfid;
    char* name;
    char* filter;
    char* desc;
    hf_register_info* hfarray;
    gboolean hf_registered;
    module_t *prefs_module;
    eth_pref_t* prefs;
    dissector_handle_t handle;
    gboolean is_postdissector;
} eth_proto_t;

typedef struct {const gchar* str; enum ftenum id; } eth_ft_types_t;


#define VALUE_STRING "ValueString"
typedef GArray* ValueString;

#define PROTO_FIELD "ProtoField"
typedef struct _eth_field_t* ProtoField;

#define PROTO_FIELD_ARRAY "ProtoFieldArr"
typedef GArray* ProtoFieldArray;

#define ETT "SubTreeType"
typedef int* Ett;

#define ETT_ARRAY "SubTreeTypeArr"
typedef GArray* EttArray;

#define PROTO "Proto"
typedef struct _eth_proto_t* Proto;

#define DISSECTOR_TABLE "DissectorTable"
typedef struct _eth_distbl_t {
    dissector_table_t table;
    gchar* name;
}* DissectorTable;

#define DISSECTOR "Dissector"
typedef dissector_handle_t Dissector;

#define BYTE_ARRAY "ByteArray"
typedef GByteArray* ByteArray;

#define TVB "Tvb"
typedef tvbuff_t* Tvb;

#define COLUMN "Column"
typedef struct _eth_col_info {
    column_info* cinfo;
    gint col;
}* Column;

#define COLUMNS "Columns"
typedef column_info* Columns;

#define PINFO "Pinfo"
typedef packet_info* Pinfo;

#define TREE "Tree"
typedef proto_tree* Tree;

#define ITEM "Item"
typedef proto_item* Item;

#define ADDRESS "Address"
typedef address* Address;

#define INTERESTING "Interesting"
typedef header_field_info* Interesting;

#define TAP "Tap"
typedef struct _eth_tap {
    const gchar* name;
    GPtrArray* interesting_fields;
    gchar* filter;
    gboolean registered;
}* Tap;

#define NOP

#define LUA_CLASS_DEFINE(C,CN,check_code) \
C to##C(lua_State* L, int index) { \
    C* v = (C*)lua_touserdata (L, index); \
    if (!v) luaL_typerror(L,index,CN); \
    return *v; \
} \
C check##C(lua_State* L, int index) { \
    C* p; \
    luaL_checktype(L,index,LUA_TUSERDATA); \
    p = (C*)luaL_checkudata(L, index, CN); \
    check_code; \
    return *p; \
} \
C* push##C(lua_State* L, C v) { \
    C* p = lua_newuserdata(L,sizeof(C)); *p = v; \
    luaL_getmetatable(L, CN); lua_setmetatable(L, -2); \
    return p; \
}

extern packet_info* lua_pinfo;
extern proto_tree* lua_tree;
extern dissector_handle_t lua_data_handle;

#define LUA_CLASS_DECLARE(C,CN) \
extern C to##C(lua_State* L, int index); \
extern C check##C(lua_State* L, int index); \
extern C* push##C(lua_State* L, C v); \
extern int C##_register(lua_State* L);

LUA_CLASS_DECLARE(Tap,TAP);
LUA_CLASS_DECLARE(Interesting,INTERESTING);
LUA_CLASS_DECLARE(ValueString,VALUE_STRING);
LUA_CLASS_DECLARE(ProtoField,PROTO_FIELD);
LUA_CLASS_DECLARE(ProtoFieldArray,PROTO_FIELD_ARRAY);
LUA_CLASS_DECLARE(Ett,ETT);
LUA_CLASS_DECLARE(EttArray,ETT_ARRAY);
LUA_CLASS_DECLARE(Proto,PROTO);
LUA_CLASS_DECLARE(ByteArray,BYTE_ARRAY);
LUA_CLASS_DECLARE(Tvb,TVB);
LUA_CLASS_DECLARE(Column,COLUMN);
LUA_CLASS_DECLARE(Columns,COLUMNS);
LUA_CLASS_DECLARE(Pinfo,PINFO);
LUA_CLASS_DECLARE(Tree,TREE);
LUA_CLASS_DECLARE(Item,ITEM);
LUA_CLASS_DECLARE(Dissector,DISSECTOR);
LUA_CLASS_DECLARE(DissectorTable,DISSECTOR_TABLE);

extern void dissect_lua(tvbuff_t* tvb, packet_info* pinfo, proto_tree* tree);
extern int lua_tap_packet(void *tapdata, packet_info *pinfo, epan_dissect_t *edt, const void *data _U_);
extern void lua_tap_reset(void *tapdata);
extern void lua_tap_draw(void *tapdata);
    
#endif
