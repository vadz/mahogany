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
#   include "Mcommon.h"
#   include "MHelp.h"

#   include <wx/layout.h>
#   include <wx/menu.h>
#   include <wx/statusbr.h>
#   include <wx/statbox.h>
#   include <wx/textdlg.h>
#endif // USE_PCH

#include "modules/Calendar.h"

#include "UIdArray.h"

#include "HeaderInfo.h"
#include "SendMessage.h"

#include "gui/wxOptionsDlg.h"
#include "gui/wxOptionsPage.h"
#include "gui/wxMainFrame.h"

#include <wx/calctrl.h>
#include <wx/spinbutt.h>

#if wxUSE_DRAG_AND_DROP
   #include "Mdnd.h"
#endif // wxUSE_DRAG_AND_DROP

#define MODULE_NAME    MMODULE_INTERFACE_CALENDAR

#define MP_MOD_CALENDAR_BOX        _T("Calendar")
#define MP_MOD_CALENDAR_BOX_D      _T("Calendar")
#define MP_MOD_CALENDAR_SHOWONSTARTUP        _T("ShowOnStartup")
#define MP_MOD_CALENDAR_SHOWONSTARTUP_D      1l

extern const MOption MP_DATE_FMT;
extern const MOption MP_FROM_ADDRESS;
extern const MOption MP_NEWMAIL_FOLDER;

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

class wxDateTimeWithRepeat : public wxDateTime
{
public:
   wxDateTimeWithRepeat(const wxDateTime &dt,
                        const wxDateTime &edt)
      : wxDateTime(dt)
      {
         m_DayRepeat = 0; m_MonthRepeat = 0; m_YearRepeat = 0;
         m_EndDate = edt;
      }
   wxDateTimeWithRepeat(const wxDateTime &dt = wxDefaultDateTime)
      : wxDateTime(dt)
      {
         m_DayRepeat = 0; m_MonthRepeat = 0; m_YearRepeat = 0;
         m_EndDate = dt;
         // m_EndDate == dt --> no end date
      }

   long GetDayRepeat(void) const   { return m_DayRepeat; }
   long GetMonthRepeat(void) const { return m_MonthRepeat; }
   long GetYearRepeat(void) const  { return m_YearRepeat; }

   void SetDayRepeat(long int d)   { m_DayRepeat = d; }
   void SetWeekRepeat(long int w)  { SetDayRepeat(w*7); }
   void SetMonthRepeat(long int m) { m_MonthRepeat = m; }
   void SetYearRepeat(long int y)  { m_YearRepeat = y; }

   bool HasEndDate(void) const
      {
         return GetDay() != m_EndDate.GetDay()
            || GetMonth() != m_EndDate.GetMonth()
            || GetYear() != m_EndDate.GetYear()
            ;
      }
   bool IsRepeating(void) const
      {
         return GetDayRepeat() != 0
            || GetMonthRepeat() != 0
            || GetYearRepeat() != 0;
      }
   const wxDateTime & GetEndDate(void) const { return m_EndDate; }
   void SetEndDate(const wxDateTime &dt) {  m_EndDate = dt; }

   long GetDayRepeatEnd(void) const   { return m_EndDate.GetDay(); }
   long GetMonthRepeatEnd(void) const { return m_EndDate.GetMonth(); }
   long GetYearRepeatEnd(void) const  { return m_EndDate.GetYear(); }

private:
   long m_DayRepeat,
        m_MonthRepeat,
        m_YearRepeat;

   wxDateTime m_EndDate;
};

enum ActionEnum { CAL_ACTION_ILLEGAL, CAL_ACTION_REMIND, CAL_ACTION_SEND };

class AlarmInfo : public MObject
{
public:
   AlarmInfo(const wxDateTimeWithRepeat &dt, const String &subject,
             UIdType uid,
             ActionEnum action = CAL_ACTION_REMIND)
      {
         m_DT = dt;
         m_Subject = subject;
         m_Action = action;
         m_UId = uid;
      }
   const wxDateTimeWithRepeat & GetDate(void) const
      { return m_DT; }
   const wxString & GetSubject(void) const
      { return m_Subject; }
   ActionEnum GetAction(void) const
      { return m_Action; }
   UIdType GetUId(void) const
      { return m_UId; }
private:
   wxDateTimeWithRepeat m_DT;
   wxString m_Subject;
   ActionEnum m_Action;
   UIdType m_UId;

