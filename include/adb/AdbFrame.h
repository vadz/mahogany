///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   adb/AdbFrame.h - ADB edit frame interface
// Purpose:     we provide a function to create and show the ADB editor frame
// Author:      Vadim Zeitlin
// Modified by: 
// Created:     09.08.98
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef   _ADBFRAME_H
#define   _ADBFRAME_H

class wxFrame;

/// create the ADB frame if it doesn't exist and show it
void ShowAdbFrame(wxFrame *parent);

#endif  //_ADBFRAME_H
