/*-*- c++ -*-********************************************************
 * wxFTCanvas: a canvas for editing formatted text                  *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$       *
 *******************************************************************/

/*
  - each Object knows its size and how to draw itself
  - the list is responsible for calculating positions
  - the draw coordinates for each object are the top left corner
  - coordinates only get calculated when things get redrawn
  - during redraw each line gets iterated over twice, first just
    calculating baselines and positions, second to actually draw it
  - the cursor position is the position before an object, i.e. if the
    buffer starts with a text-object, cursor 0,0 is just before the
    first character
*/

#ifdef __GNUG__
#pragma implementation "wxllist.h"
#endif

#include   "wxllist.h"
#include   "iostream"

#define   VAR(x)   cout << #x"=" << x << endl;
#define   DBG_POINT(p)   cout << #p << ": " << p.x << ',' << p.y << endl
#define   TRACE(f)   cout << #f":" << endl;

#ifdef DEBUG
static const char *_t[] = { "invalid", "text", "cmd", "icon",
                             "linebreak"};

void
wxLObjectBase::Debug(void)
{
   CoordType bl = 0;
   cerr << _t[GetType()] << ": size=" << GetSize(&bl).x << ","
        << GetSize(&bl).y << " bl=" << bl; 
}
#endif

//-------------------------- wxLObjectText

wxLObjectText::wxLObjectText(const wxString &txt)
{
   m_Text = txt;
   m_Width = 0;
   m_Height = 0;
}


wxPoint
wxLObjectText::GetSize(CoordType *baseLine) const
{
   if(baseLine) *baseLine = m_BaseLine;
   return wxPoint(m_Width, m_Height);
}

void
wxLObjectText::Draw(wxDC &dc, wxPoint position, CoordType baseLine,
                    bool draw)
{
   long descent = 0l;
   dc.GetTextExtent(m_Text,&m_Width, &m_Height, &descent);
   //FIXME: wxGTK does not set descent to a descent value yet.
   if(descent == 0)
      descent = (2*m_Height)/10;  // crude fix
   m_BaseLine = m_Height - descent;
   position.y += baseLine-m_BaseLine;
   if(draw)
      dc.DrawText(m_Text,position.x,position.y);
#   ifdef   DEBUG
//   dc.DrawRectangle(position.x, position.y, m_Width, m_Height);
#   endif
}

#ifdef DEBUG
void
wxLObjectText::Debug(void)
{
   wxLObjectBase::Debug();
   cerr << " `" << m_Text << '\'';
}
#endif

//-------------------------- wxLObjectIcon

wxLObjectIcon::wxLObjectIcon(wxIcon *icon)
{
   m_Icon = icon;
}

void
wxLObjectIcon::Draw(wxDC &dc, wxPoint position, CoordType baseLine,
                    bool draw)
{
   position.y += baseLine - m_Icon->GetHeight();
   if(draw)
      dc.DrawIcon(m_Icon,position.x,position.y);
}

wxPoint
wxLObjectIcon::GetSize(CoordType *baseLine) const
{
   wxASSERT(baseLine);
   *baseLine = m_Icon->GetHeight();
   return wxPoint(m_Icon->GetWidth(), m_Icon->GetHeight());
}

//-------------------------- wxLObjectCmd


wxLObjectCmd::wxLObjectCmd(int size, int family, int style, int
                           weight, bool underline,
                           wxColour const *fg, wxColour const *bg)
   
{
   m_font = new wxFont(size,family,style,weight,underline);
   m_ColourFG = fg;
   m_ColourBG = bg;
}

wxLObjectCmd::~wxLObjectCmd()
{
   delete m_font;
}


void
wxLObjectCmd::Draw(wxDC &dc, wxPoint position, CoordType lineHeight,
                   bool draw)
{
   wxASSERT(m_font);
   // this get called even when draw==false, so that recalculation
   // uses right font sizes
   dc.SetFont(m_font);
   if(m_ColourFG)
      dc.SetTextForeground(*m_ColourFG);
   if(m_ColourBG)
      dc.SetTextBackground(*m_ColourBG);
}

//-------------------------- wxwxLayoutList

wxLayoutList::wxLayoutList()
{
   Clear();
}

wxLayoutList::~wxLayoutList()
{
}


