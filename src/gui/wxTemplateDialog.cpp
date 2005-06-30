///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   TemplateDialog.cpp - the template configuration dialog
// Purpose:     these dialogs are used mainly by the options dialog, but may be
//              also used from elsewhere
// Author:      Vadim Zeitlin
// Modified by: VZ at 09.05.00 to allow editing all templates
// Created:     16.07.99
// CVS-ID:      $Id$
// Copyright:   (c) 1999 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
#  include "Mcommon.h"
#  include "strutil.h"
#  include "guidef.h"
#  include "Profile.h"

#  include <wx/layout.h>
#  include <wx/statbox.h>
#  include <wx/stattext.h>
#  include <wx/menu.h>
#  include <wx/filedlg.h>
#endif // USE_PCH

#include "gui/wxDialogLayout.h"

#include "TemplateDialog.h"

#include <wx/confbase.h>

// ----------------------------------------------------------------------------
// persistent msgboxes we use here
// ----------------------------------------------------------------------------

extern const MPersMsgBox *M_MSGBOX_DELETE_TEMPLATE;
extern const MPersMsgBox *M_MSGBOX_SAVE_TEMPLATE;

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

static const wxChar *gs_templateNames[MessageTemplate_Max] =
{
   gettext_noop("New message"),
   gettext_noop("New article"),
   gettext_noop("Reply"),
   gettext_noop("Follow-up"),
   gettext_noop("Forward")
};

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// the text window for editing templates: it has a popup menu which allows the
// user to insert any of existing macros
class TemplateEditor : public wxTextCtrl
{
public:
   // ctor
   TemplateEditor(const TemplatePopupMenuItem& menu, wxWindow *parent)
      : wxTextCtrl(parent, -1, _T(""), wxDefaultPosition, wxDefaultSize,
                   wxTE_MULTILINE),
        m_menuInfo(menu)
   {
      m_menu = NULL;
   }

   virtual ~TemplateEditor() { if ( m_menu ) delete m_menu; }

   // callbacks
   void OnRClick(wxMouseEvent& event); // show the popup menu
   void OnMenu(wxCommandEvent& event); // insert the selection into text

private:
   // creates the popup menu if it doesn't exist yet
   void CreatePopupMenu();

   // CreatePopupMenu() helper
   void AppendMenuItem(wxMenu *menu, const TemplatePopupMenuItem& menuitem);

   // the popup menu description
   const TemplatePopupMenuItem& m_menuInfo;
   WX_DEFINE_ARRAY(const TemplatePopupMenuItem *, ArrayPopupMenuItems);
   ArrayPopupMenuItems m_items;

   // the popup menu itself
   wxMenu *m_menu;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(TemplateEditor)
};

// the dialog for editing the templates for the given folder (profile): it
// contains a listbox for choosing the template and the text control to edit
// the selected template
class wxFolderTemplatesDialog : public wxOptionsPageSubdialog
{
public:
   wxFolderTemplatesDialog(const TemplatePopupMenuItem& menu,
                           Profile *profile,
                           wxWindow *parent);

   // did the user really change anything?
   bool WasChanged() const { return m_wasChanged; }

   // called by wxWindows when [Ok] button was pressed
   virtual bool TransferDataFromWindow();

   // callbacks
   void OnListboxSelection(wxCommandEvent& event);

private:
   // saves changes to current template
   void SaveChanges();

   // updates the contents of the text control
   void UpdateText();

   Profile        *m_profile;
   MessageTemplateKind m_kind;         // of template being edited
   wxTextCtrl         *m_textctrl;
   bool                m_wasChanged;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxFolderTemplatesDialog)
};

// common base for wxAllTemplatesDialog and wxChooseTemplateDialog
class wxTemplatesDialogBase : public wxManuallyLaidOutDialog
{
public:
   wxTemplatesDialogBase(MessageTemplateKind kind,
                         const TemplatePopupMenuItem& menu,
                         wxWindow *parent);

