/* packet-ssl-utils.c
 *
 * $Id$
 *
 * ssl manipulation functions
 * By Paolo Abeni <paolo.abeni@email.com>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "packet-ssl-utils.h"

void 
ssl_data_set(StringInfo* str, unsigned char* data, unsigned int len)
{
    memcpy(str->data, data, len);
    str->data_len = len;
}

#ifdef HAVE_LIBGNUTLS

/* hmac abstraction layer */
#define SSL_HMAC gcry_md_hd_t

static inline void 
ssl_hmac_init(SSL_HMAC* md, const void * key, int len, int algo)
{
    gcry_md_open(md,algo, GCRY_MD_FLAG_HMAC); 
    gcry_md_setkey (*(md), key, len);
}
static inline void 
ssl_hmac_update(SSL_HMAC* md, const void* data, int len)
{
    gcry_md_write(*(md), data, len);
}
static inline void 
ssl_hmac_final(SSL_HMAC* md, unsigned char* data, unsigned int* datalen)
{ 
    int algo = gcry_md_get_algo (*(md));
    unsigned int len = gcry_md_get_algo_dlen(algo);
    memcpy(data, gcry_md_read(*(md), algo), len);
    *datalen =len;
}
static inline void 
ssl_hmac_cleanup(SSL_HMAC* md) 
{ 
    gcry_md_close(*(md)); 
}

/* memory digest abstraction layer*/
#define SSL_MD gcry_md_hd_t

static inline void 
ssl_md_init(SSL_MD* md, int algo)
{
    gcry_md_open(md,algo, 0); 
}
static inline void 
ssl_md_update(SSL_MD* md, unsigned char* data, int len) 
{ 
    gcry_md_write(*(md), data, len); 
}
static inline void 
ssl_md_final(SSL_MD* md, unsigned char* data, unsigned int* datalen)
{ 
    int algo = gcry_md_get_algo (*(md));
    int len = gcry_md_get_algo_dlen (algo);
    memcpy(data, gcry_md_read(*(md),  algo), len);
    *datalen = len;
}
static inline void 
ssl_md_cleanup(SSL_MD* md) 
{ 
    gcry_md_close(*(md)); 
}

/* md5 /sha abstraction layer */
#define SSL_SHA_CTX gcry_md_hd_t
#define SSL_MD5_CTX gcry_md_hd_t

static inline void 
ssl_sha_init(SSL_SHA_CTX* md)
{
    gcry_md_open(md,GCRY_MD_SHA1, 0); 
}
static inline void 
ssl_sha_update(SSL_SHA_CTX* md, unsigned char* data, int len) 
{ 
    gcry_md_write(*(md), data, len);
}
static inline void 
ssl_sha_final(unsigned char* buf, SSL_SHA_CTX* md)
{
    memcpy(buf, gcry_md_read(*(md),  GCRY_MD_SHA1), 
        gcry_md_get_algo_dlen(GCRY_MD_SHA1));
}
static inline void 
ssl_sha_cleanup(SSL_SHA_CTX* md)
{
    gcry_md_close(*(md));
}

static inline int 
ssl_md5_init(SSL_MD5_CTX* md)
{
    return gcry_md_open(md,GCRY_MD_MD5, 0); 
}
static inline void 
ssl_md5_update(SSL_MD5_CTX* md, unsigned char* data, int len)
{
    gcry_md_write(*(md), data, len);
}
static inline void 
ssl_md5_final(unsigned char* buf, SSL_MD5_CTX* md)
{
    memcpy(buf, gcry_md_read(*(md),  GCRY_MD_MD5), 
        gcry_md_get_algo_dlen(GCRY_MD_MD5));
}
static inline void
ssl_md5_cleanup(SSL_MD5_CTX* md)
{
    gcry_md_close(*(md));
}

static int
ssl_cipher_setiv(gcry_cipher_hd_t *cipher,unsigned char* iv, int iv_len)
{
  unsigned char * ivp;
  int ret=0;
  int i;
  gcry_cipher_hd_t c;
  c=(gcry_cipher_hd_t)*cipher;
  
  ssl_debug_printf("--------------------------------------------------------------------");
  /*for(ivp=c->iv,i=0; i < iv_len; i++ )
    {
      ssl_debug_printf("%d ",ivp[i]);
      i++;
    }
  */
  ssl_debug_printf("--------------------------------------------------------------------");
  ret = gcry_cipher_setiv(*(cipher), iv, iv_len);
  /*for(ivp=c->iv,i=0; i < iv_len; i++ )
    {
      ssl_debug_printf("%d ",ivp[i]);
      i++;
    }
  */
  ssl_debug_printf("--------------------------------------------------------------------");
  return ret;
}
/* stream cipher abstraction layer*/
static int 
ssl_cipher_init(gcry_cipher_hd_t *cipher, int algo, unsigned char* sk, 
        unsigned char* iv, int mode)
{
    int gcry_modes[]={
        GCRY_CIPHER_MODE_STREAM,
        GCRY_CIPHER_MODE_CBC
    };
    int err = gcry_cipher_open(cipher, algo, gcry_modes[mode], 0); 
    if (err !=0)
        return  -1;
    err = gcry_cipher_setkey(*(cipher), sk, gcry_cipher_get_algo_keylen (algo)); 
    if (err != 0)
        return -1;
    err = gcry_cipher_setiv(*(cipher), iv, gcry_cipher_get_algo_blklen (algo));
    if (err != 0)
        return -1;
    return 0;
}
static inline int 
ssl_cipher_decrypt(gcry_cipher_hd_t *cipher, unsigned char * out, int outl, 
                   const unsigned char * in,int inl)
{
    return gcry_cipher_decrypt ( *(cipher), out, outl, in, inl);
}
static inline int 
ssl_get_digest_by_name(const char*name)
{
    return gcry_md_map_name(name);
}
static inline int 
ssl_get_cipher_by_name(const char* name)
{
    return gcry_cipher_map_name(name);
}

static inline void
ssl_cipher_cleanup(gcry_cipher_hd_t *cipher)
{
    gcry_cipher_close(*cipher);
    *cipher = NULL;
}

/* private key abstraction layer */
static inline int 
ssl_get_key_len(SSL_PRIVATE_KEY* pk) {return gcry_pk_get_nbits (pk); }

gcry_err_code_t
_gcry_rsa_decrypt (int algo, gcry_mpi_t *result, gcry_mpi_t *data,
                   gcry_mpi_t *skey, int flags);
                   
#define PUBKEY_FLAG_NO_BLINDING (1 << 0) 

/* decrypt data with private key. Store decrypted data directly into input
 * buffer */
