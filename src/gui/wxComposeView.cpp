/*-*- c++ -*-********************************************************
 * wxComposeView.cc : a wxWindows look at a message                 *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$        *
 *******************************************************************/

#ifdef __GNUG__
#pragma  implementation "wxComposeView.h"
#endif

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------
#include "Mpch.h"
#include "Mcommon.h"

#ifndef USE_PCH
#  include "strutil.h"

#  include "PathFinder.h"
#  include "MimeList.h"
#  include "MimeTypes.h"
#  include "Profile.h"

#  include "MFrame.h"
#  include "MLogFrame.h"
#endif

#include "Mdefaults.h"

#include "MApplication.h"
#include "gui/wxMApp.h"

#include "FolderView.h"
#include "MailFolder.h"
#include "MailFolderCC.h"
#include "Message.h"
#include "MessageCC.h"
#include "SendMessageCC.h"
#include "Adb.h"
#include "MDialogs.h"

#include "gui/wxFontManager.h"
#include "gui/wxIconManager.h"
//#include "gui/wxFText.h"
//#include "gui/wxFTCanvas.h"

#include   "gui/wxllist.h"
#include   "gui/wxlwindow.h"
#include   "gui/wxlparser.h"
#include   "gui/wxComposeView.h"
#include   "gui/wxAdbEdit.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// control ids
enum
{
   IDB_EXPAND = 100
};

// margin between controls on the panel
#define LAYOUT_MARGIN 5

// ----------------------------------------------------------------------------
// event tables &c
// ----------------------------------------------------------------------------
IMPLEMENT_DYNAMIC_CLASS(wxComposeView, wxMFrame)

#ifdef USE_WXWINDOWS2
   BEGIN_EVENT_TABLE(wxComposeView, wxMFrame)
   // wxComposeView menu events
   EVT_MENU(WXMENU_COMPOSE_INSERTFILE, wxComposeView::OnInsertFile)
   EVT_MENU(WXMENU_COMPOSE_SEND,       wxComposeView::OnSend)
   EVT_MENU(WXMENU_COMPOSE_PRINT,      wxComposeView::OnPrint)
   EVT_MENU(WXMENU_COMPOSE_CLEAR,      wxComposeView::OnClear)
   
   // button notifications
   EVT_BUTTON(IDB_EXPAND, wxComposeView::OnExpand)

   // size change
   EVT_SIZE(wxComposeView::OnSize)
   END_EVENT_TABLE() 
#endif


   struct MimeContent
   {
      String m_FileName;
      int    m_NumericMimeType;
      String m_MimeType;
   };

