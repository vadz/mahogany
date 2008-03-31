///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   src/modules/spam/HeadersFilter.cpp
// Purpose:     SpamFilter implementation using header heuristics
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2004-07-10 (mostly extracted from Filters.cpp)
// CVS-ID:      $Id$
// Copyright:   (c) 2004 Vadim Zeitlin <vadim@wxwindows.org>
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
   #include "MApplication.h"

   #include "Moptions.h"
   #include "strutil.h"       // for strutil_restore_array
   #include "Mdefaults.h"     // for READ_APPCONFIG_TEXT
   #include "Profile.h"       // for readEntry

   #include <wx/filefn.h>     // for wxSplitPath
#endif //USE_PCH

#include "mail/MimeDecode.h"
#include "MailFolder.h"
#include "Message.h"

#include "adb/AdbManager.h"
#include "adb/AdbBook.h"
#include "adb/AdbEntry.h"

#include "Address.h"

#include "SpamFilter.h"
#include "gui/SpamOptionsPage.h"

#ifdef OS_MAC
   #undef USE_RBL
#endif

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_WHITE_LIST;

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

enum SpamTest
{
   Spam_Test_SpamAssasin,
   Spam_Test_Subject8Bit,
   Spam_Test_SubjectCapsOnly,
   Spam_Test_SubjectEndsJunk,
   Spam_Test_Korean,
   Spam_Test_XAuthWarn,
   Spam_Test_Received,
   Spam_Test_HTML,
   Spam_Test_BadMIME,
   Spam_Test_ExeAttachment,
#ifdef USE_RBL
   Spam_Test_RBL,
#endif // USE_RBL
   Spam_Test_WhiteList,

   Spam_Test_Max
};

// description of a spam test
struct SpamTestDesc
{
   // the token for this test in the "is_spam()" filter function argument
   const char *token;

   // the name of the key in the profile
   const char *name;

   // the label for this test in the dialog box
   const char *label;

   // on/off by default?
   bool isOnByDefault;
};

static const SpamTestDesc spamTestDescs[] =
{
   /// X-Spam-Status: Yes header?
   {
      "spamassassin",
      "SpamAssassin",
      gettext_noop("Been tagged as spam by Spam&Assassin"),
      true
   },

   /// does the subject contain too many (unencoded) 8 bit chars?
   {
      "subj8bit",
      "Spam8BitSubject",
      gettext_noop("Too many &8 bit characters in subject"),
      true
   },

   /// does the subject contain too many capital letters?
   {
      "subjcaps",
      "SpamCapsSubject",
      gettext_noop("Only &capitals in subject"),
      true
   },

   /// is the subject of the "...            xyz-12-foo" form?
   {
      "subjendjunk",
      "SpamJunkEndSubject",
      gettext_noop("&Junk at end of subject"),
      true
   },

   /// korean charset?
   {
      "korean",
      "SpamKoreanCharset",
      gettext_noop("&Korean charset"),
      false
   },

   /// suspicious X-Authentication-Warning header?
   {
      "xauthwarn",
      "SpamXAuthWarning",
      gettext_noop("X-Authentication-&Warning header"),
      false
   },

   /// suspicious Received: headers?
   {
      "received",
      "SpamReceived",
      gettext_noop("Suspicious \"&Received\" headers"),
      false
   },

   /// HTML contents at top level?
   {
      "html",
      "SpamHtml",
      gettext_noop("Only &HTML content"),
      false
   },

   /// suspicious MIME structure?
   {
      "badmime",
      "SpamMime",
      gettext_noop("Unusual &MIME structure"),
      false
   },

   /// executable attachment?
   {
      "exeattach",
      "SpamExeAttachment",
      gettext_noop("E&xecutable attachment"),
      false
   },

#ifdef USE_RBL
   /// blacklisted by the RBL?
   {
      "rbl",
      "SpamIsInRBL",
      gettext_noop("Been &blacklisted by RBL"),
      false
   },
#endif // USE_RBL

   /// no address in whitelist?
   {
      "whitelist",
      "SpamWhiteList",
      gettext_noop("... and not &whitelisted"),
      true
   },
};

// ----------------------------------------------------------------------------
// HeadersFilter class
// ----------------------------------------------------------------------------

class HeadersFilter : public SpamFilter
{
public:
   HeadersFilter() { }

protected:
   virtual void DoReclassify(const Message& msg, bool isSpam);
   virtual void DoTrain(const Message& msg, bool isSpam);
   virtual int DoCheckIfSpam(const Message& msg,
                             const String& param,
                             String *result);
   virtual const wxChar *GetOptionPageIconName() const { return _T("spam"); }
   virtual SpamOptionsPage *CreateOptionPage(wxListOrNoteBook *notebook,
                                             Profile *profile) const;


   DECLARE_SPAM_FILTER("headers", _("Heuristic headers test"), 30);
};

