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

   #include <wx/filefn.h>     // for wxSplitPath
#endif //USE_PCH

#include "MailFolder.h"
#include "Message.h"

#include "adb/AdbManager.h"
#include "adb/AdbBook.h"
#include "adb/AdbEntry.h"

#include "Address.h"

#include "SpamFilter.h"

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_WHITE_LIST;

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
   virtual bool DoCheckIfSpam(const Message& msg,
                              const String& param,
                              String *result);


   DECLARE_SPAM_FILTER("headers", 30);
};

IMPLEMENT_SPAM_FILTER(HeadersFilter,
                      gettext_noop("Heuristic Headers Analyzer"),
                      _T("(c) 2002-2004 Vadim Zeitlin <vadim@wxwindows.org>"));

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
   if ( subject != MailFolder::DecodeHeader(subject) )
   {
      // an encoded subject can have 8 bit chars
      return false;
   }

   size_t num8bit = 0,
          max8bit = subject.length() / 2;
   for ( const unsigned char *pc = (const unsigned char *)subject.c_str();
         *pc;
         pc++ )
   {
      if ( *pc > 127 || *pc == '?' || *pc == '!' )
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
   bool hasSpace = false;
   for ( const unsigned char *pc = (const unsigned char *)subject.c_str();
         *pc;
         pc++ )
   {
      if ( (*pc > 127) || islower(*pc) )
      {
         // not only caps
         return false;
      }

      if ( isspace(*pc) )
      {
         // remember that we have more than one word
         hasSpace = true;
      }
   }

   // message with a single work in all caps can be legitimate but if we have a
   // sentence (i.e. more than one word here) in all caps it is very suspicious
   return hasSpace;
}