// ============================================================================
// implementation
// ============================================================================
void
wxComposeView::Create(const String &iname, wxWindow *parent,
                      ProfileBase *parentProfile,
                      String const &to, String const &cc, String const &bcc,
                      bool hide)
{
   CHECK_RET( !initialised, "wxComposeView created twice" );

   m_LayoutWindow = NULL;
   nextFileID = 0;

   if(!parentProfile)
      parentProfile = mApplication.GetProfile();
   profile = new Profile(iname,parentProfile);

   // use default values for address fields if none explicitly specified
   const char *cto = strutil_isempty(to) ? READ_CONFIG(profile, MP_COMPOSE_TO) 
      : to.c_str();
   const char *ccc = strutil_isempty(cc) ? READ_CONFIG(profile, MP_COMPOSE_CC) 
      : cc.c_str();
   const char *cbcc = strutil_isempty(bcc) ? READ_CONFIG(profile, MP_COMPOSE_BCC)
      : bcc.c_str();

   // build menu
   // ----------
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

   // create panel with items
   // -----------------------
   panel = new wxPanel(this);

#ifdef USE_WXWINDOWS2
   // @@ SetLabelPosition(wxVERTICAL) in wxWin2 ??
#else
   panel->SetLabelPosition(wxVERTICAL);
#endif

   txtToLabel = CreateLabel(panel, "To:");
   txtTo      = CreateText(panel, -1, -1, -1, -1, "toField");

   aliasButton = CreateButton(panel, "Expand", "", IDB_EXPAND);

   if( READ_CONFIG(profile, MP_SHOWCC) )
   {
      txtCCLabel = CreateLabel(panel, "CC:");
      txtCC = CreateText(panel, -1, -1, -1, -1, "");
   }
   if( READ_CONFIG(profile, MP_SHOWBCC) )
   {
      txtBCCLabel = CreateLabel(panel, "BCC:");
      txtBCC = CreateText(panel, -1, -1, -1, -1, "");
   }
   txtSubjectLabel = CreateLabel(panel, "Subject:");
   txtSubject = CreateText(panel, -1, -1, -1, -1, "Subject");

   // fix the constraints
   // -------------------
   wxLayoutConstraints *c;
   SetAutoLayout(TRUE);
   panel->SetAutoLayout(TRUE);

   // with the constraints, I assume that "Subject" is the longest label
   // and all labels are right justified to the right edge of "Subject" label

   // first row: "To" fields (label and text entry) and the "Expand" button
   c = new wxLayoutConstraints;
   c->top.SameAs(panel, wxTop, LAYOUT_MARGIN);
   c->right.SameAs(txtSubjectLabel, wxRight);
   c->height.AsIs();
   c->width.AsIs();
   txtToLabel->SetConstraints(c);

   c = new wxLayoutConstraints;
   c->top.SameAs(panel, wxTop, LAYOUT_MARGIN);
   c->left.SameAs(txtSubjectLabel, wxRight, LAYOUT_MARGIN);
   c->right.SameAs(aliasButton, wxLeft, LAYOUT_MARGIN);
   c->height.AsIs(); 
   txtTo->SetConstraints(c);

   c = new wxLayoutConstraints;
   c->top.SameAs        (panel, wxTop, LAYOUT_MARGIN);
   c->right.SameAs(panel, wxRight, LAYOUT_MARGIN);
   c->width.AsIs();
   c->height.AsIs(); 
   aliasButton->SetConstraints(c);

   // second row: optional "CC" label and text entry

   // the last control we set the constraints for
   wxItem *last = txtTo;

   if( READ_CONFIG(profile, MP_SHOWCC) )
   {
      c = new wxLayoutConstraints;
      c->top.Below(txtToLabel);
      c->right.SameAs(txtSubjectLabel, wxRight);
      c->height.AsIs();
      c->width.AsIs();
      txtCCLabel->SetConstraints(c);

      c = new wxLayoutConstraints;
      c->top.Below(txtToLabel);
      c->left.SameAs(txtSubjectLabel, wxRight, LAYOUT_MARGIN);
      c->right.SameAs(panel, wxRight, LAYOUT_MARGIN);
      c->height.AsIs(); 
      txtCC->SetConstraints(c);
      last = txtCC;
   }

   // third row: optional "BCC" label and text entry
   if( READ_CONFIG(profile, MP_SHOWBCC) )
   {
      c = new wxLayoutConstraints;
      c = new wxLayoutConstraints;
      c->top.Below(last);
      c->right.SameAs(txtSubjectLabel, wxRight);
      c->height.AsIs();
      c->width.AsIs();
      txtBCCLabel->SetConstraints(c);

      c = new wxLayoutConstraints;
      c->top.Below(last);
      c->left.SameAs(txtSubjectLabel, wxRight, LAYOUT_MARGIN);
      c->right.SameAs(panel, wxRight, LAYOUT_MARGIN);
      c->height.AsIs(); 
      txtBCC->SetConstraints(c);
      last = txtBCC;
   }

   // fourth row: subject line
   c = new wxLayoutConstraints;
   c->top.SameAs(last, wxBottom, LAYOUT_MARGIN); //Below  (last);
   c->left.SameAs(panel, wxLeft, LAYOUT_MARGIN);
   c->height.AsIs();
   c->width.AsIs();
   txtSubjectLabel->SetConstraints(c);

   c = new wxLayoutConstraints;
   c->top.Below(last);
   c->left.SameAs(txtSubjectLabel, wxRight, LAYOUT_MARGIN);
   c->right.SameAs(panel, wxRight, LAYOUT_MARGIN);
   c->height.AsIs(); 
   txtSubject->SetConstraints(c);
   last = txtSubjectLabel;

   // Panel itself
   c = new wxLayoutConstraints;
   c->left.SameAs(this, wxLeft);
   c->top.SameAs(this, wxTop);
   c->right.SameAs(this, wxRight);
   c->height.SameAs  (this, wxHeight);
   panel->SetConstraints(c);

   txtSubjectLabel = CreateLabel(panel, "Subject:");
   txtSubject = CreateText(panel, -1, -1, -1, -1, "Subject");

   wxStaticBox  *box = new wxStaticBox(panel, -1, "");
   CreateFTCanvas();

   // fix the constraints
   // -------------------

   // macro which centers one control vertically with respect to another one
#define VCENTER(c, ref) c->height.AsIs();         \
   c->centreY.SameAs(ref, wxCentreY, -1)

      SetAutoLayout(TRUE);
   panel->SetAutoLayout(TRUE);

   // with the constraints, I assume that "Subject" is the longest label
   // and all labels are right justified to the right edge of "Subject" label

   // first row: "To" fields (label and text entry) and the "Expand" button
   c = new wxLayoutConstraints;
   c->top.SameAs(panel, wxTop, 15);
   c->right.SameAs(txtSubjectLabel, wxRight);
   c->height.AsIs();
   c->width.AsIs();
   txtToLabel->SetConstraints(c);

   c = new wxLayoutConstraints;
   c->left.SameAs(txtSubjectLabel, wxRight);
   c->right.SameAs(aliasButton, wxLeft, 1);
   VCENTER(c, txtToLabel); 
   txtTo->SetConstraints(c);

   c = new wxLayoutConstraints;
   c->right.SameAs(panel, wxRight, 7);
   c->width.AsIs();
   VCENTER(c, txtToLabel); 
   aliasButton->SetConstraints(c);

   // this variable holds the last (from top to bottom) text field we created
   // (we need it because txtCC and txtBCC are optional and the subject field
   // must be under the last header line)
   wxText *txtLast = txtTo;

   // second row: "CC" label and text entry
   if ( txtCCLabel != NULL ) {
      c = new wxLayoutConstraints;
      c->top.Below(aliasButton, 2);
      c->right.SameAs(txtSubjectLabel, wxRight);
      c->height.AsIs();
      c->width.AsIs();
      txtCCLabel->SetConstraints(c);

      c = new wxLayoutConstraints;
      c->left.SameAs(txtSubjectLabel, wxRight);
      c->right.SameAs(panel, wxRight, 5);
      VCENTER(c, txtCCLabel); 
      txtCC->SetConstraints(c);

      txtLast = txtCC;
   }

   // third row: "BCC" label and text entry
   if ( txtBCCLabel != NULL ) {
      c = new wxLayoutConstraints;
      c->top.Below(txtLast, 1);
      c->right.SameAs(txtSubjectLabel, wxRight);
      c->height.AsIs();
      c->width.AsIs();
      txtBCCLabel->SetConstraints(c);

      c = new wxLayoutConstraints;
      c->left.SameAs(txtSubjectLabel, wxRight);
      c->right.SameAs(panel, wxRight, 5);
      VCENTER(c, txtBCCLabel); 
      txtBCC->SetConstraints(c);

      txtLast = txtBCC;
   }

   if( READ_CONFIG(profile, MP_COMPOSE_USE_SIGNATURE) )
   {
      size_t size;
      ifstream istr;
      char *buffer;
      istr.open(READ_CONFIG(profile, MP_COMPOSE_SIGNATURE));
      if(istr)
      {
         istr.seekg(0,ios::end);
         size = istr.tellg();
         buffer = new char [size+1];
         if( READ_CONFIG(profile, MP_COMPOSE_USE_SIGNATURE_SEPARATOR) )
         {
            m_LayoutWindow->GetLayoutList().Insert("--");
            m_LayoutWindow->GetLayoutList().LineBreak();;
         }
         istr.seekg(0,ios::beg);
         istr.read(buffer, size);
         buffer[size] = '\0';
         if(! istr.fail())
            m_LayoutWindow->GetLayoutList().Insert(buffer);
         delete [] buffer;
         istr.close();
         m_LayoutWindow->GetLayoutList().SetCursor(wxPoint(0,0));
      }
   }

   CreateStatusBar();

   initialised = true;
   if(! hide)
      Show(TRUE);
   //   txtTo->SetFocus();
}