IMPLEMENT_SPAM_FILTER(HeadersFilter,
                      gettext_noop("Simple Heuristic Headers Analyzer"),
                      "(c) 2002-2004 Vadim Zeitlin <vadim@wxwindows.org>");


// ----------------------------------------------------------------------------
// SpamOption and derived classes, used by HeadersOptionsPage
// ----------------------------------------------------------------------------

typedef scoped_array<wxOptionsPage::FieldInfo> ArrayFieldInfo;

/*
   Represents a single spam option.

   This is an ABC, derived classes below are for the concrete options.
 */
class SpamOption
{
public:
   SpamOption(SpamTest test) : m_test(test) { }

   /// Is it on or off by default?
   virtual bool GetDefaultValue() const
   {
      return spamTestDescs[m_test].isOnByDefault;
   }

   /// The token used in spam filter string for this option
   virtual const char *Token() const
   {
      return spamTestDescs[m_test].token;
   }

   /// The name of the profile entry used to pass the value to config dialog
   virtual const char *GetProfileName() const
   {
      return spamTestDescs[m_test].name;
   }

   /// The label of the correponding checkbox in the dialog
   virtual const char *DialogLabel() const
   {
      return spamTestDescs[m_test].label;
   }

   /// The number of entries created by BuildFields()
   virtual size_t GetEntriesCount() const { return 1; }

   /// Initialize the entries needed by this option in fieldInfo
   virtual size_t BuildFieldInfo(ArrayFieldInfo& fields, size_t n) const
   {
      wxOptionsPage::FieldInfo& info = fields[n];
      info.label = DialogLabel();
      info.flags = wxOptionsPage::Field_Bool;
      info.enable = -1;

      return 1;
   }

   /// Is it currently active/checked?
   bool m_active;

protected:
   /// virtual dtor
   virtual ~SpamOption() { }

private:
   SpamTest m_test;
};


class SpamOptionAssassin : public SpamOption
{
public:
   SpamOptionAssassin() : SpamOption(Spam_Test_SpamAssasin) { }
};


class SpamOption8Bit : public SpamOption
{
public:
   SpamOption8Bit() : SpamOption(Spam_Test_Subject8Bit) { }
};


class SpamOptionCaps : public SpamOption
{
public:
   SpamOptionCaps() : SpamOption(Spam_Test_SubjectCapsOnly) { }
};


class SpamOptionJunkSubject : public SpamOption
{
public:
   SpamOptionJunkSubject() : SpamOption(Spam_Test_SubjectEndsJunk) { }
};


class SpamOptionKorean : public SpamOption
{
public:
   SpamOptionKorean() : SpamOption(Spam_Test_Korean) { }
};

class SpamOptionExeAttach : public SpamOption
{
public:
   SpamOptionExeAttach() : SpamOption(Spam_Test_ExeAttachment) { }

   virtual size_t GetEntriesCount() const
   {
      return SpamOption::GetEntriesCount() + 1;
   }

   virtual size_t BuildFieldInfo(ArrayFieldInfo& fields, size_t n) const
   {
      size_t count = SpamOption::BuildFieldInfo(fields, n);
      wxOptionsPage::FieldInfo& info = fields[n + count];
      info.label = gettext_noop(
            "(beware: this will not catch all dangerous attachments!)");
      info.flags = wxOptionsPage::Field_Message;
      info.enable = -1;

      return count + 1;
   }
};

class SpamOptionXAuth : public SpamOption
{
public:
   SpamOptionXAuth() : SpamOption(Spam_Test_XAuthWarn) { }
};


class SpamOptionReceived : public SpamOption
{
public:
   SpamOptionReceived() : SpamOption(Spam_Test_Received) { }
};


class SpamOptionHtml : public SpamOption
{
public:
   SpamOptionHtml() : SpamOption(Spam_Test_HTML) { }
};


class SpamOptionMime : public SpamOption
{
public:
   SpamOptionMime() : SpamOption(Spam_Test_BadMIME) { }
};

class SpamOptionWhiteList : public SpamOption
{
public:
   SpamOptionWhiteList() : SpamOption(Spam_Test_WhiteList) { }
};

#ifdef USE_RBL

class SpamOptionRbl : public SpamOption
{
public:
   SpamOptionRbl() : SpamOption(Spam_Test_RBL) { }
};

#endif // USE_RBL

// ----------------------------------------------------------------------------
// HeadersOptionsPage
// ----------------------------------------------------------------------------

class HeadersOptionsPage : public SpamOptionsPage
{
public:
   HeadersOptionsPage(wxListOrNoteBook *notebook, Profile *profile);

   void FromString(const String &source);
   String ToString();

private:
   ConfigValueDefault *GetConfigValues();
   FieldInfo *GetFieldInfo();

   void SetDefaults();
   void SetFalse();

   size_t GetConfigEntryCount();

   scoped_array<ConfigValueDefault> m_configValues;
   ArrayFieldInfo m_fieldInfo;

