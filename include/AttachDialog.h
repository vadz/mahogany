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
  Show the dialog allowing the user to edit the attachment properties. It can
  be shown either automatically when the user attaches the file in which case
  allowDisable should be pointing to a bool var with value == true which will
  be set to false by this function if the user chose to disable showing this
  dialog; or when the user clicks on the attachment in which case allowDisable
  should be NULL.

  @param parent the parent for the dialog
  @param properties to edit, can't be NULL
  @param allowDisable if true, the dialog will have "don't show again" checkbox
  @return true if the user has changed something, false otherwise
 */
extern bool ShowAttachmentDialog(wxWindow *parent,
                                 AttachmentProperties *properties,
                                 bool *allowDisable = NULL);

#endif // _ATTACHDIALOG_H_
