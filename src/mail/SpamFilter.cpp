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
#include <wx/persist/bookctrl.h>

#include "MAtExit.h"

#include "gui/wxOptionsPage.h"
#include "gui/wxOptionsDlg.h"

#include "Message.h"

#include "SpamFilter.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// separator used between filters in is_spam() arguments
static const wxChar FILTERS_SEPARATOR = _T(';');

// ----------------------------------------------------------------------------
// local functions
// ----------------------------------------------------------------------------

namespace
{

/**
   Return the name in which the status (enabled or disabled) of the given
   filter is stored in the config.
 */
inline String GetSpamFilterKeyName(const char *name)
{
   return String::Format("UseSpamFilter_%s", name);
}

/**
   Return true if the given filter is enabled in the specified profile.
 */
inline bool IsSpamFilterEnabled(const Profile *profile, const char *name)
{
   return profile->readEntry(GetSpamFilterKeyName(name), true);
}

/**
   Enable or disable the given spam filter.
 */
inline void EnableSpamFilter(Profile *profile, const char *name, bool enable)
{
   profile->writeEntry(GetSpamFilterKeyName(name), enable);
}

} // anonymous namespace

// ----------------------------------------------------------------------------
// local classes
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
      m_chkEnable = NULL;

      CreateAllControls();
      Fit();
      Layout();
   }

   virtual wxControl *CreateControlsAbove(wxPanel *panel)
   {
      wxControl *last = NULL;

      last = CreateMessage(panel,
            _("This dialog is used to configure the options of the individual "
              "spam filters and\n"
              "to select whether they are used for the \"Message|Spam\" menu "
              "commands.\n"
              "\n"
              "Notice that you still need to configure a filter rule with a "
              "\"Check if spam\" test\n"
              "to actually use the spam filters configured here.\n"), last);

      m_chkEnable = CreateCheckBox(panel,
                        _("&Use the selected filter"),
                        -1, // no max width as we have just one field
                        last);
      last = m_chkEnable;

      Connect(wxEVT_COMMAND_CHECKBOX_CLICKED,
               wxCommandEventHandler(SpamOptionsDialog::OnCheckBox));

      return last;
   }

   virtual void CreateNotebook(wxPanel *panel)
   {
      m_notebook = new MBookCtrl(panel, wxID_ANY);
      Connect(M_EVT_BOOK_PAGE_CHANGED,
               MBookEventHandler(SpamOptionsDialog::OnPageChanged));

      if ( m_imagelist )
         m_notebook->AssignImageList(m_imagelist);

      // now we can put the pages into it
      for ( SpamFilter *p = SpamFilter::ms_first; p; p = p->m_next )
      {
         // update internal data first so that our OnPageChanged() handler
         // could use it when it's called from CreateOptionsPage() below
         const String name = p->GetName();
         m_used.push_back(IsSpamFilterEnabled(m_profile, name));
         m_names.push_back(name);

         p->CreateOptionPage(m_notebook, m_profile);
      }

      wxPersistentRegisterAndRestore(m_notebook, "SpamOptions");
   }

   virtual bool TransferDataFromWindow()
   {
      if ( !wxOptionsEditDialog::TransferDataFromWindow() )
         return false;

      // update the data in the config
      const size_t count = m_used.size();
      for ( size_t n = 0; n < count; n++ )
      {
         const String& name = m_names[n];
         const bool used = m_used[n] != false;
         if ( IsSpamFilterEnabled(m_profile, name) != used )
            EnableSpamFilter(m_profile, name, used);
      }

      return true;
   }

protected:
   virtual Profile *GetProfile() const
   {
      SafeIncRef(m_profile);
      return m_profile;
   }

private:
   // update the internal data from checkbox value
   void OnCheckBox(wxCommandEvent& event)
   {
      if ( event.GetEventObject() != m_chkEnable )
         return;

      const int sel = m_notebook->GetSelection();
      CHECK_RET( sel != wxNOT_FOUND, "no active page?" );

      m_used[sel] = m_chkEnable->GetValue();

      SetDirty();
   }

   // update the checkbox from internal data
   void OnPageChanged(wxNotebookEvent& event)
   {
      const int sel = event.GetSelection();
      if ( sel != wxNOT_FOUND )
         m_chkEnable->SetValue(m_used[sel] != false);
   }


   Profile *m_profile;
   wxImageList *m_imagelist;
   wxCheckBox *m_chkEnable;

   // the names of the spam filters, indexed by their order in the notebook
   wxArrayString m_names;

   // the boolean value of the checkbox indicating whether the filter should be
   // used for each of the pages, indexed by their order in the notebook
   wxArrayInt m_used;
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

/* static */
Profile *SpamFilter::GetProfile(const Message& msg)
{
   Profile *profile = msg.GetProfile();
   if ( !profile )
      profile = mApplication->GetProfile();

   ASSERT_MSG( profile, "should have some profile to use" );

   return profile;
}

// ----------------------------------------------------------------------------
// forward the real operations to all filters in the spam chain
// ----------------------------------------------------------------------------

/* static */
bool SpamFilter::Reclassify(const Message& msg, bool isSpam)
{
   Profile * const profile = GetProfile(msg);
   if ( !profile )
      return false;

   LoadAll();

   bool rc = true;
   for ( SpamFilter *p = ms_first; p; p = p->m_next )
   {
      if ( IsSpamFilterEnabled(profile, p->GetName()) )
      {
         if ( !p->DoReclassify(profile, msg, isSpam) )
            rc = false;
      }
   }

   return rc;
}

/* static */
void SpamFilter::Train(const Message& msg, bool isSpam)
{
   Profile * const profile = GetProfile(msg);
   if ( !profile )
      return;

   LoadAll();

   for ( SpamFilter *p = ms_first; p; p = p->m_next )
   {
      if ( IsSpamFilterEnabled(profile, p->GetName()) )
         p->DoTrain(profile, msg, isSpam);
   }
}

/* static */
bool
SpamFilter::CheckIfSpam(const Message& msg,
                        const String& paramsAll,
                        String *result)
{
   Profile * const profile = GetProfile(msg);
   if ( !profile )
      return false;

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
      const String name(p->GetName());

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
      else // use only configured filters
      {
         if ( !IsSpamFilterEnabled(profile, p->GetName()) )
            continue;
      }

      // DoCheckIfSpam() may return -1 in addition to true or false which is
      // treated as "definitively false", i.e. not only this spam filter didn't
      // recognize this message as spam but it shouldn't be even checked with
      // the others (this is mainly used for whitelisting support)
      switch ( p->DoCheckIfSpam(profile, msg, param, result) )
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
bool SpamFilter::Configure(Profile *profile, wxFrame *parent)
{
   LoadAll();

   // first get all the icon names: we need them to create the notebook
   wxImageList *imagelist = wxNotebookWithImages::ShouldShowIcons()
                              ? new wxImageList(32, 32)
                              : NULL;

   size_t nPages = 0;
   for ( SpamFilter *p = ms_first; p; p = p->m_next )
   {
      const char * const iconname = p->GetOptionPageIconName();
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


   // do create the dialog (this will create all the pages inside its
   // CreateNotebook()) and show it
   SpamOptionsDialog dlg(parent, profile, imagelist);

   return dlg.ShowModal() == wxID_OK;
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
SpamFilter::CreateOptionPage(MBookCtrl * /* notebook */,
                             Profile * /* profile */) const
{
   return NULL;
}

SpamFilter::~SpamFilter()
{
}