   SpamOptionAssassin m_checkSpamAssassin;
   SpamOption *PickAssassin() { return &m_checkSpamAssassin; }

   SpamOption8Bit m_check8bit;
   SpamOption *Pick8bit() { return &m_check8bit; }

   SpamOptionCaps m_checkCaps;
   SpamOption *PickCaps() { return &m_checkCaps; }

   SpamOptionJunkSubject m_checkJunkSubj;
   SpamOption *PickJunkSubject() { return &m_checkJunkSubj; }

   SpamOptionKorean m_checkKorean;
   SpamOption *PickKorean() { return &m_checkKorean; }

   SpamOptionExeAttach m_checkExeAttach;
   SpamOption *PickExeAttach() { return &m_checkExeAttach; }

   SpamOptionXAuth m_checkXAuthWarn;
   SpamOption *PickXAuthWarn() { return &m_checkXAuthWarn; }

   SpamOptionReceived m_checkReceived;
   SpamOption *PickReceived() { return &m_checkReceived; }

   SpamOptionHtml m_checkHtml;
   SpamOption *PickHtml() { return &m_checkHtml; }

   SpamOptionMime m_checkMime;
   SpamOption *PickMime() { return &m_checkMime; }

   SpamOptionWhiteList m_whitelist;
   SpamOption *PickWhiteList() { return &m_whitelist; }

#ifdef USE_RBL
   SpamOptionRbl m_checkRBL;
   SpamOption *PickRBL() { return &m_checkRBL; }
#endif // USE_RBL

   // the number of entries in controls/config arrays we need, initially 0 but
   // computed by GetConfigEntryCount() when it is called for the first time
   size_t m_nEntries;


   typedef SpamOption *(HeadersOptionsPage::*PickMember)();
   static const PickMember ms_members[];
   static const size_t ms_count;

   class Iterator
   {
   public:
      Iterator(HeadersOptionsPage *container)
         : m_container(container), m_index(0) {}

      SpamOption *operator->()
      {
         return (m_container->*HeadersOptionsPage::ms_members[m_index])();
      }
      void operator++() { if ( !IsEnd() ) m_index++; }
      bool IsEnd() { return m_index == HeadersOptionsPage::ms_count; }
      size_t Index() { return m_index; }

   private:
      HeadersOptionsPage *m_container;
      size_t m_index;
   };

   friend class HeadersOptionsPage::Iterator;
};


// ============================================================================
// HeadersFilter implementation
// ============================================================================

// ----------------------------------------------------------------------------
// different tests used by Process() below
// ----------------------------------------------------------------------------

// check whether the subject contains too many raw 8 bit characters
static bool CheckSubjectFor8Bit(const String& subject)
{
   // consider that the message is a spam if its subject contains more
   // than half of non alpha numeric chars but isn't properly encoded
   if ( subject != MIME::DecodeHeader(subject) )
   {
      // an encoded subject can have 8 bit chars
      return false;
   }

   size_t num8bit = 0,
          max8bit = subject.length() / 2;
   for ( String::const_iterator i = subject.begin(); i != subject.end(); ++i )
   {
      if ( (unsigned)*i > 127 )
      {
         if ( num8bit++ == max8bit )
         {
            return true;
         }
      }
   }

   return false;
}

// check if the subject is in capitals
static bool CheckSubjectForCapitals(const String& subject)
{
   bool hasSpace = false,
        oneWord = true;
   for ( String::const_iterator i = subject.begin(); i != subject.end(); ++i )
   {
      const wxUChar ch = *i;

      if ( (ch > 127) || islower(ch) )
      {
         // not only caps
         return false;
      }

      if ( isspace(ch) )
      {
         // remember that we have a space
         hasSpace = true;
      }
      else if ( hasSpace )
      {
         // non-space after a space -- remember that we have multiple words
         oneWord = false;
      }
   }

   // message with a single work in all caps can be legitimate but if we have a
   // sentence (i.e. more than one word here) in all caps it is very suspicious
   return !oneWord;
}

// check if the subject is of the form "...      foo-12-xyz": spammers seem to
// often tackle a unique alphanumeric tail to the subjects of their messages
static bool CheckSubjectForJunkAtEnd(const String& subject)
{
   // we look for at least that many consecutive spaces before starting looking
   // for a junk tail
   static const size_t NUM_SPACES = 6;
   static const wxChar *SPACES = _T("      ");

   // and the tail should have at least that many symbols
   static const size_t LEN_TAIL = 4;

   // and the sum of both should be at least equal to this (very short tails
   // without many preceding spaces could occur in a legal message)
   static const size_t LEN_SPACES_AND_TAIL = 15;

   const wxChar *p = wxStrstr(subject, SPACES);
   if ( !p )
      return false;

   // skip all whitespace
   const wxChar * const startSpaces = p;
   p += NUM_SPACES;
   while ( ((unsigned)*p < 128) && wxIsspace(*p) )
      p++;

   // start of the tail
   const wxChar * const startTail = p;
   while ( *p && (wxIsalnum(*p) || wxStrchr(_T("-_{}+"), *p)) )
      p++;

   // only junk (and enough of it) until the end?
   return !*p &&
            (size_t)(p - startTail) >= LEN_TAIL &&
               (size_t)(p - startSpaces) >= LEN_SPACES_AND_TAIL;
}

