/*-*- c++ -*-********************************************************
 * wxAdbEdit.cc : a wxWindows look at a message                 *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.2  1998/03/26 23:05:40  VZ
 * Necessary changes to make it compile under Windows (VC++ only)
 * Header reorganization to be able to use precompiled headers
 *
 * Revision 1.1  1998/03/14 12:21:21  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifdef __GNUG__
#pragma	implementation "wxAdbEdit.h"
#endif

#include    "Mpch.h"
#include	  "Mcommon.h"

#include	"MFrame.h"
#include	"MLogFrame.h"

#include	"Mdefaults.h"

#include	"PathFinder.h"
#include	"MimeList.h"
#include	"MimeTypes.h"
#include	"Profile.h"

#include  "MApplication.h"

#include  "Adb.h"

#include	"gui/wxtab.h"
#include	"gui/wxAdbEdit.h"

#if 0
void
wxAdbEditPanel::Create(wxMFrame *parent, int x, int y, int width, int
		   height)
{
   wxPanel::Create(parent,x,y,width,height);
   SetLabelPosition(wxVERTICAL);

   wxLayoutConstraints *c;

   c = GLOBAL_NEW wxLayoutConstraints;
   c->top.SameAs	(parent, wxTop);
   c->left.SameAs	(parent, wxLeft);
   c->right.SameAs	(parent, wxRight);
   c->bottom.SameAs	(parent, wxBottom);
   this->SetConstraints(c);
   
   wxMessage *nameLabel = CreateLabel(this, "Key:");
   c = GLOBAL_NEW wxLayoutConstraints;
   c->top.SameAs(this, wxTop, 5);
   c->left.SameAs(this, wxLeft, 5);
   c->height.AsIs();
   c->width.AsIs();
   nameLabel->SetConstraints(c);

   key = CreateText(this, -1, -1, -1, -1, "");
   c = GLOBAL_NEW wxLayoutConstraints;
   c->top.SameAs(this, wxTop, 5);
   c->left.SameAs(nameLabel, wxRight, 5);
   c->right.SameAs(this, wxRight, 5);
   c->height.AsIs();
   key->SetConstraints(c);

   listBox = CreateListBox(this, -1, -1, -1, -1);
   c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(nameLabel);
   c->left.SameAs	(this, wxLeft, 5);
   c->height.AsIs();
   c->right.SameAs	(this, wxRight, 5);
   listBox->SetConstraints(c);


   wxMessage *fnLabel = CreateLabel(this, "Formatted Name:");
   c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(listBox);
   c->left.SameAs	(this, wxLeft, 5);
   c->height.AsIs();
   c->width.AsIs();
   fnLabel->SetConstraints(c);

   formattedName = CreateText(this, -1, -1, -1, -1, "");
   c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(fnLabel);
   c->left.SameAs	(this, wxLeft, 5);
   c->height.AsIs();
   c->right.SameAs	(this, wxRight, 5);
   formattedName->SetConstraints(c);

   wxMessage *fnPrLabel = CreateLabel(this,"Prefix:");
   wxMessage *fnFiLabel = CreateLabel(this,"First");
   wxMessage *fnOtLabel = CreateLabel(this,"Other:");
   wxMessage *fnFaLabel = CreateLabel(this,"Family:");
   wxMessage *fnPoLabel = CreateLabel(this,"Postfix:");
   
   strNamePrefix = CreateText(this, -1, -1, -1, -1, "");
   strNameFirst = CreateText(this, -1, -1, -1, -1, "");
   strNameOther = CreateText(this, -1, -1, -1, -1, "");
   strNameFamily = CreateText(this, -1, -1, -1, -1, "");
   strNamePostfix = CreateText(this, -1, -1, -1, -1, "");

   c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(formattedName);
   c->left.SameAs	(this, wxLeft, 5);
   c->height.AsIs();
   c->width.AsIs();
   fnPrLabel->SetConstraints(c);

   c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(fnPrLabel);
   c->left.SameAs	(fnPrLabel, wxLeft);
   c->height.AsIs();
   c->width.PercentOf	(this, wxWidth, 10);
   strNamePrefix->SetConstraints(c);
   
   c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(formattedName);
   c->left.SameAs	(strNamePrefix, wxRight, 5);
   c->height.AsIs();
   c->width.AsIs();
   fnFiLabel->SetConstraints(c);

   c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(fnFiLabel);
   c->left.SameAs	(fnFiLabel, wxLeft);
   c->height.AsIs();
   c->width.PercentOf	(this, wxWidth, 30);
   strNameFirst->SetConstraints(c);

   c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(formattedName);
   c->left.SameAs	(strNameFirst, wxRight, 5);
   c->height.AsIs();
   c->width.AsIs();
   fnOtLabel->SetConstraints(c);

   c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(fnOtLabel);
   c->left.SameAs	(fnOtLabel, wxLeft);
   c->height.AsIs();
   c->width.PercentOf	(this, wxWidth, 10);
   strNameOther->SetConstraints(c);

   c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(formattedName);
   c->left.SameAs	(strNameOther, wxRight, 5);
   c->height.AsIs();
   c->width.AsIs();
   fnFaLabel->SetConstraints(c);

   c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(fnFaLabel);
   c->left.SameAs	(fnFaLabel, wxLeft);
   c->height.AsIs();
   c->width.PercentOf	(this, wxWidth, 30);
   strNameFamily->SetConstraints(c);

   c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(formattedName);
   c->left.SameAs	(strNameFamily, wxRight, 5);
   c->height.AsIs();
   c->width.AsIs();
   fnPoLabel->SetConstraints(c);

   c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(fnPoLabel);
   c->left.SameAs	(fnPoLabel, wxLeft);
   c->height.AsIs();
   c->width.PercentOf	(this, wxWidth, 5);
   strNamePostfix->SetConstraints(c);

   wxMessage *tiLabel = CreateLabel(this, "Title:");
   c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(strNamePostfix);
   c->left.SameAs	(this, wxLeft, 5);
   c->height.AsIs();
   c->width.AsIs();
   tiLabel->SetConstraints(c);
   
   title = CreateText(this, -1, -1, -1, -1, "");
   c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(strNamePostfix);
   c->left.SameAs	(tiLabel, wxRight, 5);
   c->height.AsIs();
   c->width.AsIs();
   title->SetConstraints(c);

   wxMessage *orLabel = CreateLabel(this, "Organisation:");
   c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(strNamePostfix);
   c->left.SameAs	(title, wxRight, 5);
   c->height.AsIs();
   c->width.AsIs();
   orLabel->SetConstraints(c);
   
   organisation = GLOBAL_NEW wxText(this,NULL,NULL,"");
   c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(strNamePostfix);
   c->left.SameAs	(orLabel, wxRight, 5);
   c->height.AsIs();
   c->width.AsIs();
   organisation->SetConstraints(c);

   wxMessage *hAddrLabel = CreateLabel(this,"Home Address:");
   c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(title);
   c->left.SameAs	(this, wxLeft, 5);
   c->height.AsIs();   c->width.AsIs();
   title->SetConstraints(c);

   wxMessage *hApbLabel = CreateLabel(this,"PO Box:");
   c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(hAddrLabel);
   c->left.SameAs	(this, wxLeft, 5);
   c->height.AsIs();   c->width.AsIs();
   title->SetConstraints(c);
   
   hApb = CreateText(this, -1, -1, -1, -1, "");
   c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(hAddrLabel);
   c->left.SameAs	(hApbLabel, wxRight, 5);
   c->height.AsIs();   c->right.PercentOf(this, wxWidth, 40);
   title->SetConstraints(c);

   wxMessage *hAexLabel = CreateLabel(this, "Extra:");
   c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(hApbLabel);
   c->left.SameAs	(this, wxLeft, 5);
   c->height.AsIs();   c->width.AsIs();
   title->SetConstraints(c);

   hAex = CreateText(this, -1, -1, -1, -1, "");
   c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(hApbLabel);
   c->left.SameAs	(hAexLabel, wxRight, 5);
   c->height.AsIs();   c->right.PercentOf(this, wxWidth, 40);
   title->SetConstraints(c);

#if 0
   wxMessage *hAstLabel = CreateLabel(this, "Street:");
   c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(hAexLabel);
   c->left.SameAs	(this, wxLeft, 5);
   c->height.AsIs();   c->width.AsIs();
   title->SetConstraints(c);

   hAst = GLOBAL_NEW CreateText(this, -1, -1, -1, -1, "");
      c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(hAexLabel);
   c->left.SameAs	(hAstLabel, wxRight, 5);
   c->height.AsIs();   c->right.PercentOf(this, wxWidth, 40);
   title->SetConstraints(c);

   wxMessage *hAloLabel = CreateLabel(this,"Locality:");
   c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(hAstLabel);
   c->left.SameAs	(this, wxLeft, 5);
   c->height.AsIs();   c->width.AsIs();
   title->SetConstraints(c);

   hAlo = CreateText(this, -1, -1, -1, -1, "");
      c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(hAstLabel);
   c->left.SameAs	(hAloLabel, wxRight, 5);
   c->height.AsIs();   c->right.PercentOf(this, wxWidth, 40);
   title->SetConstraints(c);

   wxMessage *hAreLabel = CreateLabel(this,"Region:");
   c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(hAloLabel);
   c->left.SameAs	(this, wxLeft, 5);
   c->height.AsIs();   c->width.AsIs();
   title->SetConstraints(c);

   hAre = CreateText(this, -1, -1, -1, -1, "");
      c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(hAloLabel);
   c->left.SameAs	(hAreLabel, wxRight, 5);
   c->height.AsIs();   c->right.PercentOf(this, wxWidth, 40);
   title->SetConstraints(c);

   wxMessage *hApcLabel = CreateLabel(this,"Postcode:");
   c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(hAreLabel);
   c->left.SameAs	(this, wxLeft, 5);
   c->height.AsIs();   c->width.AsIs();
   title->SetConstraints(c);

   hApc = CreateText(this, -1, -1, -1, -1, "");
      c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(hAreLabel);
   c->left.SameAs	(hApcLabel, wxRight, 5);
   c->height.AsIs();   c->right.PercentOf(this, wxWidth, 40);
   title->SetConstraints(c);

   wxMessage *hAcoLabel = CreateLabel(this,"Country:");
   c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(hApcLabel);
   c->left.SameAs	(this, wxLeft, 5);
   c->height.AsIs();   c->width.AsIs();
   title->SetConstraints(c);

   hAco = CreateText(this, -1, -1, -1, -1, "");
      c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(hApcLabel);
   c->left.SameAs	(hAcoLabel, wxRight, 5);
   c->height.AsIs();   c->right.PercentOf(this, wxWidth, 40);
   title->SetConstraints(c);

   wxMessage *wAddrLabel = CreateLabel(this,"Work Address:");
   c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(title);
   c->left.PercentOf	(this, wxWidth, 50);
   c->height.AsIs();   c->width.AsIs();
   title->SetConstraints(c);

   wxMessage *wApbLabel = CreateLabel(this,"PO Box:");
   c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(wAddrLabel);
   c->left.SameAs	(wAddrLabel, wxLeft);
   c->height.AsIs();   c->width.AsIs();
   title->SetConstraints(c);
   
   wApb = CreateText(this, -1, -1, -1, -1, "");
   c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(wAddrLabel);
   c->left.SameAs	(wApbLabel, wxRight, 5);
   c->height.AsIs();   c->right.PercentOf(this, wxWidth, 40);
   title->SetConstraints(c);

   wxMessage *wAexLabel = CreateLabel(this,"Extra:");
   c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(wApbLabel);
   c->left.SameAs	(wAddrLabel, wxLeft);
   c->height.AsIs();   c->width.AsIs();
   title->SetConstraints(c);

   wAex = CreateText(this, -1, -1, -1, -1, "");
   c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(wApbLabel);
   c->left.SameAs	(wAexLabel, wxRight, 5);
   c->height.AsIs();   c->right.PercentOf(this, wxWidth, 40);
   title->SetConstraints(c);

   wxMessage *wAstLabel = CreateLabel(this,"Street:");
   c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(wAexLabel);
   c->left.SameAs	(wAddrLabel, wxLeft);
   c->height.AsIs();   c->width.AsIs();
   title->SetConstraints(c);

   wAst = CreateText(this, -1, -1, -1, -1, "");
      c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(wAexLabel);
   c->left.SameAs	(wAstLabel, wxRight, 5);
   c->height.AsIs();   c->right.PercentOf(this, wxWidth, 40);
   title->SetConstraints(c);

   wxMessage *wAloLabel = CreateLabel(this,"Locality:");
   c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(wAstLabel);
   c->left.SameAs	(wAddrLabel, wxLeft);
   c->height.AsIs();   c->width.AsIs();
   title->SetConstraints(c);

   wAlo = CreateText(this, -1, -1, -1, -1, "");
      c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(wAstLabel);
   c->left.SameAs	(wAloLabel, wxRight, 5);
   c->height.AsIs();   c->right.PercentOf(this, wxWidth, 40);
   title->SetConstraints(c);

   wxMessage *wAreLabel = CreateLabel(this,"Region:");
   c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(wAloLabel);
   c->left.SameAs	(wAddrLabel, wxLeft);
   c->height.AsIs();   c->width.AsIs();
   title->SetConstraints(c);

   wAre = CreateText(this, -1, -1, -1, -1, "");
      c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(wAloLabel);
   c->left.SameAs	(wAreLabel, wxRight, 5);
   c->height.AsIs();   c->right.PercentOf(this, wxWidth, 40);
   title->SetConstraints(c);

   wxMessage *wApcLabel = CreateLabel(this,"Postcode:");
   c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(wAreLabel);
   c->left.SameAs	(wAddrLabel, wxLeft);
   c->height.AsIs();   c->width.AsIs();
   title->SetConstraints(c);

   wApc = CreateText(this, -1, -1, -1, -1, "");
      c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(wAreLabel);
   c->left.SameAs	(wApcLabel, wxRight, 5);
   c->height.AsIs();   c->right.PercentOf(this, wxWidth, 40);
   title->SetConstraints(c);

   wxMessage *wAcoLabel = CreateLabel(this,"Country:");
   c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(wApcLabel);
   c->left.SameAs	(wAddrLabel, wxLeft);
   c->height.AsIs();   c->width.AsIs();
   title->SetConstraints(c);

   wAco = CreateText(this, -1, -1, -1, -1, "");
      c = GLOBAL_NEW wxLayoutConstraints;
   c->top.Below		(wApcLabel);
   c->left.SameAs	(wAcoLabel, wxRight, 5);
   c->height.AsIs();   c->right.PercentOf(this, wxWidth, 40);
   title->SetConstraints(c);
   
   wxMessage *wPhLabel = CreateLabel(this,_("Work Phone:"));
   wxMessage *wPhCoLabel  = CreateLabel(this,_("Country:"));
   wPhCo = GLOBAL_NEW wxText(this,NULL,NULL,"");
   wxMessage *wPhArLabel  = CreateLabel(this,_("Area:"));
   wPhAr = GLOBAL_NEW wxText(this,NULL,NULL,"");
   wxMessage *wPhLoLabel  = CreateLabel(this,_("Local:"));
   wPhLo = GLOBAL_NEW wxText(this,NULL,NULL,"");

   wxMessage *wFaxLabel = CreateLabel(this,_("Work Fax:"));
   wxMessage *wFaxCoLabel  = CreateLabel(this,_("Country:"));
   wFaxCo = GLOBAL_NEW wxText(this,NULL,NULL,"");
   wxMessage *wFaxArLabel  = CreateLabel(this,_("Area:"));
   wFaxAr = GLOBAL_NEW wxText(this,NULL,NULL,"");
   wxMessage *wFaxLoLabel  = CreateLabel(this,_("Local:"));
   wFaxLo = GLOBAL_NEW wxText(this,NULL,NULL,"");


   wxMessage *hPhLabel = CreateLabel(this,_("Home Phone:"));
   wxMessage *hPhCoLabel  = CreateLabel(this,_("Country:"));
   hPhCo = GLOBAL_NEW wxText(this,NULL,NULL,"");
   wxMessage *hPhArLabel  = CreateLabel(this,_("Area:"));
   hPhAr = GLOBAL_NEW wxText(this,NULL,NULL,"");
   wxMessage *hPhLoLabel  = CreateLabel(this,_("Local:"));
   hPhLo = GLOBAL_NEW wxText(this,NULL,NULL,"");

   wxMessage *hFaxLabel = CreateLabel(this,_("Home Fax:"));
   wxMessage *hFaxCoLabel  = CreateLabel(this,_("Country:"));
   hFaxCo = GLOBAL_NEW wxText(this,NULL,NULL,"");
   wxMessage *hFaxArLabel  = CreateLabel(this,_("Area:"));
   hFaxAr = GLOBAL_NEW wxText(this,NULL,NULL,"");
   wxMessage *hFaxLoLabel  = CreateLabel(this,_("Local:"));
   hFaxLo = GLOBAL_NEW wxText(this,NULL,NULL,"");

   wxMessage *emailLabel = CreateLabel(this,_("Email:"));
   email = GLOBAL_NEW wxText(this, NULL, NULL, "");
   emailOther = GLOBAL_NEW wxListBox(this, (wxFunction) NULL, "",
			   wxSINGLE,
			   -1,-1,-1,-1,
			   0,
			   NULL,
			   wxALWAYS_SB);
   
   wxMessage *aliasLabel = CreateLabel(this,_("Alias:"));
   alias = GLOBAL_NEW wxText(this, NULL, NULL, "");

   wxMessage *urlLabel = CreateLabel(this,_("URL:"));
   url = GLOBAL_NEW wxText(this, NULL, NULL, "");
#endif
}
   
wxAdbEditPanel::wxAdbEditPanel(wxMFrame *parent, int x, int y, int width, int
		   height)
{
   Create(parent, x, y, width, height);
}

#endif

enum
{
   TAB_NAME,
   TAB_HOME_ADDRESS,
   TAB_WORK_ADDRESS,
   TAB_EMAIL
};

class wxAdbEditPanel : public wxTabbedPanel
{
private:
   wxAdbEditFrame	*frame;
   wxText		*key;
public:
   wxAdbEditPanel(wxAdbEditFrame *parent, int x, int y, int w, int h);
   void OnCommand(wxWindow &win, wxCommandEvent &event);
};

void
wxAdbEditPanel::OnCommand(wxWindow &win, wxCommandEvent &event)
{
   if(strcmp(win.GetName(),"keyField")==0)
   {
      String	tmp = key->GetValue();
      int l = tmp.length();
      if(l > 0  &&  tmp.substr(l-1,1) == " ")
      {
	 key->SetValue(WXCPTR tmp.substr(0,l-1).c_str());
	 frame->Load(mApplication.GetAdb()->Lookup(String(key->GetValue()), frame));
      }
   }
   else if(strcmp(win.GetName(), "expandButton") == 0)
	   frame->Load(mApplication.GetAdb()->Lookup(String(key->GetValue()), frame));
   else if(strcmp(win.GetName(),"saveButton") == 0)
      frame->Save();
   else if(strcmp(win.GetName(),"newButton") == 0)
      frame->New();
   else if(strcmp(win.GetName(),"delButton") == 0)
      frame->Delete();
}
   

wxAdbEditPanel::wxAdbEditPanel(wxAdbEditFrame *parent, int x, int y, int w, int h)
      : wxTabbedPanel(parent,x,y,w,h)
{
  frame = parent;

  wxMessage *nameLabel = CreateLabel(this, "Key:");
  key = CreateText(this, -1, -1, w/2, -1, "keyField");

  CreateButton(this, "Expand", "expandButton");
  CreateButton(this, "New",    "newButton");
  CreateButton(this, "Delete", "delButton");
  CreateButton(this, "Save",   "saveButton");
}

void
wxAdbEditFrame::Create(wxFrame *parent, ProfileBase *iprofile)
{
   if(initialised)
      return; // ERROR!

   view = NULL;
   
   wxMFrame::Create(ADBEDITFRAME_NAME,parent);
   
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

   int	width, height;
   this->GetClientSize(&width, &height);
   wxPanel *panel = GLOBAL_NEW wxAdbEditPanel(this,0,0,width,height);
   view = GLOBAL_NEW wxPanelTabView(panel, wxTAB_STYLE_DRAW_BOX);
   wxRectangle rect;   rect.x = 5;   rect.y = 100;
   // Could calculate the view width from the tab width and spacing,
   // as below, but let's assume we have a fixed view width.
   //rect.width = view->GetTabWidth()*4 + 3*view->GetHorizontalTabSpacing();
   rect.width = width - 30;
   rect.height = height - 100 - 10;
   view->SetViewRect(rect);

   int pwidth , pheight;
   pwidth = width - 60;
   pheight = height - 100 - 40;
   
   // Calculate the tab width for 2 tabs, based on a view width of 326 and
   // the current horizontal spacing. Adjust the view width to exactly fit
   // the tabs.
   view->CalculateTabWidth(2, TRUE);

   if (! view->AddTab(TAB_NAME, _("Name")))
      return;
   if (! view->AddTab(TAB_HOME_ADDRESS, _("Home Addr.")))
      return;
   if (! view->AddTab(TAB_WORK_ADDRESS, _("Work Addr.")))
      return;
   if (! view->AddTab(TAB_EMAIL, _("e-mail")))
      return;

   int	fieldpos, fieldwidth;
   fieldpos = pwidth/3;
   fieldwidth = (int) (pwidth * 0.6);
   
   // Panel 1 - personal Name   
   wxPanel *panel1 = CreatePanel(panel, rect.x + 20, rect.y + 10, 
                                 pwidth, pheight);
   CreateLabel(panel1, "Formatted Name:");
   formattedName = CreateText(panel1, fieldpos, -1, fieldwidth, -1, "");

   NewLine(panel1);
   CreateLabel(panel1, "Prefix:");
   strNamePrefix = CreateText(panel1, fieldpos, -1, fieldwidth, -1, "");

   NewLine(panel1);
   wxMessage *fnFiLabel = CreateLabel(panel1, "First");
   strNameFirst = CreateText(panel1, fieldpos, -1, fieldwidth, -1, "");

   NewLine(panel1);
   wxMessage *fnOtLabel = CreateLabel(panel1, "Other:");
   strNameOther = CreateText(panel1, fieldpos, -1, fieldwidth, -1, "");

   NewLine(panel1);
   wxMessage *fnFaLabel = CreateLabel(panel1, "Family:");
   strNameFamily = CreateText(panel1, fieldpos, -1, fieldwidth, -1, "");

   NewLine(panel1);
   wxMessage *fnPoLabel = CreateLabel(panel1, "Postfix:");
   strNamePostfix = CreateText(panel1, fieldpos, -1, fieldwidth, -1, "");

   NewLine(panel1);
   wxMessage *tiLabel = CreateLabel(panel1, "Title:");
   title = CreateText(panel1, fieldpos, -1, fieldwidth, -1, "");

   NewLine(panel1);
   wxMessage *orgLabel = CreateLabel(panel1, "Organisation:");
   organisation = CreateText(panel1, fieldpos, -1, fieldwidth, -1, "");

   NewLine(panel1);
   view->AddTabPanel(TAB_NAME, panel1);

   // Panel 2 - home address
   wxPanel *panel2 = CreatePanel(panel, rect.x + 20, rect.y + 10, 
                                 pwidth, pheight);
   wxMessage *hApbLabel = CreateLabel(panel2, "PO Box:");
   hApb = CreateText(panel2, fieldpos, -1,fieldwidth,-1, "");

   NewLine(panel2);
   wxMessage *hAexLabel = CreateLabel(panel2, "Extra:");
   hAex = CreateText(panel2,fieldpos, -1,fieldwidth,-1, "");

   NewLine(panel2);
   wxMessage *hAstLabel = CreateLabel(panel2, "Street:");
   hAst = CreateText(panel2,fieldpos, -1,fieldwidth,-1, "");

   NewLine(panel2);
   wxMessage *hAloLabel = CreateLabel(panel2, "Locality:");
   hAlo = CreateText(panel2,fieldpos, -1,fieldwidth,-1, "");

   NewLine(panel2);
   wxMessage *hAreLabel = CreateLabel(panel2, "Region:");
   hAre = CreateText(panel2,fieldpos, -1,fieldwidth,-1, "");

   NewLine(panel2);
   wxMessage *hApcLabel = CreateLabel(panel2, "Postcode:");
   hApc = CreateText(panel2,fieldpos, -1,fieldwidth,-1, "");

   NewLine(panel2);
   wxMessage *hAcoLabel = CreateLabel(panel2, "Country:");
   hAco = CreateText(panel2,fieldpos, -1,fieldwidth,-1, "");

   NewLine(panel2);
   wxMessage *hPhLabel = CreateLabel(panel2, "Telephone:");
   hPh = CreateText(panel2,fieldpos, -1,fieldwidth,-1, "");

   NewLine(panel2);
   wxMessage *hFaxLabel = CreateLabel(panel2, "Telefax:");
   hFax = CreateText(panel2,fieldpos, -1,fieldwidth,-1, "");

   NewLine(panel2);
   view->AddTabPanel(TAB_HOME_ADDRESS, panel2);


   // Panel 3 - work address
   wxPanel *panel3 = CreatePanel(panel, rect.x + 20, rect.y + 10,
				                         pwidth, pheight);
   wxMessage *wApbLabel = CreateLabel(panel3,"PO Box:");
   wApb = CreateText(panel3,fieldpos, -1,fieldwidth,-1, "");

   NewLine(panel3);
   wxMessage *wAexLabel = CreateLabel(panel3,"Extra:");
   wAex = CreateText(panel3,fieldpos, -1,fieldwidth,-1, "");

   NewLine(panel3);
   wxMessage *wAstLabel = CreateLabel(panel3,"Street:");
   wAst = CreateText(panel3,fieldpos, -1,fieldwidth,-1, "");

   NewLine(panel3);
   wxMessage *wAloLabel = CreateLabel(panel3,"Locality:");
   wAlo = CreateText(panel3,fieldpos, -1,fieldwidth,-1, "");

   NewLine(panel3);
   wxMessage *wAreLabel = CreateLabel(panel3,"Region:");
   wAre = CreateText(panel3,fieldpos, -1,fieldwidth,-1, "");

   NewLine(panel3);
   wxMessage *wApcLabel = CreateLabel(panel3,"Postcode:");
   wApc = CreateText(panel3,fieldpos, -1,fieldwidth,-1, "");

   NewLine(panel3);
   wxMessage *wAcoLabel = CreateLabel(panel3,"Country:");
   wAco = CreateText(panel3,fieldpos, -1,fieldwidth,-1, "");

   NewLine(panel3);
   wxMessage *wPhLabel = CreateLabel(panel3,"Telephone:");
   wPh = CreateText(panel3,fieldpos, -1,fieldwidth,-1, "");

   NewLine(panel3);
   wxMessage *wFaxLabel = CreateLabel(panel3,"Telefax:");
   wFax = CreateText(panel3,fieldpos, -1,fieldwidth,-1, "");

   NewLine(panel3);
   view->AddTabPanel(TAB_WORK_ADDRESS, panel3);
   
   // Panel 4 - email
   wxPanel *panel4 = CreatePanel(panel, rect.x + 20, rect.y + 10,
				                         pwidth, pheight);
   wxMessage *emailLabel = CreateLabel(panel4,"e-mail:");
   email = CreateText(panel4,fieldpos, -1,fieldwidth,-1, "");

   NewLine(panel4);
   wxMessage *aliasLabel = CreateLabel(panel4,"alias:");
   alias = CreateText(panel4,fieldpos, -1,fieldwidth,-1, "");

   NewLine(panel4);
   wxMessage *urlLabel = CreateLabel(panel4,"URL:");
   url = CreateText(panel4,fieldpos, -1,fieldwidth,-1, "");

   NewLine(panel4);
   wxMessage *aeLabel = CreateLabel(panel4,"alternative e-mail addresses:");

   NewLine(panel4);
   listBox = CreateListBox(panel4, -1, -1, pwidth*0.8, -1);
   
   NewLine(panel4);
   view->AddTabPanel(TAB_EMAIL, panel4);

   // Don't know why this is necessary under Motif...
   //dialog->SetSize(dialogWidth, dialogHeight-20);

   view->SetTabSelection(TAB_NAME);
   
   //Fit();
   panel->Show(TRUE);  

   if(mApplication.GetAdb()->size())
      Load(*(mApplication.GetAdb()->begin()));
   else
      New();
   
   initialised = true;
   Show(TRUE);
}

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

wxAdbEditFrame::wxAdbEditFrame(wxFrame *parent,
			       ProfileBase *iprofile)
{
   initialised = false;
   Create(parent,iprofile);
}

wxAdbEditFrame::~wxAdbEditFrame()
{
   if(! initialised)
      return;
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
   Load(*(mApplication.GetAdb()->begin()));

}

void
wxAdbEditFrame::Load(AdbEntry *ieptr)
{
   String	tmp;

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
   String	tmp;

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
