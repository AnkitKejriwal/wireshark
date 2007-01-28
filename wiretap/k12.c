/*
 * k12.c
 *
 *  routines for importing tektronix k12xx *.rf5 files
 *
 *  Copyright (c) 2005, Luis E. Garia Ontanon <luis.ontanon@gmail.com>
 *
 * $Id$
 *
 * Wiretap Library
 * Copyright (c) 1998 by Gilbert Ramirez <gram@alumni.rice.edu>
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

/*#define DEBUG_K12*/

#ifdef DEBUG_K12
#include <stdio.h>
#include <stdarg.h>

FILE* dbg_out = NULL;
char* env_file = NULL;

static unsigned debug_level = 0;

void k12_dbg(unsigned  level, char* fmt, ...) {
    va_list ap;
    
    if (level > debug_level) return;
    
    va_start(ap,fmt);
    vfprintf(dbg_out, fmt, ap);
    va_end(ap);
    
    fprintf(dbg_out,"\n");
    
}


void k12_hexdump(unsigned offset, char* label, unsigned char* b, unsigned len) {
    static const char* c2t[] = {
        "00","01","02","03","04","05","06","07","08","09","0a","0b","0c","0d","0e","0f", 
        "10","11","12","13","14","15","16","17","18","19","1a","1b","1c","1d","1e","1f", 
        "20","21","22","23","24","25","26","27","28","29","2a","2b","2c","2d","2e","2f", 
        "30","31","32","33","34","35","36","37","38","39","3a","3b","3c","3d","3e","3f", 
        "40","41","42","43","44","45","46","47","48","49","4a","4b","4c","4d","4e","4f", 
        "50","51","52","53","54","55","56","57","58","59","5a","5b","5c","5d","5e","5f", 
        "60","61","62","63","64","65","66","67","68","69","6a","6b","6c","6d","6e","6f", 
        "70","71","72","73","74","75","76","77","78","79","7a","7b","7c","7d","7e","7f", 
        "80","81","82","83","84","85","86","87","88","89","8a","8b","8c","8d","8e","8f", 
        "90","91","92","93","94","95","96","97","98","99","9a","9b","9c","9d","9e","9f", 
        "a0","a1","a2","a3","a4","a5","a6","a7","a8","a9","aa","ab","ac","ad","ae","af", 
        "b0","b1","b2","b3","b4","b5","b6","b7","b8","b9","ba","bb","bc","bd","be","bf", 
        "c0","c1","c2","c3","c4","c5","c6","c7","c8","c9","ca","cb","cc","cd","ce","cf", 
        "d0","d1","d2","d3","d4","d5","d6","d7","d8","d9","da","db","dc","dd","de","df", 
        "e0","e1","e2","e3","e4","e5","e6","e7","e8","e9","ea","eb","ec","ed","ee","ef", 
        "f0","f1","f2","f3","f4","f5","f6","f7","f8","f9","fa","fb","fc","fd","fe","ff"
    };
    unsigned i;
    
    if (debug_level < 2) return;
    
    fprintf(dbg_out,"%s(%.4x,%.4x): ",label,offset,len);
    
    for (i=0 ; i<len ; i++) {
        if (!(i%64))
            fprintf(dbg_out,"\n");
        else if (!(i%4))
            fprintf(dbg_out,"  ");
        
        fprintf(dbg_out,c2t[b[i]]);
    }
    fprintf(dbg_out,"\n");
    
}
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "wtap-int.h"
#include "wtap.h"
#include "file_wrappers.h"
#include "buffer.h"
#include "k12.h"

/*
 * the 32 bits .rf5 file contains:
 *  an 8 byte magic number
 *  32bit lenght
 *  32bit number of records
 *  other 0x200 bytes bytes of uncharted territory
 *     1 or more copies of the num_of_records in there
 *  the records whose first 32bits word is the length
 *     they are stuffed by one to four words every 0x2000 bytes
 *  and a 2 byte terminator FFFF
 */

static const guint8 k12_file_magic[] = { 0x00, 0x00, 0x02, 0x00 ,0x12, 0x05, 0x00, 0x10 };

