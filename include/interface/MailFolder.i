///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   interface/MailFolder.i: MailFolder interface
// Purpose:     exports MailFolder class
// Author:      Vadim Zeitlin
// Modified by:
// Created:     28.12.03
// CVS-ID:      $Id$
// Copyright:   (c) 2003 Vadim Zeitlin
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

%module MailFolder
%{
#include "Mcommon.h"
#include "MailFolder.h"

// SWIG doesn't supported nested classes, this hack at least makes the
// generated C code compile -- but probably not work...
#define Params MailFolder::Params
%}

%import "swigcmn.i"

%include "MailFolder.h"

