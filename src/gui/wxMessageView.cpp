/*-*- c++ -*-********************************************************
 * wxMessageView.cc : a wxWindows look at a message                 *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$        *
 *******************************************************************/

#ifdef __GNUG__
#pragma implementation "wxMessageView.h"
#endif

#include "Mpch.h"
#include "Mcommon.h"

#include "Message.h"

#ifndef USE_PCH
#  include <strutil.h>

#  include "PathFinder.h"
#  include "MimeList.h"
#  include "MimeTypes.h"
#  include "Profile.h"

#  include "MFrame.h"
#  include "MLogFrame.h"
#endif //USE_PCH

#include "Mdefaults.h"

#include "MApplication.h"
#include "gui/wxMApp.h"

#include "FolderView.h"
#include "MailFolder.h"
#include "MailFolderCC.h"

#include "Adb.h"
#include "MDialogs.h"

#include "MessageView.h"

#include "XFace.h"

#include "gui/wxFontManager.h"
#include "gui/wxIconManager.h"
#include "gui/wxFText.h"
#include "gui/wxMessageView.h"

#include   <gui/wxllist.h>
#include   <gui/wxlwindow.h>
#include   <gui/wxlparser.h>

#include <ctype.h>  // for isspace

// @@@@ for testing only
#ifndef USE_PCH
   extern "C"
   {
#     include <rfc822.h>
   }
#  include <MessageCC.h>
#endif //USE_PCH

// for clickable objects
struct ClickableInfo
{
   enum { CI_ICON, CI_URL } type;
   int id;
   String url;
};

#if !USE_WXWINDOWS2
  static void popup_callback(wxMenu& menu, wxCommandEvent& ev);
#endif // wxWin 1/2

IMPLEMENT_CLASS(wxMessageView, wxMFrame)
IMPLEMENT_CLASS(wxMessageViewPanel, wxPanel)

class myPopup : public wxMenu
{
protected:
   wxWindow *popup_parent;
   int      menu_offset;  // all selections get added to this and passed
                          // to popup_parent->OnMenuCommand(offset+entry)

#if !USE_WXWINDOWS2
    friend void popup_callback(wxMenu& menu, wxCommandEvent& ev);
#endif

public:
  myPopup(const char *name, wxWindow *frame, int offset)
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


class MimePopup : public wxMenu
{
public:
  MimePopup()
      : wxMenu(_("MIME Menu"))
  {
     Append(WXMENU_MIME_INFO,_("&Info"));
     Append(WXMENU_MIME_HANDLE,_("&Handle"));
     Append(WXMENU_MIME_SAVE,_("&Save"));
  }
};


#define   BUTTON_HEIGHT 24
#define   BUTTON_WIDTH  60

class MimeDialog : public wxDialog
{
public:
   MimeDialog(wxMessageView *parent, wxPoint const & pos, int partno)
      : wxDialog(parent, -1, _("MIME Dialog"), pos,
                 wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
      {
         m_PartNo = partno;
         m_MessageView = parent;
         new wxButton(this, WXMENU_MIME_INFO, _("Info"),wxPoint(0,0),wxSize(BUTTON_WIDTH,BUTTON_HEIGHT));
         new wxButton(this, WXMENU_MIME_HANDLE, _("Handle"),wxPoint(0,BUTTON_HEIGHT),wxSize(BUTTON_WIDTH,BUTTON_HEIGHT));
         new wxButton(this, WXMENU_MIME_SAVE, _("Save"), wxPoint(0,2*BUTTON_HEIGHT),wxSize(BUTTON_WIDTH,BUTTON_HEIGHT));
         new wxButton(this, WXMENU_MIME_DISMISS, _("Dismiss"), wxPoint(0,3*BUTTON_HEIGHT),wxSize(BUTTON_WIDTH,BUTTON_HEIGHT));
         Fit();
      }
   void OnCommandEvent(wxCommandEvent &event);
   DECLARE_EVENT_TABLE()
private:
   wxMessageView *m_MessageView;
   int m_PartNo;
};



BEGIN_EVENT_TABLE(MimeDialog, wxDialog)
   EVT_BUTTON(-1,    MimeDialog::OnCommandEvent)
END_EVENT_TABLE()

void
MimeDialog::OnCommandEvent(wxCommandEvent &event)
{
   switch(event.GetId())
   {
   case WXMENU_MIME_INFO:
      m_MessageView->MimeInfo(m_PartNo);
      break;
   case WXMENU_MIME_HANDLE:
      m_MessageView->MimeHandle(m_PartNo);
      break;
   case WXMENU_MIME_SAVE:
      m_MessageView->MimeSave(m_PartNo);
      break;
   case WXMENU_MIME_DISMISS:
      Show(false);
      break;
   }
}

   
BEGIN_EVENT_TABLE(wxMessageView, wxMFrame)
   EVT_MENU(-1,    wxMessageView::OnCommandEvent)
   EVT_TOOL(-1,    wxMessageView::OnCommandEvent)
END_EVENT_TABLE()

void
wxMessageView::Create(const String &iname, wxWindow *parent)
{
  if(initialised)
     return; // ERROR!
  
  mailMessage = NULL;
  mimeDisplayPart = 0;
  xface = NULL;
  xfaceXpm = NULL;

  m_Profile = new Profile("MessageView",
                          folder ? folder->GetProfile() : NULL);

  wxMFrame::Create(iname, parent);
  
  AddFileMenu();

  messageMenu = GLOBAL_NEW wxMenu;
  messageMenu->Append(WXMENU_MSG_REPLY,(char *)_("&Reply"));
  messageMenu->Append(WXMENU_MSG_FORWARD,(char *)_("&Forward"));
  messageMenu->Append(WXMENU_MSG_PRINT,(char *)_("&Print"));
  menuBar->Append(messageMenu, (char *)_("&Message"));

  AddHelpMenu();
  SetMenuBar(menuBar);

  m_MimePopup = NULL;
  m_LWindow = new wxLayoutWindow(this);
  m_LWindow->SetEventId(WXMENU_LAYOUT_CLICK);
  m_LWindow->SetFocus();
  m_LWindow->SetBackgroundColour( wxColour("White") ); 
  initialised = true;
}

wxMessageView::wxMessageView(const String &iname, wxWindow *parent)
             : wxMFrame(iname)
{
   initialised = false;
   folder = NULL;
   Create(iname,parent);
   Show(TRUE);
}

wxMessageView::wxMessageView(MailFolder *ifolder,
                             long num,
                             const String &iname,
                             wxWindow *parent)
             : wxMFrame(iname)
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
   int i,n,t;
   char const * cptr;
   String tmp,from,url;
   bool   lastObjectWasIcon = false; // a flag

