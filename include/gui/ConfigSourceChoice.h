///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   gui/ConfigSourceChoice.h
// Purpose:     Declaration of ConfigSourceChoice class
// Author:      Vadim Zeitlin
// Created:     2008-05-23
// CVS-ID:      $Id$
// Copyright:   (c) 2008 Vadim Zeitlin <vadim@wxwidgets.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef M_GUI_CONFIGSOURCE_H
#define M_GUI_CONFIGSOURCE_H

#include "ConfigSource.h"

#include <wx/choice.h>

/**
   A choice control allowing the user to select the config source to use.

   It provides a convenient creation method and direct access to the selected
   config source.
 */
class ConfigSourceChoice : public wxChoice
{
public:
   /**
      Create a choice control containing all the config sources.

      This is useful for option editing dialogs which need to allow the user to
      choose the config source where the changes should be saved.
      
      If there are no config sources defined, returns NULL. Otherwise returns
      the choice control with their names and positions it in the bottom right
      corner of the @a parent offset by the margin plus @a hExtra from the
      bottom. As the bottom row is normally taken by the buttons, the usual
      value for hExtra will be wxManuallyLaidOutDialog::hBtn.
    */
   static ConfigSourceChoice *Create(wxWindow *parent, int hExtra);

   /**
      Return the selected config source.

      May return NULL if there is no selection.
    */
   ConfigSource *GetSelectedSource() const;

private:
   // ctor is private, we're only created by Create()
   ConfigSourceChoice(wxWindow *parent) : wxChoice(parent, wxID_ANY) { }
};

#endif // M_GUI_CONFIGSOURCE_H
