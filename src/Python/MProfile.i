/*-*- c++ -*-********************************************************
 * Profile - managing configuration options on a per class basis    *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 *******************************************************************/
%module MProfile

%{
#include  "Mcommon.h"
#include  "Profile.h"

// we don't want to export our functions as we don't build a shared library
#undef SWIGEXPORT
#define SWIGEXPORT(a,b) a b
%}

%import MObject.i
%import MString.i

class ProfileBase : public MObjectRC
{
public:

   /** An enum explaining the possible types of profiles. In fact,
       just a number stored as a normal profile entry which must be
       maintained manually.
   */
   enum Type
   {
      /// No profile type specified.
      PT_Unknown = -1,
      /// Any profile, default argument for ListProfiles().
      PT_Any = -1,  // PT_Unknown
      /// This profile belongs to a folder.
      PT_FolderProfile = 1
   };

   /// Creates the one global config object.
   static ProfileBase * CreateGlobalConfig(const String & filename);
   /// Create a normal Profile object
   static ProfileBase * CreateProfile(const String & classname,
                                      const ProfileBase *parent);
   /// Create a Profile object for a plugin module
   static ProfileBase * CreateModuleProfile(const String & classname,
                                            const ProfileBase *parent);
   /// Create a dummy Profile just inheriting from the top level
   static ProfileBase * CreateEmptyProfile(const ProfileBase *parent);

   /// Delete the global config object
   static void DeleteGlobalConfig();

   /**@name Reading and writing entries.
      All these functions are just identical to the wxConfig ones.
   */
   //@{
   /// Read a character entry.
   String readEntry(String &  key,char *defaultvalue);
   /// Write back the character value.
   virtual bool writeEntry(String & key, String & Value);
   //@}
   /// set the path within the profile,just like cd
   virtual void   SetPath(String & path);
   /// return true if the entry is defined
   virtual bool HasEntry(String & key);
   /// delete the entry group specified by path
   virtual void DeleteGroup(String & path);
   /// return the name of the profile
   virtual String GetName(void);

   /** @name Managing environment variables

       just expose wxConfig methods (we do need them to be able to read the
       real config values, i.e. to disable expansion, sometimes)
   */
   /// are we automatically expanding env vars?
   virtual bool IsExpandingEnvVars();
   /// set the flag which tells if we should auto expand env vars
   virtual void SetExpandEnvVars(bool bDoIt = TRUE);

protected:
   /// why does egcs want this?
   ProfileBase() {}
};