int 
ssl_private_decrypt(unsigned int len, unsigned char* encr_data, SSL_PRIVATE_KEY* pk)
{
    int rc;
    size_t decr_len = 0;
    gcry_sexp_t  s_data, s_plain;
    gcry_mpi_t encr_mpi;
    size_t i, encr_len = len;
    unsigned char* decr_data_ptr;
    gcry_mpi_t text=NULL;

    /* build up a mpi rappresentation for encrypted data */
    rc = gcry_mpi_scan(&encr_mpi, GCRYMPI_FMT_USG,encr_data, encr_len, &encr_len); 
    if (rc != 0 ) {
        ssl_debug_printf("pcry_private_decrypt: can't convert encr_data to mpi (size %d):%s\n", 
            len, gcry_strerror(rc));
        return 0;
    }

#ifndef SSL_FAST    
    /* put the data into a simple list */
    rc = gcry_sexp_build(&s_data, NULL, "(enc-val(rsa(a%m)))", encr_mpi);
    if (rc != 0) {
        ssl_debug_printf("pcry_private_decrypt: can't build encr_sexp:%s \n",
             gcry_strerror(rc));
        return 0;
    }

    /* pass it to libgcrypt */
    rc = gcry_pk_decrypt(&s_plain, s_data, pk);
    gcry_sexp_release(s_data);
    if (rc != 0)
    {
        ssl_debug_printf("pcry_private_decrypt: can't decrypt key:%s\n", 
            gcry_strerror(rc));
        goto out;
    }    
    
    /* convert plain text sexp to mpi format */
    text = gcry_sexp_nth_mpi(s_plain, 0, 0);
    
    /* compute size requested for plaintext buffer */
    decr_len = len;
    if (gcry_mpi_print(GCRYMPI_FMT_USG, NULL, decr_len, &decr_len, text) != 0) {
        ssl_debug_printf("pcry_private_decrypt: can't compute decr size:%s\n",
            gcry_strerror(rc));
        decr_len = 0;
        goto out;
    }
    
    /* sanity check on out buffer */
    if (decr_len > len) {
        ssl_debug_printf("pcry_private_decrypt: decrypted data is too long ?!? (%d max %d)\n",
            decr_len, len);
        return 0;
    }

    /* write plain text to encrypted data buffer */
    decr_data_ptr = encr_data;
    if (gcry_mpi_print( GCRYMPI_FMT_USG, decr_data_ptr, decr_len, &decr_len, 
            text) != 0) {
        ssl_debug_printf("pcry_private_decrypt: can't print decr data to mpi (size %d):%s\n", 
            decr_len, gcry_strerror(rc));
        g_free(decr_data_ptr);
        decr_len = 0;
        goto out;
    }
    
    /* strip the padding*/
    rc = 0;
    for (i = 1; i < decr_len; i++) {
        if (decr_data_ptr[i] == 0) {
            rc = i+1;
            break;
        }
    }
    
    ssl_debug_printf("pcry_private_decrypt: stripping %d bytes, decr_len %d\n", 
        rc, decr_len);
    ssl_print_data("decypted_unstrip_pre_master", decr_data_ptr, decr_len);
    g_memmove(decr_data_ptr, &decr_data_ptr[rc], decr_len - rc);
    decr_len -= rc;

out:        
    gcry_sexp_release(s_plain);
#else /* SSL_FAST */
    rc = _gcry_rsa_decrypt(0, &text,  &encr_mpi, pk,0);
    gcry_mpi_print( GCRYMPI_FMT_USG, 0, 0, &decr_len, text);
    
    /* sanity check on out buffer */
    if (decr_len > len) {
        ssl_debug_printf("pcry_private_decrypt: decrypted data is too long ?!? (%d max %d)\n",
            decr_len, len);
        return 0;
    }
    
    /* write plain text to newly allocated buffer */
    decr_data_ptr = encr_data;
    if (gcry_mpi_print( GCRYMPI_FMT_USG, decr_data_ptr, decr_len, &decr_len, 
            text) != 0) {
        ssl_debug_printf("pcry_private_decrypt: can't print decr data to mpi (size %d):%s\n", 
            decr_len, gcry_strerror(rc));
        return 0;
    }
    
    /* strip the padding*/
    rc = 0;
    for (i = 1; i < decr_len; i++) {
        if (decr_data_ptr[i] == 0) {
            rc = i+1;
            break;
        }
    }
    
    ssl_debug_printf("pcry_private_decrypt: stripping %d bytes, decr_len %d\n", 
        rc, decr_len);
    ssl_print_data("decypted_unstrip_pre_master", decr_data_ptr, decr_len);
    g_memmove(decr_data_ptr, &decr_data_ptr[rc], decr_len - rc);
    decr_len -= rc;
#endif /* SSL_FAST */
    gcry_mpi_release(text);
    return decr_len;
}

/* stringinfo interface */
static int 
ssl_data_alloc(StringInfo* str, unsigned int len)
{
    str->data = g_malloc(len);
    if (!str->data)
        return -1;
    str->data_len = len;
    return 0;
}

#define PRF(ssl,secret,usage,rnd1,rnd2,out) ((ssl->version_netorder==SSLV3_VERSION)? \
        ssl3_prf(secret,usage,rnd1,rnd2,out): \
        tls_prf(secret,usage,rnd1,rnd2,out))

static const char *digests[]={
     "MD5",
     "SHA1"
};

static const char *ciphers[]={
     "DES",
     "3DES",
     "ARCFOUR", /* gnutls does not support rc4, but this should be 100% compatible*/
     "RC2",
     "IDEA",
     "AES",
     "AES256",
     "*UNKNOWN*"
};

