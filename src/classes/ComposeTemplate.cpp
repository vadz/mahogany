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
#  include <wx/log.h>

#  include "sysutil.h"
#  include "strutil.h"
#endif

#include "TemplateDialog.h"
#include "MApplication.h"

#include "Composer.h"
#include "MDialogs.h"
#include "Mdefaults.h"

#include "MessageView.h"

#include <wx/confbase.h>      // for wxExpandEnvVars()
#include <wx/file.h>
#include <wx/ffile.h>
#include <wx/tokenzr.h>

#include <wx/regex.h>

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
      Category_Message,    // the headers of the message being written
      Category_Original,   // variables pertaining to the original message
      Category_Invalid,    // unknown/invalid category
      Category_Max = Category_Invalid
   };

   // the variables in "misc" category
   enum Variable
   {
      Var_Date,            // insert the date in the default format
      Var_Cursor,          // position the cursor here after template expansion
      Var_To,              // the recipient name
      Var_Subject,         // the message subject (without any "Re"s)

      // all entries from here only apply to the reply/forward/followup
      // templates because they work with the original message
      Var_Quote,           // quote the original text
      Var_Quote822,        // include the original msg as a RFC822 attachment
      Var_Text,            // include the original text as is
      Var_Sender,          // the fullname of the sender

      Var_Invalid,
      Var_Max = Var_Invalid
   };

   // the variables in "message" category
   //
   // NB: the values should be identical to wxComposeView::RecipientType enum
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
      OriginalHeader_ReplyTo,
      OriginalHeader_Newsgroups,
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
   // return Var_Invalid if variable is unknown)
   static Variable GetVariable(const String& variable)
   {
      return (Variable)FindStringInArray(ms_templateVarNames,
                                         Var_Max, variable);
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
               Profile *profile = NULL,
               Message *msg = NULL,
               MessageView *msgview = NULL)
      : m_sink(sink), m_cv(cv)
   {
      m_profile = profile ? profile : mApplication->GetProfile();
      m_profile->IncRef();

      m_msg = msg;
      SafeIncRef(m_msg);

      m_msgview = msgview;
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
   bool ExpandMisc(const String& name, String *value) const;
   bool ExpandFile(const String& name,
                   const wxArrayString& arguments,
                   String *value) const;
   bool ExpandAttach(const String& name,
                     const wxArrayString& arguments,
                     String *value) const;
   bool ExpandCommand(const String& name, String *value) const;
#ifdef USE_PYTHON
   bool ExpandPython(const String& name, String *value) const;
#endif // USE_PYTHON
   bool ExpandMessage(const String& name, String *value) const;
   bool ExpandOriginal(const String& name, String *value) const;

   // return the reply prefix to use for message quoting when replying
   String GetReplyPrefix() const;

   // put the text quoted according to our current quoting options with the
   // given reply prefix into value
   void ExpandOriginalText(const String& text,
                           const String& prefix,
                           String *value) const;

private:
   // helper used by GetCategory and GetVariable
   static int FindStringInArray(const char *strs[], int max, const String& s);

   // the sink we use when expanding pseudo variables
   ExpansionSink& m_sink;

   // the compose view is used for expansion of the variables in "message"
   // category
   Composer& m_cv;

   // the message used for expansion of variables pertaining to the original
   // message (may be NULL for new messages)
   Message *m_msg;

   // the message viewer we use for querying the selection if necessary
   MessageView *m_msgview;

   // the profile to use for everything (global one by default)
   Profile *m_profile;

   // this array contains the list of all categories
   static const char *ms_templateVarCategories[Category_Max];

   // this array contains the list of all variables without category
   static const char *ms_templateVarNames[Var_Max];

   // this array contains the variable names from "message" category
   static const char *ms_templateMessageVars[MessageHeader_Max];

   // this array contains all the variables in the "original" category which
   // map to the headers of the original message (there are other variables in
   // this category as well)
   static const char *ms_templateOriginalVars[OriginalHeader_Max];
};

// ----------------------------------------------------------------------------
// global data: the definitions of the popum menu for the template editing
// dialog.
// ----------------------------------------------------------------------------

// NB: These menus should be kept in sync (if possible) with the variable names

// the misc submenu
static TemplatePopupMenuItem gs_popupSubmenuMisc[] =
{
   TemplatePopupMenuItem(_("Put &cursor here"), "$cursor"),
   TemplatePopupMenuItem(),
   TemplatePopupMenuItem(_("Insert current &date"), "$date"),
   TemplatePopupMenuItem(),
   TemplatePopupMenuItem(_("Insert &quoted text"), "$quote"),
   TemplatePopupMenuItem(_("&Attach original text"), "$quote822"),
};

