///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   modules/LayoutEditor.cpp: implements default MessageEditor
// Purpose:     this is a wxLayout-based implementation of MessageEditor
//              using bits and pieces of the original Karsten's code but
//              repackaged in a modular way and separated from the rest of
//              wxComposeView logic
// Author:      Vadim Zeitlin (based on code by Karsten Ballüder)
// Modified by:
// Created:     13.03.02
// CVS-ID:      $Id$
// Copyright:   (c) 1997-2002 Mahogany team
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
#   include "Mdefaults.h"
#endif // USE_PCH

#include <wx/fontutil.h>      // for wxNativeFontInfo

#include "Composer.h"
#include "MessageEditor.h"

#include "gui/wxlwindow.h"
#include "gui/wxlparser.h"

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_AUTOMATIC_WORDWRAP;
extern const MOption MP_FOCUS_FOLLOWSMOUSE;
extern const MOption MP_WRAPMARGIN;

// ----------------------------------------------------------------------------
// LayoutEditor: a wxLayout-based MessageEditor implementation
// ----------------------------------------------------------------------------

class LayoutEditor : public MessageEditor
{
public:
   // default ctor
   LayoutEditor();
   virtual ~LayoutEditor();

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

   // for wxComposerLayoutWindow only: we have to use
   bool OnFirstTimeFocus() { return MessageEditor::OnFirstTimeFocus(); }
   void OnFirstTimeModify() { MessageEditor::OnFirstTimeModify(); }

private:
   wxLayoutWindow *m_LayoutWindow;

   wxLayoutExportStatus *m_exportStatus;
};

// ----------------------------------------------------------------------------
// wxComposerLayoutWindow: a slightly enhanced wxLayoutWindow which calls back
// the composer the first time it gets focus which allows us to launch an
// external editor then (if configured to do so, of course)
// ----------------------------------------------------------------------------

class wxComposerLayoutWindow : public wxLayoutWindow
{
public:
   wxComposerLayoutWindow(LayoutEditor *editor, wxWindow *parent);

protected:
   // event handlers
   void OnKeyDown(wxKeyEvent& event);
   void OnFocus(wxFocusEvent& event);
   void OnMouseLClick(wxCommandEvent& event);
   void OnMouseRClick(wxCommandEvent& event);

private:
   LayoutEditor *m_editor;

   bool m_firstTimeModify,
        m_firstTimeFocus;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxComposerLayoutWindow)
};

// ----------------------------------------------------------------------------
// LayoutEditData: our wrapper for EditorContentPart which is needed because we
//                 we can't associate it directly with a wxLayoutObject which
//                 wants to have something derived from its UserData
// ----------------------------------------------------------------------------

class LayoutEditData : public wxLayoutObject::UserData
{
public:
   // NB: we take ownership of EditorContentPart object
   LayoutEditData(EditorContentPart *mc)
   {
      m_mc = mc;

      String name = mc->GetFileName();
      if ( !name.empty() )
      {
         name += ' ';
      }

      name << '[' << mc->GetMimeType().GetFull() << ']';

      SetLabel(name);
   }

   // get the real data back
   EditorContentPart *GetContentPart() const { m_mc->IncRef(); return m_mc; }

protected:
   // nobody should delete us directly, we're ref counted
   virtual ~LayoutEditData() { m_mc->DecRef(); }

private:
   EditorContentPart *m_mc;
};

// ============================================================================
// implementation
// ============================================================================

IMPLEMENT_MESSAGE_EDITOR(LayoutEditor,
                         _("Default message editor"),
                         _T("(c) 1997-2001 Mahogany Team"));

// ----------------------------------------------------------------------------
// wxComposerLayoutWindow
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxComposerLayoutWindow, wxLayoutWindow)
   EVT_KEY_DOWN(wxComposerLayoutWindow::OnKeyDown)
   EVT_SET_FOCUS(wxComposerLayoutWindow::OnFocus)

   EVT_MENU(WXLOWIN_MENU_LCLICK, wxComposerLayoutWindow::OnMouseLClick)
   EVT_MENU(WXLOWIN_MENU_RCLICK, wxComposerLayoutWindow::OnMouseRClick)
END_EVENT_TABLE()

wxComposerLayoutWindow::wxComposerLayoutWindow(LayoutEditor *editor,
                                               wxWindow *parent)
                      : wxLayoutWindow(parent)
{
   m_editor = editor;

   // we want to get the notifications about mouse clicks
   SetMouseTracking();

   m_firstTimeModify =
   m_firstTimeFocus = TRUE;
}