/* look in openssl/ssl/ssl_lib.c for a complete list of available cipersuite*/
static SslCipherSuite cipher_suites[]={
    {1,KEX_RSA,SIG_RSA,ENC_NULL,0,0,0,DIG_MD5,16,0, SSL_CIPHER_MODE_STREAM},
    {2,KEX_RSA,SIG_RSA,ENC_NULL,0,0,0,DIG_SHA,20,0, SSL_CIPHER_MODE_STREAM},
    {3,KEX_RSA,SIG_RSA,ENC_RC4,1,128,40,DIG_MD5,16,1, SSL_CIPHER_MODE_STREAM},
    {4,KEX_RSA,SIG_RSA,ENC_RC4,1,128,128,DIG_MD5,16,0, SSL_CIPHER_MODE_STREAM},
    {5,KEX_RSA,SIG_RSA,ENC_RC4,1,128,128,DIG_SHA,20,0, SSL_CIPHER_MODE_STREAM},
    {6,KEX_RSA,SIG_RSA,ENC_RC2,8,128,40,DIG_SHA,20,1, SSL_CIPHER_MODE_STREAM},
    {7,KEX_RSA,SIG_RSA,ENC_IDEA,8,128,128,DIG_SHA,20,0, SSL_CIPHER_MODE_STREAM},
    {8,KEX_RSA,SIG_RSA,ENC_DES,8,64,40,DIG_SHA,20,1, SSL_CIPHER_MODE_CBC},
    {9,KEX_RSA,SIG_RSA,ENC_DES,8,64,64,DIG_SHA,20,0, SSL_CIPHER_MODE_CBC},
    {10,KEX_RSA,SIG_RSA,ENC_3DES,8,192,192,DIG_SHA,20,0, SSL_CIPHER_MODE_CBC},
    {11,KEX_DH,SIG_DSS,ENC_DES,8,64,40,DIG_SHA,20,1, SSL_CIPHER_MODE_STREAM},
    {12,KEX_DH,SIG_DSS,ENC_DES,8,64,64,DIG_SHA,20,0, SSL_CIPHER_MODE_STREAM},
    {13,KEX_DH,SIG_DSS,ENC_3DES,8,192,192,DIG_SHA,20,0, SSL_CIPHER_MODE_STREAM},
    {14,KEX_DH,SIG_RSA,ENC_DES,8,64,40,DIG_SHA,20,1, SSL_CIPHER_MODE_STREAM},
    {15,KEX_DH,SIG_RSA,ENC_DES,8,64,64,DIG_SHA,20,0, SSL_CIPHER_MODE_STREAM},
    {16,KEX_DH,SIG_RSA,ENC_3DES,8,192,192,DIG_SHA,20,0, SSL_CIPHER_MODE_STREAM},
    {17,KEX_DH,SIG_DSS,ENC_DES,8,64,40,DIG_SHA,20,1, SSL_CIPHER_MODE_STREAM},
    {18,KEX_DH,SIG_DSS,ENC_DES,8,64,64,DIG_SHA,20,0, SSL_CIPHER_MODE_STREAM},
    {19,KEX_DH,SIG_DSS,ENC_3DES,8,192,192,DIG_SHA,20,0, SSL_CIPHER_MODE_STREAM},
    {20,KEX_DH,SIG_RSA,ENC_DES,8,64,40,DIG_SHA,20,1, SSL_CIPHER_MODE_STREAM},
    {21,KEX_DH,SIG_RSA,ENC_DES,8,64,64,DIG_SHA,20,0, SSL_CIPHER_MODE_STREAM},
    {22,KEX_DH,SIG_RSA,ENC_3DES,8,192,192,DIG_SHA,20,0, SSL_CIPHER_MODE_STREAM},
    {23,KEX_DH,SIG_NONE,ENC_RC4,1,128,40,DIG_MD5,16,1, SSL_CIPHER_MODE_STREAM},
    {24,KEX_DH,SIG_NONE,ENC_RC4,1,128,128,DIG_MD5,16,0, SSL_CIPHER_MODE_STREAM},
    {25,KEX_DH,SIG_NONE,ENC_DES,8,64,40,DIG_MD5,16,1, SSL_CIPHER_MODE_STREAM},
    {26,KEX_DH,SIG_NONE,ENC_DES,8,64,64,DIG_MD5,16,0, SSL_CIPHER_MODE_STREAM},
    {27,KEX_DH,SIG_NONE,ENC_3DES,8,192,192,DIG_MD5,16,0, SSL_CIPHER_MODE_STREAM},
    {47,KEX_RSA,SIG_RSA,ENC_AES,16,128,128,DIG_SHA,20,0, SSL_CIPHER_MODE_CBC},
    {53,KEX_RSA,SIG_RSA,ENC_AES256,16,256,256,DIG_SHA,20,0, SSL_CIPHER_MODE_CBC},
    {96,KEX_RSA,SIG_RSA,ENC_RC4,1,128,56,DIG_MD5,16,1, SSL_CIPHER_MODE_STREAM},
    {97,KEX_RSA,SIG_RSA,ENC_RC2,1,128,56,DIG_MD5,16,1, SSL_CIPHER_MODE_STREAM},
    {98,KEX_RSA,SIG_RSA,ENC_DES,8,64,64,DIG_SHA,20,1, SSL_CIPHER_MODE_STREAM},
    {99,KEX_DH,SIG_DSS,ENC_DES,8,64,64,DIG_SHA,16,1, SSL_CIPHER_MODE_STREAM},
    {100,KEX_RSA,SIG_RSA,ENC_RC4,1,128,56,DIG_SHA,20,1, SSL_CIPHER_MODE_STREAM},
    {101,KEX_DH,SIG_DSS,ENC_RC4,1,128,56,DIG_SHA,20,1, SSL_CIPHER_MODE_STREAM},
    {102,KEX_DH,SIG_DSS,ENC_RC4,1,128,128,DIG_SHA,20,0, SSL_CIPHER_MODE_STREAM},
    {-1, 0,0,0,0,0,0,0,0,0, 0}
};

#define MAX_BLOCK_SIZE 16
#define MAX_KEY_SIZE 32

int 
ssl_find_cipher(int num,SslCipherSuite* cs)
{
    SslCipherSuite *c;
    
    for(c=cipher_suites;c->number!=-1;c++){
        if(c->number==num){
            *cs=*c;
            return 0;
        }
    }
    
    return -1;
}

static int 
tls_hash(StringInfo* secret,
        StringInfo* seed, int md, StringInfo* out)
{
    guint8 *ptr=out->data;
    unsigned int left=out->data_len;
    int tocpy;
    guint8 *A;
    guint8 _A[20],tmp[20];
    unsigned int A_l,tmp_l;
    SSL_HMAC hm;
    
    ssl_print_string("tls_hash: hash secret", secret);
    ssl_print_string("tls_hash: hash seed", seed);
    A=seed->data;
    A_l=seed->data_len;
    
    while(left){
        ssl_hmac_init(&hm,secret->data,secret->data_len,md);
        ssl_hmac_update(&hm,A,A_l);
        ssl_hmac_final(&hm,_A,&A_l);
        ssl_hmac_cleanup(&hm);
        A=_A;
        
        ssl_hmac_init(&hm,secret->data,secret->data_len,md);
        ssl_hmac_update(&hm,A,A_l);
        ssl_hmac_update(&hm,seed->data,seed->data_len);
        ssl_hmac_final(&hm,tmp,&tmp_l);
        ssl_hmac_cleanup(&hm);
        
        tocpy=MIN(left,tmp_l);
        memcpy(ptr,tmp,tocpy);
        ptr+=tocpy;
        left-=tocpy;
    }
    
    ssl_print_string("hash out", out);
    return (0);
}    

static int 
tls_prf(StringInfo* secret, const char *usage,
        StringInfo* rnd1, StringInfo* rnd2, StringInfo* out)
{
    StringInfo seed, sha_out, md5_out;
    guint8 *ptr;
    StringInfo s1, s2;
    unsigned int i,s_l, r=-1;
    int usage_len = strlen(usage);
    
    /* initalize buffer for sha, md5 random seed*/
    if (ssl_data_alloc(&sha_out, MAX(out->data_len,20)) < 0)
        return -1;
    if (ssl_data_alloc(&md5_out, MAX(out->data_len,16)) < 0)
        goto free_sha;
    if (ssl_data_alloc(&seed, usage_len+rnd1->data_len+rnd2->data_len) < 0)
        goto free_md5;

    ptr=seed.data;
    memcpy(ptr,usage,usage_len); ptr+=usage_len;
    memcpy(ptr,rnd1->data,rnd1->data_len); ptr+=rnd1->data_len;
    memcpy(ptr,rnd2->data,rnd2->data_len); ptr+=rnd2->data_len;    
    
    /* initalize buffer for client/server seeds*/
    s_l=secret->data_len/2 + secret->data_len%2;
    if (ssl_data_alloc(&s1, s_l) < 0)
        goto free_seed;
    if (ssl_data_alloc(&s2, s_l) < 0)
        goto free_s1;
    
    memcpy(s1.data,secret->data,s_l);
    memcpy(s2.data,secret->data + (secret->data_len - s_l),s_l);

    ssl_debug_printf("tls_prf: tls_hash(md5 secret_len %d seed_len %d )\n", s1.data_len, seed.data_len);
    if(tls_hash(&s1,&seed,ssl_get_digest_by_name("MD5"),&md5_out) != 0)
        goto free_all;
    ssl_debug_printf("tls_prf: tls_hash(sha)\n");
    if(tls_hash(&s2,&seed,ssl_get_digest_by_name("SHA1"),&sha_out) != 0)
        goto free_all;
    
    for(i=0;i<out->data_len;i++)
      out->data[i]=md5_out.data[i] ^ sha_out.data[i];
    r =0;
    
    ssl_print_string("PRF out",out);
free_all:    
    free(s2.data);
free_s1:    
    free(s1.data);
free_seed:    
    free(seed.data);
free_md5:
    free(md5_out.data);    
free_sha:
    free(sha_out.data);
    return r;    
}

