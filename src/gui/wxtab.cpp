/*
 * File:     wxtab.cc
 * Purpose:  Tab library (tabbed panels/dialogs, etc.)
 * Author:   Julian Smart
 */

#ifdef __GNUG__
#pragma implementation "wxtab.h"
#endif

// For compilers that support precompilation, includes "wx.h".
#include <wx/wx_prec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

#include "wxtab.h"

IMPLEMENT_DYNAMIC_CLASS(wxTabControl, wxObject)

IMPLEMENT_DYNAMIC_CLASS(wxTabLayer, wxList)

wxTabControl::wxTabControl(wxTabView *v)
{
  view = v;
  isSelected = FALSE;
  labelFont = NULL;
  offsetX = 0;
  offsetY = 0;
  width = 0;
  height = 0;
  id = 0;
  rowPosition = 0;
  colPosition = 0;
}

wxTabControl::~wxTabControl(void)
{
}
    
void wxTabControl::OnDraw(wxDC *dc, Bool lastInRow)
{
  if (!view || !dc)
    return;
    
  // Top-left of tab view area
  int viewX = view->GetViewRect().x;
  int viewY = view->GetViewRect().y;
  
  // Top-left of tab control
  int tabX = GetX() + viewX;
  int tabY = GetY() + viewY;
  int tabHeightInc = 0;
  if (isSelected)
  {
    tabHeightInc = (view->GetTabSelectionHeight() - view->GetTabHeight());
    tabY -= tabHeightInc;
  }
  
  dc->SetPen(wxTRANSPARENT_PEN);
  
  // Draw grey background
  if (view->GetTabStyle() & wxTAB_STYLE_COLOUR_INTERIOR)
  {
    dc->SetBrush(view->GetBackgroundBrush());
    dc->DrawRectangle((float)tabX, (float)tabY, (float)GetWidth(), (float)(GetHeight() + tabHeightInc));
  }
  
  // Draw highlight and shadow
  dc->SetPen(view->GetHighlightPen());
  
  // Calculate the top of the tab beneath. It's the height of the tab, MINUS
  // a bit if the tab below happens to be selected. Check.
  wxTabControl *tabBeneath = NULL;
  int subtractThis = 0;
  if (GetColPosition() > 0)
    tabBeneath = view->FindTabControlForPosition(GetColPosition() - 1, GetRowPosition());
  if (tabBeneath && tabBeneath->IsSelected())
    subtractThis = (view->GetTabSelectionHeight() - view->GetTabHeight());
    
  // Vertical highlight: if first tab, draw to bottom of view
  if (tabX == view->GetViewRect().x && (view->GetTabStyle() & wxTAB_STYLE_DRAW_BOX))
    dc->DrawLine((float)tabX, (float)tabY, (float)tabX, (float)(view->GetViewRect().y + view->GetViewRect().height));
  else
    dc->DrawLine((float)tabX, (float)tabY, (float)tabX, (float)(tabY + GetHeight() + tabHeightInc - subtractThis));
    
  dc->DrawLine((float)tabX, (float)tabY, (float)(tabX + GetWidth()), (float)tabY);
  dc->SetPen(view->GetShadowPen());

  // Test if we're outside the right-hand edge of the view area
  if (((tabX + GetWidth()) >= view->GetViewRect().x + view->GetViewRect().width) && (view->GetTabStyle() & wxTAB_STYLE_DRAW_BOX))
  {
    int bottomY = view->GetViewRect().y + view->GetViewRect().height + GetY() + view->GetTabHeight() + view->GetTopMargin();
    // Add a tab height since we wish to draw to the bottom of the view.
    dc->DrawLine((float)(tabX + GetWidth()), (float)tabY,
      (float)(tabX + GetWidth()), (float)bottomY);
      
    // Calculate the far-right of the view, since we don't wish to
    // draw inside that
    int rightOfView = view->GetViewRect().x + view->GetViewRect().width + 1;

    // Draw the horizontal bit to connect to the view rectangle
    dc->DrawLine((float)(wxMax((tabX + GetWidth() - view->GetHorizontalTabOffset()), rightOfView)), (float)(bottomY-1),
      (float)(tabX + GetWidth()), (float)(bottomY-1));
    
    // Draw black line to emphasize shadow
    dc->SetPen(wxBLACK_PEN);
    dc->DrawLine((float)(tabX + GetWidth() + 1), (float)(tabY+1),
      (float)(tabX + GetWidth() + 1), (float)bottomY);
      
    // Draw the horizontal bit to connect to the view rectangle
    dc->DrawLine((float)(wxMax((tabX + GetWidth() - view->GetHorizontalTabOffset()), rightOfView)), (float)(bottomY),
      (float)(tabX + GetWidth() + 1), (float)(bottomY));
  }
  else
  {
    if (lastInRow)
    {
      // If last in row, must draw to the top of the view (+ margin)
      
      int topY = view->GetViewRect().y - view->GetTopMargin();
      
      // Shadow
      dc->DrawLine((float)(tabX + GetWidth()), (float)tabY, (float)(tabX + GetWidth()), (float)topY);
      // Draw black line to emphasize shadow
      dc->SetPen(wxBLACK_PEN);
      dc->DrawLine((float)(tabX + GetWidth() + 1), (float)(tabY+1), (float)(tabX + GetWidth() + 1),
         (float)topY);
    }
    else
    {
      // Calculate the top of the tab beneath. It's the height of the tab, MINUS
      // a bit if the tab below (and next col along) happens to be selected. Check.
      wxTabControl *tabBeneath = NULL;
      int subtractThis = 0;
      if (GetColPosition() > 0)
        tabBeneath = view->FindTabControlForPosition(GetColPosition() - 1, GetRowPosition() + 1);
      if (tabBeneath && tabBeneath->IsSelected())
        subtractThis = (view->GetTabSelectionHeight() - view->GetTabHeight());
    
      // Draw only to next tab down.
      dc->DrawLine((float)(tabX + GetWidth()), (float)tabY,
         (float)(tabX + GetWidth()), (float)(tabY + GetHeight() + tabHeightInc - subtractThis));

      // Draw black line to emphasize shadow
      dc->SetPen(wxBLACK_PEN);
      dc->DrawLine((float)(tabX + GetWidth() + 1), (float)(tabY+1), (float)(tabX + GetWidth() + 1),
         (float)(tabY + GetHeight() + tabHeightInc - subtractThis));
    }
  }
  
  // Draw centered text
  int textY = tabY + view->GetVerticalTabTextSpacing() + tabHeightInc;

  if (isSelected)
    dc->SetFont(view->GetSelectedTabFont());
  else
    dc->SetFont(GetFont());
    
  dc->SetTextForeground(&(view->GetTextColour()));
  dc->SetBackgroundMode(wxTRANSPARENT);
  float textWidth, textHeight;
  dc->GetTextExtent(GetLabel().GetData(), &textWidth, &textHeight);
  
  int textX = (int)(tabX + (GetWidth() - textWidth)/2.0);
  dc->DrawText(GetLabel().GetData(), (float)textX, (float)textY);
  
  if (isSelected)
  {
    dc->SetPen(view->GetHighlightPen());

    // Draw white highlight from the tab's left side to the left hand edge of the view
    dc->DrawLine((float)view->GetViewRect().x, (float)(tabY + GetHeight() + tabHeightInc),
     (float)tabX, (float)(tabY + GetHeight() + tabHeightInc));
  
    // Draw white highlight from the tab's right side to the right hand edge of the view
    dc->DrawLine((float)(tabX + GetWidth()), (float)(tabY + GetHeight() + tabHeightInc),
     (float)view->GetViewRect().x + view->GetViewRect().width, (float)(tabY + GetHeight() + tabHeightInc));
  }
}

