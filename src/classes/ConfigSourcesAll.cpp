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

   virtual void SetPath(const wxString& path)
   {
      wxConfigBase * const config = m_configSources.GetLocalConfig();
      if ( config )
      {
         config->SetPath(path);
         m_path = config->GetPath();
      }
   }

   virtual const wxString& GetPath() const { return m_path; }


   virtual bool GetFirstGroup(wxString& str, long& lIndex) const
   {
      wxConfigBase * const config = m_configSources.GetLocalConfig();
      return config && config->GetFirstGroup(str, lIndex);
   }

   virtual bool GetNextGroup(wxString& str, long& lIndex) const
   {
      wxConfigBase * const config = m_configSources.GetLocalConfig();
      return config && config->GetNextGroup(str, lIndex);
   }

   virtual bool GetFirstEntry(wxString& str, long& lIndex) const
   {
      wxConfigBase * const config = m_configSources.GetLocalConfig();
      return config && config->GetFirstEntry(str, lIndex);
   }

   virtual bool GetNextEntry(wxString& str, long& lIndex) const
   {
      wxConfigBase * const config = m_configSources.GetLocalConfig();
      return config && config->GetNextEntry(str, lIndex);
   }


   virtual size_t GetNumberOfEntries(bool bRecursive = false) const
   {
      wxConfigBase * const config = m_configSources.GetLocalConfig();
      return config ? config->GetNumberOfEntries(bRecursive) : 0;
   }

   virtual size_t GetNumberOfGroups(bool bRecursive = false) const
   {
      wxConfigBase * const config = m_configSources.GetLocalConfig();
      return config ? config->GetNumberOfGroups(bRecursive) : 0;
   }


   virtual bool HasGroup(const wxString& name) const
   {
      return m_configSources.HasGroup(MakeFullPath(name));
   }

   virtual bool HasEntry(const wxString& name) const
   {
      return m_configSources.HasEntry(MakeFullPath(name));
   }


   virtual bool Flush(bool /* bCurrentOnly */ = false)
   {
      return m_configSources.FlushAll();
   }

   virtual bool
   RenameEntry(const wxString& /* oldName */, const wxString& /* newName */)
   {
      FAIL_MSG( _T("not implemented") );

      return false;
   }

   virtual bool
   RenameGroup(const wxString& /* oldName */, const wxString& /* newName */)
   {
      FAIL_MSG( _T("not implemented") );

      return false;
   }

   virtual bool DeleteEntry(const wxString& key, bool /* groupIfEmpty */ = true)
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
         AllConfigSources::List::iterator i = m_configSources.FindEntry(path);
         if ( i == m_configSources.GetSources().end() )
         {
            CHECK( foundAny, false, _T("entry to delete doesn't exist") );
            break;
         }

         foundAny = true;
         if ( !i->DeleteEntry(path) )
            return false;
      }

      return true;
   }

   virtual bool DeleteGroup(const wxString& key)
   {
      const wxString path = MakeFullPath(key);

      bool foundAny = false;
      for ( ;; )
      {
         AllConfigSources::List::iterator i = m_configSources.FindGroup(path);
         if ( i == m_configSources.GetSources().end() )
         {
            CHECK( foundAny, false, _T("group to delete doesn't exist") );
            break;
         }

         foundAny = true;
         if ( !i->DeleteGroup(path) )
            return false;
      }

      return true;
   }

   virtual bool DeleteAll()
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
         config = *m_configSources.GetSources().begin().operator->();
      }
      else // can be shared
      {
         config = NULL;
      }

      return m_configSources.Write(m_path, ld, config);
   }


   virtual bool DoReadString(const wxString& key, wxString *pStr) const
   {
      LookupData ld(key, _T(""));
      if ( !DoRead(ld) )
         return false;

      *pStr = ld.GetString();
      return true;
   }

   virtual bool DoReadLong(const wxString& key, long *pl) const
   {
      LookupData ld(key, 0l);
      if ( !DoRead(ld) )
         return false;

      *pl = ld.GetLong();
      return true;
   }

   virtual bool DoWriteString(const wxString& key, const wxString& value)
   {
      LookupData ld(key, value);
      return DoWrite(ld);
   }

   virtual bool DoWriteLong(const wxString& key, long value)
   {
      LookupData ld(key, value);
      return DoWrite(ld);
   }

