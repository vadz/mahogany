/*-*- c++ -*-********************************************************
 * wxMessageView.cc : a wxWindows look at a message                 *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.4  1998/03/26 23:05:42  VZ
 * Necessary changes to make it compile under Windows (VC++ only)
 * Header reorganization to be able to use precompiled headers
 *
 * Revision 1.1  1998/03/14 12:21:22  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifdef __GNUG__
#pragma	implementation "wxMessageView.h"
#endif

#include	"Mpch.h"
#include	"Mcommon.h"

#if       !USE_PCH
  #include	<strutil.h>
  // for testing only

  #include	<MessageCC.h>

  extern "C"
  {
    #include	<rfc822.h>
  }
#endif

#include	"MFrame.h"
#include	"MLogFrame.h"

#include	"Mdefaults.h"

#include	"PathFinder.h"
#include	"MimeList.h"
#include	"MimeTypes.h"
#include	"Profile.h"

#include  "MApplication.h"

#include  "FolderView.h"
#include	"MailFolder.h"
#include	"MailFolderCC.h"
#include	"Message.h"

#include  "Adb.h"
#include	"MDialogs.h"

#include	"MessageView.h"

#include  "XFace.h"

#include  "gui/wxFontManager.h"
#include  "gui/wxIconManager.h"
#include	"gui/wxFText.h"
#include	"gui/wxMessageView.h"

//FIXME
static char *Icondata[200];
static void popup_callback(wxMenu& menu, wxCommandEvent& ev);

IMPLEMENT_CLASS(wxMessageView, wxMFrame)
IMPLEMENT_CLASS(wxMessageViewPanel, wxPanel)

class myPopup : public wxMenu
{
protected:
   wxFrame *popup_parent;
   int	    menu_offset;	// all selections get added to this and passed
			                    // to popup_parent->OnMenuCommand(offset+entry)

   friend void popup_callback(wxMenu& menu, wxCommandEvent& ev);

public:
   myPopup(const char *name, wxFrame *frame, int offset)
      : wxMenu((char *)name, (wxFunction) &popup_callback)
      {
         popup_parent = frame;
         menu_offset  = offset;
      }
};

static void popup_callback(wxMenu& menu, wxCommandEvent& ev)
{
  myPopup *popup = (myPopup *)&menu;
#ifdef  USE_WXWINDOWS2
  // @@@@ OnMenuCommand
#else
  popup->popup_parent->OnMenuCommand(popup->menu_offset + ev.GetSelection());
#endif
}


class wxMVCanvas : public wxCanvas
{
   wxFTOList 		* ftol;
   wxMessageView	* mv;
public:
   wxMVCanvas(wxFTOList *iftol, wxMessageView *imv)
#ifdef  USE_WXWINDOWS2
      : wxCanvas(*imv)    // @@@@ wxCanvas
#else
      : wxCanvas(imv)
#endif
      { ftol = iftol; mv = imv; }
   void OnPaint(void) { ftol->Draw(); }
   void OnEvent(wxMouseEvent &event)
      { mv->ProcessMouse(event); }
};


void
wxMessageView::Create(const String &iname, wxFrame *parent)
{
   if(initialised)
      return; // ERROR!
   mailMessage = NULL;
   mimeDisplayPart = 0;
   xface = NULL;
   
   wxMFrame::Create(iname, parent);

   AddMenuBar();
   AddFileMenu();
       
   messageMenu = GLOBAL_NEW wxMenu;
   messageMenu->Append(WXMENU_MSG_REPLY,(char *)_("&Reply"));
   messageMenu->Append(WXMENU_MSG_FORWARD,(char *)_("&Forward"));
   messageMenu->Append(WXMENU_MSG_PRINT,(char *)_("&Print"));
   menuBar->Append(messageMenu, (char *)_("&Message"));

   AddHelpMenu();
   SetMenuBar(menuBar);
   
   popupMenu = GLOBAL_NEW myPopup(_("MIME Menu"),this, WXMENU_POPUP_MIME_OFFS);
   popupMenu->Append(WXMENU_MIME_INFO,(char *)_("&Info"));
   popupMenu->Append(WXMENU_MIME_HANDLE,(char *)_("&Handle"));
   popupMenu->Append(WXMENU_MIME_SAVE,(char *)_("&Save"));
   
   ftoList = GLOBAL_NEW wxFTOList();
   ftCanvas = GLOBAL_NEW wxMVCanvas(ftoList,this);

   #ifdef  USE_WXWINDOWS2
     // @@@@ GetDC
   #else
     ftoList->SetDC(ftCanvas->GetDC());
   #endif
   
   folder = NULL;
   initialised = true;
}

wxMessageView::wxMessageView(const String &iname, wxFrame *parent)
{
   initialised = false;
   Create(iname,parent);
   Show(TRUE);
}

wxMessageView::wxMessageView(MailFolder *ifolder,
			      long num,
			      const String &iname,
			      wxFrame *parent)
{
   initialised = false;
   Create(iname,parent);
   folder = ifolder;
   ShowMessage(folder,num);

   String
      email, name;
   email = mailMessage->Address(name,MAT_FROM);
   mApplication.GetAdb()->UpdateEntry(email, name);
   Show(TRUE);
}


void
wxMessageView::Update(void)
{
   float	width, height;
   int i,n,t;
   char const * cptr;
   String	tmp,from;
   bool		lastObjectWasIcon = false; // a flag
   
   ftoList->AddFormattedText(_("<bf>From:</bf> "));
   from = mailMessage->Address(tmp,MAT_FROM);
   if(tmp.length() > 0)
      from = tmp + String(" <") + from + '>';
   ftoList->AddText(from);
   ftoList->AddFormattedText(_("\n<bf>Subject:</bf> "));
   ftoList->AddText(mailMessage->Subject());

   // need real XPM support
   #if  USE_XPM_IN_MSW
     if(folder->GetProfile() &&
        folder->GetProfile()->readEntry(MP_SHOW_XFACES, MP_SHOW_XFACES_D))
     {
        mailMessage->GetHeaderLine("X-Face", tmp);
        if(tmp.length() > 2)   //\r\n
        {
          xface = GLOBAL_NEW XFace();
          tmp = tmp.c_str()+strlen("X-Face:");
          xface->CreateFromXFace(tmp.c_str());
          xface->CreateXpm(tmp);
          char **ipm = XFace::SplitXpm(tmp);
          char *line;
          int i;
          for(i = 0; ipm[i]; i++)
          {
            icondata[i] = ipm[i];
            line = Icondata[i];
            VAR(line);
          }
     ftoList->AddIcon("XFACE", Icondata);
     ftoList->AddFormattedText(" <bf>X-Face:</bf><IMG SRC=\"XFACE\">\n");
     //FIXME: free allocated arrays
        }
     }  
   #endif

   ftoList->AddFormattedText(_("\n<bf>Date:</bf> "));
   ftoList->AddText(mailMessage->Date());
   ftoList->AddText("\n\n");


   n = mailMessage->CountParts();
   VAR(n);
   for(i = 0; i < n; i++)
   {
      t = mailMessage->GetPartType(i);
      if(mailMessage->GetPartSize(i))
      {
	 if(t == TYPETEXT || (t == TYPEMESSAGE && folder->GetProfile()
			      && folder->GetProfile()->
			      readEntry(MP_RFC822_IS_TEXT,MP_RFC822_IS_TEXT_D)))
	 {
	    cptr = mailMessage->GetPartContent(i);
	    if(cptr)
	    {
	       ftoList->AddText("\n");
	       ftoList->AddText(cptr);
	       lastObjectWasIcon = false;
	    }
	 }
	 else
	 {
	    tmp = String("<IMG SRC=\"") + mailMessage->GetPartMimeType(i) + String("\" ")
	       + String(";") + mailMessage->GetPartMimeType(i) + String(";") +
	       strutil_ltoa(i) + String(">");
	    ftoList->AddFormattedText(tmp);
	    lastObjectWasIcon = true;
	 }
      }
   }

   ftoList->AddText("\n");
   ftoList->ReCalculateLines(&width, &height);

   #ifdef  USE_WXWINDOWS2
      // @@@@ SetScrollbar?
   #else
      ftCanvas->SetScrollbars(20,20,(int)width/20,(int)(height*1.2)/20,10,10); 
   #endif
}

wxMessageView::~wxMessageView()
{
   if(! initialised)
      return;
   if(mailMessage)
      GLOBAL_DELETE mailMessage;
   if(xface)
      GLOBAL_DELETE xface;
   if(popupMenu)
      GLOBAL_DELETE	popupMenu;
   GLOBAL_DELETE ftCanvas;
   //fixme:
   GLOBAL_DELETE ftoList;
}


void
wxMessageView::ProcessMouse(wxMouseEvent &event)
{
   int x,y;
   FTObject	const * obj;
   
   if(event.RightDown())
   {
      #ifdef  USE_WXWINDOWS2
        x = y = 0; // @@@@ ViewStart?  
      #else
        ftCanvas->ViewStart(&x,&y);
      #endif
      obj = ftoList->FindClickable(event.GetX() - x, event.GetY() - y);
      if(obj)
      {
	 if(obj->GetType() == LI_ICON)
	 {
	    String	command, flags;
	    // mime type is stored after 1. semicolon:
	    // "IMG SRC="mimetype";mimetype;# of section"
	    String	type = obj->GetText();
	    char *buf = strutil_strdup(type);
	    char *token = strtok(buf,";");
	    token = strtok(NULL,";");
	    if(!token) abort(); // EH???
	    type = token;
	    token = strtok(NULL,";");
	    if(!token) abort(); // EH???
	    mimeDisplayPart = atoi(token); // remember section to use it in
				     // Popup menu
	    GLOBAL_DELETE [] buf;
	    VAR(type);
	    VAR(mApplication.GetMimeList()->GetCommand(type,command,flags));
	    VAR(command);
	    VAR(flags);

	    PopupMenu(popupMenu, event.GetX() - x, event.GetY() - y);

	 }
	 else if (obj->GetType() == LI_URL)
	 {
	    ERRORMESSAGE(("Object Type LI_URL not implemented yet."));
	 }
      }
   }
   // else do nothing
}


void
wxMessageView::Print(void)
{
#ifdef  OS_UNIX
   // set AFM path (recursive!)
   PathFinder	pf(mApplication.readEntry(MC_AFMPATH,MC_AFMPATH_D), true);
   pf.AddPaths(mApplication.GetGlobalDir(), true);
   pf.AddPaths(mApplication.GetLocalDir(), true);
   
   String	afmpath = pf.FindDirFile("Cour.afm");
   wxSetAFMPath((char *) afmpath.c_str());

   wxDC *dc = GLOBAL_NEW wxPostScriptDC(NULL, TRUE, this);
   
   if (dc && dc->Ok() && dc->StartDoc((char *)_("Printing message...")))
   {
	    dc->SetUserScale(1.0, 1.0);
	    dc->ScaleGDIClasses(TRUE);
	    ftoList->SetDC(dc,true); // enable pageing!
	    ftoList->ReCalculateLines();
	    ftoList->Draw();
	    dc->EndDoc();
   }
   GLOBAL_DELETE dc;
   ftoList->SetDC(ftCanvas->GetDC());
   ftoList->ReCalculateLines();
#endif
}

void
wxMessageView::MimeInfo(void)
{
   String message, tmp;
   int type;
   
   message =
      String(  _("MIME type: ")) 
      + mailMessage->GetPartMimeType(mimeDisplayPart) + "\n";
   
   tmp = mailMessage->GetPartDesc(mimeDisplayPart);
   if(tmp.length() > 0)
      message += String(_("Description: ")) + tmp + String("\n");
   message += String(_("Size: "))
      + strutil_ltoa(mailMessage->GetPartSize(mimeDisplayPart));
   type = mailMessage->GetPartType(mimeDisplayPart);
   if(type == TYPEMESSAGE || type == TYPETEXT)
      message += String(_(" lines"));
   else
      message += String(_(" bytes"));
   
   wxMessageBox((char *)message.c_str(), _("MIME information"),
		wxOK|wxCENTRE|wxICON_INFORMATION, this);

}

void
wxMessageView::MimeHandle(void)
{
   String message, tmp, mimetype;
   int type;
   unsigned long len;
   char const *content;

   mimetype = mailMessage->GetPartMimeType(mimeDisplayPart);
   message =  String(  _("MIME type: ")) + mimetype + "\n";
   
   tmp = mailMessage->GetPartDesc(mimeDisplayPart);
   if(tmp.length() > 0)
      message += String(_("Description: ")) + tmp + String("\n");
   message += String(_("Size: "))
      + strutil_ltoa(mailMessage->GetPartSize(mimeDisplayPart));
   type = mailMessage->GetPartType(mimeDisplayPart);
   if(type == TYPEMESSAGE || type == TYPETEXT)
      message += String(_(" lines"));
   else
      message += String(_(" bytes"));

   MimeList *ml = mApplication.GetMimeList();
   String command, flags;
   bool found =	 ml->GetCommand(mimetype, command, flags);
   if(found)
   {
//      message += String(_("\nCommand: ")) + command + '\n';
//      wxMessageBox((char *)message.c_str(), _("MIME information"),
//		   wxOK|wxCENTRE|wxICON_INFORMATION, this);

      char *filename = tmpnam(NULL);
      FILE *out = fopen(filename,"wb");
      if(out)
      {
	 content = mailMessage->GetPartContent(mimeDisplayPart,&len);
	 if(fwrite(content,1,len,out) != len)
	 {
	    ERRORMESSAGE((_("Cannot write file.")));
	    fclose(out);
	    return;
	 }
	 fclose(out);
	 command = ml->ExpandCommand(command,filename,mimetype);

  #if     defined(OS_UNIX)
    // @@@@ what about error handling here (fork, system, ...)?
    if(fork() == 0)
    {
	    system(command.c_str());
	    unlink(command.c_str());
    }
  #elif   defined(OS_WIN)
    system(command.c_str());
  #else   // unknown OS
    #error  "Command execution not implemented!"
  #endif
      }
   }
   else // what can we handle internally?
   {
      if(mimetype == "MESSAGE/RFC822")
      {
	 char *filename = wxGetTempFileName("Mtemp");
	 MimeSave(filename);
	 MailFolderCC *mf = GLOBAL_NEW MailFolderCC(filename);
	 (void) GLOBAL_NEW wxMessageView(mf, 1, "message/rfc822",this);
	 wxRemoveFile(filename);
	 GLOBAL_DELETE mf;
      }
   }
}

void
wxMessageView::MimeSave(const char *ifilename)
{
   const char *filename;
   String message;
   
   if(! ifilename)
   {
      message = _("Please choose a filename to save as:");
      filename = MDialog_FileRequester((char *)message.c_str(),this,
		     NULL,NULL,NULL,NULL,true,
			    folder ? folder->GetProfile() : NULL);
   }
   else
      filename = ifilename;
   
   if(filename)
   {
      VAR(filename);
      unsigned long len;
      char const *content =
	 mailMessage->GetPartContent(mimeDisplayPart,&len);
      if(! content)
      {
	 FATALERROR(("Cannot get message content."));
	 return;
      }
      FILE *out = fopen(filename,"wb");
      if(out)
      {
	 if(fwrite(content,1,len,out) != len)
	    ERRORMESSAGE((_("Cannot write file.")));
	 else if(! ifilename) // only display in interactive mode
	 {
	    message = String(_("Wrote ")) + strutil_ultoa(len)
	       + String(_(" bytes to file\n"))
	       + filename + String(".");
	    INFOMESSAGE((message.c_str(),this));
	 }
	 fclose(out);
      }
   }
}

void
wxMessageView::OnMenuCommand(int id)
{
   switch(id)
   {
   case WXMENU_MSG_PRINT:
      Print();
      break;
   case WXMENU_MSG_REPLY:
      //Reply();
      break;
   case WXMENU_MSG_FORWARD:
      //Forward();
      break;
   case WXMENU_POPUP_MIME_OFFS+WXMENU_MIME_INFO:
      MimeInfo();
      break;
   case WXMENU_POPUP_MIME_OFFS+WXMENU_MIME_HANDLE:
      MimeHandle();
      break;
   case WXMENU_POPUP_MIME_OFFS+WXMENU_MIME_SAVE:
      MimeSave();
      break;
   default:
      wxMFrame::OnMenuCommand(id);
   }
}

wxMessageViewPanel::wxMessageViewPanel(wxMessageView *iMessageView)
{
  messageView = iMessageView;
  #ifdef  USE_WXWINDOWS2
    // @@@@ Create(1 param)?
  #else
    Create(messageView);
  #endif
}

void
wxMessageView::ShowMessage(MailFolder *folder, long num)
{
   if(initialised) GLOBAL_DELETE mailMessage;
   
   mailMessage = folder->GetMessage(num);     

   if(mailMessage->IsInitialised())
      initialised = true;
   else
   {
      initialised = false;
      GLOBAL_DELETE	mailMessage;
      return;
   }   
   Update();

}


void
wxMessageViewPanel::OnCommand(wxWindow &win, wxCommandEvent &event)
{
   wxPanel::OnCommand(win,event);
   cerr << "OnCommand" << endl;
}