   // get the value of the template chosen
   wxString GetTemplateValue() const;

   // get the last template kind the user chose
   MessageTemplateKind GetTemplateKind() const { return m_kind; }

   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

   // callbacks
   void OnListboxSelection(wxCommandEvent& event);
   void OnCancel(wxCommandEvent& event);

protected:
   // helper function to get the correct title for the dialog
   virtual wxString GetTemplateTitle(MessageTemplateKind kind) const = 0;

   // set the template kind and update the dialog title to match it
   void SetTemplateKind(MessageTemplateKind kind);

   // fill the listbox with the templates of the given m_kind
   void FillListBox();

   // ask the user if he wants to save changes to the currently selected
   // template, if it was changed
   void CheckForChanges();

   // save the changes made to the template being edited
   void SaveChanges();

   // show the currently chosen template in the text control
   void UpdateText();

   // the kind of the templates we edit (compose/reply/...)
   MessageTemplateKind m_kind;

   // the name of the template being edited, empty if none
   wxString m_name;

   // controls
   wxPListBox *m_listbox;
   wxButton   *m_btnAdd,
              *m_btnDelete;
   wxTextCtrl *m_textctrl;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxTemplatesDialogBase)
};

// the dialog for choosing template among all those of given kind
class wxChooseTemplateDialog : public wxTemplatesDialogBase
{
public:
   wxChooseTemplateDialog(MessageTemplateKind kind,
                          const TemplatePopupMenuItem& menu,
                          wxWindow *parent);

protected:
   // event handlers
   void OnListboxSelection(wxCommandEvent& event);
   void OnListboxDoubleClick(wxCommandEvent& event);

   void DoUpdateUI()
   {
      // only enable the ok button if there is a valid selection
      FindWindow(wxID_OK)->Enable(m_listbox->GetSelection() != -1);
   }

   virtual wxString GetTemplateTitle(MessageTemplateKind kind) const;

private:
   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxChooseTemplateDialog)
};

// the dialog for editing/adding/deleting templates
class wxAllTemplatesDialog : public wxTemplatesDialogBase
{
public:
   wxAllTemplatesDialog(const TemplatePopupMenuItem& menu, wxWindow *parent);

   virtual bool TransferDataFromWindow();

protected:
   // event handlers
   void OnComboBoxChange(wxCommandEvent& event);
   void OnAddTemplate(wxCommandEvent& event);
   void OnDeleteTemplate(wxCommandEvent& event);
   void OnUpdateUIDelete(wxUpdateUIEvent& event);


   // helpers
   static const wxChar *GetEditKindPath()
   {
      return _T("/") M_SETTINGS_CONFIG_SECTION _T("/TemplateEditKind");
   }

   static MessageTemplateKind GetKindLastEdited();

   virtual wxString GetTemplateTitle(MessageTemplateKind kind) const;

private:
   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxAllTemplatesDialog)
};

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

enum
{
   Button_Template_Add = 100,
   Button_Template_Delete
};

BEGIN_EVENT_TABLE(TemplateEditor, wxTextCtrl)
   EVT_MENU(-1, TemplateEditor::OnMenu)
   EVT_RIGHT_DOWN(TemplateEditor::OnRClick)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxFolderTemplatesDialog, wxOptionsPageSubdialog)
   EVT_LISTBOX(-1, wxFolderTemplatesDialog::OnListboxSelection)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxTemplatesDialogBase, wxManuallyLaidOutDialog)
   EVT_LISTBOX(-1, wxTemplatesDialogBase::OnListboxSelection)

   EVT_BUTTON(wxID_CANCEL, wxTemplatesDialogBase::OnCancel)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxChooseTemplateDialog, wxTemplatesDialogBase)
   EVT_LISTBOX(-1, wxChooseTemplateDialog::OnListboxSelection)
   EVT_LISTBOX_DCLICK(-1, wxChooseTemplateDialog::OnListboxDoubleClick)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxAllTemplatesDialog, wxTemplatesDialogBase)
   EVT_COMBOBOX(-1, wxAllTemplatesDialog::OnComboBoxChange)

   EVT_BUTTON(Button_Template_Add,    wxAllTemplatesDialog::OnAddTemplate)
   EVT_BUTTON(Button_Template_Delete, wxAllTemplatesDialog::OnDeleteTemplate)

   EVT_UPDATE_UI(Button_Template_Delete, wxAllTemplatesDialog::OnUpdateUIDelete)
