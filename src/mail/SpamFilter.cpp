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
   #include "gui/wxIconManager.h"  // for GetBitmap
   #include "pointers.h"           // for RefCounter
   #include "strutil.h"            // for strutil_restore_array
   #include "Profile.h"            // for Profile_obj
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
   // we must have a non-NULL profile but we don't Inc/DecRef() it at all
   SpamOptionsDialog(wxFrame *parent,
                     Profile *profile,
                     String *params,
                     wxImageList *imagelist)
      : wxOptionsEditDialog(parent,
                            _("Spam filters options"),
                            _T("SpamDlg"))
   {
      m_profile = profile;
      m_params = params;
      m_imagelist = imagelist;

      CreateAllControls();
      Fit();
      Layout();
   }

   virtual void CreateNotebook(wxPanel *panel)
   {
      m_notebook = new wxPNotebook(_T("SpamOptions"), panel);
      if ( m_imagelist )
         m_notebook->AssignImageList(m_imagelist);

      // now we can put the pages into it
      for ( SpamFilter *p = SpamFilter::ms_first; p; p = p->m_next )
      {
         p->CreateOptionPage(m_notebook, m_profile, m_params);
      }
   }

protected:
   virtual Profile *GetProfile() const { return m_profile; }

private:
   Profile *m_profile;
   String *m_params;
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
                        const String& paramsAll,
                        String *result)
{
   LoadAll();

   // break down the parameters (if we have any) into names and values
   wxSortedArrayString names;
   wxArrayString values;
   if ( !paramsAll.empty() )
   {
      // hack: for backwards compatibility with older versions when there was
      // only one spam filter
      String paramsAllReal(paramsAll.Find(_T(';')) == wxNOT_FOUND
                              ? _T("headers=") + paramsAll + _T(";dspam=")
                              : paramsAll);

      wxArrayString params(strutil_restore_array(paramsAllReal, _T(';')));
      const size_t count = params.GetCount();
      for ( size_t n = 0; n < count; n++ )
      {
         const wxString& param = params[n];
         int pos = param.Find(_T('='));
         if ( pos == wxNOT_FOUND )
         {
            wxLogDebug(_T("Bogus spam function argument \"%s\" ignored."),
                       param.c_str());
            continue;
         }

         // insert the value in the same position in values array as name is
         // going to have in the names one (as it's sorted we don't know where
         // will it be)
         values.Insert(param.c_str() + pos + 1, names.Add(wxString(param, pos)));
      }
   }


   // now try all filters in turn until one of them returns true
   for ( SpamFilter *p = ms_first; p; p = p->m_next )
   {
      const String& name(p->GetName());

      String param;
      if ( !paramsAll.empty() )
      {
         int n = names.Index(name);
         if ( n == wxNOT_FOUND )
         {
            // skip filters not appearing in paramsAll
            continue;
         }

         param = values[(size_t)n];
      }
      //else: use all filters

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
bool SpamFilter::Configure(wxFrame *parent, String *params)
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

   // check if we have anything to configure
   if ( !nPages )
   {
      wxLogMessage(_("There are no configurable spam filters."));
      return false;
   }


   // now we must select a profile from which the filters options will be read
   // from and written to: this can be either the "real" profile or a temporary
   // one populated with the value of *params and deleted after copying all the
   // changes back to *params
   Profile_obj profile(Profile::CreateModuleProfile(SPAM_FILTER_INTERFACE));
   if ( params )
   {
      // ensure that we can undo any changes later by calling Discard()
      profile->Suspend();
   }

   // do create the dialog (this will create all the pages inside its
   // CreateNotebook()) and show it
   SpamOptionsDialog dlg(parent, profile, params, imagelist);

   bool ok = dlg.ShowModal() == wxID_OK;

   if ( params )
   {
      profile->Discard();
   }

   return ok;
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

// ----------------------------------------------------------------------------
// trivial implementations of the base class "almost pure" virtuals
// ----------------------------------------------------------------------------

SpamOptionsPage *
SpamFilter::CreateOptionPage(wxListOrNoteBook * /* notebook */,
                             Profile * /* profile */,
                             String * /* params */) const
{
   return NULL;
}

SpamFilter::~SpamFilter()
{
}

