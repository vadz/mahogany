///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   HeadersDialogs.h - dialogs to configure headers
// Purpose:     utility dialogs used from various places
// Author:      Vadim Zeitlin
// Modified by:
// Created:     14.04.99
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _HEADERDIALOGS_H
#define _HEADERDIALOGS_H

class wxWindow;

/** Show the dialog to configure outgoing headers for given profile

    @return true if Ok button was pressed, false otherwise
 */
extern bool ConfigureComposeHeaders(ProfileBase *profile, wxWindow *parent);

/** Show the dialog to configure message view headers for given profile

    @return true if Ok button was pressed, false otherwise
 */
extern bool ConfigureMsgViewHeaders(ProfileBase *profile, wxWindow *parent);

#endif // _HEADERDIALOGS_H