   MOBJECT_NAME(AlarmInfo);
};

WX_DEFINE_ARRAY(AlarmInfo *, AlarmList);


// a timer checking every hour to see if the day has changed:
class DayCheckTimer : public wxTimer
{
public:
   DayCheckTimer(class CalendarModule *module)
      { m_Module = module; m_started = FALSE; }

   virtual bool Start(void)
      { m_started = TRUE; return wxTimer::Start(60*60*1000, TRUE); }

   virtual void Notify(void);

    virtual void Stop()
      { if ( m_started ) wxTimer::Stop(); }

public:
   class CalendarModule *m_Module;
   bool m_started;

   DECLARE_NO_COPY_CLASS(DayCheckTimer)
};

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

   virtual bool ScheduleMessage(class SendMessage *msg);
   virtual bool ScheduleMessage(class Message *msg);

   void OnTimer(void);
   bool OnASFolderResultEvent(MEventASFolderResultData & ev );
   bool OnFolderUpdateEvent(MEventFolderUpdateData &ev );

private:
   /** Calendar constructor.
       As the class has no usable interface, this doesn't do much, but
       it displays a small dialog to say hello.
   */
   CalendarModule(MInterface *mi);

   /// register with the main frame
   bool RegisterWithMainFrame();

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

   DayCheckTimer *m_Timer;
   GCC_DTOR_WARN_OFF
};

void
DayCheckTimer::Notify(void)
{
   m_Module->OnTimer();
}

class wxDateDialog : public wxManuallyLaidOutDialog
{
public:
   wxDateDialog(const wxDateTime &dt, wxWindow *parent);

   // reset the selected options to their default values
   virtual bool TransferDataFromWindow()
      {
         m_Date = m_CalCtrl->GetDate();
         if(m_CheckBox->GetValue())
         {
            long int num = m_SpinButton->GetValue();

            switch(m_Choice->GetSelection())
            {
            case 0: // days
               m_Date.SetDayRepeat(num); break;
            case 1: // weeks
               m_Date.SetWeekRepeat(num); break;
            case 2: // months
               m_Date.SetMonthRepeat(num); break;
            case 3: // years
               m_Date.SetYearRepeat(num); break;
            default:
               ASSERT(0); // cannot happen
            }
            if(m_EndsOn->GetValue())
               m_Date.SetEndDate(m_CalCtrlEnd->GetDate());
            else
               m_Date.SetEndDate(m_Date); // no end date
         }
         return TRUE;
      }
   virtual bool TransferDataToWindow()
      {
         m_CalCtrl->SetDate(m_Date);
         if(m_Date.GetDayRepeat()
            || m_Date.GetMonthRepeat()
            || m_Date.GetYearRepeat())
         {
            m_CheckBox->Enable(TRUE);
            long num = 1;
            if(m_Date.GetDayRepeat())
            {
               num = m_Date.GetDayRepeat();
               m_Choice->SetSelection(0); // day
            }
            else if(m_Date.GetMonthRepeat())
            {
               num = m_Date.GetMonthRepeat();
               m_Choice->SetSelection(2); // month
            }
            else if(m_Date.GetYearRepeat())
            {
               num = m_Date.GetYearRepeat();
               m_Choice->SetSelection(3); // year
            }
            wxString tmp;
            tmp.Printf("%ld", num); m_TextCtrl->SetValue(tmp);

            if(m_Date.HasEndDate())
            {
               m_EndsOn->Enable(TRUE);
               m_CalCtrlEnd->SetDate(m_Date.GetEndDate());
            }
         }
         return TRUE;
      }
   void OnCheckBox(wxCommandEvent & WXUNUSED(event) )
      {
         UpdateUI();
      }
   void UpdateUI(void)
      {
         bool enable = m_CheckBox->GetValue();
         m_RepeatText->Enable(enable);
         m_TextCtrl->Enable(enable);
         m_SpinButton->Enable(enable);
         m_Choice->Enable(enable);
         m_EndsOn->Enable(enable);
         bool enable2 = enable && m_EndsOn->GetValue();
         m_CalCtrlEnd->Enable(enable2);
      }

