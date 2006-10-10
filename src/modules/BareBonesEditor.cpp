///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   modules/BareBonesEditor.cpp: implements minimal MessageEditor
// Purpose:     This editor will be there until some better native editor
//              is developed
// Author:      Robert Vazan
// Modified by:
// Created:     12.07.2003
// CVS-ID:      $Id$
// Copyright:   (c) 2003 Mahogany team
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
#   include "Mcommon.h"
#   include "strutil.h"
#   include "Mdefaults.h"
#   include "guidef.h"           // for EnsureAvailableTextEncoding

#   include <wx/sizer.h>
#   include <wx/textctrl.h>
#   include <wx/button.h>        // for wxButton
#   include <wx/panel.h>         // for wxPanel
#endif // USE_PCH

#include <wx/imaglist.h>         // for wxImageList
#include <wx/notebook.h>
#include <wx/listctrl.h>         // for wxListEvent

#include "gui/wxMenuDefs.h"      // for WXMENU_COMPOSE_INSERTFILE

#include "Composer.h"
#ifdef OS_WIN
#include "Mpers.h"
#endif // OS_WIN
#include "gui/wxMDialogs.h"

#include "MessageEditor.h"

#if wxUSE_WCHAR_T && defined(OS_WIN)
   #include <wx/encconv.h>
   #include <wx/fontmap.h>
#endif // wxUSE_WCHAR_T

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_WRAPMARGIN;
extern const MOption MP_REPLY_MSGPREFIX;

// ----------------------------------------------------------------------------
// persistent msgboxes we use here
// ----------------------------------------------------------------------------

extern const MPersMsgBox *M_MSGBOX_FORMAT_PARAGRAPH_BEFORE_EXIT;
extern const MPersMsgBox *M_MSGBOX_REMOVE_ALL_ATTACH;

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// control ids
enum
{
   Button_FormatParagraph = 100,
   Button_FormatAll,
   Button_UnformatParagraph,
   Button_UnformatAll,

   Button_AttachFile = 200,
   Button_RemoveAllAttach,

   ListCtrl_Attachments
};

// ----------------------------------------------------------------------------
// BareBonesEditor: minimalistic MessageEditor implementation
// ----------------------------------------------------------------------------

class FormattedParagraph;
class wxBareBonesEditorNotebook;
class wxBareBonesTextControl;

class BareBonesEditor : public MessageEditor
{
public:
   // default ctor
   BareBonesEditor();
   virtual ~BareBonesEditor();

   // accessors
   virtual wxWindow *GetWindow() const;
   virtual bool IsModified() const;
   virtual bool IsEmpty() const;
   virtual unsigned long ComputeHash() const;

   // creation
   virtual void Create(Composer *composer, wxWindow *parent);
   virtual void UpdateOptions();
   virtual bool FinishWork();

   // operations
   virtual void Clear();
   virtual void Enable(bool enable);
   virtual void ResetDirty();
   virtual void SetEncoding(wxFontEncoding encoding);
   virtual void Copy();
   virtual void Cut();
   virtual void Paste();

   virtual bool Print();
   virtual void PrintPreview();

   virtual void MoveCursorTo(unsigned long x, unsigned long y);
   virtual void MoveCursorBy(long x, long y);
   virtual void SetFocus();

   // content
   virtual void InsertAttachment(const wxBitmap& icon, EditorContentPart *mc);
   virtual void InsertText(const String& text, InsertMode insMode);
   virtual EditorContentPart *GetFirstPart();
   virtual EditorContentPart *GetNextPart();

   // for wxBareBonesTextControl only: we have to use
   bool OnFirstTimeFocus() { return MessageEditor::OnFirstTimeFocus(); }
   void OnFirstTimeModify() { MessageEditor::OnFirstTimeModify(); }

   bool HasEncoding() const { return m_encoding != wxFONTENCODING_SYSTEM; }

   void DeleteAllAttachments();

   friend class FormattedParagraph;
   friend class wxBareBonesEditorNotebook;

private:
   // must be called when the encoding changes to reflect it at wxTextCtrl
   // level
   void SetFontEncoding(wxFontEncoding encoding);


   wxBareBonesEditorNotebook *m_notebook;
   wxBareBonesTextControl *m_textControl;
   wxListCtrl *m_attachments;

   // the encoding to use for the text in the control (and the font)
   wxFontEncoding m_encoding;

   int m_getNextAttachement;

   bool m_hasNewAttachments;
};

class FormattedParagraph
{
public:
   FormattedParagraph(wxTextCtrl *control,BareBonesEditor *editor);

