/*
 * lua_tap.c
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

#include "elua.h"

ELUA_CLASS_DEFINE(Field,NOP)
/*
 A Field extractor to to obtain field values.
 */

static GPtrArray* wanted_fields = NULL;

/*
 * field extractor registartion is tricky, In order to allow
 * the user to define them in the body of the script we will
 * populate the Field value with a pointer of the abbrev of it
 * to later replace it with the hfi.
 *
 * This will be added to the wanted_fields array that will
 * exists only while they can be defined, and be cleared right
 * after the fields are primed.
 */

void lua_prime_all_fields(proto_tree* tree _U_) {
    GString* fake_tap_filter = g_string_new("frame");
    guint i;
    static gboolean fake_tap = FALSE;
    
    for(i=0; i < wanted_fields->len; i++) {
        Field f = g_ptr_array_index(wanted_fields,i);
        gchar* name = *((gchar**)f);

        *f = proto_registrar_get_byname(name);

        if (!*f) {
            report_failure("Could not find field `%s'",name);
            *f = NULL;
            g_free(name);
            continue;
        }

        g_free(name);
        
        g_string_sprintfa(fake_tap_filter," || %s",(*f)->abbrev);
        fake_tap = TRUE;
    }
    
    g_ptr_array_free(wanted_fields,TRUE);
    wanted_fields = NULL;
    
    if (fake_tap) {
        /* a boring tap :-) */
        GString* error = register_tap_listener("frame",
                                      &fake_tap,
                                      fake_tap_filter->str,
                                      NULL, NULL, NULL);
        
        if (error) {
            report_failure("while registering lua_fake_tap:\n%s",error->str);
            g_string_free(error,TRUE);
        }
    }
    
}

ELUA_CONSTRUCTOR Field_new(lua_State *L) {
	/*
	 Create a Field extractor
	 */
#define ELUA_ARG_Field_new_FIELDNAME 1 /* The filter name of the field (e.g. ip.addr) */
    const gchar* name = luaL_checkstring(L,ELUA_ARG_Field_new_FIELDNAME);
    Field f;
    
    if (!name) return 0;
    
	if (!proto_registrar_get_byname(name)) 
		ELUA_ARG_ERROR(Field_new,FIELDNAME,"a field with this name must exist");
	
    if (!wanted_fields)
		ELUA_ERROR(Field_get,"a Field extractor must be defined before Taps or Dissectors get called");
    
    f = g_malloc(sizeof(void*));
    *f = (header_field_info*)g_strdup(name); /* cheating */
    
    g_ptr_array_add(wanted_fields,f);
    
    pushField(L,f);
    ELUA_RETURN(1); /* The field extractor */
}

ELUA_METAMETHOD Field__call (lua_State* L) {
    Field f = checkField(L,1);
    int items_found = 0;
    header_field_info* in = *f;

    if (! in) {
        luaL_error(L,"invalid field");
        return 0;            
    }

    if (! lua_pinfo ) {
        ELUA_ERROR(Field__call,"fields cannot be used outside dissectors or taps");
        return 0;
    }
    
    for (;in;in = in->same_name_next) {
        GPtrArray* found = proto_find_finfo(lua_tree->tree, in->id);
        guint i;
        if (found) {
            for (i=0; i<found->len; i++) {
                field_info* fi = g_ptr_array_index(found,i);
                switch(fi->hfinfo->type) {
                    case FT_UINT8:
                    case FT_UINT16:
                    case FT_UINT24:
                    case FT_UINT32:
                    case FT_FRAMENUM:
                    case FT_INT8:
                    case FT_INT16:
                    case FT_INT24:
                    case FT_INT32:
                        lua_pushnumber(L,(lua_Number)fvalue_get_integer(&(fi->value)));
                        items_found++;
                        break;
                    case FT_FLOAT:
                    case FT_DOUBLE:
                        lua_pushnumber(L,(lua_Number)fvalue_get_floating(&(fi->value)));
                        items_found++;
                        break;
                    case FT_UINT64:
                    case FT_INT64:
                        /* XXX: get them as strings for now */
                    case FT_ETHER:
                    case FT_IPv4:
                    case FT_IPv6:
                    case FT_IPXNET:
                        /* XXX: use Address ??? */
                    case FT_STRING:
                    case FT_STRINGZ:
                    case FT_BYTES:
                    case FT_UINT_BYTES:
                    case FT_GUID:
                    case FT_OID:
                        lua_pushstring(L,fvalue_to_string_repr(&fi->value,FTREPR_DISPLAY,NULL));
                        items_found++;
                        break;
                    default:
                        luaL_error(L,"FT_ not yet supported");
                        return items_found;
                }
            }
        }
    }
    ELUA_RETURN(items_found); /* All the values of this field */
}

