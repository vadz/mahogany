///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   ConfigSource.h: source of configuration information
// Purpose:     ConfigSource objects are used only by Profile class
// Author:      Vadim Zeitlin
// Modified by:
// Created:     23.08.03
// CVS-ID:      $Id$
// Copyright:   (c) 2003 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M licence
///////////////////////////////////////////////////////////////////////////////

/**
   @file ConfigSource.h
   @brief Declaration of ConfigSource class.

   ConfigSource is a source of configuration information. Currently it may be
   either a local file (all platforms), registry hive (Win32) or data from an
   IMAP server. In the future it could also be an XML database (Mac OS X), or
   be retrieved using ACAP or LDAP.

   Each ConfigSource has a name which must be globally unique. Empty name is
   special: unnamed ConfigSource object always exists and is local. This object
   is used for boot strapping (i.e. it stores the settings telling us which
   other config sources to use and where they are located).

   Profile keeps the list of all active config sources and uses them
   appropriately, i.e. it reads data from all of them in order specified by the
   user (unnamed config source is always used first however). The only
   acceptable use of ConfigSource outside of Profile is for importing/exporting
   the data between different formats.
 */

#ifndef _M_CONFIGSOURCE_H_
#define _M_CONFIGSOURCE_H_

#include "MObject.h"            // for MObjectRC

#include "MModule.h"

class WXDLLEXPORT wxConfigBase;

/// The name of the config section containing the config sources info
#define M_CONFIGSRC_CONFIG_SECTION _T("/Configs")

/**
   ConfigSource is a source of configuration information.

   This is an abstract base class which defines the interface needed by
   Profile. It is similar to wxConfig and is supposed to use a hierarchical
   tree-like structure for storing keys and values just as wxConfig does.
 */
class ConfigSource : public MObjectRC
{
public:
   /**
      This class is used with GetFirst/NextEntry/Group().
   */
   class EnumData
   {
   public:
      EnumData() { }

      // for internal use only
      String& Key() { return m_key; }
      long& Cookie() { return m_cookie; }

   private:
      String m_key;
      long m_cookie;

      // we can't be copied
      EnumData(const EnumData&);
      EnumData& operator=(const EnumData&);
   };


   /**
      @name Creating config sources.

      We provide static creator functions for creating the default config
      source and for creating the config source from the settings in a config
      section. On startup, Profile scans the config sources section of the
      default config and calls Create() for all its subgroups.

      In all cases, the caller is responsible for calling DecRef() on the
      returned pointer if it is not NULL.
    */
   //@{

   /**
      Create the default config source.

      The name of the config file may be empty in which case the default
      location is used: either ~/.M/config under Unix or wxRegConfig (and not
      file at all) under Windows.

      @param filename the name of config file or empty
      @param default config source to be DecRef()d by caller or NULL
    */
   static ConfigSource *CreateDefault(const String& filename);

   /**
      Create the config source using the parameters from the given section.

      This function uses the "Type" value to find the ConfigSourceFactory to
      use and then calls its Create().

      @param config the config containing the parameters
      @param name its section name, without the trailing slash
      @param new config source to be DecRef()d by caller or NULL
    */
   static ConfigSource *Create(const ConfigSource& config, const String& name);

   //@}


   /**
      Copy contents of one config source to another.

      By default this function copies everything from the source config to the
      destination one, irrecoverably overwriting the old contents of the
      destination.

      @param configDst where to copy
      @param configSrc what to copy
      @param pathDst path under which we write in configDst (root by default)
      @param pathSrc path where we start from in configSrc (root by default)
      @return true if copy was successful, false otherwise
    */
   static bool Copy(ConfigSource& configDst,
                    const ConfigSource& configSrc,
                    const String& pathDst = wxEmptyString,
                    const String& pathSrc = wxEmptyString);


   /**
      @name Object description.
    */
   //@{

