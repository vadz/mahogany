/*-*- c++ -*-********************************************************
 * wxAdbEdit.cc : a wxWindows look at a message                     *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$             *
 ********************************************************************
 * $Log$
 * Revision 1.8  1998/06/22 22:38:28  VZ
 * wxAdbEditFrame now uses wxNotebook (and works)
 *
 * Revision 1.7  1998/06/05 16:56:16  VZ
 *
 * many changes among which:
 *  1) AppBase class is now the same to MApplication as FrameBase to wxMFrame,
 *     i.e. there is wxMApp inheriting from AppBse and wxApp
 *  2) wxMLogFrame changed (but will probably change again because I wrote a
 *     generic wxLogFrame for wxWin2 - we can as well use it instead)
 *  3) Profile stuff simplified (but still seems to work :-), at least with
 *     wxConfig), no more AppProfile separate class.
 *  4) wxTab "#ifdef USE_WXWINDOWS2"'d out in wxAdbEdit.cc because not only
 *     it doesn't work with wxWin2, but also breaks wxClassInfo::Initialize
 *     Classes
 *  5) wxFTCanvas tweaked and now _almost_ works (but not quite)
 *  6) constraints in wxComposeView changed to work under both wxGTK and
 *     wxMSW (but there is an annoying warning about unsatisfied constraints
 *     coming from I don't know where)
 *  7) some more wxWin2 specific things corrected to avoid (some) crashes.
 *  8) many other minor changes I completely forgot about.
 *
 * Revision 1.6  1998/05/24 14:48:14  KB
 * lots of progress on Python, but cannot call functions yet
 * kbList fixes again?
 *
 * Revision 1.5  1998/05/18 17:48:36  KB
 * more list<>->kbList changes, fixes for wxXt, improved makefiles
 *
 * Revision 1.4  1998/05/15 21:59:33  VZ
 *
 * added 4th argument (id, unused in wxWin1) to CreateButton() calls
 *
 * Revision 1.3  1998/04/22 19:56:31  KB
 * Fixed _lots_ of problems introduced by Vadim's efforts to introduce
 * precompiled headers. Compiles and runs again under Linux/wxXt. Header
 * organisation is not optimal yet and needs further
 * cleanup. Reintroduced some older fixes which apparently got lost
 * before.
 *
 * Revision 1.2  1998/03/26 23:05:40  VZ
 * Necessary changes to make it compile under Windows (VC++ only)
 * Header reorganization to be able to use precompiled headers
 *
 * Revision 1.1  1998/03/14 12:21:21  karsten
 * first try at a complete archive
 *
 *******************************************************************/

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------
#ifdef __GNUG__
#pragma  implementation "wxAdbEdit.h"
#endif

#include "Mpch.h"
#include "Mcommon.h"

#ifndef  USE_PCH
#  include "MFrame.h"
#  include "MLogFrame.h"

#  include "kbList.h"
#  include "PathFinder.h"
#  include "MimeList.h"
#  include "MimeTypes.h"
#  include "Profile.h"

#  include "kbList.h"
#endif // USE_PCH

#include "Mdefaults.h"

#include "MApplication.h"
#include "gui/wxMApp.h"

#include "Adb.h"

#include "gui/wxAdbEdit.h"

#ifdef   USE_WXWINDOWS2
#  include <wx/notebook.h>
#else    // wxWin1
#  include "gui/wxtab.h"
#endif   // wxWin1/2

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// control ids
enum
{
   AdbEdit_ExpandBtn,
   AdbEdit_NewBtn,
   AdbEdit_DeleteBtn,
   AdbEdit_SaveBtn,
   AdbEdit_KeyText,
   AdbEdit_Notebook
};

enum
{
   TAB_NAME,
   TAB_HOME_ADDRESS,
   TAB_WORK_ADDRESS,
   TAB_EMAIL
};

// ----------------------------------------------------------------------------
// classes
// ----------------------------------------------------------------------------
class wxAdbEditPanel : public wxPanel
{
private:
   wxAdbEditFrame *frame;
   wxText         *key;

public:
   wxAdbEditPanel(wxAdbEditFrame *frame, wxWindow *parent,
                  int x = -1, int y = -1, int w = -1, int h = -1);

   void OnButtonPress(wxCommandEvent &event);
   void OnTextEnter(wxCommandEvent &event);


