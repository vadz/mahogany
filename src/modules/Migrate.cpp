//////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   modules/Migrate.cpp: the "Migrate tool" Mahogany plugin
// Purpose:     implements functionality of the "Migrate" plugin
// Author:      Vadim Zeitlin
// Modified by:
// Created:     08.10.02
// CVS-ID:      $Id$
// Copyright:   (C) 2002 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#ifdef __GNUG__
   #pragma implementation "MFPool.h"
#endif

#include  "Mpch.h"

#ifndef USE_PCH
   #include "Mcommon.h"

   #include "gui/wxMainFrame.h"
#endif // USE_PCH

#include "MModule.h"

// ----------------------------------------------------------------------------
// MigrateModule: implementation of MModule by this plugin
// ----------------------------------------------------------------------------

class MigrateModule : public MModule
{
public:
   MigrateModule(MInterface *minterface);

   virtual int Entry(int arg, ...);

private:
   // add a menu entry for us to the main frame menu, return true if ok
   bool RegisterWithMainFrame();

   // the main worker function
   bool DoMigrate();

   MMODULE_DEFINE();
};

// ============================================================================
// MigrateModule implementation
// ============================================================================

MMODULE_BEGIN_IMPLEMENT(MigrateModule,
                        "Migrate",
                        "Migrate",
                        "Migration tool",
                        "1.00")
   MMODULE_PROP("description",
                _("Allows to migrate entire IMAP server mail hierarchy to "
                  "another IMAP server or local storage."))
   MMODULE_PROP("author", "Vadim Zeitlin <vadim@wxwindows.org>")
MMODULE_END_IMPLEMENT(MigrateModule)

/* static */
MModule *
MigrateModule::Init(int verMajor, int verMinor, int verRelease,
                    MInterface *minterface, int *errorCode)
{
   if ( !MMODULE_SAME_VERSION(verMajor, verMinor, verRelease))
   {
      if( errorCode )
         *errorCode = MMODULE_ERR_INCOMPATIBLE_VERSIONS;
      return NULL;
   }

   return new MigrateModule(minterface);
}

MigrateModule::MigrateModule(MInterface *minterface)
{
   SetMInterface(minterface);
}

int
MigrateModule::Entry(int arg, ...)
{
   switch( arg )
   {
      case MMOD_FUNC_INIT:
         return RegisterWithMainFrame() ? 0 : -1;

      // Menu event
      case MMOD_FUNC_MENUEVENT:
      {
         va_list ap;
         va_start(ap, arg);
         int id = va_arg(ap, int);
         va_end(ap);

         if ( id == WXMENU_MODULES_MIGRATE_DO )
            return DoMigrate() ? 0 : -1;

         FAIL_MSG( _T("unexpected menu event in migrate module") );
         // fall through
      }

      default:
         return 0;
   }
}

bool
MigrateModule::RegisterWithMainFrame()
{
   MAppBase *mapp = m_MInterface->GetMApplication();
   CHECK( mapp, false, _T("can't init migration module -- no app object") );

   wxMFrame *mframe = mapp->TopLevelFrame();
   CHECK( mframe, false, _T("can't init migration module -- no main frame") );

   ((wxMainFrame *)mframe)->AddModulesMenu
                            (
                             _("&Migrate..."),
                             _("Migrate IMAP server contents"),
                             WXMENU_MODULES_MIGRATE_DO
                            );

   return true;
}

bool
MigrateModule::DoMigrate()
{
   return 0;
}