static int 
ssl3_generate_export_iv(StringInfo* r1,
        StringInfo* r2, StringInfo* out)
{
    SSL_MD5_CTX md5;
    guint8 tmp[16];
    
    ssl_md5_init(&md5);
    ssl_md5_update(&md5,r1->data,r1->data_len);
    ssl_md5_update(&md5,r2->data,r2->data_len);
    ssl_md5_final(tmp,&md5);
    ssl_md5_cleanup(&md5);
    
    memcpy(out->data,tmp,out->data_len);
    ssl_print_string("export iv", out);
    
    return(0);
}

static int 
ssl3_prf(StringInfo* secret, const char* usage,
        StringInfo* r1,
        StringInfo* r2,StringInfo* out)
{
    SSL_MD5_CTX md5;
    SSL_SHA_CTX sha;
    StringInfo *rnd1,*rnd2;
    unsigned int off;
    int i=0,j;
    guint8 buf[20];
    
    rnd1=r1; rnd2=r2;
    
    for(off=0;off<out->data_len;off+=16){
        unsigned char outbuf[16];
        int tocpy;
        i++;
        
        ssl_debug_printf("ssl3_prf: sha1_hash(%d)\n",i);
        /* A, BB, CCC,  ... */
        for(j=0;j<i;j++){
            buf[j]=64+i;
        }
        
        ssl_sha_init(&sha);
        ssl_sha_update(&sha,buf,i);
        ssl_sha_update(&sha,secret->data,secret->data_len);
        
        if(!strcmp(usage,"client write key") || !strcmp(usage,"server write key")){
            ssl_sha_update(&sha,rnd2->data,rnd2->data_len);
            ssl_sha_update(&sha,rnd1->data,rnd1->data_len);
        }
        else{
            ssl_sha_update(&sha,rnd1->data,rnd1->data_len);
            ssl_sha_update(&sha,rnd2->data,rnd2->data_len);
        }
        
        ssl_sha_final(buf,&sha);
        ssl_sha_cleanup(&sha);
        
        ssl_debug_printf("ssl3_prf: md5_hash(%d) datalen %d\n",i, 
            secret->data_len);
        ssl_md5_init(&md5);
        ssl_md5_update(&md5,secret->data,secret->data_len);
        ssl_md5_update(&md5,buf,20);
        ssl_md5_final(outbuf,&md5);
        ssl_md5_cleanup(&md5);
        
        tocpy=MIN(out->data_len-off,16);
        memcpy(out->data+off,outbuf,tocpy);
    }
    
    return(0);
}

static int 
ssl_create_decoder(SslDecoder *dec, SslCipherSuite *cipher_suite, 
        guint8 *mk, guint8 *sk, guint8 *iv)
{
    int ciph=0;

    /* Find the SSLeay cipher */
    if(cipher_suite->enc!=ENC_NULL) {
        ssl_debug_printf("ssl_create_decoder CIPHER: %s\n", ciphers[cipher_suite->enc-0x30]);
        ciph=ssl_get_cipher_by_name(ciphers[cipher_suite->enc-0x30]);
    }
    if (ciph == 0) {
        ssl_debug_printf("ssl_create_decoder can't find cipher %s\n", 
            ciphers[(cipher_suite->enc-0x30) > 7 ? 7 : (cipher_suite->enc-0x30)]);
        return -1;
    }
    
    /* init mac buffer: mac storage is embedded into decoder struct to save a
     memory allocation and waste samo more memory*/
    dec->cipher_suite=cipher_suite;
    dec->mac_key.data = dec->_mac_key;
    ssl_data_set(&dec->mac_key, mk, cipher_suite->dig_len);
    dec->seq = 0;
    
    if (dec->evp)
        ssl_cipher_cleanup(&dec->evp);

    if (ssl_cipher_init(&dec->evp,ciph,sk,iv,cipher_suite->mode) < 0) {
        ssl_debug_printf("ssl_create_decoder: can't create cipher id:%d mode:%d\n",
            ciph, cipher_suite->mode);
        return -1;
    }

    ssl_debug_printf("decoder initialized (digest len %d)\n", cipher_suite->dig_len);
    return 0;    
}

