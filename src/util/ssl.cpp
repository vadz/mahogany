///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   util/ssl.cpp
// Purpose:     SSL support functions
// Author:      Karsten Ballüder
// Modified by:
// Created:     24.01.01 (extracted from MailFolderCC.cpp)
// CVS-ID:      $Id$
// Copyright:   (C) 1997-2000 by Karsten Ballüder (ballueder@gmx.net)
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
#if defined(OS_UNIX) || defined(__CYGWIN__)

#include <wx/dynlib.h>

extern const MOption MP_SSL_DLL_CRYPTO;
extern const MOption MP_SSL_DLL_SSL;

/* These functions will serve as stubs for the real openSSL library
   and must be initialised at runtime as c-client actually links
   against these. */

#include <openssl/ssl.h>

// starting from 0.9.6a, OpenSSL uses void, as it should, instead of char
#include <openssl/opensslv.h>
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

#define SSL_LOOKUP(name) \
      stub_##name = (name##_TYPE) gs_dllSll.GetSymbol(wxConvertMB2WX(#name)); \
      if ( !stub_##name ) \
         goto error

#define CRYPTO_LOOKUP(name) \
      stub_##name = (name##_TYPE) gs_dllCrypto.GetSymbol(wxConvertMB2WX(#name)); \
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
SSL_DEF( int,  SSL_pending, (SSL *s), (s) );
SSL_DEF( int,  SSL_library_init, (void ), () );
SSL_DEF_VOID( SSL_load_error_strings, (void ), () );
SSL_DEF( SSL_CTX *,SSL_CTX_new, (SSL_METHOD *meth), (meth) );
SSL_DEF( const char *, SSL_CIPHER_get_name, (SSL_CIPHER *c), (c) );
SSL_DEF( int, SSL_CIPHER_get_bits, (SSL_CIPHER *c, int *alg_bits), (c,alg_bits) );
SSL_DEF( SSL_CIPHER *, SSL_get_current_cipher ,(SSL *s), (s) );
SSL_DEF( int, SSL_get_fd, (SSL *s), (s) );
SSL_DEF( int, SSL_set_fd, (SSL *s, int fd), (s, fd) );
SSL_DEF( int, SSL_get_error, (SSL *s, int ret_code), (s, ret_code) );
SSL_DEF( X509 *, SSL_get_peer_certificate, (SSL *s), (s) );

SSL_DEF_VOID( RAND_seed, (const void *buf,int num), (buf, num) );
SSL_DEF( BIO *, BIO_new_socket, (int sock, int close_flag), (sock, close_flag) );
SSL_DEF( long, SSL_CTX_ctrl, (SSL_CTX *ctx,int cmd, long larg, ssl_parg parg), (ctx,cmd,larg,parg) );
SSL_DEF_VOID( SSL_CTX_set_verify, (SSL_CTX *ctx,int mode, int (*callback)(int, X509_STORE_CTX *)), (ctx,mode,callback) );
SSL_DEF( int, SSL_CTX_load_verify_locations, (SSL_CTX *ctx, const char *CAfile, const char *CApath), (ctx, CAfile, CApath) );
SSL_DEF( int, SSL_CTX_set_default_verify_paths, (SSL_CTX *ctx), (ctx) );
SSL_DEF_VOID( SSL_set_bio, (SSL *s, BIO *rbio,BIO *wbio), (s,rbio,wbio) );
SSL_DEF_VOID( SSL_set_connect_state, (SSL *s), (s) );
SSL_DEF( int, SSL_state, (SSL *ssl), (ssl) );
SSL_DEF( long,    SSL_ctrl, (SSL *ssl,int cmd, long larg, ssl_parg parg), (ssl,cmd,larg,parg) );
SSL_DEF_VOID( ERR_load_crypto_strings, (void), () );
SSL_DEF( SSL_METHOD *,TLSv1_server_method, (void), () );
SSL_DEF( SSL_METHOD *,SSLv23_server_method, (void), () );
SSL_DEF( int, SSL_CTX_set_cipher_list, (SSL_CTX *ctx,const char *str), (ctx,str) );
SSL_DEF( int, SSL_CTX_use_certificate_chain_file, (SSL_CTX *ctx, const char *file), (ctx,file) );
SSL_DEF( int, SSL_CTX_use_RSAPrivateKey_file, (SSL_CTX *ctx, const char *file, int type), (ctx,file,type) );
SSL_DEF_VOID( SSL_CTX_set_tmp_rsa_callback, (SSL_CTX *ctx, RSA *(*cb)(SSL *,int, int)), (ctx,cb) );
SSL_DEF( int,  SSL_accept, (SSL *ssl), (ssl) );
SSL_DEF( int, X509_STORE_CTX_get_error, (X509_STORE_CTX *ctx), (ctx) );
SSL_DEF( const char *, X509_verify_cert_error_string, (long n), (n) );
SSL_DEF( X509 *, X509_STORE_CTX_get_current_cert, (X509_STORE_CTX *ctx), (ctx) );
SSL_DEF( X509_NAME *, X509_get_subject_name, (X509 *a), (a) );
SSL_DEF( char *, X509_NAME_oneline, (X509_NAME *a,char *buf,int size), (a,buf,size) );
SSL_DEF( int, SSL_shutdown, (SSL *s), (s) );
SSL_DEF_VOID( SSL_CTX_free, (SSL_CTX *ctx), (ctx) );
SSL_DEF( RSA *, RSA_generate_key, (int bits, unsigned long e,void (*cb)(int,int,void *),void *cb_arg), (bits,e,cb,cb_arg) );
SSL_DEF(SSL_METHOD *, TLSv1_client_method, (void), () );
SSL_DEF(SSL_METHOD *, SSLv23_client_method, (void), () );

SSL_DEF( unsigned long, ERR_get_error, (void), () );
SSL_DEF( char *, ERR_error_string, (unsigned long e, char *p), (e, p) );

#undef SSL_DEF

} // extern "C"

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

   SSL_LOOKUP(SSL_new );
   SSL_LOOKUP(SSL_free );
   SSL_LOOKUP(SSL_set_rfd );
   SSL_LOOKUP(SSL_set_wfd );
   SSL_LOOKUP(SSL_set_read_ahead );
   SSL_LOOKUP(SSL_connect );
   SSL_LOOKUP(SSL_read );
   SSL_LOOKUP(SSL_write );
   SSL_LOOKUP(SSL_pending );
   SSL_LOOKUP(SSL_library_init );
   SSL_LOOKUP(SSL_load_error_strings );
   SSL_LOOKUP(SSL_CTX_new );
   SSL_LOOKUP(SSL_CIPHER_get_name );
   SSL_LOOKUP(SSL_CIPHER_get_bits);
   SSL_LOOKUP(SSL_get_current_cipher );
   SSL_LOOKUP(SSL_get_fd);
   SSL_LOOKUP(SSL_set_fd);
   SSL_LOOKUP(SSL_get_error);
   SSL_LOOKUP(SSL_get_peer_certificate);
   SSL_LOOKUP(RAND_seed);
   SSL_LOOKUP(BIO_new_socket);
   SSL_LOOKUP(SSL_CTX_ctrl);
   SSL_LOOKUP(SSL_CTX_set_verify);
   SSL_LOOKUP(SSL_CTX_load_verify_locations);
   SSL_LOOKUP(SSL_CTX_set_default_verify_paths);
   SSL_LOOKUP(SSL_set_bio);
   SSL_LOOKUP(SSL_set_connect_state);
   SSL_LOOKUP(SSL_state);
   SSL_LOOKUP(SSL_ctrl);
   SSL_LOOKUP(ERR_load_crypto_strings);
   SSL_LOOKUP(TLSv1_server_method);
   SSL_LOOKUP(SSLv23_server_method);
   SSL_LOOKUP(SSL_CTX_set_cipher_list);
   SSL_LOOKUP(SSL_CTX_use_certificate_chain_file);
   SSL_LOOKUP(SSL_CTX_use_RSAPrivateKey_file);
   SSL_LOOKUP(SSL_CTX_set_tmp_rsa_callback);
   SSL_LOOKUP(SSL_accept);
   SSL_LOOKUP(X509_STORE_CTX_get_error);
   SSL_LOOKUP(X509_verify_cert_error_string);
   SSL_LOOKUP(X509_STORE_CTX_get_current_cert);
   SSL_LOOKUP(X509_get_subject_name);
   SSL_LOOKUP(X509_NAME_oneline);
   SSL_LOOKUP(SSL_shutdown);
   SSL_LOOKUP(SSL_CTX_free);
   SSL_LOOKUP(RSA_generate_key);
   SSL_LOOKUP(TLSv1_client_method );
   SSL_LOOKUP(SSLv23_client_method );

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
         _T("SSL tip"),
         _T("SSLLibTip")
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

