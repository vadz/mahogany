///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   Python/MDialogs.i: some common exported dialog functions
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
#include "interface/MDialogs.h"
using namespace MDialogs;
%}
bool Message(const char *message);
void Status(const char *message);
