/*-*- c++ -*-********************************************************
 * Calendar - a Calendar module for Mahogany                        *
 *                                                                  *
 * (C) 2000 by Karsten Ballüder (Ballueder@gmx.net)                 *
 *                                                                  *
 * $Id$
 *******************************************************************/

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
#   include "Mconfig.h"
#   include "Mcommon.h"
#   include "MDialogs.h"
#   include "Mdefaults.h"
#   include "gui/wxMenuDefs.h"
#   include "MMainFrame.h"
#endif

#ifdef EXPERIMENTAL


#include "MModule.h"
#include "modules/Calendar.h"

#include "Mversion.h"
#include "MInterface.h"
#include "Message.h"

#include "ASMailFolder.h"
#include "Message.h"

#include "SendMessageCC.h"

#include "gui/wxDialogLayout.h"
#include "gui/wxOptionsDlg.h"
#include "gui/wxOptionsPage.h"
#include "gui/wxMDialogs.h"

#include <wx/menu.h>
#include <wx/calctrl.h>
#include <wx/persctrl.h>
#include <wx/textdlg.h>
#include <wx/layout.h>
#include <wx/statusbr.h>

#define MODULE_NAME    MMODULE_INTERFACE_CALENDAR

#define MP_MOD_CALENDAR_BOX        "Calendar"
#define MP_MOD_CALENDAR_BOX_D      "Calendar"
#define MP_MOD_CALENDAR_SHOWONSTARTUP        "ShowOnStartup"
#define MP_MOD_CALENDAR_SHOWONSTARTUP_D      1l

enum ActionEnum { CAL_ACTION_ILLEGAL, CAL_ACTION_REMIND, CAL_ACTION_SEND };

class AlarmInfo : public MObject
{
public:
   AlarmInfo(const wxDateTime &dt, const String &subject,
             UIdType uid,
             ActionEnum action = CAL_ACTION_REMIND)
      {
         m_DT = dt;
         m_Subject = subject;
         m_Action = action;
         m_UId = uid;
      }
   const wxDateTime & GetDate(void) const
      { return m_DT; }
   const wxString & GetSubject(void) const
      { return m_Subject; }
   ActionEnum GetAction(void) const
      { return m_Action; }
   UIdType GetUId(void) const
      { return m_UId; }
private:
   wxDateTime m_DT;
   wxString m_Subject;
   ActionEnum m_Action;
   UIdType m_UId;

   MOBJECT_NAME(AlarmInfo);
};

#include <wx/dynarray.h>
WX_DEFINE_ARRAY(AlarmInfo *, AlarmList);



///------------------------------
/// MModule interface:
///------------------------------

class CalendarModule : public MModule_Calendar
{
   /** Override the Entry() function to allow Main() and Config()
       functions. */
   virtual int Entry(int arg, ...);
   void Configure(void);
   MMODULE_DEFINE();

   virtual void ScheduleMessage(class SendMessageCC *msg);
   
   void OnTimer(void);
private:
   /** Calendar constructor.
       As the class has no usable interface, this doesn´t do much, but
       it displays a small dialog to say hello.
   */
   CalendarModule(MInterface *mi);

   ~CalendarModule();

   bool ProcessMenuEvent(int id);
   inline void ErrorMessage(const String &msg)
      { m_MInterface->MessageDialog(msg,NULL,"Calendar module error!");wxYield(); }

   bool OnMEvent(MEventData &event);
   class CalEventReceiver *m_EventReceiver;
   friend class CalEventReceiver;
   
   void CreateFrame(void);
   void TellDeleteFrame(void)
      {
         m_Frame = NULL;
      }
   friend class CalendarFrame;
   class CalendarFrame *m_Frame;
   wxMenu *m_CalendarMenu;
   wxDateTime m_Today;


   GCC_DTOR_WARN_OFF
};


static const char *ButtonLabels [] =
{
   gettext_noop("New..."),
   gettext_noop("Schedule!"),
   gettext_noop("Delete Entry"),
   NULL
};

enum ButtonIndices
{
   Button_New = 0,
   Button_Schedule,
   Button_Delete,
   Button_Max
};

// This class tells the module when it disappears
class CalendarFrame : public wxMFrame
{
public:
   CalendarFrame(CalendarModule *module);
   ~CalendarFrame();
   bool Show(bool show);
   /// switch frame display on/off
   void Toggle();

   void AddReminder(void);
   void AddReminder(const wxString &text,
                    ActionEnum action = CAL_ACTION_REMIND,
                    const wxDateTime &when = wxDefaultDateTime);
   void ScheduleMessage(SendMessageCC *msg);
   
