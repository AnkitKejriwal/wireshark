/*
 * packet-lua.c
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

#include "packet-lua.h"
#include <epan/nstime.h>
#include <math.h>

static lua_State* L = NULL;

packet_info* lua_pinfo;
proto_tree* lua_tree;
tvbuff_t* lua_tvb;
int lua_malformed;
dissector_handle_t lua_data_handle;

static int lua_format_date(lua_State* LS) {
    lua_Number time = luaL_checknumber(LS,1);
    nstime_t then;
    gchar* str;
    
    then.secs = (guint32)floor(time);
    then.nsecs = (guint32) ( (time-(double)(then.secs))*1000000000);
    str = abs_time_to_str(&then);    
    lua_pushstring(LS,str);
    
    return 1;
}

static int lua_format_time(lua_State* LS) {
    lua_Number time = luaL_checknumber(LS,1);
    nstime_t then;
    gchar* str;
    
    then.secs = (guint32)floor(time);
    then.nsecs = (guint32) ( (time-(double)(then.secs))*1000000000);
    str = rel_time_to_str(&then);    
    lua_pushstring(LS,str);
    
    return 1;
}

static int lua_report_failure(lua_State* LS) {
    const gchar* s = luaL_checkstring(LS,1);
    report_failure("%s",s);
    return 0;
}

/* ethereal uses lua */

int lua_tap_packet(void *tapdata, packet_info *pinfo, epan_dissect_t *edt, const void *data _U_) {
    Tap tap = tapdata;
    
    lua_pushstring(L, "_ethereal_pinfo");
    pushPinfo(L, pinfo);
    lua_settable(L, LUA_GLOBALSINDEX);
    
    lua_tree = edt->tree;
    
    /* XXX in C */
    lua_dostring(L,ep_strdup_printf("taps.%s(_ethereal_pinfo);",tap->name));
    
    return 1;
}

void lua_tap_reset(void *tapdata) {
    Tap tap = tapdata;
    /* XXX in C */
    lua_dostring(L,ep_strdup_printf("tap_resets.%s();",tap->name));
}

void lua_tap_draw(void *tapdata) {
    Tap tap = tapdata;
    /* XXX in C */
    lua_dostring(L,ep_strdup_printf("tap_draws.%s();",tap->name));
}

void dissect_lua(tvbuff_t* tvb, packet_info* pinfo, proto_tree* tree) {
    lua_pinfo = pinfo;
    lua_tree = tree;
    lua_tvb = tvb;

    /*
     * equivalent to Lua:
     * dissectors[current_proto](tvb,pinfo,tree)
     */
    
    lua_settop(L,0);

    lua_getglobal(L, LUA_DISSECTORS_TABLE);
    
    if (!lua_istable(L, -1)) {
        g_warning("either `" LUA_DISSECTORS_TABLE "' does not exist or it is not a table!");

        lua_pinfo = NULL;
        lua_tree = NULL;
        lua_tvb = NULL;
        
        return;
    }
    
    
    lua_pushstring(L, pinfo->current_proto);

    lua_gettable(L, -2);  

    lua_remove(L,1);

    
    if (!lua_isfunction(L,1)) {
        g_warning("`" LUA_DISSECTORS_TABLE ".%s' is not a function, is a %s",
                  pinfo->current_proto,lua_typename(L,lua_type(L,1)));

        lua_pinfo = NULL;
        lua_tree = NULL;
        lua_tvb = NULL;
        
        return;        
    }
    
    pushTvb(L, tvb);
    pushPinfo(L, pinfo);
    pushProtoTree(L, tree);
    
    switch ( lua_pcall(L,3,0,0) ) {
        case 0:
            /* OK */
            break;
        case LUA_ERRRUN:
            g_warning("Runtime error while calling " LUA_DISSECTORS_TABLE ".%s() ",pinfo->current_proto);
            break;
        case LUA_ERRMEM:
            g_warning("Memory alloc error while calling " LUA_DISSECTORS_TABLE ".%s() ",pinfo->current_proto);
            break;
        default:
            g_assert_not_reached();
            break;
    }

    lua_pinfo = NULL;
    lua_tree = NULL;
    lua_tvb = NULL;
}