// the file insert/attach sub menu
static TemplatePopupMenuItem gs_popupSubmenuFile[] =
{
   TemplatePopupMenuItem(_("&Insert file..."), "${file:%s}", TRUE),
   TemplatePopupMenuItem(_("Insert &any file..."), "${file:%s?ask", TRUE),
   TemplatePopupMenuItem(_("Insert &quoted file..."), "${file:%s?quote}", TRUE),
   TemplatePopupMenuItem(_("&Attach file..."), "${attach:%s}", TRUE),
};

// the message submenu
static TemplatePopupMenuItem gs_popupSubmenuMessage[] =
{
   TemplatePopupMenuItem(_("&To"), "${message:to}"),
   TemplatePopupMenuItem(_("&First name"), "${message:firstname}"),
   TemplatePopupMenuItem(_("&Last name"), "${message:lastname}"),
   TemplatePopupMenuItem(_("&Subject"), "${message:subject}"),
   TemplatePopupMenuItem(_("&CC"), "${message:cc}"),
   TemplatePopupMenuItem(_("&BCC"), "${message:bcc}"),
};

// the original message submenu
static TemplatePopupMenuItem gs_popupSubmenuOriginal[] =
{
   TemplatePopupMenuItem(_("&Date"), "${original:date}"),
   TemplatePopupMenuItem(_("&From"), "${original:from}"),
   TemplatePopupMenuItem(_("&Subject"), "${original:subject}"),
   TemplatePopupMenuItem(_("Full &name"), "${original:fullname}"),
   TemplatePopupMenuItem(_("F&irst name"), "${original:firstname}"),
   TemplatePopupMenuItem(_("&Last name"), "${original:lastname}"),
   TemplatePopupMenuItem(_("&To"), "${original:to}"),
   TemplatePopupMenuItem(_("&Reply to"), "${original:replyto}"),
   TemplatePopupMenuItem(_("&Newsgroups"), "${original:newsgroups}"),
   TemplatePopupMenuItem(),
   TemplatePopupMenuItem(_("Insert &quoted text"), "$quote"),
   TemplatePopupMenuItem(_("&Attach original text"), "$quote822"),
   TemplatePopupMenuItem(_("Insert &unquoted Text"), "$text"),
};

// the whole menu
static TemplatePopupMenuItem gs_popupMenu[] =
{
   TemplatePopupMenuItem(_("&Miscellaneous"),
                         gs_popupSubmenuMisc,
                         WXSIZEOF(gs_popupSubmenuMisc)),
   TemplatePopupMenuItem(_("Message &headers"),
                         gs_popupSubmenuMessage,
                         WXSIZEOF(gs_popupSubmenuMessage)),
   TemplatePopupMenuItem(_("&Original message"),
                         gs_popupSubmenuOriginal,
                         WXSIZEOF(gs_popupSubmenuOriginal)),
   TemplatePopupMenuItem(_("Insert or attach a &file"),
                         gs_popupSubmenuFile,
                         WXSIZEOF(gs_popupSubmenuFile)),
   TemplatePopupMenuItem(),
   TemplatePopupMenuItem(_("&Execute command..."), "${cmd:%s}", FALSE),
};

const TemplatePopupMenuItem& g_ComposeViewTemplatePopupMenu =
   TemplatePopupMenuItem("", gs_popupMenu, WXSIZEOF(gs_popupMenu));

// ============================================================================
// implementation
// ============================================================================

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
      // FIXME: this supposes that there is no autowrap - is it true?
      int deltaX = 0, deltaY = 0;
      for ( const char *pc = text.c_str(); *pc; pc++ )
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
               "something is very wrong in template expansion sink" );

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
   cv.MoveCursorTo(m_x, m_y);

   // as the inserted text comes from the program, not from the user, don't
   // mark the composer contents as dirty
   cv.ResetDirty();
}

// ----------------------------------------------------------------------------
// VarExpander - used by wxComposeView
// ----------------------------------------------------------------------------

const char *VarExpander::ms_templateVarCategories[] =
{
   "",
   "file",
   "attach",
   "cmd",
#ifdef USE_PYTHON
   "python",
#endif // USE_PYTHON
   "message",
   "original",
};

const char *VarExpander::ms_templateVarNames[] =
{
   "date",
   "cursor",
   "to",
   "subject",
   "quote",
   "quote822",
   "text",
   "sender",
};

const char *VarExpander::ms_templateMessageVars[] =
{
   "to",
   "cc",
   "bcc",
   "subject",
   "firstname",
   "lastname",
};

