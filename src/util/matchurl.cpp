///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   util/matchurl.cpp - matching URLs in text
// Purpose:     implements Aho-Cora algorithm and uses it for URL matching
// Author:      Xavier Nodet (core), Vadim Zeitlin (specialization to URLs)
// Modified by:
// Created:     25.04.02
// CVS-ID:      $Id$
// Copyright:   (c) 2002 Mahogany Team
// Licence:     Mahogany license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
   #include "Mcommon.h"
   #include "strutil.h"
#endif

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

/// KeywordDetectorCell represents a single keyword letter with links from it
class KeywordDetectorCell
{
public:
   KeywordDetectorCell(char c) :
      _c(c), _son(NULL), _brother(NULL), _isKey(0), _back(NULL) {}

   ~KeywordDetectorCell()
   {
      if (_c != '\000')
      {
         delete _son;
         delete _brother;
      } // else: this is a back-node, and thus it points up in the
      // tree, to a node that is already deleted, or will soon be.
   }

private:
   char _c;
   KeywordDetectorCell* _son;
   KeywordDetectorCell* _brother;
   int _isKey;
   KeywordDetectorCell* _back;

   KeywordDetectorCell* computeBackArc(KeywordDetectorCell* root,
         KeywordDetectorCell* parentBack);
   void computeBackArcs(KeywordDetectorCell* root,
         KeywordDetectorCell* parentBack);

   friend class KeywordDetector;
};


/**
  * Keywords scanner
  */
class KeywordDetector
{
public:
   KeywordDetector() : _root(NULL) { }
   ~KeywordDetector() { delete _root; }

public:
   /// Adds a new keyword to the list of detected keywords
   void addNewKeyword(const char* key);

   /** Returns the length of the longest keyword
     starting at the beginning of the string given as parameter.
     Returns 0 if no keyword found or not at the beginning of
     the string.
     This method does not need that computeBackArcs has been called.
     Keywords can be added between calls to this method.
    */
   int scanAtStart(const char* toBeScanned);

   /** Scans the given string to find a keyword.
     Returns the starting position of the first
     keyword in the string.
     lng is the length of the longest keyword that
     starts in this position.
     computeBackArcs must have been called before the
     first call to this function.
    */
   int scan(const char* toBeScanned, int& lng) {
      return scan(toBeScanned, lng, _root);
   }

   /** Does all the precomputations needed after all the
     keywords have been added, and before the first call
     to scan.
    */
   void computeBackArcs() {
      _root->computeBackArcs(_root, 0);
   }

private:
   void addNewKeyword(const char* key,
         KeywordDetectorCell* current,
         int toBeAdded = 1);
   int scanAtStart(const char* toBeScanned,
         KeywordDetectorCell* current,
         int longueurDejaVue, int lngDernierTrouve);
   int scan(const char* toBeScanned, int& lng, KeywordDetectorCell* current);

   KeywordDetectorCell* _root;
};

/// URLDetector simply uses KeywordDetector to detect specifically the URLs
class URLDetector : public KeywordDetector
{
public:
   URLDetector();

   /**
     Finds the first URL in the string.

     @param str the string in which we're looking for the URLs
     @param len the length of the match is returned here, no match => len = 0
     @return the position of the first match or -1 if nothing found
    */
   int FindURL(const char *str, int& len);
};

// ============================================================================
// KeywordDetector implementation
// ============================================================================

void KeywordDetector::addNewKeyword(const char* key)
{
   //
   // Ajout d'un mot-clef dans l'arbre
   //
   if (key && (strlen(key) > 0))
      addNewKeyword(key, _root);
}


