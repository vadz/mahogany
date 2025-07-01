///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   src/modules/spam/ServerSideFilter.cpp
// Purpose:     SpamFilter implementation using server-side filtering
// Author:      Vadim Zeitlin
// Created:     2008-04-28
// CVS-ID:      $Id$
// Copyright:   (c) 2008 Vadim Zeitlin <vadim@wxwindows.org>
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
#endif //USE_PCH

#include "MFolder.h"
#include "MailFolder.h"
#include "Message.h"
#include "HeaderInfo.h"
#include "UIdArray.h"
#include "SendMessage.h"

#include "SpamFilter.h"
#include "gui/SpamOptionsPage.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// names for the options we use (all are empty strings by default)
static const char *SRV_SPAM_HEADER_NAME = "ServerSpamHeaderName";
static const char *SRV_SPAM_HEADER_VALUE = "ServerSpamHeaderValue";
static const char *SRV_SPAM_FOLDER = "ServerSpamFolder";
static const char *SRV_SPAM_BOUNCE_HAM = "ServerSpamBounceHam";
static const char *SRV_SPAM_BOUNCE_SPAM = "ServerSpamBounceSpam";

// ----------------------------------------------------------------------------
// ServerSideFilter class
// ----------------------------------------------------------------------------

class ServerSideFilter : public SpamFilter
{
public:
   ServerSideFilter() { }

protected:
   virtual bool DoReclassify(const Profile *profile,
                             const Message& msg,
                             bool isSpam);
   virtual void DoTrain(const Profile *profile,
                        const Message& msg,
                        bool isSpam);
   virtual int DoCheckIfSpam(const Profile *profile,
                             const Message& msg,
                             const String& param,
                             String *result);
   virtual const char *GetOptionPageIconName() const { return "serverspam"; }
   virtual SpamOptionsPage *CreateOptionPage(MBookCtrl *notebook,
                                             Profile *profile) const;

private:
   DECLARE_SPAM_FILTER("serverside", _("Server Side"), 10);
};

IMPLEMENT_SPAM_FILTER(ServerSideFilter,
                      gettext_noop("Server-Side Spam Filter"),
                      "(c) 2008 Vadim Zeitlin <vadim@wxwindows.org>");

// ----------------------------------------------------------------------------
// ServerSideOptionsPage
// ----------------------------------------------------------------------------

class ServerSideOptionsPage : public SpamOptionsPage
{
public:
   ServerSideOptionsPage(const ServerSideFilter *filter,
                         MBookCtrl *parent,
                         Profile *profile);

private:
   ServerSideFilter *m_filter;
};

// ============================================================================
// ServerSideFilter implementation
// ============================================================================

// ----------------------------------------------------------------------------
// ServerSideFilter public API implementation
// ----------------------------------------------------------------------------

bool
ServerSideFilter::DoReclassify(const Profile *profile,
                               const Message& msg,
                               bool isSpam)
{
   const String folder = profile->readEntry(SRV_SPAM_FOLDER);
   if ( !folder.empty() )
   {
      if ( !isSpam )
      {
         wxLogMessage(_("Server-side filter using spam folder can only "
                        "reclassify messages as spam by moving them into "
                        "this folder. To classify a message as non-spam it is "
                        "enough to move it out of the junk folder."));
         return false;
      }

      String reason; // explanation of why error happened (if it did)
      MailFolder * const mf = msg.GetFolder();
      if ( !mf )
      {
         reason = _("failed to get the folder containing the message");
      }
      else
      {
         UIdArray uids;
         uids.push_back(msg.GetUId());
         if ( !mf->SaveMessages(&uids, folder) )
            reason = _("failed to copy message to the spam folder ") + folder;
      }

      if ( !reason.empty() )
      {
         wxLogError(_("Error while reclassifying the message: %s."),
                    reason);
         return false;
      }
   }
   else // reclassify by bouncing to some address
   {
      const String addr = profile->readEntry(isSpam ? SRV_SPAM_BOUNCE_SPAM
                                                    : SRV_SPAM_BOUNCE_HAM);
      if ( addr.empty() )
      {
         wxLogWarning(_("You need to configure either a server-side folder "
                        "containing spam or the address to bounce messages "
                        "to in order to train the server-side spam filter."));
         return false;
      }

      SendMessage::Bounce(addr, profile, msg);
   }

   return true;
}

void
ServerSideFilter::DoTrain(const Profile * /* profile */,
                          const Message& /* msg */,
                          bool /* isSpam */)
{
   wxLogError(_("Server-side spam filter can't be trained from the client."));
}

int
ServerSideFilter::DoCheckIfSpam(const Profile *profile,
                                const Message& msg,
                                const String& param,
                                String *result)
{
   // first get the header to check: if any of the required values are not set,
   // this filter is not used at all so we simply return false
   const String headerName = profile->readEntry(SRV_SPAM_HEADER_NAME);
   if ( headerName.empty() )
      return false;

   const String headerValue = profile->readEntry(SRV_SPAM_HEADER_VALUE);
   if ( headerValue.empty() )
      return false;


   // do check the configured header
   String value;
   if ( !msg.GetHeaderLine(headerName, value) )
      return false;

   if ( !value.StartsWith(headerValue) )
      return false;

   if ( result )
      result->Printf("%s: %s", headerName, headerValue);

   return true;
}

// ----------------------------------------------------------------------------
// ServerSideFilter user-configurable options
// ----------------------------------------------------------------------------

SpamOptionsPage *
ServerSideFilter::CreateOptionPage(MBookCtrl *notebook,
                                   Profile *profile) const
{
   return new ServerSideOptionsPage(this, notebook, profile);
}

