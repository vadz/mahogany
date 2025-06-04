/*-*- c++ -*-********************************************************
 * wxlwindow.cpp : a scrolled Window for displaying/entering rich text*
 *                                                                  *
 * (C) 1998-2000 by Karsten Ballüder (ballueder@gmx.net)            *
 *                                                                  *
 * $Id$
 *******************************************************************/

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifdef M_BASEDIR
#   ifndef USE_PCH
#     include "Mcommon.h"
#     include "gui/wxMenuDefs.h"
#     include "gui/wxMApp.h"
#   endif // USE_PCH

#   include <wx/menu.h>
#   include <wx/statusbr.h>
#   include <wx/textctrl.h>
#   include <wx/dcmemory.h>
#   include <wx/dcclient.h>

#   include "gui/wxlwindow.h"
#   include "gui/wxlparser.h"

#   include "MDialogs.h"
#   include "strutil.h"
#else
#   ifdef   __WXMSW__
#       include <wx/msw/private.h>
#   endif

#   include "wxlwindow.h"
#   include "wxlparser.h"
#endif

#include <wx/clipbrd.h>
#include <wx/dataobj.h>

#ifdef WXLAYOUT_USE_CARET
#   include <wx/caret.h>
#endif // WXLAYOUT_USE_CARET

#include <ctype.h>

// ----------------------------------------------------------------------------
// macros
// ----------------------------------------------------------------------------

#ifdef WXLAYOUT_DEBUG
#  define WXLO_DEBUG(x)      wxLogDebug x
#else
#  define WXLO_DEBUG(x)
#endif


// define this to enable (broken) update speedups
//#define WXLO_PARTIAL_REFRESH

// for profiling in debug mode:
WXLO_TIMER_DEFINE(UpdateTimer);
WXLO_TIMER_DEFINE(BlitTimer);
WXLO_TIMER_DEFINE(LayoutTimer);
WXLO_TIMER_DEFINE(TmpTimer);
WXLO_TIMER_DEFINE(DrawTimer);

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

/// offsets to put a nice frame around text
#define WXLO_XOFFSET   4
#define WXLO_YOFFSET   4

/// offset to the right and bottom for when to redraw scrollbars
#define   WXLO_ROFFSET   20
#define   WXLO_BOFFSET   20

/// scroll margins when selecting with the mouse
#define WXLO_SCROLLMARGIN_X   10
#define WXLO_SCROLLMARGIN_Y   10

/// the size of one scrollbar page in pixels
static const int X_SCROLL_PAGE = 10;
static const int Y_SCROLL_PAGE = 20;



// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxLayoutWindow,wxScrolledWindow)
   EVT_SIZE    (wxLayoutWindow::OnSize)

   EVT_PAINT    (wxLayoutWindow::OnPaint)

   EVT_CHAR     (wxLayoutWindow::OnChar)
   EVT_KEY_DOWN (wxLayoutWindow::OnKeyDown)
   EVT_KEY_UP   (wxLayoutWindow::OnKeyUp)

   EVT_LEFT_DOWN(wxLayoutWindow::OnLeftMouseDown)
   EVT_LEFT_UP(wxLayoutWindow::OnLeftMouseUp)
   EVT_RIGHT_DOWN(wxLayoutWindow::OnRightMouseClick)
   EVT_LEFT_DCLICK(wxLayoutWindow::OnMouseDblClick)
   EVT_MIDDLE_DOWN(wxLayoutWindow::OnMiddleMouseDown)
   EVT_MOTION    (wxLayoutWindow::OnMouseMove)

   EVT_UPDATE_UI(WXLOWIN_MENU_UNDERLINE, wxLayoutWindow::OnUpdateMenuUnderline)
   EVT_UPDATE_UI(WXLOWIN_MENU_BOLD, wxLayoutWindow::OnUpdateMenuBold)
   EVT_UPDATE_UI(WXLOWIN_MENU_ITALICS, wxLayoutWindow::OnUpdateMenuItalic)
   EVT_MENU_RANGE(WXLOWIN_MENU_FIRST, WXLOWIN_MENU_LAST, wxLayoutWindow::OnMenu)

   EVT_SET_FOCUS(wxLayoutWindow::OnSetFocus)
   EVT_KILL_FOCUS(wxLayoutWindow::OnKillFocus)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// function prototypes
// ----------------------------------------------------------------------------

/// returns TRUE if keyCode is one of arrows/home/end/page{up|down} keys
static bool IsDirectionKey(long keyCode);

// ============================================================================
// implementation
// ============================================================================

#ifndef wxWANTS_CHARS
#   define wxWANTS_CHARS 0
#endif

// ----------------------------------------------------------------------------
// wxLayoutWindow
// ----------------------------------------------------------------------------

