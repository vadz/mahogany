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
   #include "Mdefaults.h"

   #include <wx/utils.h>               // for wxGetHomeDir

   #include "Profile.h"                // for M_PROFILE_CONFIG_SECTION
#endif // USE_PCH

#include <wx/confbase.h>               // for wxConfigBase
#ifdef OS_WIN
   #include <wx/msw/regconf.h>
#endif

#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/fileconf.h>               // for wxFileConfig

#ifdef OS_UNIX
   #include <sys/types.h>
   #include <unistd.h>
   #include <sys/stat.h>
   #include <fcntl.h>
#endif //Unix

#include <errno.h>

#include "ConfigSource.h"
#include "ConfigSourceLocal.h"

class MOption;

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

class ConfigSourceLocalFactory : public ConfigSourceFactory
{
public:
   // the type of the local config source
   static const wxChar *Type() { return gettext_noop("file"); }

   virtual const wxChar *GetType() const { return Type(); }
   virtual ConfigSource *Create(const ConfigSource& config, const String& name);
   virtual bool
       Save(ConfigSource& config, const String& name, const String& spec);

private:
   // the path used for storing the file name in config
   static const wxChar *FileNamePath() { return _T("/FileName"); }
};

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_CONFIG_SOURCE_TYPE;

// ----------------------------------------------------------------------------
// globals
// ----------------------------------------------------------------------------

// the main config source
static ConfigSource *gs_configSourceGlobal = NULL;

// ============================================================================
// ConfigSource implementation
// ============================================================================

// ----------------------------------------------------------------------------
// CreateXXX() methods
// ----------------------------------------------------------------------------

/* static */ ConfigSource *
ConfigSource::CreateDefault(const String& filename)
{
   // FIXME-MT
   if ( !gs_configSourceGlobal )
   {
      gs_configSourceGlobal = ConfigSourceLocal::CreateDefault(filename);
   }
   else
   {
      gs_configSourceGlobal->IncRef();
   }

   return gs_configSourceGlobal;
}

/* static */ ConfigSource *
ConfigSource::Create(const ConfigSource& config, const String& name)
{
   // get the type of config source to create
   String path(name);
   path << _T('/') << GetOptionName(MP_CONFIG_SOURCE_TYPE);

   String type;
   if ( !config.Read(path, &type) )
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

   ConfigSource *configNew = factory->Create(config, name);
   if ( configNew && !configNew->IsOk() )
   {
      // creation failed, don't return an invalid object
      configNew->DecRef();
      configNew = NULL;
   }

   return configNew;
}

ConfigSource::~ConfigSource()
{
   // nothing to do here
}

