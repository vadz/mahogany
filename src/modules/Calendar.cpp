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
#include <wx/textdlg.h>

#define MODULE_NAME    "Calendar"

#define MP_MOD_CALENDAR_BOX        "Calendar"
#define MP_MOD_CALENDAR_BOX_D      "Calendar"
#define MP_MOD_CALENDAR_SHOWONSTARTUP        "ShowOnStartup"
#define MP_MOD_CALENDAR_SHOWONSTARTUP_D      1l


class AlarmInfo
{
public:
   AlarmInfo(int year, int month, int day, const String &subject)
      {
         m_Year = year; m_Month = month; m_Day = day;
         m_Subject = subject;
      }
private:
   int m_Year, m_Month, m_Day;
   wxString m_Subject;
};

#include <wx/dynarray.h>
WX_DEFINE_ARRAY(AlarmInfo *, AlarmList);



///------------------------------
/// MModule interface:
///------------------------------

class CalendarModule : public MModule
{
   /** Override the Entry() function to allow Main() and Config()
       functions. */
   virtual int Entry(int arg, ...);
   void Configure(void);
   MMODULE_DEFINE();

private:
   /** Calendar constructor.
       As the class has no usable interface, this doesn´t do much, but
       it displays a small dialog to say hello.
   */
   CalendarModule(MInterface *mi);

   ~CalendarModule();

   bool ProcessMenuEvent(int id);
   void GetConfig(void);
   inline void ErrorMessage(const String &msg)
      { m_MInterface->MessageDialog(msg,NULL,"Calendar module error!");wxYield(); }
   inline void Message(const String &msg)
      { m_MInterface->MessageDialog(msg,NULL,"Calendar module"); wxYield(); }
   inline void StatusMessageDialog(const String &msg)
      { m_MInterface->StatusMessage(msg);wxYield();}


   String MakeDateLine(const wxDateTime &dt);
   wxDateTime ParseDateLine(const wxString &line);

   /// build the alarms list
   void ParseFolder(void);
   void ClearAlarms(void);
   
   void AddReminder(void);
   void AddReminder(const wxString &text);

   ProfileBase *m_Profile;
   String m_FolderName;
   String m_MyEmail;
   bool m_Show;
   ASMailFolder *m_Folder;
   AlarmList m_Alarms;
   
   bool OnMEvent(MEventData &event);
   class CalEventReceiver *m_EventReceiver;
   friend class CalEventReceiver;
   
   void CreateFrame(void);
   void TellDeleteFrame(void) { m_Frame = NULL; m_CalCtrl = NULL; }
   friend class CalendarFrame;
   MFrame *m_Frame;
   wxCalendarCtrl *m_CalCtrl;
   
