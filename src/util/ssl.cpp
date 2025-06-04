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
#endif // USE_PCH

bool InitSSL()
{
   ssl_onceonlyinit();

   return true;
}

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

