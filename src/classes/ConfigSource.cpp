///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   ConfigSource.cpp
// Purpose:     implementation of ConfigSource and company
// Author:      Vadim Zeitlin
// Modified by:
// Created:     23.08.03
// CVS-ID:      $Id$
// Copyright:   (c) 2003 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M licence
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

#ifdef __GNUG__
   #pragma implementation "ConfigSource.h"
#endif

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include  "Mpch.h"

#ifndef USE_PCH
   #include "Mcommon.h"

   #ifdef  OS_WIN
      #include <wx/msw/regconf.h>
   #else
      #include <wx/confbase.h>
   #endif

   #include <wx/config.h>
   #include <wx/fileconf.h>
   #include <wx/dir.h>
   #include <wx/utils.h>
#endif

#ifdef OS_UNIX
   #include <sys/types.h>
   #include <unistd.h>
   #include <sys/stat.h>
   #include <fcntl.h>
#endif //Unix

#include "ConfigSource.h"

#include "Mdefaults.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// the type of the local config source
#define CONFIG_SOURCE_TYPE_LOCAL "local"

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

class ConfigSourceLocalFactory : public ConfigSourceFactory
{
public:
   virtual const char *GetType() const { return CONFIG_SOURCE_TYPE_LOCAL; }
   virtual ConfigSource *Create(const ConfigSource& config, const String& name);
};

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_CONFIG_SOURCE_TYPE;

// ============================================================================
// ConfigSource implementation
// ============================================================================

// ----------------------------------------------------------------------------
// CreateXXX() methods
// ----------------------------------------------------------------------------

/* static */ ConfigSource *
ConfigSource::CreateDefault(const String& filename)
{
   return new ConfigSourceLocal(filename);
}

/* static */ ConfigSource *
ConfigSource::Create(const ConfigSource& config, const String& name)
{
   // get the type of config source to create
   String type;
   if ( !config.Read(GetOptionName(MP_CONFIG_SOURCE_TYPE), &type) )
   {
      wxLogError(_("Invalid config source \"%s\" without type."), name.c_str());

      return NULL;
   }

   // find the factory for the objects of this type
   ConfigSourceFactory_obj factory(ConfigSourceFactory::Find(type));
   if ( !factory )
   {
      wxLogError(_("Unknown type \"%s\" for config source \"%s\"."),
                 type.c_str(), name.c_str());

      return NULL;
   }

   return factory->Create(config, name);
}

ConfigSource::~ConfigSource()
{
   // nothing to do here
}

// ============================================================================
// ConfigSourceFactory implementation
// ============================================================================

// ----------------------------------------------------------------------------
// ConfigSourceFactory::EnumData implementation
// ----------------------------------------------------------------------------

ConfigSourceFactory::EnumData::EnumData()
{
   m_listing = NULL;
}

void ConfigSourceFactory::EnumData::Reset()
{
   if ( m_listing )
   {
      m_listing->DecRef();
      m_listing = NULL;
   }
}

const MModuleListingEntry *ConfigSourceFactory::EnumData::GetNext()
{
   // retrieve the listing if not done yet
   if ( !m_listing )
   {
      m_listing = MModule::ListAvailableModules(CONFIG_SOURCE_INTERFACE);
      if ( !m_listing )
      {
         // no config source modules at all
         return NULL;
      }

      m_current = 0;
   }

   if ( m_current == m_listing->Count() )
   {
      // no more
      return NULL;
   }

   return &(*m_listing)[m_current++];
}

ConfigSourceFactory::EnumData::~EnumData()
{
   if ( m_listing )
      m_listing->DecRef();
}

// ----------------------------------------------------------------------------
// enumerating the config source factories
// ----------------------------------------------------------------------------

/* static */ ConfigSourceFactory *
ConfigSourceFactory::GetFirst(EnumData& data)
{
   // this may be needed if we reuse the same data for 2 enumerations, one
   // after another
   data.Reset();

   // the first factory in the list is always the local one
   return new ConfigSourceLocalFactory;
}

/* static */ ConfigSourceFactory *
ConfigSourceFactory::GetNext(EnumData& data)
{
   const MModuleListingEntry * const entry = data.GetNext();
   if ( !entry )
      return NULL;

   MModule *module = MModule::LoadModule(entry->GetName());
   if ( !module )
      return NULL;

   ConfigSourceFactory *
      fact = static_cast<ConfigSourceFactoryModule *>(module)->CreateFactory();

   // if the factory has been loaded, it keeps a lock on the module which
   // prevents it from being unloaded here, otherwise we don't need it any more
   module->DecRef();

   return fact;
}