Bool wxTabControl::HitTest(int x, int y)
{
  // Top-left of tab control
  int tabX1 = GetX() + view->GetViewRect().x;
  int tabY1 = GetY() + view->GetViewRect().y;
  
  // Bottom-right
  int tabX2 = tabX1 + GetWidth();
  int tabY2 = tabY1 + GetHeight();
  
  if (x >= tabX1 && y >= tabY1 && x <= tabX2 && y <= tabY2)
    return TRUE;
  else
    return FALSE;
}

IMPLEMENT_DYNAMIC_CLASS(wxTabView, wxObject)

wxTabView::wxTabView(long style)
{
  tabStyle = style;
  tabSelection = -1;
  tabHeight = 20;
  tabSelectionHeight = 24;
  tabWidth = 80;
  tabHorizontalOffset = 10;
  tabHorizontalSpacing = 2;
  tabVerticalTextSpacing = 3;
  topMargin = 5;
  tabViewRect.x = 20;
  tabViewRect.y = 20;
  tabViewRect.width = 300;
  tabViewRect.x = 300;
  highlightColour = *wxWHITE;
  shadowColour = wxColour(128, 128, 128);
  backgroundColour = *wxLIGHT_GREY;
  textColour = *wxBLACK;
  highlightPen = wxWHITE_PEN;
  shadowPen = wxGREY_PEN;
  backgroundPen = wxLIGHT_GREY_PEN;
  backgroundBrush = wxLIGHT_GREY_BRUSH;
  tabFont = wxTheFontList->FindOrCreateFont(11, wxSWISS, wxNORMAL, wxNORMAL);
  tabSelectedFont = wxTheFontList->FindOrCreateFont(11, wxSWISS, wxNORMAL, wxBOLD);
  tabDC = NULL;
  layers.DeleteContents(TRUE);
}

