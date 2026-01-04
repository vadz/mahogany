///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   gui/ConfigSourceChoice.cpp
// Purpose:     Implementation of ConfigSourceChoice class
// Author:      Vadim Zeitlin
// Created:     2008-05-23
// CVS-ID:      $Id$
// Copyright:   (c) 2008 Vadim Zeitlin <vadim@wxwidgets.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
#  include "guidef.h"
#endif

#include "ConfigSourcesAll.h"

#include "gui/ConfigSourceChoice.h"

#include <wx/layout.h>
#include <wx/statline.h>
#include <wx/stattext.h>

// ============================================================================
// ConfigSourceChoice implementation
// ============================================================================

/* static */
ConfigSourceChoice *
ConfigSourceChoice::Create(wxWindow *parent, int hExtra)
{
   const AllConfigSources::List& sources = AllConfigSources::Get().GetSources();
   if ( sources.size() == 1 )
      return NULL;

   ConfigSourceChoice * const chcSources = new ConfigSourceChoice(parent);

   for ( AllConfigSources::List::iterator i = sources.begin(),
                                        end = sources.end();
         i != end;
         ++i )
   {
      chcSources->Append(i->GetName());
   }

   wxLayoutConstraints *c;

   c = new wxLayoutConstraints;
   c->right.SameAs(parent, wxRight, LAYOUT_X_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   c->bottom.SameAs(parent, wxBottom, 5*LAYOUT_Y_MARGIN + hExtra);
   chcSources->SetConstraints(c);

   wxStaticText *label = new wxStaticText(parent, -1, _("&Save changes to:"));
   c = new wxLayoutConstraints;
   c->right.LeftOf(chcSources, LAYOUT_X_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   c->centreY.SameAs(chcSources, wxCentreY);
   label->SetConstraints(c);

   wxStaticLine *line = new wxStaticLine(parent, -1);
   c = new wxLayoutConstraints;
   c->left.SameAs(parent, wxLeft, LAYOUT_X_MARGIN);
   c->right.SameAs(parent, wxRight, LAYOUT_X_MARGIN);
   c->height.AsIs();
   c->bottom.Above(chcSources, -LAYOUT_Y_MARGIN);
   line->SetConstraints(c);

   return chcSources;
}

ConfigSource *ConfigSourceChoice::GetSelectedSource() const
{
   const int sel = GetSelection();
   ConfigSource *config = NULL;
   if ( sel != wxNOT_FOUND )
   {
      AllConfigSources::List::iterator
         i = AllConfigSources::Get().GetSources().begin();
      for ( int n = 0; n < sel; n++ )
         ++i;

      config = i.operator->();
   }

   return config;
}
