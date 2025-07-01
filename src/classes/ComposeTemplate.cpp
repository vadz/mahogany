///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   class/ComposeTemplate.cpp - template parser for composer
// Purpose:     used to initialize composer with the text from template
//              expansion
// Author:      Vadim Zeitlin
// Modified by:
// Created:     13.04.01 (extracted from src/gui/wxComposeView.cpp)
// CVS-ID:      $Id$
// Copyright:   (c) 2001 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
#  include "Mcommon.h"
#  include "sysutil.h"
#  include "strutil.h"
#  include "MApplication.h"
#  include "Profile.h"
#  include "Mdefaults.h"

#  include <wx/frame.h>
#endif // USE_PCH

#include "mail/MimeDecode.h"
#include "TemplateDialog.h"

#include "Composer.h"
#include "gui/wxMDialogs.h"

#include "Address.h"
#include "Message.h"

#include "Mpers.h"

#ifdef USE_PYTHON
   #include "PythonHelp.h"
#endif // USE_PYTHON

#include <wx/confbase.h>      // for wxExpandEnvVars()
#include <wx/file.h>            // for wxFile
#include <wx/ffile.h>
#include <wx/textfile.h>
#include <wx/tokenzr.h>

#include <wx/regex.h>

#include "wx/persctrl.h"

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_WRAP_QUOTED;
extern const MOption MP_COMPOSETEMPLATEPATH_GLOBAL;
extern const MOption MP_COMPOSETEMPLATEPATH_USER;
extern const MOption MP_COMPOSE_SIGNATURE;
extern const MOption MP_COMPOSE_USE_SIGNATURE;
extern const MOption MP_COMPOSE_USE_SIGNATURE_SEPARATOR;
extern const MOption MP_DATE_FMT;
extern const MOption MP_REPLY_MSGPREFIX;
extern const MOption MP_REPLY_MSGPREFIX_FROM_XATTR;
extern const MOption MP_REPLY_MSGPREFIX_FROM_SENDER;
extern const MOption MP_REPLY_QUOTE_EMPTY;
extern const MOption MP_WRAPMARGIN;

// ----------------------------------------------------------------------------
// persistent msgboxes we use here
// ----------------------------------------------------------------------------

extern const MPersMsgBox *M_MSGBOX_SIGNATURE_LENGTH;
extern const MPersMsgBox *M_MSGBOX_ASK_FOR_SIG;

// ----------------------------------------------------------------------------
// the classes which are used together with compose view - we have here a
// derivation of variable expander and expansion sink which are used for parsing
// the templates, but are completely hidden from the outside world because we
// provide just a single public function, ExpandTemplate()
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// this struct and array are used by ExpansionSink only - they contain the
// information about the attachments
// ----------------------------------------------------------------------------

struct AttachmentInfo
{
   AttachmentInfo(void *d,
                  size_t l,
                  const String& m,
                  const String& f) : mimetype(m), filename(f)
      { data = d; len = l; }

   void   *data;
   size_t  len;
   String  mimetype,
           filename;
};

WX_DECLARE_OBJARRAY(AttachmentInfo, ArrayAttachmentInfo);

// ----------------------------------------------------------------------------
// implementation of template sink which stores the data internally and then
// puts it into the composer when InsertTextInto() is called
// ----------------------------------------------------------------------------

class ExpansionSink : public MessageTemplateSink
{
public:
   // ctor
   ExpansionSink() { m_hasCursorPosition = FALSE; m_x = m_y = 0; }

   // called after successful parsing of the template to insert the resulting
   // text into the compose view
   void InsertTextInto(Composer& cv) const;

   // implement base class pure virtual function
   virtual bool Output(const String& text);

   // TODO the functions below should be in the base class (as pure virtuals)
   //      somehow, not here

   // called by VarExpander to remember the current position as the initial
   // cursor position
   void RememberCursorPosition() { m_hasCursorPosition = TRUE; }

   // called by VarExpander to insert an attachment
   void InsertAttachment(void *data, size_t len,
                         const String& mimetype,
                         const String& filename);

private:
   // as soon as m_hasCursorPosition becomes TRUE we stop to update the current
   // cursor position which we normally keep track of in m_x and m_y - so that
   // we'll have the position of the cursor when RememberCursorPosition() in
   // them at the end
   bool m_hasCursorPosition;
   int  m_x, m_y;

   // before each m_attachments[n] there is text from m_texts[n] - and after
   // the last attachment there is m_text
   wxString m_text;
   wxArrayString m_texts;
   ArrayAttachmentInfo m_attachments;
};

// ----------------------------------------------------------------------------
// VarExpander is the implementation of MessageTemplateVarExpander used here
// ----------------------------------------------------------------------------

class VarExpander : public MessageTemplateVarExpander
{
public:
   // all categories
   enum Category
   {
      Category_Misc,       // empty category - misc variables
      Category_File,       // insert file
      Category_Attach,     // attach file
      Category_Command,    // execute external command
#ifdef USE_PYTHON
      Category_Python,     // execute Python script
#endif // USE_PYTHON
      Category_Message,    // access the headers of the message being written
      Category_Original,   // variables pertaining to the original message
      Category_Headers,    // set the headers of the message being written
      Category_Invalid,    // unknown/invalid category
      Category_Max = Category_Invalid
   };

   // the variables in "misc" category
   enum Variable
   {
      MiscVar_Date,        // insert the date in the default format
      MiscVar_Cursor,      // position the cursor here after template expansion
      MiscVar_To,          // the recipient name
      MiscVar_Cc,          // the copied-to recipient name
      MiscVar_Subject,     // the message subject (without any "Re"s)

      // all entries from here only apply to the reply/forward/followup
      // templates because they work with the original message
      MiscVar_Quote,       // quote the original text
      MiscVar_Quote822,    // include the original msg as a RFC822 attachment
      MiscVar_Text,        // include the original text as is
      MiscVar_Sender,      // the fullname of the sender
      MiscVar_Signature,   // the signature

