/*-*- c++ -*-********************************************************
 * wxAdbEdit.cc : a wxWindows look at a message                 *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.1  1998/03/14 12:21:21  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifdef __GNUG__
#pragma	implementation "wxAdbEdit.h"
#endif

#include	"wxtab.h"

#include	<Mcommon.h>
#include	<MApplication.h>
#include	<strutil.h>
#include	<wxAdbEdit.h>
#include	<Mdefaults.h>
#include	<MDialogs.h>

#if 0
void
wxAdbEditPanel::Create(wxMFrame *parent, int x, int y, int width, int
		   height)
{
   wxPanel::Create(parent,x,y,width,height);
   SetLabelPosition(wxVERTICAL);

   wxLayoutConstraints *c;

   c = NEW wxLayoutConstraints;
   c->top.SameAs	(parent, wxTop);
   c->left.SameAs	(parent, wxLeft);
   c->right.SameAs	(parent, wxRight);
   c->bottom.SameAs	(parent, wxBottom);
   this->SetConstraints(c);
   
   wxMessage *nameLabel = NEW wxMessage(this,_("Key:"));
   c = NEW wxLayoutConstraints;
   c->top.SameAs        (this, wxTop, 5);
   c->left.SameAs	(this, wxLeft, 5);
   c->height.AsIs();
   c->width.AsIs();
   nameLabel->SetConstraints(c);

   key = NEW wxText(this,NULL,NULL,"");
   c = NEW wxLayoutConstraints;
   c->top.SameAs        (this, wxTop, 5);
   c->left.SameAs	(nameLabel, wxRight, 5);
   c->right.SameAs	(this, wxRight, 5);
   c->height.AsIs();
   key->SetConstraints(c);

   listBox = NEW wxListBox(this, (wxFunction) NULL, "",
			   wxSINGLE,
			   -1,-1,-1,-1,
			   0,
			   NULL,
			   wxALWAYS_SB);
   c = NEW wxLayoutConstraints;
   c->top.Below		(nameLabel);
   c->left.SameAs	(this, wxLeft, 5);
   c->height.AsIs();
   c->right.SameAs	(this, wxRight, 5);
   listBox->SetConstraints(c);


   wxMessage *fnLabel = NEW wxMessage(this,_("Formatted Name:"));
   c = NEW wxLayoutConstraints;
   c->top.Below		(listBox);
   c->left.SameAs	(this, wxLeft, 5);
   c->height.AsIs();
   c->width.AsIs();
   fnLabel->SetConstraints(c);

   formattedName = NEW wxText(this,NULL,NULL,"");
   c = NEW wxLayoutConstraints;
   c->top.Below		(fnLabel);
   c->left.SameAs	(this, wxLeft, 5);
   c->height.AsIs();
   c->right.SameAs	(this, wxRight, 5);
   formattedName->SetConstraints(c);

   wxMessage *fnPrLabel = NEW wxMessage(this,_("Prefix:"));
   wxMessage *fnFiLabel = NEW wxMessage(this,_("First"));
   wxMessage *fnOtLabel = NEW wxMessage(this,_("Other:"));
   wxMessage *fnFaLabel = NEW wxMessage(this,_("Family:"));
   wxMessage *fnPoLabel = NEW wxMessage(this,_("Postfix:"));
   
   strNamePrefix = NEW wxText(this,NULL,NULL,"");
   strNameFirst = NEW  wxText(this,NULL,NULL,"");
   strNameOther = NEW  wxText(this,NULL,NULL,"");
   strNameFamily = NEW  wxText(this,NULL,NULL,"");
   strNamePostfix = NEW  wxText(this,NULL,NULL,"");

   c = NEW wxLayoutConstraints;
   c->top.Below		(formattedName);
   c->left.SameAs	(this, wxLeft, 5);
   c->height.AsIs();
   c->width.AsIs();
   fnPrLabel->SetConstraints(c);

   c = NEW wxLayoutConstraints;
   c->top.Below		(fnPrLabel);
   c->left.SameAs	(fnPrLabel, wxLeft);
   c->height.AsIs();
   c->width.PercentOf	(this, wxWidth, 10);
   strNamePrefix->SetConstraints(c);
   
   c = NEW wxLayoutConstraints;
   c->top.Below		(formattedName);
   c->left.SameAs	(strNamePrefix, wxRight, 5);
   c->height.AsIs();
   c->width.AsIs();
   fnFiLabel->SetConstraints(c);

   c = NEW wxLayoutConstraints;
   c->top.Below		(fnFiLabel);
   c->left.SameAs	(fnFiLabel, wxLeft);
   c->height.AsIs();
   c->width.PercentOf	(this, wxWidth, 30);
   strNameFirst->SetConstraints(c);

   c = NEW wxLayoutConstraints;
   c->top.Below		(formattedName);
   c->left.SameAs	(strNameFirst, wxRight, 5);
   c->height.AsIs();
   c->width.AsIs();
   fnOtLabel->SetConstraints(c);

   c = NEW wxLayoutConstraints;
   c->top.Below		(fnOtLabel);
   c->left.SameAs	(fnOtLabel, wxLeft);
   c->height.AsIs();
   c->width.PercentOf	(this, wxWidth, 10);
   strNameOther->SetConstraints(c);

   c = NEW wxLayoutConstraints;
   c->top.Below		(formattedName);
   c->left.SameAs	(strNameOther, wxRight, 5);
   c->height.AsIs();
   c->width.AsIs();
   fnFaLabel->SetConstraints(c);

   c = NEW wxLayoutConstraints;
   c->top.Below		(fnFaLabel);
   c->left.SameAs	(fnFaLabel, wxLeft);
   c->height.AsIs();
   c->width.PercentOf	(this, wxWidth, 30);
   strNameFamily->SetConstraints(c);

   c = NEW wxLayoutConstraints;
   c->top.Below		(formattedName);
   c->left.SameAs	(strNameFamily, wxRight, 5);
   c->height.AsIs();
   c->width.AsIs();
   fnPoLabel->SetConstraints(c);

   c = NEW wxLayoutConstraints;
   c->top.Below		(fnPoLabel);
   c->left.SameAs	(fnPoLabel, wxLeft);
   c->height.AsIs();
   c->width.PercentOf	(this, wxWidth, 5);
   strNamePostfix->SetConstraints(c);

   wxMessage *tiLabel = NEW wxMessage(this,_("Title:"));
   c = NEW wxLayoutConstraints;
   c->top.Below		(strNamePostfix);
   c->left.SameAs	(this, wxLeft, 5);
   c->height.AsIs();
   c->width.AsIs();
   tiLabel->SetConstraints(c);
   
   title = NEW wxText(this,NULL,NULL,"");
   c = NEW wxLayoutConstraints;
   c->top.Below		(strNamePostfix);
   c->left.SameAs	(tiLabel, wxRight, 5);
   c->height.AsIs();
   c->width.AsIs();
   title->SetConstraints(c);

   wxMessage *orLabel = NEW wxMessage(this,_("Organisation:"));
   c = NEW wxLayoutConstraints;
   c->top.Below		(strNamePostfix);
   c->left.SameAs	(title, wxRight, 5);
   c->height.AsIs();
   c->width.AsIs();
   orLabel->SetConstraints(c);
   
   organisation = NEW wxText(this,NULL,NULL,"");
   c = NEW wxLayoutConstraints;
   c->top.Below		(strNamePostfix);
   c->left.SameAs	(orLabel, wxRight, 5);
   c->height.AsIs();
   c->width.AsIs();
   organisation->SetConstraints(c);

   wxMessage *hAddrLabel = NEW wxMessage(this,_("Home Address:"));
   c = NEW wxLayoutConstraints;
   c->top.Below		(title);
   c->left.SameAs	(this, wxLeft, 5);
   c->height.AsIs();   c->width.AsIs();
   title->SetConstraints(c);

   wxMessage *hApbLabel = NEW wxMessage(this,_("PO Box:"));
   c = NEW wxLayoutConstraints;
   c->top.Below		(hAddrLabel);
   c->left.SameAs	(this, wxLeft, 5);
   c->height.AsIs();   c->width.AsIs();
   title->SetConstraints(c);
   
   hApb = NEW wxText(this,NULL,NULL,"");
   c = NEW wxLayoutConstraints;
   c->top.Below		(hAddrLabel);
   c->left.SameAs	(hApbLabel, wxRight, 5);
   c->height.AsIs();   c->right.PercentOf(this, wxWidth, 40);
   title->SetConstraints(c);

   wxMessage *hAexLabel = NEW wxMessage(this,_("Extra:"));
   c = NEW wxLayoutConstraints;
   c->top.Below		(hApbLabel);
   c->left.SameAs	(this, wxLeft, 5);
   c->height.AsIs();   c->width.AsIs();
   title->SetConstraints(c);

   hAex = NEW wxText(this,NULL,NULL,"");
   c = NEW wxLayoutConstraints;
   c->top.Below		(hApbLabel);
   c->left.SameAs	(hAexLabel, wxRight, 5);
   c->height.AsIs();   c->right.PercentOf(this, wxWidth, 40);
   title->SetConstraints(c);

#if 0
   wxMessage *hAstLabel = NEW wxMessage(this,_("Street:"));
   c = NEW wxLayoutConstraints;
   c->top.Below		(hAexLabel);
   c->left.SameAs	(this, wxLeft, 5);
   c->height.AsIs();   c->width.AsIs();
   title->SetConstraints(c);

   hAst = NEW wxText(this,NULL,NULL,"");
      c = NEW wxLayoutConstraints;
   c->top.Below		(hAexLabel);
   c->left.SameAs	(hAstLabel, wxRight, 5);
   c->height.AsIs();   c->right.PercentOf(this, wxWidth, 40);
   title->SetConstraints(c);

   wxMessage *hAloLabel = NEW wxMessage(this,_("Locality:"));
   c = NEW wxLayoutConstraints;
   c->top.Below		(hAstLabel);
   c->left.SameAs	(this, wxLeft, 5);
   c->height.AsIs();   c->width.AsIs();
   title->SetConstraints(c);

   hAlo = NEW wxText(this,NULL,NULL,"");
      c = NEW wxLayoutConstraints;
   c->top.Below		(hAstLabel);
   c->left.SameAs	(hAloLabel, wxRight, 5);
   c->height.AsIs();   c->right.PercentOf(this, wxWidth, 40);
   title->SetConstraints(c);

   wxMessage *hAreLabel = NEW wxMessage(this,_("Region:"));
   c = NEW wxLayoutConstraints;
   c->top.Below		(hAloLabel);
   c->left.SameAs	(this, wxLeft, 5);
   c->height.AsIs();   c->width.AsIs();
   title->SetConstraints(c);

   hAre = NEW wxText(this,NULL,NULL,"");
      c = NEW wxLayoutConstraints;
   c->top.Below		(hAloLabel);
   c->left.SameAs	(hAreLabel, wxRight, 5);
   c->height.AsIs();   c->right.PercentOf(this, wxWidth, 40);
   title->SetConstraints(c);

   wxMessage *hApcLabel = NEW wxMessage(this,_("Postcode:"));
   c = NEW wxLayoutConstraints;
   c->top.Below		(hAreLabel);
   c->left.SameAs	(this, wxLeft, 5);
   c->height.AsIs();   c->width.AsIs();
   title->SetConstraints(c);

   hApc = NEW wxText(this,NULL,NULL,"");
      c = NEW wxLayoutConstraints;
   c->top.Below		(hAreLabel);
   c->left.SameAs	(hApcLabel, wxRight, 5);
   c->height.AsIs();   c->right.PercentOf(this, wxWidth, 40);
   title->SetConstraints(c);

   wxMessage *hAcoLabel = NEW wxMessage(this,_("Country:"));
   c = NEW wxLayoutConstraints;
   c->top.Below		(hApcLabel);
   c->left.SameAs	(this, wxLeft, 5);
   c->height.AsIs();   c->width.AsIs();
   title->SetConstraints(c);

   hAco = NEW wxText(this,NULL,NULL,"");
      c = NEW wxLayoutConstraints;
   c->top.Below		(hApcLabel);
   c->left.SameAs	(hAcoLabel, wxRight, 5);
   c->height.AsIs();   c->right.PercentOf(this, wxWidth, 40);
   title->SetConstraints(c);

   wxMessage *wAddrLabel = NEW wxMessage(this,_("Work Address:"));
   c = NEW wxLayoutConstraints;
   c->top.Below		(title);
   c->left.PercentOf	(this, wxWidth, 50);
   c->height.AsIs();   c->width.AsIs();
   title->SetConstraints(c);

   wxMessage *wApbLabel = NEW wxMessage(this,_("PO Box:"));
   c = NEW wxLayoutConstraints;
   c->top.Below		(wAddrLabel);
   c->left.SameAs	(wAddrLabel, wxLeft);
   c->height.AsIs();   c->width.AsIs();
   title->SetConstraints(c);
   
   wApb = NEW wxText(this,NULL,NULL,"");
   c = NEW wxLayoutConstraints;
   c->top.Below		(wAddrLabel);
   c->left.SameAs	(wApbLabel, wxRight, 5);
   c->height.AsIs();   c->right.PercentOf(this, wxWidth, 40);
   title->SetConstraints(c);

   wxMessage *wAexLabel = NEW wxMessage(this,_("Extra:"));
   c = NEW wxLayoutConstraints;
   c->top.Below		(wApbLabel);
   c->left.SameAs	(wAddrLabel, wxLeft);
   c->height.AsIs();   c->width.AsIs();
   title->SetConstraints(c);

   wAex = NEW wxText(this,NULL,NULL,"");
   c = NEW wxLayoutConstraints;
   c->top.Below		(wApbLabel);
   c->left.SameAs	(wAexLabel, wxRight, 5);
   c->height.AsIs();   c->right.PercentOf(this, wxWidth, 40);
   title->SetConstraints(c);

   wxMessage *wAstLabel = NEW wxMessage(this,_("Street:"));
   c = NEW wxLayoutConstraints;
   c->top.Below		(wAexLabel);
   c->left.SameAs	(wAddrLabel, wxLeft);
   c->height.AsIs();   c->width.AsIs();
   title->SetConstraints(c);

   wAst = NEW wxText(this,NULL,NULL,"");
      c = NEW wxLayoutConstraints;
   c->top.Below		(wAexLabel);
   c->left.SameAs	(wAstLabel, wxRight, 5);
   c->height.AsIs();   c->right.PercentOf(this, wxWidth, 40);
   title->SetConstraints(c);

   wxMessage *wAloLabel = NEW wxMessage(this,_("Locality:"));
   c = NEW wxLayoutConstraints;
   c->top.Below		(wAstLabel);
   c->left.SameAs	(wAddrLabel, wxLeft);
   c->height.AsIs();   c->width.AsIs();
   title->SetConstraints(c);

   wAlo = NEW wxText(this,NULL,NULL,"");
      c = NEW wxLayoutConstraints;
   c->top.Below		(wAstLabel);
   c->left.SameAs	(wAloLabel, wxRight, 5);
   c->height.AsIs();   c->right.PercentOf(this, wxWidth, 40);
   title->SetConstraints(c);

   wxMessage *wAreLabel = NEW wxMessage(this,_("Region:"));
   c = NEW wxLayoutConstraints;
   c->top.Below		(wAloLabel);
   c->left.SameAs	(wAddrLabel, wxLeft);
   c->height.AsIs();   c->width.AsIs();
   title->SetConstraints(c);

   wAre = NEW wxText(this,NULL,NULL,"");
      c = NEW wxLayoutConstraints;
   c->top.Below		(wAloLabel);
   c->left.SameAs	(wAreLabel, wxRight, 5);
   c->height.AsIs();   c->right.PercentOf(this, wxWidth, 40);
   title->SetConstraints(c);

   wxMessage *wApcLabel = NEW wxMessage(this,_("Postcode:"));
   c = NEW wxLayoutConstraints;
   c->top.Below		(wAreLabel);
   c->left.SameAs	(wAddrLabel, wxLeft);
   c->height.AsIs();   c->width.AsIs();
   title->SetConstraints(c);

   wApc = NEW wxText(this,NULL,NULL,"");
      c = NEW wxLayoutConstraints;
   c->top.Below		(wAreLabel);
   c->left.SameAs	(wApcLabel, wxRight, 5);
   c->height.AsIs();   c->right.PercentOf(this, wxWidth, 40);
   title->SetConstraints(c);

   wxMessage *wAcoLabel = NEW wxMessage(this,_("Country:"));
   c = NEW wxLayoutConstraints;
   c->top.Below		(wApcLabel);
   c->left.SameAs	(wAddrLabel, wxLeft);
   c->height.AsIs();   c->width.AsIs();
   title->SetConstraints(c);

   wAco = NEW wxText(this,NULL,NULL,"");
      c = NEW wxLayoutConstraints;
   c->top.Below		(wApcLabel);
   c->left.SameAs	(wAcoLabel, wxRight, 5);
   c->height.AsIs();   c->right.PercentOf(this, wxWidth, 40);
   title->SetConstraints(c);
   
   wxMessage *wPhLabel = NEW wxMessage(this,_("Work Phone:"));
   wxMessage *wPhCoLabel  = NEW wxMessage(this,_("Country:"));
   wPhCo = NEW wxText(this,NULL,NULL,"");
   wxMessage *wPhArLabel  = NEW wxMessage(this,_("Area:"));
   wPhAr = NEW wxText(this,NULL,NULL,"");
   wxMessage *wPhLoLabel  = NEW wxMessage(this,_("Local:"));
   wPhLo = NEW wxText(this,NULL,NULL,"");

   wxMessage *wFaxLabel = NEW wxMessage(this,_("Work Fax:"));
   wxMessage *wFaxCoLabel  = NEW wxMessage(this,_("Country:"));
   wFaxCo = NEW wxText(this,NULL,NULL,"");
   wxMessage *wFaxArLabel  = NEW wxMessage(this,_("Area:"));
   wFaxAr = NEW wxText(this,NULL,NULL,"");
   wxMessage *wFaxLoLabel  = NEW wxMessage(this,_("Local:"));
   wFaxLo = NEW wxText(this,NULL,NULL,"");


   wxMessage *hPhLabel = NEW wxMessage(this,_("Home Phone:"));
   wxMessage *hPhCoLabel  = NEW wxMessage(this,_("Country:"));
   hPhCo = NEW wxText(this,NULL,NULL,"");
   wxMessage *hPhArLabel  = NEW wxMessage(this,_("Area:"));
   hPhAr = NEW wxText(this,NULL,NULL,"");
   wxMessage *hPhLoLabel  = NEW wxMessage(this,_("Local:"));
   hPhLo = NEW wxText(this,NULL,NULL,"");

   wxMessage *hFaxLabel = NEW wxMessage(this,_("Home Fax:"));
   wxMessage *hFaxCoLabel  = NEW wxMessage(this,_("Country:"));
   hFaxCo = NEW wxText(this,NULL,NULL,"");
   wxMessage *hFaxArLabel  = NEW wxMessage(this,_("Area:"));
   hFaxAr = NEW wxText(this,NULL,NULL,"");
   wxMessage *hFaxLoLabel  = NEW wxMessage(this,_("Local:"));
   hFaxLo = NEW wxText(this,NULL,NULL,"");

   wxMessage *emailLabel = NEW wxMessage(this,_("Email:"));
   email = NEW wxText(this, NULL, NULL, "");
   emailOther = NEW wxListBox(this, (wxFunction) NULL, "",
			   wxSINGLE,
			   -1,-1,-1,-1,
			   0,
			   NULL,
			   wxALWAYS_SB);
   
   wxMessage *aliasLabel = NEW wxMessage(this,_("Alias:"));
   alias = NEW wxText(this, NULL, NULL, "");

   wxMessage *urlLabel = NEW wxMessage(this,_("URL:"));
   url = NEW wxText(this, NULL, NULL, "");
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
	 frame->Load(mApplication.GetAdb()->Lookup(key->GetValue(), frame));
      }
   }
   else if(strcmp(win.GetName(), "expandButton") == 0)
	   frame->Load(mApplication.GetAdb()->Lookup(key->GetValue(), frame));
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

   wxMessage *nameLabel = NEW wxMessage(this,_("Key:"));
   key = NEW wxText(this,NULL,NULL,"", -1,-1,w/2,-1,0,"keyField");
   NEW wxButton(this,NULL,_("Expand"), -1,-1,-1,-1,0,"expandButton");
   NEW wxButton(this,NULL,_("New"), -1,-1,-1,-1,0,"newButton");
   NEW wxButton(this,NULL,_("Delete"),-1,-1,-1,-1,0,"delButton");
   NEW wxButton(this,NULL,_("Save"), -1,-1,-1,-1,0,"saveButton");
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
   
   Show(False);
   
   fileMenu = NEW wxMenu;
   fileMenu->Append(WXMENU_FILE_CLOSE,WXCPTR _("&Close"));
   fileMenu->AppendSeparator();
   fileMenu->Append(WXMENU_FILE_EXIT,WXCPTR _("&Exit"));

   menuBar = NEW wxMenuBar;
   menuBar->Append(fileMenu, WXCPTR _("&File"));

   SetMenuBar(menuBar);

   int	width, height;
   this->GetClientSize(&width, &height);
   wxPanel *panel = NEW wxAdbEditPanel(this,0,0,width,height);
   view = NEW wxPanelTabView(panel, wxTAB_STYLE_DRAW_BOX);
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
   wxPanel *panel1 = NEW wxPanel(panel, rect.x + 20, rect.y + 10,
				 pwidth, pheight);
   NEW wxMessage(panel1, _("Formatted Name:"));
   formattedName = NEW wxText(panel1,NULL,NULL,"",fieldpos,-1,fieldwidth);
   panel1->NewLine();
   NEW wxMessage(panel1,_("Prefix:"));
   strNamePrefix = NEW wxText(panel1,NULL,NULL,"",fieldpos,-1,fieldwidth);
   panel1->NewLine();
   wxMessage *fnFiLabel = NEW wxMessage(panel1,_("First"));
   strNameFirst = NEW  wxText(panel1,NULL,NULL,"", fieldpos,-1,fieldwidth);
   panel1->NewLine();
   wxMessage *fnOtLabel = NEW wxMessage(panel1,_("Other:"));
   strNameOther = NEW wxText(panel1,NULL,NULL,"",fieldpos,-1,fieldwidth);
   panel1->NewLine();
   wxMessage *fnFaLabel = NEW wxMessage(panel1,_("Family:"));
   strNameFamily = NEW wxText(panel1,NULL,NULL,"",fieldpos,-1,fieldwidth);
   panel1->NewLine();
   wxMessage *fnPoLabel = NEW wxMessage(panel1,_("Postfix:"));
   strNamePostfix = NEW wxText(panel1,NULL,NULL,"",fieldpos,-1,fieldwidth);
   panel1->NewLine();
   wxMessage *tiLabel = NEW wxMessage(panel1,_("Title:"));
   title = NEW wxText(panel1,NULL,NULL,"",fieldpos,-1,fieldwidth);
   panel1->NewLine();
   wxMessage *orgLabel = NEW wxMessage(panel1,_("Organisation:"));
   organisation = NEW wxText(panel1,NULL,NULL,"",fieldpos,-1,fieldwidth);
   panel1->NewLine();
   view->AddTabPanel(TAB_NAME, panel1);

   
   // Panel 2 - home address
   wxPanel *panel2 = NEW wxPanel(panel, rect.x + 20, rect.y + 10,
				 pwidth, pheight);
   wxMessage *hApbLabel = NEW wxMessage(panel2,_("PO Box:"));
   hApb = NEW wxText(panel2,NULL,NULL,"",fieldpos,-1,fieldwidth);
   panel2->NewLine();
   wxMessage *hAexLabel = NEW wxMessage(panel2,_("Extra:"));
   hAex = NEW wxText(panel2,NULL,NULL,"",fieldpos,-1,fieldwidth);
   panel2->NewLine();
   wxMessage *hAstLabel = NEW wxMessage(panel2,_("Street:"));
   hAst = NEW wxText(panel2,NULL,NULL,"",fieldpos,-1,fieldwidth);
   panel2->NewLine();
   wxMessage *hAloLabel = NEW wxMessage(panel2,_("Locality:"));
   hAlo = NEW wxText(panel2,NULL,NULL,"",fieldpos,-1,fieldwidth);
   panel2->NewLine();
   wxMessage *hAreLabel = NEW wxMessage(panel2,_("Region:"));
   hAre = NEW wxText(panel2,NULL,NULL,"",fieldpos,-1,fieldwidth);
   panel2->NewLine();
   wxMessage *hApcLabel = NEW wxMessage(panel2,_("Postcode:"));
   hApc = NEW wxText(panel2,NULL,NULL,"",fieldpos,-1,fieldwidth);
   panel2->NewLine();
   wxMessage *hAcoLabel = NEW wxMessage(panel2,_("Country:"));
   hAco = NEW wxText(panel2,NULL,NULL,"",fieldpos,-1,fieldwidth);
   panel2->NewLine();
   wxMessage *hPhLabel = NEW wxMessage(panel2,_("Telephone:"));
   hPh = NEW wxText(panel2,NULL,NULL,"",fieldpos,-1,fieldwidth);
   panel2->NewLine();
   wxMessage *hFaxLabel = NEW wxMessage(panel2,_("Telefax:"));
   hFax = NEW wxText(panel2,NULL,NULL,"",fieldpos,-1,fieldwidth);
   panel2->NewLine();
   
   view->AddTabPanel(TAB_HOME_ADDRESS, panel2);


   // Panel 3 - work address
   wxPanel *panel3 = NEW wxPanel(panel, rect.x + 20, rect.y + 10,
				 pwidth, pheight);
   wxMessage *wApbLabel = NEW wxMessage(panel3,_("PO Box:"));
   wApb = NEW wxText(panel3,NULL,NULL,"",fieldpos,-1,fieldwidth);
   panel3->NewLine();
   wxMessage *wAexLabel = NEW wxMessage(panel3,_("Extra:"));
   wAex = NEW wxText(panel3,NULL,NULL,"",fieldpos,-1,fieldwidth);
   panel3->NewLine();
   wxMessage *wAstLabel = NEW wxMessage(panel3,_("Street:"));
   wAst = NEW wxText(panel3,NULL,NULL,"",fieldpos,-1,fieldwidth);
   panel3->NewLine();
   wxMessage *wAloLabel = NEW wxMessage(panel3,_("Locality:"));
   wAlo = NEW wxText(panel3,NULL,NULL,"",fieldpos,-1,fieldwidth);
   panel3->NewLine();
   wxMessage *wAreLabel = NEW wxMessage(panel3,_("Region:"));
   wAre = NEW wxText(panel3,NULL,NULL,"",fieldpos,-1,fieldwidth);
   panel3->NewLine();
   wxMessage *wApcLabel = NEW wxMessage(panel3,_("Postcode:"));
   wApc = NEW wxText(panel3,NULL,NULL,"",fieldpos,-1,fieldwidth);
   panel3->NewLine();
   wxMessage *wAcoLabel = NEW wxMessage(panel3,_("Country:"));
   wAco = NEW wxText(panel3,NULL,NULL,"",fieldpos,-1,fieldwidth);
   panel3->NewLine();
   wxMessage *wPhLabel = NEW wxMessage(panel3,_("Telephone:"));
   wPh = NEW wxText(panel3,NULL,NULL,"",fieldpos,-1,fieldwidth);
   panel3->NewLine();
   wxMessage *wFaxLabel = NEW wxMessage(panel3,_("Telefax:"));
   wFax = NEW wxText(panel3,NULL,NULL,"",fieldpos,-1,fieldwidth);
   panel3->NewLine();
   view->AddTabPanel(TAB_WORK_ADDRESS, panel3);
   
   // Panel 4 - email
   wxPanel *panel4 = NEW wxPanel(panel, rect.x + 20, rect.y + 10,
				 pwidth, pheight);
   wxMessage *emailLabel = NEW wxMessage(panel4,_("e-mail:"));
   email = NEW wxText(panel4,NULL,NULL,"",fieldpos,-1,fieldwidth);
   panel4->NewLine();
   wxMessage *aliasLabel = NEW wxMessage(panel4,_("alias:"));
   alias = NEW wxText(panel4,NULL,NULL,"",fieldpos,-1,fieldwidth);
   panel4->NewLine();
   wxMessage *urlLabel = NEW wxMessage(panel4,_("URL:"));
   url = NEW wxText(panel4,NULL,NULL,"",fieldpos,-1,fieldwidth);
   panel4->NewLine();
   wxMessage *aeLabel = NEW wxMessage(panel4,_("alternative e-mail addresses:"));
   panel4->NewLine();
   listBox = NEW wxListBox(panel4, (wxFunction) NULL, "",
			   wxSINGLE,
			   -1,-1,pwidth*0.8,-1,
			   0,
			   NULL,
			   wxALWAYS_SB);
   
   panel4->NewLine();
   view->AddTabPanel(TAB_EMAIL, panel4);


   // Don't know why this is necessary under Motif...
   //dialog->SetSize(dialogWidth, dialogHeight-20);

   view->SetTabSelection(TAB_NAME);
   
//   Fit();
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
      DELETE this;
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