const char *VarExpander::ms_templateOriginalVars[] =
{
   "date",
   "from",
   "subject",
   "fullname",
   "firstname",
   "lastname",
   "to",
   "replyto",
   "newsgroups",
};

int
VarExpander::FindStringInArray(const char *strings[],
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
   *value = '?';

   wxFFile file(filename);

   return file.IsOpened() && file.ReadAll(value);
}

String
VarExpander::GetAbsFilename(const String& name)
{
   String filename = wxExpandEnvVars(name);
   if ( !IsAbsPath(filename) )
   {
      Profile *profile = mApplication->GetProfile();
      String path = profile->readEntry(MP_COMPOSETEMPLATEPATH_USER,
                                       mApplication->GetLocalDir());
      if ( !path || path.Last() != '/' )
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
         String path = profile->readEntry(MP_COMPOSETEMPLATEPATH_GLOBAL,
                                          mApplication->GetGlobalDir());
         if ( !path || path.Last() != '/' )
         {
            path += '/';
         }

         filename.Prepend(path);
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
         return ExpandCommand(name, value);

#ifdef USE_PYTHON
      case Category_Python:
         return ExpandPython(name, value);
#endif // USE_PYTHON

      case Category_Message:
         return ExpandMessage(name, value);

      case Category_Original:
         return ExpandOriginal(name, value);

      case Category_Misc:
         return ExpandMisc(name, value);

      default:
         // unknown category
         return FALSE;
   }
}

bool
VarExpander::ExpandMisc(const String& name, String *value) const
{
   // deal with all special cases
   switch ( GetVariable(name.Lower()) )
   {
      case Var_Date:
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

      case Var_Cursor:
         m_sink.RememberCursorPosition();
         break;

      case Var_To:
         // just the shorthand for "message:to"
         return ExpandMessage("to", value);

      case Var_Subject:
         return ExpandMessage("subject", value);

      case Var_Quote:
         return ExpandOriginal("quote", value);

      case Var_Quote822:
         return ExpandOriginal("quote822", value);

      case Var_Text:
         return ExpandOriginal("text", value);

      case Var_Sender:
         ExpandOriginal("from", value);
         return TRUE;

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
   if ( arguments.Index("ask", FALSE /* no case */) != wxNOT_FOUND )
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
                    name.c_str());

         return FALSE;
      }

      // do we want to quote the files contents before inserting?
      if ( arguments.Index("quote", FALSE /* no case */) != wxNOT_FOUND )
      {
         String prefix = READ_CONFIG(m_profile, MP_REPLY_MSGPREFIX);
         String quotedValue;
         quotedValue.Alloc(value->length());

         const char *cptr = value->c_str();
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
   if ( arguments.Index("ask", FALSE /* no case */) != wxNOT_FOUND )
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
                    name.c_str());

         return FALSE;
      }

      // guess MIME type from extension
      m_sink.InsertAttachment(strutil_strdup(value->c_str()),
                              value->length(),
                              "", // will be determined from filename laer
                              filename);

      // avoid inserting file as text additionally
      value->Empty();
   }
   //else: no file, nothing to attach

   return TRUE;
}

bool
VarExpander::ExpandCommand(const String& name, String *value) const
{
   // execute a command (FIXME get the wxProcess class allowing stdout
   // redirection and use it here)
   MTempFileName temp;

   bool ok = temp.IsOk();
   wxString filename = temp.GetName(), command;

   if ( ok )
      command << name << " > " << filename;

   if ( ok )
      ok = system(command) == 0;

   if ( ok )
      ok = SlurpFile(filename, value);

   if ( !ok )
   {
      wxLogSysError(_("Failed to execute the command '%s'"), name.c_str());

      return FALSE;
   }

   return TRUE;
}

