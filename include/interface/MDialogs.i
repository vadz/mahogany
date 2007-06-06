///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   interface/MDialogs.i: interface file for dialog functions
// Purpose:     functions here can be used from Python to interface with user
// Author:      Vadim Zeitlin
// Modified by:
// Created:     19.12.03
// CVS-ID:      $Id$
// Copyright:   (c) 2003 Vadim Zeitlin
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

%module MDialogs
%{
#include "Mcommon.h"
#include "gui/wxMDialogs.h"
%}

%inline
%{
bool Message(const char *message)
{
   return MDialog_Message(wxString(message), NULL);
}

void Status(const char *message)
{
   wxLogStatus(_T("%s"), message);
}
%}