// check the given MIME part and all of its children for Korean charset, return
// true if any of them has it
static bool CheckMimePartForKoreanCSet(const MimePart *part)
{
   while ( part )
   {
      if ( CheckMimePartForKoreanCSet(part->GetNested()) )
         return true;

      String cset = part->GetParam(_T("charset")).Lower();
      if ( cset == _T("ks_c_5601-1987") || cset == _T("euc-kr") )
      {
         return true;
      }

      part = part->GetNext();
   }

   return false;
}

// CheckForExeAttach() helper
static bool IsExeFilename(const String& filename)
{
   if ( filename.empty() )
      return false;

   String ext;
   wxSplitPath(filename, NULL, NULL, &ext);

   // FIXME: make configurable
   static wxSortedArrayString extsExe;
   if ( extsExe.IsEmpty() )
   {
      // from http://web.mit.edu/services/mail/attachments.html#filtering
      static const wxChar *exts[] =
      {
          _T("ade"),
          _T("adp"),
          _T("bas"),
          _T("bat"),
          _T("chm"),
          _T("cmd"),
          _T("com"),
          _T("cpl"),
          _T("crt"),
          _T("eml"),
          _T("exe"),
          _T("hlp"),
          _T("hta"),
          _T("inf"),
          _T("ins"),
          _T("isp"),
          _T("jse"),
          _T("lnk"),
          _T("mdb"),
          _T("mde"),
          _T("msc"),
          _T("msi"),
          _T("msp"),
          _T("mst"),
          _T("pcd"),
          _T("pif"),
          _T("reg"),
          _T("scr"),
          _T("sct"),
          _T("shs"),
          _T("url"),
          _T("vbs"),
          _T("vbe"),
          _T("wsf"),
          _T("wsh"),
          _T("wsc"),
      };

      extsExe.Alloc(WXSIZEOF(exts));
      for ( size_t n = 0; n < WXSIZEOF(exts); n++ )
         extsExe.Add(exts[n]);
   }

   return extsExe.Index(ext.Lower()) != wxNOT_FOUND;
}

// check if we have any executable attachments in this message
static bool CheckForExeAttach(const MimePart *part)
{
   while ( part )
   {
      if ( CheckForExeAttach(part->GetNested()) )
         return true;

      if ( IsExeFilename(part->GetParam(_T("filename"))) ||
               IsExeFilename(part->GetParam(_T("name"))) )
         return true;

      part = part->GetNext();
   }

   return false;
}

// check the value of X-Spam-Status header and return true if we believe it
// indicates that this is a spam
static bool CheckXSpamStatus(const String& value)
{
   // SpamAssassin adds header "X-Spam-Status: Yes" for the messages it
   // believes to be spams, so simply check if the header value looks like this
   return value.Lower().StartsWith(_T("yes"));
}

// check the value of X-Authentication-Warning header and return true if we
// believe it indicates that this is a spam
static bool CheckXAuthWarning(const String& value)
{
   // check for "^.*Host.+claimed to be.+$" regex manually
   static const wxChar *HOST_STRING = _T("Host ");
   static const wxChar *CLAIMED_STRING = _T("claimed to be ");

   const wxChar *pc = wxStrstr(value, HOST_STRING);
   if ( !pc )
      return false;

   const wxChar *pc2 = wxStrstr(pc + 1, CLAIMED_STRING);
   if ( !pc2 )
      return false;

   // there seems to be a few common misconfiguration problems which
   // result in generation of X-Authentication-Warning headers like this
   //
   //    Host host.domain.com [ip] claimed to be host
   //
   // or like that:
   //
   //    Host alias.domain.com [ip] claimed to be mail.domain.com
   //
   // which are, of course, harmless, i.e. don't mean that this is spam, so we
   // try to filter them out

   // skip to the hostnames
   pc += wxStrlen(HOST_STRING);
   pc2 += wxStrlen(CLAIMED_STRING);

   // check if the host names are equal (case 1 above)
   const wxChar *host1 = pc,
              *host2 = pc2;

   bool hostsEqual = true;
   while ( *host1 != '.' && *host2 != '\r' )
   {
      if ( *host1++ != *host2++ )
      {
         // hosts don't match
         hostsEqual = false;

         break;
      }
   }

   // did the host names match?
   if ( hostsEqual && *host1 == '.' && *host2 == '\r' )
   {
      // they did, ignore this warning
      return false;
   }

   // check if the domains match
   const wxChar *domain1 = wxStrchr(pc, _T('.')),
                *domain2 = wxStrchr(pc2, _T('.'));

   if ( !domain1 || !domain2 )
   {
      // no domain at all -- this looks suspicious
      return true;
   }

   while ( *domain1 != ' ' && *domain2 != '\r' )
   {
      if ( *domain1++ != *domain2++ )
      {
         // domain differ as well, seems like a valid warning
         return true;
      }
   }

   // did the domains match fully?
   return *domain1 == ' ' && *domain2 == '\r';
}

