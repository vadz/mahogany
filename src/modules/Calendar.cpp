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
#   include "strutil.h"
#   include "Mdefaults.h"
#   include "gui/wxMenuDefs.h"
#   include "MMainFrame.h"
#endif

#ifdef EXPERIMENTAL

#include "MModule.h"

#include "Mversion.h"
#include "MInterface.h"
#include "Message.h"

#include "SendMessageCC.h"

#include "gui/wxDialogLayout.h"
#include "gui/wxOptionsDlg.h"
#include "gui/wxOptionsPage.h"
#include "gui/wxMDialogs.h"

#include <wx/menu.h>
#include <wx/calctrl.h>

#define MODULE_NAME    "Calendar"

#define MP_MOD_CALENDAR_BOX        "Calendar"
#define MP_MOD_CALENDAR_BOX_D      "Calendar"
#define MP_MOD_CALENDAR_SHOWONSTARTUP        "ShowOnStartup"
#define MP_MOD_CALENDAR_SHOWONSTARTUP_D      1l

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
      { m_MInterface->Message(msg,NULL,"Calendar module error!");wxYield(); }
   inline void Message(const String &msg)
      { m_MInterface->Message(msg,NULL,"Calendar module"); wxYield(); }
   inline void StatusMessage(const String &msg)
      { m_MInterface->StatusMessage(msg);wxYield();}

   
   String m_CalendarFolder;
   bool m_Show;
   
   void CreateFrame(void);
   void TellDeleteFrame(void) { m_Frame = NULL; m_CalCtrl = NULL; }
   friend class CalendarFrame;
   
   MFrame *m_Frame;
   wxCalendarCtrl *m_CalCtrl;
   
   GCC_DTOR_WARN_OFF
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
CalendarModule::GetConfig(void)
{
   ProfileBase * appConf = m_MInterface->GetGlobalProfile();
   m_CalendarFolder = appConf->readEntry(MP_MOD_CALENDAR_BOX,
                                         MP_MOD_CALENDAR_BOX_D); 
   m_Show = appConf->readEntry(MP_MOD_CALENDAR_SHOWONSTARTUP,
                               MP_MOD_CALENDAR_SHOWONSTARTUP_D);  
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
   m_CalCtrl = new wxCalendarCtrl(m_Frame,-1);
   m_Frame->Fit();
   m_Frame->Show(m_Show);
}

CalendarModule::CalendarModule(MInterface *minterface)
            : MModule()
{
   SetMInterface(minterface);

   wxMenu * calendarMenu = new wxMenu("", wxMENU_TEAROFF);
   calendarMenu->Append(WXMENU_MODULES_CALENDAR_SHOW, _("&Show"));
   calendarMenu->Break();
   calendarMenu->Append(WXMENU_MODULES_CALENDAR_CONFIG, _("&Configure"));

   MAppBase *mapp = m_MInterface->GetMApplication();
   ((wxMainFrame *)mapp->TopLevelFrame())->AddModulesMenu(_("&Calendar Module"),
                                        _("Functionality to interact with your Calendar based palmtop."),
                                        calendarMenu,
                                        -1);
   GetConfig();
   CreateFrame();
}

CalendarModule::~CalendarModule()
{
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
   { gettext_noop("Mailbox for calendar"), wxOptionsPage::Field_Text, 0},
   { gettext_noop("Show on Startup"), wxOptionsPage::Field_Bool, 0},
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
   ProfileBase * p= m_MInterface->CreateModuleProfile(MODULE_NAME);
   ShowCustomOptionsDialog(gs_OptionsPageDesc, p, NULL);
   p->DecRef();
}


#endif // experimental
