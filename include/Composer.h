///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   Composer.h - abstract composer interface
// Purpose:     Composer class declaration
// Author:      Vadim Zeitlin
// Modified by:
// Created:     20.07.01
// CVS-ID:      $Id$
// Copyright:   (c) 2001 Vadim Zeitlin
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _COMPOSER_H_
#define _COMPOSER_H_

#include "MailFolder.h"    // for MailFolder::Params

#include <wx/colour.h>     // wxColour

class Profile;
class Message;
class MessageView;
class MimePart;
class wxComposeView;

// ----------------------------------------------------------------------------
// Composer provides GUI-independent interface for the compose frame
// ----------------------------------------------------------------------------

/// the composer options (i.e. values read from profile)
struct ComposerOptions
{
   /// font description
   String m_font;

   /// font family and size used only if m_font is empty
   int m_fontFamily,
       m_fontSize;

   /// composer colours
   wxColour m_fg,
            m_bg;

   /// ctor initializes everything to some invalid values
   ComposerOptions();

   /// read the options from the given profile
   void Read(Profile *profile);

   /// get the font using either m_font or m_fontFamily/Size
   wxFont GetFont() const;
};

class Composer
{
public:
   /// recipient address types
   enum RecipientType
   {
      Recipient_To,
      Recipient_Cc,
      Recipient_Bcc,
      Recipient_Newsgroup,
      Recipient_Fcc,
      Recipient_None,
      Recipient_Max
   };

   /** @name Different ways to create a new composer window */
   //@{

   /** Constructor for posting news.
       @param profile parent profile
       @return pointer to the new compose view
    */
   static Composer *CreateNewArticle(const MailFolder::Params& params,
                                     Profile *profile = NULL);

   /// short cut
   static Composer *CreateNewArticle(Profile *profile = NULL)
      { return CreateNewArticle(MailFolder::Params(), profile); }

   /** Constructor for sending a follow-up to newsgroup.

       @param profile parent profile
       @param original message that we replied to
       @return pointer to the new compose view
    */
   static Composer *CreateFollowUpArticle(const MailFolder::Params& params,
                                          Profile *profile,
                                          Message * original = NULL);

   /** Constructor for sending mail.
       @param profile parent profile
       @return pointer to the new compose view
    */
   static Composer *CreateNewMessage(const MailFolder::Params& params,
                                     Profile *profile = NULL);

   /// short cut
   static Composer *CreateNewMessage(Profile *profile = NULL)
      { return CreateNewMessage(MailFolder::Params(), profile); }

   /** Constructor for sending a reply to a message.

       @param profile parent profile
       @param original message that we replied to
       @return pointer to the new compose view
    */
   static Composer *CreateReplyMessage(const MailFolder::Params& params,
                                       Profile *profile,
                                       Message * original = NULL);

   /** Constructor for forwarding a message.

       @param templ is the template to use
       @param profile parent profile
       @return pointer to the new compose view
    */
   static Composer *CreateFwdMessage(const MailFolder::Params& params,
                                     Profile *profile,
                                     Message *original = NULL);

   /**
     Create a composer window initialized with an existing message.

     @param profile the profile to use for the new composer
     @param msg the message to edit
     @return pointer to the new compose view
    */
   static Composer *EditMessage(Profile *profile, Message *message);

   //@}

   /**
      @name Other static methods
    */
   //@{

   /**
     Save contents of all opened composer windows

     @return the number of the windows closed or -1 on error
   */
   static int SaveAll();

   /// restore any previously saved window
   static bool RestoreAll();

   //@}

   /** @name Accessing composer data */
   //@{

   /// get (all) addresses of this type as a single string
   virtual String GetRecipients(RecipientType type) const = 0;

   /// get the currently entered subject
   virtual String GetSubject() const = 0;

   //@}

   /** @name Set the composer headers */
   //@{

   /// sets the "From" header
   virtual void SetFrom(const String& from) = 0;

   /// Set the default value for the "From" header (if we have it)
   virtual void SetDefaultFrom() = 0;

   /// sets Subject field
   virtual void SetSubject(const String& subj) = 0;

   /// adds recipients from addr (Recipient_Max means to reuse the last)
   virtual void AddRecipients(const String& addr,
                              RecipientType rcptType = Recipient_Max) = 0;

   /// adds a "To" recipient
   void AddTo(const String& addr) { AddRecipients(addr, Recipient_To); }