int 
ssl_generate_keyring_material(SslDecryptSession*ssl_session)
{
    StringInfo key_block;
    guint8 _iv_c[MAX_BLOCK_SIZE],_iv_s[MAX_BLOCK_SIZE];
    guint8 _key_c[MAX_KEY_SIZE],_key_s[MAX_KEY_SIZE];
    int needed;
    guint8 *ptr,*c_wk,*s_wk,*c_mk,*s_mk,*c_iv = _iv_c,*s_iv = _iv_s;
    
    /* if master_key is not yet generate, create it now*/    
    if (!(ssl_session->state & SSL_MASTER_SECRET)) {
        ssl_debug_printf("ssl_generate_keyring_material:PRF(pre_master_secret)\n");
        if (PRF(ssl_session,&ssl_session->pre_master_secret,"master secret",
                &ssl_session->client_random,
                &ssl_session->server_random, &ssl_session->master_secret)) {
            ssl_debug_printf("ssl_generate_keyring_material can't generate master_secret\n");
            return -1;
        }
        ssl_print_string("master secret",&ssl_session->master_secret);
    }
    
    /* Compute the key block. First figure out how much data we need*/
    needed=ssl_session->cipher_suite.dig_len*2;
    needed+=ssl_session->cipher_suite.bits / 4;
    if(ssl_session->cipher_suite.block>1) 
        needed+=ssl_session->cipher_suite.block*2;
    
    key_block.data_len = needed;
    key_block.data = g_malloc(needed);
    if (!key_block.data) {
        ssl_debug_printf("ssl_generate_keyring_material can't allacate key_block\n");
        return -1;
    }
    ssl_debug_printf("ssl_generate_keyring_material sess key generation\n");
    if (PRF(ssl_session,&ssl_session->master_secret,"key expansion",
            &ssl_session->server_random,&ssl_session->client_random,
            &key_block)) {
        ssl_debug_printf("ssl_generate_keyring_material can't generate key_block\n");
        goto fail;
    }
    ssl_print_string("key expansion", &key_block);
    
    ptr=key_block.data;
    c_mk=ptr; ptr+=ssl_session->cipher_suite.dig_len;
    s_mk=ptr; ptr+=ssl_session->cipher_suite.dig_len;
    
    c_wk=ptr; ptr+=ssl_session->cipher_suite.eff_bits/8;
    s_wk=ptr; ptr+=ssl_session->cipher_suite.eff_bits/8;
    
    if(ssl_session->cipher_suite.block>1){
        c_iv=ptr; ptr+=ssl_session->cipher_suite.block;
        s_iv=ptr; ptr+=ssl_session->cipher_suite.block;
    }
    
    if(ssl_session->cipher_suite.export){
        StringInfo iv_c,iv_s;
        StringInfo key_c,key_s;
        StringInfo k;
        
        if(ssl_session->cipher_suite.block>1){
            
            /* We only have room for MAX_BLOCK_SIZE bytes IVs, but that's
             all we should need. This is a sanity check */
            if(ssl_session->cipher_suite.block>MAX_BLOCK_SIZE) {
                ssl_debug_printf("ssl_generate_keyring_material cipher suite block must be at most %d nut is %d\n", 
                    MAX_BLOCK_SIZE, ssl_session->cipher_suite.block);
                goto fail;
            }
        
            iv_c.data = _iv_c;
            iv_c.data_len = ssl_session->cipher_suite.block;
            iv_s.data = _iv_s;
            iv_s.data_len = ssl_session->cipher_suite.block;
            
            if(ssl_session->version_netorder==SSLV3_VERSION){
                ssl_debug_printf("ssl_generate_keyring_material ssl3_generate_export_iv\n");
                if (ssl3_generate_export_iv(&ssl_session->client_random,
                        &ssl_session->server_random,&iv_c)) {
                    ssl_debug_printf("ssl_generate_keyring_material can't generate sslv3 client iv\n");
                    goto fail;
                }
                ssl_debug_printf("ssl_generate_keyring_material ssl3_generate_export_iv(2)\n");
                if (ssl3_generate_export_iv(&ssl_session->server_random,
                        &ssl_session->client_random,&iv_s)) {
                    ssl_debug_printf("ssl_generate_keyring_material can't generate sslv3 server iv\n");
                    goto fail;
                }            
            }
            else{
                guint8 _iv_block[MAX_BLOCK_SIZE * 2];
                StringInfo iv_block;
                StringInfo key_null;
                guint8 _key_null;
                
                key_null.data = &_key_null;
                key_null.data_len = 0;
                                
                iv_block.data = _iv_block;
                iv_block.data_len = ssl_session->cipher_suite.block*2;
                
                ssl_debug_printf("ssl_generate_keyring_material prf(iv_block)\n");
                if(PRF(ssl_session,&key_null, "IV block",
                        &ssl_session->client_random,
                        &ssl_session->server_random,&iv_block)) {
                    ssl_debug_printf("ssl_generate_keyring_material can't generate tls31 iv block\n");
                    goto fail;
                }
                
                memcpy(_iv_c,iv_block.data,ssl_session->cipher_suite.block);
                memcpy(_iv_s,iv_block.data+ssl_session->cipher_suite.block,
                    ssl_session->cipher_suite.block);
            }
            
            c_iv=_iv_c;
            s_iv=_iv_s;
        }
        
        if (ssl_session->version_netorder==SSLV3_VERSION){
            
            SSL_MD5_CTX md5;
            ssl_debug_printf("ssl_generate_keyring_material MD5(client_random)\n");
            
            ssl_md5_init(&md5);
            ssl_md5_update(&md5,c_wk,ssl_session->cipher_suite.eff_bits/8);
            ssl_md5_update(&md5,ssl_session->client_random.data,
                ssl_session->client_random.data_len);
            ssl_md5_update(&md5,ssl_session->server_random.data,
                ssl_session->server_random.data_len);        
            ssl_md5_final(_key_c,&md5);
            ssl_md5_cleanup(&md5);
            c_wk=_key_c;
            
            ssl_md5_init(&md5);
            ssl_debug_printf("ssl_generate_keyring_material MD5(server_random)\n");
            ssl_md5_update(&md5,s_wk,ssl_session->cipher_suite.eff_bits/8);
            ssl_md5_update(&md5,ssl_session->server_random.data,
                ssl_session->server_random.data_len);
            ssl_md5_update(&md5,ssl_session->client_random.data,
                ssl_session->client_random.data_len);
            ssl_md5_final(_key_s,&md5);
            ssl_md5_cleanup(&md5);
            s_wk=_key_s;
        }
        else{
            key_c.data = _key_c;
            key_c.data_len = sizeof(_key_c);
            key_s.data = _key_s;
            key_s.data_len = sizeof(_key_s);
            
            k.data = c_wk;
            k.data_len = ssl_session->cipher_suite.eff_bits/8;
            ssl_debug_printf("ssl_generate_keyring_material PRF(key_c)\n");
            if (PRF(ssl_session,&k,"client write key",
                    &ssl_session->client_random,
                    &ssl_session->server_random, &key_c)) {
                ssl_debug_printf("ssl_generate_keyring_material can't generate tll31 server key \n");        
                goto fail;
            }
            c_wk=_key_c;
            
            k.data = s_wk;
            k.data_len = ssl_session->cipher_suite.eff_bits/8;
            ssl_debug_printf("ssl_generate_keyring_material PRF(key_s)\n");
            if(PRF(ssl_session,&k,"server write key",
                    &ssl_session->client_random,
                    &ssl_session->server_random, &key_s)) {
                ssl_debug_printf("ssl_generate_keyring_material can't generate tll31 client key \n");
                goto fail;
            }
            s_wk=_key_s;
        }
    }
    
    /* show key material info */
    ssl_print_data("Client MAC key",c_mk,ssl_session->cipher_suite.dig_len);
    ssl_print_data("Server MAC key",s_mk,ssl_session->cipher_suite.dig_len);    
    ssl_print_data("Client Write key",c_wk,ssl_session->cipher_suite.bits/8);
    ssl_print_data("Server Write key",s_wk,ssl_session->cipher_suite.bits/8);    
        
    if(ssl_session->cipher_suite.block>1) {
        ssl_print_data("Client Write IV",c_iv,ssl_session->cipher_suite.block);
        ssl_print_data("Server Write IV",s_iv,ssl_session->cipher_suite.block);
    }
    else {
        ssl_print_data("Client Write IV",c_iv,8);
        ssl_print_data("Server Write IV",s_iv,8);
    }
    
    /* create both client and server ciphers*/
    ssl_debug_printf("ssl_generate_keyring_material ssl_create_decoder(client)\n");
    if (ssl_create_decoder(&ssl_session->client,
            &ssl_session->cipher_suite,c_mk,c_wk,c_iv)) {
        ssl_debug_printf("ssl_generate_keyring_material can't init client decoder\n");        
        goto fail;
    }
    ssl_debug_printf("ssl_generate_keyring_material ssl_create_decoder(server)\n");
    if (ssl_create_decoder(&ssl_session->server,
            &ssl_session->cipher_suite,s_mk,s_wk,s_iv)) {
        ssl_debug_printf("ssl_generate_keyring_material can't init client decoder\n");        
        goto fail;
    }
      
    ssl_debug_printf("ssl_generate_keyring_material client seq %d server seq %d\n",
        ssl_session->client.seq, ssl_session->server.seq);
    g_free(key_block.data);
    return 0;
    
fail:
    g_free(key_block.data);
    return -1;
}