void
wxLayoutList::LineBreak(void)
{
   Insert(new wxLObjectLineBreak);
   m_CursorPosition.x = 0; m_CursorPosition.y++;
}

void
wxLayoutList::SetFont(int family, int size, int style, int weight,
                      int underline, wxColour const *fg,
                      wxColour const *bg)
{
   if(family != -1)    m_FontFamily = family;
   if(size != -1)      m_FontPtSize = size;
   if(style != -1)     m_FontStyle = style;
   if(weight != -1)    m_FontWeight = weight;
   if(underline != -1) m_FontUnderline = underline;

   if(fg != NULL)     m_ColourFG = fg;
   if(bg != NULL)     m_ColourBG = bg;
   
   Insert(
      new wxLObjectCmd(m_FontPtSize,m_FontFamily,m_FontStyle,m_FontWeight,m_FontUnderline,
                       m_ColourFG, m_ColourBG));
}

void
wxLayoutList::SetFont(int family, int size, int style, int weight,
                      int underline, char const *fg, char const *bg)
{
   wxColour const
      * cfg = NULL,
      * cbg = NULL;

   if( fg )
      cfg = wxTheColourDatabase->FindColour(fg);
   if( bg )
      cbg = wxTheColourDatabase->FindColour(bg);
   
   SetFont(family,size,style,weight,underline,cfg,cbg);
}


/// for access by wxLayoutWindow:
void
wxLayoutList::GetSize(CoordType *max_x, CoordType *max_y,
                      CoordType *lineHeight)
{
   wxASSERT(max_x); wxASSERT(max_y); wxASSERT(lineHeight);
   *max_x = m_MaxX;
   *max_y = m_MaxY;
   *lineHeight = m_LineHeight;
}

wxLObjectBase *
wxLayoutList::Draw(wxDC &dc, bool findObject, wxPoint const &findCoords)
{
   wxLObjectList::iterator i;

   // in case we need to look for an object
   wxLObjectBase *foundObject = NULL;
   
   // first object in current line
   wxLObjectList::iterator headOfLine;

   // do we need to recalculate current line?
   bool recalculate = false;

   // do we calculate or draw? Either true or false.
   bool draw = false;
   // drawing parameters:
   wxPoint position = wxPoint(0,0);
   wxPoint position_HeadOfLine;
   CoordType baseLine = m_FontPtSize;
   CoordType baseLineSkip = (12 * baseLine)/10;

   // we trace the objects' cursor positions so we can draw the cursor
   wxPoint cursor = wxPoint(0,0);
   // the cursor position inside the object
   CoordType cursorOffset = 0;
   // object under cursor
   wxLObjectList::iterator cursorObject = FindCurrentObject(&cursorOffset);
   
   // queried from each object:
   wxPoint       size = wxPoint(0,0);
   CoordType     objBaseLine = baseLine;
   wxLObjectType type;

   VAR(findObject); VAR(findCoords.x); VAR(findCoords.y);
   // if the cursorobject is a cmd, we need to find the first
   // printable object:
   while(cursorObject != end()
         && (*cursorObject)->GetType() == LO_CMD)
      cursorObject++;

   headOfLine = begin();
   position_HeadOfLine = position;

   // setting up the default:
   dc.SetTextForeground( *wxBLACK );
   dc.SetFont( *wxNORMAL_FONT );

   // we calculate everything for drawing a line, then rewind to the
   // begin of line and actually draw it
   i = begin();
   for(;;)
   {
      recalculate = false;

      if(i == end())
         break;
      type = (*i)->GetType();

      // to initialise sizes of objects, we need to call Draw
      (*i)->Draw(dc, position, baseLine, draw);

      // update coordinates for next object:
      size = (*i)->GetSize(&objBaseLine);
      if(findObject && draw)  // we need to look for an object
      {
         if(findCoords.y >= position.y
            && findCoords.y <= position.y+size.y
            && findCoords.x >= position.x
            && findCoords.x <= position.x+size.x)
         {
            foundObject = *i;
            findObject = false; // speeds things up
         }
      }
      // draw the cursor
      if(m_Editable && draw && i == cursorObject)
      {
         if(type == LO_TEXT) // special treatment
         {
            long descent = 0l; long width, height;
            wxLObjectText *tobj = (wxLObjectText *)*i;
            wxString  str = tobj->GetText();
            VAR(m_CursorPosition.x); VAR(cursor.x);
            str = str.substr(0, cursorOffset);
            VAR(str);
            dc.GetTextExtent(str, &width,&height, &descent);
            VAR(height);
            VAR(width); VAR(descent);
            dc.DrawLine(position.x+width,
                        position.y+(baseLineSkip-height),
                        position.x+width, position.y+baseLineSkip);
         }
         else
         {
            if(type == LO_LINEBREAK)
                dc.DrawLine(0, position.y+baseLineSkip, 0, position.y+2*baseLineSkip);
            else
            {
               if(size.x == 0)
               {
                  if(size.y == 0)
                     dc.DrawLine(position.x, position.y, position.x, position.y+baseLineSkip);
                  else
                     dc.DrawLine(position.x, position.y, position.x, position.y+size.y);
               }
               else
                  dc.DrawRectangle(position.x, position.y, size.x, size.y);
            }
         }
      }

      // calculate next object's position:
      position.x += size.x;
      
      // do we need to increase the line's height?
      if(size.y > baseLineSkip)
      {
         baseLineSkip = size.y;
         recalculate = true;
      }
      if(objBaseLine > baseLine)
      {
         baseLine = objBaseLine;
         recalculate = true;
      }

      // now check whether we have finished handling this line:
      if(type == LO_LINEBREAK || i == tail())
      {
         if(recalculate)  // do this line again
         {
            position.x = position_HeadOfLine.x;
            i = headOfLine;
            continue;
         }

         if(! draw) // finished calculating sizes
         {  // do this line again, this time drawing it
            position = position_HeadOfLine;
            draw = true;
            i = headOfLine;
            continue;
         }
         else // we have drawn a line, so continue calculating next one
            draw = false;
      }
      
      // is it a linebreak?
      if(type == LO_LINEBREAK || i == tail())
      {
         position.x = 0;
         position.y += baseLineSkip;
         baseLine = m_FontPtSize;
         baseLineSkip = (12 * baseLine)/10;
         headOfLine = i;
         headOfLine++;
         position_HeadOfLine = position;
      }
      i++;
   }
   m_MaxX = position.x;
   m_MaxY = position.y;
   return foundObject;
}

