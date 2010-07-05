///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   interface/HeaderInfo.i: HeaderInfo and HeaderInfoList interface
// Purpose:     exports HeaderInfo and HeaderInfoList classes
// Author:      Vadim Zeitlin
// Modified by:
// Created:     28.12.03
// CVS-ID:      $Id$
// Copyright:   (c) 2003 Vadim Zeitlin
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

%module HeaderInfo
%{
#include "Mcommon.h"
#include "HeaderInfo.h"
%}

%include "swigcmn.i"

%ignore HeaderInfoList::operator[];
%include "HeaderInfo.h"