wxTabView::~wxTabView()
{
}
  
// Automatically positions tabs
wxTabControl *wxTabView::AddTab(int id, const wxString& label)
{
  // First, find which layer we should be adding to.
  wxNode *node = layers.Last();
  if (!node)
  {
    wxTabLayer *newLayer = new wxTabLayer;
    node = layers.Append(newLayer);
  }
  // Check if adding another tab control would go off the
  // right-hand edge of the layer.
  wxTabLayer *tabLayer = (wxTabLayer *)node->Data();
  wxNode *lastTabNode = tabLayer->Last();
  if (lastTabNode)
  {
    wxTabControl *lastTab = (wxTabControl *)lastTabNode->Data();
    // Start another layer (row)
    if ((lastTab->GetX() + lastTab->GetWidth() + GetHorizontalTabSpacing()) > GetViewRect().width)
    {
      tabLayer = new wxTabLayer;
      layers.Append(tabLayer);
      lastTabNode = NULL;
    }
  }
  int layer = layers.Number() - 1;
  
  wxTabControl *tabControl = OnCreateTabControl();
  tabControl->SetRowPosition(tabLayer->Number());
  tabControl->SetColPosition(layer);
  
  wxTabControl *lastTab = NULL;
  if (lastTabNode)
    lastTab = (wxTabControl *)lastTabNode->Data();
  
  // Top of new tab
  int verticalOffset = (- GetTopMargin()) - ((layer+1)*GetTabHeight());
  // Offset from view top-left
  int horizontalOffset = 0;
  if (!lastTab)
    horizontalOffset = layer*GetHorizontalTabOffset();
  else
    horizontalOffset = lastTab->GetX() + GetTabWidth() + GetHorizontalTabSpacing();
  
  tabControl->SetPosition(horizontalOffset, verticalOffset);
  tabControl->SetSize(GetTabWidth(), GetTabHeight());
  tabControl->SetId(id);
  tabControl->SetLabel(label);
  tabControl->SetFont(GetTabFont());
  
  tabLayer->Append(tabControl);
  
  return tabControl;
}
  
void wxTabView::ClearTabs()
{
  layers.Clear();
}
  
