/*-*- c++ -*-********************************************************
 * Profile - managing configuration options on a per class basis    *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 *******************************************************************/

%module MProfile

%{
#include "Mswig.h"
#include "Profile.h"
%}

%import MObject.i
%import MString.i

class Profile : public MObjectRC
{
public:
   /// Creates the one global config object.
   static Profile * CreateGlobalConfig(const String & filename);
   /// Create a normal Profile object
   static Profile * CreateProfile(const String & classname,
                                      const Profile *parent);
   /// Create a Profile object for a plugin module
   static Profile * CreateModuleProfile(const String & classname,
                                            const Profile *parent);
   /// Create a dummy Profile just inheriting from the top level
   static Profile * CreateEmptyProfile(const Profile *parent);

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
   Profile() {}
};