      MiscVar_Invalid,
      MiscVar_Max = MiscVar_Invalid
   };

   // the variables in "message" category
   //
   // NB: the values should be identical to RecipientType enum
   //     (or the code in ExpandMessage should be changed)
   enum MessageHeader
   {
      MessageHeader_To,
      MessageHeader_Cc,
      MessageHeader_Bcc,
      MessageHeader_LastControl = MessageHeader_Bcc,
      MessageHeader_Subject,
      MessageHeader_FirstName,
      MessageHeader_LastName,
      MessageHeader_Invalid,
      MessageHeader_Max = MessageHeader_Invalid
   };

   // the variables from "original" category which map to headers
   enum OriginalHeader
   {
      OriginalHeader_Date,
      OriginalHeader_From,
      OriginalHeader_Subject,
      OriginalHeader_PersonalName,
      OriginalHeader_FirstName,
      OriginalHeader_LastName,
      OriginalHeader_To,
      OriginalHeader_Cc,
      OriginalHeader_ReplyTo,
      OriginalHeader_Newsgroups,
      OriginalHeader_Domain,
      OriginalHeader_All,
      OriginalHeader_Invalid,
      OriginalHeader_Max = OriginalHeader_Invalid
   };

   // get the category from the category string (may return Category_Invalid)
   static Category GetCategory(const String& category)
   {
      return (Category)FindStringInArray(ms_templateVarCategories,
                                         Category_Max, category);
   }

   // get the variable id from the string (works for misc variables only, will
   // return MiscVar_Invalid if variable is unknown)
   static Variable GetVariable(const String& variable)
   {
      return (Variable)FindStringInArray(ms_templateMiscVars,
                                         MiscVar_Max, variable);
   }

   // get the header corresponding to the variable of "message" category
   static MessageHeader GetMessageHeader(const String& variable)
   {
      return (MessageHeader)FindStringInArray(ms_templateMessageVars,
                                              MessageHeader_Max, variable);
   }

   // get the header corresponding to the variable of "original" category
   // (will return OriginalHeader_Invalid if there is no corresponding header -
   // note that this doesn't mean that the variable is invalid because, for
   // example, "quote" doesn't correspond to any header, yet $(original:quote)
   // is perfectly valid)
   static OriginalHeader GetOriginalHeader(const String& variable)
   {
      return (OriginalHeader)FindStringInArray(ms_templateOriginalVars,
                                               OriginalHeader_Max, variable);
   }

   // ctor takes the sink (we need it to implement some pseudo macros such as
   // "$CURSOR") and also a pointer to message for things like $QUOTE - this
   // may (and in fact should) be NULL for new messages, in this case using the
   // macros which require it will result in an error.
   //
   // And we also need the compose view to expand the macros in the "message"
   // category.
   VarExpander(ExpansionSink& sink,
               Composer& cv,
               Profile *profile,
               Message *msg,
               const String& textToQuote)
      : m_sink(sink), m_cv(cv), m_textToQuote(textToQuote)
   {
      m_profile = profile ? profile : mApplication->GetProfile();
      m_profile->IncRef();

      m_msg = msg;
      SafeIncRef(m_msg);
   }

   virtual ~VarExpander()
   {
      m_profile->DecRef();
      SafeDecRef(m_msg);
   }

   // implement base class pure virtual function
   virtual bool Expand(const String& category,
                       const String& name,
                       const wxArrayString& arguments,
                       String *value) const;

protected:
   // read the file into the string, return TRUE on success
   static bool SlurpFile(const String& filename, String *value);

   // try to find the filename the user wants if only the name was specified
   // (look in the standard locations...)
   static String GetAbsFilename(const String& name);

   // Expand determines the category and then calls one of these functions
   bool ExpandMisc(const String& name,
                   const wxArrayString& arguments,
                   String *value) const;
   bool ExpandFile(const String& name,
                   const wxArrayString& arguments,
                   String *value) const;
   bool ExpandAttach(const String& name,
                     const wxArrayString& arguments,
                     String *value) const;
   bool ExpandCommand(const String& name,
                      const wxArrayString& arguments,
                      String *value) const;
#ifdef USE_PYTHON
   bool ExpandPython(const String& name,
                     const wxArrayString& arguments,
                     String *value) const;
#endif // USE_PYTHON
   bool ExpandMessage(const String& name, String *value) const;
   bool ExpandOriginal(const String& name, String *value) const;

   bool SetHeaderValue(const String& name,
                       const wxArrayString& arguments,
                       String *value) const;

   // handle quoting the original message
   void DoQuoteOriginal(bool isQuote, String *value) const;

   // get the signature to use (including the signature separator, if any)
   String GetSignature() const;

private:
   // helper used by GetCategory and GetVariable
   static int FindStringInArray(const wxChar *strs[], int max, const String& s);

   // the sink we use when expanding pseudo variables
   ExpansionSink& m_sink;

   // the compose view is used for expansion of the variables in "message"
   // category
   Composer& m_cv;

   // the message used for expansion of variables pertaining to the original
   // message (may be NULL for new messages)
   Message *m_msg;

   // the text to be quoted in a reply/followup
   String m_textToQuote;

   // the profile to use for everything (global one by default)
   Profile *m_profile;

   // this array contains the list of all categories
   static const wxChar *ms_templateVarCategories[Category_Max];

   // this array contains the list of all variables without category
   static const wxChar *ms_templateMiscVars[MiscVar_Max];

   // this array contains the variable names from "message" category
   static const wxChar *ms_templateMessageVars[MessageHeader_Max];

   // this array contains all the variables in the "original" category which
   // map to the headers of the original message (there are other variables in
   // this category as well)
   static const wxChar *ms_templateOriginalVars[OriginalHeader_Max];

   DECLARE_NO_COPY_CLASS(VarExpander)
};

