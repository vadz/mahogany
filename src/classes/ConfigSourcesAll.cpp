///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   ConfigSourcesAll.cpp
// Purpose:     implementation of AllConfigSources and dependent classes
// Author:      Vadim Zeitlin
// Creatd:      2005-07-04 (extracted from Profile.cpp)
// CVS-ID:      $Id$
// Copyright:   (c) 1998-2005 Vadim Zeitlin <vadim@zeitlins.org>
// Licence:     M licence
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
   #include "Mcommon.h"
   #include "Mdefaults.h"
   #include "MApplication.h"

   #include <wx/config.h>
#endif // USE_PCH

#include <wx/persctrl.h>

#include "ConfigSourcesAll.h"
#include "ConfigSourceLocal.h"
#include "ConfigPrivate.h"

#include <algorithm>

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_CONFIG_SOURCE_PRIO;
extern const MOption MP_CONFIG_SOURCE_TYPE;

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

/**
   wxConfigMultiplexer is a wxConfig façade for AllConfigSources.

   wxConfigMultiplexer presents wxConfig interface for AllConfigSources
   functionality, i.e. it reads from all config sources and not just from the
   local config.
 */
class wxConfigMultiplexer : public wxConfigBase
{
public:
   wxConfigMultiplexer(AllConfigSources& configSources)
      : m_configSources(configSources)
   {
   }

   void SetPath(const wxString& path) override
   {
      wxConfigBase * const config = m_configSources.GetLocalConfig();
      if ( config )
      {
         config->SetPath(path);
         m_path = config->GetPath();
      }
   }

   const wxString& GetPath() const override { return m_path; }


   bool GetFirstGroup(wxString& str, long& lIndex) const override
   {
      wxConfigBase * const config = m_configSources.GetLocalConfig();
      return config && config->GetFirstGroup(str, lIndex);
   }

   bool GetNextGroup(wxString& str, long& lIndex) const override
   {
      wxConfigBase * const config = m_configSources.GetLocalConfig();
      return config && config->GetNextGroup(str, lIndex);
   }

   bool GetFirstEntry(wxString& str, long& lIndex) const override
   {
      wxConfigBase * const config = m_configSources.GetLocalConfig();
      return config && config->GetFirstEntry(str, lIndex);
   }

   bool GetNextEntry(wxString& str, long& lIndex) const override
   {
      wxConfigBase * const config = m_configSources.GetLocalConfig();
      return config && config->GetNextEntry(str, lIndex);
   }


   size_t GetNumberOfEntries(bool bRecursive = false) const override
   {
      wxConfigBase * const config = m_configSources.GetLocalConfig();
      return config ? config->GetNumberOfEntries(bRecursive) : 0;
   }

   size_t GetNumberOfGroups(bool bRecursive = false) const override
   {
      wxConfigBase * const config = m_configSources.GetLocalConfig();
      return config ? config->GetNumberOfGroups(bRecursive) : 0;
   }


   bool HasGroup(const wxString& name) const override
   {
      return m_configSources.HasGroup(MakeFullPath(name));
   }

   bool HasEntry(const wxString& name) const override
   {
      return m_configSources.HasEntry(MakeFullPath(name));
   }


   bool Flush(bool /* bCurrentOnly */ = false) override
   {
      return m_configSources.FlushAll();
   }

   bool
   RenameEntry(const wxString& /* oldName */, const wxString& /* newName */) override
   {
      FAIL_MSG( _T("not implemented") );

      return false;
   }

   bool
   RenameGroup(const wxString& /* oldName */, const wxString& /* newName */) override
   {
      FAIL_MSG( _T("not implemented") );

      return false;
   }