// ----------------------------------------------------------------------------
// retrieving factory by name
// ----------------------------------------------------------------------------

/* static */ ConfigSourceFactory *
ConfigSourceFactory::Find(const String& type)
{
   ConfigSourceFactory::EnumData data;
   for ( ConfigSourceFactory *fact = ConfigSourceFactory::GetFirst(data);
         fact;
         fact = ConfigSourceFactory::GetNext(data) )
   {
      if ( fact->GetType() == type )
         return fact;

      fact->DecRef();
   }

   return NULL;
}

// ============================================================================
// ConfigSourceLocal implementation
// ============================================================================

// ----------------------------------------------------------------------------
// ConfigSourceLocal creation
// ----------------------------------------------------------------------------

ConfigSourceLocal::ConfigSourceLocal(const String& filename, const String& name)
                 : ConfigSource(name), m_config(NULL)
{
   String localFilePath, globalFilePath;

#ifdef OS_UNIX
   // if the filename is empty, use the default one
   if ( localFilePath.empty() )
   {
      localFilePath = wxGetHomeDir();
      if ( localFilePath.empty() )
      {
         // don't create our files in the root directory, try the current one
         // instead
         localFilePath = wxGetCwd();
      }

      if ( !localFilePath.empty() )
      {
         localFilePath << '/';
      }
      //else: it will be in the current one, what else can we do?

      localFilePath << '.' << M_APPLICATIONNAME;

      if ( !wxDir::Exists(localFilePath) )
      {
         if ( !wxMkdir(localFilePath, 0700) )
         {
            wxLogError(_("Cannot create the directory for configuration "
                         "files '%s'."), localFilePath.c_str());
            return;
         }

         wxLogInfo(_("Created directory '%s' for configuration files."),
                   localFilePath.c_str());

         // also create an empty config file with the right permissions:
         String filename = localFilePath + "/config";

         // pass false to Create() to avoid overwriting the existing file
         wxFile file;
         if ( !file.Create(filename, false, wxS_IRUSR | wxS_IWUSR) &
               !wxFile::Exists(filename) )
         {
            wxLogError(_("Could not create initial configuration file."));
         }
      }

      // Check whether other users can write our config dir.
      //
      // This must not be allowed as they could change the behaviour of the
      // program unknowingly to the user!
      struct stat st;
      if ( stat(localFilePath, &st) == 0 )
      {
         if ( st.st_mode & (S_IWGRP | S_IWOTH) )
         {
            // No other user must have write access to the config dir.
            String msg;
            msg.Printf(_("Configuration directory '%s' was writable for other users.\n"
                         "The programs settings might have been changed without "
                         "your knowledge and the passwords stored in your config\n"
                         "file (if any) could have been compromised!\n\n"),
                       localFilePath.c_str());

            if ( chmod(localFilePath, st.st_mode & ~(S_IWGRP | S_IWOTH)) == 0 )
            {
               msg += _("This has been fixed now, the directory is no longer writable for others.");
            }
            else
            {
               msg << String::Format(_("Failed to correct the directory access "
                                       "rights (%s), please do it manually and "
                                       "restart the program!"),
                                     strerror(errno));
            }

            wxLogError(msg);
         }
      }
      else
      {
         wxLogSysError(_("Failed to access the directory '%s' containing "
                         "the configuration files."),
                       localFilePath.c_str());
      }

      localFilePath += "/config";

      // Check whether other users can read our config file.
      //
      // This must not be allowed as we store passwords in it!
      if ( stat(localFilePath, &st) == 0 )
      {
         if ( st.st_mode & (S_IWGRP | S_IWOTH | S_IRGRP | S_IROTH) )
         {
            // No other user must have access to the config file.
            String msg;
            if ( st.st_mode & (S_IWGRP | S_IWOTH) )
            {
               msg.Printf(_("Configuration file %s was writable by other users.\n"
                            "The program settings could have been changed without\n"
                            "your knowledge, please consider reinstalling the "
                            "program!"),
                          localFilePath.c_str());
            }

            if ( st.st_mode & (S_IRGRP | S_IROTH) )
            {
               if ( !msg.empty() )
                  msg += "\n\n";

               msg += String::Format
                      (
                        _("Configuration file '%s' was readable for other users.\n"
                          "Passwords may have been compromised, please "
                          "consider changing them!"),
                        localFilePath.c_str()
                      );
            }

            msg += "\n\n";

            if ( chmod(localFilePath, S_IRUSR | S_IWUSR) == 0 )
            {
               msg += _("This has been fixed now, the file is no longer readable for others.");
            }
            else
            {
               msg << String::Format(_("The attempt to make the file unreadable "
                                       "for others has failed: %s"),
                                     strerror(errno));
            }

            wxLogError(msg);
         }
      }
      //else: not an error, file may be simply not there yet
   }

   // look for the global config file in the following places in order:
   //    1. compile-time specified installation dir
   //    2. run-time specified installation dir
   //    3. default installation dir
   String globalFileName;
   globalFileName << M_APPLICATIONNAME << ".conf";
   globalFilePath << M_PREFIX << "/etc/" << globalFileName;
   if ( !wxFileExists(globalFilePath) )
   {
      const char *dir = getenv("MAHOGANY_DIR");
      if ( dir )
      {
         globalFilePath.clear();
         globalFilePath << dir << "/etc/" << globalFileName;
      }
   }

   if ( !wxFileExists(globalFilePath) )
   {
      // wxConfig will look for it in the default location(s)
      globalFilePath = globalFileName;
   }
#elif defined(OS_WIN)
   // under Windows we just use the registry if the filename is not specified
   if ( filename.empty() )
   {
      // don't give explicit name, but rather use the default logic (it's
      // perfectly ok, for the registry case our keys are under vendor\appname)
      m_config = new wxRegConfig
                     (
                        M_APPLICATIONNAME,
                        M_VENDORNAME,
                        _T(""),
                        _T(""),
                        wxCONFIG_USE_LOCAL_FILE | wxCONFIG_USE_GLOBAL_FILE
                     );

      // skip wxFileConfig creation below
      return;
   }
#else  // !Windows, !Unix
   #error "Don't know default config file location for this platform"
#endif // OS

   wxFileConfig *fileconf = new wxFileConfig
                                (
                                    M_APPLICATIONNAME,
                                    M_VENDORNAME,
                                    localFilePath,
                                    globalFilePath,
                                    wxCONFIG_USE_LOCAL_FILE |
                                    (globalFilePath.empty()
                                       ? 0 : wxCONFIG_USE_GLOBAL_FILE)
                                );

   // don't give the others even read access to our config file as it stores,
   // among other things, the passwords
   fileconf->SetUmask(0077);

   m_config = fileconf;
}