void KeywordDetector::addNewKeyword(const char* key,
                                    KeywordDetectorCell* current,
                                    int toBeAdded)
{
   if (! current)
   {
      // This is the first keyword inserted
      // We must create adn save the root of the tree
      KeywordDetectorCell* newCell = new KeywordDetectorCell(key[0]);
      _root = newCell;
      addNewKeyword(key, _root);
      return;
   }

   if (current->_c == key[0])
   {
      // The key is correct: this is the correct cell
      if (key[1] == '\000')
      {
         // And we have reached the end of a keyword
         // Donc la cellule courante marque la fin
         // d'un mot-clef
         current->_isKey = toBeAdded;
      }
      else
      {
         // The keyword to add (or remove) is not finished
         if (! current->_son) {
            // No son yet. We have to create one
            KeywordDetectorCell* newCell = new KeywordDetectorCell(key[1]);
            current->_son = newCell;
         }
         addNewKeyword(key+1, current->_son);
      }
   }
   else
   {
      // This is not the correct cell
      if (! current->_brother)
      {
         // No brother. Let's create one
         KeywordDetectorCell* newCell = new KeywordDetectorCell(key[0]);
         current->_brother = newCell;
      }

      addNewKeyword(key, current->_brother);
   }
}


int
KeywordDetector::scanAtStart(const char* toBeScanned,
                             KeywordDetectorCell* current,
                             int longueurDejaVue,
                             int lngDernierTrouve)
{
   if (! current)
      return lngDernierTrouve;

   if (current->_c == toBeScanned[0])
   {
      // This is the correct cell
      if (current->_isKey == 1)
      {
         // And this is the end of a keyword
         // let's try to find a longer one, but do
         // remember that we already saw one, the length
         // of which is longueurDejaVue+1
         return scanAtStart(toBeScanned+1,
               current->_son,
               longueurDejaVue+1,
               longueurDejaVue+1);
      }
      else
      {
         // This is not the end of a keyword
         // Go on
         CHECK( current->_son, -1, _T("current cell must have a son") );
         return scanAtStart(toBeScanned+1, current->_son, longueurDejaVue+1, lngDernierTrouve);
      }
   }
   else
   {
      // Not a correct cell
      if (! current->_brother)
      {
         // And no more cell. Let's return the length
         // we may have already found
         return lngDernierTrouve;
      }
      else
      {
         // There is another cell to try
         return scanAtStart(toBeScanned, current->_brother, longueurDejaVue, lngDernierTrouve);
      }
   }
}


int KeywordDetector::scanAtStart(const char* toBeScanned)
{
   return scanAtStart(toBeScanned, _root, 0, 0);
}


int KeywordDetector::scan(const char* toBeScanned,
                          int& lng, KeywordDetectorCell* current)
{
   int currentPosition = 0;
   lng = 0;
   bool atRootLevel = true;
   while (toBeScanned[0])
   {
      if (current->_c == toBeScanned[0])
      {
         // This is the correct cell
         if (current->_isKey == 1)
         {
            // And the end of a keyword.
            // let's try to find a longer one, but do
            // remember that we already saw one, the length
            // of which is longueurDejaVue+1

            lng =  scanAtStart(toBeScanned+1,
                  current->_son,
                  lng+1,
                  lng+1);
            return currentPosition;
         }
         else
         {
            // This is not the end of a keyword
            // Go on
            CHECK( current->_son, -1, _T("current cell must have a son") );
            current = current->_son;
            atRootLevel = false;
            lng++;
            toBeScanned = toBeScanned + 1;
         }
      }
      else
      {
         // Not a correct cell
         current = current->_brother;
         if (!current)
         {
            current = _root;
            if (atRootLevel)
            {
               toBeScanned++;
               if ( !lng )
                  lng = 1;
            }
            atRootLevel = true;
            currentPosition = currentPosition + lng;
            lng = 0;
         }
         else if (current->_c == '\000')
         {
            // This is a 'back-link'
            currentPosition = currentPosition + lng - current->_isKey;
            lng = current->_isKey;
            ASSERT_MSG( lng > 0, _T("length must be non 0") );
            current = current->_son;
         }
      }
   }
   lng = 0;
   return 0;
}

