///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   src/mail/SpamFilter.cpp
// Purpose:     implements static methods of SpamFilter
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2004-07-04
// CVS-ID:      $Id$
// Copyright:   (c) 2004 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M licence
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
   #include "Mcommon.h"
#endif // USE_PCH

#include <wx/imaglist.h>

#include "MAtExit.h"

#include "gui/wxOptionsPage.h"
#include "gui/wxOptionsDlg.h"

#include "SpamFilter.h"

// ----------------------------------------------------------------------------
// local types
// ----------------------------------------------------------------------------

class SpamOptionsDialog : public wxOptionsEditDialog
{
public:
   SpamOptionsDialog(wxFrame *parent, wxImageList *imagelist)
      : wxOptionsEditDialog(parent,
                            _("Spam filters options"),
                            _T("SpamDlg"))
   {
      m_imagelist = imagelist;

      CreateAllControls();
      Layout();
   }

   virtual void CreateNotebook(wxPanel *panel)
   {
      m_notebook = new wxPNotebook(_T("SpamOptions"), panel);
      if ( m_imagelist )
         m_notebook->SetImageList(m_imagelist);

      // now we can put the pages into it
      for ( SpamFilter *p = SpamFilter::ms_first; p; p = p->m_next )
      {
         p->CreateOptionPage(m_notebook);
      }
   }

protected:
   virtual Profile *GetProfile() const { return mApplication->GetProfile(); }

private:
   wxImageList *m_imagelist;
};

// ----------------------------------------------------------------------------
// local globals
// ----------------------------------------------------------------------------

// ensure that filters are cleaned up on exit
static MRunFunctionAtExit gs_runFilterCleanup(SpamFilter::UnloadAll);

// ============================================================================
// SpamFilter implementation
// ============================================================================

SpamFilter *SpamFilter::ms_first = NULL;
bool SpamFilter::ms_loaded = false;

// ----------------------------------------------------------------------------
// forward the real operations to all filters in the spam chain
// ----------------------------------------------------------------------------

/* static */
void SpamFilter::Reclassify(const Message& msg, bool isSpam)
{
   LoadAll();

   for ( SpamFilter *p = ms_first; p; p = p->m_next )
   {
      p->DoReclassify(msg, isSpam);
   }
}

/* static */
void SpamFilter::Train(const Message& msg, bool isSpam)
{
   LoadAll();

   for ( SpamFilter *p = ms_first; p; p = p->m_next )
   {
      p->DoTrain(msg, isSpam);
   }
}

/* static */
bool
SpamFilter::CheckIfSpam(const Message& msg,
                        const wxArrayString& params,
                        String *result)
{
   LoadAll();

   // break down the parameters into names and parameters
   wxSortedArrayString names;
   wxArrayString values;
   const size_t count = params.GetCount();
   for ( size_t n = 0; n < count; n++ )
   {
      wxString param = params[n];
      int pos = param.Find(_T('='));
      if ( pos == wxNOT_FOUND )
      {
         // hack: for backwards compatibility with older versions when there
         // was only one spam filter
         param = _T("headers=") + param;
         pos = 7;
      }

      // insert the value in the same position in values array as name is going
      // to have in the names one (as it's sorted we don't know where will it
      // be)
      values.Insert(param.c_str() + pos + 1, names.Add(wxString(param, pos)));
   }


   // now try all filters in turn until one of them returns true
   for ( SpamFilter *p = ms_first; p; p = p->m_next )
   {
      const String& name(p->GetName());

      int n = names.Index(name);
      String param;
      if ( n != wxNOT_FOUND )
         param = values[(size_t)n];

      if ( p->DoCheckIfSpam(msg, param, result) )
      {
         if ( result )
            *result = name + _T(": ") + *result;

         return true;
      }
   }

   return false;
}

// ----------------------------------------------------------------------------
// spam filters configuration
// ----------------------------------------------------------------------------

/* static */
void SpamFilter::Configure(wxFrame *parent)
{
   LoadAll();

   // first get all the icon names: we need them to create the notebook
   wxImageList *imagelist = wxNotebookWithImages::ShouldShowIcons()
                              ? new wxImageList(32, 32)
                              : NULL;

   size_t nPages = 0;
   for ( SpamFilter *p = ms_first; p; p = p->m_next )
   {
      const wxChar *iconname = p->GetOptionPageIconName();
      if ( iconname )
      {
         nPages++;
         if ( imagelist )
            imagelist->Add(mApplication->GetIconManager()->GetBitmap(iconname));
      }
   }

   if ( !nPages )
   {
      wxLogMessage(_("There are no configurable spam filters."));
      return;
   }

   // do create the dialog and show it
   SpamOptionsDialog dlg(parent, imagelist);

   (void)dlg.ShowModal();
}

// ----------------------------------------------------------------------------
// loading/unloading spam filters
// ----------------------------------------------------------------------------

/* static */
void SpamFilter::DoLoadAll()
{
   ms_loaded = true;

   // load the listing of all available spam filters
   RefCounter<MModuleListing>
      listing(MModule::ListAvailableModules(SPAM_FILTER_INTERFACE));
   if ( !listing )
      return;

   // found some, now create them
   const size_t count = listing->Count();
   for ( size_t n = 0; n < count; n++ )
   {
      const MModuleListingEntry& entry = (*listing)[n];
      const String& name = entry.GetName();
      MModule * const module = MModule::LoadModule(name);
      if ( !module )
      {
         wxLogError(_("Failed to load spam filter \"%s\"."), name.c_str());
         continue;
      }

      SpamFilterFactory * const
         factory = static_cast<SpamFilterFactory *>(module);

      // create the filter and insert it into the linked list ordered by cost
      SpamFilter * const filterNew = factory->Create();

      const unsigned int cost = filterNew->GetCost();
      SpamFilter **next = &ms_first;
      for ( SpamFilter *filter = ms_first; ; filter = filter->m_next )
      {
         // insert it here?
         if ( !filter || cost < filter->GetCost() )
         {
            // yes, adjust the links
            *next = filterNew;

            filterNew->m_next = filter;

            break;
         }

         next = &filter->m_next;
      }

      factory->DecRef();
   }
}

/* static */
void SpamFilter::UnloadAll()
{
   if ( ms_loaded )
   {
      // free all currently loaded filters
      while ( ms_first )
      {
         SpamFilter *p = ms_first;
         ms_first = p->m_next;

         p->DecRef();
      }

      ms_loaded = false;
   }
}

