///////////////////////////////////////////////////////////////////////////////
// Name:        generic/vcarddlg.cpp
// Purpose:     the dialog for editing vCards
// Author:      Vadim Zeitlin
// Modified by:
// Created:     16.05.00
// RCS-ID:      $Id$
// Copyright:   (c) 2000 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// this is done in common/vcard.cpp
#if 0 //def __GNUG__
    #pragma implementation "vcard.h"
#endif

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/log.h"
    #include "wx/intl.h"
    #include "wx/datetime.h"

    #include "wx/stattext.h"
    #include "wx/statbox.h"
    #include "wx/checkbox.h"
    #include "wx/listbox.h"
    #include "wx/textctrl.h"
    #include "wx/sizer.h"
    #include "wx/notebook.h"
    #include "wx/textdlg.h"
#endif //WX_PRECOMP

#include "wx/vcard.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// control ids
enum
{
    // the buttons ids must be consecutive in this order
    Email_Add,
    Email_Modify,
    Email_Delete,

    Addr_Add,
    Addr_Modify,
    Addr_Delete,

    Addr_List,

    Max
};

// ----------------------------------------------------------------------------
// vCard editing dialog
// ----------------------------------------------------------------------------

class wxVCardDialog : public wxDialog
{
public:
    wxVCardDialog(wxVCard *vcard);

    virtual bool TransferDataToWindow();
    virtual bool TransferDataFromWindow();

protected:
    // event handlers
    void OnEmailAdd(wxCommandEvent&);
    void OnEmailModify(wxCommandEvent&);
    void OnEmailDelete(wxCommandEvent&);

    void OnAddrAdd(wxCommandEvent&);
    void OnAddrModify(wxCommandEvent&);
    void OnAddrDelete(wxCommandEvent&);

    void OnAddressChange(wxCommandEvent&);

    void OnUpdateEMail(wxUpdateUIEvent&);

    // creates a vertical sizer containing the buttons used with listboxes:
    // add/modify/remove
    wxSizer *CreateLboxButtonsSizer(wxWindow *parent, int idFirstButton);

    // creates a horizontal box sizer containg a label and the control
    wxSizer *CreateLabelSizer(wxWindow *parent,
                              const wxString& label,
                              wxControl *control);

    // fill the address data fields from an address entry
    void UpdateAddressData(int n);

private:
    wxTextCtrl *m_fullname,
               *m_firstname,
               *m_lastname,
               *m_middlename,
               *m_prefixname,
               *m_suffixname,
               *m_orgname,
               *m_department,
               *m_title,
               *m_birthday,
               *m_url,
               *m_postoffice,
               *m_extaddr,
               *m_street,
               *m_city,
               *m_region,
               *m_postcode,
               *m_country;

    wxCheckBox *m_domesticAddr,
               *m_internationalAddr,
               *m_postalAddr,
               *m_parcelAddr,
               *m_homeAddr,
               *m_workAddr;

    wxListBox *m_emails,
              *m_addresses;

    wxVCard *m_vcard;

    DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxVCardDialog, wxDialog)
    EVT_BUTTON(Email_Add, wxVCardDialog::OnEmailAdd)
    EVT_BUTTON(Email_Modify, wxVCardDialog::OnEmailModify)
    EVT_BUTTON(Email_Delete, wxVCardDialog::OnEmailDelete)

    EVT_BUTTON(Addr_Add, wxVCardDialog::OnAddrAdd)
    EVT_BUTTON(Addr_Modify, wxVCardDialog::OnAddrModify)
    EVT_BUTTON(Addr_Delete, wxVCardDialog::OnAddrDelete)

    EVT_UPDATE_UI(Email_Modify, wxVCardDialog::OnUpdateEMail)
    EVT_UPDATE_UI(Email_Delete, wxVCardDialog::OnUpdateEMail)

    EVT_LISTBOX(Addr_List, wxVCardDialog::OnAddressChange)
END_EVENT_TABLE()

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxVCardDialog construction
// ----------------------------------------------------------------------------