// Draw all tabs
void wxTabView::Draw(void)
{
  if (!GetDC())
    return;
  
  GetDC()->BeginDrawing();
  
  // Draw layers in reverse order
  wxNode *node = layers.Last();
  while (node)
  {
    wxTabLayer *layer = (wxTabLayer *)node->Data();
    wxNode *node2 = layer->First();
    while (node2)
    {
      wxTabControl *control = (wxTabControl *)node2->Data();
      control->OnDraw(GetDC(), (node2->Next() == NULL));
      node2 = node2->Next();
    }
    
    node = node->Previous();
  }

  if (GetTabStyle() & wxTAB_STYLE_DRAW_BOX)
  {
    GetDC()->SetPen(GetShadowPen());

    // Draw bottom line
    GetDC()->DrawLine((float)(GetViewRect().x+1), (float)(GetViewRect().y + GetViewRect().height - 1),
      (float)(GetViewRect().x + GetViewRect().width), (float)(GetViewRect().y + GetViewRect().height - 1));

    // Draw right line
    GetDC()->DrawLine((float)(GetViewRect().x + GetViewRect().width), (float)(GetViewRect().y - GetTopMargin() + 1),
      (float)(GetViewRect().x + GetViewRect().width), (float)(GetViewRect().y + GetViewRect().height - 1));

    GetDC()->SetPen(wxBLACK_PEN);

    // Draw bottom line
    GetDC()->DrawLine((float)(GetViewRect().x), (float)(GetViewRect().y + GetViewRect().height),
      (float)(GetViewRect().x + GetViewRect().width+1), (float)(GetViewRect().y + GetViewRect().height));

    // Draw right line
    GetDC()->DrawLine((float)(GetViewRect().x + GetViewRect().width + 1), (float)(GetViewRect().y - GetTopMargin()),
      (float)(GetViewRect().x + GetViewRect().width + 1), (float)(GetViewRect().y + GetViewRect().height + 1));
  }
  
  GetDC()->EndDrawing();
}
  
// Process mouse event, return FALSE if we didn't process it
Bool wxTabView::OnEvent(wxMouseEvent& event)
{
  if (!event.LeftDown())
    return FALSE;
    
  float x, y;
  event.Position(&x, &y);
  
  wxTabControl *hitControl = NULL;
  
  wxNode *node = layers.First();
  while (node)
  {
    wxTabLayer *layer = (wxTabLayer *)node->Data();
    wxNode *node2 = layer->First();
    while (node2)
    {
      wxTabControl *control = (wxTabControl *)node2->Data();
      if (control->HitTest((int)x, (int)y))
      {
        hitControl = control;
        node = NULL;
        node2 = NULL;
      }
      else
        node2 = node2->Next();
    }
  
    if (node)
      node = node->Next();
  }
  
  if (!hitControl)
    return FALSE;
    
  wxTabControl *currentTab = FindTabControlForId(tabSelection);
    
  if (hitControl == currentTab)
    return FALSE;
    
  ChangeTab(hitControl);
  
  return TRUE;
}

Bool wxTabView::ChangeTab(wxTabControl *control)
{
  wxTabControl *currentTab = FindTabControlForId(tabSelection);
  int oldTab = -1;
  if (currentTab)
    oldTab = currentTab->GetId();
  
  if (control == currentTab)
    return TRUE;
    
  if (layers.Number() == 0)
    return FALSE;
    
  if (!OnTabPreActivate(control->GetId(), oldTab))
    return FALSE;
    
  wxTabLayer *firstLayer = (wxTabLayer *)layers.First()->Data();
    
  // Find what column this tab is at, so we can swap with the one at the bottom.
  // If we're on the bottom layer, then no need to swap.
  if (!firstLayer->Member(control))
  {
    // Do a swap
    int col = 0;
    wxNode *thisNode = FindTabNodeAndColumn(control, &col);
    if (!thisNode)
      return FALSE;
    wxNode *otherNode = firstLayer->Nth(col);
    if (!otherNode)
      return FALSE;
     wxTabControl *otherTab = (wxTabControl *)otherNode->Data();
     
     // We now have pointers to the tab to be changed to,
     // and the tab on the first layer. Swap tab structures and
     // position details.
     
     int thisX = control->GetX();
     int thisY = control->GetY();
     int thisColPos = control->GetColPosition();
     int otherX = otherTab->GetX();
     int otherY = otherTab->GetY();
     int otherColPos = otherTab->GetColPosition();
     
     control->SetPosition(otherX, otherY);
     control->SetColPosition(otherColPos);
     otherTab->SetPosition(thisX, thisY);
     otherTab->SetColPosition(thisColPos);
     
     // Swap the data for the nodes
     thisNode->SetData(otherTab);
     otherNode->SetData(control);
  }
  if (currentTab)
  {
    currentTab->SetSelected(FALSE);
  }
    
  control->SetSelected(TRUE);
  tabSelection = control->GetId();

  OnTabActivate(control->GetId(), oldTab);
  
  // Leave window refresh for the implementing window

  return TRUE;
}
  
