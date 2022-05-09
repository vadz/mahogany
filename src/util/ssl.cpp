///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   util/ssl.cpp
// Purpose:     SSL support functions
// Author:      Karsten Ball�der
// Modified by:
// Created:     24.01.01 (extracted from MailFolderCC.cpp)
// CVS-ID:      $Id$
// Copyright:   (C) 1997-2000 by Karsten Ball�der (ballueder@gmx.net)
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#include "Mpch.h"

#include "Mcommon.h"

#ifdef USE_SSL

#ifndef USE_PCH
   #include "Mdefaults.h"

   // needed to get ssl_onceonlyinit() declaration
   #include "Mcclient.h"

   #if defined(OS_UNIX) || defined(__CYGWIN__)
      #include "Profile.h"
      #include "MApplication.h"
   #endif
#endif // USE_PCH

#include "gui/wxMDialogs.h"

// under Unix we have to load libssl and libcrypto to provide access to them to
// c-client
#if (defined(OS_UNIX) || defined(__CYGWIN__)) && !defined(__WINE__)

#include <wx/dynlib.h>

#include <stdint.h>

extern const MOption MP_SSL_DLL_CRYPTO;
extern const MOption MP_SSL_DLL_SSL;

/* These functions will serve as stubs for the real openSSL library
   and must be initialised at runtime as c-client actually links
   against these. */

#include <openssl/ssl.h>
#include <openssl/opensslv.h>

// starting from 0.9.6a, OpenSSL uses void, as it should, instead of char
#if defined(OPENSSL_VERSION_NUMBER) && (OPENSSL_VERSION_NUMBER > 0x0090600fL)
   #define ssl_data_t void *
#else // old ssl
   #define ssl_data_t char *
#endif
// starting from 0.9.7, OpenSSL uses void instead of char
#if defined(OPENSSL_VERSION_NUMBER) && (OPENSSL_VERSION_NUMBER >= 0x0090700fL)
   #define ssl_parg void *
#else // old ssl
   #define ssl_parg char *
#endif

// starting from 0.9.7g OpenSSL has discovered const correctness (better late
// than never)
#if defined(OPENSSL_VERSION_NUMBER) && (OPENSSL_VERSION_NUMBER >= 0x0090707fL)
   #define ssl_const1 const
#else
   #define ssl_const1
#endif
// OpenSSL 1.0.0-beta3
#if defined(OPENSSL_VERSION_NUMBER) && (OPENSSL_VERSION_NUMBER >= 0x10000003L)
   #define ssl_const2 const
   #define ssl_STACK _STACK
   #define sk_value_t void *
#else
   #define ssl_const2
   #define ssl_STACK STACK
   #define sk_value_t char *
#endif
// OpenSSL 1.0.2g
#if defined(OPENSSL_VERSION_NUMBER) && (OPENSSL_VERSION_NUMBER >= 0x1000207fL)
   #define ssl_const3 const
#else
   #define ssl_const3
#endif
// OpenSSL 1.1.0
#if defined(OPENSSL_VERSION_NUMBER) && (OPENSSL_VERSION_NUMBER >= 0x10100000L)
   #define ssl_const110 const

   // A couple of functions used by c-client code were deprecated in OpenSSL
   // 1.1.0 and preserved only as compatibility defines. We still want to
   // define stub functions for them though and macros just get into the way.
   #undef SSL_library_init
   #undef SSL_load_error_strings
   #undef ERR_load_crypto_strings
   #undef SSL_CTX_set_tmp_rsa_callback
   #undef SSLv23_client_method
   #undef SSLv23_server_method
   #undef sk_num
   #undef sk_value
#else
   #define ssl_const110

   // Conversely, a few functions existed as macros originally but need to be
   // loaded dynamically in 1.1.0. Handling them is simpler as c-client code
   // will still use the macros with old versions, while we'll provide the
   // forwarder functions for the later ones.
   #undef SSL_in_init
   #undef SSL_CTX_set_options
#endif
// OpenSSL 1.1.1
#if defined(OPENSSL_VERSION_NUMBER) && (OPENSSL_VERSION_NUMBER >= 0x1010100fL)
   #define ssl_const111 const
#else
   #define ssl_const111
#endif
// OpenSSL 3.0
#if defined(OPENSSL_VERSION_NUMBER) && (OPENSSL_VERSION_NUMBER >= 0x30000000L)
   #define ssl_const30 const
   #define SSL_get_peer_certificate SSL_get1_peer_certificate
#else
   #define ssl_const30
#endif

/* This is our interface to the library and auth_ssl.c in c-client
   which are all in "C" */
