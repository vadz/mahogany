/*-*- c++-*-
 * File:     wxtab.h
 * Purpose:  Tab library (tabbed panels/dialogs, etc.)
 * Author:   Julian Smart
 *
 */

#ifdefndef wx_tabh
#define wx_tabh

#ifdefdef __GNUG__
#pragma interface "wxtab.h"
#endif

#ifdef  USE_WXWINDOWS2
#	include <wx/hash.h>
#	include <wx/string.h>
#else //wxWin 1
#	include <wx/wx_hash.h>
#	include <wx/wxstring.h>
#endif

class wxTabView;

/*
 * A wxTabControl is the internal and visual representation
 * of the tab.
 */

class wxTabControl: public wxObject
{
  DECLARE_DYNAMIC_CLASS(wxTabControl)
  protected:
    wxTabView *view;
    wxString controlLabel;
    Bool isSelected;
    wxFont *labelFont;
    int offsetX; // Offsets from top-left of tab view area (the area below the tabs)
    int offsetY;
    int width;
    int height;
    int id;
    int rowPosition; // Position in row from 0
    int colPosition; // Position in col from 0
  public:
    wxTabControl(wxTabView *v = NULL);
    ~wxTabControl(void);

    virtual void OnDraw(wxDC *dc, Bool lastInRow);
    inline void SetLabel(const wxString& str) { controlLabel = str; }
    inline wxString GetLabel(void) { return controlLabel; }

    inline void SetFont(wxFont *f) { labelFont = f; }
    inline wxFont *GetFont(void) { return labelFont; }

    inline void SetSelected(Bool sel) { isSelected = sel; }
    inline Bool IsSelected(void) { return isSelected; }

    inline void SetPosition(int x, int y) { offsetX = x; offsetY = y; }
    inline void SetSize(int x, int y) { width = x; height = y; }

    inline void SetRowPosition(int r) { rowPosition = r; }
    inline int GetRowPosition() { return rowPosition; }
    inline void SetColPosition(int c) { colPosition = c; }
    inline int GetColPosition() { return colPosition; }

    inline int GetX(void) { return offsetX; }
    inline int GetY(void) { return offsetY; }
    inline int GetWidth(void) { return width; }
    inline int GetHeight(void) { return height; }

    inline int GetId(void) { return id; }
    inline void SetId(int i) { id = i; }

    virtual Bool HitTest(int x, int y);
};

/*
 * Each wxTabLayer is a list of tabs. E.g. there
 * are 3 layers in the MS Word Options dialog.
 */

class wxTabLayer: public wxList
{
  DECLARE_DYNAMIC_CLASS(wxTabLayer)
  protected:
  public:
    wxTabLayer(void)
    {
      DeleteContents(TRUE);
    }
};

/*
 * The wxTabView controls and draws the tabbed object
 */

#define wxTAB_STYLE_DRAW_BOX         1   // Draws 3D boxes round tab layers
#define wxTAB_STYLE_COLOUR_INTERIOR  2   // Colours interior of tabs, otherwise draws outline

class wxTabView: public wxObject
{
  DECLARE_DYNAMIC_CLASS(wxTabView)
 private:
 protected:
   // List of layers, from front to back.
   wxList layers;

   // Selected tab
   int tabSelection;

   // Usual tab height
   int tabHeight;

   // The height of the selected tab
   int tabSelectionHeight;

   // Usual tab width
   int tabWidth;

   // Space between tabs
   int tabHorizontalSpacing;

   // Space between top of normal tab and text
   int tabVerticalTextSpacing;

   // Horizontal offset of each tab row above the first
   int tabHorizontalOffset;

   // Vertical offset between tab rows: NOT NEEDED: use tab height.
//   int tabVerticalOffset;

   // The distance between the bottom of the first tab row
   // and the top of the client area (i.e. the margin)
   int topMargin;

   // The position and size of the view above which the tabs are placed.
   // I.e., the internal client area of the sheet.
   wxRectangle tabViewRect;

   // Bitlist of styles
   long tabStyle;

   // Colours
   wxColour highlightColour;
   wxColour shadowColour;
   wxColour backgroundColour;
   wxColour textColour;

   // Pen and brush cache
   wxPen *highlightPen;
   wxPen *shadowPen;
   wxPen *backgroundPen;
   wxBrush *backgroundBrush;

   wxFont *tabFont;
   wxFont *tabSelectedFont;

   wxDC *tabDC;
 public:
  wxTabView(long style = wxTAB_STYLE_DRAW_BOX | wxTAB_STYLE_COLOUR_INTERIOR);
  ~wxTabView();

  inline int GetNumberOfLayers() { return layers.Number(); }

  // Automatically positions tabs
  wxTabControl *AddTab(int id, const wxString& label);
  
  void ClearTabs();
  
  inline void SetDC(wxDC *dc) { tabDC = dc; }
  inline wxDC *GetDC() { return tabDC; }

  // Draw all tabs
  virtual void Draw(void);
  
  // Process mouse event, return FALSE if we didnt process it
  virtual Bool OnEvent(wxMouseEvent& event);
  
  // Called when a tab is activated
  virtual void OnTabActivate(int activateId, int deactivateId);
  // Allows vetoing
  virtual Bool OnTabPreActivate(int activateId, int deactivateId) { return TRUE; };
  
  // Allows use of application-supplied wxTabControl classes.
  virtual wxTabControl *OnCreateTabControl(void) { return new wxTabControl(this); }