// check the value of Received headers and return true if we believe they
// indicate that this is a spam
static bool CheckReceivedHeaders(const String& value)
{
   static const wxChar *FROM_UNKNOWN_STR = _T("from unknown");
   static const size_t FROM_UNKNOWN_LEN = wxStrlen(FROM_UNKNOWN_STR);

   // "Received: from unknown" is very suspicious, especially if it appears in
   // the last "Received:" header, i.e. the first one in chronological order
   const wxChar *pc = wxStrstr(value, FROM_UNKNOWN_STR);
   if ( !pc )
      return false;

   // it should be in the beginning of the header line
   if ( pc != value.c_str() && *(pc - 1) != '\n' )
      return false;

   // and it should be the last Received: header -- unfortunately there are
   // sometimes "from unknown" warnings for legitimate mail so we try to reduce
   // the number of false positives by checking the last header only as this is
   // normally the only one which the spammer directly controls
   pc = wxStrstr(pc + FROM_UNKNOWN_LEN, _T("\r\n"));
   if ( pc )
   {
      // skip the line end
      pc += 2;
   }

   // and there shouldn't be any more lines after this one
   return pc && *pc == '\0';
}

// check if we have an HTML-only message
static bool CheckForHTMLOnly(const Message& msg)
{
   const MimePart *part = msg.GetTopMimePart();
   if ( !part )
   {
      // no MIME info at all?
      return false;
   }

   // we accept the multipart/alternative messages with a text/plain (but see
   // below) and a text/html part but not the top level text/html messages
   //
   // the things are further complicated by the fact that some spammers send
   // HTML messages without MIME-Version header which results in them
   // [correctly] not being recognizad as MIME by c-client at all and so they
   // have the default type of text/plain and we have to check for this
   // (common) case specially
   const MimeType type = part->GetType();
   switch ( type.GetPrimary() )
   {
      case MimeType::TEXT:
         {
            String subtype = type.GetSubType();

            if ( subtype == _T("PLAIN") )
            {
               // check if it was really in the message or returned by c-client in
               // absence of MIME-Version
               String value;
               if ( !msg.GetHeaderLine(_T("MIME-Version"), value) )
               {
                  if ( msg.GetHeaderLine(_T("Content-Type"), value) )
                  {
                     if ( wxStrstr(value.MakeLower(), _T("text/html")) )
                        return true;
                  }
               }
            }
            else if ( subtype == _T("HTML") )
            {
               return true;
            }
         }
         break;

      case MimeType::MULTIPART:
         // if we have only one subpart and it's HTML, flag it as spam
         {
            const MimePart *subpart1 = part->GetNested();
            if ( subpart1 &&
                  !subpart1->GetNext() &&
                     subpart1->GetType() == _T("TEXT/HTML") )
            {
               return true;
            }

            if ( type.GetSubType() == _T("ALTERNATIVE") )
            {
               // although multipart/alternative messages with a text/plain and a
               // text/html type are legal, spammers sometimes send them with an
               // empty text part -- which is not
               //
               // note that a text line with 2 blank lines is going to have
               // size 4 and as no valid message is probably going to have just
               // 2 letters, let's consider messages with small size empty too
               if ( subpart1->GetType() == _T("TEXT/PLAIN") &&
                           subpart1->GetSize() < 5 )
               {
                  const MimePart *subpart2 = subpart1->GetNext();
                  if ( subpart2 &&
                        !subpart2->GetNext() &&
                           subpart2->GetType() == _T("TEXT/HTML") )
                  {
                     // multipart/alternative message with text/plain and html
                     // subparts [only] and the text part is empty -- junk
                     return true;
                  }
               }
            }
         }
         break;

      default:
         // it's a MIME message but of non TEXT type
         ;
   }

   return false;
}

// check if we have a message with "suspicious" MIME structure
static bool CheckForSuspiciousMIME(const Message& msg)
{
   // we consider multipart messages with only one part suspicious: although
   // formally valid, there is really no legitimate reason to send them
   //
   // the sole exception is for multipart/mixed messages with a single part:
   // although they are weird too, there *is* some reason to send them, namely
   // to protect the Base 64 encoded parts from being corrupted by the mailing
   // list trailers and although no client known to me does this currently it
   // may happen in the future
   const MimePart *part = msg.GetTopMimePart();
   if ( !part ||
            part->GetType().GetPrimary() != MimeType::MULTIPART ||
               part->GetType().GetSubType() == _T("MIXED") )
   {
      // either not multipart at all or multipart/mixed which we don't check
      return false;
   }

   // only return true if we have exactly one subpart
   const MimePart *subpart = part->GetNested();

   return subpart && !subpart->GetNext();
}