static int Field_tostring(lua_State* L) {
    Field f = checkField(L,1);

    if ( !(f && *f) ) {
        luaL_error(L,"invalid Field");
        return 0;
    }
    
    if (wanted_fields) {
        lua_pushstring(L,*((gchar**)f));
    } else {
        lua_pushstring(L,(*f)->abbrev);
    }
    
    return 1;
}

static const luaL_reg Field_methods[] = {
    {"new", Field_new},
    {0, 0}
};

static const luaL_reg Field_meta[] = {
    {"__tostring", Field_tostring},
    {"__call", Field__call},
    {0, 0}
};

int Field_register(lua_State* L) {

    wanted_fields = g_ptr_array_new();

    ELUA_REGISTER_CLASS(Field);

    return 1;
}



ELUA_CLASS_DEFINE(Tap,NOP)
/*
    A Tap Listener, is once for called every packet that matches a certain filter.
    It can read the tree and the packet's buffer but it cannot add elements to the tree. 
 */
struct _eth_tap {
    gchar* name;
    gchar* filter;
    lua_State* L;
    int packet_ref;
    int draw_ref;
    int init_ref;
    int data_ref;
};


int tap_packet_cb_error_handler(lua_State* L) {
    const gchar* error =  lua_tostring(L,1);
    static gchar* last_error = NULL;
    static int repeated = 0;
    static int next = 2;
	const gchar* where =  (lua_pinfo) ?
		ep_strdup_printf("Lua: on packet %i Error During execution of Tap Packet Callback",lua_pinfo->fd->num) :
		ep_strdup_printf("Lua: Error During execution of Tap Packet Callback") ;
	
    /* show the error the 1st, 3rd, 5th, 9th, 17th, 33th... time it appears to avoid window flooding */ 
    /* XXX the last series of identical errors won't be shown (the user however gets at least one message) */
    
    if (! last_error) {
        report_failure("%s:\n%s",where,error);
        last_error = g_strdup(error);
        repeated = 0;
        next = 2;
        return 0;
    }
    
    if (g_str_equal(last_error,error) ) {
        repeated++;
        if ( repeated == next ) {
            report_failure("%s happened %i times:\n %s",where,repeated,error);
            next *= 2;
        }
    } else {
        report_failure("%s happened %i times:\n %s",where,repeated,last_error);
        g_free(last_error);
        last_error = g_strdup(error);
        repeated = 0;
        next = 2;
        report_failure("%s:\n %s",where,error);
    }
    
    return 0;    
}


