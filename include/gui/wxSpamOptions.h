///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   gui/SpamOptions.h - Hooks for wxFilterDialogs.cpp
// Purpose:     Splits spam options off wxFilterDialogs.cpp
// Author:      Robert Vazan
// Modified by:
// Created:     05 September 2003
// CVS-ID:      $Id$
// Copyright:   (c) 2003 Mahogany Team
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _WXSPAMOPTIONS_H_
#define _WXSPAMOPTIONS_H_

#ifdef __GNUG__
   #pragma interface "wxSpamOptions.h"
#endif

#include "MObject.h"
#include "pointers.h"

DECLARE_REF_COUNTER(SpamOptionManager)

class SpamOptionManager : public MObjectRC
{
public:
   virtual ~SpamOptionManager() {};
   
   virtual void FromString(const String &source) = 0;
   virtual String ToString() = 0;
   
   virtual bool ShowDialog(wxFrame *parent) = 0;
   
   static RefCounter<SpamOptionManager> Create();
};

#endif // _WXSPAMOPTIONS_H_