   void OnSpin(long int delta)
      {
         wxString tmp = m_TextCtrl->GetValue();
         long l = 1;
         sscanf(tmp.c_str(),"%ld", &l);
         l += delta;
         if(l < 1)
            l = 1;
         m_SpinButton->SetValue(l);
         tmp.Printf("%ld", l);
         m_TextCtrl->SetValue(tmp);
      }
   void OnSpinUp(wxCommandEvent & WXUNUSED(event) ) { OnSpin(+1); }

   void OnSpinDown(wxCommandEvent & WXUNUSED(event) ) { OnSpin(-1); }

   wxDateTimeWithRepeat GetDate() { return m_Date; }
protected:
   wxDateTimeWithRepeat     m_Date;
   wxCalendarCtrl *m_CalCtrl;

   // for the repeat settings:
   wxStaticText *m_RepeatText;
   wxTextCtrl   *m_TextCtrl;
   wxSpinButton *m_SpinButton;
   wxChoice    *m_Choice;
   wxCheckBox  *m_CheckBox;
   wxCheckBox  *m_EndsOn;
   wxCalendarCtrl *m_CalCtrlEnd;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxDateDialog)
};

BEGIN_EVENT_TABLE(wxDateDialog, wxManuallyLaidOutDialog)
   EVT_CHECKBOX(-1, wxDateDialog::OnCheckBox)
   EVT_SPIN_UP(-1, wxDateDialog::OnSpinUp)
   EVT_SPIN_DOWN(-1, wxDateDialog::OnSpinDown)
END_EVENT_TABLE()