// ----------------------------------------------------------------------------
// global data: the definitions of the popum menu for the template editing
// dialog.
// ----------------------------------------------------------------------------

// Leave this line here, it's a cut-&-paste buffer
// Available accels: ABCDEFGHIJKLMNOPQRSTUVWXYZ

// NB: These menus should be kept in sync (if possible) with the variable names

// the misc submenu
//
// Available accels: BEFGHIJKLMNOPRTUVWXYZ
static TemplatePopupMenuItem gs_popupSubmenuMisc[] =
{
   TemplatePopupMenuItem(gettext_noop("Put &cursor here"), _T("$cursor")),
   TemplatePopupMenuItem(gettext_noop("Insert &signature"), _T("$signature")),
   TemplatePopupMenuItem(),
   TemplatePopupMenuItem(gettext_noop("Insert current &date"), _T("$date")),
   TemplatePopupMenuItem(),
   TemplatePopupMenuItem(gettext_noop("Insert &quoted text"), _T("$quote")),
   TemplatePopupMenuItem(gettext_noop("&Attach original text"), _T("$quote822")),
};

// the file insert/attach sub menu
//
// Available accels: BCDEFGHJKLMNOPRSUVWXYZ
static TemplatePopupMenuItem gs_popupSubmenuFile[] =
{
   TemplatePopupMenuItem(gettext_noop("&Insert file..."), _T("${file:%s}"), TRUE),
   TemplatePopupMenuItem(gettext_noop("Insert &any file..."), _T("${file:%s?ask"), TRUE),
   TemplatePopupMenuItem(gettext_noop("Insert &quoted file..."), _T("${file:%s?quote}"), TRUE),
   TemplatePopupMenuItem(gettext_noop("A&ttach file..."), _T("${attach:%s}"), TRUE),
};

// the message submenu
//
// Available accels: ADEGHIJKMNOPQRUVWXYZ
static TemplatePopupMenuItem gs_popupSubmenuMessage[] =
{
   TemplatePopupMenuItem(gettext_noop("&To"), _T("${message:to}")),
   TemplatePopupMenuItem(gettext_noop("&First name"), _T("${message:firstname}")),
   TemplatePopupMenuItem(gettext_noop("&Last name"), _T("${message:lastname}")),
   TemplatePopupMenuItem(gettext_noop("&Subject"), _T("${message:subject}")),
   TemplatePopupMenuItem(gettext_noop("&CC"), _T("${message:cc}")),
   TemplatePopupMenuItem(gettext_noop("&BCC"), _T("${message:bcc}")),
};

// the original message submenu
//
// Available accels: BCEHJKMOPVWXYZ
static TemplatePopupMenuItem gs_popupSubmenuOriginal[] =
{
   TemplatePopupMenuItem(gettext_noop("&Date"), _T("${original:date}")),
   TemplatePopupMenuItem(gettext_noop("&From"), _T("${original:from}")),
   TemplatePopupMenuItem(gettext_noop("&Subject"), _T("${original:subject}")),
   TemplatePopupMenuItem(gettext_noop("Full &name"), _T("${original:fullname}")),
   TemplatePopupMenuItem(gettext_noop("F&irst name"), _T("${original:firstname}")),
   TemplatePopupMenuItem(gettext_noop("&Last name"), _T("${original:lastname}")),
   TemplatePopupMenuItem(gettext_noop("&To"), _T("${original:to}")),
   TemplatePopupMenuItem(gettext_noop("&Reply to"), _T("${original:replyto}")),
   TemplatePopupMenuItem(gettext_noop("News&groups"), _T("${original:newsgroups}")),
   TemplatePopupMenuItem(gettext_noop("Entire &header"), _T("${original:header}")),
   TemplatePopupMenuItem(),
   TemplatePopupMenuItem(gettext_noop("Insert &quoted text"), _T("$quote")),
   TemplatePopupMenuItem(gettext_noop("&Attach original text"), _T("$quote822")),
   TemplatePopupMenuItem(gettext_noop("Insert &unquoted Text"), _T("$text")),
};

// the whole menu
//
// Available accels: ABCDEGIJKLNQRSTUVWYZ
static TemplatePopupMenuItem gs_popupMenu[] =
{
   TemplatePopupMenuItem(gettext_noop("&Miscellaneous"),
                         gs_popupSubmenuMisc,
                         WXSIZEOF(gs_popupSubmenuMisc)),
   TemplatePopupMenuItem(gettext_noop("Message &headers"),
                         gs_popupSubmenuMessage,
                         WXSIZEOF(gs_popupSubmenuMessage)),
   TemplatePopupMenuItem(gettext_noop("&Original message"),
                         gs_popupSubmenuOriginal,
                         WXSIZEOF(gs_popupSubmenuOriginal)),
   TemplatePopupMenuItem(gettext_noop("Insert or attach a &file"),
                         gs_popupSubmenuFile,
                         WXSIZEOF(gs_popupSubmenuFile)),
   TemplatePopupMenuItem(),
#ifdef USE_PYTHON
   TemplatePopupMenuItem(gettext_noop("Run &Python function..."), _T("${python:%s}"), FALSE),
#endif // USE_PYTHON
   TemplatePopupMenuItem(gettext_noop("E&xecute command..."), _T("${cmd:%s}"), FALSE),
};

const TemplatePopupMenuItem& g_ComposeViewTemplatePopupMenu =
   TemplatePopupMenuItem(wxEmptyString, gs_popupMenu, WXSIZEOF(gs_popupMenu));

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// helper functions
// ----------------------------------------------------------------------------

// return the personal name part of the first address of the given type
//
// FIXME: this function is bad as it completely ignores the other addresses
static String GetNameForAddress(Message *msg, MessageAddressType type)
{
   String value;

   AddressList_obj addrList(msg->GetAddressList(type));
   if ( addrList )
   {
      Address *addr = addrList->GetFirst();
      if ( addr )
         value = addr->GetName();
   }

   return value;
}

