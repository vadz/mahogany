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

#  include "MApplication.h"
#  include "gui/wxMApp.h"
#endif

#include "Mdefaults.h"

#include "FolderView.h"
#include "MailFolder.h"
#include "MailFolderCC.h"
#include "Message.h"
#include "MessageCC.h"
#include "SendMessageCC.h"


#include "MDialogs.h"

#include "gui/wxFontManager.h"
#include "gui/wxIconManager.h"

#include "gui/wxllist.h"
#include "gui/wxlwindow.h"
#include "gui/wxlparser.h"
#include "gui/wxComposeView.h"


#include <wx/textfile.h>

#include "adb/AdbEntry.h"
#include "adb/AdbManager.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// control ids
enum
{
   IDB_EXPAND = 100
};

// code here was written with assumption that x and y margins are the same
#define LAYOUT_MARGIN LAYOUT_X_MARGIN

// ----------------------------------------------------------------------------
// other types
// ----------------------------------------------------------------------------
WX_DEFINE_ARRAY(AdbEntry *, ArrayAdbEntries);
WX_DEFINE_ARRAY(AdbBook *, ArrayAdbBooks);

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

      // can we close now?
      EVT_CLOSE(wxComposeView::OnCloseWindow)
   END_EVENT_TABLE()
#endif


struct MimeContent
{
   enum { MIMECONTENT_FILE, MIMECONTENT_DATA } m_Type;
   char *m_Data;
   size_t m_Length;
   String m_FileName;
   int    m_NumericMimeType;
   String m_MimeType;
   MimeContent()
      { m_Type = MIMECONTENT_FILE; m_Data = NULL; } // safe settings
   ~MimeContent()
      {
         if(m_Type == MIMECONTENT_DATA && m_Data != NULL)
            delete m_Data;
      }
};

