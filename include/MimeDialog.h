///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   MimeDialog.h
// Purpose:     functions for showing MIME-related dialogs
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2004-09-27
// CVS-ID:      $Id$
// Copyright:   (c) 2001-2004 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _M_MIMEDIALOG_H_
#define _M_MIMEDIALOG_H_

class wxWindow;

/**
   Ask the user for a command to be used to open an attachment of the given
   MIME type.

   @param parent the parent window for the dialog
   @param mimetype the MIME type, as string (e.g. "APPLICATION/OCTET-STREAM")
   @param openAsMsg if this parameter is not NULL, an additional checkbox
                    allowing the user to open the attachment as a message is
                    shown and it is filled with its value (true if the user
                    chose to open attachment as a message)
   @return string containing the command (with a single "%s" format specifier
           which should be replaced with a command name) or empty string if
           user cancelled the dialog
 */
extern String
GetCommandForMimeType(wxWindow *parent,
                      const String& mimetype,
                      bool *openAsMsg = NULL);

#endif //  _M_MIMEDIALOG_H_