int 
ssl_decrypt_pre_master_secret(SslDecryptSession*ssl_session, 
    StringInfo* entrypted_pre_master, SSL_PRIVATE_KEY *pk)
{
    int i;
        
    if(ssl_session->cipher_suite.kex!=KEX_RSA) {
        ssl_debug_printf("ssl_decrypt_pre_master_secret key %d diferent from KEX_RSA(%d)\n",
            ssl_session->cipher_suite.kex, KEX_RSA);
        return(-1);
    }

    /* with tls key loading will fail if not rsa type, so no need to check*/
    ssl_print_string("pre master encrypted",entrypted_pre_master);
    ssl_debug_printf("ssl_decrypt_pre_master_secret:RSA_private_decrypt\n");
    i=ssl_private_decrypt(entrypted_pre_master->data_len,
        entrypted_pre_master->data, pk);

    if (i!=48) {
        ssl_debug_printf("ssl_decrypt_pre_master_secret wrong "
            "pre_master_secret lenght (%d, expected %d)\n", i, 48);
        return -1;
    }

    /* the decrypted data has been written into the pre_master key buffer */
    ssl_session->pre_master_secret.data = entrypted_pre_master->data;
    ssl_session->pre_master_secret.data_len=48;  
    ssl_print_string("pre master secret",&ssl_session->pre_master_secret);

    /* Remove the master secret if it was there.
       This force keying material regeneration in
       case we're renegotiating */
    ssl_session->state &= ~(SSL_MASTER_SECRET|SSL_HAVE_SESSION_KEY);
    return 0;
}
 
/* convert network byte order 32 byte number to right-aligned host byte order *
 * 8 bytes buffer */
static int fmt_seq(guint32 num, guint8* buf)
{
    guint32 netnum;

    memset(buf,0,8);
    netnum=g_htonl(num);
    memcpy(buf+4,&netnum,4);

    return(0);
}

static int 
tls_check_mac(SslDecoder*decoder, int ct,int ver, guint8* data,
        guint32 datalen, guint8* mac)
{
    SSL_HMAC hm;
    int md;
    guint32 len;
    guint8 buf[20];

    md=ssl_get_digest_by_name(digests[decoder->cipher_suite->dig-0x40]);
    ssl_debug_printf("tls_check_mac mac type:%s md %d\n",
        digests[decoder->cipher_suite->dig-0x40], md);
    
    ssl_hmac_init(&hm,decoder->mac_key.data,decoder->mac_key.data_len,md);
    
    /* hash sequence number */
    fmt_seq(decoder->seq,buf);
    
    decoder->seq++;
    
    ssl_hmac_update(&hm,buf,8);
    
    /* hash content type */
    buf[0]=ct;
    ssl_hmac_update(&hm,buf,1);

    /* hash version,data lenght and data*/
    *((gint16*)buf) = g_htons(ver);
    ssl_hmac_update(&hm,buf,2); 
    
    *((gint16*)buf) = g_htons(datalen);
    ssl_hmac_update(&hm,buf,2);
    ssl_hmac_update(&hm,data,datalen);

    /* get digest and digest len*/
    ssl_hmac_final(&hm,buf,&len);
    ssl_print_data("Mac", buf, len);
    if(memcmp(mac,buf,len))
        return -1;

    ssl_hmac_cleanup(&hm);
    return(0);
}

int 
ssl3_check_mac(SslDecoder*decoder,int ct,guint8* data,
        guint32 datalen, guint8* mac)
{
    SSL_MD mc;
    int md;
    guint32 len;
    guint8 buf[64],dgst[20];
    int pad_ct;
    
    pad_ct=(decoder->cipher_suite->dig==DIG_SHA)?40:48;

    /* get cipher used for digest comptuation */
    md=ssl_get_digest_by_name(digests[decoder->cipher_suite->dig-0x40]);
    ssl_md_init(&mc,md);

    /* do hash computation on data && padding */
    ssl_md_update(&mc,decoder->mac_key.data,decoder->mac_key.data_len);

    /* hash padding*/
    memset(buf,0x36,pad_ct);
    ssl_md_update(&mc,buf,pad_ct);

    /* hash sequence number */
    fmt_seq(decoder->seq,buf);
    decoder->seq++;
    ssl_md_update(&mc,buf,8);

    /* hash content type */
    buf[0]=ct;
    ssl_md_update(&mc,buf,1);

    /* hash data lenght in network byte order and data*/ 
    *((gint16* )buf) = g_htons(datalen);
    ssl_md_update(&mc,buf,2);
    ssl_md_update(&mc,data,datalen);

    /* get partial digest */
    ssl_md_final(&mc,dgst,&len);
    ssl_md_cleanup(&mc);

    ssl_md_init(&mc,md);

    /* hash mac key */
    ssl_md_update(&mc,decoder->mac_key.data,decoder->mac_key.data_len);

    /* hash padding and partial digest*/
    memset(buf,0x5c,pad_ct);
    ssl_md_update(&mc,buf,pad_ct);
    ssl_md_update(&mc,dgst,len);

    ssl_md_final(&mc,dgst,&len);
    ssl_md_cleanup(&mc);

    if(memcmp(mac,dgst,len))
        return -1;

    return(0);
}
 