wxLayoutWindow::wxLayoutWindow(wxWindow *parent)
              : wxScrolledWindow(parent, -1,
                                 wxDefaultPosition, wxDefaultSize,
                                 wxHSCROLL | wxVSCROLL |
                                 wxBORDER |
                                 wxWANTS_CHARS),
                m_llist(NULL)
{
   SetStatusBar(NULL); // don't use statusbar
   m_Editable = false;
   m_doSendEvents = false;
   m_ViewStartX = 0; m_ViewStartY = 0;
   m_DoPopupMenu = true;
   m_PopupMenu = MakeFormatMenu();
   m_bitmap = NULL;
   m_llist = new wxLayoutList();
   m_BGbitmap = NULL;
   m_ScrollToCursor = false;
#if defined(__WXGTK__) || defined(EXPERIMENTAL_FOCUS_FOLLOWS)
   m_FocusFollowMode = false;
#endif
   SetWordWrap(false);
   SetWrapMargin(0);

   // no scrollbars initially
   m_hasHScrollbar =
   m_hasVScrollbar = false;

   m_Selecting = false;

#ifdef WXLAYOUT_USE_CARET
   // FIXME cursor size shouldn't be hardcoded
   wxCaret *caret = new wxCaret(this, 2, 20);
   SetCaret(caret);
   m_llist->SetCaret(caret);
#endif // WXLAYOUT_USE_CARET

   m_HaveFocus = FALSE;
   m_HandCursor = FALSE;
   m_CursorVisibility = -1;
   SetCursor(wxCURSOR_IBEAM);
   SetDirty();

   // at least under Windows, this should be the default behaviour
   m_AutoDeleteSelection = TRUE;
}

wxLayoutWindow::~wxLayoutWindow()
{
   delete m_bitmap;
   delete m_llist;
   delete m_PopupMenu;
   SetBackgroundBitmap(NULL);
}

void
wxLayoutWindow::Clear(int family,
                      int size,
                      int style,
                      int weight,
                      int underline,
                      wxColour *fg,
                      wxColour *bg,
                      bool noUpdate)
{
   GetLayoutList()->Clear(family,size,style,weight,underline,fg,bg);

   DoClearWindow(noUpdate);
}

void
wxLayoutWindow::Clear(const wxFont& font,
                      wxColour *fg,
                      wxColour *bg,
                      bool noUpdate)
{
   GetLayoutList()->Clear(font, fg, bg);

   DoClearWindow(noUpdate);
}

void
wxLayoutWindow::DoClearWindow(bool noUpdate)
{
   SetBackgroundColour(GetLayoutList()->GetDefaultStyleInfo().GetBGColour());
   ClearBackground();
   ResizeScrollbars(true);
   SetDirty();
   SetModified(FALSE);
   if ( m_Editable )
      m_CursorVisibility = 1;

#ifdef WXLAYOUT_USE_CARET
   if ( m_CursorVisibility == 1 )
      GetCaret()->Show();
#endif // WXLAYOUT_USE_CARET

   if ( !noUpdate )
   {
      RequestUpdate();
   }
}

void wxLayoutWindow::Refresh(bool eraseBackground, const wxRect *rect)
{
   wxScrolledWindow::Refresh(eraseBackground, rect);
}