/* static */ bool
ConfigSource::Copy(ConfigSource& configDst,
                   const ConfigSource& configSrc,
                   const String& pathDst,
                   const String& pathSrc)
{
   ConfigSource::EnumData cookie;
   String name;

   const String pathSrcSlash(pathSrc + _T('/')),
                pathDstSlash(pathDst + _T('/'));

   // first copy all the entries
   bool cont = configSrc.GetFirstEntry(pathSrc, name, cookie);
   while ( cont )
   {
      // const_cast is ok because we don't copy to ourselves but to a different
      // configDst
      if ( !((ConfigSource&)configSrc).CopyEntry(pathSrcSlash + name,
                                                 pathDstSlash + name,
                                                 &configDst) )
      {
         return false;
      }

      cont = configSrc.GetNextEntry(name, cookie);
   }

   // and then (recursively) copy all subgroups
   cont = configSrc.GetFirstGroup(pathSrc, name, cookie);
   while ( cont )
   {
      if ( !Copy(configDst, configSrc,
                 pathSrcSlash + name, pathDstSlash + name) )
      {
         return false;
      }

      cont = configSrc.GetNextGroup(name, cookie);
   }

   return true;
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

ConfigSourceLocal::ConfigSourceLocal(const String& name)
                 : ConfigSource(name, ConfigSourceLocalFactory::Type())
{
   m_config = NULL;
}

bool ConfigSourceLocal::InitDefault(const String& filename)
{
   bool rc = false;

   String localFilePath = filename,
          globalFilePath;

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
         localFilePath << _T('/');
      }
      //else: it will be in the current one, what else can we do?

      localFilePath << _T('.') << M_APPLICATIONNAME;

      if ( !wxDir::Exists(localFilePath) )
      {
         if ( !wxMkdir(localFilePath, 0700) )
         {
            wxLogError(_("Cannot create the directory for configuration "
                         "files '%s'."), localFilePath.c_str());
            return NULL;
         }

         wxLogInfo(_("Created directory '%s' for configuration files."),
                   localFilePath.c_str());

         // also create an empty config file with the right permissions:
         String filename;
         filename << localFilePath << DIR_SEPARATOR << _T("config");

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
      if ( wxStat(localFilePath, &st) == 0 )
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

            if ( chmod(localFilePath.fn_str(), st.st_mode & ~(S_IWGRP | S_IWOTH)) == 0 )
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

      localFilePath << DIR_SEPARATOR << _T("config");

      // Check whether other users can read our config file.
      //
      // This must not be allowed as we store passwords in it!
      if ( wxStat(localFilePath, &st) == 0 )
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
                  msg += _T("\n\n");

               msg += String::Format
                      (
                        _("Configuration file '%s' was readable for other users.\n"
                          "Passwords may have been compromised, please "
                          "consider changing them!"),
                        localFilePath.c_str()
                      );
            }

            msg += _T("\n\n");

            if ( chmod(localFilePath.fn_str(), S_IRUSR | S_IWUSR) == 0 )
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
   globalFileName << M_APPLICATIONNAME << _T(".conf");
   globalFilePath << M_PREFIX << DIR_SEPARATOR << _T("etc") << DIR_SEPARATOR << globalFileName;
   if ( !wxFileExists(globalFilePath) )
   {
      const wxChar *dir = wxGetenv(_T("MAHOGANY_DIR"));
      if ( dir )
      {
         globalFilePath.clear();
         globalFilePath << dir << DIR_SEPARATOR << _T("etc") << DIR_SEPARATOR << globalFileName;
      }
   }

   if ( !wxFileExists(globalFilePath) )
   {
      // wxConfig will look for it in the default location(s)
      globalFilePath = globalFileName;
   }
#elif defined(OS_WIN)
   // under Windows we just use the registry if the filename is not specified
   if ( localFilePath.empty() )
   {
      rc = InitRegistry();

      // one extra complication: the user wants to always want to use config
      // file instead of the registry, this is indicated by the presence of
      if ( rc )
      {
         localFilePath = GetFilePath(m_config);
         if ( !localFilePath.empty() )
         {
            // we want to use wxFileConfig finally...
            delete m_config;
            m_config = NULL; // not really needed now, but safer
            rc = false;
         }
         else // do use wxRegConfig created above
         {
            // as for wxFileConfig, see comment in InitFile()
            m_config->SetExpandEnvVars(false);
         }
      }
   }
#else  // !Windows, !Unix
   #error "Don't know default config file location for this platform"
#endif // OS

   if ( !rc )
   {
      rc = InitFile(localFilePath, globalFilePath);
   }

   return rc;
}

bool
ConfigSourceLocal::InitFile(const String& localFilePath,
                            const String& globalFilePath)
{
   ASSERT_MSG( !localFilePath.empty(),
               _T("invalid local config file path") );

   // make the file name absolute if it isn't already or, if it is, remember
   // its path to use it as base for the other config files (we can't use
   // MAppBase::GetLocalDir() here as it isn't set yet)
   static String s_pathConfigFiles;

   wxFileName fn(localFilePath);
   if ( fn.IsAbsolute() )
   {
      if ( s_pathConfigFiles.empty() )
         s_pathConfigFiles = fn.GetPath();
   }
   else // relative path
   {
      if ( !s_pathConfigFiles.empty() )
         fn.MakeAbsolute(s_pathConfigFiles);
   }

   wxFileConfig *fileconf = new wxFileConfig
                                (
                                    M_APPLICATIONNAME,
                                    M_VENDORNAME,
                                    fn.GetFullPath(),
                                    globalFilePath,
                                    wxCONFIG_USE_LOCAL_FILE |
                                    (globalFilePath.empty()
                                       ? 0 : wxCONFIG_USE_GLOBAL_FILE)
                                );

   // don't give the others even read access to our config file as it stores,
   // among other things, the passwords
   fileconf->SetUmask(0077);

   // disable wxConfig built-in environment variables expansion as we do it
   // ourselves at Profile level and want to control it from there
   fileconf->SetExpandEnvVars(false);

   DoInit(fileconf, localFilePath);

   return true;
}


#ifdef OS_WIN

/* static */
wxConfigBase *ConfigSourceLocal::DoCreateRegConfig()
{
   // don't give explicit name, but rather use the default logic (it's
   // perfectly ok, for the registry case our keys are under vendor\appname)
   return new wxRegConfig
              (
                  M_APPLICATIONNAME,
                  M_VENDORNAME,
                  _T(""),
                  _T(""),
                  wxCONFIG_USE_LOCAL_FILE |
                  wxCONFIG_USE_GLOBAL_FILE
              );
}

bool ConfigSourceLocal::InitRegistry()
{
   wxConfigBase *regconf = DoCreateRegConfig();

   // don't call SetExpandEnvVars() here: this allows to use env vars in config
   // file value

   DoInit(regconf);

   return true;
}

