/*-*- c++ -*-********************************************************
 * wxLwindow.h : a scrolled Window for displaying/entering rich text*
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *******************************************************************/
#ifndef WXLWINDOW_H
#define WXLWINDOW_H

#ifdef __GNUG__
#   pragma interface "wxlwindow.h"
#endif

#ifndef USE_PCH
#  include   <wx/wx.h>
#endif

#include   "wxllist.h"

class wxLayoutWindow : public wxScrolledWindow
{
public:
   wxLayoutWindow(wxWindow *parent);

   wxLayoutList & GetLayoutList(void) { return m_llist; }

   // clears the window and sets default parameters:
   void Clear(int family = wxROMAN, int size=12, int style=wxNORMAL, int weight=wxNORMAL,
              int underline=0, char const *fg="black", char const
              *bg="white")
      {
         GetLayoutList().Clear(family,size,style,weight,underline,fg,bg);
         SetBackgroundColour( *GetLayoutList().GetDefaults()->GetBGColour());

         m_bDirty = FALSE;
      }

   // callbacks
   // NB: these functions are used as event handlers and must not be virtual
   void OnPaint(wxPaintEvent &event);

   void OnLeftMouseClick(wxMouseEvent& event)
      { OnMouse(WXMENU_LAYOUT_LCLICK, event); }
   void OnRightMouseClick(wxMouseEvent& event)
      { OnMouse(WXMENU_LAYOUT_RCLICK, event); }
   void OnMouseDblClick(wxMouseEvent& event)
      { OnMouse(WXMENU_LAYOUT_DBLCLICK, event); }

   void OnChar(wxKeyEvent& event);

#ifdef __WXMSW__
   virtual long MSWGetDlgCode();
#endif //MSW

   void UpdateScrollbars(void);
   void Print(void);

   /// if the flag is true, we send events when user clicks on embedded objects
   void SetMouseTracking(bool doIt = true) { m_doSendEvents = doIt; }

   virtual ~wxLayoutWindow() { }

   // dirty flag access
   bool IsDirty() const { return m_llist.IsDirty(); }
   void ResetDirty()    { m_llist.ResetDirty();     }

protected:
   // generic function for mouse events processing
   void OnMouse(int eventId, wxMouseEvent& event);

   /// for sending events
   wxWindow *m_Parent;
   bool m_doSendEvents;

   /// the layout list to be displayed
   wxLayoutList m_llist;
   /// have we already set the scrollbars?
   bool m_ScrollbarsSet;
   /// if we want to find an object:
   wxPoint m_FindPos;
   wxLayoutObjectBase *m_FoundObject;
   wxPoint m_ClickPosition;

   /// do we have unsaved data?
   bool m_bDirty;

private:
   DECLARE_EVENT_TABLE()
};

#endif