   /// Get the (user-readable) name of this object
   const String& GetName() const { return m_name; }

   /// Get the type of this object (same as type of factory used to create it)
   const String& GetType() const { return m_type; }

   /**
      Get the specification of this object.

      The meaning of this string is type-dependent, e.g. it is a full path of
      the file for "file" config sources and an IMAP spec in c-client format
      for "imap" sources and maybe something else entirely for the other ones.
      The only requirement is that this string be in the same format as used by
      ConfigSourceFactory::Save().
    */
   virtual String GetSpec() const = 0;

   /**
      Return true if the object was created successfully.

      This is used by ConfigSourceFactory and classes derived from it created
      by IMPLEMENT_CONFIG_SOURCE().

      @return true if ok, if false the config source object can't be used
    */
   virtual bool IsOk() const = 0;

   /// Return true if this object may be used without network
   virtual bool IsLocal() const = 0;

   //@}


   /**
      @name Reading data.

      We provide only methods for reading strings and longs, the caller must
      check the range if a bool/char/short/int and not long is needed. Reading
      float/double directly is not supported, use strings instead if yuou
      really need this.

      Each method below returns true if the value was found and retrieved,
      false if it wasn't found or an error occured (such errors would usually
      be ignored anyhow...).
    */
   //@{

   /**
      Read a string.

      @param name the name of the key to read it from
      @param value pointer to the result, must not be NULL!
    */
   virtual bool Read(const String& name, String *value) const = 0;

   /**
      Read an integer

      @param name the name of the key to read it from
      @param value pointer to the result, must not be NULL!
    */
   virtual bool Read(const String& name, long *value) const = 0;

   //@}


   /**
      @name Writing data.

      As above, onlky strings and longs are directly supported.

      All Write() methods below return true if ok and false on error -- this
      should be checked for and handled!
    */
   //@{

   /// Write a string
   virtual bool Write(const String& name, const String& value) = 0;

   /// Write an integer
   virtual bool Write(const String& name, long value) = 0;

   /**
      Flush, i.e. permanently save, the data written to this config source.

      This may do nothing at all or take a long time to complete depending on
      the implementation.

      @return true if the data was successfully flushed, false on error
    */
   virtual bool Flush() = 0;

   //@}


   /**
      @name Enumerating entries and groups.

      The enumeration methods work with an opaque cookie parameter which must
      be first passed to GetrFirstXXX() and then to GetNextXXX(). All methods
      return true if the next group/entry was retrieved or false if there are
      no more of them or an error occured.
    */
   //@{

   /// Get the first subgroup of the given key
   virtual bool GetFirstGroup(const String& key,
                                 String& group, EnumData& cookie) const = 0;

   /// Get the next subgroup of the given key
   virtual bool GetNextGroup(String& group, EnumData& cookie) const = 0;

   /// Get the first entry of the given key
   virtual bool GetFirstEntry(const String& key,
                                 String& entry, EnumData& cookie) const = 0;

   /// Get the next entry of the given key
   virtual bool GetNextEntry(String& entry, EnumData& cookie) const = 0;

   /**
      Checks whether the given group exists.

      Note to implementors: the path may contain slashes, so this function
      should recurse for each path component, not just enumerate all immediate
      subgroups!

      @param path the group to check existence of
      @return true if the group exists (even if it is empty), false otherwise
    */
   virtual bool HasGroup(const String& path) const = 0;

   /**
      Checks whether the given entry exists.

      @sa HasGroup

      @param path the entry to check existence of (may contain slashes)
      @return true if the entry exists
    */
   virtual bool HasEntry(const String& path) const = 0;

   //@}


   /**
      @name Other operations on entries and groups.
    */
   //@{

   /// Delete the given entry (it must exist).
   virtual bool DeleteEntry(const String& name) = 0;

   /// Delete the group with all its subgroups (TAKE CARE!)
   virtual bool DeleteGroup(const String& name) = 0;

