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

#ifdef USE_SSL
#include "Mdefaults.h"
#include "Mcommon.h"
#include "Profile.h"
#include "MApplication.h"

#include <wx/dynlib.h>

/* These functions will serve as stubs for the real openSSL library
   and must be initialised at runtime as c-client actually links
   against these. */

#include <openssl/ssl.h>
/* This is our interface to the library and auth_ssl.c in c-client
   which are all in "C" */
extern "C" {

extern long ssl_init_Mahogany()
#if 1 // VZ: my test code
{
   extern void ssl_onceonlyinit(void);

   ssl_onceonlyinit();

   return 0;
}
#else
;
#endif


#define SSL_DEF(returnval, name, args) \
   typedef returnval (* name##_TYPE) args ; \
   name##_TYPE stub_##name = NULL

#define SSL_LOOKUP(name) \
      stub_##name = (name##_TYPE) wxDllLoader::GetSymbol(slldll, #name); \
      if(stub_##name == NULL) success = FALSE
#define CRYPTO_LOOKUP(name) \
      stub_##name = (name##_TYPE) wxDllLoader::GetSymbol(cryptodll, #name); \
      if(stub_##name == NULL) success = FALSE

SSL_DEF( SSL *, SSL_new, (SSL_CTX *ctx) );
SSL_DEF( void, SSL_free, (SSL *ssl) );
SSL_DEF( int,  SSL_set_rfd, (SSL *s, int fd) );
SSL_DEF( int,  SSL_set_wfd, (SSL *s, int fd) );
SSL_DEF( void, SSL_set_read_ahead, (SSL *s, int yes) );
SSL_DEF( int,  SSL_connect, (SSL *ssl) );
SSL_DEF( int,  SSL_read, (SSL *ssl,char *buf,int num) );
SSL_DEF( int,  SSL_write, (SSL *ssl,const char *buf,int num) );
SSL_DEF( int,  SSL_pending, (SSL *s) );
SSL_DEF( int,  SSL_library_init, (void ) );
SSL_DEF( void, SSL_load_error_strings, (void ) );
SSL_DEF( SSL_CTX *,SSL_CTX_new, (SSL_METHOD *meth) );
SSL_DEF( unsigned long, ERR_get_error, (void) );
SSL_DEF( char *, ERR_error_string, (unsigned long e, char *p));
//extern SSL_get_cipher_bits();
SSL_DEF( const char *, SSL_CIPHER_get_name, (SSL_CIPHER *c) );
SSL_DEF( int, SSL_CIPHER_get_bits, (SSL_CIPHER *c, int *alg_bits) );
//extern SSL_get_cipher();
SSL_DEF( SSL_CIPHER *, SSL_get_current_cipher ,(SSL *s) );
#   if defined(SSLV3ONLYSERVER) && !defined(TLSV1ONLYSERVER)
SSL_DEF(SSL_METHOD *, SSLv3_client_method, (void) );
#   elif defined(TLSV1ONLYSERVER) && !defined(SSLV3ONLYSERVER)
SSL_DEF(int, TLSv1_client_method, () );
#   else
SSL_DEF(SSL_METHOD *, SSLv23_client_method, (void) );
#   endif

#undef SSL_DEF

SSL     * SSL_new(SSL_CTX *ctx)
{ return (*stub_SSL_new)(ctx); }
void  SSL_free(SSL *ssl)
{ (*stub_SSL_free)(ssl); }
int  SSL_set_rfd(SSL *s, int fd)
{ return (*stub_SSL_set_rfd)(s,fd); }
int  SSL_set_wfd(SSL *s, int fd)
{ return (*stub_SSL_set_wfd)(s,fd); }
void  SSL_set_read_ahead(SSL *s, int yes)
{ (*stub_SSL_set_read_ahead)(s,yes); }
int   SSL_connect(SSL *ssl)
{ return (*stub_SSL_connect)(ssl); }
int   SSL_read(SSL *ssl,char * buf, int num)
{ return (*stub_SSL_read)(ssl, buf, num); }
int   SSL_write(SSL *ssl,const char *buf,int num)
{ return (*stub_SSL_write)(ssl, buf, num); }
int  SSL_pending(SSL *s)
{ return (*stub_SSL_pending)(s); }
int       SSL_library_init(void )
{ return (*stub_SSL_library_init)(); }
void  SSL_load_error_strings(void )
{ (*stub_SSL_load_error_strings)(); }
SSL_CTX * SSL_CTX_new(SSL_METHOD *meth)
{ return (*stub_SSL_CTX_new)(meth); }
unsigned long ERR_get_error(void)
{ return (*stub_ERR_get_error)(); }
char * ERR_error_string(unsigned long e, char *p)
{ return (*stub_ERR_error_string)(e,p); }
const char * SSL_CIPHER_get_name(SSL_CIPHER *c)
{ return (*stub_SSL_CIPHER_get_name)(c); }
SSL_CIPHER * SSL_get_current_cipher(SSL *s)
{ return (*stub_SSL_get_current_cipher)(s); }
int SSL_CIPHER_get_bits(SSL_CIPHER *c, int *alg_bits)
{
  return (*stub_SSL_CIPHER_get_bits)(c,alg_bits);
}


#   if defined(SSLV3ONLYSERVER) && !defined(TLSV1ONLYSERVER)
SSL_METHOD *  SSLv3_client_method(void)
{ return (*stub_SSLv3_client_method)(); }
#   elif defined(TLSV1ONLYSERVER) && !defined(SSLV3ONLYSERVER)
int TLSv1_client_method(void)
{ (* stub_TLSv1_client_method)(); }
#   else
SSL_METHOD * SSLv23_client_method(void)
{ return (* stub_SSLv23_client_method)(); }
#   endif

} // extern "C"

bool gs_SSL_loaded = FALSE;
bool gs_SSL_available = FALSE;

bool InitSSL(void) /* FIXME: MT */
{
   if(gs_SSL_loaded)
      return gs_SSL_available;

   String ssl_dll = READ_APPCONFIG(MP_SSL_DLL_SSL);
   String crypto_dll = READ_APPCONFIG(MP_SSL_DLL_CRYPTO);

   STATUSMESSAGE((_("Trying to load '%s' and '%s'..."),
                  crypto_dll.c_str(),
                  ssl_dll.c_str()));

   bool success = FALSE;
   wxDllType cryptodll = wxDllLoader::LoadLibrary(crypto_dll, &success);
   if(! success) return FALSE;
   wxDllType slldll = wxDllLoader::LoadLibrary(ssl_dll, &success);
   if(! success) return FALSE;

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
   SSL_LOOKUP(SSL_get_current_cipher );
   CRYPTO_LOOKUP(ERR_get_error);
   CRYPTO_LOOKUP(ERR_error_string);
   SSL_LOOKUP(SSL_CIPHER_get_bits);
#   if defined(SSLV3ONLYSERVER) && !defined(TLSV1ONLYSERVER)
   SSL_LOOKUP(SSLv3_client_method );
#   elif defined(TLSV1ONLYSERVER) && !defined(SSLV3ONLYSERVER)
   SSL_LOOKUP(TLSv1_client_method );
#else
   SSL_LOOKUP(SSLv23_client_method );
#   endif
   gs_SSL_available = success;
   gs_SSL_loaded = TRUE;

   if(success) // only now is it safe to call this
   {
      STATUSMESSAGE((_("Successfully loaded '%s' and '%s' - "
                       "SSL authentification now available."),
                     crypto_dll.c_str(),
                     ssl_dll.c_str()));

      ssl_init_Mahogany();
   }
   return success;
}

#undef SSL_LOOKUP

#endif // USE_SSL