   /// checks if anything needs to be done:
   void CheckUpdate(void);
   /// re-reads config
   void GetConfig(void);

   void OnButton(wxCommandEvent& event);
   void OnUpdateUI(wxUpdateUIEvent& event);

protected:

   String MakeDateLine(const wxDateTime &dt);
   wxDateTime ParseDateLine(const wxString &line);

   /// build the alarms list
   void ParseFolder(void);
   void ClearAlarms(void);
   

private:
   MInterface * m_MInterface;
   /// profile settings
   ProfileBase *m_Profile;
   String m_FolderName;
   String m_MyEmail;
   String m_DateFormat;
   String m_NewMailFolder;
   ASMailFolder *m_Folder;
   AlarmList m_Alarms;
   bool m_Show;
   bool m_PickDate;

   String m_ScheduleMessage;
   
   CalendarModule *m_Module;
   wxCalendarCtrl *m_CalCtrl;
   wxListCtrl     *m_ListCtrl;
   wxButton *m_Buttons[Button_Max];
   
   DECLARE_EVENT_TABLE()
};


class CalEventReceiver : public MEventReceiver
{
public:
   CalEventReceiver(class CalendarModule *calmod)
      {
         m_CalMod = calmod;
         m_regASFResult = MEventManager::Register(*this, MEventId_ASFolderResult);
      }
   virtual bool OnMEvent(MEventData& event)
      {
         return m_CalMod->OnMEvent(event);
      }
   ~CalEventReceiver()
      {
            MEventManager::Deregister(m_regASFResult);
      }
private:
   class CalendarModule *m_CalMod;
   void *m_regASFResult;
};



///------------------------------
/// Calendar frame class:
///------------------------------
/* Calculates a common button size for buttons with labels when given 
   an array with their labels. */
static
wxSize GetButtonSize(const char * labels[], wxWindow *win)
{
   long widthLabel, heightLabel;
   long heightBtn, widthBtn;
   long maxWidth = 0, maxHeight = 0;
   
   wxClientDC dc(win);
   dc.SetFont(wxSystemSettings::GetSystemFont(wxSYS_DEFAULT_GUI_FONT));
   for(size_t idx = 0; labels[idx]; idx++)
   {
      dc.GetTextExtent(_(labels[idx]), &widthLabel, &heightLabel);
      heightBtn = TEXT_HEIGHT_FROM_LABEL(heightLabel);
      widthBtn = BUTTON_WIDTH_FROM_HEIGHT(heightBtn);
      if(widthBtn > maxWidth) maxWidth = widthBtn;
      if(heightBtn > maxHeight) maxHeight = heightBtn;
   }
   return wxSize(maxWidth, maxHeight);
}

BEGIN_EVENT_TABLE(CalendarFrame, wxMFrame)
   EVT_BUTTON(-1,CalendarFrame::OnButton)
   EVT_UPDATE_UI(-1,CalendarFrame::OnUpdateUI)
END_EVENT_TABLE()