// extract first or last name from the given string
//
// the logic here is primitive: the last word is taken to be the last name and
// anything preceding it is the first name
static String ExtractFirstOrLastName(const String& fullname, bool first)
{
   String value;

   AddressList_obj addrList(fullname);
   for ( Address *addr = addrList->GetFirst();
         addr;
         addr = addrList->GetNext(addr) )
   {
      if ( !value.empty() )
         value += _T(", ");

      const String name = addr->GetName();
      const size_t posSpace = name.find_last_of(" ");
      if ( posSpace == String::npos )
      {
         // if there are no spaces we have no choice but to take the
         // entire name, whether we need the first or last name
         value += name;
      }
      else // split the name in first and last parts
      {
         if ( first )
         {
            // first name is everything before the last word
            value += name.substr(0, posSpace);
         }
         else // extract the last name
         {
            // last name is just the last word
            value += name.substr(posSpace + 1);
         }
      }
   }

   return value;
}

static inline String ExtractFirstName(const String& fullname)
{
   return ExtractFirstOrLastName(fullname, true);
}

static inline String ExtractLastName(const String& fullname)
{
   return ExtractFirstOrLastName(fullname, false);
}

// return the reply prefix to use for message quoting when replying
static String GetReplyPrefix(Message *msg, Profile *profile)
{
   String prefix;

   // X-Attribution header value overrides everything else if it exists
   if ( msg && READ_CONFIG_BOOL(profile, MP_REPLY_MSGPREFIX_FROM_XATTR) )
   {
      msg->GetDecodedHeaderLine(_T("X-Attribution"), prefix);
   }

   // prepend the senders initials to the reply prefix (this
   // will make reply prefix like "VZ>")
   if ( prefix.empty() &&
            READ_CONFIG(profile, MP_REPLY_MSGPREFIX_FROM_SENDER) )
   {
      // take from address, not reply-to which can be set to
      // reply to a mailing list, for example
      String name = GetNameForAddress(msg, MAT_FROM);
      if ( name.empty() )
      {
         // no from address? try to find anything else
         name = GetNameForAddress(msg, MAT_REPLYTO);
      }

      name = MIME::DecodeHeader(name);

      // it's (quite) common to have quotes around the personal
      // part of the address, remove them if so

      // remove spaces
      name.Trim(TRUE);
      name.Trim(FALSE);
      if ( !name.empty() )
      {
         if ( name[0u] == '"' && name.Last() == '"' )
         {
            name = name.Mid(1, name.length() - 2);
         }

         // take the first letter of each word (include single quote in
         // delimiters so that names like "Foo O'Bar" and "Baz d'ABC" are
         // handled correctly and use wxTOKEN_STRTOK to avoid empty tokens)
         wxStringTokenizer tk(name, " '-", wxTOKEN_STRTOK);
         while ( tk.HasMoreTokens() )
         {
            const wxString& token = tk.GetNextToken();
            if ( token == "via" )
            {
               // Many mailing lists rewrite "From:" header to be "Original
               // Sender via Mailing List <mailing@list.address>" to avoid
               // problems with DKIM, don't include the nonsensical "vML"
               // string into our attribution here.
               if ( !prefix.empty() )
                  break;

               // Unless we have nothing in it yet, in this case "via" might
               // have been a false positive?
            }

            wxChar chInitial = token[0u];

            if ( chInitial == '<' )
            {
               // this must be the start of embedded "<...>"
               // address, skip it completely
               break;
            }

            // only take letters as initials
            if ( wxIsalpha(chInitial) )
            {
               prefix += chInitial;
            }
         }
      }
   }

   // and then the standard reply prefix too
   prefix += READ_CONFIG(profile, MP_REPLY_MSGPREFIX);

   return prefix;
}

// put the text quoted according to our current quoting options with the given
// reply prefix into value
static String
ExpandOriginalText(const String& text,
                   const String& prefix,
                   Profile *profile)
{
   String value;

   // should we quote the empty lines?
   //
   // this option is ignored when we're inserting text verbatim (hence without
   // reply prefix) and not quoting it
   bool quoteEmpty = !prefix.empty() &&
                        READ_CONFIG(profile, MP_REPLY_QUOTE_EMPTY);

   // where to break lines (if at all)?
   size_t wrapMargin;
   if ( READ_CONFIG(profile, MP_WRAP_QUOTED) )
   {
      wrapMargin = READ_CONFIG(profile, MP_WRAPMARGIN);
      if ( wrapMargin <= prefix.length() )
      {
         wxLogError(_("The configured automatic wrap margin (%u) is too "
                      "small, please increase it.\n"
                      "\n"
                      "Disabling automatic wrapping for now."), wrapMargin);

         profile->writeEntry(MP_WRAP_QUOTED, false);
         wrapMargin = 0;
      }
   }
   else
   {
      // don't wrap
      wrapMargin = 0;
   }

   // if != 0, then we're at the end of the current line
   size_t lenEOL = 0;

   // the current line
   String lineCur;

   for ( const wxChar *cptr = text.c_str(); ; cptr++ )
   {
      // start of [real] new line?
      if ( lineCur.empty() )
      {
         if ( !quoteEmpty && (lenEOL = IsEndOfLine(cptr)) != 0 )
         {
            // this line is empty, skip it entirely (i.e. don't output the
            // prefix for it)
            cptr += lenEOL - 1;

            value += '\n';

            continue;
         }

         lineCur += prefix;
      }

      if ( !*cptr || (lenEOL = IsEndOfLine(cptr)) != 0 )
      {
         // sanity test
         ASSERT_MSG( !wrapMargin || lineCur.length() <= wrapMargin,
                     _T("logic error in auto wrap code") );

         value += lineCur;

         if ( !*cptr )
         {
            // end of text
            break;
         }

         // put just '\n' in output, we don't need "\r\n"
         value += '\n';

         lineCur.clear();

         // -1 to compensate for ++ in the loop
         cptr += lenEOL - 1;
      }
      else // !EOL
      {
         lineCur += *cptr;

         // we don't need to wrap a line if it is its last character anyhow
         if ( wrapMargin && lineCur.length() >= wrapMargin
               && !IsEndOfLine(cptr + 1) )
         {
            // break the line before the last word
            size_t n = wrapMargin - 1;
            while ( n > prefix.length() )
            {
               if ( wxIsspace(lineCur[n]) )
                  break;

               n--;
            }

            if ( n == prefix.length() )
            {
               // no space found in the line or it is in prefix which
               // we don't want to wrap - so just cut the line right here
               n = wrapMargin;
            }

            value.append(lineCur, 0, n);
            value += '\n';

            // we don't need to start the new line with spaces so remove them
            // from the tail
            while ( n < lineCur.length() && wxIsspace(lineCur[n]) )
            {
               n++;
            }

            lineCur.erase(0, n);
            lineCur.Prepend(prefix);
         }
      }
   }

   return value;
}