void wxComposerLayoutWindow::OnKeyDown(wxKeyEvent& event)
{
   if ( m_firstTimeModify )
   {
      m_firstTimeModify = FALSE;

      m_editor->OnFirstTimeModify();
   }

   event.Skip();
}

void wxComposerLayoutWindow::OnFocus(wxFocusEvent& event)
{
   if ( m_firstTimeFocus )
   {
      m_firstTimeFocus = FALSE;

      if ( m_editor->OnFirstTimeFocus() )
      {
         // composer doesn't need first modification notification any more
         // because it modified the text itself
         m_firstTimeModify = FALSE;
      }
   }

   event.Skip();
}

void wxComposerLayoutWindow::OnMouseLClick(wxCommandEvent& event)
{
   wxLayoutObject *obj = (wxLayoutObject *)event.GetClientData();
   LayoutEditData *data = (LayoutEditData *)obj->GetUserData();
   if ( data )
   {
      EditorContentPart *part = data->GetContentPart();
      if ( part )
      {
         m_editor->EditAttachmentProperties(part);
         part->DecRef();
      }

      data->DecRef();
   }
}

void wxComposerLayoutWindow::OnMouseRClick(wxCommandEvent& event)
{
   wxLayoutObject *obj = (wxLayoutObject *)event.GetClientData();
   LayoutEditData *data = (LayoutEditData *)obj->GetUserData();
   if ( data )
   {
      EditorContentPart *part = data->GetContentPart();
      if ( part )
      {
         CoordType x, y;
         obj->GetSize(&x, &y);

         m_editor->ShowAttachmentMenu(part, wxPoint(x, y));
         part->DecRef();
      }

      data->DecRef();
   }
}

// ----------------------------------------------------------------------------
// LayoutEditor ctor/dtor
// ----------------------------------------------------------------------------

LayoutEditor::LayoutEditor()
{
   m_LayoutWindow = NULL;

   m_exportStatus = NULL;
}

LayoutEditor::~LayoutEditor()
{
   delete m_LayoutWindow;

   // it must have been deleted before but check for it and avoid memory leaks
   // just in case
   if ( m_exportStatus )
   {
      FAIL_MSG( _T("GetNextPart() must be called until it returns NULL!") );

      delete m_exportStatus;
   }
}

void LayoutEditor::Create(Composer *composer, wxWindow *parent)
{
   // save it to be able to call GetProfile() and GetOptions()
   m_composer = composer;

   m_LayoutWindow = new wxComposerLayoutWindow(this, parent);

   Profile *profile = GetProfile();

#if defined(__WXGTK__) || defined(EXPERIMENTAL_FOCUS_FOLLOWS)
   m_LayoutWindow->
      SetFocusFollowMode(READ_CONFIG_BOOL(profile, MP_FOCUS_FOLLOWSMOUSE));
#endif

   Enable(true);
   m_LayoutWindow->SetCursorVisibility(1);
   Clear();

   m_LayoutWindow->SetWrapMargin(READ_CONFIG(profile, MP_WRAPMARGIN));
   m_LayoutWindow->SetWordWrap(READ_CONFIG_BOOL(profile, MP_AUTOMATIC_WORDWRAP));

   // use this status bar for various messages
   m_LayoutWindow->SetStatusBar(composer->GetFrame()->GetStatusBar(), 0, 1);
}

void LayoutEditor::UpdateOptions()
{
   // nothing to do here so far as it's not called anyway
}

bool LayoutEditor::FinishWork()
{
   return true;
}

// ----------------------------------------------------------------------------
// LayoutEditor accessors
// ----------------------------------------------------------------------------

wxWindow *LayoutEditor::GetWindow() const
{
   return m_LayoutWindow;
}

bool LayoutEditor::IsModified() const
{
   return m_LayoutWindow->IsModified();
}

bool LayoutEditor::IsEmpty() const
{
   // see below: the hash is just the text length
   //
   // if our "hash" ever changes, we'd need to be less lazy here
   return !ComputeHash();
}

unsigned long LayoutEditor::ComputeHash() const
{
   // TODO: our hash is quite lame actually - it is just the text length!
   unsigned long len = 0;

   wxLayoutExportObject *exp;
   wxLayoutExportStatus status(m_LayoutWindow->GetLayoutList());

   while( (exp = wxLayoutExport(&status, WXLO_EXPORT_AS_TEXT)) != NULL )
   {
      // non text objects get ignored
      if (exp->type == WXLO_EXPORT_TEXT )
      {
         len += exp->content.text->length();
      }

      delete exp;
   }

   return len;
}