CalendarFrame::CalendarFrame(CalendarModule *module)
   : wxMFrame(_("Mahogany : Calendar"), NULL )
{
   m_Module = module;
   m_MInterface = module->GetMInterface();
   m_Profile = m_MInterface->CreateModuleProfile(MODULE_NAME);

   AddFileMenu();
   AddHelpMenu();

   wxMenu * menu = new wxMenu(wxMENU_TEAROFF);
   menu->Append(WXMENU_MODULES_CALENDAR_ADD_REMINDER, _("&Add Reminder"));
   GetMenuBar()->Append(menu,_("&Calendar"));

   wxPanel *panel = new wxPanel(this);
   wxLayoutConstraints *c = new wxLayoutConstraints;
   c->top.SameAs(this, wxTop);
   c->left.SameAs(this, wxLeft);
   c->width.SameAs(this, wxWidth);
   c->height.SameAs(this, wxHeight);
   panel->SetConstraints(c);
   
   m_CalCtrl = new wxCalendarCtrl(panel,-1, wxDefaultDateTime,
                                  wxDefaultPosition,
                                  wxDefaultSize,
                                  wxCAL_SHOW_HOLIDAYS|wxCAL_BORDER_SQUARE);


   wxSize ButtonSize = ::GetButtonSize(ButtonLabels, panel);
   for(size_t idx = 0; ButtonLabels[idx]; idx++)
   {
      c = new wxLayoutConstraints;
      c->left.RightOf(m_CalCtrl, 40);
      c->width.Absolute(ButtonSize.GetX());
      if(idx == 0)
         c->top.SameAs(m_CalCtrl, wxTop, 20);
      else
         c->top.Below(m_Buttons[idx-1], 40);
      c->height.AsIs();
      m_Buttons[idx] = new wxButton(this, -1, _(ButtonLabels[idx]),
                                    wxDefaultPosition, ButtonSize);
      m_Buttons[idx]->SetConstraints(c);
   }

   m_ListCtrl = m_MInterface->CreatePListCtrl("CalendarModuleListCtrl",
                                panel,
                                -1,
                                wxLC_REPORT|wxBORDER);
   m_ListCtrl->InsertColumn( 0, _("Date"), wxLIST_FORMAT_LEFT);
   m_ListCtrl->InsertColumn( 1, _("Subject"), wxLIST_FORMAT_LEFT);
   m_ListCtrl->InsertColumn( 2, _("Action"), wxLIST_FORMAT_LEFT);

   c = new wxLayoutConstraints;
   c->top.Below(m_CalCtrl, 20);
   c->left.SameAs(panel, wxLeft);
   c->width.SameAs(panel, wxWidth);
   c->height.PercentOf(panel, wxHeight, 40);
   m_ListCtrl->SetConstraints(c);

   panel->SetAutoLayout(TRUE);
   SetAutoLayout(TRUE);

   CreateStatusBar();
   GetConfig();
   m_PickDate = FALSE;
   Show(m_Show);
}

void
CalendarFrame::OnUpdateUI(wxUpdateUIEvent& event)
{
   if(m_PickDate)
   {
      // special mode, all others disabled
      m_Buttons[Button_New]->Enable(FALSE);
      m_Buttons[Button_Schedule]->Enable(TRUE);
      m_Buttons[Button_Delete]->Enable(FALSE);
   }
   else
   {
      m_Buttons[Button_Delete]->Enable(
         m_ListCtrl->GetSelectedItemCount() > 0
         );
      m_Buttons[Button_New]->Enable(TRUE);
      m_Buttons[Button_Schedule]->Enable(FALSE);
   }
}

void
CalendarFrame::OnButton( wxCommandEvent &event )
{
   wxObject *obj = event.GetEventObject();

   for(size_t idx = 0; ButtonLabels[idx]; idx++)
      if(obj == m_Buttons[idx])
      {
         if(idx == Button_New)
         {
            AddReminder();
         }
         else if(idx == Button_Schedule)
         {
            AddReminder(m_ScheduleMessage, CAL_ACTION_SEND,
                        m_CalCtrl->GetDate());
         }
         else if(idx == Button_Delete)
         {
            //FIXME not implemented
         }
      }
   event.Skip();
}

CalendarFrame::~CalendarFrame()
{
   m_Module->TellDeleteFrame();
//   delete m_EventReceiver;
   if(m_Folder) m_Folder->DecRef();
   m_Profile->DecRef();
   ClearAlarms();
}

bool
CalendarFrame::Show(bool show)
{
   m_Show = show;
   m_Module->m_CalendarMenu->Check(WXMENU_MODULES_CALENDAR_SHOW, m_Show);
   return wxFrame::Show(show);
}

void
CalendarFrame::Toggle(void)
{
   if(m_Show)
      m_Show = FALSE;
   else
      m_Show = TRUE;
   Show(m_Show);
}



void
CalendarFrame::GetConfig(void)
{
   wxString oldFolderName = m_FolderName;

   // settings read from normal module profile:
   m_FolderName = m_Profile->readEntry(MP_MOD_CALENDAR_BOX,
                                       MP_MOD_CALENDAR_BOX_D); 
   m_Show = m_Profile->readEntry(MP_MOD_CALENDAR_SHOWONSTARTUP,
                                 MP_MOD_CALENDAR_SHOWONSTARTUP_D);

   // settings read from folder profile:
   ProfileBase *fp = m_MInterface->CreateProfile(m_FolderName);
   m_MyEmail = fp->readEntry(MP_RETURN_ADDRESS,
                             MP_RETURN_ADDRESS_D);
   if(m_MyEmail.Length() == 0)
   {
      m_MyEmail = fp->readEntry(MP_USERNAME, MP_USERNAME_D);
      m_MyEmail << '@' << fp->readEntry(MP_HOSTNAME, MP_HOSTNAME_D);
   }
   m_DateFormat = READ_CONFIG(fp, MP_DATE_FMT);
   m_NewMailFolder = READ_CONFIG(fp, MP_NEWMAIL_FOLDER);
   fp->DecRef();

   // updates:
   if(m_FolderName != oldFolderName)
   {
      if(m_Folder) m_Folder->DecRef();

      // check if folder profile entry exists:
      MFolder *mf = m_MInterface->GetMFolder(m_FolderName);
      if(mf == NULL)
      {
         if(! m_MInterface->CreateMailFolder(
            m_FolderName,MF_FILE,MF_FLAGS_DEFAULT|MF_FLAGS_HIDDEN,
            m_FolderName,
            _("This folder is used to store data for the calendar plugin module.")))
         {
            wxString msg;
            msg.Printf(_("Cannot create calendar module folder '%s'."),
                       m_FolderName.c_str());
            m_Module->ErrorMessage(msg);
         }
      }
      else
         mf->DecRef();
      m_Folder = m_MInterface->OpenASMailFolder(m_FolderName);
      ParseFolder();
   }
}