   ClickableInfo *ci;
   
   wxLayoutList &llist = m_LWindow->GetLayoutList();
   wxLayoutObjectBase *obj = NULL;

   m_LWindow->GetLayoutList().SetEditable(true);
   m_LWindow->GetLayoutList().Clear(wxROMAN,16);

   /* FIXME
     m_LWindow->GetLayoutList().Clear(
      m_Profile->readEntry(MP_FTEXT_FONT,MP_FTEXT_FONT_D),
      m_Profile->readEntry(MP_FTEXT_SIZE,MP_FTEXT_SIZE_D),
      m_Profile->readEntry(MP_FTEXT_STYLE,MP_FTEXT_STYLE_D),
      m_Profile->readEntry(MP_FTEXT_WEIGHT,MP_FTEXT_WEIGHT_D),
      0,
      m_Profile->readEntry(MP_FTEXT_FGCOLOUR,MP_FTEXT_FGCOLOUR_D),
      m_Profile->readEntry(MP_FTEXT_BGCOLOUR,MP_FTEXT_BGCOLOUR_D));
   */
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
          llist.Insert(new wxLayoutObjectIcon(new wxIcon(xfaceXpm,-1,-1)));
          llist.LineBreak();
       }
     }
   }
#endif
#endif
   llist.SetFontWeight(wxBOLD);
   llist.Insert(_("From: "));
   llist.SetFontWeight(wxNORMAL);
   from = mailMessage->Address(tmp,MAT_FROM);
   if(tmp.length() > 0)
      from = tmp + String(" <") + from + '>';
   llist.Insert(from);
   llist.LineBreak();
   llist.SetFontWeight(wxBOLD);
   llist.Insert(_("Subject: "));
   llist.SetFontWeight(wxNORMAL);
   llist.Insert(mailMessage->Subject());
   llist.LineBreak();
   llist.SetFontWeight(wxBOLD);
   llist.Insert(_("Date: "));
   llist.SetFontWeight(wxNORMAL);
   llist.Insert(mailMessage->Date());
   llist.LineBreak();llist.LineBreak();

   // iterate over all parts
   n = mailMessage->CountParts();
   for(i = 0; i < n; i++)
   {
      t = mailMessage->GetPartType(i);
      if(mailMessage->GetPartSize(i) == 0)
         continue; // ignore empty parts
      // insert text:
      if(t == TYPETEXT || (t == TYPEMESSAGE && folder->GetProfile()
                           && folder->GetProfile()->
                           readEntry(MP_RFC822_IS_TEXT,MP_RFC822_IS_TEXT_D)))
      {
         cptr = mailMessage->GetPartContent(i);
         if(cptr == NULL)
            continue; // error ?
         llist.LineBreak();
         if(folder->GetProfile() &&
            folder->GetProfile()->readEntry(MP_HIGHLIGHT_URLS,
                                            MP_HIGHLIGHT_URLS_D))
         {
            tmp = cptr;
            String url;
            String before;

            do
            {
               before =  strutil_findurl(tmp,url);
               wxLayoutImportText(llist,before);
               if(!strutil_isempty(url))
               {
                  obj->SetUserData(ci); // gets freed by list
                  ci = new ClickableInfo;
                  ci->type = ClickableInfo::CI_URL;
                  ci->url = url;
                  obj = new wxLayoutObjectText(url);
                  obj->SetUserData(ci);
                  llist.SetFontColour("BLUE");
                  llist.Insert(obj);
                  llist.SetFontColour("BLACK");
               }
            }while(! strutil_isempty(tmp));
            lastObjectWasIcon = false;
         }
      }
      else // insert an icon
      {
         wxIcon *icn;
         if(t == TYPEIMAGE
            && folder->GetProfile()
            && folder->GetProfile()->readEntry(MP_INLINE_GFX,MP_INLINE_GFX_D))
         {
            char *filename = wxGetTempFileName("Mtemp");
            MimeSave(i,filename);
            icn = new wxIcon();
            if(icn->LoadFile(filename,0))
               obj = new wxLayoutObjectIcon(icn);
            else
            {
               delete icn;
               icn = mApplication.GetIconManager()->GetIcon(mailMessage->GetPartMimeType(i));
            }
            wxRemoveFile(filename);
         }
         else
            icn = mApplication.GetIconManager()->GetIcon(mailMessage->GetPartMimeType(i));
         obj = new wxLayoutObjectIcon(icn);

         ci = new ClickableInfo;
         ci->type = ClickableInfo::CI_ICON;
         ci->id = i;
         obj->SetUserData(ci); // gets freed by list
         llist.Insert(obj);
         lastObjectWasIcon = true;
      }
   }

   llist.LineBreak();
   m_LWindow->GetLayoutList().SetEditable(false);
   m_LWindow->Refresh();
   m_LWindow->UpdateScrollbars();
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
   if(m_MimePopup)
      delete m_MimePopup;
}