   void FromCursor();
   void First();
   void Next();
   bool IsEmpty() const { return m_from == m_to; }
   void Format();
   void Unformat();
   bool NeedsFormat();
   void FormatAll();
   void UnformatAll();

private:
   bool IsWhiteLine(int line);
   int FindLineByWhite(int start,bool white);
   String Get();
   void Set(const String &modified);
   int LineToPosition(int line);
   bool IsQuoted() const;
   String UnformatCommon();
   String FormatCommon();
   int SizeWithoutNewline(const String &paragraph);
   int FindLineLength(
      const String &paragraph,int lineStart,int paragraphEnd) const;
   size_t RFind(const String &where,wxChar what,size_t start) const;
   size_t FindLastNotOf(const String &where,wxChar what,size_t start) const;
   void FindSignature();
   bool PastSignature() { return m_from >= m_signature; }

   wxTextCtrl *m_control;
   int m_from;
   int m_to;
   int m_margin;
   String m_prefix;
   int m_signature; // Line with signature separator
   Profile *m_profile;
};

// ----------------------------------------------------------------------------
// the notebook containing wxBareBonesTextControl and wxBareBonesAttachments
// ----------------------------------------------------------------------------

class wxBareBonesEditorNotebook : public wxNotebook
{
public:
   wxBareBonesEditorNotebook(BareBonesEditor *editor,wxWindow *parent);

   wxBareBonesTextControl *GetTextControl() const { return m_textControl; }
   wxListCtrl *GetList() const { return m_attachments; }

protected:
   void OnFormatParagraph(wxCommandEvent& event);
   void OnFormatAll(wxCommandEvent& event);
   void OnUnformatParagraph(wxCommandEvent& event);
   void OnUnformatAll(wxCommandEvent& event);

   void OnAttachFile(wxCommandEvent& event);
   void OnRemoveAllAttach(wxCommandEvent& event);

   void OnItemActivate(wxListEvent& event);
   void OnItemRightClick(wxListEvent& event);

private:
   wxSizer *CreateButtonRow(wxWindow *parent);
   void CreateAttachmentPage();

   BareBonesEditor *m_editor;
   wxBareBonesTextControl *m_textControl;
   wxListCtrl *m_attachments;

   DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// the text control where the user types the message text
// ----------------------------------------------------------------------------

class wxBareBonesTextControl : public wxTextCtrl
{
public:
   wxBareBonesTextControl(BareBonesEditor *editor,wxWindow *parent);

#if wxUSE_WCHAR_T && defined(OS_WIN)
   // get the text as a Unicode string
   wxWCharBuffer GetUnicodeText() const;
#endif // wxUSE_WCHAR_T

protected:
   // event handlers
   void OnKeyDown(wxKeyEvent& event);
   void OnFocus(wxFocusEvent& event);

private:
   BareBonesEditor *m_editor;

   bool m_firstTimeModify;
   bool m_firstTimeFocus;

   DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// the list control in icon mode showing all the attachments
// ----------------------------------------------------------------------------

class wxBareBonesAttachments : public wxListView
{
public:
   wxBareBonesAttachments(wxWindow *parent)
      : wxListView(parent,ListCtrl_Attachments,wxDefaultPosition,
         wxDefaultSize,wxLC_ICON | wxLC_SINGLE_SEL | wxSUNKEN_BORDER)
      {}

protected:
   void OnKeyDown(wxKeyEvent& event);

private:
   DECLARE_EVENT_TABLE()
};

// FIXME: this duplicates the code in Singature.cpp and Trailer.cpp
//        and should somehow be shared (but it is also used incorrectly
//        here, i.e. we don't detect signature starting from the end...)
bool DoesSignatureStartHere(const wxChar *cptr)
{
   bool isSig = false;

   // hard coded detection for standard signature separator "--"
   // and the mailing list trailer "____...___"
   if ( cptr[0] == '-' && cptr[1] == '-' )
   {
      // there may be an optional space after "--" (in fact the
      // space should be there but some people don't put it)
      const wxChar *p = cptr + 2;
      if ( IsEndOfLine(p) || (*p == ' ' && IsEndOfLine(p + 1)) )
      {
         // looks like the start of the sig
         isSig = true;
      }
   }
   else if ( cptr[0] == '_' )
   {
      const wxChar *p = cptr + 1;
      while ( *p == '_' )
         p++;

      // arbitrarily consider that there should be at least 40 underscores...
      if ( IsEndOfLine(p) && p - cptr >= 40 )
      {
         // looks like the mailing list trailer
         isSig = true;
      }
   }

   return isSig;
}

// ============================================================================
// implementation
// ============================================================================

IMPLEMENT_MESSAGE_EDITOR(BareBonesEditor,
                         _("Minimal message editor"),
                         _T("(c) 2003 Mahogany Team"));

// ----------------------------------------------------------------------------
// FormattedParagraph
// ----------------------------------------------------------------------------

FormattedParagraph::FormattedParagraph(wxTextCtrl *control,
                                       BareBonesEditor *editor)
   : m_control(control), m_from(0), m_to(0), m_profile(editor->GetProfile())
{
   m_margin = READ_CONFIG(m_profile,MP_WRAPMARGIN);
   if(m_margin <= 0)
      m_margin = 1;
   m_prefix = READ_CONFIG_TEXT(m_profile,MP_REPLY_MSGPREFIX);
}

void FormattedParagraph::FromCursor()
{
   long throwAwayX;
   long currentLine;
   m_control->PositionToXY(m_control->GetInsertionPoint(),
      &throwAwayX,&currentLine);

   if(IsWhiteLine(currentLine) && !(currentLine > 0
      && !IsWhiteLine(currentLine-1)))
   {
      m_from = m_to = currentLine;
   }
   else
   {
      int upNonWhite;
      for(upNonWhite = currentLine;
         upNonWhite > 0 && !IsWhiteLine(upNonWhite-1); --upNonWhite)
      {
      }
      m_from = upNonWhite;
      m_to = FindLineByWhite(m_from+1,true);
   }
}

bool FormattedParagraph::IsWhiteLine(int line)
{
   String text = m_control->GetLineText(line);

   bool graph = false;
   for(size_t each = 0; each < text.size(); ++each )
      graph = graph || wxIsgraph(text[each]);

   return !graph;
}

int FormattedParagraph::FindLineByWhite(int start,bool white)
{
   int lineCount = m_control->GetNumberOfLines();
   int result;
   for(result = start; result < lineCount && IsWhiteLine(result) != white;
      ++result)
   {
   }
   return result;
}

void FormattedParagraph::First()
{
   m_from = FindLineByWhite(0,false);
   m_to = FindLineByWhite(m_from,true);
}

void FormattedParagraph::Next()
{
   m_from = FindLineByWhite(m_to,false);
   m_to = FindLineByWhite(m_from,true);
}

void FormattedParagraph::Format()
{
   if(!IsEmpty() && !IsQuoted())
      Set(FormatCommon());
}

void FormattedParagraph::Unformat()
{
   if(!IsEmpty() && !IsQuoted())
      Set(UnformatCommon());
}

String FormattedParagraph::Get()
{
   return m_control->GetRange(LineToPosition(m_from),LineToPosition(m_to));
}

void FormattedParagraph::Set(const String &modified)
{
   m_control->Replace(LineToPosition(m_from),LineToPosition(m_to),modified);
   int lineCount = 0;
   size_t newline;
   for(newline = modified.find(_T('\n')); newline != modified.npos;
      newline = modified.find(_T('\n'),newline+1))
   {
      ++lineCount;
   }
   if(!modified.empty() && modified[modified.size()-1] != _T('\n'))
      ++lineCount;
   int shift = lineCount-(m_to-m_from);
   m_to += shift;
   m_signature += shift;
}

int FormattedParagraph::LineToPosition(int line)
{
   if(line >= m_control->GetNumberOfLines())
      return m_control->GetLastPosition();
   return m_control->XYToPosition(0,line);
}

bool FormattedParagraph::IsQuoted() const
{
   for ( int line = m_from; line < m_to; ++line )
   {
      if ( !m_control->GetLineText(line).StartsWith(m_prefix) )
         return false;
   }

   return true;
}

String FormattedParagraph::UnformatCommon()
{
   String modified(Get());
   size_t newline;
   for(newline = modified.find(_T('\n'));
      newline != modified.npos && newline != modified.size()-1;
      newline = modified.find(_T('\n'),newline+1))
   {
      modified[newline] = _T(' ');
   }
   return modified;
}

bool FormattedParagraph::NeedsFormat()
{
   return !IsEmpty() && !IsQuoted() && Get() != FormatCommon();
}

void FormattedParagraph::FormatAll()
{
   FindSignature();
   for(First(); !IsEmpty() && !PastSignature(); Next())
      Format();
}

void FormattedParagraph::UnformatAll()
{
   FindSignature();
   for(First(); !IsEmpty() && !PastSignature(); Next())
      Unformat();
}

String FormattedParagraph::FormatCommon()
{
   String modified;
   int lineStart;
   int lineLength;

   const String old(UnformatCommon());
   const int contentSize = SizeWithoutNewline(old);

   for(lineStart = 0; contentSize-lineStart > m_margin;
      lineStart += lineLength+1)
   {
      lineLength = FindLineLength(old,lineStart,contentSize);
      modified.append(old,lineStart,lineLength);
      modified.append(1,_T('\n'));
   }

   if(lineStart < contentSize)
      modified.append(old,lineStart,old.size()-lineStart);

   return modified;
}

int FormattedParagraph::SizeWithoutNewline(const String &paragraph)
{
   int contentSize;

   if(!paragraph.empty() && paragraph[paragraph.size()-1] == _T('\n'))
      contentSize = paragraph.size()-1;
   else
      contentSize = paragraph.size();

   return contentSize;
}

int FormattedParagraph::FindLineLength(
   const String &paragraph,int lineStart,int paragraphEnd) const
{
   int lineLength;

   // Try to find at least one space on the line. Pick the last one.
   // Include one character past margin in case it is space.
   size_t findSpace = RFind(paragraph,_T(' '),lineStart+m_margin);
   if(findSpace != paragraph.npos && (int)findSpace >= lineStart)
   {
      // There are spaces and wrapping within margin is possible.
      // Let's see whether there is something else than spaces.
      // Pick last non-space character that precedes our terminating space.
      size_t findNonSpace = paragraph.find_last_not_of(_T(' '),findSpace);
      if(findNonSpace != paragraph.npos && (int)findNonSpace >= lineStart)
      {
         // There is some text followed by some spaces within margin.
         // If there are multiple spaces, cut first one.
         lineLength = findNonSpace-lineStart+1;
      }
      else
      {
         // Whole line up to margin consists of spaces. Maybe there is
         // some word that crosses margin at end. Stuff all spaces on
         // separate line.
         lineLength = findSpace-lineStart;
      }
   }
   else
   {
      // It's one word longer than margin. Find its end.
      size_t wordEnd = paragraph.find(_T(' '),lineStart+m_margin);
      if(wordEnd != paragraph.npos)
      {
         // It's not the last word in paragraph. Cut at first space.
         lineLength = wordEnd-lineStart;
      }
      else
      {
         // It's last word in paragraph. Stuff it all on one line.
         lineLength = paragraphEnd-lineStart;
      }
   }

   return lineLength;
}

size_t FormattedParagraph::RFind(
   const String &where,wxChar what,size_t start) const
{
#if wxCHECK_VERSION(2,5,0) // Bug in wxWindows implementation of rfind
   size_t result = where.rfind(what,start);
#else
   size_t result = where.npos;
   int cursor;

   for( cursor = start; cursor >= 0; --cursor )
   {
      if( where[cursor] == what )
      {
         result = (size_t)cursor;
         break;
      }
   }
#endif

   return result;
}

size_t FormattedParagraph::FindLastNotOf(
   const String &where,wxChar what,size_t start) const
{
#if wxCHECK_VERSION(2,5,0) // Bug in wxWindows implem. of find_last_not_of
   size_t result = where.find_last_not_of(what,start);
#else
   size_t result = where.npos;
   int cursor;

   for( cursor = start; cursor >= 0; --cursor )
   {
      if( where[cursor] != what )
      {
         result = (size_t)cursor;
         break;
      }
   }
#endif

   return result;
}

void FormattedParagraph::FindSignature()
{
   int count = m_control->GetNumberOfLines();

   for ( m_signature = 0; m_signature < count; ++m_signature )
   {
      String line = m_control->GetLineText(m_signature);
      line += '\n';
      if ( DoesSignatureStartHere(line) )
         break;
   }
}

// ----------------------------------------------------------------------------
// wxBareBonesEditorNotebook
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxBareBonesEditorNotebook, wxNotebook)
   EVT_BUTTON(Button_FormatParagraph,
      wxBareBonesEditorNotebook::OnFormatParagraph)
   EVT_BUTTON(Button_FormatAll,
      wxBareBonesEditorNotebook::OnFormatAll)
   EVT_BUTTON(Button_UnformatParagraph,
      wxBareBonesEditorNotebook::OnUnformatParagraph)
   EVT_BUTTON(Button_UnformatAll,
      wxBareBonesEditorNotebook::OnUnformatAll)