struct _k12_t {
	guint32 file_len;
	guint32 num_of_records; /* XXX: not sure about this */
	
	GHashTable* src_by_id; /* k12_srcdsc_recs by input */
	GHashTable* src_by_name; /* k12_srcdsc_recs by stack_name */
};

typedef struct _k12_src_desc_t {
	guint32 input;
	guint32 input_type;
	gchar* input_name;
	gchar* stack_file;
	k12_input_info_t input_info;
} k12_src_desc_t;


/* so far we've seen only 7 types of records */
#define K12_REC_PACKET		0x00010020
#define K12_REC_SRCDSC		0x00070041 /* port-stack mapping + more, the key of the whole thing */
#define K12_REC_SCENARIO	0x00070040 /* what appears as the window's title */
#define K12_REC_70042		0x00070042 /* XXX: ??? */ 
#define K12_REC_70044		0x00070044 /* text with a grammar (conditions/responses) */
#define K12_REC_20030		0x00020030 /* human readable start time  */ 
#define K12_REC_20032		0x00020031 /* human readable stop time */

#define K12_MASK_PACKET		0xfffffff0


#define K12_RECORD_LEN         0x0
#define K12_RECORD_TYPE        0x4
#define K12_RECORD_FRAME_LEN   0x8
#define K12_RECORD_INPUT      0xc

#define K12_PACKET_TIMESTAMP  0x18
#define K12_PACKET_FRAME      0x20

#define K12_PACKET_OFFSET_VP  0x08
#define K12_PACKET_OFFSET_VC  0x0a
#define K12_PACKET_OFFSET_CID 0x0c

#define K12_SRCDESC_EXTRALEN   0x1e
#define K12_SRCDESC_NAMELEN    0x20
#define K12_SRCDESC_STACKLEN   0x22

#define K12_SRCDESC_EXTRATYPE  0x24
#define K12_SRCDESC_ATM_VPI    0x38
#define K12_SRCDESC_ATM_VCI    0x3a
#define K12_SRCDESC_DS0_MASK   0x3c


/*
 * get_record: Get the next record into a buffer
 *   Every about 0x2000 bytes 0x10 bytes are inserted in the file,
 *   even in the middle of a record.
 *   This reads the next record without the eventual 0x10 bytes.
 *   returns the lenght of the record + the stuffing (if any)
 *
 * XXX: works at most with 0x1FFF bytes per record 
 */
static gint get_record(guint8* buffer, FILE* fh, gint64 file_offset) {
	long read;
	long len;
	gint64 i;
	gint64 junky_offset = 0x2000 - ( (file_offset - 0x200) % 0x2000 );
    
#ifdef DEBUG_K12
    k12_dbg(5,"k12:get_record: ENTER offset=%lld",file_offset);
#endif
    
	if  ( junky_offset != 0x2000 ) {
		
		/* safe header */
		read = file_read(buffer,1, 0x4,fh);
		
		if (read == 2 && buffer[0] == 0xff && buffer[1] == 0xff) {
#ifdef DEBUG_K12
            k12_dbg(1,"k12:get_record: EOF");
#endif
            
			return 0;
		} else if ( read != 0x4 ) {
#ifdef DEBUG_K12
            k12_dbg(1,"k12:get_record: SHORT READ");
#endif
            
			return -1;
		}
		
		len = pntohl(buffer) & 0x00001FFF;
		
		if (junky_offset > len) {
			/* safe body */
			if (len - 0x4 <= 0) {
#ifdef DEBUG_K12
                k12_dbg(1,"k12:get_record: TOO SHORT");
#endif                
				return -1;
			}
			
			if ( file_read(buffer+0x4, 1, len - 0x4, fh) < len - 0x4 ) {
#ifdef DEBUG_K12
                k12_dbg(1,"k12:get_record: SHORT READ");
#endif
                
				return -1;
			} else {
#ifdef DEBUG_K12
                k12_hexdump(file_offset, "GOT record", buffer, len);
#endif
                
				return len;
			}
		} else {
			if ( file_read(buffer+0x4, 1, len + 0xC, fh) < len ) {
				return -1;
			}
			
			for (i = junky_offset; i < len; i++) {
				buffer[i] = buffer[i+0x10];
			}
#ifdef DEBUG_K12
            k12_hexdump(file_offset, "GOT record", buffer, len);
#endif
            
			return len + 0x10;
		}
	} else {
		/* unsafe header */
		
		read = file_read(buffer,1,0x14,fh);
		
		if (read == 2 && buffer[0] == 0xff && buffer[1] == 0xff) {
#ifdef DEBUG_K12
            k12_dbg(1,"k12:get_record: EOF");
#endif
            
			return 0;
		} else if ( read < 0x14 ){
#ifdef DEBUG_K12
            k12_dbg(1,"k12:get_record: SHORT READ");
#endif
            
			return -1;
		}
		
		for (i = 0; i < 0x10; i++) {
			buffer[i] = buffer[i+0x10];
		}
		
		len = pntohl(buffer) & 0x00001FFF;
		
		if (len - 0x4 <= 0) {
#ifdef DEBUG_K12
            k12_dbg(1,"k12:get_record: SHORT RECORD");
#endif
			return -1;
        }
		/* safe body */
		if ( file_read(buffer + 0x4, 1, len - 0x4,fh) < len - 0x4 ) {
#ifdef DEBUG_K12
            k12_dbg(1,"k12:get_record: READ ERROR");
#endif
			return -1;
		} else {
            
#ifdef DEBUG_K12
            k12_hexdump(file_offset, "GOT stuffed record", buffer, len);
#endif
            
			return len + 0x10;
		}
	}
}

