/*-*- c++ -*-********************************************************
 * wxLwindow.h : a scrolled Window for displaying/entering rich text*
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *******************************************************************/

#ifdef __GNUG__
#   pragma implementation "wxlwindow.h"
#endif

#include "Mpch.h"

#include "gui/wxlwindow.h"

BEGIN_EVENT_TABLE(wxLayoutWindow,wxScrolledWindow)
   EVT_PAINT    (wxLayoutWindow::OnPaint)
   EVT_CHAR     (wxLayoutWindow::OnChar)

   EVT_LEFT_DOWN(wxLayoutWindow::OnLeftMouseClick)
   EVT_RIGHT_DOWN(wxLayoutWindow::OnRightMouseClick)
   EVT_LEFT_DCLICK(wxLayoutWindow::OnMouseDblClick)
END_EVENT_TABLE()

wxLayoutWindow::wxLayoutWindow(wxWindow *parent)
   : wxScrolledWindow(parent, -1, wxDefaultPosition, wxDefaultSize,
                      wxHSCROLL | wxVSCROLL | wxBORDER)

{
   m_ScrollbarsSet = false;
   m_doSendEvents = false;
}

#ifdef __WXMSW__
long
wxLayoutWindow::MSWGetDlgCode()
{
   // if we don't return this, we won't get OnChar() events
   return DLGC_WANTCHARS | DLGC_WANTARROWS | DLGC_WANTMESSAGE;
}
#endif //MSW


void
wxLayoutWindow::OnMouse(int eventId, wxMouseEvent& event)
{
   wxPaintDC dc( this );
   PrepareDC( dc );     

   SetFocus();

   if(!m_doSendEvents) // nothing to do
      return;

   m_FindPos.x = dc.DeviceToLogicalX(event.GetX());
   m_FindPos.y = dc.DeviceToLogicalY(event.GetY());

   TRACEMESSAGE(("wxLayoutWindow::OnMouse: (%d, %d) -> (%d, %d)",
                 event.GetX(), event.GetY(), m_FindPos.x, m_FindPos.y));

   // find the object at this position
   m_FoundObject = m_llist.Find(m_FindPos);
   if(m_FoundObject)
   {
      wxCommandEvent commandEvent(wxEVENT_TYPE_MENU_COMMAND, eventId);
      commandEvent.SetEventObject( this );
      commandEvent.SetClientData((char *)m_FoundObject);
      m_ClickPosition = wxPoint(event.GetX(), event.GetY());
      GetEventHandler()->ProcessEvent(commandEvent);
   }
}

/*
 * some simple keyboard handling
 */
void
wxLayoutWindow::OnChar(wxKeyEvent& event)
{
   if(! m_llist.IsEditable()) // do nothing
   {
      event.Skip();
      return;
   }
   
   long keyCode = event.KeyCode();
   wxPoint p;
   CoordType help;
   
   switch(event.KeyCode())
   {
      case WXK_RIGHT:
      m_llist.MoveCursor(1);
      break;
   case WXK_LEFT:
      m_llist.MoveCursor(-1);
      break;
   case WXK_UP:
      m_llist.MoveCursor(0,-1);
      break;
   case WXK_DOWN:
      m_llist.MoveCursor(0,1);
      break;
   case WXK_PRIOR:
      m_llist.MoveCursor(0,-20);
      break;
   case WXK_NEXT:
      m_llist.MoveCursor(0,20);
      break;
   case WXK_HOME:
      p = m_llist.GetCursor();
      p.x = 0;
      m_llist.SetCursor(p);
      break;
   case WXK_END:
      p = m_llist.GetCursor();
      p.x = m_llist.GetLineLength(m_llist.FindCurrentObject(NULL));
      m_llist.SetCursor(p);
      break;
   case WXK_DELETE :
      if(event.ControlDown()) // delete to end of line
      {
         help = m_llist.GetLineLength(
            m_llist.FindCurrentObject(NULL))
            - m_llist.GetCursor().x;
         m_llist.Delete(help ? help : 1);
      }
      else
         m_llist.Delete(1);
      break;
   case WXK_BACK: // backspace
      if(m_llist.MoveCursor(-1)) {
         m_llist.Delete(1);
      }
      break;
   case WXK_RETURN:
      m_llist.LineBreak();
      break;

#ifdef WXLAYOUT_DEBUG
   case WXK_F1:
      m_llist.Debug();
      break;
#endif
      
   default:
      if(keyCode < 256 && keyCode >= 32)
      {
         String tmp;
         tmp += keyCode;
         m_llist.Insert(tmp);
      }
      break;
   }

   Refresh();
   UpdateScrollbars();
}

void
wxLayoutWindow::OnPaint( wxPaintEvent &WXUNUSED(event))  // or: OnDraw(wxDC& dc)
{
   wxPaintDC dc( this );  // only when used as OnPaint for OnDraw we
   PrepareDC( dc );       // can skip the first two lines

   if( m_doSendEvents ) // look for keyclicks
      m_FoundObject = m_llist.Draw(dc,true,m_FindPos);
   else
      m_llist.Draw(dc);

   if(! m_ScrollbarsSet)
   {
      m_ScrollbarsSet = true; // avoid recursion
      UpdateScrollbars();
   }
}

void
wxLayoutWindow::UpdateScrollbars(void)
{
   CoordType
      max_x, max_y, lineHeight;
   
   m_llist.GetSize(&max_x, &max_y, &lineHeight);
   SetScrollbars(10, lineHeight, max_x/10+1, max_y/lineHeight+1);
   EnableScrolling(true,true);
}

void
wxLayoutWindow::Print(void)
{
   wxPostScriptDC   dc("layout.ps",true,this);
   if (dc.Ok() && dc.StartDoc((char *)_("Printing message...")))
   {
      //dc.SetUserScale(1.0, 1.0);
      m_llist.Draw(dc);
      dc.EndDoc();
   }
}
