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

/** Show the dialog to allow the user change a value for a custom header,
    returns the header name and value in output variables.

    Also remembers if the user wants this header to always have this value - in
    this case, the header name/value are remembered in the "Custom header"
    subgroup of the profile object.

    @return true if Ok button was pressed, false otherwise
 */
extern bool ConfigureCustomHeader(ProfileBase *profile, wxWindow *parent,
                                  String *headerName, String *headerValue,
                                  bool *storedInProfile = (bool *)NULL);

#endif // _HEADERDIALOGS_H