static gboolean k12_read(wtap *wth, int *err, gchar **err_info _U_, gint64 *data_offset) {
	k12_src_desc_t* src_desc;
	guint8 buffer[0x2000];
	gint64 offset;
	long len;
	guint32 type;
	guint64 ts;
	
	offset = wth->data_offset;
    
#ifdef DEBUG_K12
    k12_dbg(5,"k12_read: offset=%i",offset);
#endif
    
	/* ignore the record if it isn't a packet */	
	do {
		*data_offset = offset;
		
		len = get_record(buffer, wth->fh, offset);
		
		if (len < 0) {
			*err = WTAP_ERR_SHORT_READ;
			return FALSE;
		} else if (len == 0) {
			*err = 0;
			return FALSE;
		}
		
		type = pntohl(buffer + K12_RECORD_TYPE);
        
#ifdef DEBUG_K12
        k12_dbg(5,"k12_read: record type=%i",offset);
#endif
        
		offset += len;
		
	} while ( (type & K12_MASK_PACKET) != K12_REC_PACKET );
	
	wth->data_offset = offset;
	
	ts = pntohll(buffer + K12_PACKET_TIMESTAMP);
	
    
	wth->phdr.ts.secs = (guint32) ((ts / 2000000) + 631152000);
	wth->phdr.ts.nsecs = (guint32) ( (ts % 2000000) * 500 );
    
#ifdef DEBUG_K12
    k12_dbg(5,"k12_read: secs=%u nsecs=%u", wth->phdr.ts.secs,wth->phdr.ts.nsecs);
#endif
    
	wth->phdr.len = wth->phdr.caplen = pntohl(buffer + K12_RECORD_FRAME_LEN) & 0x00001FFF;
	
	/* the frame */
	buffer_assure_space(wth->frame_buffer, wth->phdr.caplen);
	memcpy(buffer_start_ptr(wth->frame_buffer), buffer + K12_PACKET_FRAME, wth->phdr.caplen);
	
	wth->pseudo_header.k12.input = pntohl(buffer + K12_RECORD_INPUT);
    
#ifdef DEBUG_K12
    k12_dbg(5,"k12_read: wth->pseudo_header.k12.input=%x wth->phdr.len=%i",wth->pseudo_header.k12.input,wth->phdr.len);
#endif
    
    src_desc = g_hash_table_lookup(wth->capture.k12->src_by_id,GUINT_TO_POINTER(wth->pseudo_header.k12.input));
	
	if (src_desc) {
        
#ifdef DEBUG_K12
        k12_dbg(5,"k12_read: input_name='%s' stack_file='%s' type=%x",src_desc->input_name,src_desc->stack_file,src_desc->input_type);
#endif
        wth->pseudo_header.k12.input_name = src_desc->input_name;
		wth->pseudo_header.k12.stack_file = src_desc->stack_file;
		wth->pseudo_header.k12.input_type = src_desc->input_type;
        
        switch(src_desc->input_type) {
            case K12_PORT_ATMPVC:
                wth->pseudo_header.k12.input_info.atm.vp =  pntohs(buffer + (K12_PACKET_FRAME + wth->phdr.caplen + K12_PACKET_OFFSET_VP));
                wth->pseudo_header.k12.input_info.atm.vc =  pntohs(buffer + (K12_PACKET_FRAME + wth->phdr.caplen + K12_PACKET_OFFSET_VC));
                wth->pseudo_header.k12.input_info.atm.cid =  *((unsigned char*)(buffer + K12_PACKET_FRAME + wth->phdr.len + K12_PACKET_OFFSET_CID));
                break;
            default:
                memcpy(&(wth->pseudo_header.k12.input_info),&(src_desc->input_info),sizeof(src_desc->input_info));
                break;
                
        }
	} else {
        
#ifdef DEBUG_K12
        k12_dbg(5,"k12_read: NO RECORD FOUND");
#endif
        
		memset(&(wth->pseudo_header),0,sizeof(wth->pseudo_header));
		wth->pseudo_header.k12.input_name = "unknown port";
		wth->pseudo_header.k12.stack_file = "unknown stack file";
	}
	
	wth->pseudo_header.k12.stuff = wth->capture.k12;
    
	return TRUE;
}