// ============================================================================
// ServerSideOptionsPage implementation
// ============================================================================

enum
{
   ServerSidePageField_Help,
   ServerSidePageField_HeaderHelp,
   ServerSidePageField_Header,
   ServerSidePageField_ValueHelp,
   ServerSidePageField_Value,
   ServerSidePageField_SpamFolderHelp,
   ServerSidePageField_SpamFolder,
   ServerSidePageField_BounceHelp,
   ServerSidePageField_HamBounce,
   ServerSidePageField_SpamBounce,
   ServerSidePageField_Max
};

ServerSideOptionsPage::ServerSideOptionsPage(const ServerSideFilter *filter,
                                             MBookCtrl *notebook,
                                             Profile *profile)
      : SpamOptionsPage(notebook, profile)
{
   static const wxOptionsPage::FieldInfo s_fields[] =
   {
      {
         gettext_noop
         (
            "This page is used for configuring Mahogany integration\n"
            "with a server-side spam filter. Such filters are operated\n"
            "by some ISPs and typically insert special headers in the\n"
            "incoming messages indicating whether an email was recognized\n"
            "as spam.\n"
            "\n"
            "If the filter is of probabilistic nature, there should also\n"
            "be a way to train it, i.e. correct its mistakes when it wrongly\n"
            "classifies a non-spam message as ham or, on the contrary, doesn't\n"
            "recognize a spam. This is usually done either by resending the\n"
            "message which was incorrectly classified to some special address\n"
            "or moving it to a special \"Spam\" folder and you should choose\n"
            "the appropriate option below.\n"
            "\n"
            "Notice that operation of server-side filter is not under Mahogany\n"
            "control. If you prefer to do spam checks locally, please use\n"
            "the high-quality integrated DSPAM filter instead of this one.\n"
         ),
         wxOptionsPage::Field_Message,
         -1
      },

      {
         gettext_noop
         (
            "\n"
            "Enter the name of the header added by the server-side filter.\n"
            "For example, use \"X-Spam-Flag\" for SpamAssasin or\n"
            "\"X-DSPAM-Result\" for DSPAM."
         ),
         wxOptionsPage::Field_Message,
         -1
      },
      {
         gettext_noop("&Header to check"),
         wxOptionsPage::Field_Text,
         -1
      },

      {
         gettext_noop
         (
            "\n"
            "Enter the value of the header selected above indicating that\n"
            "the message is spam. E.g. use \"YES\" with \"X-Spam-Flag\" or\n"
            "\"Spam\" with \"X-DSPAM-Result\". This value must match (case-"
            "sensitively)\n"
            "in the beginning of the string for the message to be recognized "
            "as spam."
         ),
         wxOptionsPage::Field_Message,
         ServerSidePageField_Header
      },
      {
         gettext_noop("&Value indicating spam"),
         wxOptionsPage::Field_Text,
         ServerSidePageField_Header
      },

      {
         gettext_noop
         (
            "\n\n"
            "If the message reclassification is done by moving it in a special\n"
            "folder, select it here. Otherwise leave it empty and, if the\n"
            "filter supports reclassification at all, fill in the addresses below."
         ),
         wxOptionsPage::Field_Message,
         -1
      },
      {
         gettext_noop("Server &folder for spam"),
         wxOptionsPage::Field_Folder,
         -1
      },

      {
         gettext_noop
         (
            "When the message was mistakenly recognized as spam, the\n"
            "\"Mark as ham\" menu command will resend it to the first address\n"
            "(typically something like non-spam-username@server.com). When a\n"
            "spam message was not recognized as such, the \"Dispose as spam\"\n"
            "command will resend it to the second address."
         ),
         wxOptionsPage::Field_Message,
         -ServerSidePageField_SpamFolder
      },
      {
         gettext_noop("Resend &non-spam to"),
         wxOptionsPage::Field_Text,
         -ServerSidePageField_SpamFolder
      },
      {
         gettext_noop("Resend &spam to"),
         wxOptionsPage::Field_Text,
         -ServerSidePageField_SpamFolder
      },
   };

   wxCOMPILE_TIME_ASSERT2( WXSIZEOF(s_fields) == ServerSidePageField_Max,
                              FieldsNotInSync, ServerSideFields );


   #define CONFIG_STR(name) ConfigValueDefault(name, "")
   #define CONFIG_NONE()  ConfigValueNone()

   static const ConfigValueDefault s_values[] =
   {
      CONFIG_NONE(),
      CONFIG_NONE(),
      CONFIG_STR(SRV_SPAM_HEADER_NAME),
      CONFIG_NONE(),
      CONFIG_STR(SRV_SPAM_HEADER_VALUE),
      CONFIG_NONE(),
      CONFIG_STR(SRV_SPAM_FOLDER),
      CONFIG_NONE(),
      CONFIG_STR(SRV_SPAM_BOUNCE_HAM),
      CONFIG_STR(SRV_SPAM_BOUNCE_SPAM),
   };

   #undef CONFIG_NONE
   #undef CONFIG_STR

   wxCOMPILE_TIME_ASSERT2( WXSIZEOF(s_values) == ServerSidePageField_Max,
                              ValuesNotInSync, ServerSideValues );


   Create
   (
      notebook,
      gettext_noop("Server Side"),
      profile,
      s_fields,
      s_values,
      ServerSidePageField_Max,
      0,                            // offset from the start (none)
      -1,                           // help id (none)
      notebook->GetPageCount()      // image id
   );
}

