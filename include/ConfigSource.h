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

#ifdef __GNUG__
   #pragma interface "ConfigSource.h"
#endif

#include "MObject.h"

#include "MModule.h"

class WXDLLEXPORT wxConfigBase;

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
    */
   static ConfigSource *CreateDefault(const String& filename);

   /**
      Create the config source using the parameters from the given section.

      This function uses the "Type" value to find the ConfigSourceFactory to
      use and then calls its Create().

      @param config the config containing the parameters
      @param name its section name, without the trailing slash
    */
   static ConfigSource *Create(const ConfigSource& config, const String& name);

   //@}


   /**
      @name Object description.
    */
   //@{

   /// Get the (untranslated and hence not user-readable) name of this object
   const String& GetName() const { return m_name; }

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
   virtual bool Read(const String& name, String *value) = 0;

   /**
      Read an integer

      @param name the name of the key to read it from
      @param value pointer to the result, must not be NULL!
    */
   virtual bool Read(const String& name, long *value) = 0;

   //@}


   /**
      @name Writing data.

      As above, onlky strings and longs are directly supported.

      All methods below return true if ok and false on error -- this should be
      checked for and handled!
    */
   //@{

   /// Write a string
   virtual bool Write(const String& name, const String& value) = 0;

   /// Write an integer
   virtual bool Write(const String& name, long value) = 0;

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
   virtual bool GetFirstGroup(String& group, long& cookie) const = 0;

   /// Get the next subgroup of the given key
   virtual bool GetNextGroup(String& group, long& cookie) const = 0;

   /// Get the first entry of the given key
   virtual bool GetFirstEntry(String& entry, long& cookie) const = 0;

   /// Get the next entry of the given key
   virtual bool GetNextEntry(String& entry, long& cookie) const = 0;

   //@}


   /**
      @name Deleting entries and groups.
    */
   //@{

   /// Delete the given entry
   virtual bool DeleteEntry(const String& name) = 0;

   /// Delete the group with all its subgroups (TAKE CARE!)
   virtual bool DeleteGroup(const String& name) = 0;

   //@}


protected:
   /// Constrructor is protected, you can only create derived classes
   ConfigSource(const String& name) : m_name(name) { }

   /// Virtual destructor as for any base class
   virtual ~ConfigSource();

private:
   /// the name of this object, it is set once on creation and can't be changed
   const String m_name;
};


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
      Create a local config source.

      If the filename parameter is empty, the default location is used.

      @sa ConfigSource::CreateDefault()

      @param filename the name of config file or empty
      @param name the name for the ConfigSource object
    */
   ConfigSourceLocal(const String& filename, const String& name = _T(""));

   // implement base class pure virtuals
   virtual bool IsOk() const;
   virtual bool IsLocal() const;
   virtual bool Read(const String& name, String *value);
   virtual bool Read(const String& name, long *value);
   virtual bool Write(const String& name, const String& value);
   virtual bool Write(const String& name, long value);
   virtual bool GetFirstGroup(String& group, long& cookie) const;
   virtual bool GetNextGroup(String& group, long& cookie) const;
   virtual bool GetFirstEntry(String& entry, long& cookie) const;
   virtual bool GetNextEntry(String& entry, long& cookie) const;
   virtual bool DeleteEntry(const String& name);
   virtual bool DeleteGroup(const String& name);

private:
   wxConfigBase *m_config;
};


/**
   ConfigSourceFactory creates ConfigSource objects.

   This class is used to create config sources from their types.

   It derives from MModule which means that config source factories can be
   loaded during run-time as/if needed.
 */
class ConfigSourceFactory : public MModule
{
public:
   /**
      Finds the factory for creating the config sources of given type.

      @param type the config source type ("file", "imap", ...)
      @return the factory for the objects of this type or NULL if none
    */
   static ConfigSourceFactory *Find(const String& type);

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
};

DECLARE_AUTOPTR(ConfigSourceFactory);

/**
   The config source module interface name.

   ConfigSourceFactory::Find() uses this to find all modules containing
   ConfigSourceFactory implementations.
 */
#define CONIFG_SOURCE_INTERFACE _T("ConfigSource")

/**
   This macro is used to create ConfigSourceFactory-derived classes.

   The implementation of classes deriving from ConfigSourceFactory is
   straightforward: we just have to create an object of the specified class in
   Create() and return it so we provide a macro to automate this.

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
      virtual const char *GetType() const { return type; }                 \
                                                                           \
      virtual ConfigSource *Create(const ConfigSource& config,             \
                                   const St& name)                         \
      {                                                                    \
         cname *configNew = new cname(config, name);                       \
         if ( !configNew->IsOk() )                                         \
         {                                                                 \
            delete configNew;                                              \
            configNew = NULL;                                              \
         }                                                                 \
                                                                           \
         return configNew;                                                 \
      }                                                                    \
                                                                           \
      MMODULE_DEFINE();                                                    \
      DEFAULT_ENTRY_FUNC;                                                  \
   };                                                                      \
   MMODULE_BEGIN_IMPLEMENT(cname##Factory, #cname,                         \
                           CONIFG_SOURCE_INTERFACE, desc, "1.00")          \
      MMODULE_PROP("author", cpyright)                                     \
   MMODULE_END_IMPLEMENT(cname##Factory)                                   \
   MModule *cname##Factory::Init(int /* version_major */,                  \
                                 int /* version_minor */,                  \
                                 int /* version_release */,                \
                                 MInterface * /* minterface */,            \
                                 int * /* errorCode */)                    \
   {                                                                       \
      return new cname##Factory();                                         \
   }

#endif // _M_CONFIGSOURCE_H_

