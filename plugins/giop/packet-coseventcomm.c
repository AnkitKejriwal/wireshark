/* packet-coseventcomm.c
 * Routines for IDL dissection
 *
 * Autogenerated from idl2eth
 * Copyright 2001 Frank Singleton <frank.singleton@ericsson.com>
 */


/*
 * Ethereal - Network traffic analyzer
 * By Gerald Combs
 * Copyright 1999 Gerald Combs
 */
 

/*
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
 *
 */


#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "plugins/plugin_api.h"

#include <stdio.h>
#include <stdlib.h>
#include <gmodule.h>

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif

#ifdef NEED_SNPRINTF_H
# ifdef HAVE_STDARG_H
#  include <stdarg.h>
# else
#  include <varargs.h>
# endif
# include "snprintf.h"
#endif

#include <string.h>
#include <glib.h>
#include "packet.h"
#include "proto.h"
#include "packet-giop.h"

#include "plugins/plugin_api_defs.h"

#ifndef __ETHEREAL_STATIC__
G_MODULE_EXPORT const gchar version[] = "0.0.1";
#endif



/* Initialise the protocol and subtree pointers */

static int proto_coseventcomm = -1;

static gint ett_coseventcomm = -1;



/* Initialise the initial Alignment */

static guint32  boundary = GIOP_HEADER_SIZE;  /* initial value */




/* Initialise the Registered fields */

/* TODO - Use registered fields */


/*
 * IDL Operations Start
 */
 
 
static const char CosEventComm_PushConsumer_push_op[] = "push" ;
static const char CosEventComm_PushConsumer_disconnect_push_consumer_op[] = "disconnect_push_consumer" ;
static const char CosEventComm_PushSupplier_disconnect_push_supplier_op[] = "disconnect_push_supplier" ;
static const char CosEventComm_PullSupplier_pull_op[] = "pull" ;
static const char CosEventComm_PullSupplier_try_pull_op[] = "try_pull" ;
static const char CosEventComm_PullSupplier_disconnect_pull_supplier_op[] = "disconnect_pull_supplier" ;
static const char CosEventComm_PullConsumer_disconnect_pull_consumer_op[] = "disconnect_pull_consumer" ;

/*
 * IDL Operations End
 */
 

/*  Begin Exception (containing members) String  Declare  */


    
/*  End Exception (containing members) String Declare  */


/*  Begin Exception Helper Functions  */


    
/*  End Exception Helper Functions  */



/*
 * Main delegator for exception handling
 *
 */
 
static gboolean decode_user_exception(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int *offset, MessageHeader *header, gchar *operation ) {
    
    gboolean be;                        /* big endianess */

    



    return FALSE;    /* user exception not found */

}
    


/*
 * IDL:omg.org/CosEventComm/PushConsumer/push:1.0
 */
 

static void decode_CosEventComm_PushConsumer_push(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int *offset, MessageHeader *header, gchar *operation) {

    gboolean stream_is_big_endian;          /* big endianess */

    
    /* Operation specific Variable declarations Begin */
    
    
    /* Operation specific Variable declarations End */
    
    
    stream_is_big_endian = is_big_endian(header);
    
    switch(header->message_type) {
    
    case Request:
    
        get_CDR_any(tvb,tree,offset,stream_is_big_endian, boundary, header);
        
        
        break;
        
    case Reply:
    
        switch(header->rep_status) {
        
        case NO_EXCEPTION:
        
            
            /* Function returns void */
            
            
            break;
            
        case USER_EXCEPTION:
        
            break;
            
        default:
        
            /* Unknown Exception */
        
            g_warning("Unknown Exception ");
        
            
        
            break;
        
        
        }   /* switch(header->message_type) */
        
        break;   
        
    default:
    
        /* Unknown GIOP Message */
    
        g_warning("Unknown GIOP Message");
        
    
        break;
    
        
    } /* switch(header->message_type) */ 
    
}


/*
 * IDL:omg.org/CosEventComm/PushConsumer/disconnect_push_consumer:1.0
 */
 