// ----------------------------------------------------------------------------
// ExpansionSink - the sink used with wxComposeView
// ----------------------------------------------------------------------------

#include <wx/arrimpl.cpp>
WX_DEFINE_OBJARRAY(ArrayAttachmentInfo);

bool
ExpansionSink::Output(const String& text)
{
   if ( !m_hasCursorPosition )
   {
      // update the current x and y position
      //
      // TODO: this supposes that there is no autowrap, to be changed if/when
      //       it appears
      int deltaX = 0, deltaY = 0;
      for ( const wxChar *pc = text.c_str(); *pc; pc++ )
      {
         if ( *pc == '\n' )
         {
            deltaX = -m_x;
            deltaY++;
         }
         else
         {
            deltaX++;
         }
      }

      m_x += deltaX;
      m_y += deltaY;
   }
   //else: we don't have to count anything, we already have the cursor position
   //      and this is all we want

   // we always add this text to the last "component" of text (i.e. the one
   // after the last attachment)
   m_text += text;

   return TRUE;
}

void
ExpansionSink::InsertAttachment(void *data,
                                size_t len,
                                const String& mimetype,
                                const String& filename)
{
   if ( !m_hasCursorPosition )
   {
      // an attachment count as one cursor position
      m_x++;
   }

   // create a new attachment info object (NB: it will be deleted by the array
   // automatically because we pass it by pointer and not by reference)
   m_attachments.Add(new AttachmentInfo(data, len, mimetype, filename));

   // the last component of text becomes the text before this attachment
   m_texts.Add(m_text);

   // and the new last component is empty
   m_text.Empty();
}

void
ExpansionSink::InsertTextInto(Composer& cv) const
{
   size_t nCount = m_texts.GetCount();
   ASSERT_MSG( m_attachments.GetCount() == nCount,
               _T("something is very wrong in template expansion sink") );

   for ( size_t n = 0; n < nCount; n++ )
   {
      cv.InsertText(m_texts[n]);

      AttachmentInfo& attInfo = m_attachments[n];
      cv.InsertData(attInfo.data, attInfo.len,
                    attInfo.mimetype,
                    attInfo.filename);
   }

   cv.InsertText(m_text);

   // position the cursor - if RememberCursorPosition() hadn't been called, it
   // will be put in (0, 0)
   cv.MoveCursorBy(m_x, m_y);

   // as the inserted text comes from the program, not from the user, don't
   // mark the composer contents as dirty
   cv.ResetDirty();
}

// ----------------------------------------------------------------------------
// VarExpander - used by wxComposeView
// ----------------------------------------------------------------------------

const wxChar *VarExpander::ms_templateVarCategories[] =
{
   wxEmptyString,
   _T("file"),
   _T("attach"),
   _T("cmd"),
#ifdef USE_PYTHON
   _T("python"),
#endif // USE_PYTHON
   _T("message"),
   _T("original"),
   _T("header"),
};

const wxChar *VarExpander::ms_templateMiscVars[] =
{
   _T("date"),
   _T("cursor"),
   _T("to"),
   _T("cc"),
   _T("subject"),
   _T("quote"),
   _T("quote822"),
   _T("text"),
   _T("sender"),
   _T("signature"),
};

const wxChar *VarExpander::ms_templateMessageVars[] =
{
   _T("to"),
   _T("cc"),
   _T("bcc"),
   _T("subject"),
   _T("firstname"),
   _T("lastname"),
};

const wxChar *VarExpander::ms_templateOriginalVars[] =
{
   _T("date"),
   _T("from"),
   _T("subject"),
   _T("fullname"),
   _T("firstname"),
   _T("lastname"),
   _T("to"),
   _T("cc"),
   _T("replyto"),
   _T("newsgroups"),
   _T("domain"),
   _T("header"),
};

int
VarExpander::FindStringInArray(const wxChar *strings[],
                               int max,
                               const String& s)
{
   int n;
   for ( n = 0; n < max; n++ )
   {
      if ( strings[n] == s )
         break;
   }

   return n;
}

bool
VarExpander::SlurpFile(const String& filename, String *value)
{
   // it's important that value is not empty even if we return FALSE because if
   // it's empty when Expand() returns the template parser will log a
   // misleading error message about "unknown variable"
   *value = _T('?');

   wxFFile file(filename);

   return file.IsOpened() && file.ReadAll(value);
}