#ifdef USE_PYTHON
bool
VarExpander::ExpandPython(const String& name, String *value) const
{
   // TODO
   return FALSE;
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
      case MessageHeader_FirstName:
      case MessageHeader_LastName:
         {
            // FIXME: this won't work if there are several addresses!

            wxString to = m_cv.GetRecipients(Composer::Recipient_To);
            if ( header == MessageHeader_FirstName )
               *value = Message::GetFirstNameFromAddress(to);
            else
               *value = Message::GetLastNameFromAddress(to);
         }
         break;

      default:
         CHECK( header <= MessageHeader_LastControl, FALSE,
                "unexpected macro in message category" );

         // the MessageHeader enum values are the same as RecipientType ones,
         // so no translation is needed
         *value = m_cv.GetRecipients((Composer::RecipientType)header);
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
            m_msg->Address(*value, MAT_REPLYTO);
            break;

         case OriginalHeader_To:
            m_msg->Address(*value, MAT_TO);
            break;

         case OriginalHeader_PersonalName:
            m_msg->Address(*value, MAT_FROM);
            break;

         case OriginalHeader_FirstName:
            *value = m_msg->GetAddressFirstName(MAT_FROM);
            break;

         case OriginalHeader_LastName:
            *value = m_msg->GetAddressLastName(MAT_FROM);
            break;

         case OriginalHeader_Newsgroups:
            m_msg->GetHeaderLine("Newsgroups", *value);
            break;

         default:
            isHeader = false;

            // it isn't a variable which maps directly onto header, check the
            // others
            if ( name == "text" || name == "quote" )
            {
               // insert the original text (optionally prefixed by reply
               // string)
               String prefix;
               if ( name == "quote" )
               {
                  prefix = GetReplyPrefix();
               }
               //else: name == text, so no reply prefix at all

               // do we include everything or just the selection?

               // first: can we get the selection?
               bool justSelection = m_msgview != NULL;

               // second: should we use the selection?
               if ( justSelection &&
                     !READ_CONFIG(m_profile, MP_REPLY_QUOTE_SELECTION) )
               {
                  justSelection = false;
               }

               // third: do we have any selection?
               if ( justSelection )
               {
                  String selection = m_msgview->GetSelection();
                  if ( selection.empty() )
                  {
                     // take everything if no selection
                     justSelection = false;
                  }
                  else
                  {
                     // include the selection only in the template expansion
                     ExpandOriginalText(selection, prefix, value);
                  }
               }

               // quote everything
               if ( !justSelection )
               {
                  // was the last message part a text one?
                  bool lastWasPlainText = false;

                  int nParts = m_msg->CountParts();
                  for ( int nPart = 0; nPart < nParts; nPart++ )
                  {
                     if ( m_msg->GetPartType(nPart) == MimeType::TEXT )
                     {
                        // FIXME: we lack propert multipart/alternative support -
                        //        until we have it we can at least avoid
                        //        inserting all parts of such messages when
                        //        replying in the most common case (2 parts:
                        //        text/plain and text/html)
                        String mimeType = m_msg->GetPartMimeType(nPart).Lower();
                        if ( mimeType == "text/html" && lastWasPlainText )
                        {
                           // skip it
                           continue;
                        }

                        lastWasPlainText = mimeType == "text/plain";

                        ExpandOriginalText(m_msg->GetPartContent(nPart),
                                           prefix,
                                           value);
                     }
                     else
                     {
                        lastWasPlainText = false;
                     }
                  }
               }
            }
            else if ( name == "quote822" )
            {
               // insert the original message as RFC822 attachment
               String str;
               m_msg->WriteToString(str);
               m_sink.InsertAttachment(strutil_strdup(str), str.Length(),
                                       "message/rfc822", "");
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
         *value = MailFolder::DecodeHeader(*value);
      }
   }

   return TRUE;
}

// ----------------------------------------------------------------------------
// ExpandOriginal helpers
// ----------------------------------------------------------------------------

String VarExpander::GetReplyPrefix() const
{
   String prefix;

   // prepend the senders initials to the reply prefix (this
   // will make reply prefix like "VZ>")
   if ( READ_CONFIG(m_profile, MP_REPLY_MSGPREFIX_FROM_SENDER) )
   {
      // take from address, not reply-to which can be set to
      // reply to a mailing list, for example
      String name;
      m_msg->Address(name, MAT_FROM);
      if ( name.empty() )
      {
         // no from address? try to find anything else
         m_msg->Address(name, MAT_REPLYTO);
      }

      name = MailFolder::DecodeHeader(name);

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

         // take the first letter of each word
         wxStringTokenizer tk(name);
         while ( tk.HasMoreTokens() )
         {
            char chInitial = tk.GetNextToken()[0u];

            if ( chInitial == '<' )
            {
               // this must be the start of embedded "<...>"
               // address, skip it completely
               break;
            }

            // only take letters as initials
            if ( isalpha(chInitial) )
            {
               prefix += chInitial;
            }
         }
      }
   }

   // and then the standard reply prefix too
   prefix += READ_CONFIG(m_profile, MP_REPLY_MSGPREFIX);

   return prefix;
}

inline bool IsEndOfLine(const char *p)
{
   return p[0] == '\r' && p[1] == '\n';
}

// length of end of line suffix ("\r\n")
static const size_t EOL_LEN = 2;