   // create a label and a text control in the panel and set up constraints to
   // put the label to the left of the text control and both of them beneath
   // top control. If !top, the controls are put at the top of the panel
   wxTextCtrl *LayoutTextWithLabel(const char *szLabel, wxTextCtrl *top);

   // put a row of buttons in the bottom right corner
   void wxAdbEditPanel::LayoutButtons(uint nButtons, wxButton *aButtons[]);

   DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(wxAdbEditFrame, wxMFrame)
   EVT_SIZE(wxAdbEditFrame::OnSize)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxAdbEditPanel, wxPanel)
   EVT_BUTTON(AdbEdit_ExpandBtn, wxAdbEditPanel::OnButtonPress)
   EVT_BUTTON(AdbEdit_NewBtn,    wxAdbEditPanel::OnButtonPress)
   EVT_BUTTON(AdbEdit_DeleteBtn, wxAdbEditPanel::OnButtonPress)
   EVT_BUTTON(AdbEdit_SaveBtn,   wxAdbEditPanel::OnButtonPress)

   EVT_TEXT_ENTER(AdbEdit_KeyText, wxAdbEditPanel::OnTextEnter)

   //EVT_NOTEBOOK_PAGE_CHANGED(AdbEdit_Notebook, wxAdbEditPanel::OnPageChange)
END_EVENT_TABLE()

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxAdbEditPanel
// ----------------------------------------------------------------------------

void wxAdbEditPanel::OnTextEnter(wxCommandEvent &event)
{
   String tmp = key->GetValue();
   int l = tmp.length();
   if( l > 0  &&  tmp.substr(l-1, 1) == " " )
   {
      key->SetValue(WXCPTR tmp.substr(0, l-1).c_str());
      frame->Load(key->GetValue()); 
   }
}

void
wxAdbEditPanel::OnButtonPress(wxCommandEvent &event)
{
   switch ( event.GetId() ) {
      case AdbEdit_ExpandBtn:
         frame->Load(key->GetValue()); 
         break;

      case AdbEdit_NewBtn:
         frame->New();
         break;

      case AdbEdit_SaveBtn:
         frame->Save();
         break;

      case AdbEdit_DeleteBtn:
         frame->Delete();
         break;

      default:
         FAIL_MSG("unknown button in wxAdbEditPanel::OnButtonPress");
   }
}


wxAdbEditPanel::wxAdbEditPanel(wxAdbEditFrame *frame_, wxWindow *parent,
                               int x, int y, int w, int h)
#ifdef USE_WXWINDOWS2
      : wxPanel(parent, -1, wxPoint(x, y), wxSize(w, h))
#else  // wxWin1
      : wxTabbedPanel(parent,x,y,w,h)
#endif // wxWin1/2
{
   frame = frame_;

   // create key text field with a label at the top
   wxMessage *nameLabel = CreateLabel(this, "Key:");
   key = GLOBAL_NEW wxTextCtrl(this, AdbEdit_KeyText, "",
                               wxDefaultPosition,
                               wxDefaultSize,
                               wxSUNKEN_BORDER);

   wxLayoutConstraints *c;
   c = new wxLayoutConstraints;
   c->top.SameAs(this, wxTop, 5);
   c->left.SameAs(this, wxLeft, 5);
   c->width.AsIs();
   c->height.AsIs();
   nameLabel->SetConstraints(c);

   c = new wxLayoutConstraints;
   c->top.SameAs(this, wxTop, 5);
   c->left.SameAs(nameLabel, wxRight, 5);
   c->right.SameAs(this, wxRight, 5);
   c->height.AsIs();
   key->SetConstraints(c);

   // create a row of buttons at the bottom (from right to left)
   wxButton *buttons[4];
   buttons[0] = CreateButton(this, "Expand", "expandButton", AdbEdit_ExpandBtn);
   buttons[1] = CreateButton(this, "New",    "newButton",    AdbEdit_NewBtn);
   buttons[2] = CreateButton(this, "Delete", "delButton",    AdbEdit_DeleteBtn);
   buttons[3] = CreateButton(this, "Save",   "saveButton",   AdbEdit_SaveBtn);

   LayoutButtons(WXSIZEOF(buttons), buttons);

   SetAutoLayout(TRUE);
}