static void decode_CosEventComm_PushConsumer_disconnect_push_consumer(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int *offset, MessageHeader *header, gchar *operation) {

    gboolean stream_is_big_endian;          /* big endianess */

    
    /* Operation specific Variable declarations Begin */
    
    
    /* Operation specific Variable declarations End */
    
    
    stream_is_big_endian = is_big_endian(header);
    
    switch(header->message_type) {
    
    case Request:
    
        break;
        
    case Reply:
    
        switch(header->rep_status) {
        
        case NO_EXCEPTION:
        
            
            /* Function returns void */
            
            
            break;
            
        case USER_EXCEPTION:
        
            break;
            
        default:
        
            /* Unknown Exception */
        
            g_warning("Unknown Exception ");
        
            
        
            break;
        
        
        }   /* switch(header->message_type) */
        
        break;   
        
    default:
    
        /* Unknown GIOP Message */
    
        g_warning("Unknown GIOP Message");
        
    
        break;
    
        
    } /* switch(header->message_type) */ 
    
}


/*
 * IDL:omg.org/CosEventComm/PushSupplier/disconnect_push_supplier:1.0
 */
 

static void decode_CosEventComm_PushSupplier_disconnect_push_supplier(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int *offset, MessageHeader *header, gchar *operation) {

    gboolean stream_is_big_endian;          /* big endianess */

    
    /* Operation specific Variable declarations Begin */
    
    
    /* Operation specific Variable declarations End */
    
    
    stream_is_big_endian = is_big_endian(header);
    
    switch(header->message_type) {
    
    case Request:
    
        break;
        
    case Reply:
    
        switch(header->rep_status) {
        
        case NO_EXCEPTION:
        
            
            /* Function returns void */
            
            
            break;
            
        case USER_EXCEPTION:
        
            break;
            
        default:
        
            /* Unknown Exception */
        
            g_warning("Unknown Exception ");
        
            
        
            break;
        
        
        }   /* switch(header->message_type) */
        
        break;   
        
    default:
    
        /* Unknown GIOP Message */
    
        g_warning("Unknown GIOP Message");
        
    
        break;
    
        
    } /* switch(header->message_type) */ 
    
}


/*
 * IDL:omg.org/CosEventComm/PullSupplier/pull:1.0
 */
 

static void decode_CosEventComm_PullSupplier_pull(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int *offset, MessageHeader *header, gchar *operation) {

    gboolean stream_is_big_endian;          /* big endianess */

    
    /* Operation specific Variable declarations Begin */
    
    
    /* Operation specific Variable declarations End */
    
    
    stream_is_big_endian = is_big_endian(header);
    
    switch(header->message_type) {
    
    case Request:
    
        break;
        
    case Reply:
    
        switch(header->rep_status) {
        
        case NO_EXCEPTION:
        
            get_CDR_any(tvb,tree,offset,stream_is_big_endian, boundary, header);
            
            
            break;
            
        case USER_EXCEPTION:
        
            break;
            
        default:
        
            /* Unknown Exception */
        
            g_warning("Unknown Exception ");
        
            
        
            break;
        
        
        }   /* switch(header->message_type) */
        
        break;   
        
    default:
    
        /* Unknown GIOP Message */
    
        g_warning("Unknown GIOP Message");
        
    
        break;
    
        
    } /* switch(header->message_type) */ 
    
}


/*
 * IDL:omg.org/CosEventComm/PullSupplier/try_pull:1.0
 */
 

static void decode_CosEventComm_PullSupplier_try_pull(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int *offset, MessageHeader *header, gchar *operation) {

    gboolean stream_is_big_endian;          /* big endianess */

    
    /* Operation specific Variable declarations Begin */
    
    guint8    u_octet1;
    
    /* Operation specific Variable declarations End */
    
    
    stream_is_big_endian = is_big_endian(header);
    
    switch(header->message_type) {
    
    case Request:
    
        break;
        
    case Reply:
    
        switch(header->rep_status) {
        
        case NO_EXCEPTION:
        
            get_CDR_any(tvb,tree,offset,stream_is_big_endian, boundary, header);
            
            
            u_octet1 = get_CDR_boolean(tvb,offset);
            if (tree) {
               proto_tree_add_text(tree,tvb,*offset-1,1,"has_event = %u",u_octet1);
            }
            
            break;
            
        case USER_EXCEPTION:
        
            break;
            
        default:
        
            /* Unknown Exception */
        
            g_warning("Unknown Exception ");
        
            
        
            break;
        
        
        }   /* switch(header->message_type) */
        
        break;   
        
    default:
    
        /* Unknown GIOP Message */
    
        g_warning("Unknown GIOP Message");
        
    
        break;
    
        
    } /* switch(header->message_type) */ 
    
}