int lua_tap_packet(void *tapdata, packet_info *pinfo, epan_dissect_t *edt, const void *data _U_) {
    Tap tap = tapdata;
    int retval = 0;

    if (tap->packet_ref == LUA_NOREF) return 0;

    lua_settop(tap->L,0);
    
    lua_pushcfunction(tap->L,tap_packet_cb_error_handler);
    lua_rawgeti(tap->L, LUA_REGISTRYINDEX, tap->packet_ref);
    
    push_Pinfo(tap->L, pinfo);
    push_Tvb(tap->L, edt->tvb);
    
    lua_rawgeti(tap->L, LUA_REGISTRYINDEX, tap->data_ref);
    
    lua_pinfo = pinfo; 
    lua_tvb = edt->tvb;
    lua_tree = ep_alloc(sizeof(struct _eth_treeitem));
	lua_tree->tree = edt->tree;
	lua_tree->item = NULL;
    
    switch ( lua_pcall(tap->L,3,1,1) ) {
        case 0:
            
            if (lua_gettop(tap->L) == 1)
                retval = luaL_checkint(tap->L,1);
            else 
                retval = 1;
            
            break;
        case LUA_ERRRUN:
            break;
        case LUA_ERRMEM:
            g_warning("Memory alloc error while calling %s.packet() ",tap->name);
            break;
        default:
            g_assert_not_reached();
            break;
    }
    
    clear_outstanding_pinfos();
    clear_outstanding_tvbs();
    
    lua_pinfo = NULL; 
    lua_tvb = NULL;
    lua_tree = NULL;
    
    return retval;
}

int tap_reset_cb_error_handler(lua_State* L) {
    const gchar* error =  lua_tostring(L,1);
    report_failure("Lua: Error During execution of Tap init Callback:\n %s",error);
    return 1;
}

void lua_tap_reset(void *tapdata) {
    Tap tap = tapdata;
    
    if (tap->init_ref == LUA_NOREF) return;
    
    lua_pushcfunction(tap->L,tap_reset_cb_error_handler);
    lua_rawgeti(tap->L, LUA_REGISTRYINDEX, tap->init_ref);
    lua_rawgeti(tap->L, LUA_REGISTRYINDEX, tap->data_ref);
    
    switch ( lua_pcall(tap->L,1,0,1) ) {
        case 0:
            break;
        case LUA_ERRRUN:
            g_warning("Runtime error while calling %s.init() ",tap->name);
            break;
        case LUA_ERRMEM:
            g_warning("Memory alloc error while calling %s.init() ",tap->name);
            break;
        default:
            g_assert_not_reached();
            break;
    }
}

int tap_draw_cb_error_handler(lua_State* L) {
    const gchar* error =  lua_tostring(L,1);
    report_failure("Lua: Error During execution of Tap Draw Callback:\n %s",error);
    return 1;
}

void lua_tap_draw(void *tapdata) {
    Tap tap = tapdata;
    const gchar* error;
    if (tap->draw_ref == LUA_NOREF) return;
    
    lua_pushcfunction(tap->L,tap_reset_cb_error_handler);
    lua_rawgeti(tap->L, LUA_REGISTRYINDEX, tap->draw_ref);
    lua_rawgeti(tap->L, LUA_REGISTRYINDEX, tap->data_ref);
    
    switch ( lua_pcall(tap->L,1,0,1) ) {
        case 0:
            /* OK */
            break;
        case LUA_ERRRUN:
            error = lua_tostring(tap->L,-1);
            g_warning("Runtime error while calling %s.draw(): %s",tap->name,error);
            break;
        case LUA_ERRMEM:
            g_warning("Memory alloc error while calling %s.draw() ",tap->name);
            break;
        default:
            g_assert_not_reached();
            break;
    }
}