#ifdef DEBUG
void
wxLayoutList::Debug(void)
{
   CoordType               offs;
   wxLObjectList::iterator i;

   cerr <<
      "------------------------debug start-------------------------" << endl;
   for(i = begin(); i != end(); i++)
   {
      (*i)->Debug();
      cerr << endl;
   }
   cerr <<
      "-----------------------debug end----------------------------"
        << endl;
   // show current object:
   cerr << "Cursor: "
        << m_CursorPosition.x << ','
        << m_CursorPosition.y;
   
   i = FindCurrentObject(&offs);
   cerr << " line length: " << GetLineLength(i) << "  ";
   if(i == end())
   {
      cerr << "<<no object found>>" << endl;
      return;  // FIXME we should set cursor position to maximum allowed
      // value then
   }
   if((*i)->GetType() == LO_TEXT)
   {
      cerr << " \"" << ((wxLObjectText *)(*i))->GetText() << "\", offs: "
           << offs << endl;
   }
   else
      cerr << ' ' << _t[(*i)->GetType()] << endl;

}
#endif

/******************** editing stuff ********************/

wxLObjectList::iterator 
wxLayoutList::FindObjectCursor(wxPoint const &cpos, CoordType *offset)
{
   wxPoint cursor = wxPoint(0,0);  // runs along the objects
   CoordType width;
   wxLObjectList::iterator i;

#ifdef DEBUG
   cerr << "Looking for object at " << cpos.x << ',' << cpos.y <<
      endl;
#endif
   for(i = begin(); i != end() && cursor.y <= cpos.y; i++)
   {
      width = 0;
      if((*i)->GetType() == LO_LINEBREAK)
      {
         if(cpos.y == cursor.y)
         {
            --i;
            if(offset)
               *offset = (*i)->CountPositions();
            return i;
         }
         cursor.x = 0; cursor.y ++;
      }
      else
         cursor.x += (width = (*i)->CountPositions());
      if(cursor.y == cpos.y && (cursor.x > cpos.x ||
                                ((*i)->GetType() != LO_CMD && cursor.x == cpos.x))
         ) // found it ?   
      {
         if(offset)
            *offset = cpos.x-(cursor.x-width);  // 0==cursor before
         // the object
#ifdef DEBUG
         cerr << "   found object at " << cursor.x-width << ',' <<
            cursor.y << ", type:" << _t[(*i)->GetType()] <<endl;
#endif   
         return i;
      }
   }
#ifdef DEBUG
   cerr << "   not found" << endl;
#endif
   return end(); // not found
}