void
wxComposeView::CreateFTCanvas(void)
{
   m_LayoutWindow = new wxLayoutWindow(panel);
   // Canvas
   wxLayoutConstraints *c = new wxLayoutConstraints;
   c->left.SameAs  (panel, wxLeft);
   c->top.SameAs   (txtSubjectLabel, wxBottom, LAYOUT_MARGIN);
   c->right.SameAs (panel, wxRight);
   c->bottom.SameAs(panel, wxBottom);
   m_LayoutWindow->SetConstraints(c);
   m_LayoutWindow->GetLayoutList().SetEditable(true);
   //FIXMEm_LayoutWindow->SetWrapMargin(READ_CONFIG(profile, MP_COMPOSE_WRAPMARGIN));
}

#ifdef USE_WXWINDOWS2
void
wxComposeView::OnSize(wxSizeEvent& event)
{
#  ifdef USE_WXGTK
   Layout();
#  else  // MSW
   wxMFrame::OnSize(event);
#  endif // GTK
}
#else //wxWin1
void
wxComposeView::OnSize(int  w, int h)
{
   Layout();
}
#endif //wxWin1/2

wxComposeView::wxComposeView(const String &iname,
                             wxWindow *parent,
                             ProfileBase *parentProfile,
                             bool hide)
   : wxMFrame(iname,parent)
{
   initialised = false;
   Create(iname,parent,parentProfile, "","","",hide);
}