void VarExpander::ExpandOriginalText(const String& text,
                                     const String& prefix,
                                     String *value) const
{
   // should we quote the empty lines?
   //
   // this option is ignore when we're inserting text verbatim (hence without
   // reply prefix) and not quoting it
   bool quoteEmpty = !prefix.empty() &&
                        READ_CONFIG(m_profile, MP_REPLY_QUOTE_EMPTY);

   // where to break lines (if at all)?
   size_t wrapMargin;
   if ( READ_CONFIG(m_profile, MP_AUTOMATIC_WORDWRAP) )
   {
      wrapMargin = READ_CONFIG(m_profile, MP_WRAPMARGIN);
      if ( wrapMargin <= prefix.length() )
      {
         wxLogError(_("The configured automatic wrap margin (%u) is too "
                      "small, please increase it.\n"
                      "\n"
                      "Disabling automatic wrapping for now."), wrapMargin);

         m_profile->writeEntry(MP_AUTOMATIC_WORDWRAP, false);
         wrapMargin = 0;
      }
   }
   else
   {
      // don't wrap
      wrapMargin = 0;
   }

   // should we detect the signature and discard it?
   bool detectSig = READ_CONFIG(m_profile, MP_REPLY_DETECT_SIG) != 0;

#if wxUSE_REGEX
   // a RE to detect the start of the signature
   wxRegEx reSig;
   if ( detectSig )
   {
      String sig = READ_CONFIG(m_profile, MP_REPLY_SIG_SEPARATOR);

      // we implicitly anchor the RE at start/end of line
      //
      // VZ: couldn't we just use wxRE_NEWLINE in Compile() instead of "\r\n"?
      String sigRE;
      sigRE << '^' << sig << "\r\n";

      if ( !reSig.Compile(sigRE, wxRE_NOSUB) )
      {
         wxLogError(_("Regular expression '%s' used for detecting the "
                       "signature start is invalid, please modify it.\n"
                       "\n"
                       "Disabling sinature stripping for now."),
                    sigRE.c_str());

         m_profile->writeEntry(MP_REPLY_DETECT_SIG, false);
         detectSig = false;
      }
   }
#endif // wxUSE_REGEX

   // the current line
   String lineCur;

   for ( const char *cptr = text.c_str(); ; cptr++ )
   {
      // start of [real] new line?
      if ( lineCur.empty() )
      {
         if ( detectSig )
         {
#if wxUSE_REGEX
            if ( reSig.Matches(cptr) )
               break;
#else // !wxUSE_REGEX
            // hard coded detection for standard signature separator "--"
            if ( cptr[0] == '-' && cptr[1] == '-' )
            {
               // there may be an optional space after "--"
               const char *p = cptr + 2;
               if ( IsEndOfLine(p) || (*p == ' ' && IsEndOfLine(p + 1)) )
               {
                  // the rest is the sig - skip
                  break;
               }
            }
#endif // wxUSE_REGEX/!wxUSE_REGEX
         }

         if ( !quoteEmpty && IsEndOfLine(cptr + 1) )
         {
            // this line is empty, skip it entirely
            cptr += EOL_LEN - 1;

            continue;
         }

         lineCur += prefix;
      }

      if ( !*cptr || IsEndOfLine(cptr) )
      {
         // sanity test
         ASSERT_MSG( !wrapMargin || lineCur.length() <= wrapMargin,
                     "logic error in auto wrap code" );

         *value += lineCur;

         if ( !*cptr )
         {
            // end of text
            break;
         }

         // put just '\n' in output, we don't need "\r\n"
         *value += '\n';

         lineCur.clear();

         // -1 to compensate for ++ in the loop
         cptr += EOL_LEN - 1;
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
               if ( isspace(lineCur[n]) )
                  break;

               n--;
            }

            if ( n == prefix.length() )
            {
               // no space found in the line or it is in prefix which
               // we don't want to wrap - so just cut the line right here
               n = wrapMargin;
            }

            value->append(lineCur, n);
            *value += '\n';

            // we don't need to start the new line with spaces so remove them
            // from the tail
            while ( n < lineCur.length() && isspace(lineCur[n]) )
            {
               n++;
            }

            lineCur.erase(0, n);
            lineCur.Prepend(prefix);
         }
      }
   }
}

// ----------------------------------------------------------------------------
// public API
// ----------------------------------------------------------------------------

extern bool ExpandTemplate(Composer& cv,
                           Profile *profile,
                           const String& templateValue,
                           Message *msg,
                           MessageView *msgview)
{
   ExpansionSink sink;
   VarExpander expander(sink, cv, profile, msg, msgview);
   MessageTemplateParser parser(templateValue, _("template"), &expander);
   if ( !parser.Parse(sink) )
   {
      return false;
   }

   sink.InsertTextInto(cv);

   return true;
}