extern "C" {

/*
   This macro is used to do the following things:

   1. define the type for the given SSL functions
   2. declare a variable of this type named stub_SSL_XXX and init it to NULL
   3. declare our SSL_XXX which simply forwards to stub_SSL_XXX
 */
#define SSL_DEF2(returnval, name, args, params, ret) \
   typedef returnval (* name##_TYPE) args ; \
   name##_TYPE stub_##name = NULL; \
   returnval name args \
      { ret (*stub_##name) params; }

// the SSL_DEF macro for non void functions
#define SSL_DEF(returnval, name, args, params) \
   SSL_DEF2(returnval, name, args, params, return)

// ... and for the void ones
#define SSL_DEF_VOID(name, args, params) \
   SSL_DEF2(void, name, args, params, (void))

#define SSL_LOOKUP_NO_ERROR_CHECK(name) \
      stub_##name = (name##_TYPE) gs_dllSll.GetSymbol(#name)

#define SSL_LOOKUP(name) \
      SSL_LOOKUP_NO_ERROR_CHECK(name); \
      if ( !stub_##name ) \
         goto error

#define CRYPTO_LOOKUP(name) \
      stub_##name = (name##_TYPE) gs_dllCrypto.GetSymbol(#name); \
      if ( !stub_##name ) \
         goto error

SSL_DEF( SSL *, SSL_new, (SSL_CTX *ctx), (ctx) );
SSL_DEF_VOID( SSL_free, (SSL *s), (s) );
SSL_DEF( int,  SSL_set_rfd, (SSL *s, int fd), (s, fd) );
SSL_DEF( int,  SSL_set_wfd, (SSL *s, int fd), (s, fd) );
SSL_DEF_VOID( SSL_set_read_ahead, (SSL *s, int yes), (s, yes) );
SSL_DEF( int,  SSL_connect, (SSL *s), (s) );
SSL_DEF( int,  SSL_read, (SSL *s,ssl_data_t buf,int num), (s, buf, num) );
SSL_DEF( int,  SSL_write, (SSL *s,const ssl_data_t buf,int num), (s, buf, num) );
SSL_DEF( int,  SSL_pending, (ssl_const1 SSL *s), (s) );
SSL_DEF( int,  SSL_library_init, (void ), () );
SSL_DEF_VOID( SSL_load_error_strings, (void ), () );
SSL_DEF( SSL_CTX *,SSL_CTX_new, (ssl_const2 SSL_METHOD *meth), (meth) );
SSL_DEF( const char *, SSL_CIPHER_get_name, (ssl_const1 SSL_CIPHER *c), (c) );
SSL_DEF( int, SSL_CIPHER_get_bits, (ssl_const1 SSL_CIPHER *c, int *alg_bits), (c,alg_bits) );
SSL_DEF( ssl_const2 SSL_CIPHER *, SSL_get_current_cipher ,(ssl_const1 SSL *s), (s) );
SSL_DEF( int, SSL_get_fd, (ssl_const1 SSL *s), (s) );
SSL_DEF( int, SSL_set_fd, (SSL *s, int fd), (s, fd) );
SSL_DEF( int, SSL_get_error, (ssl_const1 SSL *s, int ret_code), (s, ret_code) );
SSL_DEF( X509 *, SSL_get_peer_certificate, (ssl_const1 SSL *s), (s) );

SSL_DEF( int, sk_num, (const ssl_STACK *s), (s) );
SSL_DEF( sk_value_t, sk_value, (const ssl_STACK *s, int n), (s, n) );

SSL_DEF( int, OPENSSL_sk_num, (const struct stack_st *s), (s) );
SSL_DEF( void*, OPENSSL_sk_value, (const struct stack_st *s, int n), (s, n) );

SSL_DEF_VOID( RAND_seed, (const void *buf,int num), (buf, num) );
SSL_DEF( BIO *, BIO_new_socket, (int sock, int close_flag), (sock, close_flag) );
SSL_DEF( BIO *, BIO_new_mem_buf, (ssl_const3 void *buf, int len), (buf, len) );
SSL_DEF( int, BIO_free, (BIO *a), (a) );
SSL_DEF( long, SSL_CTX_ctrl, (SSL_CTX *ctx,int cmd, long larg, ssl_parg parg), (ctx,cmd,larg,parg) );
SSL_DEF_VOID( SSL_CTX_set_verify, (SSL_CTX *ctx,int mode, int (*callback)(int, X509_STORE_CTX *)), (ctx,mode,callback) );
SSL_DEF( int, SSL_CTX_load_verify_locations, (SSL_CTX *ctx, const char *CAfile, const char *CApath), (ctx, CAfile, CApath) );
SSL_DEF( int, SSL_CTX_set_default_verify_paths, (SSL_CTX *ctx), (ctx) );
SSL_DEF_VOID( SSL_set_bio, (SSL *s, BIO *rbio,BIO *wbio), (s,rbio,wbio) );
SSL_DEF_VOID( SSL_set_connect_state, (SSL *s), (s) );
SSL_DEF( int, SSL_state, (ssl_const1 SSL *ssl), (ssl) );
SSL_DEF( long,    SSL_ctrl, (SSL *ssl,int cmd, long larg, ssl_parg parg), (ssl,cmd,larg,parg) );
SSL_DEF_VOID( ERR_load_crypto_strings, (void), () );
SSL_DEF( ssl_const2 SSL_METHOD *,TLSv1_server_method, (void), () );
SSL_DEF( ssl_const2 SSL_METHOD *,SSLv23_server_method, (void), () );
SSL_DEF( int, SSL_CTX_set_cipher_list, (SSL_CTX *ctx,const char *str), (ctx,str) );
SSL_DEF( int, SSL_CTX_use_certificate_chain_file, (SSL_CTX *ctx, const char *file), (ctx,file) );
SSL_DEF( int, SSL_CTX_use_RSAPrivateKey_file, (SSL_CTX *ctx, const char *file, int type), (ctx,file,type) );
SSL_DEF_VOID( SSL_CTX_set_tmp_rsa_callback, (SSL_CTX *ctx, RSA *(*cb)(SSL *,int, int)), (ctx,cb) );
SSL_DEF( int,  SSL_accept, (SSL *ssl), (ssl) );
SSL_DEF( int, X509_STORE_CTX_get_error, (ssl_const30 X509_STORE_CTX *ctx), (ctx) );
SSL_DEF( const char *, X509_verify_cert_error_string, (long n), (n) );
SSL_DEF( X509 *, X509_STORE_CTX_get_current_cert, (ssl_const30 X509_STORE_CTX *ctx), (ctx) );
SSL_DEF( X509_NAME *, X509_get_subject_name, (ssl_const110 X509 *a), (a) );
SSL_DEF( char *, X509_NAME_oneline, (ssl_const110 X509_NAME *a,char *buf,int size), (a,buf,size) );
SSL_DEF( void *, X509_get_ext_d2i, (ssl_const110 X509 *x, int nid, int *crit, int *idx), (x, nid, crit, idx) );
SSL_DEF_VOID( X509_free, (X509 *x), (x) );
SSL_DEF( int, SSL_shutdown, (SSL *s), (s) );
SSL_DEF( int, SSL_CTX_use_certificate, (SSL_CTX *ctx, X509 *x), (ctx, x) );
SSL_DEF( int, SSL_CTX_use_PrivateKey, (SSL_CTX *ctx, EVP_PKEY *pk), (ctx, pk) );
SSL_DEF_VOID( SSL_CTX_free, (SSL_CTX *ctx), (ctx) );
SSL_DEF( RSA *, RSA_generate_key, (int bits, unsigned long e,void (*cb)(int,int,void *),void *cb_arg), (bits,e,cb,cb_arg) );
SSL_DEF(ssl_const2 SSL_METHOD *, TLSv1_client_method, (void), () );
SSL_DEF(ssl_const2 SSL_METHOD *, SSLv23_client_method, (void), () );
SSL_DEF_VOID( EVP_PKEY_free, (EVP_PKEY *pk), (pk) );
SSL_DEF( X509 *, PEM_read_bio_X509, (BIO *bp, X509 **x, pem_password_cb *cb, void *u), (bp, x, cb, u) );
SSL_DEF( EVP_PKEY *, PEM_read_bio_PrivateKey, (BIO *bp, EVP_PKEY **x, pem_password_cb *cb, void *u), (bp, x, cb, u) );

SSL_DEF( unsigned long, ERR_get_error, (void), () );
SSL_DEF( char *, ERR_error_string, (unsigned long e, char *p), (e, p) );

// Those are macros until 1.1.0, functions in it.
SSL_DEF( int, SSL_in_init, (ssl_const111 SSL* s), (s) );
SSL_DEF( unsigned long, SSL_CTX_set_options, (SSL_CTX *ctx, unsigned long op), (ctx, op) );

// These functions are used inside macro expansions in openssl/ssl.h.
SSL_DEF( int, OPENSSL_init_ssl, (uint64_t opts, const struct ossl_init_settings_st* settings), (opts, settings) );
SSL_DEF( int, OPENSSL_init_crypto, (uint64_t opts, const struct ossl_init_settings_st* settings), (opts, settings) );

} // extern "C"

// These functions are not used by c-client but only in this file itself.
SSL_DEF( const SSL_METHOD *, TLS_client_method, (void), () );
SSL_DEF( const SSL_METHOD *, TLS_server_method, (void), () );

static int compat_SSL_library_init()
{
   return (*stub_OPENSSL_init_ssl)(0, NULL);
}

#ifndef OPENSSL_INIT_LOAD_SSL_STRINGS
   #define OPENSSL_INIT_LOAD_SSL_STRINGS 0x00200000L
#endif
#ifndef OPENSSL_INIT_LOAD_CRYPTO_STRINGS
   #define OPENSSL_INIT_LOAD_CRYPTO_STRINGS 0x00000002L
#endif

static void compat_SSL_load_error_strings()
{
   (*stub_OPENSSL_init_ssl)(OPENSSL_INIT_LOAD_SSL_STRINGS |
                            OPENSSL_INIT_LOAD_CRYPTO_STRINGS, NULL);
}

static void compat_ERR_load_crypto_strings()
{
   (*stub_OPENSSL_init_crypto)(OPENSSL_INIT_LOAD_CRYPTO_STRINGS, NULL);
}

static void compat_SSL_CTX_set_tmp_rsa_callback(SSL_CTX *, RSA *(*)(SSL *, int, int))
{
   // Nothing to do here, this seems to be unsupported in OpenSSL 1.1.0.
}

#undef SSL_DEF

static bool gs_SSL_loaded = false;
static bool gs_SSL_available = false;

static wxDynamicLibrary gs_dllSll,
                        gs_dllCrypto;

bool InitSSL(void) /* FIXME: MT */
{
   static bool s_errMsgGiven = false;

   if(gs_SSL_loaded)
      return gs_SSL_available;

   String ssl_dll = READ_APPCONFIG(MP_SSL_DLL_SSL);
   String crypto_dll = READ_APPCONFIG(MP_SSL_DLL_CRYPTO);

   // it doesn't take long so nobody sees this message anyhow
#if 0
   STATUSMESSAGE((_("Trying to load SSL libraries '%s' and '%s'..."),
                  crypto_dll.c_str(),
                  ssl_dll.c_str()));
#endif // 0

   if ( !gs_dllCrypto.Load(crypto_dll) )
      goto error;

   if ( !gs_dllSll.Load(ssl_dll) )
      goto error;

   SSL_LOOKUP_NO_ERROR_CHECK(SSL_library_init);
   if ( stub_SSL_library_init )
   {
      // We must be using OpenSSL < 1.1.0, so all the other functions removed
      // in 1.1.0 should be there.
      SSL_LOOKUP(SSL_load_error_strings);
      SSL_LOOKUP(ERR_load_crypto_strings);
      SSL_LOOKUP(SSL_CTX_set_tmp_rsa_callback);
      SSL_LOOKUP(SSLv23_server_method);
      SSL_LOOKUP(SSLv23_client_method);
      SSL_LOOKUP(sk_num);
      SSL_LOOKUP(sk_value);
   }
   else // Perhaps it's OpenSSL 1.1.0+, check for new functions.
   {
      SSL_LOOKUP(OPENSSL_init_ssl);
      SSL_LOOKUP(OPENSSL_init_crypto);
      SSL_LOOKUP(TLS_server_method);
      SSL_LOOKUP(TLS_client_method);
      SSL_LOOKUP(OPENSSL_sk_num);
      SSL_LOOKUP(OPENSSL_sk_value);
      SSL_LOOKUP(SSL_in_init);
      SSL_LOOKUP(SSL_CTX_set_options);

      // We've got the new functions, but we still need to provide the old ones
      // to c-client
      stub_SSL_library_init = compat_SSL_library_init;
      stub_SSL_load_error_strings = compat_SSL_load_error_strings;
      stub_ERR_load_crypto_strings = compat_ERR_load_crypto_strings;
      stub_SSL_CTX_set_tmp_rsa_callback = compat_SSL_CTX_set_tmp_rsa_callback;
      stub_SSLv23_client_method = stub_TLS_client_method;
      stub_SSLv23_server_method = stub_TLS_server_method;
   }

   SSL_LOOKUP(SSL_new);
   SSL_LOOKUP(SSL_free);
   SSL_LOOKUP(SSL_set_rfd);
   SSL_LOOKUP(SSL_set_wfd);
   SSL_LOOKUP(SSL_set_read_ahead);
   SSL_LOOKUP(SSL_connect);
   SSL_LOOKUP(SSL_read);
   SSL_LOOKUP(SSL_write);
   SSL_LOOKUP(SSL_pending);
   SSL_LOOKUP(SSL_CTX_new);
   SSL_LOOKUP(SSL_CTX_use_certificate);
   SSL_LOOKUP(SSL_CTX_use_PrivateKey);
   SSL_LOOKUP(SSL_CIPHER_get_name);
   SSL_LOOKUP(SSL_CIPHER_get_bits);
   SSL_LOOKUP(SSL_get_current_cipher);
   SSL_LOOKUP(SSL_get_fd);
   SSL_LOOKUP(SSL_set_fd);
   SSL_LOOKUP(SSL_get_error);
// OpenSSL 3.0
#if defined(OPENSSL_VERSION_NUMBER) && (OPENSSL_VERSION_NUMBER >= 0x30000000L)
   SSL_LOOKUP(SSL_get1_peer_certificate);
#else
   SSL_LOOKUP(SSL_get_peer_certificate);
#endif
   SSL_LOOKUP(RAND_seed);
   SSL_LOOKUP(BIO_new_socket);
   SSL_LOOKUP(BIO_new_mem_buf);
   SSL_LOOKUP(BIO_free);
   SSL_LOOKUP(SSL_CTX_ctrl);
   SSL_LOOKUP(SSL_CTX_set_verify);
   SSL_LOOKUP(SSL_CTX_load_verify_locations);
   SSL_LOOKUP(SSL_CTX_set_default_verify_paths);
   SSL_LOOKUP(SSL_set_bio);
   SSL_LOOKUP(SSL_set_connect_state);
   SSL_LOOKUP(SSL_ctrl);
   SSL_LOOKUP(TLSv1_server_method);
   SSL_LOOKUP(SSL_CTX_set_cipher_list);
   SSL_LOOKUP(SSL_CTX_use_certificate_chain_file);
   SSL_LOOKUP(SSL_CTX_use_RSAPrivateKey_file);
   SSL_LOOKUP(SSL_accept);
   SSL_LOOKUP(X509_STORE_CTX_get_error);
   SSL_LOOKUP(X509_verify_cert_error_string);
   SSL_LOOKUP(X509_STORE_CTX_get_current_cert);
   SSL_LOOKUP(X509_get_subject_name);
   SSL_LOOKUP(X509_NAME_oneline);
   SSL_LOOKUP(X509_get_ext_d2i);
   SSL_LOOKUP(X509_free);
   SSL_LOOKUP(SSL_shutdown);
   SSL_LOOKUP(SSL_CTX_free);
   SSL_LOOKUP(RSA_generate_key);
   SSL_LOOKUP(TLSv1_client_method);
   SSL_LOOKUP(EVP_PKEY_free);
   SSL_LOOKUP(PEM_read_bio_X509);
   SSL_LOOKUP(PEM_read_bio_PrivateKey);

   CRYPTO_LOOKUP(ERR_get_error);
   CRYPTO_LOOKUP(ERR_error_string);

   gs_SSL_available =
   gs_SSL_loaded = true;

   STATUSMESSAGE((_("Successfully loaded '%s' and '%s' - "
                    "SSL authentication is now available."),
                  crypto_dll.c_str(),
                  ssl_dll.c_str()));

   ssl_onceonlyinit();

   return true;

error:
   // Unload the DLLs to avoid assertion failures when calling Load() on them
   // again if we reattempt SSL initialization later (note that Unload() itself
   // doesn't assert and just doesn't do anything if the DLL is not loaded).
   gs_dllSll.Unload();
   gs_dllCrypto.Unload();

   if ( !s_errMsgGiven )
   {
      ERRORMESSAGE((_("SSL authentication is not available.")));

      s_errMsgGiven = true;

      // show the log dialog first
      wxLog::FlushActive();

      MDialog_Message
      (
         _("You can change the locations of the SSL and crypto "
           "libraries in the Helpers page of the preferences dialog\n"
           "if you have these libraries in non default location"
           " or if they have some other names on your system."),
         NULL,
         "SSL tip",
         "SSLLibTip"
      );
   }

   return false;
}

#undef SSL_LOOKUP

#else // !OS_UNIX

bool InitSSL()
{
   ssl_onceonlyinit();

   return true;
}

#endif // OS_UNIX/!OS_UNIX

#else // !USE_SSL

bool InitSSL(void)
{
   static bool s_errMsgGiven = false;
   if ( !s_errMsgGiven )
   {
      s_errMsgGiven = true;

      ERRORMESSAGE((_("This version of the program doesn't support SSL "
                      "authentication.")));
   }

   return false;
}

#endif // USE_SSL/!USE_SSL