wxTextCtrl *wxAdbEditPanel::LayoutTextWithLabel(const char *szLabel,
                                                wxTextCtrl *top)
{
   wxLayoutConstraints *c;
   wxMessage *label = CreateLabel(this, szLabel);
   wxTextCtrl *text = GLOBAL_NEW wxTextCtrl(this, -1, "", 
                                            wxDefaultPosition,
                                            wxDefaultSize,
                                            wxSUNKEN_BORDER);

   c = new wxLayoutConstraints;
   c->top.SameAs(top ? top : key, wxBottom, 5);
   c->left.SameAs(this, wxLeft, 5);
   c->width.AsIs();
   c->height.AsIs();
   label->SetConstraints(c);

   c = new wxLayoutConstraints;
   c->top.SameAs(top ? top : key, wxBottom, 5);
   c->left.SameAs(label, wxRight, 5);
   c->right.SameAs(this, wxRight, 5);
   c->height.AsIs();
   text->SetConstraints(c);

   return text;
}

void wxAdbEditPanel::LayoutButtons(uint nButtons, wxButton *aButtons[])
{
   wxLayoutConstraints *c;

   for ( uint n = 0; n < nButtons; n++ ) {
      c = new wxLayoutConstraints;
      if ( n == 0 )
         c->right.SameAs(this, wxRight, 5);
      else
         c->right.SameAs(aButtons[n - 1], wxLeft, 5);
      c->bottom.SameAs(this, wxBottom, 5);
      c->width.AsIs();
      c->height.AsIs();

      aButtons[n]->SetConstraints(c);
   }
}

// ----------------------------------------------------------------------------
// wxAdbEditFrame
// ----------------------------------------------------------------------------

// Panel 1 - name
void wxAdbEditFrame::CreateNamePage()
{
   wxAdbEditPanel *panel = GLOBAL_NEW wxAdbEditPanel(this, m_notebook);

   formattedName = panel->LayoutTextWithLabel("Formatted Name:", NULL);
   strNamePrefix = panel->LayoutTextWithLabel("Prefix:", formattedName);
   strNameFirst = panel->LayoutTextWithLabel("First", strNamePrefix);
   strNameOther = panel->LayoutTextWithLabel("Other:", strNameFirst);
   strNameFamily = panel->LayoutTextWithLabel("Family:", strNameOther);
   strNamePostfix = panel->LayoutTextWithLabel("Postfix:", strNameFamily);
   title = panel->LayoutTextWithLabel("Title:", strNamePostfix);
   organisation = panel->LayoutTextWithLabel("Organisation:", title);

   m_notebook->AddPage(panel, "Name");
}

// Panel 2 - home address
void wxAdbEditFrame::CreateHomePage()
{
   wxAdbEditPanel *panel = GLOBAL_NEW wxAdbEditPanel(this, m_notebook);

   hApb = panel->LayoutTextWithLabel("PO Box:", NULL);
   hAex = panel->LayoutTextWithLabel("Extra:", hApb);
   hAst = panel->LayoutTextWithLabel("Street:", hAex);
   hAlo = panel->LayoutTextWithLabel("Locality:", hAst);
   hAre = panel->LayoutTextWithLabel("Region:", hAlo);
   hApc = panel->LayoutTextWithLabel("Postcode:", hAre);
   hAco = panel->LayoutTextWithLabel("Country:", hApc);
   hPh  = panel->LayoutTextWithLabel("Telephone:", hAco);
   hFax = panel->LayoutTextWithLabel("Telefax:", hPh);

   m_notebook->AddPage(panel, "Home Address");
}

// Panel 3 - work address
void wxAdbEditFrame::CreateWorkPage()
{
   wxAdbEditPanel *panel = GLOBAL_NEW wxAdbEditPanel(this, m_notebook);

   wApb = panel->LayoutTextWithLabel("PO Box:", NULL);
   wAex = panel->LayoutTextWithLabel("Extra:", wApb);
   wAst = panel->LayoutTextWithLabel("Street:", wAex);
   wAlo = panel->LayoutTextWithLabel("Locality:", wAst);
   wAre = panel->LayoutTextWithLabel("Region:", wAlo);
   wApc = panel->LayoutTextWithLabel("Postcode:", wAre);
   wAco = panel->LayoutTextWithLabel("Country:", wApc);
   wPh  = panel->LayoutTextWithLabel("Telepwone:", wAco);
   wFax = panel->LayoutTextWithLabel("Telefax:", wPh);

   m_notebook->AddPage(panel, "Work Address");
}

