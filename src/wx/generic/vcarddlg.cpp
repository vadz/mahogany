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

    #include "wx/button.h"
    #include "wx/panel.h"
    #include "wx/stattext.h"
    #include "wx/statbox.h"
    #include "wx/checkbox.h"
    #include "wx/listbox.h"
    #include "wx/sizer.h"
    #include "wx/textdlg.h"
#endif //WX_PRECOMP

#include "wx/datetime.h"
#include "wx/notebook.h"

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

    VCardControl_Max
};

// ----------------------------------------------------------------------------
// some macros to make writing sizer-based dialog slightly less painful
// ----------------------------------------------------------------------------

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

// ----------------------------------------------------------------------------
// helper class for address editing
// ----------------------------------------------------------------------------

struct wxVCardAddressData
{
    wxVCardAddressData(wxVCardAddress* addr)
    {
        if ( addr )
        {
            postoffice = addr->GetPostOffice();
            extaddr = addr->GetExtAddress();
            street = addr->GetStreet();
            city = addr->GetLocality();
            region = addr->GetRegion();
            postalcode = addr->GetPostalCode();
            country = addr->GetCountry();
            flags = addr->GetFlags();
        }
        else
        {
            flags = wxVCardAddress::Default;
        }
    }

    wxString postoffice,
             extaddr,
             street,
             city,
             region,
             postalcode,
             country;
    int flags;
};

WX_DECLARE_OBJARRAY(wxVCardAddressData, wxVCardAddresses);

#include "wx/arrimpl.cpp"

WX_DEFINE_OBJARRAY(wxVCardAddresses);

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

    void OnUpdateEMail(wxUpdateUIEvent&);
    void OnUpdateAddress(wxUpdateUIEvent&);

    // creates a vertical sizer containing the buttons used with listboxes:
    // add/modify/remove
    wxSizer *CreateLboxButtonsSizer(wxWindow *parent, int idFirstButton);

    // creates a horizontal box sizer containg a label and the control
    wxSizer *CreateLabelSizer(wxWindow *parent,
                              const wxString& label,
                              wxControl *control);

    // returns the label in the lbox for this address
    wxString GetAddressLabel(const wxVCardAddressData& data) const;

    // adds an address to the list
    void AddAddress(const wxVCardAddressData& data);

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
               *m_url;

    wxListBox *m_emails,
              *m_addresses;

    wxVCard *m_vcard;
    wxVCardAddresses m_addrData;

    DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// address editing dialog
// ----------------------------------------------------------------------------

class wxVCardAddressDialog : public wxDialog
{
public:
    wxVCardAddressDialog(wxWindow *parent, const wxVCardAddressData& data);

    virtual bool TransferDataToWindow();
    virtual bool TransferDataFromWindow();

    const wxVCardAddressData& GetData() const { return m_data; }

protected:
    wxTextCtrl *m_postoffice,
               *m_extaddr,
               *m_street,
               *m_city,
               *m_region,
               *m_postalcode,
               *m_country;

    wxCheckBox *m_domesticAddr,
               *m_internationalAddr,
               *m_postalAddr,
               *m_parcelAddr,
               *m_homeAddr,
               *m_workAddr;

    wxVCardAddressData m_data;
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

    EVT_UPDATE_UI(Addr_Modify, wxVCardDialog::OnUpdateAddress)
    EVT_UPDATE_UI(Addr_Delete, wxVCardDialog::OnUpdateAddress)
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

    // first page: the identity fields
    // ----------------------------------------------------------------------

