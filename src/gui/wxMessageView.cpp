/*-*- c++ -*-********************************************************
 * wxMessageView.cc : a wxWindows look at a message                 *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$         *
 *******************************************************************/

#ifdef __GNUG__
#pragma implementation "wxMessageView.h"
#endif

#ifdef USE_PCH
  #include "Mpch.h"
#endif

#include "Mcommon.h"
#include "Message.h"

#ifndef USE_PCH
  #include <strutil.h>

  // @@@@ for testing only
  #include <MessageCC.h>
  extern "C"
  {
    #include <rfc822.h>
  }
#endif

#include "MFrame.h"
#include "MLogFrame.h"

#include "Mdefaults.h"

#include "PathFinder.h"
#include "MimeList.h"
#include "MimeTypes.h"
#include "Profile.h"

#include "MApplication.h"

#include "FolderView.h"
#include "MailFolder.h"
#include "MailFolderCC.h"
#include "Message.h"

#include "Adb.h"
#include "MDialogs.h"

#include "MessageView.h"

#include "XFace.h"

#include "gui/wxFontManager.h"
#include "gui/wxIconManager.h"
#include "gui/wxFText.h"
#include "gui/wxMessageView.h"

#include <ctype.h>  // for isspace
 
#if !USE_WXWINDOWS2
  static void popup_callback(wxMenu& menu, wxCommandEvent& ev);
#endif // wxWin 1/2

IMPLEMENT_CLASS(wxMessageView, wxMFrame)
IMPLEMENT_CLASS(wxMessageViewPanel, wxPanel)

class myPopup : public wxMenu
{
protected:
   wxFrame *popup_parent;
   int      menu_offset;  // all selections get added to this and passed
                          // to popup_parent->OnMenuCommand(offset+entry)

  #if !USE_WXWINDOWS2
    friend void popup_callback(wxMenu& menu, wxCommandEvent& ev);
  #endif

public:
  myPopup(const char *name, wxFrame *frame, int offset)
  #if USE_WXWINDOWS2
      : wxMenu(name)
  #else  // wxWin 1
      : wxMenu((char *)name, (wxFunction) &popup_callback)
  #endif // wxWin1/2
  {
    popup_parent = frame;
    menu_offset  = offset;
  }
};

#if USE_WXWINDOWS2
  // @@@@ OnMenuCommand?
#else
static void popup_callback(wxMenu& menu, wxCommandEvent& ev)
{
  myPopup *popup = (myPopup *)&menu;
  popup->popup_parent->OnMenuCommand(popup->menu_offset + ev.GetSelection());
}
#endif


class wxMVCanvas : public wxCanvas
{
   wxFTOList *ftol;
   wxMessageView *mv;
public:
   wxMVCanvas(wxFTOList *iftol, wxMessageView *imv)
#if USE_WXWINDOWS2
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
  xfaceXpm = NULL;

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

  ftoList = GLOBAL_NEW wxFTOList((wxDC *)NULL, folder ?
      folder->GetProfile() : NULL);
  ftCanvas = GLOBAL_NEW wxMVCanvas(ftoList,this);

  #if USE_WXWINDOWS2
    // @@@@ GetDC
  #else
    ftoList->SetDC(ftCanvas->GetDC());
  #endif

  initialised = true;
}

wxMessageView::wxMessageView(const String &iname, wxFrame *parent)
{
   initialised = false;
   folder = NULL;
   Create(iname,parent);
   Show(TRUE);
}

wxMessageView::wxMessageView(MailFolder *ifolder,
                              long num,
                              const String &iname,
                              wxFrame *parent)
{
   initialised = false;
   folder = ifolder;
   Create(iname,parent);
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
   coord_t  width, height;
   int i,n,t;
   char const * cptr;
   String tmp,from;
   bool   lastObjectWasIcon = false; // a flag

#ifdef HAVE_XFACES
   // need real XPM support in windows
#ifndef OS_WIN
   if(folder->GetProfile() &&
       folder->GetProfile()->readEntry(MP_SHOW_XFACES, MP_SHOW_XFACES_D))
   {
     mailMessage->GetHeaderLine("X-Face", tmp);
     if(tmp.length() > 2)   //\r\n
     {
       xface = GLOBAL_NEW XFace();
       tmp = tmp.c_str()+strlen("X-Face:");
       xface->CreateFromXFace(tmp.c_str());
       if(xface->CreateXpm(&xfaceXpm))
       {
         ftoList->AddIcon("XFACE", xfaceXpm);
         ftoList->AddFormattedText(" \n<IMG SRC=\"XFACE\">\n");
       }
     }
   }
#endif
#endif
   ftoList->AddFormattedText(_("<b>From:</b> "));
   from = mailMessage->Address(tmp,MAT_FROM);
   if(tmp.length() > 0)
     from = tmp + String(" <") + from + '>';
   ftoList->AddText(from);
   ftoList->AddFormattedText(_("\n<b>Subject:</b> "));
   ftoList->AddText(mailMessage->Subject());
   ftoList->AddFormattedText(_("\n<b>Date:</b> "));
   ftoList->AddText(mailMessage->Date());
   ftoList->AddText("\n\n");

   n = mailMessage->CountParts();
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
            if(folder->GetProfile() &&
                folder->GetProfile()->readEntry(MP_HIGHLIGHT_URLS,
                    MP_HIGHLIGHT_URLS_D))
            {
              tmp = "";
              HighLightURLs(cptr, tmp);
              ftoList->AddFormattedText(tmp);
            }
            else
              ftoList->AddText(tmp.c_str());
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

   #if USE_WXWINDOWS2
      // @@@@ SetScrollbar?
   #else
      ftCanvas->SetScrollbars(20,20,(int)width/20,(int)(height*1.2)/20,10,10);
   #endif
}

