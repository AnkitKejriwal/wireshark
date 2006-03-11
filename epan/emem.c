/* emem.c
 * Ethereal memory management and garbage collection functions
 * Ronnie Sahlberg 2005
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <time.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef _WIN32
#include <windows.h>	/* VirtualAlloc, VirtualProtect */
#include <process.h>    /* getpid */
#endif

#include <glib.h>
#include <proto.h>
#include "emem.h"
#include <wiretap/file_util.h>


/*
 * Tools like Valgrind and ElectricFence don't work well with memchunks.
 * Uncomment the defines below to make {ep|se}_alloc() allocate each
 * object individually.
 */
/* #define EP_DEBUG_FREE 1 */
/* #define SE_DEBUG_FREE 1 */

/* Do we want to use guardpages? if available */
#define WANT_GUARD_PAGES 1

/* Do we want to use canaries ? */
#define DEBUG_USE_CANARIES 1

 
#ifdef WANT_GUARD_PAGES
/* Add guard pages at each end of our allocated memory */
#if defined(HAVE_SYSCONF) && defined(HAVE_MMAP) && defined(HAVE_MPROTECT) && defined(HAVE_STDINT_H)
#include <stdint.h>
#include <sys/types.h>
#include <sys/mman.h>
#define USE_GUARD_PAGES 1
#endif
#endif

/* When required, allocate more memory from the OS in this size chunks */
#define EMEM_PACKET_CHUNK_SIZE 10485760

/* The maximum number of allocations per chunk */
#define EMEM_ALLOCS_PER_CHUNK (EMEM_PACKET_CHUNK_SIZE / 512)


#ifdef DEBUG_USE_CANARIES
#define EMEM_CANARY_SIZE 8
#define EMEM_CANARY_DATA_SIZE (EMEM_CANARY_SIZE * 2 - 1)
guint8  ep_canary[EMEM_CANARY_DATA_SIZE], se_canary[EMEM_CANARY_DATA_SIZE];
#endif /* DEBUG_USE_CANARIES */

typedef struct _emem_chunk_t {
	struct _emem_chunk_t *next;
	unsigned int	amount_free_init;
	unsigned int	amount_free;
	unsigned int	free_offset_init;
	unsigned int	free_offset;
	char *buf;
#ifdef DEBUG_USE_CANARIES
#if ! defined(EP_DEBUG_FREE) && ! defined(SE_DEBUG_FREE)
	unsigned int	c_count;
	void		*canary[EMEM_ALLOCS_PER_CHUNK];
	guint8		cmp_len[EMEM_ALLOCS_PER_CHUNK];
#endif
#endif /* DEBUG_USE_CANARIES */
} emem_chunk_t;

typedef struct _emem_header_t {
  emem_chunk_t *free_list;
  emem_chunk_t *used_list;
} emem_header_t;

static emem_header_t ep_packet_mem;
static emem_header_t se_packet_mem;


#ifdef DEBUG_USE_CANARIES
/*
 * Set a canary value to be placed between memchunks.
 */
void
emem_canary(guint8 *canary) {
	int i;
#if GLIB_MAJOR_VERSION >= 2
	static GRand   *rand_state = NULL;
#endif


	/* First, use GLib's random function if we have it */
#if GLIB_MAJOR_VERSION >= 2
	if (rand_state == NULL) {
		rand_state = g_rand_new();
	}
	for (i = 0; i < EMEM_CANARY_DATA_SIZE; i ++) {
		canary[i] = (guint8) g_rand_int(rand_state);
	}
	return;
#else
	FILE *fp;
	size_t sz;
	/* Try /dev/urandom */
	if ((fp = eth_fopen("/dev/urandom", "r")) != NULL) {
		sz = fread(canary, EMEM_CANARY_DATA_SIZE, 1, fp);
		fclose(fp);
		if (sz == EMEM_CANARY_SIZE) {
			return;
		}
	}

	/* Our last resort */
	srandom(time(NULL) | getpid());
	for (i = 0; i < EMEM_CANARY_DATA_SIZE; i ++) {
		canary[i] = (guint8) random();
	}
	return;
#endif /* GLIB_MAJOR_VERSION >= 2 */
}

#if !defined(SE_DEBUG_FREE)
/*
 * Given an allocation size, return the amount of padding needed for
 * the canary value.
 */
static guint8
emem_canary_pad (size_t allocation) {
	guint8 pad;

	pad = EMEM_CANARY_SIZE - (allocation % EMEM_CANARY_SIZE);
	if (pad < EMEM_CANARY_SIZE)
		pad += EMEM_CANARY_SIZE;

	return pad;
}
#endif
#endif /* DEBUG_USE_CANARIES */