/*
 * IDL:omg.org/CosEventComm/PullSupplier/disconnect_pull_supplier:1.0
 */
 

static void decode_CosEventComm_PullSupplier_disconnect_pull_supplier(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int *offset, MessageHeader *header, gchar *operation) {

    gboolean stream_is_big_endian;          /* big endianess */

    
    /* Operation specific Variable declarations Begin */
    
    
    /* Operation specific Variable declarations End */
    
    
    stream_is_big_endian = is_big_endian(header);
    
    switch(header->message_type) {
    
    case Request:
    
        break;
        
    case Reply:
    
        switch(header->rep_status) {
        
        case NO_EXCEPTION:
        
            
            /* Function returns void */
            
            
            break;
            
        case USER_EXCEPTION:
        
            break;
            
        default:
        
            /* Unknown Exception */
        
            g_warning("Unknown Exception ");
        
            
        
            break;
        
        
        }   /* switch(header->message_type) */
        
        break;   
        
    default:
    
        /* Unknown GIOP Message */
    
        g_warning("Unknown GIOP Message");
        
    
        break;
    
        
    } /* switch(header->message_type) */ 
    
}


/*
 * IDL:omg.org/CosEventComm/PullConsumer/disconnect_pull_consumer:1.0
 */
 

static void decode_CosEventComm_PullConsumer_disconnect_pull_consumer(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int *offset, MessageHeader *header, gchar *operation) {

    gboolean stream_is_big_endian;          /* big endianess */

    
    /* Operation specific Variable declarations Begin */
    
    
    /* Operation specific Variable declarations End */
    
    
    stream_is_big_endian = is_big_endian(header);
    
    switch(header->message_type) {
    
    case Request:
    
        break;
        
    case Reply:
    
        switch(header->rep_status) {
        
        case NO_EXCEPTION:
        
            
            /* Function returns void */
            
            
            break;
            
        case USER_EXCEPTION:
        
            break;
            
        default:
        
            /* Unknown Exception */
        
            g_warning("Unknown Exception ");
        
            
        
            break;
        
        
        }   /* switch(header->message_type) */
        
        break;   
        
    default:
    
        /* Unknown GIOP Message */
    
        g_warning("Unknown GIOP Message");
        
    
        break;
    
        
    } /* switch(header->message_type) */ 
    
}

static gboolean dissect_coseventcomm(tvbuff_t *tvb, packet_info *pinfo, proto_tree *ptree, int *offset, MessageHeader *header, gchar *operation, gchar *idlname) {

    proto_item *ti = NULL;
    proto_tree *tree = NULL;            /* init later, inside if(tree) */
    
    gboolean be;                        /* big endianess */
    guint32  offset_saved = (*offset);  /* save in case we must back out */

    pinfo->current_proto = "COSEVENTCOMM";

    if (check_col(pinfo->fd, COL_PROTOCOL))
       col_add_str(pinfo->fd, COL_PROTOCOL, "COSEVENTCOMM");

    if (ptree) {
       ti = proto_tree_add_item(ptree, proto_coseventcomm, tvb, *offset, tvb_length(tvb) - *offset, FALSE);
       tree = proto_item_add_subtree(ti, ett_coseventcomm);
    }  


    be = is_big_endian(header);         /* get endianess - TODO use passed in stream_is_big_endian instead ? */

    /* If we have a USER Exception, then decode it and return */

    if ((header->message_type == Reply) && (header->rep_status == USER_EXCEPTION)) {

       return decode_user_exception(tvb, pinfo, tree, offset, header, operation);

    }

    

    switch(header->message_type) {
    
    case Request:
    case Reply:
    
    
        if (!strcmp(operation, CosEventComm_PushConsumer_push_op )) {
           decode_CosEventComm_PushConsumer_push(tvb, pinfo, tree, offset, header, operation);
           return TRUE;
        }
        
        if (!strcmp(operation, CosEventComm_PushConsumer_disconnect_push_consumer_op )) {
           decode_CosEventComm_PushConsumer_disconnect_push_consumer(tvb, pinfo, tree, offset, header, operation);
           return TRUE;
        }
        
        if (!strcmp(operation, CosEventComm_PushSupplier_disconnect_push_supplier_op )) {
           decode_CosEventComm_PushSupplier_disconnect_push_supplier(tvb, pinfo, tree, offset, header, operation);
           return TRUE;
        }
        
        if (!strcmp(operation, CosEventComm_PullSupplier_pull_op )) {
           decode_CosEventComm_PullSupplier_pull(tvb, pinfo, tree, offset, header, operation);
           return TRUE;
        }
        
        if (!strcmp(operation, CosEventComm_PullSupplier_try_pull_op )) {
           decode_CosEventComm_PullSupplier_try_pull(tvb, pinfo, tree, offset, header, operation);
           return TRUE;
        }
        
        if (!strcmp(operation, CosEventComm_PullSupplier_disconnect_pull_supplier_op )) {
           decode_CosEventComm_PullSupplier_disconnect_pull_supplier(tvb, pinfo, tree, offset, header, operation);
           return TRUE;
        }
        
        if (!strcmp(operation, CosEventComm_PullConsumer_disconnect_pull_consumer_op )) {
           decode_CosEventComm_PullConsumer_disconnect_pull_consumer(tvb, pinfo, tree, offset, header, operation);
           return TRUE;
        }
        
        
        break;
        
    case CancelRequest:
    case LocateRequest:
    case LocateReply:
    case CloseConnection:
    case MessageError:
    case Fragment:
       return FALSE;      /* not handled yet */
    
    default:
       return FALSE;      /* not handled yet */
    
    }   /* switch */
    
    

    return FALSE;

}  /* End of main dissector  */