void
wxLayoutWindow::OnMouse(int eventId, wxMouseEvent& event)
{
   wxClientDC dc( this );
   PrepareDC( dc );
#ifdef OS_WIN
   // we have to test this to avoid the frame
   // containing the list ctrl being raised to the top from behind another top
   // level frame
   HWND hwndTop = ::GetForegroundWindow();
   wxWindow *frame = wxGetTopLevelParent(this);
   if ( frame && frame->GetHWND() == (WXHWND)hwndTop )
#endif // OS_WIN
      if ( (eventId != WXLOWIN_MENU_MOUSEMOVE
#if defined(__WXGTK__) || defined(EXPERIMENTAL_FOCUS_FOLLOWS)
           || m_FocusFollowMode
#endif
           ) && (wxWindow::FindFocus() != this)
         )
         SetFocus();

   wxPoint findPos;
   findPos.x = dc.DeviceToLogicalX(event.GetX());
   findPos.y = dc.DeviceToLogicalY(event.GetY());

   findPos.x -= WXLO_XOFFSET;
   findPos.y -= WXLO_YOFFSET;

   if(findPos.x < 0)
      findPos.x = 0;
   if(findPos.y < 0)
      findPos.y = 0;

   m_ClickPosition = wxPoint(event.GetX(), event.GetY());

   // Scroll the window if the mouse is at the end of it:
   if(m_Selecting && eventId == WXLOWIN_MENU_MOUSEMOVE)
   {
      //WXLO_DEBUG(("selecting at : %d/%d", (int) event.GetX(), (int)event.GetY()));
      int left, top;
      GetViewStart(&left, &top);
      wxSize size = GetClientSize();
      int xdelta, ydelta;

      if(event.GetX() < WXLO_SCROLLMARGIN_X)
         xdelta = -(WXLO_SCROLLMARGIN_X-event.GetX());
      else if(event.GetX() > size.x-WXLO_SCROLLMARGIN_X)
         xdelta = event.GetX()-size.x+WXLO_SCROLLMARGIN_X;
      else
         xdelta = 0;
      if(event.GetY() < WXLO_SCROLLMARGIN_Y)
         ydelta = -(WXLO_SCROLLMARGIN_Y-event.GetY());
      else if(event.GetY() > size.y-WXLO_SCROLLMARGIN_Y)
         ydelta = event.GetY()-size.y+WXLO_SCROLLMARGIN_Y;
      else
         ydelta = 0;

      //WXLO_DEBUG(("xdelta: %d", (int) xdelta));
      if(xdelta != 0 || ydelta != 0)
      {
         top  += ydelta; if(top < 0) top = 0;
         left += xdelta; if(left < 0) left = 0;
         Scroll(left, top);
      }
   }

   wxPoint cursorPos;
   bool found;
   wxLayoutObject *obj = m_llist->FindObjectScreen(dc, findPos,
                                                   &cursorPos, &found);
   wxLayoutObject::UserData *u = obj ? obj->GetUserData() : NULL;

   // has the mouse only been moved?
   switch ( eventId )
   {
      case WXLOWIN_MENU_MOUSEMOVE:
         {
            // this variables is used to only erase the message in the status
            // bar if we had put it there previously - otherwise empting status
            // bar might be undesirable
            static bool s_hasPutMessageInStatusBar = false;

            // found is only true if we are really over an object, not just
            // behind it
            if(found && u && ! m_Selecting)
            {
               if(!m_HandCursor)
                  SetCursor(wxCURSOR_HAND);
               m_HandCursor = TRUE;
               if(m_StatusBar && m_StatusFieldLabel != -1)
               {
                  const wxString &label = u->GetLabel();
                  if(label.Length())
                  {
                     m_StatusBar->SetStatusText(label,
                                                m_StatusFieldLabel);
                     s_hasPutMessageInStatusBar = true;
                  }
               }
            }
            else
            {
               if(m_HandCursor)
                  SetCursor(wxCURSOR_IBEAM);
               m_HandCursor = FALSE;
               if( m_StatusBar && m_StatusFieldLabel != -1 &&
                   s_hasPutMessageInStatusBar )
               {
                  m_StatusBar->SetStatusText(wxEmptyString, m_StatusFieldLabel);
               }
            }
         }

         // selecting?
         if ( event.LeftIsDown() )
         {
            // m_Selecting might not be set if the button got pressed
            // outside this window, so check for it:
            if( m_Selecting )
            {
               m_llist->ContinueSelection(cursorPos, m_ClickPosition);
               RequestUpdate();  // TODO: we don't have to redraw everything!
            }
         }

         if ( u )
         {
            u->DecRef();
            u = NULL;
         }
         break;

      case WXLOWIN_MENU_LDOWN:
         {
            // always move cursor to mouse click:
            m_llist->MoveCursorTo(cursorPos);

            // clicking a mouse removes the selection
            if ( m_llist->HasSelection() )
            {
               m_llist->DiscardSelection();
               m_Selecting = false;
               RequestUpdate();     // TODO: we don't have to redraw everything!
            }

            // Calculate where the top of the visible area is:
            int x0, y0;
            GetViewStart(&x0,&y0);
            int dx, dy;
            GetScrollPixelsPerUnit(&dx, &dy);
            x0 *= dx; y0 *= dy;

            wxPoint offset(-x0+WXLO_XOFFSET, -y0+WXLO_YOFFSET);

            if(m_CursorVisibility == -1)
               m_CursorVisibility = 1;
#ifdef WXLAYOUT_USE_CARET
            if ( m_CursorVisibility == 1 )
               GetCaret()->Show();
#endif // WXLAYOUT_USE_CARET

            if(m_CursorVisibility)
            {
               // draw a thick cursor for editable windows with focus
               m_llist->DrawCursor(dc, m_HaveFocus && IsEditable(), offset);
            }

#ifdef __WXGTK__
            RequestUpdate(); // RequestUpdate suppresses flicker under GTK
#endif // wxGTK

            // start selection
            m_llist->StartSelection(wxPoint(-1, -1), m_ClickPosition);
            m_Selecting = true;
         }
         break;

      case WXLOWIN_MENU_LUP:
         if ( m_Selecting )
         {
            // end selection at the cursor position corresponding to the
            // current mouse position, but don't move cursor there.
            m_llist->EndSelection(cursorPos,m_ClickPosition);
            m_Selecting = false;
            // copy selected text to primary selection without
            // invalidating it:
            Copy(FALSE,FALSE,TRUE);

            RequestUpdate();     // TODO: we don't have to redraw everything!
         }
         break;

      case WXLOWIN_MENU_MDOWN:
         if ( IsEditable() )
         {
            // paste selected text from primary selection without
            // invalidating it:
            Paste(FALSE, TRUE);
         }
         break;

      case WXLOWIN_MENU_DBLCLICK:
         // select a word under cursor
         m_llist->MoveCursorTo(cursorPos);
         m_llist->MoveCursorWord(-1);
         m_llist->StartSelection();
         m_llist->MoveCursorWord(1, false);
         m_llist->EndSelection();
         m_Selecting = false;
         RequestUpdate();     // TODO: we don't have to redraw everything!
         break;
   }

   // notify about mouse events?
   if( m_doSendEvents )
   {
      // only do the menu if activated, editable and not on a clickable object
      if(eventId == WXLOWIN_MENU_RCLICK
         && IsEditable()
         && (! obj || u == NULL))
      {
         PopupMenu(m_PopupMenu, m_ClickPosition.x, m_ClickPosition.y);
         if(u) u->DecRef();
         return;
      }

      // find the object at this position
      if(obj)
      {
         wxCommandEvent commandEvent(wxEVT_COMMAND_MENU_SELECTED, eventId);
         commandEvent.SetEventObject( this );
         commandEvent.SetClientData((char *)obj);
         GetEventHandler()->ProcessEvent(commandEvent);
      }
   }

   if( u ) u->DecRef();
}

// ----------------------------------------------------------------------------
// keyboard handling.
// ----------------------------------------------------------------------------