String
CalendarFrame::MakeDateLine(const wxDateTime &dt)
{
   String line;
   line.Printf("%ld %ld %ld",
               (long int) dt.GetYear(),
               (long int) dt.GetMonth(),
               (long int) dt.GetDay());
   return line;
}

wxDateTime
CalendarFrame::ParseDateLine(const wxString &line)
{
   wxDateTime dt;
   long year, month, day;
   if(sscanf(line.c_str(),"%ld %ld %ld", &year, &month, &day) != 3)
   {
      m_Module->ErrorMessage(_("Cannot parse date information."));
      return wxDateTime::Today();
   }
   dt.SetYear(year); dt.SetMonth((wxDateTime::Month)month); dt.SetDay(day);
   dt.SetHour(0); dt.SetMinute(0); dt.SetSecond(0);
   dt.SetMillisecond(0);
   return dt;
}

void
CalendarFrame::ScheduleMessage(SendMessageCC *msg)
{
   m_PickDate = TRUE;
   Refresh();
   Show(TRUE); // make ourselves visible
   msg->WriteToString(m_ScheduleMessage);
}

void
CalendarFrame::AddReminder(void)
{
   wxString text =
      wxGetTextFromUser(_("Please enter the text for the reminder:"),
                        _("Mahogany: Create Reminder"),
                        ""/* default */,
                        this,
                        -1,-1, TRUE);
   AddReminder(text);
}


void
CalendarFrame::ClearAlarms(void)
{
   for(size_t count = 0; count < m_Alarms.Count(); count++)
   {
      delete m_Alarms[count];
   }
   m_Alarms.Clear();
}

void
CalendarFrame::ParseFolder(void)
{
   // we are using synchronous access which is soo much easier:
   ASSERT(m_Folder);
   MailFolder *mf = m_Folder->GetMailFolder();
   ASSERT(mf);
   if(! mf)
      return;
   HeaderInfoList *hil = mf->GetHeaders();
   if(hil)
   {
      ClearAlarms();
      for(size_t count = 0; count < hil->Count(); count ++)
      {
         class Message * msg = mf->GetMessage( (*hil)[count]->GetUId() );
         if(msg)
         {
            wxString tmp;
            msg->GetHeaderLine("X-M-CAL-CMD", tmp);
            ActionEnum action = CAL_ACTION_ILLEGAL;
            if(tmp == "REMIND")
               action = CAL_ACTION_REMIND;
            else if(tmp == "SEND")
               action = CAL_ACTION_SEND;
            if(action != CAL_ACTION_ILLEGAL)
            {
               msg->GetHeaderLine("X-M-CAL-DATE", tmp);
               wxDateTime dt = ParseDateLine(tmp);
               AlarmInfo * ai = new AlarmInfo(dt,
                                              (*hil)[count]->GetSubject(),
                                              (*hil)[count]->GetUId(),
                                              action);
               m_Alarms.Add(ai);
            }
         }
         msg->DecRef();
      }
      hil->DecRef();
   }
   // update the listctrl:
   m_ListCtrl->Clear();
   for(size_t count = 0; count < m_Alarms.Count(); count++)
   {
      m_ListCtrl->InsertItem(count, m_Alarms[count]->GetDate().Format(m_DateFormat));
      m_ListCtrl->SetItem(count,1, m_Alarms[count]->GetSubject());
      m_ListCtrl->SetItem(count,2,_("reminder"));
   }
}