String
VarExpander::GetAbsFilename(const String& name)
{
   String filename = wxExpandEnvVars(name);
   if ( !wxIsAbsolutePath(filename) )
   {
      Profile *profile = mApplication->GetProfile();
      String path = READ_CONFIG(profile, MP_COMPOSETEMPLATEPATH_USER);
      if ( path.empty() )
         path = mApplication->GetLocalDir();
      if ( !path.empty() || path.Last() != '/' )
      {
         path += '/';
      }
      String filename2 = path + filename;

      if ( wxFile::Exists(filename2) )
      {
         // ok, found
         filename = filename2;
      }
      else
      {
         // try the global dir
         String pathGlobal = READ_CONFIG(profile, MP_COMPOSETEMPLATEPATH_GLOBAL);
         if ( pathGlobal.empty() )
            pathGlobal= mApplication->GetDataDir();
         if ( !pathGlobal.empty() || pathGlobal.Last() != '/' )
         {
            pathGlobal+= '/';
         }

         filename.Prepend(pathGlobal);
      }
   }
   //else: absolute filename given, don't look anywhere else

   return filename;
}

bool
VarExpander::Expand(const String& category,
                    const String& name,
                    const wxArrayString& arguments,
                    String *value) const
{
   value->Empty();

   // comparison is case insensitive
   switch ( GetCategory(category.Lower()) )
   {
      case Category_File:
         return ExpandFile(name, arguments, value);

      case Category_Attach:
         return ExpandAttach(name, arguments, value);

      case Category_Command:
         return ExpandCommand(name, arguments, value);

#ifdef USE_PYTHON
      case Category_Python:
         return ExpandPython(name, arguments, value);
#endif // USE_PYTHON

      case Category_Message:
         return ExpandMessage(name, value);

      case Category_Original:
         return ExpandOriginal(name, value);

      case Category_Misc:
         return ExpandMisc(name, arguments, value);

      case Category_Headers:
         return SetHeaderValue(name, arguments, value);

      default:
         // unknown category
         return FALSE;
   }
}

bool
VarExpander::ExpandMisc(const String& name,
                        const wxArrayString& /* arguments */,
                        String *value) const
{
   // deal with all special cases
   switch ( GetVariable(name.Lower()) )
   {
      case MiscVar_Date:
         {
            time_t ltime;
            (void)time(&ltime);

#ifdef OS_WIN
            // MP_DATE_FMT contains '%' which are being (mis)interpreted as
            // env var expansion characters under Windows
            ProfileEnvVarSave noEnvVars(m_profile);
#endif // OS_WIN

            *value = strutil_ftime(ltime, READ_CONFIG(m_profile, MP_DATE_FMT));
         }
         break;

      case MiscVar_Cursor:
         m_sink.RememberCursorPosition();
         break;

         // some shortcuts for the values of the "original:" category
      case MiscVar_To:
      case MiscVar_Cc:
      case MiscVar_Subject:
      case MiscVar_Quote:
      case MiscVar_Quote822:
      case MiscVar_Text:
         return ExpandOriginal(name, value);

      case MiscVar_Sender:
         return ExpandOriginal(_T("from"), value);

      case MiscVar_Signature:
         *value = GetSignature();
         break;

      default:
         // unknown name
         return FALSE;
   }

   return TRUE;
}

bool
VarExpander::ExpandFile(const String& name,
                        const wxArrayString& arguments,
                        String *value) const
{
   // first check if we don't want to ask user
   String filename = GetAbsFilename(name);
   if ( arguments.Index(_T("ask"), FALSE /* no case */) != wxNOT_FOUND )
   {
      filename = MDialog_FileRequester(_("Select the file to insert"),
                                       m_cv.GetFrame(),
                                       filename);
   }

   if ( !!filename )
   {
      // insert the contents of a file
      if ( !SlurpFile(filename, value) )
      {
         wxLogError(_("Failed to insert file '%s' into the message."),
                    name);

         return FALSE;
      }

      // do we want to quote the files contents before inserting?
      if ( arguments.Index(_T("quote"), FALSE /* no case */) != wxNOT_FOUND )
      {
         String prefix = READ_CONFIG(m_profile, MP_REPLY_MSGPREFIX);
         String quotedValue;
         quotedValue.Alloc(value->length());

         const wxChar *cptr = value->c_str();
         quotedValue = prefix;
         while ( *cptr )
         {
            if ( *cptr == '\r' )
            {
               cptr++;
               continue;
            }

            quotedValue += *cptr;
            if( *cptr++ == '\n' && *cptr )
            {
               quotedValue += prefix;
            }
         }

         *value = quotedValue;
      }
   }
   //else: no file, nothing to insert

   return TRUE;
}

bool
VarExpander::ExpandAttach(const String& name,
                          const wxArrayString& arguments,
                          String *value) const
{
   String filename = GetAbsFilename(name);
   if ( arguments.Index(_T("ask"), FALSE /* no case */) != wxNOT_FOUND )
   {
      filename = MDialog_FileRequester(_("Select the file to attach"),
                                       m_cv.GetFrame(),
                                       filename);
   }

   if ( !!filename )
   {
      if ( !SlurpFile(filename, value) )
      {
         wxLogError(_("Failed to attach file '%s' to the message."),
                    name);

         return FALSE;
      }

      // guess MIME type from extension
      m_sink.InsertAttachment(wxStrdup(value->c_str()),
                              value->length(),
                              wxEmptyString, // will be determined from filename laer
                              filename);

      // avoid inserting file as text additionally
      value->Empty();
   }
   //else: no file, nothing to attach

   return TRUE;
}

bool
VarExpander::ExpandCommand(const String& name,
                           const wxArrayString& arguments,
                           String *value) const
{
   // execute a command
   MTempFileName temp;

   bool ok = temp.IsOk();
   wxString filename = temp.GetName();

   if ( ok )
   {
      wxString command = name;

      // although the arguments may be included directly in the template,
      // passing them via "?" argument mechanism allows to calculate them
      // during run-time, i.e. it makes it possible to call an external command
      // using the result of application of another template
      size_t count = arguments.GetCount();
      for ( size_t n = 0; n < count; n++ )
      {
         // forbid further expansion in the arguments by quoting them
         wxString arg = arguments[n];
         arg.Replace(_T("'"), _T("\\'"));

         command << _T(" '") << arg << '\'';
      }

      command << _T(" > ") << filename;

      ok = wxSystem(command) == 0;
   }

   if ( ok )
      ok = SlurpFile(filename, value);

   if ( !ok )
   {
      wxLogSysError(_("Failed to execute the command '%s'"), name);

      // make sure the value isn't empty to avoid message about unknown
      // variable from the parser
      *value = _T('?');

      return FALSE;
   }

   return TRUE;
}