// check whether any address field (sender or recipient) matches whitelist and
// return true (and fills match with the match in whitelist) if it does
static bool CheckWhiteList(const Message& msg, String *match)
{
   const wxArrayString
      whitelist(strutil_restore_array(READ_APPCONFIG_TEXT(MP_WHITE_LIST)));
   if ( whitelist.empty() )
      return false;

   // examine all addresses in the message header for match in the whitelist
   wxArrayString addresses;
   const size_t count = msg.ExtractAddressesFromHeader(addresses);
   if ( !count )
      return false;

   for ( size_t n = 0; n < count; n++ )
   {
      if ( Address::IsInList(whitelist, addresses[n], match) )
         return true;
   }

   return false;
}

#ifdef USE_RBL


#define __STRICT_ANSI__
#include <netinet/in.h>

// FreeBSD uses a variable name "class"
#define class xxclass
#undef MIN  // defined by glib.h
#undef MAX  // defined by glib.h
#include <arpa/nameser.h>
#undef class

#include <resolv.h>
#include <netdb.h>

#ifdef OS_SOLARIS
   extern "C" {
   // Solaris 2.5.1 has no prototypes for it:
   extern int res_init(void);
   extern int res_query(const char *, int , int ,
                        unsigned char *, int);
   };
#endif

#define BUFLEN 512

/* rblcheck()
 * Checks the specified dotted-quad address against the provided RBL
 * domain. If "txt" is non-zero, we perform a TXT record lookup. We
 * return the text returned from a TXT match, or an empty string, on
 * a successful match, or NULL on an unsuccessful match. */
static
bool CheckRBL( int a, int b, int c, int d, const String & rblDomain)
{
   char * answerBuffer = new char[ BUFLEN ];
   int len;

   String domain;
   domain.Printf(_T("%d.%d.%d.%d.%s"), d, c, b, a, rblDomain.c_str() );

   res_init();
   len = res_query( domain.ToAscii(), C_IN, T_A,
                    (unsigned char *)answerBuffer, PACKETSZ );

   if ( len != -1 )
   {
      if ( len > PACKETSZ ) // buffer was too short, re-alloc:
      {
         delete [] answerBuffer;
         answerBuffer = new char [ len ];
         // and again:
         len = res_query( domain.ToAscii(), C_IN, T_A,
                          (unsigned char *) answerBuffer, len );
      }
   }
   delete [] answerBuffer;
   return len != -1; // found, so it's known spam
}

static const wxChar * gs_RblSites[] =
{ _T("rbl.maps.vix.com"), _T("relays.orbs.org"), _T("rbl.dorkslayers.com"), NULL };

static bool findIP(String &header,
                   char openChar, char closeChar,
                   int *a, int *b, int *c, int *d)
{
   String toExamine;
   String ip;
   int pos = 0, closePos;

   toExamine = header;
   while(toExamine.Length() > 0)
   {
      if ((pos = toExamine.Find(openChar)) == wxNOT_FOUND)
         break;
      ip = toExamine.Mid(pos+1);
      closePos = ip.Find(closeChar);
      if (closePos == wxNOT_FOUND)
         // no second bracket found
         break;
      if (wxSscanf(ip.c_str(), _T("%d.%d.%d.%d"), a,b,c,d) != 4)
      {
         // no valid IP number behind open bracket, continue
         // search:
         ip = ip.Mid(closePos+1);
            toExamine = ip;
      }
      else // read the IP number
      {
         header = ip.Mid(closePos+1);
            return true;
      }
   }
   header = wxEmptyString;
   return false;
}

#endif // USE_RBL

// ----------------------------------------------------------------------------
// HeadersFilter itself
// ----------------------------------------------------------------------------

void HeadersFilter::DoReclassify(const Message& /* msg */, bool /* isSpam */)
{
   // we're too stupid to allow user to correct our erros -- nothing to do
}

void HeadersFilter::DoTrain(const Message& /* msg */, bool /* isSpam */)
{
   // we're too stupid to be trained -- nothing to do
}