// ============================================================================
// implementation
// ============================================================================
void
wxComposeView::Create(const String &iname, wxWindow * WXUNUSED(parent),
                      ProfileBase *parentProfile,
                      String const &to, String const &cc, String const &bcc,
                      bool hide)
{
   CHECK_RET( !initialised, "wxComposeView created twice" );

   m_LayoutWindow = NULL;
   nextFileID = 0;
   m_pManager = NULL;
   
   if(!parentProfile)
      parentProfile = mApplication->GetProfile();
   m_Profile = ProfileBase::CreateProfile(iname,parentProfile);

   // build menu
   // ----------
   AddFileMenu();
   WXADD_MENU(m_MenuBar, COMPOSE, "&Compose");
   AddHelpMenu();
   SetMenuBar(m_MenuBar);

   m_ToolBar = CreateToolBar();
   AddToolbarButtons(m_ToolBar, WXFRAME_COMPOSE);

   CreateStatusBar();

   // create the child controls
   // -------------------------

   // NB: order of item creation is important for constraints algorithm:
   // create first the controls whose constraints can be fixed first!
   wxLayoutConstraints *c;
   SetAutoLayout(TRUE);

   // Panel itself (fills all the frame client area)
   m_panel = new wxPanel(this, -1);//,-1);
   c = new wxLayoutConstraints;
   c->top.SameAs(this, wxTop);
   c->left.SameAs(this, wxLeft);
   c->width.SameAs(this, wxWidth);
   c->height.SameAs(this, wxHeight);
   m_panel->SetConstraints(c);
   m_panel->SetAutoLayout(TRUE);

   // box which contains all the header fields
   wxStaticBox *box = new wxStaticBox(m_panel, -1, "");
   c = new wxLayoutConstraints;
   c->left.SameAs(m_panel, wxLeft, LAYOUT_MARGIN);
   c->right.SameAs(m_panel, wxRight, LAYOUT_MARGIN);
   c->top.SameAs(m_panel, wxTop, LAYOUT_MARGIN);
   box->SetConstraints(c);

   // compose window
   CreateFTCanvas();
   c = new wxLayoutConstraints;
   c->left.SameAs(m_panel, wxLeft, LAYOUT_MARGIN);
   c->right.SameAs(m_panel, wxRight, LAYOUT_MARGIN);
   c->top.Below(box, LAYOUT_MARGIN);
   c->bottom.SameAs(m_panel, wxBottom, LAYOUT_MARGIN);
   m_LayoutWindow->SetConstraints(c);

   // layout the labels and text fields: label at the left of the field
   bool bDoShow[Field_Max];
   bDoShow[Field_To] =
      bDoShow[Field_Subject] = TRUE;  // To and subject always there
   bDoShow[Field_Cc] = READ_CONFIG(m_Profile, MP_SHOWCC) != 0;
   bDoShow[Field_Bcc] = READ_CONFIG(m_Profile, MP_SHOWBCC) != 0;

   // first determine the longest label caption
   static const char *aszLabels[Field_Max] =
   {
      "To:", "Subject:", "CC:", "BCC:",
   };
   wxClientDC dc(this);
   long width, widthMax = 0;
   size_t n;
   for ( n = 0; n < WXSIZEOF(aszLabels); n++ )
   {
      if ( !bDoShow[n] )
         continue;

      dc.GetTextExtent(aszLabels[n], &width, NULL);
      if ( width > widthMax )
         widthMax = width;
   }
   widthMax += LAYOUT_MARGIN;

   wxStaticText *label;
   wxWindow  *last = NULL;
   for ( n = 0; n < Field_Max; n++ )
   {
      if ( bDoShow[n] )
      {
         // text entry goes from right border of the label to the right border
         // of the box except for the first one where we must also leave space
         // for the button
         m_txtFields[n] = new wxTextCtrl(m_panel, -1, "");
         c = new wxLayoutConstraints;
         c->left.Absolute(widthMax + LAYOUT_MARGIN);
         if ( last )
            c->top.Below(last, LAYOUT_MARGIN);
         else
            c->top.SameAs(box, wxTop, 2*LAYOUT_MARGIN);
         if ( n == Field_To ) {
            // the [Expand] button
            wxButton *btnExpand = new wxButton(m_panel, IDB_EXPAND, "Expand");
            wxLayoutConstraints *c2 = new wxLayoutConstraints;
            c2->top.SameAs(box, wxTop, 2*LAYOUT_MARGIN);
            c2->right.SameAs(box, wxRight, LAYOUT_MARGIN);
            c2->width.AsIs();
            c2->height.AsIs();
            btnExpand->SetConstraints(c2);

            c->right.LeftOf(btnExpand, LAYOUT_MARGIN);
         }
         else
            c->right.SameAs(box, wxRight, LAYOUT_MARGIN);
         c->height.AsIs();
         m_txtFields[n]->SetConstraints(c);

         last = m_txtFields[n];

         // label is aligned to the left
         label = new wxStaticText(m_panel, -1, aszLabels[n],
                                  wxDefaultPosition, wxDefaultSize,
                                  wxALIGN_RIGHT);
         c = new wxLayoutConstraints;
         c->left.SameAs(box, wxLeft, LAYOUT_MARGIN);
         c->right.Absolute(widthMax);
         c->centreY.SameAs(last, wxCentreY, 2); // @@ looks better with 2...
         c->height.AsIs();
         label->SetConstraints(c);
      }
      else
      { // not shown
         m_txtFields[n] = NULL;
      }
   }

   // now we can fix the bottom of the box
   box->GetConstraints()->bottom.Below(last, -LAYOUT_MARGIN);

   // initialize the controls
   // -----------------------

   // set def values for the headers
   if(m_txtFields[Field_To]) {
      m_txtFields[Field_To]->SetValue(
         strutil_isempty(to) ? READ_CONFIG(m_Profile, MP_COMPOSE_TO) : to);
   }
   if(m_txtFields[Field_Cc]) {
      m_txtFields[Field_Cc]->SetValue(
         strutil_isempty(cc) ? READ_CONFIG(m_Profile, MP_COMPOSE_CC) : cc);
   }
   if(m_txtFields[Field_Bcc]) {
      m_txtFields[Field_Bcc]->SetValue(
         strutil_isempty(bcc) ? READ_CONFIG(m_Profile, MP_COMPOSE_BCC) : bcc);
   }

   // append signature
   if( READ_CONFIG(m_Profile, MP_COMPOSE_USE_SIGNATURE) )
   {
      String strSignFile = READ_CONFIG(m_Profile, MP_COMPOSE_SIGNATURE);
      if ( strSignFile.IsEmpty() ) {
         // propose to choose it now
         if ( MDialog_YesNoDialog(
               _("No signature file found. Would you like to choose one\n"
               "right now (otherwise no signature will be used)?"), this) ) {
            wxFileDialog dialog(this, "", "", "", _("All files (*.*)|*.*"),
                                wxHIDE_READONLY | wxFILE_MUST_EXIST);
            if ( dialog.ShowModal() == wxID_OK ) {
               strSignFile = dialog.GetPath();
            }
         }
      }

      if ( !strSignFile.IsEmpty() ) {
         wxTextFile fileSig(strSignFile);
         if ( fileSig.Open() ) {
            // insert separator optionally
            if( READ_CONFIG(m_Profile, MP_COMPOSE_USE_SIGNATURE_SEPARATOR) ) {
               m_LayoutWindow->GetLayoutList().Insert("--");
               m_LayoutWindow->GetLayoutList().LineBreak();;
            }

            // read the whole file
            size_t nLineCount = fileSig.GetLineCount();
            for ( size_t nLine = 0; nLine < nLineCount; nLine++ ) {
               m_LayoutWindow->GetLayoutList().Insert(fileSig[nLine]);
               m_LayoutWindow->GetLayoutList().LineBreak();;
            }

            // let's respect the netiquette
            static const size_t nMaxSigLines = 4; 
            if ( nLineCount > nMaxSigLines ) {
               wxLogWarning(_("Your signature is too long: it should not be more"
                              " than %d lines"), nMaxSigLines);
            }

            m_LayoutWindow->GetLayoutList().SetCursor(wxPoint(0,0));
         }
      }
      else {
         // no signature file
         m_Profile->writeEntry(MP_COMPOSE_USE_SIGNATURE, false);
      }
   }

   // show the frame
   // --------------
   initialised = true;
   if ( !hide ) {
      Show(TRUE);
      m_txtFields[Field_To]->SetFocus();
   }
}