void
CalendarFrame::CheckUpdate(void)
{
   MailFolder *mf = m_Folder->GetMailFolder();
   ASSERT(mf);
   if(! mf) return;

   bool deleted = FALSE;
   for(size_t count = 0; count < m_Alarms.Count(); count++)
   {
      if(m_Alarms[count]->GetDate() <= wxDateTime::Now())
      {
         Message * msg = mf->GetMessage( m_Alarms[count]->GetUId() );
         if(msg)
         {
            ActionEnum action = m_Alarms[count]->GetAction();
            if(action == CAL_ACTION_REMIND)
            {
               MailFolder *nmmf = m_MInterface->OpenMailFolder(m_NewMailFolder);
               if( nmmf && nmmf->AppendMessage(*msg) )
               {
                  wxString txt;
                  txt.Printf(_("Stored reminder `%s' in mailbox `%s'."),
                             m_Alarms[count]->GetSubject().c_str(),
                             m_NewMailFolder.c_str());
                  GetStatusBar()->SetStatusText(txt);
                  mf->DeleteMessage(m_Alarms[count]->GetUId());
                  deleted = TRUE;
               }
               if(nmmf) nmmf->DecRef();
            }
            else if(action == CAL_ACTION_SEND)
            {
               if( msg->SendOrQueue() )
               {
                  wxString txt;
                  txt.Printf(_("Send or queued message `%s'."),
                             m_Alarms[count]->GetSubject().c_str());
                  GetStatusBar()->SetStatusText(txt);
                  mf->DeleteMessage(m_Alarms[count]->GetUId());
                  deleted = TRUE;
               }
            }
            msg->DecRef();
         }
         else
            m_Module->ErrorMessage(_("Message for reminder disappeared!"));
      }
   }
   if(deleted)
      mf->ExpungeMessages();
   mf->DecRef(); 
}
   
void
CalendarFrame::AddReminder(const wxString &itext,
                           ActionEnum action,
                           const wxDateTime &when)
{
   String fmt, text;
   wxString timeStr = wxDateTime::Now().Format("%d %b %Y %H:%M:%S");

   if(action == CAL_ACTION_SEND)
   {
      timeStr = when.Format("%d %b %Y %H:%M:%S");
      // we need to prepend the required header lines to the message
      // text:
      text = "";
      text << "X-M-CAL-CMD: SEND" << "\015\012"
          << "X-M-CAL-DATE: "
          << MakeDateLine(m_CalCtrl->GetDate())
          << "\015\012"
          << itext;
   }
   else
   {
      // we need to prepend the required header lines to the message
      // text:
      fmt = "From: ";
      fmt << _("Mahogany Calendar Module ")
          << '<' << m_MyEmail << '>'
          << "\015\012"
          << "Subject: " << _("Reminder") << "\015\012"
          << "Date: %s\015\012"
          << "X-M-CAL-CMD: REMIND" << "\015\012"
          << "X-M-CAL-DATE: %s" << "\015\012"
          << "\015\012"
          << itext
          << "\015\012";
      text.Printf(fmt, timeStr.c_str(),
                  MakeDateLine(m_CalCtrl->GetDate()).c_str());
   }
   class Message * msg = m_MInterface->CreateMessage(text,UID_ILLEGAL,m_Profile);
   (void) m_Folder->AppendMessage(msg);
   msg->DecRef();
}




// a timer checking every hour to see if the day has changed:
class DayCheckTimer : public wxTimer
{
public:
   DayCheckTimer(CalendarModule *module)
      { m_Module = module; m_started = FALSE; }

   virtual bool Start(void)
      { m_started = TRUE; return wxTimer::Start(60*60*1000, TRUE); }

   virtual void Notify()
      { m_Module->OnTimer(); }

    virtual void Stop()
      { if ( m_started ) wxTimer::Stop(); }

public:
   CalendarModule *m_Module;
   bool m_started;
};









MMODULE_BEGIN_IMPLEMENT(CalendarModule,
                        "Calendar",
                        "Calendar",
                        "This module provides a calendar and scheduling plugin.",
                        "0.00")
MMODULE_END_IMPLEMENT(CalendarModule)


///------------------------------
/// Module class:
///------------------------------

/* static */

MModule *
CalendarModule::Init(int version_major, int version_minor,
                   int version_release, MInterface *minterface,
                   int *errorCode)
{
   if(! MMODULE_SAME_VERSION(version_major, version_minor,
                             version_release))
   {
      if(errorCode) *errorCode = MMODULE_ERR_INCOMPATIBLE_VERSIONS;
      return NULL;
   }

   return new CalendarModule(minterface);
}

