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
   #include <wx/frame.h>
#endif // USE_PCH

#include <wx/imaglist.h>

#include "MAtExit.h"

#include "gui/wxOptionsPage.h"
#include "gui/wxOptionsDlg.h"

#include "SpamFilter.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// separator used between filters in is_spam() arguments
static const wxChar FILTERS_SEPARATOR = _T(';');

// ----------------------------------------------------------------------------
// local types
// ----------------------------------------------------------------------------

class SpamOptionsDialog : public wxOptionsEditDialog
{
public:
   // we must have a non-NULL profile but we don't Inc/DecRef() it at all
   SpamOptionsDialog(wxFrame *parent,
                     Profile *profile,
                     wxImageList *imagelist)
      : wxOptionsEditDialog(parent,
                            _("Spam filters options"),
                            _T("SpamDlg"))
   {
      m_profile = profile;
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
         p->CreateOptionPage(m_notebook, m_profile);
      }
   }

protected:
   virtual Profile *GetProfile() const { return m_profile; }

private:
   Profile *m_profile;
   wxImageList *m_imagelist;
};

// ----------------------------------------------------------------------------
// local functions
// ----------------------------------------------------------------------------

// return the array containing "filter[=options]" strings from a string
// containing all filters separated by FILTERS_SEPARATOR
static wxArrayString SplitParams(const String& paramsAll)
{
   // backwards compatibility hack: for compatibility with older versions when
   // there was only one spam filter we treat "xxx:yyy:zzz" as arguments for
   // that filter but still add dspam
   String paramsAllReal;

   if ( paramsAll.Find(FILTERS_SEPARATOR) == wxNOT_FOUND &&
         paramsAll.Find(_T(':')) != wxNOT_FOUND )
   {
      paramsAllReal << _T("headers=") << paramsAll << _T(";dspam"); 
   }
   else if ( paramsAll.empty() )
   {
      // default value, use all
      paramsAllReal = _T("headers;dspam");
   }
   else // normal format already
   {
      paramsAllReal = paramsAll;
   }

   return strutil_restore_array(paramsAllReal, FILTERS_SEPARATOR);
}

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
// helpers
// ----------------------------------------------------------------------------

/* static */
SpamFilter *SpamFilter::FindByName(const String& name)
{
   for ( SpamFilter *filter = ms_first; filter; filter = filter->m_next )
   {
      if ( filter->GetName() == name )
         return filter;
   }

   return NULL;
}

/* static */
SpamFilter *SpamFilter::FindByLongName(const String& lname)
{
   for ( SpamFilter *filter = ms_first; filter; filter = filter->m_next )
   {
      if ( filter->GetLongName() == lname )
         return filter;
   }

   return NULL;
}

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
      wxArrayString params(SplitParams(paramsAll));
      const size_t count = params.GetCount();
      for ( size_t n = 0; n < count; n++ )
      {
         const wxString& param = params[n];
         int pos = param.Find(_T('='));

         wxString name,
                  val;
         if ( pos != wxNOT_FOUND )
         {
            name = wxString(param, pos);
            val = param.c_str() + pos + 1;
         }
         else // no value, use default options
         {
            name = param;
         }


         // insert the value in the same position in values array as name is
         // going to have in the names one (as it's sorted we don't know where
         // will it be)
         values.Insert(val, names.Add(name));
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

      // DoCheckIfSpam() may return -1 in addition to true or false which is
      // treated as "definitively false", i.e. not only this spam filter didn't
      // recognize this message as spam but it shouldn't be even checked with
      // the others (this is mainly used for whitelisting support)
      switch ( p->DoCheckIfSpam(msg, param, result) )
      {
         case true:
            if ( result )
            {
               *result = String::Format("recognized as spam by %s filter: %s",
                                        name.c_str(), result->c_str());
            }
            return true;

         case -1:
            if ( result )
            {
               *result = String::Format("recognized as non-spam by %s filter: %s",
                                        name.c_str(), result->c_str());
            }
            return false;

         default:
            FAIL_MSG( "unexpected DoCheckIfSpam() return value" );
            // fall through

         case false:
            // continue with the other filters checks
            break;
      }
   }

   return false;
}

// ----------------------------------------------------------------------------
// spam filters configuration
// ----------------------------------------------------------------------------

/* static */
bool SpamFilter::Configure(wxFrame *parent)
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
   // from and written to
   Profile_obj profile(Profile::CreateModuleProfile(SPAM_FILTER_INTERFACE));

   // do create the dialog (this will create all the pages inside its
   // CreateNotebook()) and show it
   SpamOptionsDialog dlg(parent, profile, imagelist);

   bool ok = dlg.ShowModal() == wxID_OK;

   return ok;
}

/* static */
bool SpamFilter::EditParameters(wxFrame *parent, String *params)
{
   CHECK( params, false, _T("NULL params parameter") );

   LoadAll();

   // prepare the filter descriptions for MDialog_GetSelectionsInOrder() call
   wxArrayString filters;
   wxArrayInt states;

   wxArrayString names = SplitParams(*params);
   const size_t countNames = names.GetCount();
   for ( size_t n = 0; n < countNames; n++ )
   {
      const String& name = names[n];

      // TODO: currently we allow only selecting filters order, not their
      //       parameters, in the GUI
      size_t posEq = name.find('=');
      if ( posEq != String::npos )
      {
         wxLogWarning(_("This filter rule should be edited directly as it "
                        "contains filter parameters not currently supported "
                        "by the GUI, sorry."));
         return false;
      }

      SpamFilter *filter = FindByName(name);
      if ( !filter )
      {
         wxLogDebug(_T("invalid filter name \"%s\" in isspam()"),
                    name.c_str());
         continue;
      }

      if ( filters.Index(filter->GetLongName()) != wxNOT_FOUND )
      {
         wxLogDebug(_T("duplicate filter name \"%s\" in isspam()"),
                    name.c_str());
         continue;
      }

      filters.Add(filter->GetLongName());
      states.Add(true);
   }

   for ( SpamFilter *filter = ms_first; filter; filter = filter->m_next )
   {
      if ( filters.Index(filter->GetLongName()) == wxNOT_FOUND )
      {
         filters.Add(filter->GetLongName());

         states.Add(false);
      }
      //else: already added above
   }

   if ( filters.IsEmpty() )
   {
      wxLogWarning(_("No spam filters available, nothing to configure."));
      return false;
   }

   // allow the user to select the filters he wants to use
   if ( !MDialog_GetSelectionsInOrder
         (
            _("Please select the spam filters to use for this rule:"),
            _("Spam Filters Selection"),
            &filters,
            &states,
            _T("SpamRuleParams"),
            parent
         ) )
   {
      return false;
   }

   // and now return a string with their names
   String paramsNew;
   const size_t countFilters = filters.GetCount();
   for ( size_t nFilter = 0; nFilter < countFilters; nFilter++ )
   {
      if ( states[nFilter] )
      {
         // we need to find its name from its long name
         SpamFilter *filter = FindByLongName(filters[nFilter]);
         CHECK( filter, false, _T("bogus filter long name") );

         if ( !paramsNew.empty() )
            paramsNew += FILTERS_SEPARATOR;
         paramsNew += filter->GetName();
      }
   }

   if ( paramsNew.empty() )
   {
      wxLogWarning(_("At least one spam filter should be used for this rule."));

      return false;
   }

   *params = paramsNew;

   return true;
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
                             Profile * /* profile */) const
{
   return NULL;
}

SpamFilter::~SpamFilter()
{
}

