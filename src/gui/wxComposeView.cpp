/*-*- c++ -*-********************************************************
 * wxComposeView.cc : a wxWindows look at a message                 *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.2  1998/03/16 18:22:43  karsten
 * started integration of python, fixed bug in wxFText/word wrapping
 *
 * Revision 1.1  1998/03/14 12:21:21  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifdef __GNUG__
#pragma	implementation "wxComposeView.h"
#endif

#include	<wxFTCanvas.h>
#include	<Mcommon.h>
#include	<MApplication.h>
#include	<MessageCC.h>
#include	<wxComposeView.h>
#include	<strutil.h>
#include	<Mdefaults.h>
#include	<SendMessageCC.h>
#include	<MDialogs.h>
#include	<wxAdbEdit.h>

// for testing only

#include	<MessageCC.h>

extern "C"
{
#include	<rfc822.h>
}

IMPLEMENT_DYNAMIC_CLASS(wxComposeView, wxMFrame)



void
wxComposeView::Create(const String &iname, wxFrame *parent,
		      ProfileBase *parentProfile,
		      String const &to, String const &cc, String const
   &bcc, bool hide)
{
   if(initialised)
      return; // ERROR!

   const char
      *cto = NULL,
      *ccc  = NULL,
      *cbcc = NULL;

   ftCanvas = NULL;
   nextFileID = 0;
   
   wxMFrame::Create(iname, parent);
   
   if(!parentProfile)
      parentProfile = mApplication.GetProfile();
   profile = new Profile(iname,parentProfile);

   cto = profile->readEntry(MP_COMPOSE_TO, MP_COMPOSE_TO_D);
   ccc = profile->readEntry(MP_COMPOSE_CC, MP_COMPOSE_CC_D);
   cbcc = profile->readEntry(MP_COMPOSE_BCC, MP_COMPOSE_BCC_D);
   
   if(to.length())
      cto = to.c_str();
   if(cc.length())
      ccc = cc.c_str();
   if(bcc.length())
      cbcc = bcc.c_str();
   
   AddMenuBar();
   AddFileMenu();
  
   composeMenu = new wxMenu;
   composeMenu->Append(WXMENU_COMPOSE_INSERTFILE, WXCPTR _("Insert &File"));
   composeMenu->Append(WXMENU_COMPOSE_SEND,WXCPTR _("&Send"));
   composeMenu->Append(WXMENU_COMPOSE_PRINT,WXCPTR _("&Print"));
   composeMenu->AppendSeparator();
   composeMenu->Append(WXMENU_COMPOSE_CLEAR,WXCPTR _("&Clear"));
   menuBar->Append(composeMenu, WXCPTR _("&Compose"));

   AddHelpMenu();
   SetMenuBar(menuBar);

   SetAutoLayout(TRUE);
   
   wxItem		*last;
   wxLayoutConstraints	*c;
   
   panel    = new wxPanel(this,0,0,1000,500,wxNO_DC|wxBORDER, "MyPanel");
   panel->SetLabelPosition(wxVERTICAL);

   // Create some panel items
   txtToLabel = new wxMessage(panel,_("To:"));
   txtTo = new wxText(panel,NULL,NULL,"",-1,-1,-1,-1,0,"toField");


   aliasButton = new wxButton(panel, NULL, _("Expand"));
   
   if(profile->readEntry(MP_SHOWCC,MP_SHOWCC_D))
   {
      txtCCLabel = new wxMessage(panel,_("CC:"));
      txtCC = new wxText(panel,NULL,NULL,"");
   }
   if(profile->readEntry(MP_SHOWBCC,MP_SHOWBCC_D))
   {
      txtBCCLabel = new wxMessage(panel,_("BCC:"));
      txtBCC = new wxText(panel,NULL,NULL);
   }
   txtSubjectLabel = new wxMessage(panel,_("Subject:"));
   txtSubject = new wxText(panel,NULL,NULL,"Subject");

   // with the constraints, I assume that "Subject" is the longest label
   c = new wxLayoutConstraints;
   c->top.SameAs        (panel, wxTop, 5);
   c->right.SameAs	(txtSubjectLabel, wxRight);
   c->height.AsIs();
   c->width.AsIs();
   txtToLabel->SetConstraints(c);

   c = new wxLayoutConstraints;
   c->top.SameAs        (panel, wxTop, 5);
   c->left.SameAs	(txtSubjectLabel, wxRight, 5);
   c->right.SameAs	(aliasButton, wxLeft, 5);
   c->height.AsIs(); 
   txtTo->SetConstraints(c);

   c = new wxLayoutConstraints;
   c->top.SameAs        (panel, wxTop, 5);
   c->right.SameAs	(panel, wxRight, 25);
   c->width.AsIs();
   c->height.AsIs(); 
   aliasButton->SetConstraints(c);

   last = txtTo;
   // optional CC line
   if(profile->readEntry(MP_SHOWCC,MP_SHOWCC_D))
   {
      c = new wxLayoutConstraints;
      c->top.Below  		(txtToLabel);
      c->right.SameAs	(txtSubjectLabel, wxRight);
      c->height.AsIs();
      c->width.AsIs();
      txtCCLabel->SetConstraints(c);

      c = new wxLayoutConstraints;
      c->top.Below		(txtToLabel);
      c->left.SameAs		(txtSubjectLabel, wxRight, 5);
      c->right.SameAs		(panel, wxRight, 5);
      c->height.AsIs(); 
      txtCC->SetConstraints(c);
      last = txtCC;
   }

   // optional BCC line
   if(profile->readEntry(MP_SHOWBCC,MP_SHOWBCC_D))
   {
      c = new wxLayoutConstraints;
      c = new wxLayoutConstraints;
      c->top.Below  		(last);
      c->right.SameAs		(txtSubjectLabel, wxRight);
      c->height.AsIs();
      c->width.AsIs();
      txtBCCLabel->SetConstraints(c);

      c = new wxLayoutConstraints;
      c->top.Below		(last);
      c->left.SameAs		(txtSubjectLabel, wxRight, 5);
      c->right.SameAs		(panel, wxRight, 5);
      c->height.AsIs(); 
      txtBCC->SetConstraints(c);
      last = txtBCC;
   }

   // Subject line
   c = new wxLayoutConstraints;
   c->top.SameAs		(last, wxBottom, 5); //Below  		(last);
   c->left.SameAs		(panel, wxLeft, 5);
   c->height.AsIs();
   c->width.AsIs();
   txtSubjectLabel->SetConstraints(c);
   c = new wxLayoutConstraints;
   c->top.Below			(last);
   c->left.SameAs		(txtSubjectLabel, wxRight, 5);
   c->right.SameAs		(panel, wxRight, 5);
   c->height.AsIs(); 
   txtSubject->SetConstraints(c);
   last = txtSubjectLabel;

   // Panel itself
   c = new wxLayoutConstraints;
   c->left.SameAs       (this, wxLeft);
   c->top.SameAs	(this, wxTop);
   c->right.SameAs	(this, wxRight);
   c->height.SameAs  	(this, wxHeight);
   panel->SetConstraints(c);

   CreateFTCanvas();
   
   if(profile->readEntry(MP_COMPOSE_USE_SIGNATURE,MP_COMPOSE_USE_SIGNATURE_D))
   {
      size_t size;
      ifstream istr;
      char *buffer;
      istr.open(profile->readEntry(MP_COMPOSE_SIGNATURE,MP_COMPOSE_SIGNATURE_D));
      if(istr)
      {
	 istr.seekg(0,ios::end);
	 size = istr.tellg();
	 buffer = new char [size+1];
	 if(profile->readEntry(MP_COMPOSE_USE_SIGNATURE_SEPARATOR,MP_COMPOSE_USE_SIGNATURE_SEPARATOR_D))
	    ftCanvas->AddFormattedText("--\n");
	 istr.seekg(0,ios::beg);
	 istr.read(buffer, size);
	 buffer[size] = '\0';
	 if(! istr.fail())
	    ftCanvas->AddText(buffer);
	 delete [] buffer;
	 istr.close();
	 ftCanvas->MoveCursorTo(0,0);
      }	
   }
   
   initialised = true;
   if(! hide)
      Show(TRUE);
//   txtTo->SetFocus();
}

void
wxComposeView::CreateFTCanvas(void)
{
   ftCanvas = new wxFTCanvas(panel);
   // Canvas
   wxLayoutConstraints *c = new wxLayoutConstraints;
   c->left.SameAs       (panel, wxLeft);
   c->top.SameAs	(txtSubjectLabel, wxBottom, 10);
   c->right.SameAs      (panel, wxRight);
   c->bottom.SameAs	(panel, wxBottom);
   ftCanvas->SetConstraints(c);
   ftCanvas->AllowEditing(true);
   ftCanvas->SetWrapMargin(
      profile->readEntry(MP_COMPOSE_WRAPMARGIN,MP_COMPOSE_WRAPMARGIN_D));
}

void
wxComposeView::OnSize(int  w, int h)
{
   Layout();
}

wxComposeView::wxComposeView(const String &iname, wxFrame *parent,
			     ProfileBase *parentProfile, bool hide)
{
   initialised = false;
   Create(iname,parent,parentProfile, "","","",hide);
}

wxComposeView::~wxComposeView()
{
   if(! initialised)
      return;
   delete ftCanvas;
}


void
wxComposeView::ProcessMouse(wxMouseEvent &event)
{
}

void
wxComposeView::OnCommand(wxWindow &win, wxCommandEvent &event)
{
   // this gets called when the Expand button is pressed
   Adb * adb = mApplication.GetAdb();
   AdbEntry *entry = NULL;

   String	tmp = txtTo->GetValue();
   int l = tmp.length();

   if(
      (strcmp(win.GetName(),"toField")==0
	 && tmp.substr(l-1,1) == " ")
      ||
      strcmp(win.GetName(),"button") == 0)
   {
      const char *expStr = NULL;
      if(l > 0)
      {
	 tmp = tmp.substr(0,l-1);
	 txtTo->SetValue(WXCPTR tmp.c_str());
	 expStr = strrchr(tmp.c_str(),',');
	 if(expStr)
	 {
	    tmp = tmp.substr(0,l-strlen(expStr));
	    expStr ++;
	 }
	 else
	    expStr = tmp.c_str();
	 entry = adb->Lookup(expStr, this);
	 if(! entry)
	    wxBell();
      }
      if(entry)
      {
	 String
	    tmp2 = entry->formattedName + String(" <")
	    + entry->email.preferred.c_str() + String(">");
	 
	 if(expStr == tmp.c_str())
	    tmp = tmp2;
	 else
	 {
	    tmp += ',';
	    tmp += tmp2;
	 }
	 txtTo->SetValue(WXCPTR tmp.c_str());
      }
   }
}

void
wxComposeView::OnMenuCommand(int id)
{
   switch(id)
   {
   case WXMENU_COMPOSE_INSERTFILE:
      InsertFile();
      break;
   case WXMENU_COMPOSE_SEND:
      Send();
      break;
   case WXMENU_COMPOSE_PRINT:
      Print();
      break;
   case WXMENU_COMPOSE_CLEAR:
      delete ftCanvas;
      CreateFTCanvas();
      Layout();
      OnPaint();
      break;
   default:
      wxMFrame::OnMenuCommand(id);
   }
}
void
wxComposeView::InsertFile(void)
{
   String
      mimeType,
      tmp,
      fileName;
   int
      numericType;
   
   const char
      *filename = MDialog_FileRequester(NULL,this,NULL,NULL,NULL,NULL,true,profile);

   if(! filename)
      return;

   fileName = filename;
   
   fileMap[nextFileID] = fileName;

   numericType = TYPEAPPLICATION;
   if(! mApplication.GetMimeTypes()->Lookup(fileName, mimeType, &numericType))
      mimeType = "APPLICATION/OCTET-STREAM";
   tmp = String("<IMG SRC=\"") + mimeType+ String("\" ") + String(";")
      + mimeType + String (";")  + strutil_ltoa(numericType)
      + String(";") +  strutil_ultoa(nextFileID) + String(">");

   nextFileID ++;
   
   ftCanvas->InsertText(tmp,true);
   ftCanvas->MoveCursor(1,0);
   ftCanvas->SetFocus();   
}

void
wxComposeView::Send(void)
{
   String const
      * tmp;
   String
      tmp2, mimeType, mimeSubType;
   FTObjectType
      ftoType;
   char
      *cptr, *ocptr,
      *buffer;
   ifstream
      istr;
   size_t
      size;
   int
      numMimeType;
   
   SendMessageCC sm( mApplication.GetProfile(),
		     txtSubject->GetValue(),
		     txtTo->GetValue(),
		     profile->readEntry(MP_SHOWCC,MP_SHOWCC_D) ?
		     txtCC->GetValue() : NULL,
		     profile->readEntry(MP_SHOWBCC,MP_SHOWBCC_D) ?
		     txtBCC->GetValue() : NULL);
   tmp = ftCanvas->GetContent(&ftoType, true);
   while(ftoType != LI_ILLEGAL)
   {
      switch(ftoType)
      {
      case LI_TEXT:
	 sm.AddPart(TYPETEXT,tmp->c_str(),tmp->size());
	 break;
      case LI_ICON:
	 cptr = ocptr = strutil_strdup(tmp->c_str()); // <IMG SRC="xxx"; mimetype; numMimeType;fileID >
	 cptr = strtok(cptr,";"); // IMG
	 cptr = strtok(NULL,";"); // mimetype
	 mimeType = cptr;
	 mimeSubType = strutil_after(mimeType,'/');
	 mimeType = strutil_before(mimeType, '/');
	 cptr = strtok(NULL,";"); // numericMimeType
	 numMimeType = atoi(cptr);
	 cptr = strtok(NULL,";"); // fileID
	 istr.open(fileMap[atol(cptr)].c_str());
	 if(istr)
	 {
 	    istr.seekg(0,ios::end);
	    size = istr.tellg();
	    buffer = new char [size];
 	    istr.seekg(0,ios::beg);
	    istr.read(buffer, size);
	    
	    if(! istr.fail())
	       sm.AddPart(numMimeType, buffer, size, mimeSubType);
	    else
	       SYSERRMESSAGE((_("Cannot read file."),this));
	    delete [] buffer;
	    istr.close();
	 }
	 delete [] ocptr;
	 break;
      case LI_ILLEGAL:
	 break;
      default:
	 break;
      }
      tmp = ftCanvas->GetContent(&ftoType, false);
   }
   sm.Send();
}

/// sets To field
void
wxComposeView::SetTo(const String &to)
{
   txtTo->SetValue(WXCPTR to.c_str());
}
   
/// sets CC field
void
wxComposeView::SetCC(const String &cc)
{
   txtCC->SetValue(WXCPTR cc.c_str());
}

/// sets Subject field
void
wxComposeView::SetSubject(const String &subj)
{
   txtSubject->SetValue(WXCPTR subj.c_str());
}

/// inserts a text
void
wxComposeView::InsertText(const String &txt)
{
   ftCanvas->MoveCursorTo(0,0);
   ftCanvas->InsertText(txt);
   ftCanvas->MoveCursorTo(0,0);
}


void
wxComposeView::Print(void)
{
   ftCanvas->Print();
}