/* Register the protocol with Ethereal */

void proto_register_giop_coseventcomm(void) {

   /* setup list of header fields */

#if 0
   static hf_register_info hf[] = {

      /* no fields yet */
      
   };
#endif

   /* setup protocol subtree array */

   static gint *ett[] = {
      &ett_coseventcomm,
   };

   /* Register the protocol name and description */
   
   proto_coseventcomm = proto_register_protocol("Coseventcomm Dissector Using GIOP API" , "COSEVENTCOMM", "giop-coseventcomm" );

#if 0
   proto_register_field_array(proto_coseventcomm, hf, array_length(hf));
#endif
   proto_register_subtree_array(ett,array_length(ett));
   
}




/* register me as handler for these interfaces */

void proto_register_handoff_giop_coseventcomm(void) {


    
    #if 0
    
    /* Register for Explicit Dissection */
    
    register_giop_user_module(dissect_coseventcomm, "COSEVENTCOMM", "CosEventComm/PullConsumer", proto_coseventcomm );     /* explicit dissector */
    
    #endif
    
    
    
    #if 0
    
    /* Register for Explicit Dissection */
    
    register_giop_user_module(dissect_coseventcomm, "COSEVENTCOMM", "CosEventComm/PushSupplier", proto_coseventcomm );     /* explicit dissector */
    
    #endif
    
    
    
    #if 0
    
    /* Register for Explicit Dissection */
    
    register_giop_user_module(dissect_coseventcomm, "COSEVENTCOMM", "CosEventComm/PushConsumer", proto_coseventcomm );     /* explicit dissector */
    
    #endif
    
    
    
    #if 0
    
    /* Register for Explicit Dissection */
    
    register_giop_user_module(dissect_coseventcomm, "COSEVENTCOMM", "CosEventComm/PullSupplier", proto_coseventcomm );     /* explicit dissector */
    
    #endif
    
    
    
    
    /* Register for Heuristic Dissection */
    
    register_giop_user(dissect_coseventcomm, "COSEVENTCOMM" ,proto_coseventcomm);     /* heuristic dissector */ 
    
    

}



#ifndef __ETHEREAL_STATIC__

G_MODULE_EXPORT void
plugin_reg_handoff(void){
   proto_register_handoff_giop_coseventcomm();
}

G_MODULE_EXPORT void
plugin_init(plugin_address_table_t *pat){
   /* initialise the table of pointers needed in Win32 DLLs */
   plugin_address_table_init(pat);
   if (proto_coseventcomm == -1) {
     proto_register_giop_coseventcomm();
   }
}

#endif