wxVCardDialog::wxVCardDialog(wxVCard *vcard)
             : wxDialog(NULL, -1, _("Edit vCard"),
                        wxDefaultPosition, wxDefaultSize,
                        wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER )
{
    // init members
    // ------------
    m_vcard = vcard;

    // create controls
    // ===============

    // the top sizer contains the notebooks at the top and the buttons in the
    // bottom
    // ----------------------------------------------------------------------

    wxBoxSizer *sizerTop = new wxBoxSizer( wxVERTICAL );

    wxNotebook *notebook = new wxNotebook( this, -1 );
    wxNotebookSizer *sizerNotebook = new wxNotebookSizer( notebook );
    sizerTop->Add(sizerNotebook, 1, wxGROW | wxALL, 5);
    sizerTop->Add(CreateButtonSizer(wxOK | wxCANCEL), 0,
                  wxALIGN_RIGHT | (wxALL & ~wxRIGHT), 10);

    #define ADD_TEXT_LABEL_TO_SIZER(sizer, label, var)  \
        var = new wxTextCtrl(panel, -1);                \
        sizer->Add(CreateLabelSizer(panel, label, var), \
                   0, wxGROW | wxALL, 5)

    #define ADD_TEXT_LABEL_TO_GRIDSIZER(sizer, label, var)     \
        var = new wxTextCtrl(panel, -1);                       \
        sizer->Add(new wxStaticText(panel, -1, label), 0,      \
                   wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);   \
        sizer->Add(var, 1, wxGROW | wxALIGN_CENTER_VERTICAL)

    #define ADD_CHECKBOX_TO_SIZER(sizer, label, var)    \
        var = new wxCheckBox(panel, -1, label);         \
        sizer->Add(var, 0, wxALIGN_CENTER_VERTICAL)

    // first page: the identity fields
    // ----------------------------------------------------------------------

    wxPanel *panel = new wxPanel(notebook);
    wxBoxSizer *sizerPanel = new wxBoxSizer(wxVERTICAL);
    wxStaticBoxSizer *sizerName = new wxStaticBoxSizer
                                       (
                                        new wxStaticBox(panel, -1, _("&Name")),
                                        wxVERTICAL
                                       );

    ADD_TEXT_LABEL_TO_SIZER(sizerName, _("&Full name:"), m_fullname);
    sizerName->Add(0, 10);

    // thie 4 column sizer has 5 columns, in fact, as there is an extra spacer
    // between the rows 1 and 2
    wxFlexGridSizer *sizer4Col = new wxFlexGridSizer(5, 5, 5);
    sizer4Col->AddGrowableCol(1);
    sizer4Col->AddGrowableCol(4);

    ADD_TEXT_LABEL_TO_GRIDSIZER(sizer4Col, _("&First:"), m_firstname);
    sizer4Col->Add(10, 0);
    ADD_TEXT_LABEL_TO_GRIDSIZER(sizer4Col, _("&Middle:"), m_middlename);

    ADD_TEXT_LABEL_TO_GRIDSIZER(sizer4Col, _("&Last:"), m_lastname);
    sizer4Col->Add(10, 0);
    ADD_TEXT_LABEL_TO_GRIDSIZER(sizer4Col, _("&Suffix:"), m_suffixname);

    ADD_TEXT_LABEL_TO_GRIDSIZER(sizer4Col, _("&Prefix:"), m_prefixname);

    sizerName->Add(sizer4Col, 0, wxGROW | wxLEFT | wxRIGHT, 5);

    wxBoxSizer *sizerBirthDay = new wxBoxSizer(wxHORIZONTAL);
    ADD_TEXT_LABEL_TO_SIZER(sizerBirthDay, _("&Birthdate:"), m_birthday);

    wxStaticBoxSizer *sizerOrg = new wxStaticBoxSizer
                                       (
                                        new wxStaticBox(panel, -1, _("&Organization")),
                                        wxHORIZONTAL
                                       );

    wxFlexGridSizer *sizer2Col = new wxFlexGridSizer(2, 5, 5);
    sizer2Col->AddGrowableCol(1);
    ADD_TEXT_LABEL_TO_GRIDSIZER(sizer2Col, _("N&ame:"), m_orgname);
    ADD_TEXT_LABEL_TO_GRIDSIZER(sizer2Col, _("&Department:"), m_department);
    ADD_TEXT_LABEL_TO_GRIDSIZER(sizer2Col, _("&Title:"), m_title);
    sizerOrg->Add(sizer2Col, 1, wxGROW | (wxALL & ~wxTOP), 5);

    sizerPanel->Add(sizerName, 0, wxGROW | wxALL, 5);
    sizerPanel->Add(sizerBirthDay, 0, wxGROW | wxALL, 5);
    sizerPanel->Add(sizerOrg, 0, wxGROW | wxALL, 5);

    panel->SetAutoLayout(TRUE);
    panel->SetSizer(sizerPanel);

    notebook->AddPage(panel, _("Identity"));

    // second page: the network-related fields
    // ---------------------------------------

    panel = new wxPanel(notebook);

    sizerPanel = new wxBoxSizer(wxVERTICAL);
    ADD_TEXT_LABEL_TO_SIZER(sizerPanel, _("&URL:"), m_url);

    wxSizer *sizerEmail = new wxStaticBoxSizer
                              (
                               new wxStaticBox(panel, -1, _("&Email list")),
                               wxHORIZONTAL
                              );

    m_emails = new wxListBox(panel, -1);
    sizerEmail->Add(m_emails, 1, wxGROW | wxALL, 5);
    sizerEmail->Add(CreateLboxButtonsSizer(panel, Email_Add), 0, wxCENTRE);

    sizerPanel->Add(sizerEmail, 1, wxGROW | wxALL, 5);

    panel->SetAutoLayout(TRUE);
    panel->SetSizer(sizerPanel);

    notebook->AddPage(panel, _("Network"));

    // third page: address
    // -------------------

    panel = new wxPanel(notebook);

    sizerPanel = new wxBoxSizer(wxHORIZONTAL);

    wxSizer *sizerAddr = new wxStaticBoxSizer
                              (
                               new wxStaticBox(panel, -1, _("&Addresses")),
                               wxHORIZONTAL
                              );

    m_addresses = new wxListBox(panel, Addr_List);
    sizerAddr->Add(m_addresses, 1, wxGROW | wxALL, 5);
    sizerAddr->Add(CreateLboxButtonsSizer(panel, Addr_Add), 0, wxCENTRE);

    sizerPanel->Add(sizerAddr, 1, wxGROW | wxALL, 5);

    sizerAddr = new wxStaticBoxSizer
                    (
                     new wxStaticBox(panel, -1, _("&Data")),
                     wxVERTICAL
                    );

    sizer2Col = new wxFlexGridSizer(2, 5, 5);
    sizer2Col->AddGrowableCol(1);
    ADD_TEXT_LABEL_TO_GRIDSIZER(sizer2Col, _("&Post office:"), m_postoffice);
    ADD_TEXT_LABEL_TO_GRIDSIZER(sizer2Col, _("&Extended:"), m_extaddr);
    ADD_TEXT_LABEL_TO_GRIDSIZER(sizer2Col, _("&Street:"), m_street);
    ADD_TEXT_LABEL_TO_GRIDSIZER(sizer2Col, _("&City:"), m_city);
    ADD_TEXT_LABEL_TO_GRIDSIZER(sizer2Col, _("&Region:"), m_region);
    ADD_TEXT_LABEL_TO_GRIDSIZER(sizer2Col, _("&Postal code:"), m_postcode);
    ADD_TEXT_LABEL_TO_GRIDSIZER(sizer2Col, _("&Country:"), m_country);

    sizerAddr->Add(sizer2Col, 0, wxGROW | wxALL, 5);

    sizer2Col = new wxFlexGridSizer(2, 5, 5);
    ADD_CHECKBOX_TO_SIZER(sizer2Col, _("Domestic"), m_domesticAddr);
    ADD_CHECKBOX_TO_SIZER(sizer2Col, _("International"), m_internationalAddr);
    ADD_CHECKBOX_TO_SIZER(sizer2Col, _("Postal"), m_postalAddr);
    ADD_CHECKBOX_TO_SIZER(sizer2Col, _("Parcel"), m_parcelAddr);
    ADD_CHECKBOX_TO_SIZER(sizer2Col, _("Home"), m_homeAddr);
    ADD_CHECKBOX_TO_SIZER(sizer2Col, _("Work"), m_workAddr);

    sizerAddr->Add(sizer2Col, 0, wxGROW | wxALL, 5);

    sizerPanel->Add(sizerAddr, 1, wxGROW | wxALL, 5);

    panel->SetAutoLayout(TRUE);
    panel->SetSizer(sizerPanel);

    notebook->AddPage(panel, _("Address"));

    // do use the sizer
    // ----------------

    SetAutoLayout( TRUE );
    SetSizer( sizerTop );
    sizerTop->Fit( this );
    sizerTop->SetSizeHints( this );

    // position the dialog
    // -------------------

    // make it wider than its (strictly) minimal width
    SetClientSize(wxSize(200, -1));
    Centre();
}