    wxPanel *panel = new wxPanel(notebook);
    wxBoxSizer *sizerPanel = new wxBoxSizer(wxVERTICAL);
    wxStaticBoxSizer *sizerName = new wxStaticBoxSizer
                                       (
                                        new wxStaticBox(panel, -1, _("&Name:")),
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

    // third page: addresses and address labels
    // ----------------------------------------

    panel = new wxPanel(notebook);

    sizerPanel = new wxBoxSizer(wxHORIZONTAL);

    wxSizer *sizerAddr = new wxStaticBoxSizer
                              (
                               new wxStaticBox(panel, -1, _("&Addresses")),
                               wxHORIZONTAL
                              );

    m_addresses = new wxListBox(panel, -1);
    sizerAddr->Add(m_addresses, 1, wxGROW | wxALL, 5);
    sizerAddr->Add(CreateLboxButtonsSizer(panel, Addr_Add), 0, wxCENTRE);

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
            delete email;

            email = m_vcard->GetNextEMail(&cookie);
        }
    }

    // address page
    {
        void *cookie;
        wxVCardAddress *addr = m_vcard->GetFirstAddress(&cookie);
        while ( addr )
        {
            AddAddress(wxVCardAddressData(addr));

            delete addr;

            addr = m_vcard->GetNextAddress(&cookie);
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

    // address page
    {
        size_t count = m_addrData.GetCount();
        for ( size_t n = 0; n < count; n++ )
        {
            const wxVCardAddressData& d = m_addrData[n];
            m_vcard->AddAddress(d.postoffice,
                                d.extaddr,
                                d.street,
                                d.region,
                                d.postalcode,
                                d.city,
                                d.country,
                                d.flags);
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
    wxVCardAddressDialog dlg(this, wxVCardAddressData(NULL));
    if ( dlg.ShowModal() == wxID_OK )
    {
        AddAddress(dlg.GetData());
    }
}

void wxVCardDialog::OnAddrModify(wxCommandEvent& WXUNUSED(event))
{
    int sel = m_addresses->GetSelection();
    wxCHECK_RET( sel != -1, _T("button should be disabled") );

    wxVCardAddressDialog dlg(this, m_addrData[sel]);
    if ( dlg.ShowModal() == wxID_OK )
    {
        m_addrData[sel] = dlg.GetData();
        m_addresses->SetString(sel, GetAddressLabel(m_addrData[sel]));
    }
}

void wxVCardDialog::OnAddrDelete(wxCommandEvent& WXUNUSED(event))
{
    int sel = m_addresses->GetSelection();
    wxCHECK_RET( sel != -1, _T("button should be disabled") );

    m_addrData.RemoveAt(sel);
    m_addresses->Delete(sel);
}

void wxVCardDialog::OnUpdateEMail(wxUpdateUIEvent& event)
{
    event.Enable( m_emails->GetSelection() != -1 );
}

void wxVCardDialog::OnUpdateAddress(wxUpdateUIEvent& event)
{
    event.Enable( m_addresses->GetSelection() != -1 );
}

// ----------------------------------------------------------------------------
// address helpers
// ----------------------------------------------------------------------------

wxString wxVCardDialog::GetAddressLabel(const wxVCardAddressData& data) const
{
    // take the address start as the string to appear in the listbox
    wxString label;
    label << data.extaddr << _T(' ') << data.street;
    return label;
}

void wxVCardDialog::AddAddress(const wxVCardAddressData& data)
{
    m_addrData.Add(data);
    m_addresses->Append(GetAddressLabel(data));
}

// ----------------------------------------------------------------------------
// wxVCardAddressDialog
// ----------------------------------------------------------------------------

wxVCardAddressDialog::wxVCardAddressDialog(wxWindow *parent,
                                           const wxVCardAddressData& data)
                    : wxDialog(NULL, -1, _("Edit Address"),
                               wxDefaultPosition, wxDefaultSize,
                               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER ),
                      m_data(data)
{
    // create controls
    // ---------------

    wxSizer *sizerTop = new wxBoxSizer(wxVERTICAL);

    wxWindow *panel = this; // macros use this variable

    wxStaticBoxSizer *sizerAddr = new wxStaticBoxSizer
                                        (
                                         new wxStaticBox(panel, -1, _("&Data")),
                                         wxVERTICAL
                                        );

    wxFlexGridSizer *sizer2Col = new wxFlexGridSizer(2, 5, 5);
    sizer2Col->AddGrowableCol(1);
    ADD_TEXT_LABEL_TO_GRIDSIZER(sizer2Col, _("&Post office:"), m_postoffice);
    ADD_TEXT_LABEL_TO_GRIDSIZER(sizer2Col, _("&Extended:"), m_extaddr);
    ADD_TEXT_LABEL_TO_GRIDSIZER(sizer2Col, _("&Street:"), m_street);
    ADD_TEXT_LABEL_TO_GRIDSIZER(sizer2Col, _("&City:"), m_city);
    ADD_TEXT_LABEL_TO_GRIDSIZER(sizer2Col, _("&Region:"), m_region);
    ADD_TEXT_LABEL_TO_GRIDSIZER(sizer2Col, _("&Postal code:"), m_postalcode);
    ADD_TEXT_LABEL_TO_GRIDSIZER(sizer2Col, _("&Country:"), m_country);

    sizerAddr->Add(sizer2Col, 0, wxGROW | wxALL, 5);

    sizer2Col = new wxFlexGridSizer(2, 5, 5);
    ADD_CHECKBOX_TO_SIZER(sizer2Col, _("Domestic"), m_domesticAddr);
    ADD_CHECKBOX_TO_SIZER(sizer2Col, _("International"), m_internationalAddr);
    ADD_CHECKBOX_TO_SIZER(sizer2Col, _("Postal"), m_postalAddr);
    ADD_CHECKBOX_TO_SIZER(sizer2Col, _("Parcel"), m_parcelAddr);
    ADD_CHECKBOX_TO_SIZER(sizer2Col, _("Home"), m_homeAddr);
    ADD_CHECKBOX_TO_SIZER(sizer2Col, _("Work"), m_workAddr);

    wxStaticBoxSizer *sizerAddrType = new wxStaticBoxSizer
                                            (
                                             new wxStaticBox(panel, -1, _("&Type")),
                                             wxVERTICAL
                                            );
    sizerAddrType->Add(sizer2Col, 0, wxGROW);

    sizerAddr->Add(sizerAddrType, 0, wxCENTRE | wxALL, 5);

    sizerTop->Add(sizerAddr, 1, wxGROW | wxALL, 5);
    sizerTop->Add(CreateButtonSizer(wxOK | wxCANCEL), 0,
                  wxALIGN_RIGHT | (wxALL & ~wxRIGHT), 10);

    SetAutoLayout(TRUE);
    SetSizer(sizerTop);
    sizerTop->Fit(this);
    sizerTop->SetSizeHints(this);

    CentreOnParent();
}

bool wxVCardAddressDialog::TransferDataToWindow()
{
    #define ADDR_TO_DLG(field) m_##field->SetValue(m_data.field)

    ADDR_TO_DLG(postoffice);
    ADDR_TO_DLG(extaddr);
    ADDR_TO_DLG(street);
    ADDR_TO_DLG(region);
    ADDR_TO_DLG(postalcode);
    ADDR_TO_DLG(city);
    ADDR_TO_DLG(country);

    #define SET_ADDR_FLAG(flag, ctrl) \
        if ( m_data.flags & wxVCardAddress::flag ) ctrl->SetValue(TRUE)

    SET_ADDR_FLAG(Domestic, m_domesticAddr);
    SET_ADDR_FLAG(Intl, m_internationalAddr);
    SET_ADDR_FLAG(Postal, m_postalAddr);
    SET_ADDR_FLAG(Parcel, m_parcelAddr);
    SET_ADDR_FLAG(Home, m_homeAddr);
    SET_ADDR_FLAG(Work, m_workAddr);

    #undef SET_ADDR_FLAG
    #undef ADDR_TO_DLG

    return TRUE;
}

bool wxVCardAddressDialog::TransferDataFromWindow()
{
    #define DLG_TO_ADDR(field) m_data.field = m_##field->GetValue()

    DLG_TO_ADDR(postoffice);
    DLG_TO_ADDR(extaddr);
    DLG_TO_ADDR(street);
    DLG_TO_ADDR(region);
    DLG_TO_ADDR(postalcode);
    DLG_TO_ADDR(city);
    DLG_TO_ADDR(country);

    #define GET_ADDR_FLAG(flag, ctrl) \
        if ( ctrl->GetValue() ) m_data.flags |= wxVCardAddress::flag

    m_data.flags = 0;

    GET_ADDR_FLAG(Domestic, m_domesticAddr);
    GET_ADDR_FLAG(Intl, m_internationalAddr);
    GET_ADDR_FLAG(Postal, m_postalAddr);
    GET_ADDR_FLAG(Parcel, m_parcelAddr);
    GET_ADDR_FLAG(Home, m_homeAddr);
    GET_ADDR_FLAG(Work, m_workAddr);

    #undef GET_ADDR_FLAG
    #undef DLG_TO_ADDR

    return TRUE;
}

// ----------------------------------------------------------------------------
// our public API
// ----------------------------------------------------------------------------

extern bool WXDLLMAYEXP wxEditVCard(wxVCard *vcard)
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
