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
   /// Create a normal Profile object
   static ProfileBase * CreateProfile(String & classname, ProfileBase *parent);
   /// Create a global configuration profile object
   static ProfileBase * CreateGlobalConfig(String &  filename);
   
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
   /// query the current path
   virtual String GetPath(void);
   /// return true if the entry is defined
   virtual bool HasEntry(String & key);
   /// delete the entry group specified by path
   virtual void DeleteGroup(String & path);
   /// return the name of the profile
   virtual String GetProfileName(void);

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