wxLObjectList::iterator 
wxLayoutList::FindCurrentObject(CoordType *offset)
{
   wxLObjectList::iterator obj = end();

   obj = FindObjectCursor(m_CursorPosition, offset);
   if(obj == end())  // not ideal yet
   {
      obj = tail();
      if(obj != end()) // tail really exists
         *offset = (*obj)->CountPositions(); // at the end of it
   }
   return obj;
}

void
wxLayoutList::MoveCursor(int dx, int dy)
{
   CoordType offs, lineLength;
   wxLObjectList::iterator i;


   if(dy > 0 && m_CursorPosition.y < m_MaxLine)
      m_CursorPosition.y += dy;
   else if(dy < 0 && m_CursorPosition.y > 0)
      m_CursorPosition.y += dy; // dy is negative
   if(m_CursorPosition.y < 0)
      m_CursorPosition.y = 0;
   else if (m_CursorPosition.y > m_MaxLine)
      m_CursorPosition.y = m_MaxLine;
   
   while(dx > 0)
   {
      i = FindCurrentObject(&offs);
      lineLength = GetLineLength(i);
      if(m_CursorPosition.x < lineLength)
      {
         m_CursorPosition.x ++;
         dx--;
         continue;
      }
      else
      {
         if(m_CursorPosition.y < m_MaxLine)
         {
            m_CursorPosition.y++;
            m_CursorPosition.x = 0;
            dx--;
         }
         else
            break; // cannot move there
      }
   }
   while(dx < 0)
   {
      if(m_CursorPosition.x > 0)
      {
         m_CursorPosition.x --;
         dx++;
      }
      else
      {
         if(m_CursorPosition.y > 0)
         {
            m_CursorPosition.y --;
            m_CursorPosition.x = 0;
            i = FindCurrentObject(&offs);
            lineLength = GetLineLength(i);
            m_CursorPosition.x = lineLength;
            dx++;
            continue;
         }
         else
            break; // cannot move left any more
      }
   }
   // final adjustment:
   i = FindCurrentObject(&offs);
   lineLength = GetLineLength(i);
   if(m_CursorPosition.x > lineLength)
      m_CursorPosition.x = lineLength;
      
#ifdef   DEBUG
   i = FindCurrentObject(&offs);
   cerr << "Cursor: "
        << m_CursorPosition.x << ','
        << m_CursorPosition.y;

   if(i == end())
   {
      cerr << "<<no object found>>" << endl;
      return;  // FIXME we should set cursor position to maximum allowed
      // value then
   }
   if((*i)->GetType() == LO_TEXT)
   {
      cerr << " \"" << ((wxLObjectText *)(*i))->GetText() << "\", offs: "
           << offs << endl;
   }
   else
      cerr << ' ' << _t[(*i)->GetType()] << endl;
#endif
}

void
wxLayoutList::Delete(CoordType count)
{
   TRACE(Delete);

   if(!m_Editable)
      return;

   VAR(count);

   CoordType offs, len;
   wxLObjectList::iterator i;
      
   do
   {
      i  = FindCurrentObject(&offs);
      if(i == end())
         return;
#ifdef DEBUG
      cerr << "trying to delete: " << _t[(*i)->GetType()] << endl;
#endif
      if((*i)->GetType() == LO_LINEBREAK)
         m_MaxLine--;
      if((*i)->GetType() == LO_TEXT)
      {
         wxLObjectText *tobj = (wxLObjectText *)*i;
         len = tobj->CountPositions();
         // If we find the end of a text object, this means that we
         // have to delete from the object following it.
         if(offs == len)
         {
            i++;
            if((*i)->GetType() == LO_TEXT)
            {
               offs = 0;  // delete from begin of next string
               tobj = (wxLObjectText *)*i;
               len = tobj->CountPositions();
            }
            else
            {
               erase(i);
               return;
            }
         }
         if(len <= count) // delete this object
         {
            count -= len;
            erase(i);
         }
         else
         {
            len = count;
            VAR(offs); VAR(len);
            tobj->GetText().erase(offs,len);
            return; // we are done
         }
      }
      else // delete the object
      {
         len = (*i)->CountPositions();
         erase(i); // after this, i is the iterator for the following object
         if(count > len)
            count -= len;
         else
            count = 0;
      }
   }
   while(count && i != end());      
}