END_EVENT_TABLE()

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// TemplateEditor
// ----------------------------------------------------------------------------

void TemplateEditor::OnRClick(wxMouseEvent& event)
{
   if ( !IsEnabled() )
   {
      // don't allow inserting the text into a disabled control
      return;
   }

   // create the menu if it hadn't been created yet
   CreatePopupMenu();

   // show it
   if ( m_menu )
   {
      (void)PopupMenu(m_menu, event.GetPosition());
   }
}

void TemplateEditor::OnMenu(wxCommandEvent& event)
{
   size_t id = (size_t)event.GetId();
   CHECK_RET( id < m_items.GetCount(), _T("unexpected menu event") );

   const TemplatePopupMenuItem *menuitem = m_items[id];
   CHECK_RET( menuitem, _T("no menu item") );

   String value;
   switch ( menuitem->type )
   {
         // put it here because in the last case clause we have some variables
         // which would be otherwise have to be taken inside another block...
      default:
         FAIL_MSG(_T("unexpected menu item type"));
         return;

      case TemplatePopupMenuItem::Normal:
         // nothing special to do - just insert the corresponding text
         value = menuitem->format;
         break;

      case TemplatePopupMenuItem::File:
         // choose the file (can't be more specific in messages because we
         // don't really know what is it for...)
         value = wxPFileSelector(_T("TemplateFile"),
                                 _("Please choose a file"),
                                 NULL, NULL, NULL, NULL,
                                 wxOPEN | wxFILE_MUST_EXIST,
                                 this);
         if ( !value )
         {
            // user cancelled
            return;
         }

         // fall through

      case TemplatePopupMenuItem::Text:
         if ( !value )
         {
            // get some text from user (FIXME the prompts are really stupid)
            if ( !MInputBox(&value, _("Value for template variable"),
                            _T("Value"), this, _T("TemplateText")) )
            {
               // user cancelled
               return;
            }
         }
         //else: it was, in fact, a filename and it was already selected

         // quote the value if needed - that is if it contains anything but
         // alphanumeric characters
         bool needsQuotes = false;
         for ( const wxChar *pc = value.c_str(); *pc; pc++ )
         {
            if ( !wxIsalnum(*pc) )
            {
               needsQuotes = true;

               break;
            }
         }

         if ( needsQuotes )
         {
            String value2(_T('"'));
            for ( const wxChar *pc = value.c_str(); *pc; pc++ )
            {
               if ( *pc == '"' || *pc == '\\' )
               {
                  value2 += '\\';
               }

               value2 += *pc;
            }

            // closing quote
            value2 += '"';

            value = value2;
         }

         // check that the format string contains exactly what it must
         ASSERT_MSG( strutil_extract_formatspec(menuitem->format) == _T("s"),
                     _T("incorrect format string") );

         value = String::Format(menuitem->format, value.c_str());
         break;
   }

   WriteText(value);
}

