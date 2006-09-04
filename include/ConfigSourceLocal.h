///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   ConfigSourceLocal.h
// Purpose:     declaration of ConfigSourceLocal class
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2005-07-05 (extracted from ConfigSource.h)
// CVS-ID:      $Id$
// Copyright:   (c) 2003-2005 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _M_CONFIGSOURCELOCAL_H_
#define _M_CONFIGSOURCELOCAL_H_

#include "ConfigSource.h"

#ifndef USE_PCH
#ifdef __WINE__
   #include "Profile.h"                // for M_PROFILE_CONFIG_SECTION
#endif // __WINE__
#endif // USE_PCH

/**
   ConfigSourceLocal uses wxConfig to implement ConfigSource.

   This class uses wxFileConfig or wxRegConfig if we're running under Windows
   and the filename specified in the ctor is empty. In any case, it uses a
   local file/whatever and this explains its name (and also the fact that
   ConfigSourceConfig would have been really ugly).
 */
class ConfigSourceLocal : public ConfigSource
{
public:
   /**
      Create the default local config.

      @sa ConfigSource::CreateDefault

      @param filename the name of config file or empty to use the default one
    */
   static ConfigSourceLocal *CreateDefault(const String& filename = wxEmptyString)
   {
      ConfigSourceLocal *config = new ConfigSourceLocal();
      if ( !config->InitDefault(filename) )
      {
         delete config;
         config = NULL;
      }

      return config;
   }

   /**
      Create the config source associated with the given file.

      Unlike CreateDefault() above, this method never uses registry. Also, the
      file name must not be empty here.
    */
   static ConfigSourceLocal *CreateFile(const String& filename,
                                        const String& name = wxEmptyString)
   {
      ConfigSourceLocal *config = new ConfigSourceLocal(name);
      if ( !config->InitFile(filename) )
      {
         delete config;
         config = NULL;
      }

      return config;
   }

#ifdef OS_WIN
   /**
      Create the registry config source.

      This shouldn't be normally used explicitly, use CreateDefault() instead.
    */
   static ConfigSourceLocal *CreateRegistry()
   {
      ConfigSourceLocal *config = new ConfigSourceLocal();
      if ( !config->InitRegistry() )
      {
         delete config;
         config = NULL;
      }

      return config;
   }

   /**
      Configure CreateDefault() to use a file instead of the registry.

      The value of config file name is always stored in the registry as this is
      where we look first, hence if we're already using a config file and not
      registry we can't use the usual ConfigSource or Profile methods to write
      it but have to use this special function.

      @sa GetFilePath()

      @param filename if not empty, use the file with this name instead of the
                      registry, otherwise use the registry
    */
   static void UseFile(const String& filename);

   /**
      Return the name of the config file used by CreateDefault() or an empty
      string if we're using the registry.
    */
   static String GetFilePath();
#endif // OS_WIN


   virtual ~ConfigSourceLocal();

   // implement base class pure virtuals
   virtual String GetSpec() const;
   virtual bool IsOk() const;
   virtual bool IsLocal() const;
   virtual bool Read(const String& name, String *value) const;
   virtual bool Read(const String& name, long *value) const;
   virtual bool Write(const String& name, const String& value);
   virtual bool Write(const String& name, long value);
   virtual bool Flush();
   virtual bool GetFirstGroup(const String& key,
                                 String& group, EnumData& cookie) const;
   virtual bool GetNextGroup(String& group, EnumData& cookie) const;
   virtual bool GetFirstEntry(const String& key,
                                 String& entry, EnumData& cookie) const;
   virtual bool GetNextEntry(String& entry, EnumData& cookie) const;
   virtual bool HasGroup(const String& path) const;
   virtual bool HasEntry(const String& path) const;
   virtual bool DeleteEntry(const String& name);
   virtual bool DeleteGroup(const String& name);
   virtual bool CopyEntry(const String& nameSrc,
                          const String& nameDst,
                          ConfigSource *configDst);
   virtual bool RenameGroup(const String& pathOld, const String& nameNew);

   // for internal use by ProfileImpl only, don't use elsewhere
   wxConfigBase *GetConfig() const { return m_config; }

protected:
   /**
      Create a local config source.

      Ctor is protected, we're only created with our static CreateXXX() methods.

      @param name the name for the ConfigSource object
    */
   ConfigSourceLocal(const String& name = String());

   // do init m_config and m_path with the given values
   void DoInit(wxConfigBase *config) { m_config = config; }
   void DoInit(wxConfigBase *config, const wxString& path)
   {
      m_config = config;
      m_path = path;
   }

#ifdef OS_WIN
   // create registry config for us
   static wxConfigBase *DoCreateRegConfig();

   // initialize m_config with wxRegConfig we use under Windows
   bool InitRegistry();

   // get the location used for "config file" option in the registry (relative
   // to wxRegConfig root)
   static String GetConfigFileKey()
   {
      return M_PROFILE_CONFIG_SECTION _T("/UseConfigFile");
   }

   // get the config file location from the given wxRegConfig
   static String GetFilePath(wxConfigBase *config);
#endif // OS_WIN

   // initialize m_config with a wxFileConfig
   bool InitFile(const String& local, const String& global = String());

   // create either wxRegConfig or wxFileConfig using the same logic as
   // CreateDefault()
   bool InitDefault(const String& filename);

private:
   // the config object we use
   wxConfigBase *m_config;

   // the path of the file if m_config is a wxFileConfig, empty for wxRegConfig
   wxString m_path;
};

#endif // _M_CONFIGSOURCELOCAL_H_