/* Initialize the packet-lifetime memory allocation pool.
 * This function should be called only once when Ethereal or Tethereal starts
 * up.
 */
void
ep_init_chunk(void)
{
	ep_packet_mem.free_list=NULL;
	ep_packet_mem.used_list=NULL;

#ifdef DEBUG_USE_CANARIES
	emem_canary(ep_canary);
#endif /* DEBUG_USE_CANARIES */
}
/* Initialize the capture-lifetime memory allocation pool.
 * This function should be called only once when Ethereal or Tethereal starts
 * up.
 */
void
se_init_chunk(void)
{
	se_packet_mem.free_list=NULL;
	se_packet_mem.used_list=NULL;

#ifdef DEBUG_USE_CANARIES
	emem_canary(se_canary);
#endif /* DEBUG_USE_CANARIES */
}

#if !defined(SE_DEBUG_FREE)
static void
emem_create_chunk(emem_chunk_t **free_list) {
#if defined (_WIN32)
	SYSTEM_INFO sysinfo;
	int pagesize;
	BOOL ret;
	char *buf_end, *prot1, *prot2;
	DWORD oldprot;
#elif defined(USE_GUARD_PAGES)
	intptr_t pagesize = sysconf(_SC_PAGESIZE);
	int ret;
	char *buf_end, *prot1, *prot2;
#endif
	/* we dont have any free data, so we must allocate a new one */
	if(!*free_list){
		emem_chunk_t *npc;
		npc = g_malloc(sizeof(emem_chunk_t));
		npc->next = NULL;
#ifdef DEBUG_USE_CANARIES
#if ! defined(EP_DEBUG_FREE) && ! defined(SE_DEBUG_FREE)
		npc->c_count = 0;
#endif
#endif /* DEBUG_USE_CANARIES */

		*free_list = npc;
#if defined (_WIN32)
		/*
		 * MSDN documents VirtualAlloc/VirtualProtect at
		 * http://msdn.microsoft.com/library/en-us/memory/base/creating_guard_pages.asp
		 */
		GetSystemInfo(&sysinfo);
		pagesize = sysinfo.dwPageSize;

		/* XXX - is MEM_COMMIT|MEM_RESERVE correct? */
		npc->buf = VirtualAlloc(NULL, EMEM_PACKET_CHUNK_SIZE,
			MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
		g_assert(npc->buf != NULL);
		buf_end = npc->buf + EMEM_PACKET_CHUNK_SIZE;

		/* Align our guard pages on page-sized boundaries */
		prot1 = (char *) ((((int) npc->buf + pagesize - 1) / pagesize) * pagesize);
		prot2 = (char *) ((((int) buf_end - (1 * pagesize)) / pagesize) * pagesize);

		ret = VirtualProtect(prot1, pagesize, PAGE_NOACCESS, &oldprot);
		g_assert(ret == TRUE);
		ret = VirtualProtect(prot2, pagesize, PAGE_NOACCESS, &oldprot);
		g_assert(ret == TRUE);

		npc->amount_free_init = prot2 - prot1 - pagesize;
		npc->amount_free = npc->amount_free_init;
		npc->free_offset_init = (prot1 - npc->buf) + pagesize;
		npc->free_offset = npc->free_offset_init;

#elif defined(USE_GUARD_PAGES)
		npc->buf = mmap(NULL, EMEM_PACKET_CHUNK_SIZE,
			PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0);
		g_assert(npc->buf != MAP_FAILED);
		buf_end = npc->buf + EMEM_PACKET_CHUNK_SIZE;

		/* Align our guard pages on page-sized boundaries */
		prot1 = (char *) ((((intptr_t) npc->buf + pagesize - 1) / pagesize) * pagesize);
		prot2 = (char *) ((((intptr_t) buf_end - (1 * pagesize)) / pagesize) * pagesize);
		ret = mprotect(prot1, pagesize, PROT_NONE);
		g_assert(ret != -1);
		ret = mprotect(prot2, pagesize, PROT_NONE);
		g_assert(ret != -1);

		npc->amount_free_init = prot2 - prot1 - pagesize;
		npc->amount_free = npc->amount_free_init;
		npc->free_offset_init = (prot1 - npc->buf) + pagesize;
		npc->free_offset = npc->free_offset_init;

#else /* Is there a draft in here? */
		npc->amount_free_init = EMEM_PACKET_CHUNK_SIZE;
		npc->amount_free = npc->amount_free_init;
		npc->free_offset_init = 0;
		npc->free_offset = npc->free_offset_init;
		npc->buf = g_malloc(EMEM_PACKET_CHUNK_SIZE);
#endif /* USE_GUARD_PAGES */
	}
}
#endif

/* allocate 'size' amount of memory with an allocation lifetime until the
 * next packet.
 */
void *
ep_alloc(size_t size)
{
	void *buf;
#ifndef EP_DEBUG_FREE
#ifdef DEBUG_USE_CANARIES
	void *cptr;
	guint8 pad = emem_canary_pad(size);
#else
	static guint8 pad=8;
#endif /* DEBUG_USE_CANARIES */
	emem_chunk_t *free_list;
#endif

#ifndef EP_DEBUG_FREE
	/* Round up to an 8 byte boundary.  Make sure we have at least
	 * 8 pad bytes for our canary.
	 */
	size += pad;

	/* make sure we dont try to allocate too much (arbitrary limit) */
	DISSECTOR_ASSERT(size<(EMEM_PACKET_CHUNK_SIZE>>2));

	emem_create_chunk(&ep_packet_mem.free_list);

	/* oops, we need to allocate more memory to serve this request
         * than we have free. move this node to the used list and try again
	 */
	if(size>ep_packet_mem.free_list->amount_free
#ifdef DEBUG_USE_CANARIES
              || ep_packet_mem.free_list->c_count >= EMEM_ALLOCS_PER_CHUNK
#endif /* DEBUG_USE_CANARIES */
        ){
		emem_chunk_t *npc;
		npc=ep_packet_mem.free_list;
		ep_packet_mem.free_list=ep_packet_mem.free_list->next;
		npc->next=ep_packet_mem.used_list;
		ep_packet_mem.used_list=npc;
	}

	emem_create_chunk(&ep_packet_mem.free_list);

	free_list = ep_packet_mem.free_list;

	buf = free_list->buf + free_list->free_offset;

	free_list->amount_free -= size;
	free_list->free_offset += size;

#ifdef DEBUG_USE_CANARIES
	cptr = (char *)buf + size - pad;
	memcpy(cptr, &ep_canary, pad);
	free_list->canary[free_list->c_count] = cptr;
	free_list->cmp_len[free_list->c_count] = pad;
	free_list->c_count++;
#endif /* DEBUG_USE_CANARIES */

#else /* EP_DEBUG_FREE */
	emem_chunk_t *npc;

	npc=g_malloc(sizeof(emem_chunk_t));
	npc->next=ep_packet_mem.used_list;
	npc->amount_free=size;
	npc->free_offset=0;
	npc->buf=g_malloc(size);
	buf = npc->buf;
	ep_packet_mem.used_list=npc;
#endif /* EP_DEBUG_FREE */

	return buf;
}
/* allocate 'size' amount of memory with an allocation lifetime until the
 * next capture.
 */
void *
se_alloc(size_t size)
{
	void *buf;
#ifndef SE_DEBUG_FREE
#ifdef DEBUG_USE_CANARIES
	void *cptr;
	guint8 pad = emem_canary_pad(size);
#else
	static guint8 pad=8;
#endif /* DEBUG_USE_CANARIES */
	emem_chunk_t *free_list;
#endif

#ifndef SE_DEBUG_FREE
	/* Round up to an 8 byte boundary.  Make sure we have at least
	 * 8 pad bytes for our canary.
	 */
	size += pad;

	/* make sure we dont try to allocate too much (arbitrary limit) */
	DISSECTOR_ASSERT(size<(EMEM_PACKET_CHUNK_SIZE>>2));

	emem_create_chunk(&se_packet_mem.free_list);

	/* oops, we need to allocate more memory to serve this request
         * than we have free. move this node to the used list and try again
	 */
	if(size>se_packet_mem.free_list->amount_free
#ifdef DEBUG_USE_CANARIES
        || se_packet_mem.free_list->c_count >= EMEM_ALLOCS_PER_CHUNK
#endif /* DEBUG_USE_CANARIES */
        ){
		emem_chunk_t *npc;
		npc=se_packet_mem.free_list;
		se_packet_mem.free_list=se_packet_mem.free_list->next;
		npc->next=se_packet_mem.used_list;
		se_packet_mem.used_list=npc;
	}

	emem_create_chunk(&se_packet_mem.free_list);

	free_list = se_packet_mem.free_list;

	buf = free_list->buf + free_list->free_offset;

	free_list->amount_free -= size;
	free_list->free_offset += size;

#ifdef DEBUG_USE_CANARIES
	cptr = (char *)buf + size - pad;
	memcpy(cptr, &se_canary, pad);
	free_list->canary[free_list->c_count] = cptr;
	free_list->cmp_len[free_list->c_count] = pad;
	free_list->c_count++;
#endif /* DEBUG_USE_CANARIES */

#else /* SE_DEBUG_FREE */
	emem_chunk_t *npc;

	npc=g_malloc(sizeof(emem_chunk_t));
	npc->next=se_packet_mem.used_list;
	npc->amount_free=size;
	npc->free_offset=0;
	npc->buf=g_malloc(size);
	buf = npc->buf;
	se_packet_mem.used_list=npc;
#endif /* SE_DEBUG_FREE */

	return buf;
}


void* ep_alloc0(size_t size) {
	return memset(ep_alloc(size),'\0',size);
}

gchar* ep_strdup(const gchar* src) {
	guint len = strlen(src);
	gchar* dst;

	dst = strncpy(ep_alloc(len+1), src, len);

	dst[len] = '\0';

	return dst;
}

gchar* ep_strndup(const gchar* src, size_t len) {
	gchar* dst = ep_alloc(len+1);
	guint i;

	for (i = 0; src[i] && i < len; i++)
		dst[i] = src[i];

	dst[i] = '\0';

	return dst;
}

void* ep_memdup(const void* src, size_t len) {
	return memcpy(ep_alloc(len), src, len);
}

gchar* ep_strdup_vprintf(const gchar* fmt, va_list ap) {
	va_list ap2;
	guint len;
	gchar* dst;

	G_VA_COPY(ap2, ap);

	len = g_printf_string_upper_bound(fmt, ap);

	dst = ep_alloc(len+1);
	g_vsnprintf (dst, len, fmt, ap2);
	va_end(ap2);

	return dst;
}

gchar* ep_strdup_printf(const gchar* fmt, ...) {
	va_list ap;
	gchar* dst;

	va_start(ap,fmt);
	dst = ep_strdup_vprintf(fmt, ap);
	va_end(ap);
	return dst;
}

gchar** ep_strsplit(const gchar* string, const gchar* sep, int max_tokens) {
	gchar* splitted;
	gchar* s;
	guint tokens;
	guint str_len;
	guint sep_len;
	guint i;
	gchar** vec;
	enum { AT_START, IN_PAD, IN_TOKEN } state;
	guint curr_tok = 0;

	if ( ! string
		 || ! sep
		 || ! sep[0])
		return NULL;

	s = splitted = ep_strdup(string);
	str_len = strlen(splitted);
	sep_len = strlen(sep);

	if (max_tokens < 1) max_tokens = INT_MAX;

	tokens = 1;


	while (tokens <= (guint)max_tokens && ( s = strstr(s,sep) )) {
		tokens++;

		for(i=0; i < sep_len; i++ )
			s[i] = '\0';

		s += sep_len;

	}

	vec = ep_alloc_array(gchar*,tokens+1);
	state = AT_START;

	for (i=0; i< str_len; i++) {
		switch(state) {
			case AT_START:
				switch(splitted[i]) {
					case '\0':
						state  = IN_PAD;
						continue;
					default:
						vec[curr_tok] = &(splitted[i]);
						curr_tok++;
						state = IN_TOKEN;
						continue;
				}
			case IN_TOKEN:
				switch(splitted[i]) {
					case '\0':
						state = IN_PAD;
					default:
						continue;
				}
			case IN_PAD:
				switch(splitted[i]) {
					default:
						vec[curr_tok] = &(splitted[i]);
						curr_tok++;
						state = IN_TOKEN;
					case '\0':
						continue;
				}
		}
	}

	vec[curr_tok] = NULL;

	return vec;
}



void* se_alloc0(size_t size) {
	return memset(se_alloc(size),'\0',size);
}

/* If str is NULL, just return the string "<NULL>" so that the callers dont
 * have to bother checking it.
 */
gchar* se_strdup(const gchar* src) {
	guint len;
	gchar* dst;

	if(!src){
		return "<NULL>";
	}

	len = strlen(src);
	dst = strncpy(se_alloc(len+1), src, len);

	dst[len] = '\0';

	return dst;
}

gchar* se_strndup(const gchar* src, size_t len) {
	gchar* dst = se_alloc(len+1);
	guint i;

	for (i = 0; src[i] && i < len; i++)
		dst[i] = src[i];

	dst[i] = '\0';

	return dst;
}

void* se_memdup(const void* src, size_t len) {
	return memcpy(se_alloc(len), src, len);
}

gchar* se_strdup_vprintf(const gchar* fmt, va_list ap) {
	va_list ap2;
	guint len;
	gchar* dst;

	G_VA_COPY(ap2, ap);

	len = g_printf_string_upper_bound(fmt, ap);

	dst = se_alloc(len+1);
	g_vsnprintf (dst, len, fmt, ap2);
	va_end(ap2);

	return dst;
}

gchar* se_strdup_printf(const gchar* fmt, ...) {
	va_list ap;
	gchar* dst;

	va_start(ap,fmt);
	dst = se_strdup_vprintf(fmt, ap);
	va_end(ap);
	return dst;
}

/* release all allocated memory back to the pool.
 */
void
ep_free_all(void)
{
	emem_chunk_t *npc;
#ifndef EP_DEBUG_FREE
#ifdef DEBUG_USE_CANARIES
	guint i;
#endif /* DEBUG_USE_CANARIES */
#endif

	/* move all used chunks over to the free list */
	while(ep_packet_mem.used_list){
		npc=ep_packet_mem.used_list;
		ep_packet_mem.used_list=ep_packet_mem.used_list->next;
		npc->next=ep_packet_mem.free_list;
		ep_packet_mem.free_list=npc;
	}

	/* clear them all out */
	npc = ep_packet_mem.free_list;
	while (npc != NULL) {
#ifndef EP_DEBUG_FREE
#ifdef DEBUG_USE_CANARIES
		for (i = 0; i < npc->c_count; i++) {
			if (memcmp(npc->canary[i], &ep_canary, npc->cmp_len[i]) != 0)
				g_error("Per-packet memory corrupted.");
		}
		npc->c_count = 0;
#endif /* DEBUG_USE_CANARIES */
		npc->amount_free = npc->amount_free_init;
		npc->free_offset = npc->free_offset_init;
		npc = npc->next;
#else /* EP_DEBUG_FREE */
		emem_chunk_t *next = npc->next;

		g_free(npc->buf);
		g_free(npc);
		npc = next;
#endif /* EP_DEBUG_FREE */
	}

#ifdef EP_DEBUG_FREE
	ep_init_chunk();
#endif
}
/* release all allocated memory back to the pool.
 */
void
se_free_all(void)
{
	emem_chunk_t *npc;
	se_tree_t *se_tree_list;
#ifndef SE_DEBUG_FREE
#ifdef DEBUG_USE_CANARIES
	guint i;
#endif /* DEBUG_USE_CANARIES */
#endif


	/* move all used chunks over to the free list */
	while(se_packet_mem.used_list){
		npc=se_packet_mem.used_list;
		se_packet_mem.used_list=se_packet_mem.used_list->next;
		npc->next=se_packet_mem.free_list;
		se_packet_mem.free_list=npc;
	}

	/* clear them all out */
	npc = se_packet_mem.free_list;
	while (npc != NULL) {
#ifndef SE_DEBUG_FREE
#ifdef DEBUG_USE_CANARIES
		for (i = 0; i < npc->c_count; i++) {
			if (memcmp(npc->canary[i], &se_canary, npc->cmp_len[i]) != 0)
				g_error("Per-session memory corrupted.");
		}
		npc->c_count = 0;
#endif /* DEBUG_USE_CANARIES */
		npc->amount_free = npc->amount_free_init;
		npc->free_offset = npc->free_offset_init;
		npc = npc->next;
#else /* SE_DEBUG_FREE */
		emem_chunk_t *next = npc->next;

		g_free(npc->buf);
		g_free(npc);
		npc = next;
#endif /* SE_DEBUG_FREE */
	}

#ifdef SE_DEBUG_FREE
		se_init_chunk();
#endif

	/* release/reset all se allocated trees */
	for(se_tree_list=se_trees;se_tree_list;se_tree_list=se_tree_list->next){
		se_tree_list->tree=NULL;
	}
}


ep_stack_t ep_stack_new(void) {
    ep_stack_t s = ep_new(struct _ep_stack_frame_t*);
    *s = ep_new0(struct _ep_stack_frame_t);
    return s;
}

/*  for ep_stack_t we'll keep the popped frames so we reuse them instead
of allocating new ones.
*/


void* ep_stack_push(ep_stack_t stack, void* data) {
    struct _ep_stack_frame_t* frame;
    struct _ep_stack_frame_t* head = (*stack);

    if (head->above) {
        frame = head->above;
    } else {
       frame = ep_new(struct _ep_stack_frame_t);
       head->above = frame;
       frame->below = head;
       frame->above = NULL;
    }

    frame->payload = data;
    (*stack) = frame;

    return data;
}

void* ep_stack_pop(ep_stack_t stack) {

    if ((*stack)->below) {
        (*stack) = (*stack)->below;
        return (*stack)->above->payload;
    } else {
        return NULL;
    }
}



#ifdef REMOVED
void print_tree_item(se_tree_node_t *node, int level){
	int i;
	for(i=0;i<level;i++){
		printf("   ");
	}
	printf("%s  KEY:0x%08x node:0x%08x parent:0x%08x left:0x%08x right:0x%08x\n",node->rb_color==SE_TREE_RB_COLOR_BLACK?"BLACK":"RED",node->key32,(int)node,(int)node->parent,(int)node->left,(int)node->right);
	if(node->left)
		print_tree_item(node->left,level+1);
	if(node->right)
		print_tree_item(node->right,level+1);
}

void print_tree(se_tree_node_t *node){
	if(!node){
		return;
	}
	while(node->parent){
		node=node->parent;
	}
	print_tree_item(node,0);
}
#endif



/* routines to manage se allocated red-black trees */
se_tree_t *se_trees=NULL;

se_tree_t *
se_tree_create(int type, char *name)
{
	se_tree_t *tree_list;

	tree_list=malloc(sizeof(se_tree_t));
	tree_list->next=se_trees;
	tree_list->type=type;
	tree_list->tree=NULL;
	tree_list->name=name;
	se_trees=tree_list;

	return tree_list;
}



void *
se_tree_lookup32(se_tree_t *se_tree, guint32 key)
{
	se_tree_node_t *node;

	node=se_tree->tree;

	while(node){
		if(key==node->key32){
			return node->data;
		}
		if(key<node->key32){
			node=node->left;
			continue;
		}
		if(key>node->key32){
			node=node->right;
			continue;
		}
	}
	return NULL;
}


static inline se_tree_node_t *
se_tree_uncle(se_tree_node_t *parent, se_tree_node_t *grandparent)
{
	if(parent==grandparent->left){
		return grandparent->right;
	}
	return grandparent->left;
}

static inline void rb_insert_case1(se_tree_t *se_tree, se_tree_node_t *node);
static inline void rb_insert_case2(se_tree_t *se_tree, se_tree_node_t *node, se_tree_node_t *parent);

static inline void
rotate_left(se_tree_t *se_tree, se_tree_node_t *node)
{
	if(node->parent){
		if(node->parent->left==node){
			node->parent->left=node->right;
		} else {
			node->parent->right=node->right;
		}
	} else {
		se_tree->tree=node->right;
	}
	node->right->parent=node->parent;
	node->parent=node->right;
	node->right=node->right->left;
	if(node->right){
		node->right->parent=node;
	}
	node->parent->left=node;
}

static inline void
rotate_right(se_tree_t *se_tree, se_tree_node_t *node)
{
	if(node->parent){
		if(node->parent->left==node){
			node->parent->left=node->left;
		} else {
			node->parent->right=node->left;
		}
	} else {
		se_tree->tree=node->left;
	}
	node->left->parent=node->parent;
	node->parent=node->left;
	node->left=node->left->right;
	if(node->left){
		node->left->parent=node;
	}
	node->parent->right=node;
}

static inline void
rb_insert_case5(se_tree_t *se_tree, se_tree_node_t *node, se_tree_node_t *parent, se_tree_node_t *grandparent)
{
	parent->rb_color=SE_TREE_RB_COLOR_BLACK;
	grandparent->rb_color=SE_TREE_RB_COLOR_RED;
	if( (node==parent->left) && (parent==grandparent->left) ){
		rotate_right(se_tree, grandparent);
	} else {
		rotate_left(se_tree, grandparent);
	}
}

static inline void
rb_insert_case4(se_tree_t *se_tree, se_tree_node_t *node, se_tree_node_t *parent, se_tree_node_t *grandparent)
{
	if( (node==parent->right) && (parent==grandparent->left) ){
		rotate_left(se_tree, parent);
		node=node->left;
	} else if( (node==parent->left) && (parent==grandparent->right) ){
		rotate_right(se_tree, parent);
		node=node->right;
	}
	rb_insert_case5(se_tree, node, parent, grandparent);
}

/* parent is guaranteed to be non-NULL */
static inline void
rb_insert_case3(se_tree_t *se_tree, se_tree_node_t *node, se_tree_node_t *parent)
{
	se_tree_node_t *grandparent;
	se_tree_node_t *uncle;

	grandparent=parent->parent;
	uncle=se_tree_uncle(parent, grandparent);
	if(uncle && (uncle->rb_color==SE_TREE_RB_COLOR_RED)){
		parent->rb_color=SE_TREE_RB_COLOR_BLACK;
		uncle->rb_color=SE_TREE_RB_COLOR_BLACK;
		grandparent->rb_color=SE_TREE_RB_COLOR_RED;
		rb_insert_case1(se_tree, grandparent);
	} else {
		rb_insert_case4(se_tree, node, parent, grandparent);
	}
}

/* parent is guaranteed to be non-NULL */
static inline void
rb_insert_case2(se_tree_t *se_tree, se_tree_node_t *node, se_tree_node_t *parent)
{
	if(parent->rb_color==SE_TREE_RB_COLOR_BLACK){
		return;
	}
	rb_insert_case3(se_tree, node, parent);
}

static inline void
rb_insert_case1(se_tree_t *se_tree, se_tree_node_t *node)
{
	se_tree_node_t *parent;

	parent=node->parent;
	if(!parent){
		node->rb_color=SE_TREE_RB_COLOR_BLACK;
		return;
	}
	rb_insert_case2(se_tree, node, parent);
}

/* insert a new node in the tree. if this node matches an already existing node
 * then just replace the data for that node */
void
se_tree_insert32(se_tree_t *se_tree, guint32 key, void *data)
{
	se_tree_node_t *node;

	node=se_tree->tree;

	/* is this the first node ?*/
	if(!node){
		node=se_alloc(sizeof(se_tree_node_t));
		switch(se_tree->type){
		case SE_TREE_TYPE_RED_BLACK:
			node->rb_color=SE_TREE_RB_COLOR_BLACK;
			break;
		}
		node->parent=NULL;
		node->left=NULL;
		node->right=NULL;
		node->key32=key;
		node->data=data;
		se_tree->tree=node;
		return;
	}

	/* it was not the new root so walk the tree until we find where to
	 * insert this new leaf.
	 */
	while(1){
		/* this node already exists, so just replace the data pointer*/
		if(key==node->key32){
			node->data=data;
			return;
		}
		if(key<node->key32) {
			if(!node->left){
				/* new node to the left */
				se_tree_node_t *new_node;
				new_node=se_alloc(sizeof(se_tree_node_t));
				node->left=new_node;
				new_node->parent=node;
				new_node->left=NULL;
				new_node->right=NULL;
				new_node->key32=key;
				new_node->data=data;
				node=new_node;
				break;
			}
			node=node->left;
			continue;
		}
		if(key>node->key32) {
			if(!node->right){
				/* new node to the right */
				se_tree_node_t *new_node;
				new_node=se_alloc(sizeof(se_tree_node_t));
				node->right=new_node;
				new_node->parent=node;
				new_node->left=NULL;
				new_node->right=NULL;
				new_node->key32=key;
				new_node->data=data;
				node=new_node;
				break;
			}
			node=node->right;
			continue;
		}
	}

	/* node will now point to the newly created node */
	switch(se_tree->type){
	case SE_TREE_TYPE_RED_BLACK:
		node->rb_color=SE_TREE_RB_COLOR_RED;
		rb_insert_case1(se_tree, node);
		break;
	}
}

static void* lookup_or_insert32(se_tree_t *se_tree, guint32 key, void*(*func)(void*),void* ud) {
	se_tree_node_t *node;
	
	node=se_tree->tree;
	
	/* is this the first node ?*/
	if(!node){
		node=se_alloc(sizeof(se_tree_node_t));
		switch(se_tree->type){
			case SE_TREE_TYPE_RED_BLACK:
				node->rb_color=SE_TREE_RB_COLOR_BLACK;
				break;
		}
		node->parent=NULL;
		node->left=NULL;
		node->right=NULL;
		node->key32=key;
		node->data= func(ud);
		se_tree->tree=node;
		return node->data;
	}
	
	/* it was not the new root so walk the tree until we find where to
		* insert this new leaf.
		*/
	while(1){
		/* this node already exists, so just return the data pointer*/
		if(key==node->key32){
			return node->data;
		}
		if(key<node->key32) {
			if(!node->left){
				/* new node to the left */
				se_tree_node_t *new_node;
				new_node=se_alloc(sizeof(se_tree_node_t));
				node->left=new_node;
				new_node->parent=node;
				new_node->left=NULL;
				new_node->right=NULL;
				new_node->key32=key;
				new_node->data= func(ud);
				node=new_node;
				break;
			}
			node=node->left;
			continue;
		}
		if(key>node->key32) {
			if(!node->right){
				/* new node to the right */
				se_tree_node_t *new_node;
				new_node=se_alloc(sizeof(se_tree_node_t));
				node->right=new_node;
				new_node->parent=node;
				new_node->left=NULL;
				new_node->right=NULL;
				new_node->key32=key;
				new_node->data= func(ud);
				node=new_node;
				break;
			}
			node=node->right;
			continue;
		}
	}
	
	/* node will now point to the newly created node */
	switch(se_tree->type){
		case SE_TREE_TYPE_RED_BLACK:
			node->rb_color=SE_TREE_RB_COLOR_RED;
			rb_insert_case1(se_tree, node);
			break;
	}
	
	return node->data;
}

/* When the se data is released, this entire tree will dissapear as if it
 * never existed including all metadata associated with the tree.
 */
se_tree_t *
se_tree_create_non_persistent(int type, char *name)
{
	se_tree_t *tree_list;

	tree_list=se_alloc(sizeof(se_tree_t));
	tree_list->next=NULL;
	tree_list->type=type;
	tree_list->tree=NULL;
	tree_list->name=name;

	return tree_list;
}

static void* create_sub_tree(void* d) {
	se_tree_t *se_tree = d;
	return se_tree_create_non_persistent(se_tree->type, "subtree");
}

/* insert a new node in the tree. if this node matches an already existing node
 * then just replace the data for that node */

void
se_tree_insert32_array(se_tree_t *se_tree, se_tree_key_t *key, void *data)
{
	se_tree_t *next_tree;

	if((key[0].length<1)||(key[0].length>100)){
	        DISSECTOR_ASSERT_NOT_REACHED();
	}
	if((key[0].length==1)&&(key[1].length==0)){
		se_tree_insert32(se_tree, *key[0].key, data);
		return;
	}
	
	next_tree=lookup_or_insert32(se_tree, *key[0].key, create_sub_tree, se_tree);

	if(key[0].length==1){
		key++;
	} else {
		key[0].length--;
		key[0].key++;
	}
	se_tree_insert32_array(next_tree, key, data);
}

void *
se_tree_lookup32_array(se_tree_t *se_tree, se_tree_key_t *key)
{
	se_tree_t *next_tree;

	if((key[0].length<1)||(key[0].length>100)){
	        DISSECTOR_ASSERT_NOT_REACHED();
	}
	if((key[0].length==1)&&(key[1].length==0)){
		return se_tree_lookup32(se_tree, *key[0].key);
	}
	next_tree=se_tree_lookup32(se_tree, *key[0].key);
	if(!next_tree){
		return NULL;
	}
	if(key[0].length==1){
		key++;
	} else {
		key[0].length--;
		key[0].key++;
	}
	return se_tree_lookup32_array(next_tree, key);
}


void se_tree_insert_string(se_string_hash_t* se_tree, const gchar* k, void* v) {
	guint32 len = strlen(k);
	guint32 div = (len-1)/4;
	guint32 residual = 0;
	se_tree_key_t key[] = {
		{1,&len},
		{div,(guint32*)(&k[0])},
		{1,&residual},
		{0,NULL}
	};

	if (! div) {
		key[1].length = key[2].length;
		key[1].key = key[2].key;
		key[2].length = 0;
		key[2].key = NULL;
	}

	div *= 4;

	switch(len%4) {
		case 0:
			residual |= ( k[div+3] << 24 );
		case 3:
			residual |= ( k[div+2] << 16 );
		case 2:
			residual |= ( k[div+1] << 8  );
		case 1:
			residual |= k[div];
			break;
	}

	se_tree_insert32_array(se_tree,key,v);
}

void* se_tree_lookup_string(se_string_hash_t* se_tree, const gchar* k) {
	guint32 len = strlen(k);
	guint32 div = (len-1)/4;
	guint32 residual = 0;
	se_tree_key_t key[] = {
		{1,&len},
		{div,(guint32*)(&k[0])},
		{1,&residual},
		{0,NULL}
	};

	if (! div) {
		key[1].length = key[2].length;
		key[1].key = key[2].key;
		key[2].length = 0;
		key[2].key = NULL;
	}

	div *= 4;

	switch(len%4) {
		case 0:
			residual |= k[div+3] << 24;
		case 3:
			residual |= k[div+2] << 16;
		case 2:
			residual |= k[div+1] << 8;
		case 1:
			residual |= k[div];
			break;
	}

	return se_tree_lookup32_array(se_tree, key);
}