  void SetHighlightColour(const wxColour& col);
  void SetShadowColour(const wxColour& col);
  void SetBackgroundColour(const wxColour& col);
  inline void SetTextColour(const wxColour& col) { textColour = col; }
  
  inline wxColour GetHighlightColour(void) { return highlightColour; }
  inline wxColour GetShadowColour(void) { return shadowColour; }
  inline wxColour GetBackgroundColour(void) { return backgroundColour; }
  inline wxColour GetTextColour(void) { return textColour; }
  inline wxPen *GetHighlightPen(void) { return highlightPen; }
  inline wxPen *GetShadowPen(void) { return shadowPen; }
  inline wxPen *GetBackgroundPen(void) { return backgroundPen; }
  inline wxBrush *GetBackgroundBrush(void) { return backgroundBrush; }
  
  inline void SetViewRect(const wxRectangle& rect) { tabViewRect = rect; }
  inline wxRectangle GetViewRect(void) { return tabViewRect; }
  
  // Calculate tab width to fit to view, and optionally adjust the view
  // to fit the tabs exactly.
  int CalculateTabWidth(int noTabs, Bool adjustView = FALSE);
  
  inline void SetTabStyle(long style) { tabStyle = style; }
  inline long GetTabStyle(void) { return tabStyle; }
  
  inline void SetTabSize(int w, int h) { tabWidth = w; tabHeight = h; }
  inline int GetTabWidth(void) { return tabWidth; }
  inline int GetTabHeight(void) { return tabHeight; }
  inline void SetTabSelectionHeight(int h) { tabSelectionHeight = h; }
  inline int GetTabSelectionHeight(void) { return tabSelectionHeight; }
  
  inline int GetTopMargin(void) { return topMargin; }
  inline void SetTopMargin(int margin) { topMargin = margin; }
  
  void SetTabSelection(int sel);
  inline int GetTabSelection() { return tabSelection; }
  
  // Find tab control for id
  wxTabControl *FindTabControlForId(int id);

  // Find tab control for layer, position (starting from zero)
  wxTabControl *FindTabControlForPosition(int layer, int position);
  
  inline int GetHorizontalTabOffset() { return tabHorizontalOffset; }
  inline int GetHorizontalTabSpacing() { return tabHorizontalSpacing; }
  
  inline void SetVerticalTabTextSpacing(int s) { tabVerticalTextSpacing = s; }
  inline int GetVerticalTabTextSpacing() { return tabVerticalTextSpacing; }
  
  inline wxFont *GetTabFont() { return tabFont; }
  inline void SetTabFont(wxFont *f) { tabFont = f; }

  inline wxFont *GetSelectedTabFont() { return tabSelectedFont; }
  inline void SetSelectedTabFont(wxFont *f) { tabSelectedFont = f; }
  // Find the node and the column at which this control is positioned.
  wxNode *FindTabNodeAndColumn(wxTabControl *control, int *col);
  
  // Do the necessary to change to this tab
  virtual Bool ChangeTab(wxTabControl *control);
};

/*
 * A dialog box class that is tab-friendly
 */
 
#ifdef USE_WXWINDOWS2
class wxTabbedDialogBox : public wxDialog
#else
class wxTabbedDialogBox : public wxDialogBox
#endif
{
   DECLARE_DYNAMIC_CLASS(wxTabbedDialogBox)
 
 protected:
   wxTabView *tabView;
   
 public:

   wxTabbedDialogBox(wxWindow *parent, const char *title, Bool modal, 
                     int x = -1, int y = -1, int width = -1, int height = -1, 
                     long windowStyle = wxDEFAULT_DIALOG_STYLE, 
                     const char *name = "dialogBox");
   ~wxTabbedDialogBox(void);
 
   inline wxTabView *GetTabView() { return tabView; }
   inline void SetTabView(wxTabView *v) { tabView = v; }
  
   virtual ON_CLOSE_TYPE OnClose(void);
   virtual void OnEvent(wxMouseEvent& event);
   virtual void OnPaint(void);
};

/*
 * A panel class that is tab-friendly
 */

class wxTabbedPanel: public wxPanel
{
   DECLARE_DYNAMIC_CLASS(wxTabbedPanel)
 
 protected:
   wxTabView *tabView;
   
 public:

   wxTabbedPanel(wxWindow *parent, int x = -1, int y = -1, 
                                   int width = -1, int height = -1, 
                                   long windowStyle = 0, 
                                   const char *name = "panel");
   ~wxTabbedPanel(void);
 
   inline wxTabView *GetTabView() { return tabView; }
   inline void SetTabView(wxTabView *v) { tabView = v; }
  
   virtual void OnEvent(wxMouseEvent& event);
   virtual void OnPaint(void);
};

class wxPanelTabView: public wxTabView
{
  DECLARE_DYNAMIC_CLASS(wxPanelTabView)
 private:
 protected:
   // List of panels, one for each tab. Indexed
   // by tab ID.
   wxList tabPanels;
   wxPanel *currentPanel;
   wxPanel *panel;
 public:
  wxPanelTabView(wxPanel *pan, long style = wxTAB_STYLE_DRAW_BOX | wxTAB_STYLE_COLOUR_INTERIOR);
  ~wxPanelTabView(void);

  // Called when a tab is activated
  virtual void OnTabActivate(int activateId, int deactivateId);

  // Specific to this class
   void AddTabPanel(int id, wxPanel *panel);
   wxPanel *GetTabPanel(int id);
   void ClearPanels(Bool deletePanels = TRUE);
   wxPanel *GetCurrentPanel() { return currentPanel; }
   
   void ShowPanelForTab(int id);
};

#endif // wx_tabh
  