   bool DeleteEntry(const wxString& key, bool /* groupIfEmpty */ = true) override
   {
      wxString path;
      if ( *key.c_str() == _T('/') )
         path = key;
      else
         path << m_path << _T('/') << key;

      // we need to delete all these entries
      bool foundAny = false;
      for ( ;; )
      {
         const auto i = m_configSources.FindEntry(path);
         if ( i == m_configSources.GetSources().end() )
         {
            CHECK( foundAny, false, _T("entry to delete doesn't exist") );
            break;
         }

         foundAny = true;
         if ( !(*i)->DeleteEntry(path) )
            return false;
      }

      return true;
   }

   bool DeleteGroup(const wxString& key) override
   {
      const wxString path = MakeFullPath(key);

      bool foundAny = false;
      for ( ;; )
      {
         const auto i = m_configSources.FindGroup(path);
         if ( i == m_configSources.GetSources().end() )
         {
            CHECK( foundAny, false, _T("group to delete doesn't exist") );
            break;
         }

         foundAny = true;
         if ( !(*i)->DeleteGroup(path) )
            return false;
      }

      return true;
   }

   bool DeleteAll() override
   {
      // this is too dangerous, we don't provide any way to wipe out all config
      // information
      return false;
   }

protected:
   wxString MakeFullPath(const wxString& key) const
   {
      wxString path;
      if ( *key.c_str() == _T('/') )
         path = key;
      else
         path << m_path << _T('/') << key;

      return path;
   }

   bool DoRead(LookupData& ld) const
   {
      return m_configSources.Read(m_path, ld);
   }

   bool DoWrite(LookupData& ld)
   {
      // most of the persistent controls settings can be shared between the
      // installations and so can be written to the global config source, but
      // some of them only make sense for this machine and should be written to
      // the local config source
      ConfigSource *config;
      if ( m_path.StartsWith(M_FRAMES_CONFIG_SECTION) ||
               m_path.StartsWith(_T("/") M_SETTINGS_CONFIG_SECTION _T("/") +
                                    wxPSplitterWindow::GetConfigPath()) )
      {
         config = m_configSources.GetSources().front().get();
      }
      else // can be shared
      {
         config = NULL;
      }

      return m_configSources.Write(m_path, ld, config);
   }


   bool DoReadString(const wxString& key, wxString *pStr) const override
   {
      LookupData ld(key, wxEmptyString);
      if ( !DoRead(ld) )
         return false;

      *pStr = ld.GetString();
      return true;
   }

   bool DoReadLong(const wxString& key, long *pl) const override
   {
      LookupData ld(key, 0l);
      if ( !DoRead(ld) )
         return false;

      *pl = ld.GetLong();
      return true;
   }

   bool DoWriteString(const wxString& key, const wxString& value) override
   {
      LookupData ld(key, value);
      return DoWrite(ld);
   }

   bool DoWriteLong(const wxString& key, long value) override
   {
      LookupData ld(key, value);
      return DoWrite(ld);
   }

   bool DoReadBinary(const wxString& key, wxMemoryBuffer* buf) const override
   {
      FAIL_MSG( "binary data unsupported" );
      return false;
   }

   bool DoWriteBinary(const wxString& key, const wxMemoryBuffer& buf) override
   {
      FAIL_MSG( "binary data unsupported" );
      return false;
   }

private:
   AllConfigSources& m_configSources;
   wxString m_path;

   DECLARE_NO_COPY_CLASS(wxConfigMultiplexer)
};


// ============================================================================
// AllConfigSources implementation
// ============================================================================

AllConfigSources *AllConfigSources::ms_theInstance = NULL;

// ----------------------------------------------------------------------------
// AllConfigSources creation
// ----------------------------------------------------------------------------