private:
   AllConfigSources& m_configSources;
   wxString m_path;

   DECLARE_NO_COPY_CLASS(wxConfigMultiplexer)
};


// ============================================================================
// AllConfigSources implementation
// ============================================================================

M_LIST(LongList, long);

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
   m_sources.push_back(configLocal);

   // now get all the other configs
   LongList priorities;

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

         List::iterator i = m_sources.begin();
         ++i;              // skip local config which is always first

         LongList::iterator j;
         for ( j = priorities.begin(); j != priorities.end(); ++i, ++j )
         {
            if ( *j > prio )
               break;
         }

         m_sources.insert(i, config);
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

   const List::iterator end = m_sources.end();
   for ( List::iterator i = m_sources.begin(); i != end; ++i )
   {
      if ( isNumeric ? i->Read(fullpath, data.GetLongPtr())
                     : i->Read(fullpath, data.GetStringPtr()) )
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
   if ( !config )
   {
      CHECK( !m_sources.empty(), false,
               _T("can't write to profile if no config sources exist") );

      List::iterator i = m_sources.end();
      config = *((--i).operator->());
   }

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

   const List::iterator end = m_sources.end();
   for ( List::iterator i = m_sources.begin(); i != end; ++i )
   {
      rc &= CopyGroup(i.operator->(), pathSrc, pathDst);
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

AllConfigSources::List::iterator
AllConfigSources::FindGroup(const String& path) const
{
   const List::iterator end = m_sources.end();
   for ( List::iterator i = m_sources.begin(); i != end; ++i )
   {
      if ( i->HasGroup(path) )
         return i;
   }

   return end;
}

AllConfigSources::List::iterator
AllConfigSources::FindEntry(const String& path) const
{
   const List::iterator end = m_sources.end();
   for ( List::iterator i = m_sources.begin(); i != end; ++i )
   {
      if ( i->HasEntry(path) )
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

   const List::iterator end = m_sources.end();
   for ( List::iterator i = m_sources.begin(); i != end; ++i )
   {
      rc &= i->Flush();
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

   const List::iterator end = m_sources.end();
   for ( List::iterator i = m_sources.begin(); i != end; ++i )
   {
      ConfigSource::EnumData cookie;
      for ( bool cont = i->GetFirstGroup(parent, group, cookie);
            cont;
            cont = i->GetNextGroup(group, cookie) )
      {
         if ( group == name )
         {
            // this config has that group, do rename it
            if ( i->RenameGroup(pathOld, nameNew) )
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

   const List::iterator end = m_sources.end();
   for ( List::iterator i = m_sources.begin(); i != end; ++i )
   {
      ConfigSource::EnumData cookie;
      for ( bool cont = i->GetFirstEntry(parent, entry, cookie);
            cont;
            cont = i->GetNextEntry(entry, cookie) )
      {
         if ( entry == name )
         {
            // this config has that entry, do remove it
            rc &= i->DeleteEntry(path);

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

   const List::iterator end = m_sources.end();
   for ( List::iterator i = m_sources.begin(); i != end; ++i )
   {
      ConfigSource::EnumData cookie;
      for ( bool cont = i->GetFirstGroup(parent, group, cookie);
            cont;
            cont = i->GetNextGroup(group, cookie) )
      {
         if ( group == name )
         {
            // this config has that group, do remove it
            rc &= i->DeleteGroup(path);

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
      config = static_cast<ConfigSourceLocal *>(*m_sources.begin());

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

   ConfigSource& config = **m_sources.begin();

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
                       type.c_str());
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

