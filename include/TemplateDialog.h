///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   TemplateDialog.h - dialog to configure compose templates
// Purpose:     these dialogs are used mainly by the options dialog, but may be
//              also used from elsewhere
// Author:      Vadim Zeitlin
// Modified by:
// Created:     16.07.99
// CVS-ID:      $Id$
// Copyright:   (c) 1999 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _TEMPLATEDIALOG_H
#define _TEMPLATEDIALOG_H

class wxWindow;

/** Show the dialog allowing the user to configure the message templates.

    @return true if something was changed, false otherwise
 */
extern bool ConfigureTemplates(ProfileBase *profile, wxWindow *parent);

#endif // _TEMPLATEDIALOG_H