// Panel 4 - email
void wxAdbEditFrame::CreateMailPage()
{
   wxAdbEditPanel *panel = GLOBAL_NEW wxAdbEditPanel(this, m_notebook);

   email = panel->LayoutTextWithLabel("e-mail:", NULL);
   alias = panel->LayoutTextWithLabel("alias:", email);
   url   = panel->LayoutTextWithLabel("URL:", alias);

   wxMessage *label = CreateLabel(panel,"alternative e-mail addresses:");

   wxLayoutConstraints *c;
   c = new wxLayoutConstraints;
   c->top.SameAs(url, wxBottom, 5);
   c->left.SameAs(panel, wxLeft, 5);
   c->width.AsIs();
   c->height.AsIs();
   label->SetConstraints(c);

   listBox = CreateListBox(panel, -1, -1, -1, -1);
   c = new wxLayoutConstraints;
   c->top.SameAs(label, wxBottom, 5);
   c->left.SameAs(panel, wxLeft, 5);
   c->right.SameAs(panel, wxRight, 5);
   c->height.AsIs();
   listBox->SetConstraints(c);

   m_notebook->AddPage(panel, "E-Mail");
}


void
wxAdbEditFrame::Create(wxFrame *parent, ProfileBase *iprofile)
{
   CHECK_RET( !initialised, "wxAdbEditFrame created twice" );

   view = NULL;
   
   profile = iprofile;
   eptr = NULL;
   
   Show(FALSE);
   
   fileMenu = GLOBAL_NEW wxMenu;
   fileMenu->Append(WXMENU_FILE_CLOSE,WXCPTR _("&Close"));
   fileMenu->AppendSeparator();
   fileMenu->Append(WXMENU_FILE_EXIT,WXCPTR _("&Exit"));

   menuBar = GLOBAL_NEW wxMenuBar;
   menuBar->Append(fileMenu, WXCPTR _("&File"));

   SetMenuBar(menuBar);

   m_notebook = new wxNotebook(this, AdbEdit_Notebook);

   CreateNamePage();
   CreateHomePage();
   CreateWorkPage();
   CreateMailPage();

   m_notebook->Show(TRUE);  

   if(mApplication.GetAdb()->size())
      Load(*(mApplication.GetAdb()->begin()));
   else
      New();
   
   initialised = true;
   Show(TRUE);
}

#ifdef USE_WXWINDOWS2

void wxAdbEditFrame::OnSize(wxSizeEvent& event)
{
   event.Skip();
}

#else

void
wxAdbEditFrame::OnSize(int  w, int h)
{
   if(! view)
      return;
   
   wxRectangle rect;   rect.x = 5;   rect.y = 100;
      // as below, but let's assume we have a fixed view width.
   //rect.width = view->GetTabWidth()*4 + 3*view->GetHorizontalTabSpacing();
   rect.width = w - 30;
   rect.height = h - 100 - 10;
   view->SetViewRect(rect);

   int pwidth , pheight;
   pwidth = w - 60;
   pheight = h - 100 - 40;
   
   // Calculate the tab width for 2 tabs, based on a view width of 326 and
   // the current horizontal spacing. Adjust the view width to exactly fit
   // the tabs.
   view->CalculateTabWidth(2, TRUE);
}

#endif // wxWin2

wxAdbEditFrame::wxAdbEditFrame(wxFrame *parent, ProfileBase *iprofile)
              : wxMFrame(ADBEDITFRAME_NAME, parent)
{
   initialised = false;
   Create(parent,iprofile);
}

wxAdbEditFrame::~wxAdbEditFrame()
{
}

void
wxAdbEditFrame::OnMenuCommand(int id)
{
   switch(id)
   {
   case WXMENU_FILE_CLOSE:
      OnClose();
      GLOBAL_DELETE this;
      break;
   case WXMENU_FILE_EXIT:
      mApplication.Exit();
      break;
   default:
      wxMFrame::OnMenuCommand(id);
   }
}

void
wxAdbEditFrame::New(void)
{
   eptr = mApplication.GetAdb()->NewEntry();
   Load(eptr);
}


void
wxAdbEditFrame::Delete(void)
{
   if(! eptr)
      return;
   mApplication.GetAdb()->Delete(eptr);
   Load(*mApplication.GetAdb()->begin());

}