static gboolean k12_seek_read(wtap *wth, gint64 seek_off, union wtap_pseudo_header *pseudo_header, guchar *pd, int length, int *err _U_, gchar **err_info _U_) {
	k12_src_desc_t* src_desc;
	guint8 buffer[0x2000];
    guint32 input;
    
#ifdef DEBUG_K12
    k12_dbg(5,"k12_seek_read: ENTER");
#endif
    
	if ( file_seek(wth->random_fh, seek_off, SEEK_SET, err) == -1) {
#ifdef DEBUG_K12
        k12_dbg(5,"k12_seek_read: SEEK ERROR");
#endif
		return FALSE;
	}
	
	if (get_record(buffer, wth->random_fh, seek_off) < 1) {
#ifdef DEBUG_K12
        k12_dbg(5,"k12_seek_read: READ ERROR");
#endif
		return FALSE;
	}
	
	memcpy(pd, buffer + K12_PACKET_FRAME, length);
	
    input = pntohl(buffer + K12_RECORD_INPUT);
#ifdef DEBUG_K12
    k12_dbg(5,"k12_seek_read: input=%.8x",input);
#endif
    
	src_desc = g_hash_table_lookup(wth->capture.k12->src_by_id,GUINT_TO_POINTER(input));
    
	if (src_desc) {
        
#ifdef DEBUG_K12
        k12_dbg(5,"k12_seek_read: input_name='%s' stack_file='%s' type=%x",src_desc->input_name,src_desc->stack_file,src_desc->input_type);
#endif
        if (pseudo_header) {
            pseudo_header->k12.input_name = src_desc->input_name;
            pseudo_header->k12.stack_file = src_desc->stack_file;
            pseudo_header->k12.input_type = src_desc->input_type;
            
            switch(src_desc->input_type) {
                case K12_PORT_ATMPVC:
                    pseudo_header->k12.input_info.atm.vp =  pntohs(buffer + K12_PACKET_FRAME + length + K12_PACKET_OFFSET_VP);
                    pseudo_header->k12.input_info.atm.vc =  pntohs(buffer + K12_PACKET_FRAME + length + K12_PACKET_OFFSET_VC);
                    pseudo_header->k12.input_info.atm.cid =  *((unsigned char*)(buffer + K12_PACKET_FRAME + length + K12_PACKET_OFFSET_CID));
                    break;
                default:
                    memcpy(&(pseudo_header->k12.input_info),&(src_desc->input_info),sizeof(src_desc->input_info));
                    break;
            }
            
        }
        
        wth->pseudo_header.k12.input_name = src_desc->input_name;
		wth->pseudo_header.k12.stack_file = src_desc->stack_file;
		wth->pseudo_header.k12.input_type = src_desc->input_type;
		
        switch(src_desc->input_type) {
            case K12_PORT_ATMPVC:
                wth->pseudo_header.k12.input_info.atm.vp =  pntohs(buffer + K12_PACKET_FRAME + length + K12_PACKET_OFFSET_VP);
                wth->pseudo_header.k12.input_info.atm.vc =  pntohs(buffer + K12_PACKET_FRAME + length + K12_PACKET_OFFSET_VC);
                wth->pseudo_header.k12.input_info.atm.cid =  *((unsigned char*)(buffer + K12_PACKET_FRAME + length + K12_PACKET_OFFSET_CID));
                break;
            default:
                memcpy(&(wth->pseudo_header.k12.input_info),&(src_desc->input_info),sizeof(src_desc->input_info));
                break;
        }
        
	} else {
        
#ifdef DEBUG_K12
        k12_dbg(5,"k12_seek_read: NO SRC_RECORD FOUND");
#endif
        
        if (pseudo_header) {
            memset(&(pseudo_header->k12),0,sizeof(pseudo_header->k12));
            pseudo_header->k12.input_name = "unknown port";
            pseudo_header->k12.stack_file = "unknown stack file";
        }
        
		memset(&(wth->pseudo_header.k12),0,sizeof(wth->pseudo_header.k12));
		wth->pseudo_header.k12.input_name = "unknown port";
		wth->pseudo_header.k12.stack_file = "unknown stack file";
        
    }
	
    if (pseudo_header) {
        pseudo_header->k12.input = input;
        pseudo_header->k12.stuff = wth->capture.k12;
    }
    
    wth->pseudo_header.k12.input = input;
	wth->pseudo_header.k12.stuff = wth->capture.k12;
    
#ifdef DEBUG_K12
    k12_dbg(5,"k12_seek_read: DONE OK");
#endif
    
	return TRUE;
}