void TemplateEditor::AppendMenuItem(wxMenu *menu,
                                    const TemplatePopupMenuItem& menuitem)
{
   switch ( menuitem.type )
   {
      case TemplatePopupMenuItem::Submenu:
         {
            // first create the entry for the submenu
            wxMenu *submenu = new wxMenu;
            menu->Append(m_items.GetCount(), wxGetTranslation(menuitem.label), submenu);

            // next subitems
            for ( size_t n = 0; n < menuitem.nSubItems; n++ )
            {
               AppendMenuItem(submenu, menuitem.submenu[n]);
            }
         }
         break;

      case TemplatePopupMenuItem::None:
         // as we don't use an id, we should change m_items.GetCount(), so
         // return immediately instead of just "break"
         menu->AppendSeparator();
         return;

      case TemplatePopupMenuItem::Normal:
      case TemplatePopupMenuItem::File:
      case TemplatePopupMenuItem::Text:
         menu->Append(m_items.GetCount(), wxGetTranslation(menuitem.label));
         break;

      default:
         FAIL_MSG(_T("unknown popup menu item type"));
         return;
   }

   m_items.Add(&menuitem);
}

void TemplateEditor::CreatePopupMenu()
{
   if ( m_menu )
   {
      // menu already created
      return;
   }

   // the top level pseudo item must have type "Submenu" - otherwise we
   // wouldn't have any menu at all
   CHECK_RET( m_menuInfo.type == TemplatePopupMenuItem::Submenu, _T("no menu?") );

   m_menu = new wxMenu;
   m_menu->SetTitle(_("Please choose"));

   for ( size_t n = 0; n < m_menuInfo.nSubItems; n++ )
   {
      AppendMenuItem(m_menu, m_menuInfo.submenu[n]);
   }
}

// ----------------------------------------------------------------------------
// wxFolderTemplatesDialog
// ----------------------------------------------------------------------------