KeywordDetectorCell *
KeywordDetectorCell::computeBackArc(KeywordDetectorCell* root,
                                    KeywordDetectorCell* parentBack)
{
   ASSERT_MSG(parentBack == 0 || parentBack->_c == '\000',
              _T("logic error in KeywordDetectorCell?"));

   KeywordDetectorCell* back = 0;
   KeywordDetectorCell* current = 0;
   int backLevel = 0;
   if (! parentBack)
   {
      // Parent node has no back arc. So we look for
      // nextLetter in root and its siblings
      current = root;
      backLevel = 1;
   }
   else
   {
      // Parent has a back node. So we look for nextLetter
      // in *the sons* of this back node
      current = parentBack->_son;
      backLevel = parentBack->_isKey + 1;
   }

   while (current)
   {
      if (current->_c == _c)
      {
         back = current;
         break;
      }

      current = current->_brother;
   }

   if (current)
   {
      // We have found our back node
      // Find our last son
      KeywordDetectorCell* lastSon = _son;

      // There must be a son, otherwise the current node would be a keyword
      // and there is no need for a back node
      CHECK(lastSon, 0, _T("node with a back link must have a son"));

      while (lastSon->_brother)
      {
         lastSon = lastSon->_brother;
      }

      // A cell with key '\000' is points to the back node
      KeywordDetectorCell* newCell = new KeywordDetectorCell('\000');
      newCell->_son = current->_son;
      newCell->_isKey = backLevel;
      lastSon->_brother = newCell;
      return newCell;
   }

   return 0;
}

void
KeywordDetectorCell::computeBackArcs(KeywordDetectorCell* root,
                                     KeywordDetectorCell* parentBack)
{
   KeywordDetectorCell* parent = this;
   bool onRootLevel = (parent == root ? true : false);
   while (parent)
   {
      if (parent->_c == '\000')
         break;

      KeywordDetectorCell* back = 0;
      if (! (onRootLevel || parent->_isKey))
      {
         back = parent->computeBackArc(root, parentBack);
      }

      KeywordDetectorCell* son = parent->_son;
      if (son)
      {
         son->computeBackArcs(root, back);
      }

      parent = parent->_brother;
   }
}

// ============================================================================
// URLDetector implementation
// ============================================================================

/// a locale-independent isalnum()
static inline bool IsAlnum(char c)
{
   // we do *not* use isalnum() as we want to be locale-independent
   return (c >= 'a' && c <= 'z') ||
          (c >= 'A' && c <= 'Z') ||
          (c >= '0' && c <= '9');
}

/// checks a character to be a 'mark' as from RFC2396
inline bool IsURLMark(char c)
{
   return c == '-' || c == '_' || c == '.' || c == '!' || c == '~' ||
          c == '*' || c == '\'' || c == '(' || c == ')';
}

/// checks a character to be 'reserved' as from RFC2396
inline bool IsURLReserved(char c)
{
   return c == ';' || c == '/' || c == '?' || c == ':' || c == '@' ||
          c == '&' || c == '=' || c == '+' || c == '$' || c == ',';
}

/// checks a character to be a valid part of an URL
inline bool IsURLChar(char c)
{
   return IsAlnum(c) || IsURLMark(c) || IsURLReserved(c) || c == '%' || c == '#';
}

/// check if this is this atext as defined in RFC 2822
static inline bool IsATextChar(char c)
{
   return IsAlnum(c) || strchr("!#$%&'*+-/=?^_`{|}~", c);
}

/// check if the character is valid in the personal part of an address
static inline bool IsLocalPartChar(char c)
{
   // we don't support quoted local parts here
   return IsATextChar(c) || (c == '.');
}

/// check if the character is valid in the domain part of an address
static inline bool IsDomainChar(char c)
{
   // we don't really support the domain literals but we still include '[' and
   // ']' just in case
   return IsLocalPartChar(c) || c == '[' || c == ']';
}

URLDetector::URLDetector()
{
   // we detect a few common URL schemes
   addNewKeyword("http:");
   addNewKeyword("https:");
   addNewKeyword("mailto:");
   addNewKeyword("ftp:");
   addNewKeyword("file:");
   // addNewKeyword("ftps:"); -- does anyone really uses this?

   // also detect some common URLs even without the scheme part
   addNewKeyword("www.");
   addNewKeyword("ftp.");

   // finally detect the email addresses
   addNewKeyword("@");

   computeBackArcs();
}