wxComposeView::~wxComposeView()
{
   if(! initialised)
      return;
   delete m_LayoutWindow;
}


void
wxComposeView::ProcessMouse(wxMouseEvent &event)
{
}

void
wxComposeView::OnExpand(wxCommandEvent &event)
{
   Adb * adb = mApplication.GetAdb();
   AdbEntry *entry = NULL;

   String   tmp = txtTo->GetValue();
   int l = tmp.length();
   
   // FIXME @@@ VZ: I don't know what this test does, so I disabled it
   //if ( tmp.substr(l - 1, 1) == " " )

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

#ifndef USE_WXWINDOWS2
void
wxComposeView::OnCommand(wxWindow &win, wxCommandEvent &event)
{
   // this gets called when the Expand button is pressed
   if ( strcmp(win.GetName(),"toField") == 0 || 
        strcmp(win.GetName(),"button")  == 0    ) {
      OnExpand(event);
   }
}
#endif // wxWin1

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
      delete m_LayoutWindow;
      CreateFTCanvas();
      Layout();
#ifdef  USE_WXWINDOWS2
      Refresh();
#else //wxWin1
      OnPaint();
#endif
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
      tmp;
   
   const char
      *filename = MDialog_FileRequester(NULL,this,NULL,NULL,NULL,NULL,true,profile);

   if(! filename)
      return;

   MimeContent *mc = new MimeContent();
   mc->m_FileName =  String(filename);
   mc->m_NumericMimeType = TYPEAPPLICATION;
   if(! mApplication.GetMimeTypes()->Lookup(mc->m_FileName, mc->m_MimeType, &(mc->m_NumericMimeType)))
      mc->m_MimeType = String("APPLICATION/OCTET-STREAM");

   wxLayoutObjectIcon *obj = new wxLayoutObjectIcon(mApplication.GetIconManager()->GetIcon(mimeType));
   obj->SetUserData(mc);
   
   m_LayoutWindow->GetLayoutList().Insert(tmp);
   m_LayoutWindow->GetLayoutList().MoveCursor(1,0);
   m_LayoutWindow->SetFocus();   
}