// check if the subject is of the form "...      foo-12-xyz": spammers seem to
// often tackle a unique alphanumeric tail to the subjects of their messages
static bool CheckSubjectForJunkAtEnd(const String& subject)
{
   // we look for at least that many consecutive spaces before starting looking
   // for a junk tail
   static const size_t NUM_SPACES = 6;

   // and the tail should have at least that many symbols
   static const size_t LEN_TAIL = 4;

   // and the sumb of both should be at least equal to this (very short tails
   // without many preceding spaces could occur in a legal message)
   static const size_t LEN_SPACES_AND_TAIL = 15;

   const wxChar *p = wxStrstr(subject, String(_T(' '), NUM_SPACES));
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
   const wxChar *domain1 = wxStrchr(pc, '.'),
                *domain2 = wxStrchr(pc2, '.');

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

static wxArrayString GenerateSuperDomains(const String &domain)
{
   wxArrayString parts(strutil_restore_array(domain,_T('.')));

   wxArrayString result;
   for( size_t count = 1; count <= parts.GetCount(); ++count )
   {
      String super;
      for( size_t each = 0; each < count; ++each )
      {
         if( each )
            super += _T('.');
         super += parts[parts.GetCount() - count + each];
      }

      result.Add(super);
   }

   return result;
}

static bool WhiteListDomain(RefCounter<AdbBook> book,const String &candidate)
{
   wxArrayString domains(GenerateSuperDomains(candidate));

   for( size_t super = 0; super < domains.GetCount(); ++super )
   {
      // FIXME: Grammar without escape sequences
      if( book->Matches(String(_T("*@"))+domains[super],
            AdbLookup_EMail,AdbLookup_Match) )
      {
         return false;
      }

      // Allow whitelisting domain itself
      if( book->Matches(domains[super],AdbLookup_EMail,AdbLookup_Match) )
      {
         return false;
      }
   }

   return true;
}

static bool WhiteListListId(RefCounter<AdbBook> book,const String &id)
{
   size_t left = id.find(_T('<'));
   if( left != String::npos )
   {
      size_t right = id.find(_T('>'),left);
      if( right != String::npos )
      {
         size_t at = id.find(_T('@'),left);

         size_t begin;
         if( at != String::npos && at < right )
            begin = at;
         else
            begin = left;

         if( !WhiteListDomain(book,id.substr(begin+1,right-(begin+1))) )
            return false;
      }
   }

   return true;
}

// This is not RFC-compliant address parser and it won't be because
// it would then generate many false matches. It isn't problem because
// it is used for parsing non-standard headers anyway.
static bool IsSaneAddressCharacter(wxChar byte)
{
   return byte >= _T('a') && byte <= _T('z')
      || byte >= _T('A') && byte <= _T('Z')
      || byte >= _T('0') && byte <= _T('9')
      || byte == _T('@')
      || byte == _T('.')
      || byte == _T('-')
      || byte == _T('+')
      || byte == _T('=')
      || byte == _T('_')
      || byte == _T('~');
}

// Extract address(es) from messy string into parseable format
static String AddressFromFreeStyleHeader(const String &in)
{
   String out;

   for( size_t at = in.find(_T('@')); at != String::npos;
      at = in.find(_T('@'),at+1) )
   {
      size_t begin = at;
      for(;;)
      {
         if( !IsSaneAddressCharacter(in[begin])
            || begin < at && in[begin] == _T('@') )
         {
            ++begin;
            break;
         }
         if( begin == 0 )
            break;
         --begin;
      }

      size_t end = at;
      for(;;)
      {
         if( !IsSaneAddressCharacter(in[end])
            || end > at && in[end] == _T('@') )
         {
            --end;
            break;
         }
         if( end == in.size()-1 )
            break;
         ++end;
      }

      if( begin < at && at < end )
      {
         size_t dot = in.find(_T('.'),at+1);
         if( dot != String::npos && dot < end )
         {
            if( !out.empty() )
               out += _T(',');
            out += in.substr(begin,end-begin+1);
         }
      }
   }

   return out;
}

static const wxChar *gs_whitelistHeaders[] =
{
   // Source
   _T("From"),
   _T("Sender"),
   _T("Reply-To"),
   // Destination
   _T("To"),
   _T("Cc"),
   _T("Bcc"),
   // List
   //_T("List-Id"), // List-Id may contain all sorts of garbage (see below)
   _T("List-Help"),
   _T("List-Subscribe"),
   _T("List-Unsubscribe"),
   _T("List-Post"),
   _T("List-Owner"),
   _T("List-Archive"),
   // Obscure
   _T("Resent-To"),
   _T("Resent-Cc"),
   _T("Resent-Bcc"),
   _T("Resent-From"),
   _T("Resent-Sender"),
   // Non-standard - some entries can be probably removed without danger
   _T("Envelope-To"), // Exim
   _T("X-Envelope-To"), // Sendmail
   _T("Apparently-To"), // Procmail ^TO
   _T("X-Envelope-From"), // Procmail ^FROM_DAEMON
   _T("Mailing-List"), // Ezmlm and Yahoo
   _T("X-Mailing-List"), // SmartList
   _T("X-BeenThere"), // Mailman
   _T("Delivered-To"), // qmail
   _T("X-Delivered-To"), // fastmail.fm
   _T("X-Original-To"), // postfix 2.0
   _T("X-Rcpt-To"), // best.com
   _T("X-Real-To"), // CommuniGate Pro
   _T("X-MDMailing-List"), // Nancy Mc'Gough's Procmail Faq
   _T("Return-Path"), // Nancy Mc'Gough's Procmail Faq
   NULL
};

static enum
{
   AddressMessStandard, // Standard address fields can be parsed directly
   AddressMessUrl, // List headers contain URLs and only few are e-mails
   AddressMessGarbage, // Free style text that may contain addresses
   AddressMessUnknown // Treated the same as AddressMessGarbage
}
gs_whitelistHeaderMess[] =
{
   AddressMessStandard, // From
   AddressMessStandard, // Sender
   AddressMessStandard, // Reply-To
   AddressMessStandard, // To
   AddressMessStandard, // Cc
   AddressMessStandard, // Bcc
   AddressMessUrl, // List-Help
   AddressMessUrl, // List-Subscribe
   AddressMessUrl, // List-Unsubscribe
   AddressMessUrl, // List-Post
   AddressMessUrl, // List-Owner
   AddressMessUrl, // List-Archive
   AddressMessStandard, // Resent-To
   AddressMessStandard, // Resent-Cc
   AddressMessStandard, // Resent-Bcc
   AddressMessStandard, // Resent-From
   AddressMessStandard, // Resent-Sender
   AddressMessUnknown, // Envelope-To
   AddressMessUnknown, // X-Envelope-To
   AddressMessUnknown, // Apparently-To
   AddressMessUnknown, // X-Envelope-From
   AddressMessGarbage, // Mailing-List
   AddressMessGarbage, // X-Mailing-List
   AddressMessUnknown, // X-BeenThere
   AddressMessGarbage, // Delivered-To
   AddressMessGarbage, // X-Delivered-To
   AddressMessUnknown, // X-Original-To
   AddressMessUnknown, // X-Rcpt-To
   AddressMessUnknown, // X-Real-To
   AddressMessUnknown, // X-MDMailing-List
   AddressMessUnknown, // Return-Path
   AddressMessUnknown // NULL
};

static String SanitizeWhitelistedAddress(const String &in,size_t header)
{
   String out;

   switch(gs_whitelistHeaderMess[header])
   {
   case AddressMessStandard:
      out = in;
      break;

   case AddressMessUrl:
      out = FilterAddressList(in);
      break;

   case AddressMessGarbage:
   case AddressMessUnknown:
      out = AddressFromFreeStyleHeader(in);
      break;
   }

   return out;
}

static RefCounter<AdbBook> GetWhitelistBook()
{
   AdbManager *manager = AdbManager::Get();

   manager->LoadAll(); // HACK: So that AdbEditor's provider list is utilized

   RefCounter<AdbBook> book(
      manager->CreateBook(READ_APPCONFIG_TEXT(MP_WHITE_LIST)));

   manager->Unget();

   return book;
}

// check whether any address field (sender or recipient) matches whitelist
static bool CheckWhiteList(const Message& msg)
{
   // Go painfully through all headers to debug their format
   bool result = true;

#ifdef DEBUG
#define RETURN(value) result = value
#else
#define RETURN(value) return value
#endif

   RefCounter<AdbBook> book(GetWhitelistBook());

   wxArrayString values = msg.GetHeaderLines(gs_whitelistHeaders);
   for( size_t list = 0; list < values.GetCount(); ++list )
   {
      RefCounter<AddressList> parser(AddressList::Create(
         SanitizeWhitelistedAddress(values[list],list)));

      for( Address *candidate = parser->GetFirst(); candidate;
         candidate = parser->GetNext(candidate) )
      {
         if( !WhiteListDomain(book,candidate->GetDomain()) )
            RETURN(false);
      }
   }

   String id;
   if ( msg.GetHeaderLine(_T("List-Id"),id) )
   {
      if( !WhiteListListId(book,id) )
         RETURN(false);
   }

   return result;
#undef RETURN
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
   len = res_query( wxConvertWX2MB(domain.c_str()), C_IN, T_A,
                    (unsigned char *)answerBuffer, PACKETSZ );

   if( len != -1 )
   {
      if( len > PACKETSZ ) // buffer was too short, re-alloc:
      {
         delete [] answerBuffer;
         answerBuffer = new char [ len ];
         // and again:
         len = res_query( wxConvertWX2MB(domain.c_str()), C_IN, T_A,
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
      if((pos = toExamine.Find(openChar)) == wxNOT_FOUND)
         break;
      ip = toExamine.Mid(pos+1);
      closePos = ip.Find(closeChar);
      if(closePos == wxNOT_FOUND)
         // no second bracket found
         break;
      if(wxSscanf(ip.c_str(), _T("%d.%d.%d.%d"), a,b,c,d) != 4)
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
   header = _T("");
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

bool
HeadersFilter::DoCheckIfSpam(const Message& msg,
                             const String& param,
                             String *result)
{
   wxArrayString tests = strutil_restore_array(param);
   const size_t count = tests.GetCount();
   if ( !count )
   {
      // use [very conservative] defaults
      tests.Add(SPAM_TEST_SPAMASSASSIN);
      tests.Add(SPAM_TEST_RBL);
   }

   String value,
          spamResult;
   for ( size_t n = 0; n < count && spamResult.empty(); n++ )
   {
      const wxString& test = tests[n];
      if ( test == SPAM_TEST_SUBJ8BIT )
      {
         if ( CheckSubjectFor8Bit(msg.Subject()) )
            spamResult = _("8 bit subject");
      }
      else if ( test == SPAM_TEST_SUBJCAPS )
      {
         if ( CheckSubjectForCapitals(msg.Subject()) )
            spamResult = _("only caps in subject");
      }
      else if ( test == SPAM_TEST_SUBJENDJUNK )
      {
         if ( CheckSubjectForJunkAtEnd(msg.Subject()) )
            spamResult = _("junk at end of subject");
      }
      else if ( test == SPAM_TEST_KOREAN )
      {
         // detect all Korean charsets -- and do it for all MIME parts, not
         // just the top level one
         if ( CheckMimePartForKoreanCSet(msg.GetTopMimePart()) )
            spamResult = _("message in Korean");
      }
      else if ( test == SPAM_TEST_SPAMASSASSIN )
      {
         // SpamAssassin adds header "X-Spam-Status: Yes" to all (probably)
         // detected spams
         if ( msg.GetHeaderLine(_T("X-Spam-Status"), value) &&
                  CheckXSpamStatus(value) )
         {
            spamResult = _("tagged by SpamAssassin");
         }
      }
      else if ( test == SPAM_TEST_XAUTHWARN )
      {
         // unfortunately not only spams have this header but we consider that
         // only spammers change their address in such way
         if ( msg.GetHeaderLine(_T("X-Authentication-Warning"), value) &&
                  CheckXAuthWarning(value) )
         {
            spamResult = _("contains X-Authentication-Warning");
         }
      }
      else if ( test == SPAM_TEST_RECEIVED )
      {
         if ( msg.GetHeaderLine(_T("Received"), value) &&
                  CheckReceivedHeaders(value) )
         {
            spamResult = _("suspicious \"Received:\"");
         }
      }
      else if ( test == SPAM_TEST_MIME )
      {
         if ( CheckForSuspiciousMIME(msg) )
            spamResult = _("suspicious MIME structure");
      }
      else if ( test == SPAM_TEST_HTML )
      {
         if ( CheckForHTMLOnly(msg) )
            spamResult = _("pure HTML content");
      }
      else if ( test == SPAM_TEST_WHITE_LIST )
      {
         if ( CheckWhiteList(msg) )
            spamResult = _("not in whitelist");
      }
      else if ( test == SPAM_TEST_EXECUTABLE_ATTACHMENT )
      {
         if ( CheckForExeAttach(msg.GetTopMimePart()) )
            spamResult = _("contains Windows executable attachment");
      }
#ifdef USE_RBL
      else if ( test == SPAM_TEST_RBL )
      {
         msg.GetHeaderLine(_T("Received"), value);

         int a,b,c,d;
         String testHeader = value;

         bool rc = false;
         while ( !rc && !testHeader.empty() )
         {
            if(findIP(testHeader, '(', ')', &a, &b, &c, &d))
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
               if(findIP(testHeader, '[', ']', &a, &b, &c, &d))
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