// Called when a tab is activated
void wxTabView::OnTabActivate(int activateId, int deactivateId)
{
}
  
void wxTabView::SetHighlightColour(const wxColour& col)
{
  highlightColour = col;
  highlightPen = wxThePenList->FindOrCreatePen((wxColour *)&col, 1, wxSOLID);
}

void wxTabView::SetShadowColour(const wxColour& col)
{
  shadowColour = col;
  shadowPen = wxThePenList->FindOrCreatePen((wxColour *)&col, 1, wxSOLID);
}

void wxTabView::SetBackgroundColour(const wxColour& col)
{
  backgroundColour = col;
  backgroundPen = wxThePenList->FindOrCreatePen((wxColour *)&col, 1, wxSOLID);
  backgroundBrush = wxTheBrushList->FindOrCreateBrush((wxColour *)&col, wxSOLID);
}

void wxTabView::SetTabSelection(int sel)
{
  int oldSel = tabSelection;
  wxTabControl *control = FindTabControlForId(sel);

  if (!OnTabPreActivate(sel, oldSel))
    return;
    
  if (control)
    control->SetSelected(sel);
  else
  {
    wxMessageBox("Could not find tab for id", "Error", wxOK);
    return;
  }
    
  tabSelection = sel;
  OnTabActivate(sel, oldSel);
}

// Find tab control for id
wxTabControl *wxTabView::FindTabControlForId(int id)
{
  wxNode *node1 = layers.First();
  while (node1)
  {
    wxTabLayer *layer = (wxTabLayer *)node1->Data();
    wxNode *node2 = layer->First();
    while (node2)
    {
      wxTabControl *control = (wxTabControl *)node2->Data();
      if (control->GetId() == id)
        return control;
      node2 = node2->Next();
    }
    node1 = node1->Next();
  }
  return NULL;
}

// Find tab control for layer, position (starting from zero)
wxTabControl *wxTabView::FindTabControlForPosition(int layer, int position)
{
  wxNode *node1 = layers.Nth(layer);
  if (!node1)
    return NULL;
  wxTabLayer *tabLayer = (wxTabLayer *)node1->Data();
  wxNode *node2 = tabLayer->Nth(position);
  if (!node2)
    return NULL;
  return (wxTabControl *)node2->Data();
}

// Find the node and the column at which this control is positioned.
wxNode *wxTabView::FindTabNodeAndColumn(wxTabControl *control, int *col)
{
  wxNode *node1 = layers.First();
  while (node1)
  {
    wxTabLayer *layer = (wxTabLayer *)node1->Data();
    int c = 0;
    wxNode *node2 = layer->First();
    while (node2)
    {
      wxTabControl *cnt = (wxTabControl *)node2->Data();
      if (cnt == control)
      {
        *col = c;
        return node2;
      }
      node2 = node2->Next();
      c ++;
    }
    node1 = node1->Next();
  }
  return NULL;
}

int wxTabView::CalculateTabWidth(int noTabs, Bool adjustView)
{
  tabWidth = (int)((tabViewRect.width - ((noTabs - 1)*GetHorizontalTabSpacing()))/noTabs);
  if (adjustView)
  {
    tabViewRect.width = noTabs*tabWidth + ((noTabs-1)*GetHorizontalTabSpacing());
  }
  return tabWidth;
}