wxFolderTemplatesDialog::wxFolderTemplatesDialog(const TemplatePopupMenuItem& menu,
                                   Profile *profile,
                                   wxWindow *parent)
                : wxOptionsPageSubdialog(profile, parent,
                                         _("Configure message templates"),
                                         _T("ComposeTemplates"))
{
   // init members
   // ------------

   m_kind = MessageTemplate_Max;
   m_profile = profile;
   m_textctrl = NULL;

   // layout the controls
   // -------------------
   wxLayoutConstraints *c;

   // first the box around everything
   wxStaticBox *box = CreateStdButtonsAndBox(_T(""));

   // then a short help message
   wxStaticText *msg =
      new wxStaticText
      (
       this, -1,
       _("Select the template to edit in the list first. Then right click the\n"
         "mouse in the text control to get the list of all available macros.")
      );
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(box, wxTop, 2*LAYOUT_Y_MARGIN);
   c->height.AsIs();
   msg->SetConstraints(c);

   // we have to create the text control before the listbox as wxPListBox
   // restores the listbox selection which results in a call to our
   // OnListboxSelection() which uses m_textctrl
   m_textctrl = new TemplateEditor(menu, this);

   // on the left side is the listbox with all available templates
   wxListBox *listbox = new wxPListBox(_T("MsgTemplate"), this, -1);

   // this array should be in sync with MessageTemplateKind enum
   ASSERT_MSG( WXSIZEOF(gs_templateNames) == MessageTemplate_Max,
               _T("forgot to update the labels array?") );
   for ( size_t n = 0; n < WXSIZEOF(gs_templateNames); n++ )
   {
      listbox->Append(wxGetTranslation(gs_templateNames[n]));
   }

   c = new wxLayoutConstraints;
   c->top.Below(msg, LAYOUT_Y_MARGIN);
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->width.AsIs();
   c->height.Absolute(5*hBtn);
   listbox->SetConstraints(c);

   // to the right of it is the text control where template file can be edited
   c = new wxLayoutConstraints;
   c->top.SameAs(listbox, wxTop);
   c->height.SameAs(listbox, wxHeight);
   c->left.RightOf(listbox, LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   m_textctrl->SetConstraints(c);

   SetDefaultSize(6*wBtn, 11*hBtn);
}

void wxFolderTemplatesDialog::UpdateText()
{
   String templateValue = GetMessageTemplate(m_kind, m_profile);

   m_textctrl->SetValue(templateValue);
   m_textctrl->DiscardEdits();
}

void wxFolderTemplatesDialog::SaveChanges()
{
   m_wasChanged = true;

   // TODO: give the user the possibility to change the auto generated name
   wxString name;
   name << m_profile->GetName() << '_' << wxGetTranslation(gs_templateNames[m_kind]);
   name.Replace(_T("/"), _T("_")); // we want it flat
   SetMessageTemplate(name, m_textctrl->GetValue(), m_kind, m_profile);
}

void wxFolderTemplatesDialog::OnListboxSelection(wxCommandEvent& event)
{
   CHECK_RET( m_textctrl, _T("unexpected listbox selection event") );

   if ( m_textctrl->IsModified() )
   {
      // save it if the user doesn't veto it
      String msg;
      msg.Printf(_("You have modified the template for message of type "
                   "'%s', would you like to save it?"),
                 gs_templateNames[m_kind]);
      if ( MDialog_YesNoDialog(msg, this,
                               MDIALOG_YESNOTITLE,
                               M_DLG_YES_DEFAULT,
                               M_MSGBOX_SAVE_TEMPLATE) )
      {
         SaveChanges();
      }
   }

   // load the template for the selected type into the text ctrl
   m_kind = (MessageTemplateKind)event.GetInt();
   UpdateText();
}

bool wxFolderTemplatesDialog::TransferDataFromWindow()
{
   if ( m_textctrl->IsModified() )
   {
      // don't ask - if the user pressed ok, he does want to save changes,
      // otherwise he would have chosen cancel
      SaveChanges();
   }

   return true;
}

// ----------------------------------------------------------------------------
// wxTemplatesDialogBase
// ----------------------------------------------------------------------------

wxTemplatesDialogBase::wxTemplatesDialogBase(MessageTemplateKind kind,
                                             const TemplatePopupMenuItem& menu,
                                             wxWindow *parent)
                    : wxManuallyLaidOutDialog(parent, _T(""), _T("AllTemplates"))
{
   m_kind = kind;

   SetDefaultSize(6*wBtn, 10*hBtn);
}

bool wxTemplatesDialogBase::TransferDataToWindow()
{
   SetTemplateKind(m_kind);

   UpdateText();

   return true;
}

bool wxTemplatesDialogBase::TransferDataFromWindow()
{
   if ( m_textctrl->IsModified() )
   {
      // we were editing something, save the changes
      SaveChanges();
   }

   return true;
}

void wxTemplatesDialogBase::FillListBox()
{
   wxArrayString names = GetMessageTemplateNames(m_kind);
   size_t count = names.GetCount();
   for ( size_t n = 0; n < count; n++ )
   {
      m_listbox->Append(names[n]);
   }
}

wxString wxTemplatesDialogBase::GetTemplateValue() const
{
   wxString value;
   if ( !m_name.empty() )
   {
      value = GetMessageTemplate(m_kind, m_name);
   }

   return value;
}

void wxTemplatesDialogBase::SaveChanges()
{
   wxASSERT_MSG( !m_name.empty(), _T("shouldn't try to save") );

   SetMessageTemplate(m_name, m_textctrl->GetValue(), m_kind, NULL);
}

void wxTemplatesDialogBase::UpdateText()
{
   int sel = m_listbox->GetSelection();
   if ( sel == -1 )
   {
      m_textctrl->Clear();
      m_textctrl->Enable(false);

      m_name.clear();
   }
   else // we have selection
   {
      m_name = m_listbox->GetString(sel);
      String value = GetMessageTemplate(m_kind, m_name);

      m_textctrl->Enable(true);
      m_textctrl->SetValue(value);
   }

   // in any case, we changed it, not the user
   m_textctrl->DiscardEdits();
}

void wxTemplatesDialogBase::CheckForChanges()
{
   if ( m_textctrl->IsModified() )
   {
      // save it if the user doesn't veto it
      String msg;
      msg.Printf(_("You have modified the template '%s', "
                   "would you like to save it?"),
                 m_name.c_str());
      if ( MDialog_YesNoDialog(msg, this,
                               MDIALOG_YESNOTITLE,
                               M_DLG_YES_DEFAULT,
                               M_MSGBOX_SAVE_TEMPLATE) )
      {
         SaveChanges();
      }
   }
}

void wxTemplatesDialogBase::SetTemplateKind(MessageTemplateKind kind)
{
   m_kind = kind;

   SetTitle(GetTemplateTitle(m_kind));
}

void wxTemplatesDialogBase::OnListboxSelection(wxCommandEvent& /* event */)
{
   CheckForChanges();

   UpdateText();
}

void wxTemplatesDialogBase::OnCancel(wxCommandEvent& event)
{
   CheckForChanges();

   event.Skip();
}

// ----------------------------------------------------------------------------
// wxChooseTemplateDialog
// ----------------------------------------------------------------------------

wxChooseTemplateDialog::wxChooseTemplateDialog(MessageTemplateKind kind,
                                               const TemplatePopupMenuItem& menu,
                                               wxWindow *parent)
                      : wxTemplatesDialogBase(kind, menu, parent)
{
   // create the controls
   // -------------------

   // first the box around everything
   wxStaticBox *box = CreateStdButtonsAndBox(_("Choose the template to use:"));

   // on the left side there is a listbox with all available templates for kind
   m_listbox = new wxPListBox(_T("AllTemplates"), this, -1);

   // on the right of listbox there is a text control
   m_textctrl = new TemplateEditor(menu, this);

   // fill the listbox now to let it auto adjust the width before setting the
   // constraints
   FillListBox();


   // now lay them out
   // ----------------

   wxLayoutConstraints *c;
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(box, wxTop, 4*LAYOUT_Y_MARGIN);
   c->width.AsIs();
   c->bottom.SameAs(box, wxBottom, 2*LAYOUT_Y_MARGIN);
   m_listbox->SetConstraints(c);

   c = new wxLayoutConstraints;
   c->top.SameAs(m_listbox, wxTop);
   c->bottom.SameAs(m_listbox, wxBottom);
   c->left.RightOf(m_listbox, LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   m_textctrl->SetConstraints(c);

   // we need to restore selection right now, without waiting for the listbox
   // to do it itself later, as otherwise we'd disable the "Ok" button in
   // DoUpdateUI() initially and this means that the focus wouldn't be given to
   // it (focus never goes to disabled controls)
   m_listbox->RestoreSelection();
   DoUpdateUI();
}

void wxChooseTemplateDialog::OnListboxSelection(wxCommandEvent& event)
{
   DoUpdateUI();

   event.Skip();
}

void wxChooseTemplateDialog::OnListboxDoubleClick(wxCommandEvent& /* event */)
{
   EndModal(wxID_OK);
}

wxString
wxChooseTemplateDialog::GetTemplateTitle(MessageTemplateKind kind) const
{
   wxString title, what;
   switch ( kind )
   {
      case MessageTemplate_NewMessage:
         what = _("the new message");
         break;

      case MessageTemplate_NewArticle:
         what = _("the new article");
         break;

      case MessageTemplate_Reply:
         what = _("the reply");
         break;

      case MessageTemplate_Followup:
         what = _("the follow up");
         break;

      case MessageTemplate_Forward:
         what = _("the forwarded message");
         break;

      default:
         FAIL_MSG(_T("unknown template kind"));
   }

   title.Printf(_("Please choose template for %s"), what.c_str());

   return title;
}

// ----------------------------------------------------------------------------
// wxAllTemplatesDialog
// ----------------------------------------------------------------------------

wxAllTemplatesDialog::wxAllTemplatesDialog(const TemplatePopupMenuItem& menu,
                                           wxWindow *parent)
                    : wxTemplatesDialogBase(GetKindLastEdited(), menu, parent)
{
   wxLayoutConstraints *c;

   // first the box around everything
   wxStaticBox *box = CreateStdButtonsAndBox(_("All available templates"));

   wxStaticText *msg =
      new wxStaticText
      (
       this, -1,
       _("Select the template to edit in the list first. Then right click the\n"
         "mouse in the text control to get the list of all available macros.")
      );
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(box, wxTop, 4*LAYOUT_Y_MARGIN);
   c->height.AsIs();
   msg->SetConstraints(c);

   // on the left side there is a combo allowing to choose the template type
   // and a listbox with all available templates for this type
   m_listbox = new wxPListBox(_T("AllTemplates"), this, -1);

   // this array should be in sync with MessageTemplateKind enum
   ASSERT_MSG( WXSIZEOF(gs_templateNames) == MessageTemplate_Max,
               _T("forgot to update the labels array?") );
   wxString choices[MessageTemplate_Max];
   for ( size_t n = 0; n < WXSIZEOF(gs_templateNames); n++ )
   {
      choices[n] = wxGetTranslation(gs_templateNames[n]);
   }

   wxComboBox *combo = new wxComboBox(this, -1,
                                      gs_templateNames[m_kind],
                                      wxDefaultPosition,
                                      wxDefaultSize,
                                      WXSIZEOF(gs_templateNames), choices,
                                      wxCB_READONLY);

   c = new wxLayoutConstraints;
   c->top.Below(msg, 2*LAYOUT_Y_MARGIN);
   c->left.SameAs(msg, wxLeft, 2*LAYOUT_X_MARGIN);
   c->width.SameAs(m_listbox, wxWidth);
   c->height.AsIs();
   combo->SetConstraints(c);

   // between the listbox and the buttons there is the text control where
   // template file can be edited
   m_textctrl = new TemplateEditor(menu, this);

   // fill the listbox now to let it auto adjust the width before setting the
   // constraints
   FillListBox();

   c = new wxLayoutConstraints;
   c->top.Below(combo, 2*LAYOUT_Y_MARGIN);
   c->left.SameAs(msg, wxLeft, 2*LAYOUT_X_MARGIN);
   c->width.AsIs();
   c->bottom.SameAs(box, wxBottom, 2*LAYOUT_Y_MARGIN);
   m_listbox->SetConstraints(c);

   // put 2 buttons to add/delete templates along the right edge
   m_btnAdd = new wxButton(this, Button_Template_Add, _("&Add..."));
   c = new wxLayoutConstraints;
   c->width.Absolute(wBtn);
   c->height.Absolute(hBtn);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->bottom.SameAs(m_listbox, wxCentreY, LAYOUT_Y_MARGIN);
   m_btnAdd->SetConstraints(c);

   m_btnDelete = new wxButton(this, Button_Template_Delete, _("&Delete"));
   c = new wxLayoutConstraints;
   c->width.Absolute(wBtn);
   c->height.Absolute(hBtn);
   c->top.SameAs(m_listbox, wxCentreY, LAYOUT_Y_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   m_btnDelete->SetConstraints(c);

   c = new wxLayoutConstraints;
   c->top.SameAs(combo, wxTop);
   c->bottom.SameAs(m_listbox, wxBottom);
   c->left.RightOf(m_listbox, LAYOUT_X_MARGIN);
   c->right.LeftOf(m_btnAdd, 2*LAYOUT_X_MARGIN);
   m_textctrl->SetConstraints(c);
}

/* static */
MessageTemplateKind wxAllTemplatesDialog::GetKindLastEdited()
{
   wxConfigBase *config = wxConfigBase::Get();

   long kindLastEdited = config ? config->Read(GetEditKindPath(),
                                                   (long)MessageTemplate_Reply)
                                : MessageTemplate_Reply;
   if ( kindLastEdited < 0 || kindLastEdited >= MessageTemplate_Max )
   {
      wxLogDebug(_T("Corrupted TemplateEditKind entry in config."));

      kindLastEdited = MessageTemplate_Reply;
   }

   return (MessageTemplateKind)kindLastEdited;
}

bool wxAllTemplatesDialog::TransferDataFromWindow()
{
   if ( !wxTemplatesDialogBase::TransferDataFromWindow() )
      return false;

   // remember the last edited template category
   wxConfigBase *config = wxConfigBase::Get();
   if ( config )
   {
      config->Write(GetEditKindPath(), GetTemplateKind());
   }

   return true;
}

wxString wxAllTemplatesDialog::GetTemplateTitle(MessageTemplateKind kind) const
{
   wxString title, what;
   switch ( kind )
   {
      case MessageTemplate_NewMessage:
         what = _("composing new messages");
         break;

      case MessageTemplate_NewArticle:
         what = _("composing newsgroup articles");
         break;

      case MessageTemplate_Reply:
         what = _("replying");
         break;

      case MessageTemplate_Followup:
         what = _("writing follow ups");
         break;

      case MessageTemplate_Forward:
         what = _("forwarding");
         break;

      default:
         FAIL_MSG(_T("unknown template kind"));
   }

   title.Printf(_("Configure templates for %s"), what.c_str());

   return title;
}

void wxAllTemplatesDialog::OnComboBoxChange(wxCommandEvent& event)
{
   CheckForChanges();

   SetTemplateKind((MessageTemplateKind)event.GetInt());

   m_listbox->Clear();
   FillListBox();
   UpdateText();
}

void wxAllTemplatesDialog::OnAddTemplate(wxCommandEvent& /* event */)
{
   // get the name for the new template
   wxString name;
   if ( !MInputBox(
                   &name,
                   _("Create new template"),
                   _("Name for the new template:"),
                   this,
                   _T("AddTemplate")
                  ) )
   {
      // cancelled
      return;
   }

   // append the new string to the listbox and select it
   m_name = name;
   m_listbox->Append(name);
   m_listbox->SetSelection(m_listbox->GetCount() - 1);
   m_textctrl->Clear();
   m_textctrl->DiscardEdits();
}

void wxAllTemplatesDialog::OnDeleteTemplate(wxCommandEvent& /* event */)
{
   wxASSERT_MSG( !!m_name, _T("shouldn't try to delete") );

   String msg;
   msg.Printf(_("Do you really want to delete the template '%s'?"),
              m_name.c_str());
   if ( MDialog_YesNoDialog(msg, this,
                            MDIALOG_YESNOTITLE,
                            M_DLG_NO_DEFAULT,
                            M_MSGBOX_DELETE_TEMPLATE) )
   {
      m_listbox->Delete(m_listbox->GetSelection());

      DeleteMessageTemplate(m_kind, m_name);

      UpdateText();
   }
}

void wxAllTemplatesDialog::OnUpdateUIDelete(wxUpdateUIEvent& event)
{
   // only enable delete button if some template was chosen
   event.Enable(!!m_name);
}

// ----------------------------------------------------------------------------
// our public interface
// ----------------------------------------------------------------------------

// edit the templates for the given folder/profile
bool ConfigureTemplates(Profile *profile,
                        wxWindow *parent,
                        const TemplatePopupMenuItem& menu)
{
   wxFolderTemplatesDialog dlg(menu, profile, parent);

   return dlg.ShowModal() == wxID_OK && dlg.WasChanged();
}

// select a template from all existing ones
String ChooseTemplateFor(MessageTemplateKind kind,
                         wxWindow *parent,
                         const TemplatePopupMenuItem& menu)
{
   wxString value;
   wxChooseTemplateDialog dlg(kind, menu, parent);
   if ( dlg.ShowModal() == wxID_OK )
   {
      value = dlg.GetTemplateValue();
   }

   return value;
}

// edit any templates
void EditTemplates(wxWindow *parent,
                   const TemplatePopupMenuItem& menu)
{
   wxAllTemplatesDialog dlg(menu, parent);
   (void)dlg.ShowModal();
}