bool
VarExpander::SetHeaderValue(const String& name,
                            const wxArrayString& arguments,
                            String *value) const
{
   if ( arguments.GetCount() != 1 )
   {
      wxLogError(_("${header:%s} requires exactly one argument."),
                 name);

      *value = _T('?');

      return FALSE;
   }

   String headerValue = arguments[0];

   // is it one of the standard headers or some other one?
   String headerName = name.Lower();
   if ( headerName == _T("subject") )
      m_cv.SetSubject(headerValue);
   else if ( headerName == _T("from") )
      m_cv.SetFrom(headerValue);
   else if ( headerName == _T("to") )
      m_cv.AddTo(headerValue);
   else if ( headerName == _T("cc") )
      m_cv.AddCc(headerValue);
   else if ( headerName == _T("bcc") )
      m_cv.AddBcc(headerValue);
   else if ( headerName == _T("fcc") )
      m_cv.AddFcc(headerValue);
   else // some other header
      m_cv.AddHeaderEntry(headerName, headerValue);

   return TRUE;
}

#ifdef USE_PYTHON

bool
VarExpander::ExpandPython(const String& name,
                          const wxArrayString& arguments,
                          String *value) const
{
   // call Python function with the given name
   if ( !PythonStringFunction(name, arguments, value) )
   {
      return FALSE;
   }

   return TRUE;
}

#endif // USE_PYTHON

bool
VarExpander::ExpandMessage(const String& name, String *value) const
{
   MessageHeader header = GetMessageHeader(name.Lower());
   if ( header == MessageHeader_Invalid )
   {
      // unknown variable
      return FALSE;
   }

   switch ( header )
   {
      case MessageHeader_Subject:
         *value = m_cv.GetSubject();
         break;

      case MessageHeader_FirstName:
      case MessageHeader_LastName:
         *value = ExtractFirstOrLastName
                  (
                     m_cv.GetRecipients(Recipient_To),
                     header == MessageHeader_FirstName
                  );
         break;

      default:
         CHECK( header <= MessageHeader_LastControl, FALSE,
                _T("unexpected macro in message category") );

         // the MessageHeader enum values are the same as RecipientType ones,
         // so no translation is needed
         *value = m_cv.GetRecipients((RecipientType)header);
   }

   return TRUE;
}

bool
VarExpander::ExpandOriginal(const String& Name, String *value) const
{
   // insert the quoted text of the original message - of course, this
   // only works if we have this original message
   if ( !m_msg )
   {
      // unfortunately we don't have the real name of the variable
      // here
      wxLogWarning(_("The variables using the original message cannot "
                     "be used in this template; variable ignored."));
   }
   else
   {
      // headers need to be decoded before showing them to the user
      bool isHeader = true;

      String name = Name.Lower();
      switch ( GetOriginalHeader(name) )
      {
         case OriginalHeader_Date:
            *value = m_msg->Date();
            break;

         case OriginalHeader_From:
            *value = m_msg->From();
            break;

         case OriginalHeader_Subject:
            *value = m_msg->Subject();
            break;

         case OriginalHeader_ReplyTo:
            *value = GetNameForAddress(m_msg, MAT_REPLYTO);
            break;

         case OriginalHeader_To:
            *value = m_msg->GetAddressesString(MAT_TO);
            break;

         case OriginalHeader_Cc:
            *value = m_msg->GetAddressesString(MAT_CC);
            break;

         case OriginalHeader_PersonalName:
            *value = GetNameForAddress(m_msg, MAT_FROM);
            break;

         case OriginalHeader_FirstName:
            *value = ExtractFirstName(GetNameForAddress(m_msg, MAT_FROM));
            break;

         case OriginalHeader_LastName:
            *value = ExtractLastName(GetNameForAddress(m_msg, MAT_FROM));
            break;

         case OriginalHeader_Newsgroups:
            m_msg->GetHeaderLine(_T("Newsgroups"), *value);
            break;

         case OriginalHeader_Domain:
            {
               AddressList_obj addrList(m_msg->From());
               Address *addr = addrList->GetFirst();
               if ( addr )
               {
                  *value = addr->GetDomain();
               }
            }
            break;

         case OriginalHeader_All:
            *value = m_msg->GetHeader();
            break;

         default:
            isHeader = false;

            // it isn't a variable which maps directly onto header, check the
            // others
            bool isQuote = name == _T("quote");
            if ( isQuote || name == _T("text") )
            {
               DoQuoteOriginal(isQuote, value);
            }
            else if ( name == _T("quote822") )
            {
               // insert the original message as RFC822 attachment
               String str;
               m_msg->WriteToString(str);
               m_sink.InsertAttachment(wxStrdup(str), str.Length(),
                                       _T("message/rfc822"), wxEmptyString);
            }
            else
            {
               return FALSE;
            }
      }

      if ( isHeader )
      {
         // we show the decoded headers to the user and then encode them back
         // when sending the messages
         //
         // FIXME: of course, this means that we lose the additional encoding
         //        info
         *value = MIME::DecodeHeader(*value);
      }
   }

   return TRUE;
}

// ----------------------------------------------------------------------------
// ExpandMisc("signature") helper
// ----------------------------------------------------------------------------