void
wxLayoutWindow::OnChar(wxKeyEvent& event)
{
   int keyCode = event.GetKeyCode();
   const bool ctrlDown = event.ControlDown();
   const bool shiftDown = event.ShiftDown();
   const bool altDown = event.AltDown();

#ifdef WXLAYOUT_DEBUG
   if(keyCode == WXK_F1)
   {
      m_llist->Debug();
      return;
   }
#endif

   // Force m_Selecting to be false if shift is no longer
   // pressed. OnKeyUp() cannot catch all Shift-Up events.
   if(m_Selecting && !event.ShiftDown())
   {
      m_Selecting = false;
      m_llist->EndSelection();
      //m_llist->DiscardSelection(); //FIXME: correct?
      // VS - no, it would make selecting text with keyboard a NOP
      Copy(FALSE,FALSE,TRUE);
   }

   // If we deleted the selection here, we must not execute the
   // deletion in Delete/Backspace handling.
   bool deletedSelection = false;
   // pressing any non-arrow key optionally replaces the selection:
   if(m_AutoDeleteSelection
      && IsEditable()
      && !m_Selecting
      && m_llist->HasSelection()
      && ! IsDirectionKey(keyCode)
      && ! (altDown || ctrlDown)
      )
   {
      m_llist->DeleteSelection();
      deletedSelection = true;
      SetDirty();
   }

   // <Shift>+<arrow> starts selection
   if ( IsDirectionKey(keyCode) )
   {
      // just continue the old selection
      if ( m_Selecting && event.ShiftDown() )
            m_llist->ContinueSelection();
      else
      {
         m_llist->DiscardSelection();
         m_Selecting = false;
         if( event.ShiftDown() )
         {
            m_Selecting = true;
            m_llist->StartSelection();
         }
      }
   }

   // If needed, make cursor visible:
   if(m_CursorVisibility == -1)
      m_CursorVisibility = 1;

   /* These two nested switches work like this:
      The first one processes all non-editing keycodes, to move the
      cursor, etc. It's default will process all keycodes causing
      modifications to the buffer, but only if editing is allowed.
   */
   switch(keyCode)
   {
   case WXK_RIGHT:
      if ( ctrlDown )
         m_llist->MoveCursorWord(1);
      else
         m_llist->MoveCursorHorizontally(1);
      break;
   case WXK_LEFT:
      if ( ctrlDown )
         m_llist->MoveCursorWord(-1);
      else
         m_llist->MoveCursorHorizontally(-1);
      break;
   case WXK_UP:
      m_llist->MoveCursorVertically(-1);
      break;
   case WXK_DOWN:
      m_llist->MoveCursorVertically(1);
      break;
   case WXK_PAGEUP:
      m_llist->MoveCursorVertically(-Y_SCROLL_PAGE);
      break;
   case WXK_PAGEDOWN:
      m_llist->MoveCursorVertically(Y_SCROLL_PAGE);
      break;
   case WXK_HOME:
      if ( ctrlDown )
         m_llist->MoveCursorTo(wxPoint(0, 0));
      else
         m_llist->MoveCursorToBeginOfLine();
      break;
   case WXK_END:
      if ( ctrlDown )
         m_llist->MoveCursorToEnd();
      else
         m_llist->MoveCursorToEndOfLine();
      break;

   default:
      if(ctrlDown && ! shiftDown && ! IsEditable())
      {
         switch(keyCode)
         {
         case 'c':
            // this should work even in read-only mode
            Copy(WXLO_COPY_FORMAT, FALSE);
            break;
         case 's': // search
            Find(wxEmptyString);
            break;
         case 't': // search again
            FindAgain();
            break;
         default:
            // we don't handle it, maybe an accelerator?
            event.Skip();
            ;
         }
      }
      else if( IsEditable() )
      {
         /* First, handle control keys */
         if(ctrlDown && !shiftDown && ! altDown)
         {
            if(keyCode >= 'A' && keyCode <= 'Z')
               keyCode = tolower(keyCode);
            switch(keyCode)
            {
               case WXK_INSERT:
                  Copy(WXLO_COPY_FORMAT, FALSE);
                  break;
               case WXK_DELETE :
                  if(! deletedSelection)
                  {
                     m_llist->DeleteWord();
                     SetDirty();
                  }
                  break;
               case 'd':
                  if(! deletedSelection) // already done
                  {
                     m_llist->Delete(1);
                     SetDirty();
                  }
                  break;
               case 'y':
                  m_llist->DeleteLines(1);
                  SetDirty();
                  break;
               case 'h': // like backspace
                  if(m_llist->MoveCursorHorizontally(-1))
                  {
                     m_llist->Delete(1);
                     SetDirty();
                  }
                  break;
               case 's': // search
                  Find(wxEmptyString);
                  break;
               case 't': // search again
                  FindAgain();
                  break;
               case 'u':
                  m_llist->DeleteToBeginOfLine();
                  SetDirty();
                  break;
               case 'k':
                  m_llist->DeleteToEndOfLine();
                  SetDirty();
                  break;
               case 'c':
                  Copy(WXLO_COPY_FORMAT, FALSE);
                  break;
               case 'v':
                  Paste( WXLO_COPY_FORMAT, FALSE );
                  break;
               case 'x':
                  Cut( WXLO_COPY_FORMAT, FALSE );
                  break;
               case 'w':
                  if(m_WrapMargin > 0)
                     m_llist->WrapLine(m_WrapMargin);
                  break;
               case 'q':
                  if(m_WrapMargin > 0)
                     m_llist->WrapAll(m_WrapMargin);
                  break;
#ifdef WXLAYOUT_DEBUG
               case WXK_F1:
                  m_llist->SetFont(-1,-1,-1,-1,true);  // underlined
                  break;
               case 'l':
                  Refresh(TRUE);
                  break;
#endif
               default:
               // we don't handle it, maybe an accelerator?
               event.Skip();
            }
         }
         // ALT only:
         else if( altDown && ! ctrlDown )
         {
            switch(keyCode)
            {
               case WXK_DELETE:
               case 'd':
                  m_llist->DeleteWord();
                  SetDirty();
                  break;
               default:
                  // we don't handle it, maybe an accelerator?
                  event.Skip();
            }
         }
         // no control keys:
         else if ( ! altDown && ! ctrlDown)
         {
            switch(keyCode)
            {
               case WXK_INSERT:
                  if(event.ShiftDown())
                     Paste();
                  break;
               case WXK_DELETE :
                  if(event.ShiftDown())
                     Cut(WXLO_COPY_FORMAT, FALSE);
                  else
                     if(! deletedSelection)
                     {
                        m_llist->Delete(1);
                        SetDirty();
                     }
                  break;
               case WXK_BACK: // backspace
                  if(! deletedSelection)
                     if(m_llist->MoveCursorHorizontally(-1))
                     {
                        m_llist->Delete(1);
                        SetDirty();
                     }
                  break;
               case WXK_RETURN:
                  if(m_DoWordWrap &&
                     m_WrapMargin > 0
                     && m_llist->GetCursorPos().x > m_WrapMargin)
                     m_llist->WrapLine(m_WrapMargin);
                  m_llist->LineBreak();
                  SetDirty();
                  break;

               case WXK_TAB:
                  if ( !event.ShiftDown() )
                  {
                     // TODO should be configurable
                     static const int tabSize = 8;

                     CoordType x = m_llist->GetCursorPos().x;
                     size_t numSpaces = tabSize - x % tabSize;
                     m_llist->Insert(wxString(' ', numSpaces));
                     SetDirty();
                  }
                  break;

               default:
                  if((!(ctrlDown || altDown))
                     && (keyCode < 256 && keyCode >= 32)
                     )
                  {
                     if(m_DoWordWrap
                        && m_WrapMargin > 0
                        && m_llist->GetCursorPos().x > m_WrapMargin
                        && isspace(keyCode))
                        m_llist->WrapLine(m_WrapMargin);
                     m_llist->Insert((wxChar)keyCode);
                     SetDirty();
                  }
                  else
                     // we don't handle it, maybe an accelerator?
                     event.Skip();
                  break;
            }
         }
         else
         {
            event.Skip();
         }
      }// if(IsEditable())
      else
      {
         // we don't handle it, maybe an accelerator?
         event.Skip();
      }
   }// first switch()

   if ( m_Selecting )
   {
      // continue selection to the current (new) cursor position
      m_llist->ContinueSelection();
   }
   ScrollToCursor();
   // refresh the screen
   RequestUpdate();
}

