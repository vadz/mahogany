/*-*- c++ -*-********************************************************
 * Adb - a flexible addressbook database class                      *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.2  1998/03/26 23:05:35  VZ
 * Necessary changes to make it compile under Windows (VC++ only)
 * Header reorganization to be able to use precompiled headers
 *
 * Revision 1.1  1998/03/14 12:21:10  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifndef ADB_H
#define ADB_H

#ifdef __GNUG__
#pragma interface "Adb.h"
#endif

/**@name Adb classes */
//@{

struct AdbNameStruct
   {
      String	family;
      String	first;
      String	other;
      String	prefix;
      String	suffix;

      void parse(String const &in);
      void write(String &out) const;
};

struct AdbAddrStruct
{
   String	poBox;
   String	extra;
   String	street;
   String	locality;
   String	region;
   String	postcode;
   String	country;

   void parse(String const &in);
   void write(String &out) const;
};

struct AdbTelStruct
{
   String	country;
   String	area;
   String	local;

   void parse(String const &in);
   void write(String &out) const;
};

struct AdbEmailStruct
{
   String	preferred;
   std::list<String> other;

   void parse(String const &in);
   void write(String &out) const;
};

class AdbEntry
{
public:
   String	 formattedName;
   AdbNameStruct structuredName;
   String	 title;
   String	 organisation;
   AdbAddrStruct homeAddress;
   AdbAddrStruct workAddress;
   AdbTelStruct	 workPhone;
   AdbTelStruct	 workFax;
   AdbTelStruct	 homePhone;
   AdbTelStruct	 homeFax;
   AdbEmailStruct email;
   String	  alias;
   String	 url;
   struct tm	 lastChangedDate;
   struct tm	 birthDay;
   bool		 empty;
   
   bool load(istream &istream);
   bool save(ostream &istream);
   AdbEntry() { empty = true; }
   bool Empty(void) { return empty; }
   ~AdbEntry() {}
};


typedef	std::list<AdbEntry *>		AdbEntryListType;
typedef AdbEntryListType::iterator	AdbEntryIterator;

typedef	std::list<AdbEntry *> 		AdbExpandListType;
typedef	AdbExpandListType::iterator 	AdbExpandListIterator;

/**
   Adb: an address database class
*/
class Adb : public CommonBase , public AdbEntryListType
{
private:
   String	fileName;
public:
   /**
      Constructor.
      @param filename the database file to be associated with
   */
   Adb(String const &filename);

   /**
      Destructor.
   */
   ~Adb();


   AdbExpandListType *Expand(String const &name);

   AdbEntry *NewEntry(void);
   void AddEntry(AdbEntry *eptr);
   /// creates a new entry if it doesn't exist yet
   void UpdateEntry(String email, String name = "", MFrame *parent = NULL);
   void Update(AdbEntry *eptr) {};	// mark it as changed
   void Delete(AdbEntry *eptr);
   AdbEntry *Lookup(String const &key, MFrame *parent = NULL); 
   /// initialised == there is a list of paths
   bool	IsInitialised(void) const { return true; }

   CB_DECLARE_CLASS(Adb, CommonBase);
};

//@}

#endif
