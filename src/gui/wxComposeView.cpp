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

   END_EVENT_TABLE()
#endif


   struct MimeContent
   {
      enum { MIMECONTENT_FILE, MIMECONTENT_DATA } m_Type;
      const char *m_Data;
      size_t m_Length;
      String m_FileName;
      int    m_NumericMimeType;
      String m_MimeType;
      MimeContent()
         { m_Type = MIMECONTENT_FILE; m_Data = NULL; } // safe settings
      ~MimeContent()
         {
            if(m_Type == MIMECONTENT_DATA && m_Data != NULL)
               delete [] m_Data;
         }
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

#ifdef USE_WXWINDOWS2
   // @@ SetLabelPosition(wxVERTICAL) in wxWin2 ??
#else
   m_panel->SetLabelPosition(wxVERTICAL);
#endif

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
   bDoShow[Field_Cc] = READ_CONFIG(profile, MP_SHOWCC);
   bDoShow[Field_Bcc] = READ_CONFIG(profile, MP_SHOWBCC);

   // first determine the longest label caption
   static const char *aszLabels[Field_Max] =
   {
      "To:", "Subject:", "CC:", "BCC:",
   };
   wxClientDC dc(this);
   long width, widthMax = 0;
   uint n;
   for ( n = 0; n < WXSIZEOF(aszLabels); n++ ) {
      if ( !bDoShow[n] )
         continue;

      dc.GetTextExtent(aszLabels[n], &width, NULL);
      if ( width > widthMax )
         widthMax = width;
   }
   widthMax += LAYOUT_MARGIN;

   wxStaticText *label;
   wxWindow  *last = NULL;
   for ( n = 0; n < Field_Max; n++ ) {
      if ( bDoShow[n] ) {
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
            wxButton *aliasButton = new wxButton(m_panel, -1, "Expand");
            wxLayoutConstraints *c2 = new wxLayoutConstraints;
            c2->top.SameAs(box, wxTop, 2*LAYOUT_MARGIN);
            c2->right.SameAs(box, wxRight, LAYOUT_MARGIN);
            c2->width.AsIs();
            c2->height.AsIs();
            aliasButton->SetConstraints(c2);

            c->right.LeftOf(aliasButton, LAYOUT_MARGIN);
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
         c->centreY.SameAs(last, wxCentreY, 3); // @@ looks better...
         c->height.AsIs();
         label->SetConstraints(c);
      }
      else { // not shown
         m_txtFields[n] = NULL;
      }
   }

   // now we can fix the bottom of the box
   box->GetConstraints()->bottom.Below(last, -LAYOUT_MARGIN);

   // initialize the controls
   // -----------------------

   // set def values for the headers
   m_txtFields[Field_To]->SetValue(
      strutil_isempty(to) ? READ_CONFIG(profile, MP_COMPOSE_TO) : to.c_str());
   m_txtFields[Field_Cc]->SetValue(
      strutil_isempty(cc) ? READ_CONFIG(profile, MP_COMPOSE_CC) : cc.c_str());
   m_txtFields[Field_Bcc]->SetValue(
      strutil_isempty(bcc) ? READ_CONFIG(profile, MP_COMPOSE_BCC) : bcc.c_str());

   // append signature
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
            wxLayoutImportText(m_LayoutWindow->GetLayoutList(),buffer);
         delete [] buffer;
         istr.close();
         m_LayoutWindow->GetLayoutList().SetCursor(wxPoint(0,0));
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

   String   tmp = Str(m_txtFields[Field_To]->GetValue());
   int l = tmp.length();

   // FIXME @@@ VZ: I don't know what this test does, so I disabled it
   //if ( tmp.substr(l - 1, 1) == " " )

   const char *expStr = NULL;
   if(l > 0)
   {
      tmp = tmp.substr(0,l-1);
      m_txtFields[Field_To]->SetValue(WXCPTR tmp.c_str());
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
      m_txtFields[Field_To]->SetValue(WXCPTR tmp.c_str());
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
   mc->m_Data = data;
   mc->m_Length = length;
   mc->m_Type = MimeContent::MIMECONTENT_DATA;
   
   wxLayoutObjectIcon *obj = new wxLayoutObjectIcon(mApplication.GetIconManager()->GetIcon(mc->m_MimeType));
   obj->SetUserData(mc);
   m_LayoutWindow->GetLayoutList().Insert(obj);
}

void
wxComposeView::InsertFile(const char *filename, const char *mimetype,
                          int num_mimetype)
{
   MimeContent 
      *mc = new MimeContent();

   if(filename == NULL)
   {
      filename = MDialog_FileRequester(NULL,this,NULL,NULL,NULL,NULL,true,profile);
      if(! filename)
         return;
      mc->m_NumericMimeType = TYPEAPPLICATION;
      if(! mApplication.GetMimeTypes()->Lookup(mc->m_FileName, mc->m_MimeType, &(mc->m_NumericMimeType)))
         mc->m_MimeType = String("APPLICATION/OCTET-STREAM");
   }
   else
   {
      mc->m_NumericMimeType = num_mimetype;
      mc->m_MimeType = mimetype;
   }

   mc->m_FileName = filename;
   mc->m_Type = MimeContent::MIMECONTENT_FILE;
   wxLayoutObjectIcon *obj = new wxLayoutObjectIcon(mApplication.GetIconManager()->GetIcon(mc->m_MimeType));
   obj->SetUserData(mc);

   m_LayoutWindow->GetLayoutList().Insert(obj);
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
         mApplication.GetProfile(),
         (const char *)m_txtFields[Field_Subject]->GetValue(),
         (const char *)m_txtFields[Field_To]->GetValue(),
         READ_CONFIG(profile, MP_SHOWCC) ? m_txtFields[Field_Cc]->GetValue().c_str()
         : (const char *)NULL,
         READ_CONFIG(profile, MP_SHOWBCC) ? m_txtFields[Field_Bcc]->GetValue().c_str()
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
               {
                  String dirtyHack = "Cannot open file: ";
                  dirtyHack +=mc->m_FileName;
                  SYSERRMESSAGE((_(Str(dirtyHack)),this));
               }
               break;
            case MimeContent::MIMECONTENT_DATA:
               sm.AddPart(mc->m_NumericMimeType, mc->m_Data, mc->m_Length,
                          strutil_after(mc->m_MimeType,'/')  //subtype
                        );
               break;
            }
         }
      }
      delete export;
   }
   sm.Send();
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
   m_LayoutWindow->GetLayoutList().Insert(txt);
   m_LayoutWindow->GetLayoutList().SetCursor(wxPoint(0,0));
}


void
wxComposeView::Print(void)
{
   m_LayoutWindow->Print();
}