AllConfigSources::AllConfigSources(const String& filename)
{
   // first create the local config source, if this fails we can't do anything
   // more
   ConfigSource *configLocal = ConfigSource::CreateDefault(filename);
   if ( !configLocal )
      return;

   if ( !configLocal->IsOk() )
   {
      configLocal->DecRef();
      return;
   }

   // also register out special wxConfig for persistent controls use
   delete wxConfig::Set(new wxConfigMultiplexer(*this));


   // now build the list of all config source we use
   // ----------------------------------------------

   // local config is always first
   m_sources.emplace_back(configLocal);

   // now get all the other configs, keeping them sorted by priority: this
   // vector contains the priorities of all config sources except the local
   // one, in the same order as they're stored in m_sources
   std::vector<long> priorities;

   ConfigSource::EnumData cookie;
   const String key(M_CONFIGSRC_CONFIG_SECTION),
                slash(_T('/')),
                valuePrio(slash + GetOptionName(MP_CONFIG_SOURCE_PRIO));

   String name;
   for ( bool cont = configLocal->GetFirstGroup(key, name, cookie);
         cont;
         cont = configLocal->GetNextGroup(name, cookie) )
   {
      const String subkey = key + slash + name;

      ConfigSource *config = ConfigSource::Create(*configLocal, subkey);
      if ( config )
      {
         // normally Create() shouldn't return it in this case
         ASSERT_MSG( config->IsOk(), _T("invalid config source created") );

         // find the place to insert this config source at
         long prio;
         if ( !configLocal->Read(subkey + valuePrio, &prio) )
         {
            // insert at the end by default
            prio = INT_MAX;
         }

         // insert after all the sources with the same priority, if any
         const auto j = std::upper_bound(priorities.begin(),
                                         priorities.end(),
                                         prio);

         // +1 to skip local config which is always first
         m_sources.emplace(m_sources.begin() + (j - priorities.begin()) + 1,
                           config);
         priorities.insert(j, prio);
      }
      //else: creation failed, don't do anything
   }
}

AllConfigSources::~AllConfigSources()
{
   // we can't allow wxConfigMultiplexer to live any longer
   delete wxConfig::Set(NULL);
}

// ----------------------------------------------------------------------------
// AllConfigSources reading and writing
// ----------------------------------------------------------------------------

bool AllConfigSources::Read(const String& path, LookupData& data) const
{
   const String& key = data.GetKey();
   ASSERT_MSG( !key.empty(), _T("empty config key") );

   String fullpath;
   if ( *key.c_str() != _T('/') )
   {
      fullpath += path;
      if ( fullpath.empty() || fullpath.Last() != _T('/') )
         fullpath += _T('/');
   }
   fullpath += key;

   ASSERT_MSG( *fullpath.c_str() == _T('/'), _T("config paths must be absolute") );

   ASSERT_MSG( fullpath.length() < 3 ||
                  fullpath[1u] != 'M' ||
                     fullpath[2u] != _T('/'),
                        _T("config path must not start with /M") );

   const bool isNumeric = data.GetType() == LookupData::LD_LONG;

   for ( const auto& config : m_sources )
   {
      if ( isNumeric ? config->Read(fullpath, data.GetLongPtr())
                     : config->Read(fullpath, data.GetStringPtr()) )
      {
         return true;
      }
   }

   return false;
}

bool
AllConfigSources::Write(const String& path,
                        const LookupData& data,
                        ConfigSource *config)
{
   // construct the full path
   const String key = data.GetKey();
   String fullpath;
   if ( *key.c_str() != _T('/') )
      fullpath << path << _T('/');

   fullpath << key;

   ASSERT_MSG( *fullpath.c_str() == _T('/'), _T("config paths must be absolute") );

   ASSERT_MSG( fullpath.length() < 3 ||
                  fullpath[1u] != 'M' ||
                     fullpath[2u] != _T('/'),
                        _T("config path must not start with /M") );


   // find where to write the data to
   if ( !config )
   {
      // we need to find the first config source in which this value is already
      // present as writing it should overwrite any existing value and for this
      // it has to be written to higher priority config source

      // we also have to check for the normal entries if we're writing a
      // suspended one
      extern const char SUSPEND_PATH[]; // from Profile.cpp
      String fullpathUnsusp;
      size_t posSusp = fullpath.find(SUSPEND_PATH);
      if ( posSusp != String::npos )
      {
         // +1 for the slash
         fullpathUnsusp = fullpath;
         fullpathUnsusp.erase(posSusp, strlen(SUSPEND_PATH) + 1);
      }

      // note that this loop terminates with config set to the last source if
      // the element is not found anywhere, just as desired
      for ( const auto& source : m_sources )
      {
         config = source.get();
         if ( config->HasEntry(fullpath) ||
               (!fullpathUnsusp.empty() && config->HasEntry(fullpathUnsusp)) )
         {
            // write to this one to overwrite the existing entry
            break;
         }
      }

      CHECK( config, false,
               _T("can't write to profile if no config sources exist") );
   }


   // finally do write it
   return data.GetType() == LookupData::LD_LONG
            ? config->Write(fullpath, data.GetLong())
            : config->Write(fullpath, data.GetString());
}