   EVT_BUTTON(Button_AttachFile,
      wxBareBonesEditorNotebook::OnAttachFile)
   EVT_BUTTON(Button_RemoveAllAttach,
      wxBareBonesEditorNotebook::OnRemoveAllAttach)

   EVT_LIST_ITEM_ACTIVATED(ListCtrl_Attachments,
      wxBareBonesEditorNotebook::OnItemActivate)
   EVT_LIST_ITEM_RIGHT_CLICK(ListCtrl_Attachments,
      wxBareBonesEditorNotebook::OnItemRightClick)
END_EVENT_TABLE()

wxBareBonesEditorNotebook::wxBareBonesEditorNotebook(
   BareBonesEditor *editor,wxWindow *parent)
   : wxNotebook(parent,-1), m_editor(editor)
{
   wxPanel *body = new wxPanel(this);
   AddPage(body,_("Message Body"));

   wxSizer *textColumn = new wxBoxSizer(wxVERTICAL);
   body->SetSizer(textColumn);

   m_textControl = new wxBareBonesTextControl(editor,body);
   textColumn->Add(m_textControl,1,wxEXPAND);

   textColumn->Add(CreateButtonRow(body),
                   0,
                   wxTOP | wxBOTTOM | wxALIGN_CENTER_HORIZONTAL,
                   LAYOUT_Y_MARGIN);

   CreateAttachmentPage();
}

wxSizer *wxBareBonesEditorNotebook::CreateButtonRow(wxWindow *parent)
{
   wxSizer *buttonRow = new wxBoxSizer(wxHORIZONTAL);

   buttonRow->Add(new wxButton(parent,Button_FormatParagraph,
      _("Wrap &Paragraph")),0,wxEXPAND|wxRIGHT, LAYOUT_X_MARGIN);
   buttonRow->Add(new wxButton(parent,Button_FormatAll,
      _("&Wrap All")),0,wxEXPAND|wxLEFT|wxRIGHT, LAYOUT_X_MARGIN);
   buttonRow->Add(new wxButton(parent,Button_UnformatParagraph,
      _("Unwrap Paragrap&h")),0,wxEXPAND|wxLEFT|wxRIGHT, LAYOUT_X_MARGIN);
   buttonRow->Add(new wxButton(parent,Button_UnformatAll,
      _("&Unwrap All")),0,wxEXPAND|wxLEFT, LAYOUT_X_MARGIN);

   return buttonRow;
}

void wxBareBonesEditorNotebook::CreateAttachmentPage()
{
   wxPanel *files = new wxPanel(this);
   AddPage(files,_("Attachments"));

   wxBoxSizer *fileSizer = new wxBoxSizer(wxVERTICAL);
   files->SetSizer(fileSizer);

   m_attachments = new wxBareBonesAttachments(files);
   fileSizer->Add(m_attachments,1,wxEXPAND);

   wxSizer *sizerBtns = new wxBoxSizer(wxHORIZONTAL);
   sizerBtns->Add(new wxButton(files, Button_AttachFile, _("&Attach file...")),
                  0,
                  wxRIGHT, LAYOUT_X_MARGIN);
   sizerBtns->Add(new wxButton(files, Button_RemoveAllAttach, _("&Remove all")),
                  0,
                  wxLEFT, LAYOUT_X_MARGIN);

   fileSizer->Add(sizerBtns,
                  0,
                  wxTOP | wxBOTTOM | wxALIGN_CENTER_HORIZONTAL,
                  LAYOUT_Y_MARGIN);

   // FIXME: don't hardcode 32 here
   m_attachments->AssignImageList(new wxImageList(32,32),wxIMAGE_LIST_NORMAL);
}

void wxBareBonesEditorNotebook::OnFormatParagraph(wxCommandEvent& /* event */)
{
   FormattedParagraph paragraph(m_textControl,m_editor);

   paragraph.FromCursor();
   paragraph.Format();

   m_textControl->SetFocus();
}

void wxBareBonesEditorNotebook::OnFormatAll(wxCommandEvent& /* event */)
{
   FormattedParagraph paragraph(m_textControl,m_editor);
   paragraph.FormatAll();

   m_textControl->SetFocus();
}

void wxBareBonesEditorNotebook::OnUnformatParagraph(wxCommandEvent& /* event */)
{
   FormattedParagraph paragraph(m_textControl,m_editor);

   paragraph.FromCursor();
   paragraph.Unformat();

   m_textControl->SetFocus();
}

void wxBareBonesEditorNotebook::OnUnformatAll(wxCommandEvent& /* event */)
{
   FormattedParagraph paragraph(m_textControl,m_editor);
   paragraph.UnformatAll();

   m_textControl->SetFocus();
}

void wxBareBonesEditorNotebook::OnAttachFile(wxCommandEvent& /* event */)
{
   // we don't want to duplicate the code from wxComposeView.cpp here so just
   // emulate a menu command
   wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED, WXMENU_COMPOSE_INSERTFILE);