   /// adds a "Cc" recipient
   void AddCc(const String& addr) { AddRecipients(addr, Recipient_Cc); }

   /// adds a "Bcc" recipient
   void AddBcc(const String& addr) { AddRecipients(addr, Recipient_Bcc); }

   /// adds a "Fcc" recipient
   void AddFcc(const String& fcc) { AddRecipients(fcc, Recipient_Fcc); }

   /// adds a Newsgroup
   void AddNewsgroup(const String& addr) { AddRecipients(addr, Recipient_Newsgroup); }

   /** Sets the address fields, To:, CC: and BCC:.
       @param To primary address to send mail to
       @param CC carbon copy addresses
       @param BCC blind carbon copy addresses
   */
   void SetAddresses(const String& to,
                     const String& cc = _T(""),
                     const String& bcc = _T(""))
   {
      AddRecipients(to, Recipient_To);
      AddRecipients(cc, Recipient_Cc);
      AddRecipients(bcc, Recipient_Bcc);
   }

   /** Adds an extra header line.
       @param entry name of header entry
       @param value value of header entry
   */
   virtual void AddHeaderEntry(const String& entry, const String& value) = 0;

   /**
      Expand the given string: this method handles address book completion and
      mailto: URLs parsing. It modifies the string passed to it in place.

      @param text the text to expand, modified by the method
      @return the inferred recipient type or Recipient_None if invalid address
   */
   virtual RecipientType ExpandRecipient(String *text) = 0;

   //@}

   /** @name Add/insert stuff into composer */
   //@{

   /** Initializes the composer text: for example, if this is a reply, inserts
       the quoted contents of the message being replied to (except that, in
       fact, it may do whatever the user configured it to do using templates).
       The msg parameter may be NULL only for the new messages, messages
       created with CreateReply/FwdMessage require it to be !NULL.

       The msgview parameter allows to include only the selected text in the
       reply if the user configured the program to behave like this.

       @param msg the message we're replying to or forwarding
       @param msgview the message viewer to query for selection
    */
   virtual void InitText(Message *msg = NULL,
                         const MessageView *msgview = NULL) = 0;

   /** Finishes the composer initialization and shows the composer frame,
       should be called after all calls to InitText()
   */
   virtual void Launch() = 0;

   /** insert a file into buffer
       @param filename file to insert (ask the user if NULL)
       @param mimetype mimetype to use (auto detect if NULL)
       @param num_mimetype numeric mimetype
    */
   virtual void InsertFile(const wxChar *filename = NULL,
                           const wxChar *mimetype = NULL) = 0;

   /** Insert MIME content data
       @param data pointer to data (we will free() it later)
       @param len length of data
       @param mimetype mimetype to use
       @param name optional name to add in the content-type
       @param filename optional filename to add to list of parameters
    */
   virtual void InsertData(void *data,
                           size_t length,
                           const wxChar *mimetype = NULL,
                           const wxChar *name     = NULL,
                           const wxChar *filename = NULL) = 0;

   /// inserts a text
   virtual void InsertText(const String& txt) = 0;

   /// insert (recursively) a MIME part
   virtual void InsertMimePart(const MimePart *mimePart) = 0;

   /// move the cursor to the given position
   virtual void MoveCursorTo(int x, int y) = 0;

   /// move the cursor to the right and downwards by given number of characters
   virtual void MoveCursorBy(int x, int y) = 0;

   /// reset the "dirty" flag
   virtual void ResetDirty() = 0;

   /// make the composer dirty forcefully
   virtual void SetDirty() = 0;

   //@}

   /** @name Implementation only */
   //@{

   /// get the real private composer class
   virtual wxComposeView *GetComposeView() = 0;

   /// get the parent frame for the composer window
   virtual wxFrame *GetFrame() = 0;

   //@}

protected:
   typedef ComposerOptions Options;

   /// virtual dtor
   virtual ~Composer() { }

private:
   /**
     @name MessageEditor callbacks

     These functions are called by MessageEditor only and shouldn't be used
     from any outside code.
   */
   //@{

   /// called when composer window gets focus for the 1st time
   virtual bool OnFirstTimeFocus() = 0;

   /// called just before text in composer is modified for the 1st time
   virtual void OnFirstTimeModify() = 0;

   /// get the profile to use for options (NOT IncRef()'d!)
   virtual Profile *GetProfile() const = 0;

   /// get the options
   virtual const Options& GetOptions() const = 0;

   //@}

   friend class MessageEditor;
};

#endif // _COMPOSER_H_