void
wxLayoutWindow::OnKeyDown(wxKeyEvent& event)
{
   if ( event.GetKeyCode() == WXK_TAB )
   {
      if ( !IsEditable() )
      {
         GetParent()->Navigate(event.ShiftDown()
                                 ? wxNavigationKeyEvent::IsBackward
                                 : wxNavigationKeyEvent::IsForward);
      }
   }
   else
   {
      event.Skip();
   }
}

void
wxLayoutWindow::OnKeyUp(wxKeyEvent& event)
{
   if ( event.GetKeyCode() == WXK_SHIFT && m_Selecting )
   {
      m_llist->EndSelection();
      m_Selecting = false;
   }

   event.Skip();
}


void
wxLayoutWindow::ScrollToCursor(void)
{
   //is always needed to make sure we know where the cursor is
   //if(IsDirty())
   //RequestUpdate(m_llist->GetUpdateRect());


   ResizeScrollbars();

   int x0,y0,x1,y1, dx, dy;

   // Calculate where the top of the visible area is:
   GetViewStart(&x0,&y0);
   GetScrollPixelsPerUnit(&dx, &dy);
   x0 *= dx; y0 *= dy;

   WXLO_DEBUG(("ScrollToCursor: GetViewStart is %d/%d", x0, y0));

   // Get the size of the visible window:
   GetClientSize(&x1, &y1);

   // Make sure that the scrollbars are at a position so that the cursor is
   // visible if we are editing
   WXLO_DEBUG(("m_ScrollToCursor = %d", (int) m_ScrollToCursor));
   wxPoint cc = m_llist->GetCursorScreenPos();

   // the cursor should be completely visible in both directions
   wxPoint cs(m_llist->GetCursorSize());
   int nx = -1,
       ny = -1;
   if ( cc.x < x0 || cc.x >= x0 + x1 - cs.x )
   {
      nx = cc.x - x1/2;
      if ( nx < 0 )
         nx = 0;
   }

   if ( cc.y < y0 || cc.y >= y0 + y1 - cs.y )
   {
      ny = cc.y - y1/2;
      if ( ny < 0)
         ny = 0;
   }

   if( nx != -1 || ny != -1 )
   {
      // set new view start
      Scroll(nx == -1 ? -1 : (nx+dx-1)/dx, ny == -1 ? -1 : (ny+dy-1)/dy);
      // avoid recursion
      m_ScrollToCursor = false;
      RequestUpdate();
   }
}