// ----------------------------------------------------------------------------
// LayoutEditor misc operations
// ----------------------------------------------------------------------------

void LayoutEditor::Clear()
{
   ComposerOptions& options = (ComposerOptions &)GetOptions(); // const_cast

   wxFont font;
   if ( !options.m_font.empty() )
   {
      wxNativeFontInfo fontInfo;
      if ( fontInfo.FromString(options.m_font) )
      {
         font.SetNativeFontInfo(fontInfo);
      }
   }

   if ( font.Ok() )
      m_LayoutWindow->Clear(font, &options.m_fg, &options.m_bg);
   else
      m_LayoutWindow->Clear(options.m_fontFamily, options.m_fontSize,
                            (int) wxNORMAL, (int) wxNORMAL, 0,
                            &options.m_fg, &options.m_bg);
}

void LayoutEditor::Enable(bool enable)
{
   m_LayoutWindow->SetEditable(enable);
}

void LayoutEditor::ResetDirty()
{
   m_LayoutWindow->SetModified(false);
}

void LayoutEditor::SetEncoding(wxFontEncoding encoding)
{
   wxLayoutList *llist = m_LayoutWindow->GetLayoutList();
   llist->SetFontEncoding(encoding);
   llist->GetDefaultStyleInfo().enc = encoding;
   m_LayoutWindow->Refresh();
}

void LayoutEditor::MoveCursorTo(unsigned long x, unsigned long y)
{
   m_LayoutWindow->GetLayoutList()->MoveCursorTo(wxPoint(x, y));
   m_LayoutWindow->ScrollToCursor();
}

void LayoutEditor::MoveCursorBy(long x, long y)
{
   wxLayoutList *llist = m_LayoutWindow->GetLayoutList();
   llist->MoveCursorVertically(y);
   llist->MoveCursorHorizontally(x);
   m_LayoutWindow->ScrollToCursor();
}

void LayoutEditor::SetFocus()
{
   m_LayoutWindow->SetFocus();
}

// ----------------------------------------------------------------------------
// LayoutEditor clipboard operations
// ----------------------------------------------------------------------------

void LayoutEditor::Copy()
{
   m_LayoutWindow->Copy( WXLO_COPY_FORMAT, FALSE );
   m_LayoutWindow->Refresh();
}

void LayoutEditor::Cut()
{
   m_LayoutWindow->Cut( WXLO_COPY_FORMAT, FALSE );
   m_LayoutWindow->Refresh();
}

void LayoutEditor::Paste()
{
   m_LayoutWindow->Paste( WXLO_COPY_FORMAT, FALSE );
   m_LayoutWindow->Refresh();
}

// ----------------------------------------------------------------------------
// LayoutEditor printing
// ----------------------------------------------------------------------------

bool LayoutEditor::Print()
{
   return wxLayoutPrintout::Print(m_LayoutWindow,
                                  m_LayoutWindow->GetLayoutList());
}

void LayoutEditor::PrintPreview()
{
   (void)wxLayoutPrintout::PrintPreview(m_LayoutWindow->GetLayoutList());
}

// ----------------------------------------------------------------------------
// LayoutEditor contents: adding data/text
// ----------------------------------------------------------------------------

void LayoutEditor::InsertAttachment(const wxBitmap& icon, EditorContentPart *mc)
{
   wxLayoutObjectIcon *obj = new wxLayoutObjectIcon(icon);
   LayoutEditData *data = new LayoutEditData(mc);
   obj->SetUserData(data);

   // SetUserData has incremented the refCount, which is now 2
   // (it was already 1 right after creation)
   data->DecRef();

   m_LayoutWindow->GetLayoutList()->Insert(obj);
   m_LayoutWindow->SetModified();
   m_LayoutWindow->ResizeScrollbars(true /* exactly */);
   m_LayoutWindow->ScrollToCursor();
   m_LayoutWindow->Refresh();
}