static k12_t* new_k12_file_data() {
	k12_t* fd = g_malloc(sizeof(k12_t));
	
	fd->file_len = 0;
	fd->num_of_records = 0;
	fd->src_by_name = g_hash_table_new(g_str_hash,g_str_equal);
	fd->src_by_id = g_hash_table_new(g_direct_hash,g_direct_equal);
	
	return fd;
}

static gboolean destroy_srcdsc(gpointer k _U_, gpointer v, gpointer p _U_) {
	k12_src_desc_t* rec = v;
	
	if(rec->input_name)
		g_free(rec->input_name);
	
	if(rec->stack_file)
		g_free(rec->stack_file);
	
	g_free(rec);
	
	return TRUE;
}

static void destroy_k12_file_data(k12_t* fd) {
	g_hash_table_destroy(fd->src_by_id);
	g_hash_table_foreach_remove(fd->src_by_name,destroy_srcdsc,NULL);	
	g_hash_table_destroy(fd->src_by_name);
	g_free(fd);
}

static void k12_close(wtap *wth) {
	destroy_k12_file_data(wth->capture.k12);
#ifdef DEBUG_K12
    k12_dbg(5,"k12_close: CLOSED");
    if (env_file) fclose(dbg_out);
#endif
}


int k12_open(wtap *wth, int *err, gchar **err_info _U_) {
	k12_src_desc_t* rec;
	guint8 read_buffer[0x2000];
	guint32 type;
	long offset;
	long len;
	guint32 rec_len;
	guint32 extra_len;
	guint32 name_len;
	guint32 stack_len;
	guint i;
	k12_t* file_data;
    
#ifdef DEBUG_K12
    gchar* env_level = getenv("K12_DEBUG_LEVEL");
    env_file = getenv("K12_DEBUG_FILENAME");
    if ( env_file ) dbg_out = eth_fopen(env_file,"w");
    else dbg_out = stderr;
    if ( env_level ) debug_level = strtoul(env_level,NULL,10);
    k12_dbg(1,"k12_open: ENTER debug_level=%u",debug_level);
#endif
    
	if ( file_read(read_buffer,1,0x200,wth->fh) != 0x200 ) {
        
#ifdef DEBUG_K12
        k12_dbg(1,"k12_open: FILE HEADER TOO SHORT");
#endif
        
		return 0;
	} else {
		if ( memcmp(read_buffer,k12_file_magic,8) != 0 ) {
#ifdef DEBUG_K12
            k12_dbg(1,"k12_open: BAD MAGIC");
#endif
			return 0;
		}
	}
	
	offset = 0x200;
    
	file_data = new_k12_file_data();
	
	file_data->file_len = pntohl( read_buffer + 0x8);
	file_data->num_of_records = pntohl( read_buffer + 0xC );
    
#ifdef DEBUG_K12
    k12_dbg(5,"k12_open: FILE_HEADER OK: offset=%x file_len=%i records=%i",
            offset,
            file_data->file_len,
            file_data->num_of_records );
#endif
    
	do {
		
		len = get_record(read_buffer, wth->fh, offset);
        
		if ( len <= 0 ) {
#ifdef DEBUG_K12
            k12_dbg(1,"k12_open: BAD HEADER RECORD");
#endif            
			return -1;
		}
        
        
		type = pntohl( read_buffer + K12_RECORD_TYPE );
        
		if ( (type & K12_MASK_PACKET) == K12_REC_PACKET) {
			/*
			 * we are at the first packet record, rewind and leave.
			 */
			if (file_seek(wth->fh, offset, SEEK_SET, err) == -1) {
				destroy_k12_file_data(file_data);
				return -1;
			}
#ifdef DEBUG_K12
            k12_dbg(5,"k12_open: FIRST PACKET offset=%x",offset);
#endif            			
			break;
		} else if (type == K12_REC_SRCDSC) {
			rec = g_malloc0(sizeof(k12_src_desc_t));
			
			rec_len = pntohl( read_buffer + K12_RECORD_LEN );
			extra_len = pntohs( read_buffer + K12_SRCDESC_EXTRALEN );
			name_len = pntohs( read_buffer + K12_SRCDESC_NAMELEN );
			stack_len = pntohs( read_buffer + K12_SRCDESC_STACKLEN );
            
            rec->input = pntohl( read_buffer + K12_RECORD_INPUT );
            
#ifdef DEBUG_K12
            k12_dbg(5,"k12_open: INTERFACE RECORD offset=%x interface=%x",offset,rec->input);
#endif
            
			if (name_len == 0 || stack_len == 0
				|| 0x20 + extra_len + name_len + stack_len > rec_len ) {
				g_free(rec);
#ifdef DEBUG_K12
                k12_dbg(5,"k12_open: failed (name_len == 0 || stack_len == 0 "
                        "|| 0x20 + extra_len + name_len + stack_len > rec_len)  extra_len=%i name_len=%i stack_len=%i");
#endif
                
				return 0;
			}
			
			switch(( rec->input_type = pntohl( read_buffer + K12_SRCDESC_EXTRATYPE ) )) {
				case K12_PORT_DS0S:
					rec->input_info.ds0mask = 0x00000000;
					
					for (i = 0; i < 32; i++) {
						rec->input_info.ds0mask |= ( *(read_buffer + K12_SRCDESC_DS0_MASK + i) == 0xff ) ? 0x1<<(31-i) : 0x0; 
					}
						
						break;
				case K12_PORT_ATMPVC:
					rec->input_info.atm.vp = pntohs( read_buffer + K12_SRCDESC_ATM_VPI );
					rec->input_info.atm.vc = pntohs( read_buffer + K12_SRCDESC_ATM_VCI );
					break;
				default:
					break;
			}
			
			rec->input_name = g_memdup(read_buffer + K12_SRCDESC_EXTRATYPE + extra_len, name_len);
			rec->stack_file = g_memdup(read_buffer + K12_SRCDESC_EXTRATYPE + extra_len + name_len, stack_len);
			
			g_strdown(rec->stack_file);
            
			g_hash_table_insert(file_data->src_by_id,GUINT_TO_POINTER(rec->input),rec);
			g_hash_table_insert(file_data->src_by_name,rec->stack_file,rec);
			
			offset += len;
			continue;
		} else {
			offset += len;
			continue;
		}
	} while(1);
	
	wth->data_offset = offset;
	wth->file_type = WTAP_FILE_K12;
	wth->file_encap = WTAP_ENCAP_K12;
	wth->snapshot_length = 0;
	wth->subtype_read = k12_read;
	wth->subtype_seek_read = k12_seek_read;
	wth->subtype_close = k12_close;
	wth->capture.k12 = file_data;
	wth->tsprecision = WTAP_FILE_TSPREC_NSEC;	
	
	return 1;
}