void
wxLayoutWindow::OnPaint( wxPaintEvent &WXUNUSED(event))
{
   wxPaintDC dc(this);

   PrepareDC( dc );

#ifdef WXLAYOUT_USE_CARET
   // hide the caret before drawing anything
   GetCaret()->Hide();
#endif // WXLAYOUT_USE_CARET

   int x0,y0,x1,y1, dx, dy;

   // Calculate where the top of the visible area is:
   GetViewStart(&x0,&y0);
   GetScrollPixelsPerUnit(&dx, &dy);
   x0 *= dx;
   y0 *= dy;

   // Get the size of the visible window:
   GetClientSize(&x1,&y1);
   
   // can happen when the window size is too small
   if ( x1 <= 0 || y1 <= 0 )
   {
      return;
   }

   ResizeScrollbars(true);

   WXLO_TIMER_START(TmpTimer);

   /* Check whether the window has grown, if so, we need to reallocate
      the bitmap to be larger. */
   bool recreateBitmap;
   if ( !m_bitmap )
   {
      recreateBitmap = true;
   }
   else if ( x1 > m_bitmap->GetWidth() || y1 > m_bitmap->GetHeight() )
   {
      delete m_bitmap;
      recreateBitmap = true;
   }
   else
   {
      recreateBitmap = false;
   }

   if ( recreateBitmap )
   {
      // create the new backing store bitmap
      m_bitmap = new wxBitmap(x1, y1);
   }

   wxMemoryDC dcMem;
   dcMem.SelectObject(*m_bitmap);
   dcMem.SetDeviceOrigin(0,0);
   dcMem.SetBackground(m_llist->GetDefaultStyleInfo().GetBGColour());
   dcMem.SetPen(*wxTRANSPARENT_PEN);
   dcMem.SetLogicalFunction(wxCOPY);
   dcMem.Clear();
   WXLO_TIMER_STOP(TmpTimer);

   // fill the background with the background bitmap
   if(m_BGbitmap)
   {
      CoordType
         y, x,
         w = m_BGbitmap->GetWidth(),
         h = m_BGbitmap->GetHeight();
      for(y = 0; y < y1; y+=h)
         for(x = 0; x < x1; x+=w)
            dcMem.DrawBitmap(*m_BGbitmap, x, y);
      dcMem.SetBackgroundMode(wxTRANSPARENT);
   }

   // This is the important bit: we tell the list to draw itself

   // Device origins on the memDC are suspect, we translate manually
   // with the translate parameter of Draw().
   wxPoint offset(-x0+WXLO_XOFFSET,-y0+WXLO_YOFFSET);
   m_llist->Draw(dcMem, offset, y0, y0+y1);

   // We start calculating a new update rect before drawing the
   // cursor, so that the cursor coordinates get included in the next
   // update rectangle (although they are drawn on the memDC, this is
   // needed to erase it):
   m_llist->InvalidateUpdateRect();
   if(m_CursorVisibility == 1)
   {
      // draw a thick cursor for editable windows with focus
      m_llist->DrawCursor(dcMem,
                          m_HaveFocus && IsEditable(),
                          offset);
   }

   WXLO_TIMER_START(BlitTimer);
// Now copy everything to the screen:
#ifdef WXLO_PARTIAL_REFRESH
   // This somehow doesn't work, but even the following bit with the
   // whole rect at once is still a bit broken I think.
   wxRegionIterator ri(GetUpdateRegion());
   if(ri)
      while(ri)
      {
         WXLO_DEBUG(("UpdateRegion: %ld,%ld, %ld,%ld",
                     ri.GetX(),ri.GetY(),ri.GetW(),ri.GetH()));
         dc.Blit(x0+ri.GetX(),y0+ri.GetY(),ri.GetW(),ri.GetH(),
                 dcMem,ri.GetX(),ri.GetY(),wxCOPY,FALSE);
         ri++;
      }
   else
#endif
   {
      dc.Blit(x0, y0, x1, y1, &dcMem, 0, 0, wxCOPY, FALSE);
   }
   WXLO_TIMER_STOP(BlitTimer);


#ifdef WXLAYOUT_USE_CARET
   // show the caret back after everything is redrawn
   GetCaret()->Show();
#endif // WXLAYOUT_USE_CARET

   ResetDirty();

   if ( m_StatusBar && m_StatusFieldCursor != -1 )
   {
      static wxPoint s_oldCursorPos(-1, -1);

      wxPoint pos(m_llist->GetCursorPos());

      // avoid unnecessary status bar refreshes
      if ( pos != s_oldCursorPos )
      {
         s_oldCursorPos = pos;

         wxString label;
         label.Printf(_("Ln:%d Col:%d"), pos.y + 1, pos.x + 1);
         m_StatusBar->SetStatusText(label, m_StatusFieldCursor);
      }
   }

   WXLO_TIMER_PRINT(LayoutTimer);
   WXLO_TIMER_PRINT(BlitTimer);
   WXLO_TIMER_PRINT(TmpTimer);
}

void
wxLayoutWindow::OnSize(wxSizeEvent &event)
{
   if ( m_llist )
      ResizeScrollbars();

   event.Skip();
}

/*
Change the range and position of scrollbars. Has evolved into a
generic Update function which will at some time later cause a repaint
as needed.
*/

