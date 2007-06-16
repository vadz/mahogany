///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   ConfigSourcesAll.h
// Purpose:     declaration of AllConfigSources class used by Profiles
// Author:      Vadim Zeitlin
// Creatd:      2005-07-04 (extracted from Profile.cpp)
// CVS-ID:      $Id$
// Copyright:   (c) 1998-2005 Vadim Zeitlin <vadim@zeitlins.org>
// Licence:     M licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _M_CONFIGSOURCESALL_H_
#define _M_CONFIGSOURCESALL_H_

#include "lists.h"
#include "Profile.h"
#include "ConfigSource.h"

class LookupData;

/**
   AllConfigSources is a list of all config source the profiles use.

   It is just an ordered, read-only linked list with a smart ctor.
 */
class AllConfigSources
{
public:
   /// Linked list of the config sources
   M_LIST_RC_ABSTRACT(List, ConfigSource);


   /**
      Static ctor: fully initializes the config sources list.

      In order to do this, it first creates the main local config source. It
      then uses it to find what other config sources are configured and
      creates them as well. They are inserted in the list in order specified
      by their priority values.

      Side effect: this ctor also changes the global wxConfig object by
      creating a new wxConfigMultiplexer instance and setting it as global
      wxConfig using wxConfig::Set().

      @param filename the file used for the default config, may be empty
      @return the pointer to global AllConfigSources object, same as returned
              by Get() from now on
    */
   static AllConfigSources *Init(const String& filename)
   {
      return ms_theInstance = new AllConfigSources(filename);
   }

   /**
      Destructor destroys wxConfig we had set in ctor.
    */
   ~AllConfigSources();


   /**
      Reads a value from the first config source containing the requested key.

      The full path of the entry to look for is the concatenation of path and
      data.GetKey().

      @param path the path to look in (without terminating slash)
      @param data the key and type of the value to look for and which also
             contains the value read on successful return
      @return true if this key was found in any config source, false otherwise
    */
   bool Read(const String& path, LookupData& data) const;

   /**
      Writes the value to the specified or default (local) config.

      The full path of the entry to look for is the concatenation of path and
      data.GetKey().

      @param path the path to write entry to (without terminating slash)
      @param data contains the key and value to be written to it
      @param config contains the config to write to, if it is NULL the
             most global (last) config source is used
      @return true if the entry was successfully written, false otherwise
    */
   bool Write(const String& path, const LookupData& data, ConfigSource *config);

   /**
      Copies all contents of the specified group to another one.

      This is done for all config sources.

      @param pathSrc path to start copying (without trailing slash)
      @param pathDst path to copy to (without trailing slash)
      @return true if there were no errors, false otherwise
    */
   bool CopyGroup(const String& pathSrc, const String& pathDst);

   /**
      @name Entries/subgroups enumeration.

      These methods wrap the corresponding ConfigSource ones and iterate over
      all config sources we have. They never return the same value twice,
      however, even if it appears in multiple config sources.

      Only GetFirstXXX() are here, GetNextXXX() are in ProfileEnumDataImpl as
      we are not needed any more then.
    */
   //@{

   /// Start enumerating the entries.
   bool GetFirstGroup(const String& path,
                      String& group,
                      ProfileEnumDataImpl& data) const;

   /// Start enumerating the subgroups.
   bool GetFirstEntry(const String& path,
                      String& entry,
                      ProfileEnumDataImpl& data) const;


   /// Return true if the given group exists in any config source
   bool HasGroup(const String& path) const
   {
      return FindGroup(path) != m_sources.end();
   }

   /// Return true if the given entry exists in any config source
   bool HasEntry(const String& path) const
   {
      return FindEntry(path) != m_sources.end();
   }

   //@}


   /**
      Flush all config sources.

      @return true if all sources were flushed successfully, false if at least
              one of them failed
    */
   bool FlushAll();

   /**
      Rename a group in all config sources where it exists.

      @param pathOld the full path to the existing group
      @param nameNew the new name (just the name)

      @return true if the group was renamed successfully in all configs, false
              if for at least one of them it failed or if it was not found in
              any config
    */
   bool Rename(const String& pathOld, const String& nameNew);

   /**
      Delete the entry from all config sources where it exists.

      @return true if the entry was removed successfully from all configs,
              false if for at least one of them it failed
    */
   bool DeleteEntry(const String& path);

   /**
      Delete the group from all config sources where it exists.

      @return true if the group was removed successfully from all configs,
              false if for at least one of them it failed
    */
   bool DeleteGroup(const String& path);


   /**
      @name Accessors to config sources themselves.

      These methods shouldn't be normally used directly but currently are for
      configuring the sources list.
   */
   //@{

   /// Get this object itself (created on demand)
   static AllConfigSources& Get() { return *ms_theInstance; }

   /// Get the list of all sources
   List& GetSources() { return m_sources; }
   const List& GetSources() const { return m_sources; }

   /**
      Set the list of all configuration sources.

      This function overwrites the previous defined config sources.

      The unnamed local config source shouldn't be included in the arrays
      passed to this function, it's always present.

      @param names the names of all config sources
      @param types the types of sources
      @param specs the spec strings (passed to ConfigSourceFactory::Save())
      @return true if ok, false on error
    */
   bool SetSources(const wxArrayString& names,
                   const wxArrayString& types,
                   const wxArrayString& specs);

   /// Delete the global AllConfigSources object returned by Get()
   static void Cleanup() { delete ms_theInstance; ms_theInstance = NULL; }

   //@}


   /**
      @name Helper methods for wxConfigMultiplexer and legacy code only.

      Do not use these methods from elsewhere.
    */
   //@{

   /// Find the config source containing this entry
   List::iterator FindEntry(const String& path) const;

   /// Find the config source containing this group
   List::iterator FindGroup(const String& path) const;

   /**
      Get wxConfig object associated with the local config source.

      This wxConfig object should be used for storing setting which only make
      sense for the local machine (typical example: windows positions).

      @param config pointer (not to be deleted by caller) or NULL
    */
   wxConfigBase *GetLocalConfig() const;

   //@}

private:
   // ctor is private, use public static Init() instead
   AllConfigSources(const String& filename);

   // public CopyGroup() helper
   bool CopyGroup(ConfigSource *config,
                  const String& pathSrc,
                  const String& pathDst);

   List m_sources;

   static AllConfigSources *ms_theInstance;


   DECLARE_NO_COPY_CLASS(AllConfigSources)
};

#endif // _M_CONFIGSOURCESALL_H_