int k12_dump_can_write_encap(int encap) {
	
	if (encap == WTAP_ENCAP_PER_PACKET)
		return WTAP_ERR_ENCAP_PER_PACKET_UNSUPPORTED;
	
	if (encap != WTAP_ENCAP_K12)
		return WTAP_ERR_UNSUPPORTED_ENCAP;
	
	return 0;
}

static const gchar dumpy_junk[] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

static void k12_dump_record(wtap_dumper *wdh, long len,  guint8* buffer) {
	long junky_offset = (0x2000 - ( (wdh->dump.k12->file_offset - 0x200) % 0x2000 )) % 0x2000;
	
	if (len > junky_offset) {
		
		if (junky_offset)
			fwrite(buffer, 1, junky_offset, wdh->fh);
		
		fwrite(dumpy_junk, 1, 0x10, wdh->fh);
		
		fwrite(buffer+junky_offset, 1, len - junky_offset, wdh->fh);
		
		wdh->dump.k12->file_offset += len + 0x10;
	} else {
		fwrite(buffer, 1, len, wdh->fh);
		wdh->dump.k12->file_offset += len;
	}
	
	wdh->dump.k12->num_of_records++;
}

static void k12_dump_src_setting(gpointer k _U_, gpointer v, gpointer p) {
	k12_src_desc_t* src_desc = v;
	wtap_dumper *wdh = p;
	guint32 len;
	guint offset;
	guint i;
	
	union {
		guint8 buffer[0x2000];
		
		struct {
			guint32 len;
			guint32 type;
			guint32 unk32_1;
			guint32 input;
			
			guint32 unk32_2;
			guint32 unk32_3;
			guint32 unk32_4;
			guint16 unk16_1;
			guint16 extra_len;
			
			guint16 name_len;
			guint16 stack_len;
			
			struct {
				guint32 type;
				
				union {
					struct {
						guint32 unk32;
						guint8 mask[32];
					} ds0mask;
					
					struct {
						guint8 unk_data[0x10];
						guint16 vp;
						guint16 vc;
					} atm;
					
					guint32 unk;
				} desc;
			} extra;
		} record;
	} obj;
	
	obj.record.type = g_htonl(K12_REC_SRCDSC);
	obj.record.unk32_1 = g_htonl(0x00000001);
	obj.record.input = g_htonl(src_desc->input);
	
	obj.record.unk32_2 = g_htonl(0x0000060f);
	obj.record.unk32_3 = g_htonl(0x00000003);
	obj.record.unk32_4 = g_htonl(0x01000100);
    
	obj.record.unk16_1 = g_htons(0x0000);
	obj.record.name_len = strlen(src_desc->input_name) + 1;
	obj.record.stack_len = strlen(src_desc->stack_file) + 1;
    
	obj.record.extra.type = g_htonl(src_desc->input_type);
	
	switch (src_desc->input_type) {
		case K12_PORT_ATMPVC:
			obj.record.extra_len = g_htons(0x18);
			obj.record.extra.desc.atm.vp = g_htons(src_desc->input_info.atm.vp);
			obj.record.extra.desc.atm.vc = g_htons(src_desc->input_info.atm.vc);
			offset = 0x3c;
			break;
		case K12_PORT_DS0S:
			obj.record.extra_len = g_htons(0x18);
			for( i=0; i<32; i++ ) {
				obj.record.extra.desc.ds0mask.mask[i] =
                (src_desc->input_info.ds0mask & (1 << i)) ? 0xff : 0x00;
			}
                offset = 0x3c;
			break;
		default:
			obj.record.extra_len = g_htons(0x08);
			offset = 0x2c;
			break;
	}
	
	memcpy(obj.buffer + offset,
		   src_desc->input_name,
		   obj.record.name_len);
	
	memcpy(obj.buffer + offset + obj.record.name_len,
		   src_desc->stack_file,
		   obj.record.stack_len);
	
	len = offset + obj.record.name_len + obj.record.stack_len;
	len += (len % 4) ? 4 - (len % 4) : 0;
	
	obj.record.len = g_htonl(len);
	obj.record.name_len =  g_htons(obj.record.name_len);
	obj.record.stack_len = g_htons(obj.record.stack_len);
    
	k12_dump_record(wdh,len,obj.buffer);
}