int URLDetector::FindURL(const char *text, int& len)
{
   int offset = 0;

match:
   int pos = scan(text, len);
   if ( !len )
      return -1;

   // the provisional start and end of the URL, will be changed below
   const char *start = text + pos;
   const char *p = start + len;

   // there are 2 different cases: a mailto: URL or a mail address and
   // anything else which we need to treat differently
   bool isMail = *start == '@';

   if ( isMail )
   {
      // look for the start of the address
      start--;
      while ( start > text && IsLocalPartChar(*start) )
         start--;

      // have we stopped at '<'?
      bool hasAngleBracket = *start == '<';
      if ( !hasAngleBracket )
      {
         if ( !IsLocalPartChar(*start) )
         {
            // we went too far backwards
            start++;
         }
         //else: we stopped at the start of the text
      }
      //else: keep '<' as part of the URL

      // now look for the end of it
      while ( *p && IsDomainChar(*p) )
      {
         p++;
      }

      // finally we should either have the brackets from both sides or none
      // at all
      if ( hasAngleBracket )
      {
         if ( *p == '>' )
         {
            // take the right bracket as well
            p++;
         }
         else
         {
            // forget about the left one
            start++;
         }
      }
   }
   else // !bare mail address
   {
      for ( ;; )
      {
         while ( IsURLChar(*p) )
            p++;

         // URLs are frequently so long that they're spread across multiple
         // lines, so try to see if this might be the case here
         if ( p[0] != '\r' || p[1] != '\n' || !IsURLChar(p[2]) )
         {
            // it isn't
            break;
         }

         // even with all the checks below we still get too many false
         // positives so consider that only "long" URLs are wrapped where long
         // URLs are defined as the ones containing the CGI script parameters
         // or some '%' chars (i.e. escaped characters)
         if ( strcspn(start + len, "%?&\r") == (size_t)(p - start - len) )
         {
            // no CGI parameters, suppose it can't wrap
            break;
         }

         // Check that the beginning of next line is not the start of
         // another URL.
         //
         // Note that although '@' alone is recognized as the beginning
         // of an URL: here it should not be the case.
         int nextlen = 0;
         int nextpos = scan(p + 2, nextlen);
         if ( nextlen && nextpos == 0 && p[2] != '@')
         {
            // The start of the next line being the start of an URL on its own,
            // do not join the two.
            break;
         }

         // it might be a wrapped URL but it might be not: it seems like we
         // get way too many false positives if we suppose that it's already
         // the case... so restrict the wrapped URLs detection to the case
         // when they occur at the beginning of the line, possibly after some
         // white space as this is how people usually format them
         const char *q = start;
         while ( q >= text && *q != '\n' )
         {
            if ( !isspace(*q--) )
               break;
         }

         // Does the URL start at the beginning of the line, or does it have
         // a '<' just in front?
         if ( q >= text && *q != '\n' && *q != '<')
            break;

         // it did occur at the start (or after '<'), suppose the URL is wrapped and so
         // continue on the next line (no need to test the first character,
         // it had been already done above)
         p += 3;
      }
   }

   // truncate any punctuation at the end
   while ( strchr(".:,;)", *(p - 1)) )
      p--;

   len = p - start;

   // '@' matches may result in false positives, as not every '@' character
   // is inside a mailto URL so try to weed them out by requiring that the
   // mail address has a reasonable minimal length ("ab@foo.com" is probably
   // the shortest we can have, hence 10) which at least avoids matching the
   // bare '@'s
   //
   // also check that we have at least one dot in the domain part, otherwise
   // it probably isn't an address neither
   if ( (len < 10) || !memchr(text + pos + 1, '.', p - text - pos - 1) )
   {
      int offDiff = pos + len + 1;
      offset += offDiff;
      text += offDiff;

      // slightly more efficient than recursion...
      goto match;
   }

   return start - text + offset;
}

// ============================================================================
// public API implementation
// ============================================================================

String
strutil_findurl(String& str, String& url)
{
   static URLDetector s_detector;

   // the return value: the part of the text before the URL
   String before;

   int len;
   int pos = s_detector.FindURL(str, len);

   if ( !len )
   {
      // no URLs found
      str.swap(before);
      str.clear();
      url.clear();
   }
   else // found an URL
   {
      before = str.substr(0, pos);
      url = str.substr(pos, len);

      str.erase(0, pos + len);
   }

   return before;
}