/* static */
String ConfigSourceLocal::GetFilePath(wxConfigBase *config)
{
   String path;
   config->Read(GetConfigFileKey(), &path);
   return path;
}

/* static */
void ConfigSourceLocal::UseFile(const String& filename)
{
   wxConfigBase *config = DoCreateRegConfig();
   if ( filename.empty() )
      config->DeleteEntry(GetConfigFileKey());
   else
      config->Write(GetConfigFileKey(), filename);
   delete config;
}

/* static */
String ConfigSourceLocal::GetFilePath()
{
   wxConfigBase *config = DoCreateRegConfig();
   String path = GetFilePath(config);
   delete config;

   return path;
}

#endif // OS_WIN

ConfigSourceLocal::~ConfigSourceLocal()
{
   if ( this == gs_configSourceGlobal )
      gs_configSourceGlobal = NULL;

   delete m_config;
}

// ----------------------------------------------------------------------------
// ConfigSourceLocal simple accessors
// ----------------------------------------------------------------------------

String ConfigSourceLocal::GetSpec() const
{
   return m_path;
}

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

bool ConfigSourceLocal::Flush()
{
   return m_config->Flush();
}

bool
ConfigSourceLocal::GetFirstGroup(const String& key,
                                 String& group,
                                 EnumData& cookie) const
{
   cookie.Key() = key;

   m_config->SetPath(key);

   return m_config->GetFirstGroup(group, cookie.Cookie());
}

bool ConfigSourceLocal::GetNextGroup(String& group, EnumData& cookie) const
{
   m_config->SetPath(cookie.Key());

   return m_config->GetNextGroup(group, cookie.Cookie());
}

bool
ConfigSourceLocal::GetFirstEntry(const String& key,
                                 String& entry,
                                 EnumData& cookie) const
{
   cookie.Key() = key;

   m_config->SetPath(key);

   return m_config->GetFirstEntry(entry, cookie.Cookie());
}

bool ConfigSourceLocal::GetNextEntry(String& entry, EnumData& cookie) const
{
   m_config->SetPath(cookie.Key());

   return m_config->GetNextEntry(entry, cookie.Cookie());
}

bool ConfigSourceLocal::HasGroup(const String& path) const
{
   return m_config->HasGroup(path);
}

bool ConfigSourceLocal::HasEntry(const String& path) const
{
   return m_config->HasEntry(path);
}

bool ConfigSourceLocal::DeleteEntry(const String& name)
{
   return m_config->DeleteEntry(name);
}

bool ConfigSourceLocal::DeleteGroup(const String& name)
{
   return m_config->DeleteGroup(name);
}

bool
ConfigSourceLocal::CopyEntry(const String& nameSrc,
                             const String& nameDst,
                             ConfigSource *configDstOrig)
{
   ConfigSource *configDst = configDstOrig ? configDstOrig : this;

   bool rc = true;

   if ( m_config->HasEntry(nameSrc) )
   {
      switch ( m_config->GetEntryType(nameSrc) )
      {
         case wxConfigBase::Type_Unknown:
         case wxConfigBase::Type_Float:
         case wxConfigBase::Type_Boolean:
            wxFAIL_MSG(_T("unexpected entry type"));
            // fall through

         case wxConfigBase::Type_String:
            // GetEntryType() returns string for all wxFileConfig entries, so
            // try to correct it here
            {
               wxString val;
               if ( m_config->Read(nameSrc, &val) )
               {
                  long l;
                  if ( val.ToLong(&l) )
                     rc = configDst->Write(nameDst, l);
                  else
                     rc = configDst->Write(nameDst, val);
               }
            }
            break;

         case wxConfigBase::Type_Integer:
            {
               long l;
               rc = m_config->Read(nameSrc, &l) && configDst->Write(nameDst, l);
            }
            break;
      }
   }

   return rc;
}

bool
ConfigSourceLocal::RenameGroup(const String& pathOld, const String& nameNew)
{
   wxConfigPathChanger path(m_config, pathOld);

   return m_config->RenameGroup(path.Name(), nameNew);
}

// ============================================================================
// ConfigSourceLocalFactory implementation
// ============================================================================

ConfigSource *
ConfigSourceLocalFactory::Create(const ConfigSource& config, const String& name)
{
   String filename;
   if ( !config.Read(name + FileNamePath(), &filename) )
   {
      wxLogError(_("No filename for local config source \"%s\"."),
                 name.c_str());
      return NULL;
   }

   ConfigSource *configNew = ConfigSourceLocal::CreateFile(filename, name);
   if ( !configNew->IsOk() )
   {
      configNew->DecRef();
      configNew = NULL;
   }

   return configNew;
}

bool
ConfigSourceLocalFactory::Save(ConfigSource& config,
                               const String& name,
                               const String& spec)
{
   // TODO: check file name validity?

   return config.Write(name + FileNamePath(), spec);
}