   GetParent()->ProcessEvent(event);
}

void wxBareBonesEditorNotebook::OnRemoveAllAttach(wxCommandEvent& /* event */)
{
   // are there any attachment to remove, in fact?
   const int count = m_attachments->GetItemCount();
   if ( !count )
   {
      // nothing to do
      return;
   }

   // ask for confirmation before removing potentially many attachments
   if ( MDialog_YesNoDialog
        (
            _("Remove all the attachments from this message\n"
              "(files on the disk will not be affected)?"),
            this,
            MDIALOG_YESNOTITLE,
            M_DLG_NO_DEFAULT | M_DLG_NOT_ON_NO,
            M_MSGBOX_REMOVE_ALL_ATTACH
        ) )
   {
      m_editor->DeleteAllAttachments();
   }
}

void wxBareBonesEditorNotebook::OnItemActivate(wxListEvent& event)
{
   m_editor->EditAttachmentProperties((EditorContentPart *)event.GetData());
}

void wxBareBonesEditorNotebook::OnItemRightClick(wxListEvent& event)
{
   m_editor->ShowAttachmentMenu((EditorContentPart *)event.GetData(),
                                event.GetPoint());
}

// ----------------------------------------------------------------------------
// wxBareBonesAttachments
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxBareBonesAttachments, wxListView)
   EVT_KEY_DOWN(wxBareBonesAttachments::OnKeyDown)