   /**
      Copy the given entry (maybe to another group).

      @param nameSrc the full name of the entry to copy
      @param nameDst the full name to copy to (possibly in configDst)
      @param configDst config object to copy to, if NULL, same as this one
    */
   virtual bool CopyEntry(const String& nameSrc,
                          const String& nameDst,
                          ConfigSource *configDst = NULL) = 0;

   /**
      Rename a group.

      We don't provide a method for entry renaming first because we don't need
      it right now and second because doing it is already possible using
      CopyEntry() and DeleteEntry().

      @param pathOld the full path to an existing group
      @param nameNew the new name of the group (path remains the same)
      @return true if ok, false on failure
    */
   virtual bool RenameGroup(const String& pathOld, const String& nameNew) = 0;

   //@}


protected:
   /// Constrructor is protected, you can only create derived classes
   ConfigSource(const String& name, const String& type)
      : m_name(name.empty() ? String(_("Local")) : name.AfterLast(_T('/'))),
        m_type(type)
   {
   }

   /// Virtual destructor as for any base class
   virtual ~ConfigSource();

private:
   /// the name of this object, it is set once on creation and can't be changed
   const String m_name;

   /// the type of this object, can't be changed neither
   const String m_type;
};

DECLARE_AUTOPTR(ConfigSource);


/**
   ConfigSourceFactory creates ConfigSource objects.

   This class is used to create config sources from their types.
 */
class ConfigSourceFactory : public MObjectRC
{
public:
   /**
      This class is used by GetFirst() and GetNext() internally.

      To enumerate all config source factories you should simply do something
      like this:
      @code
         ConfigSourceFactory::EnumData data;
         for ( ConfigSourceFactory *fact = ConfigSourceFactory::GetFirst(data);
               fact;
               fact = ConfigSourceFactory::GetNext(data) )
         {
            ... do whatever with fact ...

            fact->DecRef();
         }
      @endcode
    */
   class EnumData
   {
   public:
      EnumData();
      ~EnumData();

   private:
      // for ConfigSourceFactory::GetFirst() only: resets the object ot the
      // initial state
      void Reset();

      // for ConfigSourceFactory::GetNext() only: returns the next listing
      // entry or NULL if no more
      const MModuleListingEntry *GetNext();     

      
      // the listing containing all config source factory modules
      MModuleListing *m_listing;

      // the index of the next element to be returned
      size_t m_current;


      // we can't be copied
      EnumData(const EnumData&);
      EnumData& operator=(const EnumData&);

      friend class ConfigSourceFactory;
   };


   /**
      Finds the factory for creating the config sources of given type.

      @param type the config source type ("file", "imap", ...)
      @return the factory for the objects of this type or NULL if none
    */
   static ConfigSourceFactory *Find(const String& type);

   /**
      Get the first factory.

      This function and the next one allow to enumerate all available config
      sources factories.

      The caller is responsible for calling DecRef() on the returned pointer if
      it is not NULL.

      @param cookie the enumeration data, should be passed to GetNext() later
      @return the first factory in the list or NULL
    */
   static ConfigSourceFactory *GetFirst(EnumData& cookie);

   /**
      Get the next factory.

      This function is used together with GetFirst() to enumerate all avilable
      factories.

      The caller is responsible for calling DecRef() on the returned pointer if
      it is not NULL.

      @param cookie the same cookie as used with GetFirst() previously
      @return the next factory in the list or NULL
    */
   static ConfigSourceFactory *GetNext(EnumData& cookie);


   /**
      Get the string identifying this config source factory.

      The factory types are case-insensitive.

      @return string uniquely identifying this factory.
    */
   virtual const wxChar *GetType() const = 0;

   /**
      Creates the config source object.

      The parameters of the object are read from a section of an existing
      (normally the default one) config. The last component of the group name
      also specifies the name to be used for the new config source object.

      @param config the existing config source containing all parameters
      @param name the config section containing the parameters
      @return new config source or NULL if creation failed.
    */
   virtual ConfigSource *Create(const ConfigSource& config,
                                const String& name) = 0;

