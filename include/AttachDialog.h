///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   AttachDialog.h
// Purpose:     dialog allowing the user to edit the attachment properties
// Author:      Vadim Zeitlin
// Modified by:
// Created:     02.09.02
// CVS-ID:      $Id$
// Copyright:   (c) 2002 Vadim Zeitlin
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _ATTACHDIALOG_H_
#define _ATTACHDIALOG_H_

#include "MimeType.h"

class wxWindow;

/// AttachmentProperties: the structure describing all attachment properties
struct AttachmentProperties
{
   /// possible values for the disposition field
   enum Disposition
   {
      Disposition_Inline,
      Disposition_Attachment,
      Disposition_Max
   };

   /// filename on the local system
   String filename;

   /// the fllename in the Content-Disposition header of the message
   String name;

   /// the attachment disposition
   Disposition disposition;

   /// the MIME type
   MimeType mimetype;

   // TODO: add encoding (QP/Base64)? also, comment field?

   AttachmentProperties();

   /// set disposition field to correspond to the INLINE/ATTACHMENT string
   void SetDisposition(const String& disp);

   /// get the disposition as a string
   String GetDisposition() const;

   bool operator==(const AttachmentProperties& other) const
   {
      return filename == other.filename &&
             name == other.name &&
             disposition == other.disposition &&
             mimetype == other.mimetype;
   }

   bool operator!=(const AttachmentProperties& other) const
   {
      return !(*this == other);
   }
};

/**
   Show the dialog allowing the user to edit the attachment properties.

   This dialog is shown when the user chooses to edit the properties of an
   already existing attachment.

   @sa ShowAttachmentDialog()

   @param parent the parent for the dialog
   @param properties to edit, can't be NULL
   @return true if the user has changed something, false otherwise
 */
extern bool
EditAttachmentProperties(wxWindow *parent, AttachmentProperties *properties);

/**
   Show the dialog allowing the user to edit the attachment properties
   immediately after attaching the file.

   The difference between this function and EditAttachmentProperties() is the
   allowDisable parameter and different return value interpretation: if the
   user cancels the EditAttachmentProperties() dialog, nothing should be done
   while if he cancels this one, the attachment shouldn't be inserted at all as
   still inserting it after the user pressed cancel would be counterintuitive.

   @param parent the parent for the dialog
   @param properties to edit, can't be NULL
   @param dontShowAgain if set to true on return, the dialog shouldn't be shown
                        the next time any more (user chose to disable it)
   @return true if the user chose ok, false if the dialog was cancelled
 */
extern bool
ShowAttachmentDialog(wxWindow *parent,
                     AttachmentProperties *properties,
                     bool& dontShowAgain);

#endif // _ATTACHDIALOG_H_