void
wxMessageView::HighLightURLs(const char *input, String &out)
{
   const char *cptr = input;

   while(*cptr)
   {

      if(strncmp(cptr, "http:", 5) == 0 || strncmp(cptr, "ftp:", 4) == 0)
      {
        const char *cptr2 = cptr;
        out += " <a href=\"";
        out += "\"";
        while(*cptr2 && ! isspace(*cptr2))
          out += *cptr2++;
        out += "\"> ";
      }
      else
      {
        // escape brackets:
        if(*cptr == '<')
          out += '<';
        else
          if(*cptr == '>')
            out += '>';
      }
      out += *cptr++;
   }
}

wxMessageView::~wxMessageView()
{
   if(! initialised)
      return;
   if(mailMessage)
      GLOBAL_DELETE mailMessage;
   if(xface)
      GLOBAL_DELETE xface;
   if(xfaceXpm)
      GLOBAL_DELETE [] xfaceXpm;
   if(popupMenu)
      GLOBAL_DELETE popupMenu;
   GLOBAL_DELETE ftCanvas;
   //fixme:
   GLOBAL_DELETE ftoList;
}


void
wxMessageView::ProcessMouse(wxMouseEvent &event)
{
  int x,y;
  FTObject const * obj;

  if(event.RightDown())
  {
    #if USE_WXWINDOWS2
      x = y = 0; // @@@@ ViewStart?  
      obj = ftoList->FindClickable(event.GetX() - x, event.GetY() - y);
    #else
      ftCanvas->ViewStart(&x,&y);
      obj = ftoList->FindClickable(event.x - x, event.y - y);
    #endif
    if(obj)
    {
      if(obj->GetType() == LI_ICON)
      {
        String command, flags;
        // mime type is stored after 1. semicolon:
        // "IMG SRC="mimetype";mimetype;# of section"
        String type = obj->GetText();
        char *buf = strutil_strdup(type);
        char *token = strtok(buf,";");
        token = strtok(NULL,";");
        if(!token) abort(); // e.g. a X-Face
        type = token;
        token = strtok(NULL,";");
        if(!token) abort(); 
        mimeDisplayPart = atoi(token); // remember section to use it in
        // Popup menu
        GLOBAL_DELETE [] buf;
        #if	USE_WXWINDOWS2
          // @@@@ PopupMenu?
        #else
          PopupMenu(popupMenu, event.x - x, event.y - y);
        #endif
      }
      else if (obj->GetType() == LI_URL)
      {
        String cmd;
        if(folder)
          cmd = folder->GetProfile()->readEntry(MP_BROWSER,MP_BROWSER_D);
        else
          cmd = mApplication.readEntry(MP_BROWSER,MP_BROWSER_D);
        cmd += ' ';
        cmd += obj->GetText();
        wxExecute(WXCPTR cmd.c_str());
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
    PathFinder pf(mApplication.readEntry(MC_AFMPATH,MC_AFMPATH_D), true);
    pf.AddPaths(mApplication.GetGlobalDir(), true);
    pf.AddPaths(mApplication.GetLocalDir(), true);

    String afmpath = pf.FindDirFile("Cour.afm");
    wxSetAFMPath((char *) afmpath.c_str());

    // @@@ Would passing the empty string also work with wxWin1?
    #if	USE_WXWINDOWS2
      wxDC *dc = GLOBAL_NEW wxPostScriptDC("", TRUE, this);
    #else  // wxWin 1
      wxDC *dc = GLOBAL_NEW wxPostScriptDC(NULL, TRUE, this);
    #endif // wxWin 1/2

    if (dc && dc->Ok() && dc->StartDoc((char *)_("Printing message...")))
    {
      dc->SetUserScale(1.0, 1.0);
      #if	USE_WXWINDOWS2
        // @@@@ ScaleGDIClasses
      #else  // wxWin 1
        dc->ScaleGDIClasses(TRUE);
      #endif // wxWin 1/2
        
      ftoList->SetDC(dc,true); // enable pageing!
      ftoList->ReCalculateLines();
      ftoList->Draw();
      dc->EndDoc();
    }
    GLOBAL_DELETE dc;
    #if USE_WXWINDOWS2
      // @@@@ GetDC()?
    #else  // wxWin 1
      ftoList->SetDC(ftCanvas->GetDC());
    #endif // wxWin 1/2ftoList->ReCalculateLines();
  #else
    // TODO: printing for Windows
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
   bool found = ml->GetCommand(mimetype, command, flags);
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

#ifdef OS_UNIX
       // @@@@ what about error handling here (fork, system, ...)?
       if(fork() == 0)
       {
         system(command.c_str());
         unlink(command.c_str());
       }
#elif   defined(OS_WIN)
       system(command.c_str());
#else   // unknown OS
#    error  "Command execution not implemented!"
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
    char const *content = mailMessage->GetPartContent(mimeDisplayPart,&len);
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
  #if USE_WXWINDOWS2
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
      GLOBAL_DELETE mailMessage;
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