bool
AllConfigSources::CopyGroup(ConfigSource *config,
                            const String& pathSrc,
                            const String& pathDst)
{
   ConfigSource::EnumData cookie;
   String name;

   const String pathSrcSlash(pathSrc + _T('/')),
                pathDstSlash(pathDst + _T('/'));

   bool rc = true;

   // first copy all the entries
   bool cont = config->GetFirstEntry(pathSrc, name, cookie);
   while ( cont )
   {
      rc &= config->CopyEntry(pathSrcSlash + name, pathDstSlash + name);

      cont = config->GetNextEntry(name, cookie);
   }

   // and then (recursively) copy all subgroups
   cont = config->GetFirstGroup(pathSrc, name, cookie);
   while ( cont )
   {
      rc &= CopyGroup(config, pathSrcSlash + name, pathDstSlash + name);

      cont = config->GetNextGroup(name, cookie);
   }

   return rc;
}

bool
AllConfigSources::CopyGroup(const String& pathSrc, const String& pathDst)
{
   bool rc = true;

   for ( const auto& config : m_sources )
   {
      rc &= CopyGroup(config.get(), pathSrc, pathDst);
   }

   return rc;
}

// ----------------------------------------------------------------------------
// AllConfigSources groups/entries enumeration
// ----------------------------------------------------------------------------

bool
AllConfigSources::GetFirstGroup(const String& path,
                                String& group,
                                ProfileEnumDataImpl& data) const
{
   data.Init(path, m_sources.begin(), m_sources.end());

   return data.GetNextGroup(group);
}

bool
AllConfigSources::GetFirstEntry(const String& path,
                                String& entry,
                                ProfileEnumDataImpl& data) const
{
   data.Init(path, m_sources.begin(), m_sources.end());

   return data.GetNextEntry(entry);
}

AllConfigSources::List::const_iterator
AllConfigSources::FindGroup(const String& path) const
{
   const auto end = m_sources.end();
   for ( auto i = m_sources.begin(); i != end; ++i )
   {
      if ( (*i)->HasGroup(path) )
         return i;
   }

   return end;
}

AllConfigSources::List::const_iterator
AllConfigSources::FindEntry(const String& path) const
{
   const auto end = m_sources.end();
   for ( auto i = m_sources.begin(); i != end; ++i )
   {
      if ( (*i)->HasEntry(path) )
         return i;
   }

   return end;
}

// ----------------------------------------------------------------------------
// miscellaneous AllConfigSources methods
// ----------------------------------------------------------------------------

bool AllConfigSources::FlushAll()
{
   bool rc = true;

   for ( const auto& config : m_sources )
   {
      rc &= config->Flush();
   }

   return rc;
}

bool AllConfigSources::Rename(const String& pathOld, const String& nameNew)
{
   bool rc = true;
   size_t numRenamed = 0;

   String parent = pathOld.BeforeLast(_T('/')),
          name = pathOld.AfterLast(_T('/')),
          group;

   for ( const auto& config : m_sources )
   {
      ConfigSource::EnumData cookie;
      for ( bool cont = config->GetFirstGroup(parent, group, cookie);
            cont;
            cont = config->GetNextGroup(group, cookie) )
      {
         if ( group == name )
         {
            // this config has that group, do rename it
            if ( config->RenameGroup(pathOld, nameNew) )
               numRenamed++;
            else
               rc = false;

            break;
         }
      }
   }

   return rc && numRenamed > 0;
}