   /**
      Save the description of the config source object in a config.

      @param config an existing config source where this source parameters to
                    be saved
      @param name the config section containing the parameters
      @param spec the specification of the config source object (in the format
                  returned by ConfigSource::GetSpec())
      @return true if ok, false on error, e.g. if the spec is invalid.
   */
   virtual bool Save(ConfigSource& config,
                     const String& name,
                     const String& spec) = 0;
};

DECLARE_AUTOPTR(ConfigSourceFactory);


/**
   ConfigSourceFactoryModule is used to load ConfigSourceFactory from shared
   libraries during run-time.
 */
class ConfigSourceFactoryModule : public MModule
{
public:
   /**
      Creates the config factory implemented by this module.

      @return pointer to the factory to be DecRef()'d by the caller or NULL
    */
   virtual ConfigSourceFactory *CreateFactory() = 0;
};

/**
   The config source module interface name.

   ConfigSourceFactory::Find() uses this to find all modules containing
   ConfigSourceFactory implementations.
 */
#define CONFIG_SOURCE_INTERFACE _T("ConfigSource")

/**
   This macro is used to create ConfigSourceFactoryModule-derived classes.

   For each new ConfigSource implementation you write you also have to write a
   factory fopr creating it and, if you want to make it a module, a
   MModule-derived class as well. Doing this is completely straightforward and
   so we provide a macro to automate this.

   This macro relies on cname having a ctor taking the same parameters as our
   Create().

   @param cname the name of the ConfigSource-derived class
   @param type config source type supported by this factory, must be a literal
          constant
   @param desc the short description shown in the config sources dialog
   @param cpyright the module author/copyright string
 */
#define IMPLEMENT_CONFIG_SOURCE(cname, type, desc, cpyright)               \
   class cname##Factory : public ConfigSourceFactory                       \
   {                                                                       \
   public:                                                                 \
      cname##Factory(MModule *module)                                      \
      {                                                                    \
         m_module = module;                                                \
      }                                                                    \
                                                                           \
      virtual ~cname##Factory()                                            \
      {                                                                    \
         if ( m_module )                                                   \
            m_module->DecRef();                                            \
      }                                                                    \
                                                                           \
      virtual const char *GetType() const { return type; }                 \
                                                                           \
      virtual ConfigSource *Create(const ConfigSource& config,             \
                                   const String& name)                     \
      {                                                                    \
         return new cname(config, name);                                   \
      }                                                                    \
                                                                           \
   private:                                                                \
      MModule *m_module;                                                   \
   };                                                                      \
                                                                           \
   class cname##FactoryModule : public ConfigSourceFactoryModule           \
   {                                                                       \
   public:                                                                 \
      virtual ConfigSourceFactory *CreateFactory()                         \
      {                                                                    \
         return new cname##Factory(this);                                  \
      }                                                                    \
                                                                           \
      MMODULE_DEFINE();                                                    \
      DEFAULT_ENTRY_FUNC;                                                  \
   };                                                                      \
                                                                           \
   MMODULE_BEGIN_IMPLEMENT(cname##FactoryModule, #cname,                   \
                           CONFIG_SOURCE_INTERFACE, desc, "1.00")          \
      MMODULE_PROP("author", cpyright)                                     \
   MMODULE_END_IMPLEMENT(cname##FactoryModule)                             \
   MModule *                                                               \
   cname##FactoryModule::Init(int /* version_major */,                     \
                              int /* version_minor */,                     \
                              int /* version_release */,                   \
                              MInterface * /* minterface */,               \
                              int * /* errorCode */)                       \
   {                                                                       \
      return new cname##FactoryModule();                                   \
   }                                                                       \

#endif // _M_CONFIGSOURCE_H_