void
wxLayoutList::Insert(wxLObjectBase *obj)
{
   wxASSERT(obj);
   CoordType offs;
   wxLObjectList::iterator i = FindCurrentObject(&offs);

   TRACE(Insert(obj));

   if(i == end())
      push_back(obj);
   else
   {      
      // do we have to split a text object?
      if((*i)->GetType() == LO_TEXT && offs != 0 && offs != (*i)->CountPositions())
      {
         wxLObjectText *tobj = (wxLObjectText *) *i;
#ifdef DEBUG
         cerr << "text: '" << tobj->GetText() << "'" << endl;
         VAR(offs);
#endif
         wxString left = tobj->GetText().substr(0,offs); // get part before cursor
         VAR(left);
         tobj->GetText() = tobj->GetText().substr(offs,(*i)->CountPositions()-offs); // keeps the right half
         VAR(tobj->GetText());
         insert(i,obj);
         insert(i,new wxLObjectText(left)); // inserts before
      }
      else
      {
         wxLObjectList::iterator j = i; // we want to apend after this object
         j++;
         if(j != end())
            insert(j, obj);
         else
            push_back(obj);
      }  
   }
   m_CursorPosition.x += obj->CountPositions();
   if(obj->GetType() == LO_LINEBREAK)
      m_MaxLine++;
}

void
wxLayoutList::Insert(wxString const &text)
{
   TRACE(Insert(text));

   if(! m_Editable)
      return;

   CoordType offs;
   wxLObjectList::iterator i = FindCurrentObject(&offs);

   if(i != end() && (*i)->GetType() == LO_TEXT)
   {  // insert into an existing text object:
      TRACE(inserting into existing object);
      wxLObjectText *tobj = (wxLObjectText *)*i;
      tobj->GetText().insert(offs,text);
   }
   else
   {
      // check whether the previous object is text:
      wxLObjectList::iterator j = i;
      j--;
      TRACE(checking previous object);
      if(0 && j != end() && (*j)->GetType() == LO_TEXT)
      {
         wxLObjectText *tobj = (wxLObjectText *)*i;
         tobj->GetText()+=text;
      }
      else  // insert a new text object
      {
         TRACE(creating new object);
         Insert(new wxLObjectText(text));  //FIXME not too optimal, slow
         return;  // position gets incremented in Insert(obj)
      }
   }
   m_CursorPosition.x += strlen(text.c_str());
}

CoordType
wxLayoutList::GetLineLength(wxLObjectList::iterator i)
{
   if(i == end())
      return 0;

   CoordType len = 0;

   // search backwards for beginning of line:
   while(i != begin() && (*i)->GetType() != LO_LINEBREAK)
      i--;
   if((*i)->GetType() == LO_LINEBREAK)
      i++;
   // now we can start counting:
   while(i != end() && (*i)->GetType() != LO_LINEBREAK)
   {
      len += (*i)->CountPositions();
      i++;
   }
   return len;
}

void
wxLayoutList::Clear(void)
{
   wxLObjectList::iterator i = begin();

   while(i != end()) // == while valid
      erase(i);

   // set defaults
   m_FontPtSize = 12;
   m_FontUnderline = false;
   m_FontFamily = wxDEFAULT;
   m_FontStyle = wxNORMAL;
   m_FontWeight = wxNORMAL;
   m_ColourFG = wxTheColourDatabase->FindColour("BLACK");
   m_ColourBG = wxTheColourDatabase->FindColour("WHITE");

   m_Position = wxPoint(0,0);
   m_CursorPosition = wxPoint(0,0);
   m_MaxLine = 0;
   m_LineHeight = (12*m_FontPtSize)/10;
   m_MaxX = 0; m_MaxY = 0;
}