END_EVENT_TABLE()

void wxBareBonesAttachments::OnKeyDown(wxKeyEvent& event)
{
   if( event.GetKeyCode() == WXK_DELETE )
   {
      int selected = GetFirstSelected();
      if( selected >= 0 )
      {
         wxListItem item;
         item.SetId(selected);
         item.SetMask(wxLIST_MASK_DATA);
         GetItem(item);

         EditorContentPart *part = (EditorContentPart *)item.GetData();

         DeleteItem(selected);

         if ( part )
            part->DecRef();
      }
   }

   event.Skip();
}

// ----------------------------------------------------------------------------
// wxBareBonesTextControl
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxBareBonesTextControl, wxTextCtrl)
   EVT_KEY_DOWN(wxBareBonesTextControl::OnKeyDown)
   EVT_SET_FOCUS(wxBareBonesTextControl::OnFocus)
END_EVENT_TABLE()

wxBareBonesTextControl::wxBareBonesTextControl(BareBonesEditor *editor,
                                               wxWindow *parent)
                      : wxTextCtrl(parent,
                                   -1,
                                   wxEmptyString,
                                   wxDefaultPosition,
                                   wxDefaultSize,
                                   // use wxTE_RICH to allow for more than 64Kb
                                   // of text under Win9x and wxTE_RICH2 for
                                   // better charset support (also for Win32)
                                   wxTE_MULTILINE |
                                   wxTE_RICH2 |
                                   wxTE_PROCESS_TAB)
{
   m_editor = editor;

   m_firstTimeModify = true;
   m_firstTimeFocus = true;
}

void wxBareBonesTextControl::OnKeyDown(wxKeyEvent& event)
{
   if ( m_firstTimeModify )
   {
      m_firstTimeModify = false;

      m_editor->OnFirstTimeModify();
   }

   event.Skip();
}

void wxBareBonesTextControl::OnFocus(wxFocusEvent& event)
{
   if ( m_firstTimeFocus )
   {
      m_firstTimeFocus = false;

      if ( m_editor->OnFirstTimeFocus() )
      {
         // composer doesn't need first modification notification any more
         // because it modified the text itself
         m_firstTimeModify = false;
      }
   }

   event.Skip();
}


#if wxUSE_WCHAR_T && defined(OS_WIN) && !defined(__WINE__)
wxWCharBuffer
wxBareBonesTextControl::GetUnicodeText() const
{
   wxString s = StreamOut(wxFONTENCODING_UTF8);
   return s.wc_str(wxConvUTF8);
}

#endif // wxUSE_WCHAR_T

// ----------------------------------------------------------------------------
// BareBonesEditor ctor/dtor
// ----------------------------------------------------------------------------

BareBonesEditor::BareBonesEditor()
{
   m_textControl = NULL;

   m_encoding = wxFONTENCODING_SYSTEM;

   m_getNextAttachement = -1;

   m_hasNewAttachments = false;
}

BareBonesEditor::~BareBonesEditor()
{
   DeleteAllAttachments();
}

void BareBonesEditor::DeleteAllAttachments()
{
   // free all EditorContentPart objects in wxListCtrl
   const int count = m_attachments->GetItemCount();
   for ( int itemIndex = 0; itemIndex < count; ++itemIndex )
   {
      wxListItem itemProperties;

      itemProperties.SetId(itemIndex);
      itemProperties.SetMask(wxLIST_MASK_DATA);

      m_attachments->GetItem(itemProperties);

      EditorContentPart *file = (EditorContentPart *)itemProperties.GetData();
      if ( file )
         file->DecRef();
   }

   m_attachments->DeleteAllItems();
   m_hasNewAttachments = false;
}

void BareBonesEditor::Create(Composer *composer, wxWindow *parent)
{
   // save it to be able to call GetProfile() and GetOptions()
   m_composer = composer;

   m_notebook = new wxBareBonesEditorNotebook(this,parent);

   m_textControl = m_notebook->GetTextControl();
   m_attachments = m_notebook->GetList();

   Enable(true);
   Clear();
}

void BareBonesEditor::UpdateOptions()
{
   // nothing to do here so far as it's not called anyway
}