// little helper function to turn kbList into a map:
const char *
wxComposeView::LookupFileName(unsigned long id)
{
   wxCVFileMapType::iterator i;
   for(i = fileMap.begin(); i != fileMap.end(); i++)
      if((*i)->id == id)
         return (*i)->filename.c_str();
   return NULL;
}
   
void
wxComposeView::Send(void)
{
   String const
      * tmp;
   String
      tmp2, mimeType, mimeSubType;
   const char
      *filename = NULL;
   char
      *cptr, *ocptr,
      *buffer;
   ifstream
      istr;
   size_t
      size;
   int
      numMimeType;

   SendMessageCC sm
      (
         mApplication.GetProfile(),
         (const char *)txtSubject->GetValue(),
         (const char *)txtTo->GetValue(),
         READ_CONFIG(profile, MP_SHOWCC) ? txtCC->GetValue().c_str()
         : (const char *)NULL,
         READ_CONFIG(profile, MP_SHOWBCC) ? txtBCC->GetValue().c_str() 
         : (const char *)NULL
         );

   wxLayoutExportObject *export;
   wxLayoutList::iterator i = m_LayoutWindow->GetLayoutList().begin();
   wxLayoutObjectBase *lo = NULL;
   MimeContent *mc = NULL;
      
   while((export = wxLayoutExport(m_LayoutWindow->GetLayoutList(),
                                  i,WXLO_EXPORT_AS_TEXT)) != NULL)
   {
      if(export->type == WXLO_EXPORT_TEXT)
         sm.AddPart(TYPETEXT,export->content.text->c_str(),export->content.text->length());
      else
      {
         lo = export->content.object;
         if(lo->GetType() == WXLO_TYPE_ICON)
         {
            mc = (MimeContent *)lo->GetUserData();
            istr.open(mc->m_FileName.c_str());
            if(istr)
            {
               istr.seekg(0,ios::end);
               size = istr.tellg();
               buffer = new char [size];
               istr.seekg(0,ios::beg);
               istr.read(buffer, size);
 
               if(! istr.fail())
                  sm.AddPart(mc->m_NumericMimeType, buffer, size,
                             strutil_after(mc->m_MimeType,'/')  //subtype
                     );
               else
                  SYSERRMESSAGE((_("Cannot read file."),this));
               delete [] buffer;
               istr.close();
            }
            else
               SYSERRMESSAGE((_("Cannot open file: ")+mc->m_FileName,this));
            delete mc;
         }
      }
      delete export;
   }
#if 0
   while(ftoType != LI_ILLEGAL)
   {
      switch(ftoType)
      {
      case LI_TEXT:
         sm.AddPart(TYPETEXT,tmp->c_str(),tmp->length());
         break;

      case LI_ICON:
         // <IMG SRC="xxx"; mimetype; numMimeType;fileID >
         cptr = ocptr = strutil_strdup(tmp->c_str());
         cptr = strtok(cptr,";"); // IMG
         cptr = strtok(NULL,";"); // mimetype
         mimeType = cptr;
         mimeSubType = strutil_after(mimeType,'/');
         mimeType = strutil_before(mimeType, '/');
         cptr = strtok(NULL,";"); // numericMimeType
         numMimeType = atoi(cptr);
         cptr = strtok(NULL,";"); // fileID
//         istr.open(fileMap[atol(cptr)].c_str());
         filename = LookupFileName(atol(cptr));
         wxCHECK_RET(filename, "file not found in wxComposeView::Send");
         istr.open(filename);
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
//FIXME      tmp = m_LayoutWindow->GetContent(&ftoType, false);
   }
#endif
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
   m_LayoutWindow->GetLayoutList().SetCursor(wxPoint(0,0));
   m_LayoutWindow->GetLayoutList().Insert(txt);
   m_LayoutWindow->GetLayoutList().SetCursor(wxPoint(0,0));
}


void
wxComposeView::Print(void)
{
   m_LayoutWindow->Print();
}