static gboolean k12_dump(wtap_dumper *wdh, const struct wtap_pkthdr *phdr,
                         const union wtap_pseudo_header *pseudo_header,
                         const guchar *pd, int *err _U_) {
	long len;
	union {
		guint8 buffer[0x2000];
		struct {
			guint32 len;
			guint32 type;
			guint32 frame_len;
			guint32 input;
			
			guint32 datum_1;
			guint32 datum_2;
			guint64 ts;
			
			guint8 frame[0x1fc0];
		} record;
	} obj;
	
	if (wdh->dump.k12->num_of_records == 0) {
		k12_t* file_data = pseudo_header->k12.stuff;
		g_hash_table_foreach(file_data->src_by_id,k12_dump_src_setting,wdh);
	}
	
	obj.record.len = 0x20 + phdr->len;
	obj.record.len += (obj.record.len % 4) ? 4 - obj.record.len % 4 : 0;
    
	len = obj.record.len;
	
	obj.record.len = g_htonl(obj.record.len);
    
	obj.record.type = g_htonl(K12_REC_PACKET);
	obj.record.frame_len = g_htonl(phdr->len);
	obj.record.input = g_htonl(pseudo_header->k12.input);
	
	obj.record.ts = GUINT64_TO_BE((((guint64)phdr->ts.secs - 631152000) * 2000000) + (phdr->ts.nsecs / 1000 * 2));
    
	memcpy(obj.record.frame,pd,phdr->len);
	
	k12_dump_record(wdh,len,obj.buffer);
	
	/* XXX if OK */
	return TRUE;
}