void LayoutEditor::InsertText(const String& text, InsertMode insMode)
{
   // insert the text in the beginning of the message replacing the old
   // text if asked for this, otherwise just append it at the end
   wxLayoutList *listNonTextObjects = NULL;
   if ( insMode == Insert_Replace )
   {
      // VZ: I don't know why exactly does this happen but exporting text and
      //     importing it back adds a '\n' at the end, so this is useful as a
      //     quick workaround for this bug - of course, it's not a real solution
      //     (FIXME)
      if ( !text.empty() )
      {
         size_t index = text.length() - 1;
         if ( text[index] == '\n' )
         {
            // check for "\r\n" too
            if ( index > 0 && text[index - 1] == '\r' )
            {
               // truncate one char before
               index--;
            }

            // FIXME: need this horrible const_cast for this hack to work
            ((String &)text)[index] = '\0';
         }
      }

      // this is not as simple as it sounds, because we normally exported all
      // the text which was in the beginning of the message and it's not one
      // text object, but possibly several text objects and line break
      // objects, so now we must delete them and then recreate the new ones...

      wxLayoutList *layoutList = m_LayoutWindow->GetLayoutList();
      wxLayoutObject *obj;
      wxLayoutExportStatus status(layoutList);
      wxLayoutExportObject *exp;
      while( (exp = wxLayoutExport(&status, WXLO_EXPORT_AS_OBJECTS)) != NULL )
      {
         if ( exp->type == WXLO_EXPORT_OBJECT )
         {
            obj = exp->content.object;
            if ( obj->GetType() == WXLO_TYPE_ICON )
            {
               if ( !listNonTextObjects )
                  listNonTextObjects = new wxLayoutList;

               listNonTextObjects->Insert(obj->Copy());
            }
            //else: ignore text and cmd objects
         }
         delete exp;
      }
      layoutList->Empty();
   }

   // now insert the new text
   wxLayoutImportText(m_LayoutWindow->GetLayoutList(), text);
   m_LayoutWindow->SetModified();
   m_LayoutWindow->SetDirty();

   // and insert the non-text objects back if we had removed them
   if ( listNonTextObjects )
   {
      wxLayoutList *layoutList = m_LayoutWindow->GetLayoutList();
      wxLayoutExportObject *exp;
      wxLayoutExportStatus status2(listNonTextObjects);
      while((exp = wxLayoutExport( &status2,
                                      WXLO_EXPORT_AS_OBJECTS)) != NULL)
         if(exp->type == WXLO_EXPORT_EMPTYLINE)
            layoutList->LineBreak();
         else
            layoutList->Insert(exp->content.object->Copy());
      delete listNonTextObjects;
   }

   m_LayoutWindow->ResizeScrollbars(true /* exactly */);
   m_LayoutWindow->ScrollToCursor();
   m_LayoutWindow->Refresh();
}

// ----------------------------------------------------------------------------
// LayoutEditor contents: enumerating the different parts
// ----------------------------------------------------------------------------

EditorContentPart *LayoutEditor::GetFirstPart()
{
   CHECK( !m_exportStatus, NULL,
          _T("GetNextPart() should be called, not GetFirstPart()") );

   m_exportStatus = new wxLayoutExportStatus(m_LayoutWindow->GetLayoutList());

   return GetNextPart();
}

EditorContentPart *LayoutEditor::GetNextPart()
{
   CHECK( m_exportStatus, NULL, _T("must call GetFirstPart() first!") );

   for ( ;; )
   {
      wxLayoutExportObject *exp = wxLayoutExport
                                  (
                                    m_exportStatus,
                                    WXLO_EXPORT_AS_TEXT,
                                    WXLO_EXPORT_WITH_CRLF
                                  );
      if ( !exp )
      {
         // nothing left, clean up and return
         break;
      }

      // we've got something, what?
      EditorContentPart *mc;
      switch ( exp->type )
      {
         case WXLO_EXPORT_TEXT:
            mc = new EditorContentPart(*exp->content.text);
            break;

         case WXLO_EXPORT_OBJECT:
            {
               wxLayoutObject *lo = exp->content.object;
               if ( lo->GetType() == WXLO_TYPE_ICON )
               {
                  LayoutEditData *data = (LayoutEditData *)lo->GetUserData();
                  mc = data->GetContentPart();

                  // undo IncRef() GetUserData() did
                  data->DecRef();
               }
               else // another object type
               {
                  delete exp;

                  // skip return at the end of the loop
                  continue;
               }
            }
            break;

         default:
            FAIL_MSG( _T("unexpected layout object type") );
            continue;
      }

      delete exp;

      return mc;
   }

   delete m_exportStatus;
   m_exportStatus = NULL;

   return NULL;
}