#if 0
static int 
dtls_check_mac(SslDecoder*decoder, int ct,int ver, guint8* data,
        guint32 datalen, guint8* mac)
{
    SSL_HMAC hm;
    int md;
    guint32 len;
    guint8 buf[20];
    guint32 netnum;
    md=ssl_get_digest_by_name(digests[decoder->cipher_suite->dig-0x40]);
    ssl_debug_printf("dtls_check_mac mac type:%s md %d\n",
        digests[decoder->cipher_suite->dig-0x40], md);
    
    ssl_hmac_init(&hm,decoder->mac_key.data,decoder->mac_key.data_len,md);
    ssl_debug_printf("dtls_check_mac seq: %d epoch: %d\n",decoder->seq,decoder->epoch);
    /* hash sequence number */
    fmt_seq(decoder->seq,buf);
    buf[0]=decoder->epoch>>8;
    buf[1]=decoder->epoch;

    ssl_hmac_update(&hm,buf,8);
   
    /* hash content type */
    buf[0]=ct;
    ssl_hmac_update(&hm,buf,1);

    /* hash version,data lenght and data */
    *((gint16*)buf) = g_htons(ver);
    ssl_hmac_update(&hm,buf,2); 
    
    *((gint16*)buf) = g_htons(datalen);
    ssl_hmac_update(&hm,buf,2);
    ssl_hmac_update(&hm,data,datalen);
    /* get digest and digest len */
    ssl_hmac_final(&hm,buf,&len);
    ssl_print_data("Mac", buf, len);
    if(memcmp(mac,buf,len))
        return -1;

    ssl_hmac_cleanup(&hm);
    return(0);
}
#endif

 
int 
ssl_decrypt_record(SslDecryptSession*ssl,SslDecoder* decoder, int ct,
        const unsigned char* in, int inl,unsigned char*out,int* outl)
{
    int pad, worklen;
    guint8 *mac;


    ssl_debug_printf("ssl_decrypt_record ciphertext len %d\n", inl);
    ssl_print_data("Ciphertext",in, inl);
  
    /* First decrypt*/
    if ((pad = ssl_cipher_decrypt(&decoder->evp,out,*outl,in,inl))!= 0)
        ssl_debug_printf("ssl_decrypt_record: %s %s\n", gcry_strsource (pad),
                    gcry_strerror (pad));

    ssl_print_data("Plaintext",out,inl);
    worklen=inl;

    /* Now strip off the padding*/
    if(decoder->cipher_suite->block!=1){
        pad=out[inl-1];
        worklen-=(pad+1);
        ssl_debug_printf("ssl_decrypt_record found padding %d final len %d\n", 
            pad, *outl);
    }

    /* And the MAC */
    worklen-=decoder->cipher_suite->dig_len;
    if (worklen < 0)
    {
        ssl_debug_printf("ssl_decrypt_record wrong record len/padding outlen %d\n work %d\n",*outl, worklen);
        return -1;
    }
    mac=out+worklen;

    /* if TLS 1.1 we use the transmitted IV and remove it after (to not modify dissector in others parts)*/
    if(ssl->version_netorder==TLSV1DOT1_VERSION){
	worklen=worklen-decoder->cipher_suite->block; 
	memcpy(out,out+decoder->cipher_suite->block,worklen);
   }
  if(ssl->version_netorder==DTLSV1DOT0_VERSION){
        worklen=worklen-decoder->cipher_suite->block; 
	memcpy(out,out+decoder->cipher_suite->block,worklen);
   }
    /* Now check the MAC */
    ssl_debug_printf("checking mac (len %d, version %X, ct %d seq %d)\n", 
        worklen, ssl->version_netorder, ct, decoder->seq);
    if(ssl->version_netorder==SSLV3_VERSION){
        if(ssl3_check_mac(decoder,ct,out,worklen,mac) < 0) {
            ssl_debug_printf("ssl_decrypt_record: mac failed\n");
            return -1;
        }
    }
    else if(ssl->version_netorder==TLSV1_VERSION || ssl->version_netorder==TLSV1DOT1_VERSION){
        if(tls_check_mac(decoder,ct,ssl->version_netorder,out,worklen,mac)< 0) {
            ssl_debug_printf("ssl_decrypt_record: mac failed\n");
            return -1;
        }
    }
    else if(ssl->version_netorder==DTLSV1DOT0_VERSION){
      /* follow the openssl dtls errors the rigth test is : dtls_check_mac(decoder,ct,ssl->version_netorder,out,worklen,mac)< 0 */
	if(tls_check_mac(decoder,ct,TLSV1_VERSION,out,worklen,mac)< 0) {
            ssl_debug_printf("ssl_decrypt_record: mac failed\n");
            return -1;
        }
    }
    ssl_debug_printf("ssl_decrypt_record: mac ok\n");
    *outl = worklen;
    return(0);
}

/* old relase of gnutls does not define the appropriate macros, so get 
 * them from the string*/
static void 
ssl_get_version(int* major, int* minor, int* patch)
{
    const char* str = gnutls_check_version(NULL);
    
    ssl_debug_printf("ssl_get_version: %s\n", str);
    sscanf(str, "%d.%d.%d", major, minor, patch);
}


SSL_PRIVATE_KEY* 
ssl_load_key(FILE* fp)
{    
    /* gnutls make our work much harded, since we have to work internally with
     * s-exp formatted data, but PEM loader export only in "gnutls_datum" 
     * format, and a datum -> s-exp convertion function does not exist.
     */
    struct gnutls_x509_privkey_int* priv_key;
    gnutls_datum key;
    gnutls_datum m, e, d, p,q, u;
    int size, major, minor, patch;
    unsigned int bytes;
    unsigned int tmp_size;
#ifdef SSL_FAST
    gcry_mpi_t* rsa_params = g_malloc(sizeof(gcry_mpi_t)*6);
#else
    gcry_mpi_t rsa_params[6];
#endif
    gcry_sexp_t rsa_priv_key;
    
    /* init private key data*/
    gnutls_x509_privkey_init(&priv_key);
    
    /* compute file size and load all file contents into a datum buffer*/
    if (fseek(fp, 0, SEEK_END) < 0) {
        ssl_debug_printf("ssl_load_key: can't fseek file\n");
        return NULL;
    }
    if ((size = ftell(fp)) < 0) {
        ssl_debug_printf("ssl_load_key: can't ftell file\n");
        return NULL;
    }
    if (fseek(fp, 0, SEEK_SET) < 0) {
        ssl_debug_printf("ssl_load_key: can't refseek file\n");
        return NULL;
    }
    key.data = g_malloc(size);
    key.size = size;
    bytes = fread(key.data, 1, key.size, fp);
    if (bytes < key.size) {
        ssl_debug_printf("ssl_load_key: can't read from file %d bytes, got %d\n", 
            key.size, bytes);
        return NULL;
    }
    
    /* import PEM data*/
    if (gnutls_x509_privkey_import(priv_key, &key, GNUTLS_X509_FMT_PEM)!=0) {
        ssl_debug_printf("ssl_load_key: can't import pem data\n");
        return NULL;
    }
    free(key.data);
    
    /* RSA get parameter */
    if (gnutls_x509_privkey_export_rsa_raw(priv_key, &m, &e, &d, &p, &q, &u) != 0) {
        ssl_debug_printf("ssl_load_key: can't export rsa param (is a rsa private key file ?!?)\n");
        return NULL;
    }
    
    /* convert each rsa parameter to mpi format*/
    if (gcry_mpi_scan( &rsa_params[0], GCRYMPI_FMT_USG, m.data,  m.size, &tmp_size) !=0) {
        ssl_debug_printf("ssl_load_key: can't convert m rsa param to int (size %d)\n", m.size);
        return NULL;
    }
    
    if (gcry_mpi_scan( &rsa_params[1], GCRYMPI_FMT_USG, e.data,  e.size, &tmp_size) != 0) {
        ssl_debug_printf("ssl_load_key: can't convert e rsa param to int (size %d)\n", e.size);
        return NULL;
    }

    /*
     * note: openssl and gnutls use 'p' and 'q' with opposite meaning:
     * our 'p' must be equal to 'q' as provided from openssl and viceversa
     */
    if (gcry_mpi_scan( &rsa_params[2], GCRYMPI_FMT_USG, d.data,  d.size, &tmp_size) !=0) {
        ssl_debug_printf("ssl_load_key: can't convert d rsa param to int (size %d)\n", d.size);
        return NULL;
    }
    
    if (gcry_mpi_scan( &rsa_params[3], GCRYMPI_FMT_USG, q.data,  q.size, &tmp_size) !=0) {
        ssl_debug_printf("ssl_load_key: can't convert q rsa param to int (size %d)\n", q.size);
        return NULL;
    }

    if (gcry_mpi_scan( &rsa_params[4], GCRYMPI_FMT_USG, p.data,  p.size, &tmp_size) !=0) {
        ssl_debug_printf("ssl_load_key: can't convert p rsa param to int (size %d)\n", p.size);
        return NULL;
    }
        
    if (gcry_mpi_scan( &rsa_params[5], GCRYMPI_FMT_USG, u.data,  u.size, &tmp_size) !=0) {
        ssl_debug_printf("ssl_load_key: can't convert u rsa param to int (size %d)\n", m.size);
        return NULL; 
    }
    
    ssl_get_version(&major, &minor, &patch);
    
    /* certain versions of gnutls require swap of rsa params 'p' and 'q' */
    if ((major <= 1) && (minor <= 0) && (patch <=13))
    {
        gcry_mpi_t tmp;
        ssl_debug_printf("ssl_load_key: swapping p and q parametes\n");
        tmp = rsa_params[4];
        rsa_params[4] = rsa_params[3];
        rsa_params[3] = tmp;
    }
    
    if  (gcry_sexp_build( &rsa_priv_key, NULL,
            "(private-key(rsa((n%m)(e%m)(d%m)(p%m)(q%m)(u%m))))", rsa_params[0], 
            rsa_params[1], rsa_params[2], rsa_params[3], rsa_params[4], 
            rsa_params[5]) != 0) {
        ssl_debug_printf("ssl_load_key: can't built rsa private key s-exp\n");
        return NULL;
    }

#if SSL_FAST    
    return rsa_params;
#else
    {
        int i;
        for (i=0; i< 6; i++)
            gcry_mpi_release(rsa_params[i]);
    }
    return rsa_priv_key;
#endif
}