bool BareBonesEditor::FinishWork()
{
   FormattedParagraph paragraph(m_textControl,this);

   bool needsFormat = false;
   for(paragraph.First(); !paragraph.IsEmpty(); paragraph.Next())
      needsFormat = needsFormat || paragraph.NeedsFormat();

   if(needsFormat)
   {
      switch
      (
         MDialog_YesNoCancel
         (
            _("Would you like to format all paragraphs first?"),
            m_notebook,
            MDIALOG_YESNOTITLE,
            M_DLG_YES_DEFAULT,
            M_MSGBOX_FORMAT_PARAGRAPH_BEFORE_EXIT
         )
      )
      {
         case MDlg_Yes:
            paragraph.FormatAll();
            return false;
         case MDlg_No:
            break;
         case MDlg_Cancel:
            return false;
      }
   }

   return true;
}

// ----------------------------------------------------------------------------
// BareBonesEditor accessors
// ----------------------------------------------------------------------------

wxWindow *BareBonesEditor::GetWindow() const
{
   return m_notebook;
}

bool BareBonesEditor::IsModified() const
{
   return m_textControl->IsModified() || m_hasNewAttachments;
}

bool BareBonesEditor::IsEmpty() const
{
   return m_textControl->GetLastPosition() == 0;
}

unsigned long BareBonesEditor::ComputeHash() const
{
   // TODO: our hash is quite lame actually - it is just the text length!
   return m_textControl->GetLastPosition();
}

// ----------------------------------------------------------------------------
// BareBonesEditor misc operations
// ----------------------------------------------------------------------------

void BareBonesEditor::Clear()
{
   const ComposerOptions& options = GetOptions();
   if ( !options.m_font.empty() )
   {
      wxFont font(GetOptions().GetFont());
      if ( font.Ok() )
         m_textControl->SetFont(font);
   }

   if ( options.m_fg.Ok() )
      m_textControl->SetForegroundColour(options.m_fg);
   if ( options.m_bg.Ok() )
      m_textControl->SetBackgroundColour(options.m_bg);

   m_textControl->Clear();

   DeleteAllAttachments();
}

void BareBonesEditor::Enable(bool enable)
{
   m_textControl->SetEditable(enable);
}

void BareBonesEditor::ResetDirty()
{
   m_textControl->DiscardEdits();
   m_hasNewAttachments = false;
}

void BareBonesEditor::SetFontEncoding(wxFontEncoding encoding)
{
   if ( encoding == wxFONTENCODING_SYSTEM )
   {
      return;
   }

   wxFont fontOld = m_textControl->GetFont();
#if 0 // FIXME: wxGtk2.4/wxFont::SetEncoding doesn't work for some reason
   wxFont fontNew(fontOld);
   fontNew.SetEncoding(m_encoding);
#else
   wxFont fontNew(fontOld.GetPointSize(),
                  fontOld.GetFamily(),
                  fontOld.GetStyle(),
                  fontOld.GetWeight(),
                  fontOld.GetUnderlined(),
                  fontOld.GetFaceName(),
                  encoding);
#endif

   if ( fontNew.Ok() )
   {
      m_textControl->SetFont(fontNew);
   }
}

void BareBonesEditor::SetEncoding(wxFontEncoding encoding)
{
   SetFontEncoding(m_encoding = encoding);
}

void BareBonesEditor::MoveCursorTo(unsigned long x, unsigned long y)
{
   long correctedY = y;
   if(correctedY < 0)
      correctedY = 0;
   long countY = m_textControl->GetNumberOfLines();
   if(countY > 0 && correctedY >= countY)
      correctedY = countY - 1;

   long correctedX = x;
   if(correctedX < 0)
      correctedX = 0;
   long countX = m_textControl->GetLineLength(correctedY);
   if(countX >= 0 && correctedX > countX)
      correctedX = countX;

   long position = m_textControl->XYToPosition(correctedX,correctedY);
   m_textControl->SetInsertionPoint(position);
   m_textControl->ShowPosition(position);
}

void BareBonesEditor::MoveCursorBy(long x, long y)
{
   long oldX;
   long oldY;
   m_textControl->PositionToXY(m_textControl->GetInsertionPoint(),
      &oldX,&oldY);
   MoveCursorTo(oldX+x,oldY+y);
}

void BareBonesEditor::SetFocus()
{
   m_textControl->SetFocus();
}

// ----------------------------------------------------------------------------
// BareBonesEditor clipboard operations
// ----------------------------------------------------------------------------

void BareBonesEditor::Copy()
{
   m_textControl->Copy();
}

void BareBonesEditor::Cut()
{
   m_textControl->Cut();
}

void BareBonesEditor::Paste()
{
   m_textControl->Paste();
}

// ----------------------------------------------------------------------------
// BareBonesEditor printing
// ----------------------------------------------------------------------------

bool BareBonesEditor::Print()
{
   return false;
}

void BareBonesEditor::PrintPreview()
{
}