/*
 * wxTabbedDialogBox
 */
 
IMPLEMENT_CLASS(wxTabbedDialogBox, wxDialogBox)

wxTabbedDialogBox::wxTabbedDialogBox(wxWindow *parent, Const char *title, Bool modal, int x, int y,
     int width, int height, long windowStyle, Constdata char *name):
   wxDialogBox(parent, title, modal, x, y, width, height, windowStyle, name)
{
  tabView = NULL;
}

wxTabbedDialogBox::~wxTabbedDialogBox(void)
{
  if (tabView)
    delete tabView;
}
 
Bool wxTabbedDialogBox::OnClose(void)
{
  return TRUE;
}

void wxTabbedDialogBox::OnEvent(wxMouseEvent& event)
{
  if (tabView)
    tabView->OnEvent(event);
}

void wxTabbedDialogBox::OnPaint(void)
{
  if (tabView)
    tabView->Draw();
}

/*
 * wxTabbedPanel
 */
 
IMPLEMENT_CLASS(wxTabbedPanel, wxPanel)

wxTabbedPanel::wxTabbedPanel(wxWindow *parent, int x, int y,
     int width, int height, long windowStyle, Constdata char *name):
   wxPanel(parent, x, y, width, height, windowStyle, name)
{
  tabView = NULL;
}

wxTabbedPanel::~wxTabbedPanel(void)
{
  delete tabView;
}
 
void wxTabbedPanel::OnEvent(wxMouseEvent& event)
{
  if (tabView)
    tabView->OnEvent(event);
}

void wxTabbedPanel::OnPaint(void)
{
  if (tabView)
    tabView->Draw();
}

/*
 * wxDialogTabView
 */
 
IMPLEMENT_CLASS(wxPanelTabView, wxTabView)
 
wxPanelTabView::wxPanelTabView(wxPanel *pan, long style): wxTabView(style), tabPanels(wxKEY_INTEGER)
{
  panel = pan;
  currentPanel = NULL;

  if (panel->IsKindOf(CLASSINFO(wxTabbedDialogBox)))
    ((wxTabbedDialogBox *)panel)->SetTabView(this);
  else if (panel->IsKindOf(CLASSINFO(wxTabbedPanel)))
    ((wxTabbedPanel *)panel)->SetTabView(this);

  SetDC(panel->GetDC());
}

wxPanelTabView::~wxPanelTabView(void)
{
  ClearPanels(TRUE);
}

// Called when a tab is activated
void wxPanelTabView::OnTabActivate(int activateId, int deactivateId)
{
  if (!panel)
    return;
    
  wxPanel *oldPanel = ((deactivateId == -1) ? (wxPanel *)NULL : GetTabPanel(deactivateId));
  wxPanel *newPanel = GetTabPanel(activateId);

  if (oldPanel)
    oldPanel->Show(FALSE);
  if (newPanel)
    newPanel->Show(TRUE);
    
  panel->Refresh();
}

   
void wxPanelTabView::AddTabPanel(int id, wxPanel *panel)
{
  tabPanels.Append((long)id, panel);
  panel->Show(FALSE);
}

wxPanel *wxPanelTabView::GetTabPanel(int id)
{
  wxNode *node = tabPanels.Find((long)id);
  if (!node)
    return NULL;
  return (wxPanel *)node->Data();    
}

void wxPanelTabView::ClearPanels(Bool deletePanels)
{
  if (deletePanels)
    tabPanels.DeleteContents(TRUE);
  tabPanels.Clear();
  tabPanels.DeleteContents(FALSE);
}

void wxPanelTabView::ShowPanelForTab(int id)
{
  wxPanel *newPanel = GetTabPanel(id);
  if (newPanel == currentPanel)
    return;
  if (currentPanel)
    currentPanel->Show(FALSE);
  newPanel->Show(TRUE);
  newPanel->Refresh();
}