bool AllConfigSources::DeleteEntry(const String& path)
{
   bool rc = true;

   String parent = path.BeforeLast(_T('/')),
          name = path.AfterLast(_T('/')),
          entry;

   for ( const auto& config : m_sources )
   {
      ConfigSource::EnumData cookie;
      for ( bool cont = config->GetFirstEntry(parent, entry, cookie);
            cont;
            cont = config->GetNextEntry(entry, cookie) )
      {
         if ( entry == name )
         {
            // this config has that entry, do remove it
            rc &= config->DeleteEntry(path);

            break;
         }
      }
   }

   return rc;
}

bool AllConfigSources::DeleteGroup(const String& path)
{
   bool rc = true;

   String parent = path.BeforeLast(_T('/')),
          name = path.AfterLast(_T('/')),
          group;

   for ( const auto& config : m_sources )
   {
      ConfigSource::EnumData cookie;
      for ( bool cont = config->GetFirstGroup(parent, group, cookie);
            cont;
            cont = config->GetNextGroup(group, cookie) )
      {
         if ( group == name )
         {
            // this config has that group, do remove it
            rc &= config->DeleteGroup(path);

            break;
         }
      }
   }

   return rc;
}


wxConfigBase *AllConfigSources::GetLocalConfig() const
{
   if ( m_sources.empty() )
      return NULL;

   // we know that the first config source is the local one...
   ConfigSourceLocal *
      config = static_cast<ConfigSourceLocal *>(m_sources.front().get());

   return config->GetConfig();
}

// ----------------------------------------------------------------------------
// changing configuration sources
// ----------------------------------------------------------------------------

bool
AllConfigSources::SetSources(const wxArrayString& names,
                             const wxArrayString& types,
                             const wxArrayString& specs)
{
   const size_t count = names.size();
   CHECK( types.size() == count && specs.size() == count, false,
            _T("array size mismatch") );

   ConfigSource& config = *m_sources.front();

   // we need just the name for RenameGroup()
   String configsBackup(_T("Configs.Old"));
   config.RenameGroup(M_CONFIGSRC_CONFIG_SECTION, configsBackup);

   // but make full path now
   const String slash(_T('/'));
   configsBackup = String(M_CONFIGSRC_CONFIG_SECTION).BeforeLast(_T('/'))
                         + slash + configsBackup;
   const String pathType = slash + GetOptionName(MP_CONFIG_SOURCE_TYPE);
   const String pathPrio = slash + GetOptionName(MP_CONFIG_SOURCE_PRIO);

   // auto assign the priorities to ensure that the sources are in order but
   // leave gaps between them to also allow for manual editing
   int prio = 10;
   for ( size_t n = 0; n < count; n++, prio += 10 )
   {
      String path;
      path << M_CONFIGSRC_CONFIG_SECTION << slash << names[n];

      const String type = types[n];
      ConfigSourceFactory_obj factory(ConfigSourceFactory::Find(type));

      if ( !factory ||
               !factory->Save(config, path, specs[n]) ||
                  !config.Write(path + pathType, type) ||
                     !config.Write(path + pathPrio, prio) )
      {
         if ( !factory )
         {
            wxLogError(_("Unknown configuration source type \"%s\"."),
                       type);
         }

         // restore old config sources
         config.DeleteGroup(M_CONFIGSRC_CONFIG_SECTION);
         config.RenameGroup(configsBackup,
                            String(M_CONFIGSRC_CONFIG_SECTION).AfterLast(_T('/')));
         return false;
      }
   }

   config.DeleteGroup(configsBackup);
   return true;
}