static void init_lua(void) {
    GString* tap_error = register_all_lua_taps();
    
    if ( tap_error ) {
        report_failure("lua tap registration problem: %s",tap_error->str);
        g_string_free(tap_error,TRUE);
    }
    
    
    /* this should be called in a more appropriate place */
    lua_prime_all_fields(NULL);
    
    if (L) {
        lua_getglobal(L, LUA_INIT_ROUTINES);

        if (!lua_istable(L, -1)) {
            g_warning("either `" LUA_INIT_ROUTINES "' does not exist or it is not a table!");
            return;
        }
        
        lua_pushnil(L);
        
        while (lua_next(L, -2) != 0) {
            const gchar* name = lua_tostring(L,-2);
            if (!lua_isfunction(L,-1)) {
                g_warning("`" LUA_INIT_ROUTINES ".%s' is not a function, is a %s",
                          name,lua_typename(L,lua_type(L,-1)));
                return;
            }
                        
            switch ( lua_pcall(L,-1,0,0) ) {
                case 0:
                    /* OK */
                    break;
                case LUA_ERRRUN:
                    g_warning("Runtime error while calling " LUA_INIT_ROUTINES ".%s() ",name);
                    break;
                case LUA_ERRMEM:
                    g_warning("Memory alloc error while calling " LUA_INIT_ROUTINES ".%s() ",name);
                    break;
                default:
                    g_assert_not_reached();
                    break;
            }
            
            lua_pop(L, 1);
        }
        
        lua_pop(L, 1);
        
    }
}

void proto_reg_handoff_lua(void) {
    lua_data_handle = find_dissector("data");
    /* XXX in C */
    if (L)
        lua_dostring(L, "for k in handoff_routines do handoff_routines[k]() end ;");
}

static const char *getF(lua_State *L _U_, void *ud, size_t *size)
{
    FILE *f=(FILE *)ud;
    static char buff[512];
    if (feof(f)) return NULL;
    *size=fread(buff,1,sizeof(buff),f);
    return (*size>0) ? buff : NULL;
}

void proto_register_lua(void)
{
    FILE* file;
    gchar* filename = getenv("ETHEREAL_LUA_INIT");
    
    /* TODO: 
        disable if not running with the right credentials
        
        if (euid == 0 && euid != ruid) return;
    */
    
    if (!filename) filename = get_persconffile_path("init.lua", FALSE);

    file = fopen(filename,"r");

    if (! file) return;
    
    register_init_routine(init_lua);    
    
    L = lua_open();
    
    luaopen_base(L);
    luaopen_table(L);
    luaopen_io(L);
    luaopen_string(L);

    ProtoField_register(L);
    ProtoFieldArray_register(L);
    SubTreeType_register(L);
    SubTreeTypeArray_register(L);
    ByteArray_register(L);
    Tvb_register(L);
    Proto_register(L);
    Column_register(L);
    Pinfo_register(L);
    ProtoTree_register(L);
    ProtoItem_register(L);
    Dissector_register(L);
    DissectorTable_register(L);
    Field_register(L);
    Columns_register(L);
    Tap_register(L);
    Address_register(L);
    
    lua_pushstring(L, "format_date");
    lua_pushcfunction(L, lua_format_date);
    lua_settable(L, LUA_GLOBALSINDEX);
    
    lua_pushstring(L, "format_time");
    lua_pushcfunction(L, lua_format_time);
    lua_settable(L, LUA_GLOBALSINDEX);
    
    lua_pushstring(L, "report_failure");
    lua_pushcfunction(L, lua_report_failure);
    lua_settable(L, LUA_GLOBALSINDEX);
            
    lua_pushstring(L, "handoff_routines");
    lua_newtable (L);
    lua_settable(L, LUA_GLOBALSINDEX);
    
    lua_pushstring(L, LUA_INIT_ROUTINES);
    lua_newtable (L);
    lua_settable(L, LUA_GLOBALSINDEX);
    
    lua_pushstring(L, LUA_DISSECTORS_TABLE);
    lua_newtable (L);
    lua_settable(L, LUA_GLOBALSINDEX);
    
    lua_pushstring(L, "taps");
    lua_newtable (L);
    lua_settable(L, LUA_GLOBALSINDEX);

    lua_pushstring(L, "tap_resets");
    lua_newtable (L);
    lua_settable(L, LUA_GLOBALSINDEX);

    lua_pushstring(L, "tap_draws");
    lua_newtable (L);
    lua_settable(L, LUA_GLOBALSINDEX);
    
    if (lua_load(L,getF,file,filename) || lua_pcall(L,0,0,0))
        fprintf(stderr,"%s\n",lua_tostring(L,-1));
    
    fclose(file);
    
    lua_data_handle = NULL;
    lua_pinfo = NULL;
    lua_tree = NULL;
    lua_tvb = NULL;
    
    lua_malformed = proto_get_id_by_filter_name("malformed");
    
}