   GCC_DTOR_WARN_OFF
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

bool
CalendarModule::ProcessMenuEvent(int id)
{
   switch(id)
   {
      case WXMENU_MODULES_CALENDAR_CONFIG:
         Configure();
         return TRUE;
      case WXMENU_MODULES_CALENDAR_SHOW:
         if(! m_Frame)
            CreateFrame();
         if(m_Show)
            m_Show = FALSE;
         else
            m_Show = TRUE;
         m_Frame->Show(m_Show);
         return TRUE;
      case WXMENU_MODULES_CALENDAR_ADD_REMINDER:
         AddReminder();
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



MMODULE_BEGIN_IMPLEMENT(CalendarModule,
                        "Calendar",
                        "Calendar",
                        "This module provides a calendar and scheduling plugin.",
                        "0.00")
MMODULE_END_IMPLEMENT(CalendarModule)


///------------------------------
/// Own functionality:
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

// This class tells the module when it disappears
class CalendarFrame : public wxMFrame
{
public:
   CalendarFrame(CalendarModule *module) : wxMFrame(_("Mahogany : Calendar"), NULL )
      { m_Module = module; }
   ~CalendarFrame()
      {
         m_Module->TellDeleteFrame();
      }
private:
   CalendarModule *m_Module;
};

void
CalendarModule::CreateFrame(void)
{
   m_Frame = new CalendarFrame(this);

   m_Frame->AddFileMenu();
   m_Frame->AddHelpMenu();

   wxMenu * menu = new wxMenu(wxMENU_TEAROFF);
   menu->Append(WXMENU_MODULES_CALENDAR_ADD_REMINDER, _("&Add Reminder"));
   m_Frame->GetMenuBar()->Append(menu,_("&Calendar"));
   
   m_CalCtrl = new wxCalendarCtrl(m_Frame,-1);
   m_Frame->Fit();
   m_Frame->Show(m_Show);
}

CalendarModule::CalendarModule(MInterface *minterface)
            : MModule()
{
   SetMInterface(minterface);

   m_Profile = m_MInterface->CreateModuleProfile(MODULE_NAME);

   wxMenu * calendarMenu = new wxMenu("", wxMENU_TEAROFF);
   calendarMenu->Append(WXMENU_MODULES_CALENDAR_SHOW, _("&Show"));
   calendarMenu->Break();
   calendarMenu->Append(WXMENU_MODULES_CALENDAR_CONFIG, _("&Configure"));

   MAppBase *mapp = m_MInterface->GetMApplication();
   ((wxMainFrame *)mapp->TopLevelFrame())->AddModulesMenu(_("&Calendar Module"),
                                        _("Functionality to interact with your Calendar based palmtop."),
                                        calendarMenu,
                                        -1);
   m_Folder = NULL;
   m_EventReceiver = new CalEventReceiver(this);
   GetConfig();
   CreateFrame();
}

CalendarModule::~CalendarModule()
{
   delete m_EventReceiver;
   if(m_Folder) m_Folder->DecRef();
   m_Profile->DecRef();
}


void
CalendarModule::GetConfig(void)
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
   fp->DecRef();

   // updates:
   if(m_FolderName != oldFolderName)
   {
      if(m_Folder) m_Folder->DecRef();
      m_Folder = ASMailFolder::OpenFolder(m_FolderName);
      ParseFolder();
   }
}

String
CalendarModule::MakeDateLine(const wxDateTime &dt)
{
   String line;
   line.Printf("%ld %ld %ld",
               (long int) dt.GetYear(),
               (long int) dt.GetMonth(),
               (long int) dt.GetDay());
   return line;
}

wxDateTime
CalendarModule::ParseDateLine(const wxString &line)
{
   wxDateTime dt;
   long year, month, day;
   if(sscanf(line.c_str(),"%ld %ld %ld", &year, &month, &day) != 3)
   {
      ErrorMessage(_("Cannot parse date information."));
      return wxDateTime::Today();
   }
   dt.SetYear(year); dt.SetMonth((wxDateTime::Month)month); dt.SetDay(day);
   dt.SetHour(0); dt.SetMinute(0); dt.SetSecond(0);
   dt.SetMillisecond(0);
   return dt;
}

void
CalendarModule::AddReminder(void)
{
   wxString text =
      wxGetTextFromUser(_("Please enter the text for the reminder:"),
                        _("Mahogany: Create Reminder"),
                        ""/* default */,
                        m_Frame,
                        -1,-1, TRUE);
   AddReminder(text);
}


bool
CalendarModule::OnMEvent(MEventData &event)
{
   return TRUE;
}

void
CalendarModule::ClearAlarms(void)
{
#if 0
   for(size_t count = 0; count < m_Alarms.Count(); count++)
   {
      delete m_Alarms[count];
   }
#endif
   m_Alarms.Clear();
}

void
CalendarModule::ParseFolder(void)
{
   // we are using synchronous access which is soo much easier:
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
            if(tmp == "REMIND")
            {
               msg->GetHeaderLine("X-M-CAL-DATE", tmp);
               wxDateTime dt = ParseDateLine(tmp);
               AlarmInfo * ai = new AlarmInfo(dt.GetYear(),
                                              dt.GetMonth(),
                                              dt.GetDay(),
                                              (*hil)[count]->GetSubject());
               m_Alarms.Add(ai);
            }
            msg->DecRef();
         }
      }
      hil->DecRef();
   }
}

void
CalendarModule::AddReminder(const wxString &itext)
{
   String fmt, text;
   wxString timeStr = wxDateTime::Now().Format("%d %b %Y %H:%M:%S");

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

   class Message * msg = m_MInterface->CreateMessage(text,UID_ILLEGAL,m_Profile);
   (void) m_Folder->AppendMessage(msg);
   msg->DecRef();
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
   ShowCustomOptionsDialog(gs_OptionsPageDesc, m_Profile, NULL);
}


#endif // experimental