void
wxMessageView::MimeInfo(int mimeDisplayPart)
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
wxMessageView::MimeHandle(int mimeDisplayPart)
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
       MimeSave(mimeDisplayPart,filename);
       MailFolderCC *mf = MailFolderCC::OpenFolder(filename);
       (void) GLOBAL_NEW wxMessageView(mf, 1, "message/rfc822",this);
       wxRemoveFile(filename);//FIXME: does this work for non-UNIX systems?
       mf->Close();
     }
   }
}

void
wxMessageView::MimeSave(int mimeDisplayPart,const char *ifilename)
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
wxMessageView::OnCommandEvent(wxCommandEvent &event)
{
   ClickableInfo *ci;
   
   if(event.GetId() == WXMENU_LAYOUT_CLICK)
   {
      wxLayoutObjectBase *obj;
      cerr << "Received click event!" << endl;
      obj = (wxLayoutObjectBase *)event.GetClientData();
      ci = (ClickableInfo *)obj->GetUserData();
      if(ci)
      {
         switch(ci->type)
         {
         case ClickableInfo::CI_ICON:
         {
            int x,y;
            GetPosition(&x,&y);
            wxPoint pos = m_LWindow->GetClickPosition();
            pos.x += x; pos.y += y;
            if(m_MimePopup)
               delete m_MimePopup;
            m_MimePopup = new MimeDialog(this,pos,ci->id);
            m_MimePopup->Show(true);
            break;
         }
         case ClickableInfo::CI_URL:
         {
            String cmd;
            if(folder)
               cmd = READ_CONFIG(folder->GetProfile(), MP_BROWSER);
            else
               cmd = READ_APPCONFIG(MP_BROWSER);
            cmd += ' '; 
            cmd += ci->url;
            wxExecute(cmd);
            break;
         }
         }
      }
   }
   else
      OnMenuCommand(event.GetId());
}


void
wxMessageView::OnMenuCommand(int id)
{
   switch(id)
   {
   case WXMENU_MSG_PRINT:
      //Print();
      break;
   case WXMENU_MSG_REPLY:
      //Reply();
      break;
   case WXMENU_MSG_FORWARD:
      //Forward();
      break;
   default:
      wxMFrame::OnMenuCommand(id);
   }
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

void
wxMessageView::Print(void)
{

   m_LWindow->Print();

   // old stuff
#if 0

#ifdef  OS_UNIX
    // set AFM path (recursive!)
    PathFinder pf(READ_APPCONFIG(MC_AFMPATH), true);
    pf.AddPaths(mApplication.GetGlobalDir(), true);
    pf.AddPaths(mApplication.GetLocalDir(), true);

    String afmpath = pf.FindDirFile("Cour.afm");
    wxSetAFMPath((char *) afmpath.c_str());

    wxDC *dc = GLOBAL_NEW wxPostScriptDC("", TRUE, this);

    if (dc && dc->Ok() && dc->StartDoc((char *)_("Printing message...")))
    {
      dc->SetUserScale(1.0, 1.0);
#if     USE_WXWINDOWS2
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
      // FIXME TODO: printing for Windows
#endif
#endif
} 