// creates a vertical sizer containing the buttons used with listboxes:
// add/modify/remove
wxSizer *wxVCardDialog::CreateLboxButtonsSizer(wxWindow *parent,
                                               int id)
{
    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);

    sizer->Add(new wxButton(parent, id++, _("&Add...")), 0, wxCENTRE | wxALL, 5);
    sizer->Add(new wxButton(parent, id++, _("&Modify...")), 0, wxCENTRE | wxALL, 5);
    sizer->Add(new wxButton(parent, id++, _("&Delete")), 0, wxCENTRE | wxALL, 5);

    return sizer;
}

// creates a horizontal box sizer containg a label and text field
wxSizer *wxVCardDialog::CreateLabelSizer(wxWindow *parent,
                                         const wxString& label,
                                         wxControl *control)
{
    wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(new wxStaticText(parent, -1, label), 0, wxALIGN_CENTRE | wxRIGHT, 5);
    sizer->Add(control, 1, wxALIGN_CENTRE | wxLEFT, 5);

    return sizer;
}

// ----------------------------------------------------------------------------
// wxVCardDialog data transfer
// ----------------------------------------------------------------------------

// transfer data from vCard to the window
bool wxVCardDialog::TransferDataToWindow()
{
    wxString value;

    #define PROP_TO_CTRL(prop, ctrl)    \
        if ( m_vcard->Get##prop(&value) ) ctrl->SetValue(value);

    // identity page first
    PROP_TO_CTRL(FullName, m_fullname);
    {
        wxString familyName, givenName, middleName, prefix, suffix;
        if ( m_vcard->GetName(&familyName, &givenName, &middleName, &prefix, &suffix) )
        {
            m_firstname->SetValue(givenName);
            m_lastname->SetValue(familyName);
            m_middlename->SetValue(middleName);
            m_prefixname->SetValue(prefix);
            m_suffixname->SetValue(suffix);
        }
    }

    {
        wxDateTime dt;
        if ( m_vcard->GetBirthDay(&dt) )
        {
            m_birthday->SetValue(dt.FormatDate());
        }
    }

    PROP_TO_CTRL(BusinessRole, m_title);
    {
        wxString org, dept;
        if ( m_vcard->GetOrganization(&org, &dept) )
        {
            m_orgname->SetValue(org);
            m_department->SetValue(dept);
        }
    }

    // network page
    PROP_TO_CTRL(URL, m_url);
    {
        void *cookie;
        wxVCardEMail *email = m_vcard->GetFirstEMail(&cookie);
        while ( email )
        {
            m_emails->Append(email->GetEMail());

            email = m_vcard->GetNextEMail(&cookie);
        }
    }

    #undef PROP_TO_CTRL

    return TRUE;
}

// transfer data to vCard from the window
bool wxVCardDialog::TransferDataFromWindow()
{
    // do the checks first
    wxDateTime dt;
    wxString birthday = m_birthday->GetValue();
    if ( !!birthday && !dt.ParseDate(birthday) )
    {
        wxLogError(_("Invalid birthday date: '%s'"),
                   m_birthday->GetValue().c_str());

        return FALSE;
    }

    wxString value;

    #define CTRL_TO_PROP(prop, ctrl)    \
        value = ctrl->GetValue();       \
        if ( !!value )                  \
            m_vcard->Set##prop(value)

    // identity page
    CTRL_TO_PROP(FullName, m_fullname);
    m_vcard->SetName(m_lastname->GetValue(),
                     m_firstname->GetValue(),
                     m_middlename->GetValue(),
                     m_prefixname->GetValue(),
                     m_suffixname->GetValue());
    if ( !!birthday )
        m_vcard->SetBirthDay(dt);
    CTRL_TO_PROP(BusinessRole, m_title);
    m_vcard->SetOrganization(m_orgname->GetValue(),
                             m_department->GetValue());

    // network page
    CTRL_TO_PROP(URL, m_url);
    {
        m_vcard->ClearEMails();

        size_t count = m_emails->GetCount();
        for ( size_t n = 0; n < count; n++ )
        {
            m_vcard->AddEMail(m_emails->GetString(n));
        }
    }

    #undef CTRL_TO_PROP

    return TRUE;
}

// ----------------------------------------------------------------------------
// wxVCardDialog event handler
// ----------------------------------------------------------------------------

void wxVCardDialog::OnEmailAdd(wxCommandEvent& WXUNUSED(event))
{
    // TODO: allow to specify the type of email
    wxString email = wxGetTextFromUser(_("Email address:"),
                                       _("New email address"),
                                       wxEmptyString,
                                       this);
    if ( !email )
    {
        // cancelled by user
        return;
    }

    m_emails->Append(email);
}

void wxVCardDialog::OnEmailModify(wxCommandEvent& WXUNUSED(event))
{
    int sel = m_emails->GetSelection();
    wxCHECK_RET( sel != -1, _T("button should be disabled") );

    wxString emailOld = m_emails->GetString(sel);

    // TODO: allow to specify the type of email
    wxString email = wxGetTextFromUser(_("Email address:"),
                                       _("Modify the email address"),
                                       emailOld,
                                       this);

    if ( !email || email == emailOld )
        return;

    m_emails->Insert(email, sel);
    m_emails->Delete(sel + 1);  // it was shifted
}

void wxVCardDialog::OnEmailDelete(wxCommandEvent& WXUNUSED(event))
{
    int sel = m_emails->GetSelection();
    wxCHECK_RET( sel != -1, _T("button should be disabled") );

    m_emails->Delete(sel);
}

void wxVCardDialog::OnAddrAdd(wxCommandEvent& WXUNUSED(event))
{
}

void wxVCardDialog::OnAddrModify(wxCommandEvent& WXUNUSED(event))
{
}

void wxVCardDialog::OnAddrDelete(wxCommandEvent& WXUNUSED(event))
{
}

void wxVCardDialog::OnAddressChange(wxCommandEvent& event)
{
    UpdateAddressData(event.GetInt());
}

void wxVCardDialog::OnUpdateEMail(wxUpdateUIEvent& event)
{
    event.Enable( m_emails->GetSelection() != -1 );
}

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

void wxVCardDialog::UpdateAddressData(int n)
{
}

// ----------------------------------------------------------------------------
// our public API
// ----------------------------------------------------------------------------

extern bool WXDLLEXPORT wxEditVCard(wxVCard *vcard)
{
    wxCHECK_MSG( vcard, FALSE, _T("NULL vCard not allowed in wxEditVCard") );

    wxVCardDialog dlg(vcard);
    return dlg.ShowModal() == wxID_OK;
}

extern wxVCard * WXDLLEXPORT wxCreateVCard()
{
    wxVCard *vcard = new wxVCard;
    if ( wxEditVCard(vcard) )
    {
        return vcard;
    }
    else // dialog cancelled
    {
        delete vcard;

        return NULL;
    }
}