wxDateDialog::wxDateDialog(const wxDateTime &dt, wxWindow *parent)
   : wxManuallyLaidOutDialog(parent,_("Pick a Date"),"CalendarModuleDateDlg")
{
   wxStaticBox *box = CreateStdButtonsAndBox(_("Date"), FALSE,
                                             MH_MODULES_CALENDAR_DATEDLG);
   wxLayoutConstraints *c;

   wxStaticText *stattext = new wxStaticText(this, -1,
                                             _("Please pick the date on which to\n"
                                               "send the message\n"
                                               "\n"));
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(box, wxTop, 6*LAYOUT_Y_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   stattext->SetConstraints(c);

   m_CalCtrl = new wxCalendarCtrl(this,-1, dt,
                                  wxDefaultPosition,
                                  wxDefaultSize,
                                  wxCAL_SHOW_HOLIDAYS|wxCAL_BORDER_SQUARE);
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->top.Below(stattext, LAYOUT_Y_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   m_CalCtrl->SetConstraints(c);

   m_CheckBox = new wxCheckBox(this, -1, _("Repeat Event"));
   c = new wxLayoutConstraints;
   c->left.RightOf(m_CalCtrl, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(box, wxTop, 6*LAYOUT_Y_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   m_CheckBox->SetConstraints(c);

   m_RepeatText = new wxStaticText(this, -1, _("every"));
   c = new wxLayoutConstraints;
   c->left.RightOf(m_CalCtrl, 2*LAYOUT_X_MARGIN);
   c->top.Below(m_CheckBox, LAYOUT_Y_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   m_RepeatText->SetConstraints(c);

   m_TextCtrl = new wxTextCtrl(this,-1);
   c = new wxLayoutConstraints;
   c->left.RightOf(m_RepeatText, LAYOUT_X_MARGIN);
   c->top.Below(m_CheckBox, LAYOUT_Y_MARGIN);
   c->width.Absolute(30);
   c->height.AsIs();
   m_TextCtrl->SetConstraints(c);
   m_TextCtrl->SetValue("1");

   m_SpinButton = new wxSpinButton(this,-1);
   c = new wxLayoutConstraints;
   c->left.RightOf(m_TextCtrl, LAYOUT_X_MARGIN);
   c->top.Below(m_CheckBox, LAYOUT_Y_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   m_SpinButton->SetConstraints(c);


   static const wxString choices[] = {
      gettext_noop("days"), gettext_noop("weeks"),
      gettext_noop("months"), gettext_noop("years")
   };
   m_Choice = new wxChoice(this,-1, wxDefaultPosition, wxDefaultSize,
                           4,choices);
   c = new wxLayoutConstraints;
   c->left.RightOf(m_SpinButton, LAYOUT_X_MARGIN);
   c->top.Below(m_CheckBox, LAYOUT_Y_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   m_Choice->SetConstraints(c);


   m_SpinButton->SetValue(1);

   m_EndsOn = new wxCheckBox(this, -1, _("Repeat ends on"));
   c = new wxLayoutConstraints;
   c->left.RightOf(m_CalCtrl, 2*LAYOUT_X_MARGIN);
   c->top.Below(m_Choice, LAYOUT_Y_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   m_EndsOn->SetConstraints(c);
   m_EndsOn->SetValue(TRUE);

   m_CalCtrlEnd = new wxCalendarCtrl(this,-1, dt,
                                     wxDefaultPosition,
                                     wxDefaultSize,
                                     wxCAL_SHOW_HOLIDAYS|wxCAL_BORDER_SQUARE);
   c = new wxLayoutConstraints;
   c->left.RightOf(m_CalCtrl, 2*LAYOUT_X_MARGIN);
   c->top.Below(m_EndsOn, LAYOUT_Y_MARGIN);
   c->width.AsIs();
   c->height.AsIs();

   m_CalCtrlEnd->SetConstraints(c);

   SetAutoLayout(TRUE);
   SetDefaultSize(420, 320, TRUE /* minimal */);
   m_Date = dt;
   TransferDataToWindow();

   UpdateUI();
}

extern
bool PickDateDialog(wxDateTimeWithRepeat &dt,
                    wxWindow *parent = NULL)
{
   wxDateDialog dlg(dt, parent);
   bool rc = dlg.ShowModal() == wxID_OK;
   if(rc)
      dt = dlg.GetDate();
   return rc;
}

// This class tells the module when it disappears
class CalendarFrame : public wxMFrame
{
public:
   CalendarFrame(CalendarModule *module, wxWindow *parent);
   ~CalendarFrame();
   bool Show(bool show);
   /// switch frame display on/off
   void Toggle();

   void AddReminder(void);
   void AddReminder(const wxString &text,
                    ActionEnum action = CAL_ACTION_REMIND,
                    const wxDateTimeWithRepeat &when = wxDefaultDateTime);
   bool ScheduleMessage(SendMessage *msg);
   bool ScheduleMessage(Message *msg);

   /** checks if anything needs to be done:
       @param mf if non-NULL, react to change in this folder if is ours
   */
   void CheckUpdate(MailFolder *mf = NULL);
   /// re-reads config
   void GetConfig(void);

protected:

   String MakeDateLine(const wxDateTimeWithRepeat &dt);
   wxDateTimeWithRepeat ParseDateLine(const wxString &line);

   /// build the alarms list
   void ParseFolder(void);
   void ClearAlarms(void);
   void DeleteOrRewrite(MailFolder *mf,
                        Message *msg,
                        const wxDateTimeWithRepeat &dt,
                        ActionEnum action);


private:
   // implement base class pure virtual methods
   virtual void DoCreateToolBar() { }
   virtual void DoCreateStatusBar() { CreateStatusBar(); }

   MInterface * m_MInterface;
   /// profile settings
   Profile *m_Profile;
   String m_FolderName;
   String m_MyEmail;
   String m_DateFormat;
   String m_NewMailFolder;
   ASMailFolder *m_Folder;
   AlarmList m_Alarms;
   bool m_Show;
   CalendarModule *m_Module;
   wxCalendarCtrl *m_CalCtrl;
   wxListCtrl     *m_ListCtrl;

   DECLARE_NO_COPY_CLASS(CalendarFrame)
};

// ----------------------------------------------------------------------------
// drop target for the calendar frame
// ----------------------------------------------------------------------------

#if wxUSE_DRAG_AND_DROP

class MMessagesCalDropTarget : public MMessagesDropTargetBase
{
public:
   MMessagesCalDropTarget(CalendarFrame *frame)
      : MMessagesDropTargetBase(frame) { m_Frame = frame; }

   // overridden base class pure virtual
   virtual wxDragResult OnMsgDrop(wxCoord /* x */, wxCoord /* y */,
                                  MMessagesDataObject *data,
                                  wxDragResult def)
   {
      MailFolder *mf = data->GetFolder();
      if(mf)
      {
         UIdArray msgsToDelete;
         for(size_t i = 0; i < data->GetMessageCount(); i++)
         {
            Message *msg = mf->GetMessage(data->GetMessageUId(i));
            if(msg)
            {
               if(m_Frame->ScheduleMessage(msg))
                  msgsToDelete.Add(data->GetMessageUId(i));
               msg->DecRef();
            }
         }
         mf->DeleteOrTrashMessages(&msgsToDelete);
         mf->DecRef();
      }
      return def;
   }
private:
   CalendarFrame *m_Frame;

   DECLARE_NO_COPY_CLASS(MMessagesCalDropTarget)
};

#endif // wxUSE_DRAG_AND_DROP

class CalEventReceiver : public MEventReceiver
{
public:
   CalEventReceiver(class CalendarModule *calmod)
      {
         m_CalMod = calmod;
         m_regASFResult = MEventManager::Register(*this, MEventId_ASFolderResult);
         ASSERT_MSG( m_regASFResult, _T("can't reg calendar with event manager"));
         m_regCookieFolderUpdate = MEventManager::Register(*this, MEventId_FolderUpdate);
         ASSERT_MSG( m_regCookieFolderUpdate, _T("can't reg calendar with event manager"));
      }
   /// event processing function
   virtual bool OnMEvent(MEventData& ev)
   {
      if ( ev.GetId() == MEventId_FolderTreeChange )
      {
         if ( ev.GetId() == MEventId_ASFolderResult )
            m_CalMod->OnASFolderResultEvent((MEventASFolderResultData &) ev );
         else if ( ev.GetId() == MEventId_FolderUpdate )
            m_CalMod->OnFolderUpdateEvent((MEventFolderUpdateData&)ev );
         return true; // continue evaluating this event
      }
      return true;
   }
   ~CalEventReceiver()
      {
            MEventManager::Deregister(m_regASFResult);
            MEventManager::Deregister(m_regCookieFolderUpdate);
      }
private:
   class CalendarModule *m_CalMod;
   void *m_regASFResult, *m_regCookieFolderUpdate;
};



///------------------------------
/// Calendar frame class:
///------------------------------
CalendarFrame::CalendarFrame(CalendarModule *module, wxWindow *parent)
   : wxMFrame(_("Mahogany : Calendar"), parent )
{
   m_Module = module;
   m_MInterface = module->GetMInterface();
   m_Profile = m_MInterface->CreateModuleProfile(MODULE_NAME);
   m_Folder = NULL;

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

#if wxUSE_DRAG_AND_DROP
   new MMessagesCalDropTarget(this);
#endif // wxUSE_DRAG_AND_DROP

   GetConfig();
   if ( m_Show )
      ShowInInitialState();
}


CalendarFrame::~CalendarFrame()
{
   m_Module->TellDeleteFrame();
   if(m_Folder) m_Folder->DecRef();
   m_Profile->DecRef();
   ClearAlarms();
}

bool
CalendarFrame::Show(bool show)
{
   // don't do anything when the app is shutting down, if we have somethign to
   // do it should be done in OnClose() (TODO)
   if ( m_MInterface->GetMApplication()->IsRunning() )
   {
      if(m_Show == FALSE)
         CheckUpdate();
      m_Show = show;
      m_Module->m_CalendarMenu->Check(WXMENU_MODULES_CALENDAR_SHOW, m_Show);
   }

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
   m_FolderName = READ_CONFIG_MOD(m_Profile, MP_MOD_CALENDAR_BOX);
   m_Show = READ_CONFIG_MOD(m_Profile, MP_MOD_CALENDAR_SHOWONSTARTUP) != 0;

   // settings read from folder profile:
   Profile *fp = m_MInterface->CreateModuleProfile(m_FolderName);
   m_MyEmail = READ_CONFIG_TEXT(fp, MP_FROM_ADDRESS);

   {
#ifdef OS_WIN
      // MP_DATE_FMT contains '%' which are being (mis)interpreted as
      // env var expansion characters under Windows
      ProfileEnvVarSave noEnvVars(fp);
#endif // OS_WIN

      m_DateFormat = READ_CONFIG_TEXT(fp, MP_DATE_FMT);
   }

   m_NewMailFolder = READ_CONFIG_TEXT(fp, MP_NEWMAIL_FOLDER);
   fp->DecRef();

   // updates:
   if(m_FolderName != oldFolderName)
   {
      if(m_Folder)
         m_Folder->DecRef();

      // check if folder profile entry exists:
      MFolder *folder = m_MInterface->GetMFolder(m_FolderName);
      if ( !folder )
      {
         folder = MFolder::Create(m_FolderName, MF_FILE);
         if ( folder )
         {
            folder->SetFlags(MF_FLAGS_DEFAULT | MF_FLAGS_HIDDEN);
            folder->SetPath(m_FolderName);
            folder->SetComment(_("This folder is used to store data for "
                                 "the calendar plugin module."));
         }
         else
         {
            wxString msg;
            msg.Printf(_("Cannot create calendar module folder '%s'."),
                       m_FolderName.c_str());
            m_Module->ErrorMessage(msg);
         }
      }

      if ( folder )
         folder->DecRef();

      m_Folder = m_MInterface->OpenASMailFolder(m_FolderName);
      ParseFolder();
   }
}

String
CalendarFrame::MakeDateLine(const wxDateTimeWithRepeat &dt)
{
   String line;
   line.Printf("%ld %ld %ld %ld %ld %ld %ld %ld %ld",
               (long int) dt.GetYear(),
               (long int) dt.GetMonth(),
               (long int) dt.GetDay(),
               (long int) dt.GetYearRepeat(),
               (long int) dt.GetMonthRepeat(),
               (long int) dt.GetDayRepeat(),
               (long int) dt.GetYearRepeatEnd(),
               (long int) dt.GetMonthRepeatEnd(),
               (long int) dt.GetDayRepeatEnd());
   return line;
}

wxDateTimeWithRepeat
CalendarFrame::ParseDateLine(const wxString &line)
{
   wxDateTimeWithRepeat dt;
   long year, month, day;
   long yr, mr, dr, yre, mre, dre;
   if(sscanf(line.c_str(),"%ld %ld %ld %ld %ld %ld %ld %ld %ld",
             &year, &month, &day,
             &yr, &mr, &dr,
             &yre, &mre, &dre) != 9)
   {
      m_Module->ErrorMessage(_("Cannot parse date information."));

      dt = wxDateTime::Today();
   }
   else
   {
      dt.Set(day, (wxDateTime::Month)month, year);

      dt.SetYearRepeat(yr);
      dt.SetMonthRepeat(mr);
      dt.SetDayRepeat(dr);

      dt.SetEndDate(wxDateTime(dre, (wxDateTime::Month)mre, yre));
   }

   return dt;
}

bool
CalendarFrame::ScheduleMessage(SendMessage *msg)
{
   wxDateTimeWithRepeat dt = m_CalCtrl->GetDate();
   if(PickDateDialog(dt))
   {
      wxString str;
      msg->WriteToString(str);
      AddReminder(str,CAL_ACTION_SEND, dt);
      Show(TRUE); // make ourselves visible
      return TRUE;
   }
   else
      return FALSE;
}

bool
CalendarFrame::ScheduleMessage(Message *msg)
{
   wxDateTimeWithRepeat dt = m_CalCtrl->GetDate();
   if(PickDateDialog(dt))
   {
      wxString str;
      msg->WriteToString(str);
      AddReminder(str,CAL_ACTION_REMIND, dt);
      Show(TRUE); // make ourselves visible
      return TRUE;
   }
   else
      return FALSE;
}

void
CalendarFrame::AddReminder(void)
{
   wxString text =
      wxGetTextFromUser(_("Please enter the text for the reminder:"),
                        _("Mahogany : Create Reminder"),
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
   CHECK_RET( m_Folder, _T("no calendar folder in the calendar module") );

   MailFolder *mf = m_Folder->GetMailFolder();
   if(! mf)
   {
      wxLogError(_("Cannot open calendar folder"));
      return;
   }

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
               wxDateTimeWithRepeat dt = ParseDateLine(tmp);
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
   for(size_t count = 0; count < m_Alarms.Count(); count++)
   {
      m_ListCtrl->InsertItem(count, m_Alarms[count]->GetDate().Format(m_DateFormat));
      m_ListCtrl->SetItem(count,1, m_Alarms[count]->GetSubject());
      m_ListCtrl->SetItem(count,2,_("reminder"));
   }
   mf->DecRef();
}

/* Depending on date and repeat settings, this either removes an
   expired entry or changes its date to the next repeat. */
void
CalendarFrame::DeleteOrRewrite(MailFolder *mf,
                               Message *msg,
                               const wxDateTimeWithRepeat &dt,
                               ActionEnum action)
{
   if(dt.IsRepeating() && dt.GetEndDate() > wxDateTime::Now())
   {
      /* We need to change the first date and re-store
         it in folder. */
      wxDateTimeWithRepeat newdt = dt;

      // we now calculate the new start date:
      wxDateTime::Tm tm = dt.GetTm();

      if(dt.GetDayRepeat())
         tm.AddDays(dt.GetDayRepeat());
      else if(dt.GetMonthRepeat())
         tm.AddMonths(dt.GetMonthRepeat());
      else if(dt.GetYearRepeat())
         tm.AddMonths(12*dt.GetYearRepeat());

      wxDateTime ndt = wxDateTime(tm);
      newdt.SetDay(ndt.GetDay());
      newdt.SetMonth(ndt.GetMonth());
      newdt.SetYear(ndt.GetYear());

      wxString text;
      msg->WriteToString(text, FALSE);
      AddReminder(text,action, newdt);
   }
   mf->DeleteMessage(msg->GetUId());
}

void
CalendarFrame::CheckUpdate(MailFolder *eventFolder)
{
   CHECK_RET( m_Folder, _T("no calendar folder in the calendar module") );

   MailFolder *mf = m_Folder->GetMailFolder();
   if ( !mf )
   {
      wxLogError(_("Cannot open calendar folder"));
      return;
   }

   // we react to an event which isn't ours, abort
   if(eventFolder != NULL && eventFolder != mf)
   {
      mf->DecRef();
      return;
   }
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

                  DeleteOrRewrite(mf, msg,
                                  m_Alarms[count]->GetDate(), action);
                  deleted = TRUE;
               }
               if(nmmf) nmmf->DecRef();
            }
            else if(action == CAL_ACTION_SEND)
            {
               SendMessage_obj sendMsg(SendMessage::CreateFromMsg
                                       (
                                        mf->GetProfile(),
                                        msg
                                       ));

               if ( sendMsg && sendMsg->SendOrQueue() )
               {
                  wxString txt;
                  txt.Printf(_("Sent or queued message `%s'."),
                             m_Alarms[count]->GetSubject().c_str());
                  GetStatusBar()->SetStatusText(txt);
                  DeleteOrRewrite(mf, msg,
                                  m_Alarms[count]->GetDate(), action);
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
                           const wxDateTimeWithRepeat &when)
{
   String fmt, text;
   wxString timeStr = wxDateTime::Now().Format("%d %b %Y %H:%M:%S");


   String tmp = strutil_enforceCRLF(itext);
   if(action == CAL_ACTION_SEND)
   {
      timeStr = when.Format("%d %b %Y %H:%M:%S");
      // we need to prepend the required header lines to the message
      // text:
      text = "";
      text << "X-M-CAL-CMD: SEND" << "\015\012"
          << "X-M-CAL-DATE: "
          << MakeDateLine(when)
          << "\015\012"
          << tmp;
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
          << tmp
          << "\015\012";
      text.Printf(fmt, timeStr.c_str(), MakeDateLine(when).c_str());
   }
   class Message * msg = m_MInterface->CreateMessage(text,UID_ILLEGAL,m_Profile);
   (void) m_Folder->AppendMessage(msg);
//causes recursion   CheckUpdate();
   msg->DecRef();
}

MMODULE_BEGIN_IMPLEMENT(CalendarModule,
                        "Calendar",
                        "Calendar",
                        "Calendar module",
                        "1.00")
   MMODULE_PROP("description",
                _("This module provides a calendar and scheduling plugin."))
   MMODULE_PROP("author", "Karsten Ballüder <karsten@phy.hw.ac.uk>")
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
      if(errorCode)
         *errorCode = MMODULE_ERR_INCOMPATIBLE_VERSIONS;
      return NULL;
   }

   return new CalendarModule(minterface);
}

CalendarModule::CalendarModule(MInterface *minterface)
{
   SetMInterface(minterface);
   m_Frame = NULL;
   m_Timer = NULL;
   m_EventReceiver = NULL;
   m_CalendarMenu = NULL;
}

CalendarModule::~CalendarModule()
{
   delete m_Timer;
   delete m_EventReceiver;
   if(m_Frame)
      m_Frame->Close();
}

bool
CalendarModule::RegisterWithMainFrame()
{
   m_CalendarMenu = new wxMenu("", wxMENU_TEAROFF);
   m_CalendarMenu->Append(WXMENU_MODULES_CALENDAR_SHOW, _("&Show"), "", TRUE);
   m_CalendarMenu->AppendSeparator();
   m_CalendarMenu->Append(WXMENU_MODULES_CALENDAR_CONFIG, _("&Configure"));

   MAppBase *mapp = m_MInterface->GetMApplication();

   wxMFrame *mframe = mapp->TopLevelFrame();
   CHECK( mframe, false, _T("can't init calendar module - no main window") );

   ((wxMainFrame *)mframe)->AddModulesMenu
                            (
                             _("&Calendar Module"),
                             _("Functionality to schedule messages and reminders."),
                             m_CalendarMenu,
                             -1
                            );

   m_Today = wxDateTime::Today();

   m_Timer = new DayCheckTimer(this);
   m_EventReceiver = new CalEventReceiver(this);
   CreateFrame();
   m_Frame->CheckUpdate();

   return true;
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
      case MMOD_FUNC_INIT:
         return RegisterWithMainFrame() ? 0 : -1;

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

bool
CalendarModule::ScheduleMessage(class SendMessage *msg)
{
   CreateFrame();
   m_Frame->Raise();
   return m_Frame->ScheduleMessage(msg);
}

bool
CalendarModule::ScheduleMessage(class Message *msg)
{
   CreateFrame();
   m_Frame->Raise();
   return m_Frame->ScheduleMessage(msg);
}


void
CalendarModule::CreateFrame(void)
{
   if(m_Frame)
      return; // nothing to do

   MAppBase *mapp = m_MInterface->GetMApplication();
   m_Frame = new CalendarFrame(this,mapp->TopLevelFrame());
}

bool
CalendarModule::OnASFolderResultEvent(MEventASFolderResultData &)
{
   return TRUE;
}

bool
CalendarModule::OnFolderUpdateEvent(MEventFolderUpdateData &ev )
{
   if(m_Frame)
      m_Frame->CheckUpdate(ev.GetFolder());
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
wxOptionsPageDesc(
   gettext_noop("Calendar module preferences"),
   "calendar",// image
   MH_MODULES_CALENDAR_CONFIG,
   // the fields description
   gs_FieldInfos,
   gs_ConfigValues,
   WXSIZEOF(gs_FieldInfos)
);

void
CalendarModule::Configure(void)
{
   Profile *p = m_MInterface->CreateModuleProfile(MODULE_NAME);
   ShowCustomOptionsDialog(gs_OptionsPageDesc, p, NULL);
   p->DecRef();
}