void ssl_free_key(SSL_PRIVATE_KEY* key)
{
#if SSL_FAST
    int i;
    for (i=0; i< 6; i++)
        gcry_mpi_release(key[i]);    
#else
    gcry_sexp_release(key);
#endif
}

void 
ssl_lib_init(void)
{
    gnutls_global_init();
}

#else /* HAVE_LIBGNUTLS */
/* no libgnutl: dummy operation to keep interface consistent*/
void 
ssl_lib_init(void)
{
}

SSL_PRIVATE_KEY* 
ssl_load_key(FILE* fp) 
{
    ssl_debug_printf("ssl_load_key: impossible without glutls. fp %p\n",fp);
    return NULL;
}

void 
ssl_free_key(SSL_PRIVATE_KEY* key _U_)
{
}

int 
ssl_find_cipher(int num,SslCipherSuite* cs) 
{
    ssl_debug_printf("ssl_find_cipher: dummy without glutls. num %d cs %p\n",
        num,cs);
    return 0; 
}
int 
ssl_generate_keyring_material(SslDecryptSession*ssl) 
{
    ssl_debug_printf("ssl_generate_keyring_material: impossible without glutls. ssl %p\n",
        ssl);
    return 0; 
}
int 
ssl_decrypt_pre_master_secret(SslDecryptSession* ssl_session, 
    StringInfo* entrypted_pre_master, SSL_PRIVATE_KEY *pk)
{
    ssl_debug_printf("ssl_decrypt_pre_master_secret: impossible without glutls."
        " ssl %p entrypted_pre_master %p pk %p\n", ssl_session,
        entrypted_pre_master, pk);
    return 0;
}

int 
ssl_decrypt_record(SslDecryptSession*ssl,SslDecoder* decoder, int ct, 
        const unsigned char* in, int inl,unsigned char*out,int* outl)
{
    ssl_debug_printf("ssl_decrypt_record: impossible without gnutls. ssl %p"
        "decoder %p ct %d, in %p inl %d out %p outl %p\n", ssl, decoder, ct,
        in, inl, out, outl);
    return 0;
}

#endif /* HAVE_LIBGNUTLS */

/* get ssl data for this session. if no ssl data is found allocate a new one*/
void 
ssl_session_init(SslDecryptSession* ssl_session)
{
    ssl_debug_printf("ssl_session_init: initializing ptr %p size %lu\n", 
        ssl_session, (unsigned long)sizeof(SslDecryptSession));

    ssl_session->master_secret.data = ssl_session->_master_secret;
    ssl_session->session_id.data = ssl_session->_session_id;
    ssl_session->client_random.data = ssl_session->_client_random;
    ssl_session->server_random.data = ssl_session->_server_random;
    ssl_session->master_secret.data_len = 48;
    ssl_session->app_data_segment.data=NULL;
    ssl_session->app_data_segment.data_len=0;
}

#ifdef SSL_DECRYPT_DEBUG

static FILE* ssl_debug_file=NULL;

void 
ssl_set_debug(char* name)
{
    static int debug_file_must_be_closed = 0;
    int use_stderr = name?(strcmp(name, SSL_DEBUG_USE_STDERR) == 0):0;
    
    if (debug_file_must_be_closed)
        fclose(ssl_debug_file);
    if (use_stderr)    
        ssl_debug_file = stderr;    
    else if (!name || (strcmp(name, "") ==0))
        ssl_debug_file = NULL;
    else
        ssl_debug_file = fopen(name, "w");    
    if (!use_stderr && ssl_debug_file)
        debug_file_must_be_closed = 1;
}


void 
ssl_debug_printf(const char* fmt, ...)
{
    va_list ap;
    int ret=0;
    if (!ssl_debug_file)  
        return;
    
    va_start(ap, fmt);
    ret += vfprintf(ssl_debug_file, fmt, ap);
    va_end(ap);
    fflush(ssl_debug_file);
}

void 
ssl_print_text_data(const char* name, const unsigned char* data, int len)
{
    int i;
    if (!ssl_debug_file)  
        return;
    fprintf(ssl_debug_file,"%s: ",name);
    for (i=0; i< len; i++) {
      fprintf(ssl_debug_file,"%c",data[i]);
    }
    fprintf(ssl_debug_file,"\n");
    fflush(ssl_debug_file);
}

void 
ssl_print_data(const char* name, const unsigned char* data, int len)
{
    int i;
    if (!ssl_debug_file)  
        return;
    fprintf(ssl_debug_file,"%s[%d]:\n",name, len);
    for (i=0; i< len; i++) {
        if ((i>0) && (i%16 == 0))
            fprintf(ssl_debug_file,"\n");
        fprintf(ssl_debug_file,"%.2x ",data[i]&255);
    }
    fprintf(ssl_debug_file,"\n");
    fflush(ssl_debug_file);
}

void 
ssl_print_string(const char* name, const StringInfo* data)
{
    ssl_print_data(name, data->data, data->data_len);
}
#endif /* SSL_DECRYPT_DEBUG */