// ----------------------------------------------------------------------------
// ConfigSourceLocal simple accessors
// ----------------------------------------------------------------------------

bool ConfigSourceLocal::IsOk() const
{
   return m_config != NULL;
}

bool ConfigSourceLocal::IsLocal() const
{
   return true;
}

// ----------------------------------------------------------------------------
// ConfigSourceLocal other methods: all simply forwarded to m_config
// ----------------------------------------------------------------------------

bool ConfigSourceLocal::Read(const String& name, String *value) const
{
   return m_config->Read(name, value);
}

bool ConfigSourceLocal::Read(const String& name, long *value) const
{
   return m_config->Read(name, value);
}

bool ConfigSourceLocal::Write(const String& name, const String& value)
{
   return m_config->Write(name, value);
}

bool ConfigSourceLocal::Write(const String& name, long value)
{
   return m_config->Write(name, value);
}

bool ConfigSourceLocal::GetFirstGroup(String& group, long& cookie) const
{
   return m_config->GetFirstGroup(group, cookie);
}

bool ConfigSourceLocal::GetNextGroup(String& group, long& cookie) const
{
   return m_config->GetNextGroup(group, cookie);
}

bool ConfigSourceLocal::GetFirstEntry(String& entry, long& cookie) const
{
   return m_config->GetFirstEntry(entry, cookie);
}

bool ConfigSourceLocal::GetNextEntry(String& entry, long& cookie) const
{
   return m_config->GetNextEntry(entry, cookie);
}

bool ConfigSourceLocal::DeleteEntry(const String& name)
{
   return DeleteEntry(name);
}

bool ConfigSourceLocal::DeleteGroup(const String& name)
{
   return DeleteGroup(name);
}

// ============================================================================
// ConfigSourceLocalFactory implementation
// ============================================================================

ConfigSource *
ConfigSourceLocalFactory::Create(const ConfigSource& config, const String& name)
{
   String filename;
   if ( !config.Read(_T(""), &filename) )
   {
      wxLogError(_("No filename for local config source \"%s\"."),
                 name.c_str());
      return NULL;
   }

   ConfigSource *configNew = new ConfigSourceLocal(filename, name);
   if ( !configNew->IsOk() )
   {
      configNew->DecRef();
      configNew = NULL;
   }

   return configNew;
}

