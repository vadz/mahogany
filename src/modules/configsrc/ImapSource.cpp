///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   modules/configsrc/ImapSource.cpp
// Purpose:     implementation of ConfigSource using data from IMAP server
// Author:      Vadim Zeitlin
// Modified by:
// Created:     26.08.03
// CVS-ID:      $Id$
// Copyright:   (c) 2003 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M licence
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include  "Mpch.h"

#ifndef USE_PCH
   #include "Mcommon.h"

   #include <wx/confbase.h>
#endif

#include <wx/fileconf.h>

#include "ConfigSource.h"

// ----------------------------------------------------------------------------
// ConfigSourceIMAP: ConfigSource implementation storing data on IMAP server
// ----------------------------------------------------------------------------

class ConfigSourceIMAP : public ConfigSource
{
public:
   // standard ctor used by our factory
   ConfigSourceIMAP(const ConfigSource& config, const String& name);

   // dtor saves data (if modified) to IMAP server
   virtual ~ConfigSourceIMAP();


   // implement base class pure virtuals
   virtual bool IsOk() const;
   virtual bool IsLocal() const;
   virtual bool Read(const String& name, String *value) const;
   virtual bool Read(const String& name, long *value) const;
   virtual bool Write(const String& name, const String& value);
   virtual bool Write(const String& name, long value);
   virtual bool Flush();
   virtual bool GetFirstGroup(const String& key,
                              String& group,
                              EnumData& cookie) const;
   virtual bool GetNextGroup(String& group, EnumData& cookie) const;
   virtual bool GetFirstEntry(const String& key,
                              String& entry,
                              EnumData& cookie) const;
   virtual bool GetNextEntry(String& entry, EnumData& cookie) const;
   virtual bool HasGroup(const String& path) const;
   virtual bool HasEntry(const String& path) const;
   virtual bool DeleteEntry(const String& name);
   virtual bool DeleteGroup(const String& name);
   virtual bool CopyEntry(const String& nameSrc, const String& nameDst);
   virtual bool RenameGroup(const String& nameOld, const String& nameNew);

public:
   // IMAP folder containing our data
   MFolder *m_folder;

   // true if we were modified
   bool m_isDirty;
};

IMPLEMENT_CONFIG_SOURCE(ConfigSourceIMAP,
                        "imap",
                        _("Settings stored on an IMAP server."),
                        _T("(c) 2003 Vadim Zeitlin <vadim@wxwindows.org>"))

// ============================================================================
// ConfigSourceIMAP implementation
// ============================================================================

// ----------------------------------------------------------------------------
// ConfigSourceIMAP construction
// ----------------------------------------------------------------------------

ConfigSourceIMAP::ConfigSourceIMAP(const ConfigSource& config,
                                   const String& name)
                : ConfigSource(name)
{
   m_dirty = false;
   m_folder = MFolder::CreateTemp();
}

ConfigSourceIMAP::~ConfigSourceIMAP()
{
   Flush();

   if ( m_folder )
      m_folder->DecRef();
}

// ----------------------------------------------------------------------------
// ConfigSourceIMAP writing data
// ----------------------------------------------------------------------------

bool
ConfigSourceIMAP::Flush()
{
   if ( m_dirty )
   {
      m_dirty = false;
   }
}