static const guint8 k12_eof[] = {0xff,0xff};

static gboolean k12_dump_close(wtap_dumper *wdh, int *err) {
	union {
		guint8 b[sizeof(guint32)];
		guint32 u;
	} d;
	
	fwrite(k12_eof, 1, 2, wdh->fh);
    
	if (fseek(wdh->fh, 8, SEEK_SET) == -1) {
		*err = errno;
		return FALSE;
	}
	
	d.u = g_htonl(wdh->dump.k12->file_len);
	
	fwrite(d.b, 1, 4, wdh->fh);
    
	d.u = g_htonl(wdh->dump.k12->num_of_records);
	
	fwrite(d.b, 1, 4, wdh->fh);
    
	return TRUE;
}


gboolean k12_dump_open(wtap_dumper *wdh, gboolean cant_seek, int *err) {
	
	if (cant_seek) {
		*err = WTAP_ERR_CANT_WRITE_TO_PIPE;
		return FALSE;
	}
	
	if ( fwrite(k12_file_magic, 1, 8, wdh->fh) != 8 ) {
		*err = errno;
		return FALSE;
	}
	
	if (fseek(wdh->fh, 0x200, SEEK_SET) == -1) {
		*err = errno;
		return FALSE;
	}
    
	wdh->subtype_write = k12_dump;
	wdh->subtype_close = k12_dump_close;
	
	wdh->dump.k12 = g_malloc(sizeof(k12_dump_t));
	wdh->dump.k12->file_len = 0x200;
	wdh->dump.k12->num_of_records = 0;
	wdh->dump.k12->file_offset  = 0x200;
	
	return TRUE;
}