void
wxComposeView::CreateFTCanvas(void)
{
   m_LayoutWindow = new wxLayoutWindow(m_panel);

   String
      fg = READ_CONFIG(m_Profile,MP_FTEXT_FGCOLOUR),
      bg = READ_CONFIG(m_Profile,MP_FTEXT_BGCOLOUR);
   
   m_LayoutWindow->Clear(READ_CONFIG(m_Profile,MP_FTEXT_FONT),
                         READ_CONFIG(m_Profile,MP_FTEXT_SIZE),
                         READ_CONFIG(m_Profile,MP_FTEXT_STYLE),
                         READ_CONFIG(m_Profile,MP_FTEXT_WEIGHT),
                         0,
                         fg.c_str(),bg.c_str());

   m_LayoutWindow->GetLayoutList().SetEditable(true);
   //FIXMEm_LayoutWindow->SetWrapMargin(READ_CONFIG(profile, MP_COMPOSE_WRAPMARGIN));
}

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
   SafeDecRef(m_pManager);

   if(!initialised)
      return;

   m_Profile->DecRef();
   delete m_LayoutWindow;
}


void
wxComposeView::ProcessMouse(wxMouseEvent &WXUNUSED(event))
{
}

void
wxComposeView::OnExpand(wxCommandEvent &WXUNUSED(event))
{
   String what = Str(m_txtFields[Field_To]->GetValue());
   int l = what.length();

   if ( l == 0 ) {
     wxLogStatus(this, _("Nothing to expand - please enter something."));

     return;
   }

   if ( ! m_pManager )
   {
     m_pManager = AdbManager::Get();
     m_pManager->LoadAll();
   }
   
   ArrayAdbEntries aEntries;
   if ( AdbLookup(aEntries, what) ) {
     int rc = MDialog_AdbLookupList(aEntries, this);

     if ( rc != -1 ) {
        wxString str;
        AdbEntry *pEntry = aEntries[rc];
        pEntry->GetField(AdbField_EMail, &str);
        m_txtFields[Field_To]->SetValue(str);

        pEntry->GetField(AdbField_NickName, &str);
        wxLogStatus(this, _("Expanded using entry '%s'"), str.c_str());
     }

     // free all entries
     size_t nCount = aEntries.Count();
     for ( size_t n = 0; n < nCount; n++ ) {
       aEntries[n]->DecRef();
     }
   }
   else {
     wxLogStatus(this, _("No matches for '%s'."), what.c_str());
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
wxComposeView::OnCloseWindow(wxCloseEvent& event)
{
   bool bDoClose = TRUE;
   if ( m_LayoutWindow && m_LayoutWindow->IsDirty() ) {
      bDoClose = MDialog_YesNoDialog
                 (
                  _("There are unsaved changes, close anyway?"),
                  this, // parent
                  true, // modal
                  MDIALOG_YESNOTITLE,
                  false // "yes" not default
                 ); 
   }

   if ( bDoClose ) {
      Destroy();
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
      m_LayoutWindow->ResetDirty();
      Close();
      break;

   case WXMENU_COMPOSE_PRINT:
      Print();
      break;

   case WXMENU_COMPOSE_CLEAR:
      m_LayoutWindow->Clear();
      break;

   default:
      wxMFrame::OnMenuCommand(id);
   }
}

void
wxComposeView::InsertData(const char *data,
                          size_t length,
                          const char *mimetype,
                          int num_mimetype)
{
   MimeContent *mc = new MimeContent();

   if(strutil_isempty(mimetype))
   {
      mc->m_MimeType = String("APPLICATION/OCTET-STREAM");
      mc->m_NumericMimeType = TYPEAPPLICATION;
   }
   else
   {
      mc->m_NumericMimeType = num_mimetype;
      mc->m_MimeType = mimetype;
   }
   mc->m_Data = (char *)data; // @@@
   mc->m_Length = length;
   mc->m_Type = MimeContent::MIMECONTENT_DATA;
   
   wxIcon icon = mApplication->GetIconManager()->GetIconFromMimeType(mc->m_MimeType);

   wxLayoutObjectIcon *obj = new wxLayoutObjectIcon(icon);
   obj->SetUserData(mc);
   m_LayoutWindow->GetLayoutList().Insert(obj);

   Refresh();
}

void
wxComposeView::InsertFile(const char *filename, const char *mimetype,
                          int num_mimetype)
{
   MimeContent 
      *mc = new MimeContent();

   if(filename == NULL)
   {
      filename = MDialog_FileRequester(NULLstring, this, NULLstring,
                                       NULLstring, NULLstring,
                                       NULLstring, true, m_Profile);
      if(! filename)
         return;
      mc->m_NumericMimeType = TYPEAPPLICATION;
      if(! mApplication->GetMimeTypes()->Lookup(mc->m_FileName, mc->m_MimeType, &(mc->m_NumericMimeType)))
         mc->m_MimeType = String("APPLICATION/OCTET-STREAM");
   }
   else
   {
      mc->m_NumericMimeType = num_mimetype;
      mc->m_MimeType = mimetype;
   }

   mc->m_FileName = filename;
   mc->m_Type = MimeContent::MIMECONTENT_FILE;

   wxIconManager *iconManager = mApplication->GetIconManager();

#  ifdef OS_WIN
      wxString strExt = wxString(filename).After('.');
      wxIcon icon = iconManager->GetIconFromExtension(strExt);
#  else
      wxIcon icon = iconManager->GetIconFromMimeType(mc->m_MimeType);
#  endif // Win/Unix

   wxLayoutObjectIcon *obj = new wxLayoutObjectIcon(icon);
   obj->SetUserData(mc);

   m_LayoutWindow->GetLayoutList().Insert(obj);

   wxLogStatus(this, _("Inserted file '%s' (as '%s')"),
               filename, mc->m_MimeType.c_str());

   Refresh();
}

void
wxComposeView::Send(void)
{
   String
      tmp2, mimeType, mimeSubType;
   char
      *buffer;
   ifstream
      istr;
   size_t
      size;

   SendMessageCC sm
      (
         mApplication->GetProfile(),
         (const char *)m_txtFields[Field_Subject]->GetValue(),
         (const char *)m_txtFields[Field_To]->GetValue(),
         READ_CONFIG(m_Profile, MP_SHOWCC) ? m_txtFields[Field_Cc]->GetValue().c_str()
         : (const char *)NULL,
         READ_CONFIG(m_Profile, MP_SHOWBCC) ? m_txtFields[Field_Bcc]->GetValue().c_str()
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
         sm.AddPart(TYPETEXT,export->content.text->c_str(),export->content.text->length(),
                    "PLAIN");
      else
      {
         lo = export->content.object;
         if(lo->GetType() == WXLO_TYPE_ICON)
         {
            mc = (MimeContent *)lo->GetUserData();
            switch(mc->m_Type)
            {
            case MimeContent::MIMECONTENT_FILE:
               istr.open(mc->m_FileName.c_str());
               if(istr)
               {
                  istr.seekg(0,ios::end);
                  size = istr.tellg();
                  buffer = new char [size];
                  istr.seekg(0,ios::beg);
                  istr.read(buffer, size);

                  MessageParameterList dlist;
                  MessageParameter *p = new MessageParameter;
                  p->name = "FILENAME"; p->value=wxFileNameFromPath(mc->m_FileName);
                  dlist.push_back(p);
                  if(! istr.fail())
                     sm.AddPart(mc->m_NumericMimeType, buffer, size,
                                strutil_after(mc->m_MimeType,'/'), //subtype
                                "INLINE",&dlist,NULL
                        );
                  else
                     SYSERRMESSAGE((_("Cannot read file."),this));
                  delete [] buffer;
                  istr.close();
               }
               else
               {
                  String dirtyHack = "Cannot open file: ";
                  dirtyHack +=mc->m_FileName;
                  SYSERRMESSAGE((_(Str(dirtyHack)),this));
               }
               break;
            case MimeContent::MIMECONTENT_DATA:
            {
               MessageParameterList dlist;
               MessageParameter *p = new MessageParameter;
               p->name = "FILENAME"; p->value=wxFileNameFromPath(mc->m_FileName);
               dlist.push_back(p);
               sm.AddPart(mc->m_NumericMimeType, mc->m_Data, mc->m_Length,
                          strutil_after(mc->m_MimeType,'/'),  //subtype
                          "INLINE",&dlist,NULL
                  );
               break;
            }
            }
         }
      }
      delete export;
   }
   sm.Send();
   if(READ_CONFIG(m_Profile,MP_USEOUTGOINGFOLDER))
   {
      String file;
      MailFolderCC *mf =
         MailFolderCC::OpenFolder(READ_CONFIG(m_Profile,MP_OUTGOINGFOLDER));
      if(mf)
      {
         file = READ_CONFIG(mf->GetProfile(),MP_FOLDER_PATH);
         if(strutil_isempty(file))
            file = READ_CONFIG(m_Profile,MP_OUTGOINGFOLDER);
         sm.WriteToFile(file,true/*append*/);
         mf->Close();
      }
   }
}

/// sets To field
void
wxComposeView::SetTo(const String &to)
{
   m_txtFields[Field_To]->SetValue(WXCPTR to.c_str());
}

/// sets CC field
void
wxComposeView::SetCC(const String &cc)
{
   m_txtFields[Field_Cc]->SetValue(WXCPTR cc.c_str());
}

/// sets Subject field
void
wxComposeView::SetSubject(const String &subj)
{
   m_txtFields[Field_Subject]->SetValue(WXCPTR subj.c_str());
}

/// inserts a text
void
wxComposeView::InsertText(const String &txt)
{
   m_LayoutWindow->GetLayoutList().SetCursor(wxPoint(0,0));
   wxLayoutImportText(m_LayoutWindow->GetLayoutList(), txt);
   m_LayoutWindow->GetLayoutList().SetCursor(wxPoint(0,0));
}


void
wxComposeView::Print(void)
{
   m_LayoutWindow->Print();
}