String VarExpander::GetSignature() const
{
   String signature;

   // first check if we want to insert it at all: this setting overrides the
   // $signature in the template because the latter is there by default and
   // it's simpler (especially for a novice user who might not know about the
   // templates at all) to just uncheck the checkbox "Use signature" in the
   // options dialog instead of editing all templates
   if ( READ_CONFIG(m_profile, MP_COMPOSE_USE_SIGNATURE) )
   {
      wxTextFile fileSig;

      // loop until we have a valid file to read the signature from
      bool hasSign = false;
      while ( !hasSign )
      {
         String strSignFile = READ_CONFIG(m_profile, MP_COMPOSE_SIGNATURE);
         if ( !strSignFile.empty() )
         {
            wxFileName fn(strSignFile);
            fn.MakeAbsolute(mApplication->GetLocalDir());

            const wxString path = fn.GetFullPath();

            // interpret the file as Unicode by default in Unicode build
#if wxUSE_UNICODE
            {
               wxLogNull noLog;
               hasSign = fileSig.Open(path);
            }

            if ( !hasSign )
#endif // wxUSE_UNICODE
            {
               // but if it fails, use the current locale encoding
               wxLogNull noLog;
               hasSign = fileSig.Open(path, wxCSConv(wxFONTENCODING_SYSTEM));
            }

            if ( !hasSign )
            {
               // and if it still fails, try to interpret it as latin1 as
               // otherwise we'd never be able to read non-UTF-8 files in UTF-8
               // locale
               hasSign = fileSig.Open(path, wxConvISO8859_1);
            }

            if ( !hasSign )
            {
               wxLogError(_("Failed to read signature file \"%s\""),
                          path);
            }
         }

         if ( !hasSign )
         {
            // no signature at all or sig file not found, propose to choose or
            // change it now
            wxString msg;
            if ( strSignFile.empty() )
            {
               msg = _("You haven't configured your signature file.");
            }
            else
            {
               // to show error message from wxTextFile::Open()
               wxLog *log = wxLog::GetActiveTarget();
               if ( log )
                  log->Flush();

               msg.Printf(_("Signature file '%s' couldn't be opened."),
                          strSignFile);
            }

            msg += _("\n\nWould you like to choose your signature "
                     "right now (otherwise no signature will be used)?");
            if ( MDialog_YesNoDialog(msg, m_cv.GetFrame(), MDIALOG_YESNOTITLE,
                                     M_DLG_YES_DEFAULT,
                                     M_MSGBOX_ASK_FOR_SIG) )
            {
               strSignFile = wxPLoadExistingFileSelector
                             (
                                 m_cv.GetFrame(),
                                 "sig",
                                 _("Choose signature file"),
                                 NULL, _T(".signature"), NULL
                             );
            }
            else
            {
               // user doesn't want to use signature file
               break;
            }

            if ( strSignFile.empty() )
            {
               // user canceled "choose signature" dialog
               break;
            }

            m_profile->writeEntry(MP_COMPOSE_SIGNATURE, strSignFile);
         }
      }

      if ( hasSign )
      {
         // insert separator optionally
         if ( READ_CONFIG(m_profile, MP_COMPOSE_USE_SIGNATURE_SEPARATOR) )
         {
            signature += _T("-- \n");
         }

         // read the whole file
         size_t nLineCount = fileSig.GetLineCount();
         for ( size_t nLine = 0; nLine < nLineCount; nLine++ )
         {
            if ( nLine )
               signature += '\n';

            signature += fileSig[nLine];
         }

         // let's respect the netiquette
         static const size_t nMaxSigLines = 4;
         if ( nLineCount > nMaxSigLines )
         {
            wxString msg;
            if ( nLineCount > 10 )
            {
               // *really* insult the user -- [s]he merits it
               msg.Printf(_("Your signature is waaaaay too long: "
                            "it should be no more than %d lines, "
                            "please trim it as it it risks to be "
                            "unreadable for the others."),
                          nMaxSigLines);
            }
            else // too long but not incredibly too long
            {
               msg.Printf(_("Your signature is too long: it should "
                            "not be more than %d lines."),
                            nMaxSigLines);
            }

            MDialog_Message(msg, m_cv.GetFrame(),
                            _("Signature is too long"),
                            GetPersMsgBoxName(M_MSGBOX_SIGNATURE_LENGTH));

         }
      }
      else
      {
         // don't ask the next time
         m_profile->writeEntry(MP_COMPOSE_USE_SIGNATURE, false);
      }
   }

   return signature;
}

// ----------------------------------------------------------------------------
// Quoting helpers
// ----------------------------------------------------------------------------

void
VarExpander::DoQuoteOriginal(bool isQuote, String *value) const
{
   if ( m_textToQuote.empty() )
   {
      // don't quote anything at all
      return;
   }

   // insert the original text (optionally prefixed by reply
   // string)
   String prefix;
   if ( isQuote )
   {
      prefix = GetReplyPrefix(m_msg, m_profile);
   }
   //else: template "text", so no reply prefix at all

   *value = ExpandOriginalText(m_textToQuote, prefix, m_profile);
}

// ----------------------------------------------------------------------------
// public API
// ----------------------------------------------------------------------------

extern bool TemplateNeedsHeaders(const String& templateValue)
{
   // check if there are any occurrences of "${message:xxx}" in the template
   //
   // TODO: really parse it using a specialized expanded and without any
   //       sink, just checking if message category appears in it
   return templateValue.Lower().Find(_T("message:")) != wxNOT_FOUND;
}

extern bool ExpandTemplate(Composer& cv,
                           Profile *profile,
                           const String& templateValue,
                           Message *msg,
                           const String& textToQuote)
{
   ExpansionSink sink;
   VarExpander expander(sink, cv, profile, msg, textToQuote);
   MessageTemplateParser parser(templateValue, _("template"), &expander);
   if ( !parser.Parse(sink) )
   {
      return false;
   }

   sink.InsertTextInto(cv);

   return true;
}

extern String QuoteText(const String& text,
                        Profile *profile,
                        Message *msgOriginal)
{
   CHECK( profile && msgOriginal, wxEmptyString, _T("NULL parameters in QuoteText") );

   const String prefix = GetReplyPrefix(msgOriginal, profile);

   return ExpandOriginalText(text, prefix, profile);
}