CalendarModule::CalendarModule(MInterface *minterface)
{
   SetMInterface(minterface);
   m_Frame = NULL;
   m_CalendarMenu = new wxMenu("", wxMENU_TEAROFF);
   m_CalendarMenu->Append(WXMENU_MODULES_CALENDAR_SHOW, _("&Show"), "", TRUE);
   m_CalendarMenu->Break();
   m_CalendarMenu->Append(WXMENU_MODULES_CALENDAR_CONFIG, _("&Configure"));

   MAppBase *mapp = m_MInterface->GetMApplication();
   ((wxMainFrame *)mapp->TopLevelFrame())->AddModulesMenu(_("&Calendar Module"),
                                        _("Functionality to interact with your Calendar based palmtop."),
                                        m_CalendarMenu,
                                        -1);
   m_Today = wxDateTime::Today();

//   m_EventReceiver = new CalEventReceiver(this);
   CreateFrame();
   m_Frame->CheckUpdate();
}

CalendarModule::~CalendarModule()
{
   if(m_Frame) m_Frame->Close();
}



bool
CalendarModule::ProcessMenuEvent(int id)
{
   switch(id)
   {
      case WXMENU_MODULES_CALENDAR_CONFIG:
         Configure();
         CreateFrame();
         m_Frame->GetConfig();
         return TRUE;
      case WXMENU_MODULES_CALENDAR_SHOW:
         CreateFrame();
         m_Frame->Toggle();
         return TRUE;
      case WXMENU_MODULES_CALENDAR_ADD_REMINDER:
         CreateFrame();
         m_Frame->AddReminder();
         return TRUE;
      default:
         return FALSE;
   }
}

int
CalendarModule::Entry(int arg, ...)
{
   switch(arg)
   {
      // GetFlags():
      case MMOD_FUNC_GETFLAGS:
         return MMOD_FLAG_HASMAIN|MMOD_FLAG_HASCONFIG;

      // Main():
      case MMOD_FUNC_MAIN:
//         Synchronise(NULL);
         return 0;

      // Configure():
      case MMOD_FUNC_CONFIG:
         Configure();
         return 0;

      // Menu event
      case MMOD_FUNC_MENUEVENT:
      {
         va_list ap;
         va_start(ap, arg);
         int id = va_arg(ap, int);
         va_end(ap);
         return ProcessMenuEvent(id);
      }

      default:
         return 0;
   }
}

void
CalendarModule::ScheduleMessage(class SendMessageCC *msg)
{
   CreateFrame();
   m_Frame->ScheduleMessage(msg);
}

   

void
CalendarModule::CreateFrame(void)
{
   if(m_Frame)
      return; // nothing to do
   m_Frame = new CalendarFrame(this);
}

bool
CalendarModule::OnMEvent(MEventData &event)
{
   return TRUE;
}


void
CalendarModule::OnTimer(void)
{
   if( wxDateTime::Today() > m_Today )
   {
      // the day changed!
      m_Today = wxDateTime::Today();
      CreateFrame();
      m_Frame->CheckUpdate();
   }
}
//---------------------------------------------------------
// The configuration dialog
//---------------------------------------------------------

#include "gui/wxDialogLayout.h"
#include "MHelp.h"


static ConfigValueDefault gs_ConfigValues [] =
{
   ConfigValueDefault(MP_MOD_CALENDAR_BOX, MP_MOD_CALENDAR_BOX_D),
   ConfigValueDefault(MP_MOD_CALENDAR_SHOWONSTARTUP, MP_MOD_CALENDAR_SHOWONSTARTUP_D),
};

static wxOptionsPage::FieldInfo gs_FieldInfos[] =
{
   { gettext_noop("Mailbox for calendar"), wxOptionsPage::Field_Text, -1},
   { gettext_noop("Show on Startup"), wxOptionsPage::Field_Bool, -1},
};

static
struct wxOptionsPageDesc  gs_OptionsPageDesc =
{
   gettext_noop("Calendar module preferences"),
   "calendar",// image
   MH_MODULES_CALENDAR_CONFIG,
   // the fields description
   gs_FieldInfos,
   gs_ConfigValues,
   WXSIZEOF(gs_FieldInfos)
};

void
CalendarModule::Configure(void)
{
   ProfileBase *p = m_MInterface->CreateModuleProfile(MODULE_NAME);
   ShowCustomOptionsDialog(gs_OptionsPageDesc, p, NULL);
   p->DecRef();
}


#endif // experimental