// ----------------------------------------------------------------------------
// BareBonesEditor contents: adding data/text
// ----------------------------------------------------------------------------

void
BareBonesEditor::InsertAttachment(const wxBitmap& icon, EditorContentPart *mc)
{
   wxListItem item;

   item.SetData(mc);
   item.SetText(mc->GetName());
   item.SetId(m_attachments->GetItemCount());
   item.SetImage(
      m_attachments->GetImageList(wxIMAGE_LIST_NORMAL)->Add(icon));
   item.SetMask(wxLIST_MASK_TEXT | wxLIST_MASK_IMAGE | wxLIST_MASK_DATA);

   m_attachments->InsertItem(item);

   m_hasNewAttachments = true;
}

void BareBonesEditor::InsertText(const String& textOrig, InsertMode insMode)
{
   // Translate CRLF to LF. Internal strings should have only LF, but
   // obviously they don't. All strings returned from c-client have CRLF
   // and not all code translates it to LF, so we have to do it here.
   //
   // VZ: this really shouldn't be necessary and it could be a big performance
   //     problem with huge messages (FIXME)
   String text(strutil_enforceLF(textOrig));


   // we may have to translate the text in another encoding if exactly this one
   // is not available (but equivalent one is)
   if ( EnsureAvailableTextEncoding(&m_encoding, &text, true /* may ask */) )
   {
      SetFontEncoding(m_encoding);
   }
   //else: don't change the font, encoding is not supported anyhow


   switch ( insMode )
   {
      case Insert_Replace:
         m_textControl->SetValue(text);

         // we want our IsModified() to return true when new text is added
         m_textControl->MarkDirty();
         break;

      case Insert_Append:
         m_textControl->AppendText(text);
         break;

      case Insert_Insert:
         m_textControl->WriteText(text);
         break;
   }
}

// ----------------------------------------------------------------------------
// BareBonesEditor contents: enumerating the different parts
// ----------------------------------------------------------------------------

// from strutil.cpp
extern wxFontEncoding GuessUnicodeCharset(const wchar_t *pwz);

EditorContentPart *BareBonesEditor::GetFirstPart()
{
   m_getNextAttachement = 0;

#if wxUSE_WCHAR_T && defined(OS_WIN) && !defined(__WINE__)
   wxFontEncoding encPart = wxFONTENCODING_SYSTEM;
   wxWCharBuffer wbuf = m_textControl->GetUnicodeText();

   for ( const wchar_t *pwc = wbuf; *pwc; pwc++ )
   {
      static wchar_t wbuf2[2] = { L'\0', L'\0' };
      wbuf2[0] = *pwc;

      const wxFontEncoding encThis = GuessUnicodeCharset(wbuf2);
      if ( encThis != wxFONTENCODING_SYSTEM && encThis != encPart )
      {
         if ( encPart != wxFONTENCODING_SYSTEM )
         {
            // we have two (or more) different encodings here, we must use UTF8
            // to send both of them inside a single message part
            encPart = wxFONTENCODING_UTF8;

            // and we don't have to look any further, we found all we wanted
            break;
         }

         // assume we're going to use this encoding for this entire part
         encPart = encThis;
      }
   }

   if ( encPart != wxFONTENCODING_SYSTEM )
   {
      // must do this first or we wouldn't get the data from MSW rich edit
      // control at all (it would try to convert text to the last set encoding
      // and drop any characters it can't convert)
      SetFontEncoding(encPart);
   }
#else
   wxFontEncoding encPart = m_encoding;
#endif // wxUSE_WCHAR_T

   wxString text = m_textControl->GetValue();

   // Translate LF to CRLF. All internal strings should have only LF, but
   // due to long bug tradition, many strings that contain messages have
   // CRLF newlines. They are passed to c-client, which expects CRLF, so
   // think twice before changing it.
   //
   // VZ: see above (FIXME)
   return new EditorContentPart(strutil_enforceCRLF(text), encPart);
}

EditorContentPart *BareBonesEditor::GetNextPart()
{
   const int count = m_attachments->GetItemCount();

   CHECK( m_getNextAttachement >= 0 || m_getNextAttachement < count, NULL,
            _T("forgot to call BareBonesEditor::GetFirstPart()?") );

   if ( m_getNextAttachement == count )
   {
      // no more parts
      m_getNextAttachement = -1;
      return NULL;
   }

   wxListItem item;
   item.SetId(m_getNextAttachement++);
   item.SetMask(wxLIST_MASK_DATA);
   m_attachments->GetItem(item);

   EditorContentPart *file = (EditorContentPart *)item.GetData();
   if ( file )
   {
      file->IncRef();
   }
   else
   {
      FAIL_MSG( _T("NULL attachment in wxBareBonesAttachments?") );
   }

   return file;
}