int
HeadersFilter::DoCheckIfSpam(const Message& msg,
                             const String& param,
                             String *result)
{
   wxArrayString tests = strutil_restore_array(param);
   size_t n;
   if ( tests.empty() )
   {
      // use defaults
      Profile_obj profile(Profile::CreateModuleProfile(SPAM_FILTER_INTERFACE));

      for ( n = 0; n < Spam_Test_Max; n++ )
      {
         const SpamTestDesc& desc = spamTestDescs[n];
         if ( profile->readEntry(desc.name, desc.isOnByDefault) )
            tests.Add(desc.token);
      }
   }

   const size_t count = tests.size();

   // check the white list first because it overrides all the others
   for ( n = 0; n < count; n++ )
   {
      if ( tests[n] == spamTestDescs[Spam_Test_WhiteList].token )
      {
         String match;
         if ( CheckWhiteList(msg, &match) )
         {
            if ( result )
               result->Printf("\"%s\" is in the white list", match.c_str());

            // this is definitely not a spam
            return -1;
         }
      }
   }


   // now check all the other tests
   String value,
          spamResult;
   for ( n = 0; n < count && spamResult.empty(); n++ )
   {
      const wxString& test = tests[n];
      if ( test == spamTestDescs[Spam_Test_Subject8Bit].token )
      {
         if ( CheckSubjectFor8Bit(msg.Subject()) )
            spamResult = _("8 bit subject");
      }
      else if ( test == spamTestDescs[Spam_Test_SubjectCapsOnly].token )
      {
         if ( CheckSubjectForCapitals(msg.Subject()) )
            spamResult = _("only caps in subject");
      }
      else if ( test == spamTestDescs[Spam_Test_SubjectEndsJunk].token )
      {
         if ( CheckSubjectForJunkAtEnd(msg.Subject()) )
            spamResult = _("junk at end of subject");
      }
      else if ( test == spamTestDescs[Spam_Test_Korean].token )
      {
         // detect all Korean charsets -- and do it for all MIME parts, not
         // just the top level one
         if ( CheckMimePartForKoreanCSet(msg.GetTopMimePart()) )
            spamResult = _("message in Korean");
      }
      else if ( test == spamTestDescs[Spam_Test_SpamAssasin].token )
      {
         // SpamAssassin adds header "X-Spam-Status: Yes" to all (probably)
         // detected spams
         if ( msg.GetHeaderLine(_T("X-Spam-Status"), value) &&
                  CheckXSpamStatus(value) )
         {
            spamResult = _("tagged by SpamAssassin");
         }
      }
      else if ( test == spamTestDescs[Spam_Test_XAuthWarn].token )
      {
         // unfortunately not only spams have this header but we consider that
         // only spammers change their address in such way
         if ( msg.GetHeaderLine(_T("X-Authentication-Warning"), value) &&
                  CheckXAuthWarning(value) )
         {
            spamResult = _("contains X-Authentication-Warning");
         }
      }
      else if ( test == spamTestDescs[Spam_Test_Received].token )
      {
         if ( msg.GetHeaderLine(_T("Received"), value) &&
                  CheckReceivedHeaders(value) )
         {
            spamResult = _("suspicious \"Received:\"");
         }
      }
      else if ( test == spamTestDescs[Spam_Test_BadMIME].token )
      {
         if ( CheckForSuspiciousMIME(msg) )
            spamResult = _("suspicious MIME structure");
      }
      else if ( test == spamTestDescs[Spam_Test_HTML].token )
      {
         if ( CheckForHTMLOnly(msg) )
            spamResult = _("pure HTML content");
      }
      else if ( test == spamTestDescs[Spam_Test_ExeAttachment].token )
      {
         if ( CheckForExeAttach(msg.GetTopMimePart()) )
            spamResult = _("contains Windows executable attachment");
      }
#ifdef USE_RBL
      else if ( test == spamTestDescs[Spam_Test_RBL].token )
      {
         msg.GetHeaderLine(_T("Received"), value);

         int a,b,c,d;
         String testHeader = value;

         bool rc = false;
         while ( !rc && !testHeader.empty() )
         {
            if (findIP(testHeader, '(', ')', &a, &b, &c, &d))
            {
               for ( int i = 0; gs_RblSites[i]; ++i )
               {
                  if ( CheckRBL(a,b,c,d,gs_RblSites[i]) )
                  {
                     rc = true;
                     break;
                  }
               }
            }
         }

         if ( !rc )
         {
            testHeader = value;
            while ( !rc && !testHeader.empty() )
            {
               if (findIP(testHeader, '[', ']', &a, &b, &c, &d))
               {
                  for ( int i = 0; gs_RblSites[i] ; ++i )
                  {
                     if ( CheckRBL(a,b,c,d,gs_RblSites[i]) )
                     {
                        rc = true;
                        break;
                     }
                  }
               }
            }
         }

         /*FIXME: if it is a hostname, maybe do a DNS lookup first? */

         if ( rc )
            spamResult = _("blacklisted by RBL");
      }
#endif // USE_RBL
      //else: simply ignore unknown tests, don't complain as it would be
      //      too annoying probably
   }

   if ( spamResult.empty() )
      return false;

   if ( result )
      *result = spamResult;

   return true;
}

// ----------------------------------------------------------------------------
// HeadersFilter configuration
// ----------------------------------------------------------------------------

SpamOptionsPage *
HeadersFilter::CreateOptionPage(wxListOrNoteBook *notebook,
                                Profile *profile) const
{
   return new HeadersOptionsPage(notebook, profile);
}