void
wxLayoutWindow::ResizeScrollbars(bool exact)
{
   wxClientDC dc( this );
   PrepareDC( dc );
//   m_llist->ForceTotalLayout();

   if(! IsDirty())
   {
      // we are laying out just the minimum, but always up to the
      // cursor line, so the cursor position is updated.
      m_llist->Layout(dc, 0);
      return;
   }
   WXLO_TIMER_START(LayoutTimer);
   m_llist->Layout(dc, -1);
   WXLO_TIMER_STOP(LayoutTimer);
   ResetDirty();

   wxPoint max = m_llist->GetSize();
   wxSize size = GetClientSize();

   WXLO_DEBUG(("ResizeScrollbars: max size = (%ld, %ld)",
               (long int)max.x, (long int) max.y));

   // in the absence of scrollbars we should compare with the client size
   if ( !m_hasHScrollbar )
      m_maxx = size.x;// - WXLO_ROFFSET;
   if ( !m_hasVScrollbar )
      m_maxy = size.y;// - WXLO_BOFFSET;

   // check if the text hasn't become too big
   // TODO why do we set both at once? they're independent...
   if( max.x > m_maxx - WXLO_ROFFSET
       || max.y > m_maxy - WXLO_BOFFSET
       || (max.x < m_maxx - X_SCROLL_PAGE)
       || (max.y < m_maxy - Y_SCROLL_PAGE)
       || exact )
   {
      // text became too large
      if ( !exact )
      {
         // add an extra bit to the sizes to avoid future updates
         max.x += WXLO_ROFFSET;
         max.y += WXLO_BOFFSET;
      }

      bool done = FALSE;
      if(max.x < X_SCROLL_PAGE && m_hasHScrollbar)
      {
         SetScrollbars(0,-1,0,-1,0,-1,true);
         m_hasHScrollbar = FALSE;
         done = TRUE;
      }
      if(max.y < Y_SCROLL_PAGE && m_hasVScrollbar)
      {
         SetScrollbars(-1,0,-1,0,-1,0,true);
         m_hasVScrollbar = FALSE;
         done = TRUE;
      }
      if(! done &&
//         (max.x > X_SCROLL_PAGE || max.y > Y_SCROLL_PAGE)
         (max.x > size.x - X_SCROLL_PAGE|| max.y > size.y - Y_SCROLL_PAGE)
         )
      {
         GetViewStart(&m_ViewStartX, &m_ViewStartY);
         SetScrollbars(X_SCROLL_PAGE,
                       Y_SCROLL_PAGE,
                       max.x / X_SCROLL_PAGE + 2,
                       max.y / Y_SCROLL_PAGE + 2,
                       m_ViewStartX,
                       m_ViewStartY,
                       true);
         m_hasHScrollbar =
            m_hasVScrollbar = true;
//         ScrollToCursor();
      }

      m_maxx = max.x + X_SCROLL_PAGE;
      m_maxy = max.y + Y_SCROLL_PAGE;
   }
}

// ----------------------------------------------------------------------------
//
// clipboard operations
//
// ----------------------------------------------------------------------------

void
wxLayoutWindow::Paste(bool usePrivate, bool primary)
{
   // this only has an effect under X11:
   wxTheClipboard->UsePrimarySelection(primary);
   // Read some text
   if (wxTheClipboard->Open())
   {
      if(usePrivate)
      {
         wxLayoutDataObject wxldo;
         if (wxTheClipboard->IsSupported( wxldo.GetFormat() ))
         {
            if(wxTheClipboard->GetData(wxldo))
            {
               wxTheClipboard->Close();
               wxString str = wxldo.GetLayoutData();
               m_llist->Read(str);
               SetDirty();
               RequestUpdate();
               return;
            }
         }
      }
      wxTextDataObject data;
      if (wxTheClipboard->IsSupported( data.GetFormat() )
          && wxTheClipboard->GetData(data) )
      {
         wxTheClipboard->Close();
         wxString text = data.GetText();
         wxLayoutImportText( m_llist, text);
         SetDirty();
         RequestUpdate();
         return;
      }
   }
   // if everything failed we can still try the primary:
   wxTheClipboard->Close();
   if(! primary) // not tried before
   {
      wxTheClipboard->UsePrimarySelection();
      if (wxTheClipboard->Open())
      {
         wxTextDataObject data;
         if (wxTheClipboard->IsSupported( data.GetFormat() )
             && wxTheClipboard->GetData(data) )
         {
            wxString text = data.GetText();
            wxLayoutImportText( m_llist, text);
            SetDirty();
            RequestUpdate();
         }
         wxTheClipboard->Close();
      }
   }
}

bool
wxLayoutWindow::Copy(bool invalidate, bool privateFormat, bool primary)
{
   // Calling GetSelection() will automatically do an EndSelection()
   // on the list, but we need to take a note of it, too:
   if(m_Selecting)
   {
      m_Selecting = false;
      m_llist->EndSelection();
   }

   wxLayoutDataObject *wldo = new wxLayoutDataObject;
   wxLayoutList *llist = m_llist->GetSelection(wldo, invalidate);
   if(! llist)
   {
      delete wldo;
      return FALSE;
   }

   // Export selection as text:
   wxString text;
   wxLayoutExportObject *exp;
   wxLayoutExportStatus status(llist);
   while((exp = wxLayoutExport( &status, WXLO_EXPORT_AS_TEXT)) != NULL)
   {
      if(exp->type == WXLO_EXPORT_TEXT)
         text << *(exp->content.text);
      delete exp;
   }
   delete llist;

   // The exporter always appends a newline, so we chop it off if it
   // is there:
   {
      size_t len = text.Length();
      if(len > 2 && text[len-2] ==  '\r') // Windows
         text = text.Mid(0,len-2);
      else if(len > 1 && text[len-1] == '\n')
         text = text.Mid(0,len-1);
   }

#if 0
if(! primary) // always copy as text-only to primary selection
   {
      wxTheClipboard->UsePrimarySelection();
      if (wxTheClipboard->Open())
      {
         wxTextDataObject *data = new wxTextDataObject( text );
         wxTheClipboard->SetData( data );
         wxTheClipboard->Close();
      }
   }
#endif

   wxTheClipboard->UsePrimarySelection(primary);
   if (wxTheClipboard->Open())
   {
      wxTextDataObject *data = new wxTextDataObject( text );
      bool rc;

         rc = wxTheClipboard->SetData( data );
      if(privateFormat)
         rc |= wxTheClipboard->SetData( wldo );
      wxTheClipboard->Close();
      return rc;
   }
   else
      delete wldo;

   return FALSE;
}

