/*-*- c++ -*-********************************************************
 * Adb - a flexible addressbook database class                      *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.4  1998/05/18 17:48:28  KB
 * more list<>->kbList changes, fixes for wxXt, improved makefiles
 *
 * Revision 1.3  1998/04/22 19:55:48  KB
 * Fixed _lots_ of problems introduced by Vadim's efforts to introduce
 * precompiled headers. Compiles and runs again under Linux/wxXt. Header
 * organisation is not optimal yet and needs further
 * cleanup. Reintroduced some older fixes which apparently got lost
 * before.
 *
 * Revision 1.2  1998/03/26 23:05:39  VZ
 * Necessary changes to make it compile under Windows (VC++ only)
 * Header reorganization to be able to use precompiled headers
 *
 * Revision 1.1  1998/03/14 12:21:18  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifdef __GNUG__
#  pragma implementation "Adb.h"
#endif

#include  "Mpch.h"
#include "Mcommon.h"

#include "MFrame.h"
#include "Adb.h"
#include "MApplication.h"
#include "MDialogs.h"
#include "strutil.h"

void
AdbNameStruct::parse(String const &in)
{
   char
      *buf2 = strutil_strdup(in),
      *buf = buf2,
      *c;
   c = strsep(&buf,";"); if(c) family = c;
   c = strsep(&buf,";"); if(c) first = c;
   c = strsep(&buf,";"); if(c) other = c;
   c = strsep(&buf,";"); if(c) prefix = c;
   c = strsep(&buf,";"); if(c) suffix = c;
   
   delete [] buf2;
}

void
AdbNameStruct::write(String &out) const
{
   out = family;
   out += ';';
   out += first;
   out += ';';
   out += other;
   out += ';';
   out += prefix;
   out += ';';
   out += suffix;
}

void
AdbAddrStruct::parse(String const &in)
{
   char
      *buf2 = strutil_strdup(in),
      *buf = buf2,
      *c;
   
   c = strsep(&buf, ";"); if(c) poBox = c;
   c = strsep(&buf,";"); if(c) extra = c;
   c = strsep(&buf,";"); if(c) street = c;
   c = strsep(&buf,";"); if(c) locality = c;
   c = strsep(&buf,";"); if(c) region = c;
   c = strsep(&buf,";"); if(c) postcode = c;
   c = strsep(&buf,";"); if(c) country = c;

   strutil_delwhitespace(poBox);
   strutil_delwhitespace(extra);
   strutil_delwhitespace(street);
   strutil_delwhitespace(locality);
   strutil_delwhitespace(region);
   strutil_delwhitespace(postcode);
   strutil_delwhitespace(country);

   delete [] buf2;
}

void
AdbAddrStruct::write(String &out) const
{
   out = poBox + ';';
   out += extra + ';';
   out += street+ ';';
   out += locality+ ';';
   out += region+ ';';
   out += postcode+ ';';
   out += country;
}

void
AdbTelStruct::parse(String const &in)
{
   char
      *buf2 = strutil_strdup(in),
      *buf = buf2,
      *c;

   if(*buf == '+')
   {
      buf++;
      c = strsep(&buf, "-");
   }
   else
      c = strsep(&buf, "-");
   if(c) country = c;
   c = strsep(&buf,"-"); if(c) area = c;
   c = strsep(&buf,";"); if(c) local = c;
   
   delete [] buf2;
}

void
AdbTelStruct::write(String  &out) const
{
   out = '+';
   out += country + '-';
   out += area + '-';
   out += local;
}

void
AdbEmailStruct::parse(String const &in)
{
   char
      *buf2 = strutil_strdup(in),
      *c, *buf = buf2,
      *cptr = NULL;
   String
      tmp;
   
   c = strsep(&buf, ";"); if(c) preferred = c;
   cptr = strsep(&buf, ";");
   while(cptr != NULL)
   {
      tmp = cptr;
      strutil_delwhitespace(tmp);
      other.push_back(new String(tmp));
      cptr = strsep(&buf, ";");
   }
   delete [] buf2;
   strutil_delwhitespace(preferred);
}

void
AdbEmailStruct::write(String  &out) const
{
   kbListIterator i;

   out = preferred;
   for(i = other.begin(); i != other.end(); i++)
      out += String(";") + *((String *)*i);
}

bool
AdbEntry::load(istream &istr)
{
   String
      line, section, value;

   bool
      inside = false; // found BEGIN ?
   
   if(istr.fail() || istr.eof())
      return false;

   for(;;)
   {
      // VZ: bad() doesn't check for failbit, and if an IO error occurs we
      //     must still exit the loop!
      if(istr.fail() || istr.eof())
         break;
      strutil_getfoldedline(istr, line);
      strutil_delwhitespace(line);
      if(*line.c_str() == '#') // comment
         continue;
      if(line.length() == 0)
         continue;
      section = strutil_before(line,':');
      strutil_toupper(section);
      value = strutil_after(line,':');
      strutil_delwhitespace(value);
      if(! inside && section == "BEGIN")
      {
         inside = true;
         continue;
      }
      else if(inside && section== "END")
         return true;
      if(inside)
      {
         if(section == "FN")
            formattedName = value;
         else if(section == "N")
            structuredName.parse(value);
         else if(section == "ADR;INTL;HOME")
            homeAddress.parse(value);
         else if(section == "ADR;INTL;WORK")
            workAddress.parse(value);
         else if(section == "TEL;WORK;VOICE")
            workPhone.parse(value);
         else if(section == "TEL;WORK;FAX")
            workFax.parse(value);
         else if(section == "TEL;HOME;VOICE")
            homePhone.parse(value);
         else if(section == "TEL;HOME;FAX")
            homeFax.parse(value);
         else if(section == "EMAIL")
            email.parse(value);
         else if(section == "TITLE")
            title = value;
         else if(section == "ORG")
            organisation = value;
         else if(section == "URL")
            url = value;
         else if(section == "BDAY")
         {
         }
         else if(section == "REV")
         {
            lastChangedDate.tm_year = atoi(value.substr(0,4).c_str());
            lastChangedDate.tm_mon = atoi(value.substr(4,2).c_str());
            lastChangedDate.tm_mday = atoi(value.substr(6,2).c_str());
            lastChangedDate.tm_hour = atoi(value.substr(9,2).c_str());
            lastChangedDate.tm_min = atoi(value.substr(11,2).c_str());
            lastChangedDate.tm_sec = atoi(value.substr(13,2).c_str());
            lastChangedDate.tm_isdst = 0;
         }
         else if(section == "ALIAS")
            alias = value;
      }
   }

   if(inside)
      return true;
   else
      return false;
}


bool
AdbEntry::save(ostream &ostr)
{
   if(! ostr || !ostr.good() )
      return false;

   String   val;

   if(formattedName.length() == 0)
   {
      if(structuredName.prefix.length())
         formattedName += structuredName.prefix + String(" ");
      if(structuredName.first.length())
         formattedName += structuredName.first + String(" ");
      if(structuredName.first.length())
         formattedName += structuredName.other + String(" ");
      if(structuredName.first.length())
         formattedName += structuredName.family + String(" ");
      if(structuredName.first.length())
         formattedName += structuredName.suffix;
   }
   
   ostr << "BEGIN" << endl;
   ostr << "FN:" << formattedName << endl;
   structuredName.write(val);
   ostr << "N:" << val << endl;
   homeAddress.write(val);
   if(val.length())
      ostr << "ADR;INTL;HOME:" << val << endl;
   workAddress.write(val);
   if(val.length())
      ostr << "ADR;INTL;WORK:" << val << endl;
   workPhone.write(val);
   if(val.length())
      ostr << "TEL;WORK;VOICE:" << val << endl;
   workFax.write(val);
   if(val.length())
      ostr << "TEL;WORK;FAX:" << val << endl;
   homePhone.write(val);
   if(val.length())
      ostr << "TEL;HOME;VOICE:" << val << endl;
   homeFax.write(val);
   if(val.length())
      ostr << "TEL;HOME;FAX:" << val << endl;
   email.write(val);
   if(val.length())
      ostr << "EMAIL:" << val << endl;
   if(title.length())
      ostr << "TITLE:" << title << endl;
   if(organisation.length())
      ostr << "ORG:" << organisation << endl;
   if(url.length())
      ostr << "URL:" << url << endl;
   if(alias.length())
      ostr << "ALIAS:" << alias << endl;

   time_t t;
   struct tm    *utc;
   time(&t);
   utc = gmtime(&t);
   ostr << "REV:";
   ostr.width(4); ostr.fill('0');
   ostr << utc->tm_year;
   ostr.width(2); ostr.fill('0');
   ostr << utc->tm_mon;
   ostr.width(2); ostr.fill('0');
   ostr << utc->tm_mday;
   ostr << 'T';
   ostr.width(2); ostr.fill('0');
   ostr  << utc->tm_hour; 
   ostr.width(2); ostr.fill('0');
   ostr << utc->tm_min; 
   ostr.width(2); ostr.fill('0');
   ostr << utc->tm_sec;
   ostr << endl;
   ostr << "END" << endl;
   return true;
}

Adb::Adb(String const &ifilename)
{
   fileName = ifilename;
   ifstream in(fileName.c_str());

   list = new AdbEntryListType(true);
   
   AdbEntry *eptr;

   for(;;)
   {
      eptr = new AdbEntry;
      if(! eptr->load(in))
      {
         delete eptr;
         break;
      }
      else
         list->push_back(eptr);
   }
   String tmp = String("Adb: read ") + strutil_ultoa(list->size()) + String(" database entries.");
   LOGMESSAGE((LOG_INFO, tmp));
}

AdbExpandListType *
Adb::Expand(String const &name)
{
   AdbEntryIterator
      i;
   AdbExpandListType
      * foundList = NULL;
   String
      a,
      b = name;
   int
      len;
   
   strutil_toupper(b);
   len = strlen(b.c_str());
   
   for(i = list->begin(); i != list->end(); i++)
   {
      a = AdbEntryCast(i)->formattedName;
      strutil_toupper(a);
      if(strutil_ncmp(a,b,len))
         goto found;
      a = AdbEntryCast(i)->structuredName.first;
      strutil_toupper(a);
      if(strutil_ncmp(a,b,len))
         goto found;
      a = AdbEntryCast(i)->structuredName.family;
      strutil_toupper(a);
      if(strutil_ncmp(a,b,len))
         goto found;
      a = AdbEntryCast(i)->structuredName.other;
      strutil_toupper(a);
      if(strutil_ncmp(a,b,len))
         goto found;
      a = AdbEntryCast(i)->alias;
      strutil_toupper(a);
      if(strutil_ncmp(a,b,len))
         goto found;
      a = AdbEntryCast(i)->email.preferred;
      if(strutil_ncmp(a,b,len))
         goto found;
      a = AdbEntryCast(i)->organisation;
      if(strutil_ncmp(a,b,len))
         goto found;
      continue;
   found:
      if(foundList == NULL)
         foundList = new AdbExpandListType(false);// don't own entries
      foundList->push_back(*i);
   }
   return foundList;
}

AdbEntry *
Adb::NewEntry(void)
{
   AdbEntry *eptr = new AdbEntry;
   AddEntry(eptr);
   return eptr;
}

void
Adb::AddEntry(AdbEntry *eptr)
{
   list->push_back(eptr);
}

void
Adb::Delete(AdbEntry *eptr)
{
   AdbEntryIterator i;
   for(i = list->begin(); i != list->end(); i++)
   {
      if(*i == eptr)
      {
         list->erase(i);
         //delete eptr; gets done by list
         return;
      }
   }
}

Adb::~Adb()
{
   AdbEntryIterator i;
   ofstream out(fileName.c_str());

   for(i = list->begin(); i != list->end(); i++)
   {
      AdbEntryCast(i)->save(out);
      //delete AdbEntryCast(i); //gets done by list
   }
   delete list;
}

AdbEntry *
Adb::Lookup(String const &key, MFrame *parent) 
{
   AdbExpandListType *list = Expand(key);
   
   if(list)
   {
      if(list->size() > 1)
         return MDialog_AdbLookupList(list, parent);
      else
         return AdbEntryCast(list->begin());
   }
   else
      wxBell();
   return NULL;
}


void
Adb::UpdateEntry(String email, String name, MFrame *parent)
{
   AdbEntry
      *entry;

   AdbExpandListType
      * list = Expand(email);

   if(! list && name.length() )
      list = Expand(name);
   
   if(list)
   {
      entry = AdbEntryCast(list->begin());  // take the first one, should be only one
      if(entry->formattedName.length() == 0)
         entry->formattedName = name;
      if(entry->email.preferred != email)
      {
         String
            msg = _("Enter the following email address as the\n" \
                    "default address for the user?\n\n");
         msg += String(_("eMail: \"")) + email + "\"\n";
         msg += String(_("Name:  \"")) + name + "\"\n";
    
         if(MDialog_YesNoDialog(msg.c_str(), parent, false, _("ADB entry"), true))
            entry->email.preferred = email;
      }
   }
   else // create new entry
   {
      entry = NewEntry();
      entry->formattedName = name;
      entry->email.preferred = email;
      Update(entry);
   }
}