// ============================================================================
// HeadersOptionsPage
// ============================================================================

const HeadersOptionsPage::PickMember HeadersOptionsPage::ms_members[] =
{
   &HeadersOptionsPage::PickAssassin,
   &HeadersOptionsPage::Pick8bit,
   &HeadersOptionsPage::PickCaps,
   &HeadersOptionsPage::PickJunkSubject,
   &HeadersOptionsPage::PickKorean,
   &HeadersOptionsPage::PickExeAttach,
   &HeadersOptionsPage::PickXAuthWarn,
   &HeadersOptionsPage::PickReceived,
   &HeadersOptionsPage::PickHtml,
   &HeadersOptionsPage::PickMime,
   &HeadersOptionsPage::PickWhiteList,
#ifdef USE_RBL
   &HeadersOptionsPage::PickRBL,
#endif // USE_RBL
};

const size_t
   HeadersOptionsPage::ms_count = WXSIZEOF(HeadersOptionsPage::ms_members);


HeadersOptionsPage::HeadersOptionsPage(wxListOrNoteBook *notebook,
                                       Profile *profile)
                  : SpamOptionsPage(notebook, profile)
{
   m_nEntries = 0;

   Create
   (
      notebook,
      gettext_noop("Headers filter"),
      profile,
      GetFieldInfo(),
      GetConfigValues(),
      GetConfigEntryCount(),
      0,
      -1,
      notebook->GetPageCount()
   );
}

size_t HeadersOptionsPage::GetConfigEntryCount()
{
   if ( !m_nEntries )
   {
      // sum up the entries of all option
      for ( HeadersOptionsPage::Iterator option(this);
            !option.IsEnd();
            ++option )
      {
         m_nEntries += option->GetEntriesCount();
      }

      // +1 for the explanation text in the beginning
      m_nEntries++;
   }

   return m_nEntries;
}

void HeadersOptionsPage::SetDefaults()
{
   for ( HeadersOptionsPage::Iterator option(this); !option.IsEnd(); ++option )
   {
      option->m_active = option->GetDefaultValue();
   }
}

void HeadersOptionsPage::SetFalse()
{
   for ( HeadersOptionsPage::Iterator option(this); !option.IsEnd(); ++option )
   {
      option->m_active = false;
   }
}

void HeadersOptionsPage::FromString(const String &source)
{
   if ( source.empty() )
   {
      SetDefaults();
      return;
   }

   SetFalse();

   const wxArrayString tests = strutil_restore_array(source);

   const size_t count = tests.GetCount();
   for ( size_t token = 0; token < count; token++ )
   {
      const wxString& actual = tests[token];

      HeadersOptionsPage::Iterator option(this);
      while ( !option.IsEnd() )
      {
         if ( actual == option->Token() )
         {
            option->m_active = true;
            break;
         }

         ++option;
      }

      if ( option.IsEnd() )
      {
         FAIL_MSG(String::Format(_T("Unknown spam test \"%s\""), actual.c_str()));
      }
   }
}

String HeadersOptionsPage::ToString()
{
   String result;

   for ( HeadersOptionsPage::Iterator option(this); !option.IsEnd(); ++option )
   {
      if ( option->m_active )
      {
         if ( !result.empty() )
            result += _T(':');

         result += option->Token();
      }
   }

   return result;
}

ConfigValueDefault *HeadersOptionsPage::GetConfigValues()
{
   // ConfigValueDefault doesn't have default ctor, so use this hack knowing
   // that ConfigValueNone has exactly the same binary layout as ValueDefault
   m_configValues.reset(new ConfigValueNone[GetConfigEntryCount()]);

   size_t n = 1;
   for ( HeadersOptionsPage::Iterator option(this); !option.IsEnd(); ++option )
   {
      ConfigValueDefault& value = m_configValues[n];
      value = ConfigValueDefault
              (
                  option->GetProfileName(),
                  option->GetDefaultValue()
              );
      n += option->GetEntriesCount();
   }

   return m_configValues.get();
}

wxOptionsPage::FieldInfo *HeadersOptionsPage::GetFieldInfo()
{
   m_fieldInfo.reset(new wxOptionsPage::FieldInfo[GetConfigEntryCount()]);

   m_fieldInfo[0].label
      = gettext_noop("Mahogany may use several heuristic tests to detect spam.\n"
                     "Please choose the ones you'd like to be used by checking\n"
                     "the corresponding entries.\n"
                     "\n"
                     "So the message is considered to be spam if it has...");
   m_fieldInfo[0].flags = wxOptionsPage::Field_Message;
   m_fieldInfo[0].enable = -1;

   size_t n = 1;
   for ( HeadersOptionsPage::Iterator option(this); !option.IsEnd(); ++option )
   {
      n += option->BuildFieldInfo(m_fieldInfo, n);
   }

   return m_fieldInfo.get();
}