void
wxAdbEditFrame::Load(AdbEntry *ieptr)
{
   String   tmp;

   if(! ieptr)
      return;
   eptr = ieptr;
   
   // name:
   formattedName->SetValue(WXCPTR eptr->formattedName.c_str());
   title->SetValue(WXCPTR eptr->title.c_str());
   organisation->SetValue(WXCPTR eptr->organisation.c_str());

   strNameFirst->SetValue(WXCPTR eptr->structuredName.first.c_str());
   strNamePrefix->SetValue(WXCPTR eptr->structuredName.prefix.c_str());
   strNamePostfix->SetValue(WXCPTR eptr->structuredName.suffix.c_str());
   strNameOther->SetValue(WXCPTR eptr->structuredName.other.c_str());
   strNameFamily->SetValue(WXCPTR eptr->structuredName.family.c_str());

   hApb->SetValue(WXCPTR eptr->homeAddress.poBox.c_str());
   hAex->SetValue(WXCPTR eptr->homeAddress.extra.c_str());
   hAst->SetValue(WXCPTR eptr->homeAddress.street.c_str());
   hAlo->SetValue(WXCPTR eptr->homeAddress.locality.c_str());
   hAre->SetValue(WXCPTR eptr->homeAddress.region.c_str());
   hApc->SetValue(WXCPTR eptr->homeAddress.postcode.c_str());
   hAco->SetValue(WXCPTR eptr->homeAddress.country.c_str());
   eptr->homePhone.write(tmp);
   hPh->SetValue(WXCPTR tmp.c_str());
   eptr->homeFax.write(tmp);
   hFax->SetValue(WXCPTR tmp.c_str());

   wApb->SetValue(WXCPTR eptr->workAddress.poBox.c_str());
   wAex->SetValue(WXCPTR eptr->workAddress.extra.c_str());
   wAst->SetValue(WXCPTR eptr->workAddress.street.c_str());
   wAlo->SetValue(WXCPTR eptr->workAddress.locality.c_str());
   wAre->SetValue(WXCPTR eptr->workAddress.region.c_str());
   wApc->SetValue(WXCPTR eptr->workAddress.postcode.c_str());
   wAco->SetValue(WXCPTR eptr->workAddress.country.c_str());
   eptr->workPhone.write(tmp);
   wPh->SetValue(WXCPTR tmp.c_str());
   eptr->workFax.write(tmp);
   wFax->SetValue(WXCPTR tmp.c_str());
   
   
   
   // email et al:
   email->SetValue(WXCPTR eptr->email.preferred.c_str());
   url->SetValue(WXCPTR eptr->url.c_str());
   alias->SetValue(WXCPTR eptr->alias.c_str());

  
}

void
wxAdbEditFrame::Save(void)
{
   String   tmp;

   if(! eptr)
   {
      eptr = mApplication.GetAdb()->NewEntry();
      mApplication.GetAdb()->AddEntry(eptr);
   }
   
   // name:
   eptr->formattedName=formattedName->GetValue();
   eptr->title=title->GetValue();
   eptr->organisation=organisation->GetValue();
   eptr->structuredName.first=strNameFirst->GetValue();
   eptr->structuredName.prefix=strNamePrefix->GetValue();
   eptr->structuredName.suffix=strNamePostfix->GetValue();
   eptr->structuredName.other=strNameOther->GetValue();
   eptr->structuredName.family=strNameFamily->GetValue();


   eptr->homeAddress.poBox =hApb->GetValue();
   eptr->homeAddress.extra =hAex->GetValue();
   eptr->homeAddress.street =hAst->GetValue();
   eptr->homeAddress.locality =hAlo->GetValue();
   eptr->homeAddress.region =hAre->GetValue();
   eptr->homeAddress.postcode =hApc->GetValue();
   eptr->homeAddress.country =hAco->GetValue();

   tmp = hPh->GetValue();
   eptr->homePhone.parse(tmp);
   tmp = hFax->GetValue();
   eptr->homeFax.parse(tmp);

   eptr->workAddress.poBox =wApb->GetValue();
   eptr->workAddress.extra =wAex->GetValue();
   eptr->workAddress.street =wAst->GetValue();
   eptr->workAddress.locality =wAlo->GetValue();
   eptr->workAddress.region =wAre->GetValue();
   eptr->workAddress.postcode =wApc->GetValue();
   eptr->workAddress.country =wAco->GetValue();

   tmp = wPh->GetValue();
   eptr->workPhone.parse(tmp);
   tmp = wFax->GetValue();
   eptr->workFax.parse(tmp);
   
   
   // email et al:
   eptr->email.preferred=email->GetValue();
   eptr->url=url->GetValue();
   eptr->alias=alias->GetValue();

   mApplication.GetAdb()->Update(eptr);
}