bool
wxLayoutWindow::Cut(bool privateFormat, bool usePrimary)
{
   if(Copy(false, privateFormat, usePrimary)) // do not invalidate selection after copy
   {
      m_llist->DeleteSelection();
      SetDirty();
      return TRUE;
   }
   else
      return FALSE;
}

// ----------------------------------------------------------------------------
// searching
// ----------------------------------------------------------------------------

bool
wxLayoutWindow::Find(const wxString &needle,
                     wxPoint * fromWhere,
                     const wxString& /* configPath */)
{
   m_FindString = needle;
   CHECK( !m_FindString.empty(), false, _T("can't search for empty string") );

   wxPoint found;

   if(fromWhere == NULL)
      found = m_llist->FindText(m_FindString, wxPoint(0, 0));
   else
      found = m_llist->FindText(m_FindString, *fromWhere);
   if(found.x != -1)
   {
      if(fromWhere)
      {
         *fromWhere = found;
         fromWhere->x ++;
      }
      m_llist->MoveCursorTo(found);
      m_llist->StartSelection(found);
      m_llist->EndSelection(wxPoint(found.x+m_FindString.Length(), found.y));

      ScrollToCursor();
      RequestUpdate();

      // advance beyond the end of the current match so that FindAgain() finds
      // the next occurrence (if any), not the same one
      m_llist->MoveCursorHorizontally(1);

      return true;
   }

   return false;
}


bool
wxLayoutWindow::FindAgain(void)
{
   wxPoint pt = m_llist->GetCursorPos();
   return Find(m_FindString, &pt);
}

// ----------------------------------------------------------------------------
// popup menu stuff
// ----------------------------------------------------------------------------

wxMenu *
wxLayoutWindow::MakeFormatMenu()
{
   wxMenu *m = new wxMenu(_("Layout Menu"));

   m->Append(WXLOWIN_MENU_LARGER   ,_("&Larger"),_("Switch to larger font."), false);
   m->Append(WXLOWIN_MENU_SMALLER  ,_("&Smaller"),_("Switch to smaller font."), false);
   m->AppendSeparator();
   m->Append(WXLOWIN_MENU_UNDERLINE, _("&Underline"),_("Underline mode."), true);
   m->Append(WXLOWIN_MENU_BOLD, _("&Bold"),_("Bold mode."), true);
   m->Append(WXLOWIN_MENU_ITALICS, _("&Italics"),_("Italics mode."), true);
   m->AppendSeparator();
   m->Append(WXLOWIN_MENU_ROMAN     ,_("&Roman"),_("Switch to roman font."), false);
   m->Append(WXLOWIN_MENU_TYPEWRITER,_("&Typewriter"),_("Switch to typewriter font."), false);
   m->Append(WXLOWIN_MENU_SANSSERIF ,_("&Sans Serif"),_("Switch to sans serif font."), false);

   return m;
}

void wxLayoutWindow::OnUpdateMenuUnderline(wxUpdateUIEvent& event)
{
   event.Check(m_llist->IsFontUnderlined());
}

void wxLayoutWindow::OnUpdateMenuBold(wxUpdateUIEvent& event)
{
   event.Check(m_llist->IsFontBold());
}

void wxLayoutWindow::OnUpdateMenuItalic(wxUpdateUIEvent& event)
{
   event.Check(m_llist->IsFontItalic());
}

void wxLayoutWindow::OnMenu(wxCommandEvent& event)
{
   switch (event.GetId())
   {
   case WXLOWIN_MENU_LARGER:
      m_llist->SetFontLarger(); RequestUpdate(); break;
   case WXLOWIN_MENU_SMALLER:
      m_llist->SetFontSmaller(); RequestUpdate(); break;
   case WXLOWIN_MENU_UNDERLINE:
      m_llist->ToggleFontUnderline(); RequestUpdate(); break;
   case WXLOWIN_MENU_BOLD:
      m_llist->ToggleFontWeight(); RequestUpdate(); break;
   case WXLOWIN_MENU_ITALICS:
      m_llist->ToggleFontItalics(); RequestUpdate(); break;
   case WXLOWIN_MENU_ROMAN:
      m_llist->SetFontFamily(wxROMAN); RequestUpdate(); break;
   case WXLOWIN_MENU_TYPEWRITER:
      m_llist->SetFontFamily(wxFIXED); RequestUpdate(); break;
   case WXLOWIN_MENU_SANSSERIF:
      m_llist->SetFontFamily(wxSWISS); RequestUpdate(); break;
   }
}

// ----------------------------------------------------------------------------
// focus
// ----------------------------------------------------------------------------

void
wxLayoutWindow::OnSetFocus(wxFocusEvent &ev)
{
   m_HaveFocus = true;
   ev.Skip();
   RequestUpdate(); // cursor must change
}

void
wxLayoutWindow::OnKillFocus(wxFocusEvent &ev)
{
   m_HaveFocus = false;
   ev.Skip();
   RequestUpdate();// cursor must change
}

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

static bool IsDirectionKey(long keyCode)
{
   switch(keyCode)
   {
      case WXK_UP:
      case WXK_DOWN:
      case WXK_RIGHT:
      case WXK_LEFT:
      case WXK_PAGEUP:
      case WXK_PAGEDOWN:
      case WXK_HOME:
      case WXK_END:
         return true;

      default:
         return false;
   }
}