ELUA_CONSTRUCTOR Tap_new(lua_State* L) {
	/* Creates a new Tap listener */
#define ELUA_ARG_Tap_new_NAME 1 /* the name of the tap */
#define ELUA_ARG_Tap_new_FILTER 2 /* a filter that when matches the tap.packet function gets called (use nil to be called for every packet) */
#define ELUA_OPTARG_Tap_new_USERDATA 3 /* a datum that will be passed as last argument to all tap functions when they get called */

    const gchar* name = luaL_checkstring(L,ELUA_ARG_Tap_new_NAME);
    const gchar* filter = luaL_optstring(L,ELUA_ARG_Tap_new_FILTER,NULL);
    Tap tap;
    GString* error;

    if (!name) ELUA_ARG_ERROR(Tap_new,NAME,"must be a string");
    
    tap = g_malloc(sizeof(struct _eth_tap));
    
    tap->name = g_strdup(name);
    tap->filter = filter ? g_strdup(filter) : NULL;
    tap->L = L;
    tap->packet_ref = LUA_NOREF;
    tap->draw_ref = LUA_NOREF;
    tap->init_ref = LUA_NOREF;

    if (lua_gettop(L) > 2) {
        lua_pushvalue(L, ELUA_OPTARG_Tap_new_USERDATA);
        tap->data_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    } else {
        tap->data_ref = LUA_NOREF;
    }
    
    error = register_tap_listener("frame", tap, tap->filter, lua_tap_reset, lua_tap_packet, lua_tap_draw);

    if (error) {
		/* ELUA_ERROR(new_tap,"tap registration error"); */
        luaL_error(L,"Error while registering tap:\n%s",error->str);
        g_string_free(error,TRUE);
        if (tap->filter) g_free(tap->filter);
        g_free(tap->name);
        luaL_unref(L, LUA_REGISTRYINDEX, tap->data_ref);
        g_free(tap);
    }
    
    pushTap(L,tap);
    ELUA_RETURN(1); /* The newly created Tap listener object */
}

ELUA_METHOD Tap_remove(lua_State* L) {
	/* Removes a tap listener */
    Tap tap = checkTap(L,1);
    
    if (!tap) return 0;
    
    remove_tap_listener(tap);
    
    return 0;
}

static int Tap_tostring(lua_State* L) {
    Tap tap = checkTap(L,1);
    gchar* str;
    
    if (!tap) return 0;
    
    str = g_strdup_printf("Tap->%s filter: %s",tap->name, tap->filter ? tap->filter : "NONE");
    lua_pushstring(L,str);
    g_free(str);
    
    return 1;
}


static int Tap_newindex(lua_State* L) { 
	/* ELUA_ATTRIBUTE Tap_packet WO A function that will be called once every packet matches the Tap listener filter.
	
		function tap.packet(pinfo,tvb,userdata) ... end
	*/
	/* ELUA_ATTRIBUTE Tap_draw WO A function that will be called once every few seconds to redraw the gui objects
				in tethereal this funtion is called oly at the very end of the capture file.
	
		function tap.draw(userdata) ... end
	*/
	/* ELUA_ATTRIBUTE Tap_reset WO A function that will be called at the end of the capture run.
	
		function tap.reset(userdata) ... end
	*/
    Tap tap = shiftTap(L,1);
    const gchar* index = lua_shiftstring(L,1);
    int* refp = NULL;
    
    if (!index) return 0;
    
    if (g_str_equal(index,"packet")) {
        refp = &(tap->packet_ref);
    } else if (g_str_equal(index,"draw")) {
        refp = &(tap->draw_ref);
    } else if (g_str_equal(index,"reset")) {
        refp = &(tap->init_ref);
    } else {
        luaL_error(L,"No such attribute `%s' for a tap",index);
        return 0;
    }
    
    if (! lua_isfunction(L,1)) {
        luaL_error(L,"Tap's attribute `%s' must be a function");
        return 0;
    }
    
    lua_pushvalue(L, 1);
    *refp = luaL_ref(L, LUA_REGISTRYINDEX);

    return 0;
}


static const luaL_reg Tap_methods[] = {
    {"new", Tap_new},
    {"remove", Tap_remove},
    {0, 0}
};

static const luaL_reg Tap_meta[] = {
    {"__tostring", Tap_tostring},
    {"__newindex", Tap_newindex},
    {0, 0}
};

int Tap_register(lua_State* L) {
    ELUA_REGISTER_CLASS(Tap);
	return 1;
}

