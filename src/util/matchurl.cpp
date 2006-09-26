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
   int scanAtStart(const wxChar* toBeScanned);

   /** Scans the given string to find a keyword.
     Returns the starting position of the first
     keyword in the string.
     lng is the length of the longest keyword that
     starts in this position.
     computeBackArcs must have been called before the
     first call to this function.
    */
   int scan(const wxChar* toBeScanned, int& lng) {
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
   int scanAtStart(const wxChar* toBeScanned,
         KeywordDetectorCell* current,
         int longueurDejaVue, int lngDernierTrouve);
   int scan(const wxChar* toBeScanned, int& lng, KeywordDetectorCell* current);

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
   int FindURL(const wxChar *str, int& len);
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
KeywordDetector::scanAtStart(const wxChar* toBeScanned,
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


int KeywordDetector::scanAtStart(const wxChar* toBeScanned)
{
   return scanAtStart(toBeScanned, _root, 0, 0);
}


int KeywordDetector::scan(const wxChar* toBeScanned,
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
static inline bool IsAlnum(unsigned char c)
{
   // normally URLs should be in plain ASCII (7bit) but in practice some broken
   // programs (people?) apparently write them using 8bit chars too so be
   // liberal here and suppose that any 8 bit char can be part of the URL as
   // there is no way for us to check it more precisely...
   //
   // OTOH we still don't use alnum() here because we don't want to depend on
   // the current locale
   return (c >= 'a' && c <= 'z') ||
          (c >= 'A' && c <= 'Z') ||
          (c >= '0' && c <= '9') ||
          (c > 0x7f);
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
   return IsAlnum(c) || IsURLMark(c) || IsURLReserved(c) || c == '%' || c == '#' ||
          c == '[' || c == ']';
}

/// check if this is this atext as defined in RFC 2822
static inline bool IsATextChar(char c)
{
   // VZ: '|' should appear here too normally but I'm too tired of seeing
   //     Mahogany highlighting URLs improperly in Bugzilla reports (where
   //     an email address is always shown in '|'-separated columns), so
   //     I removed it
   return IsAlnum(c) || strchr("!#$%&'*+-/=?^_`{}~", c);
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
   addNewKeyword("http://");
   addNewKeyword("https://");
   addNewKeyword("mailto://");
   addNewKeyword("ftp://");
   addNewKeyword("sftp://");
   addNewKeyword("file://");
   // addNewKeyword("ftps:"); -- does anyone really uses this?

   // also detect some common URLs even without the scheme part
   addNewKeyword("www.");
   addNewKeyword("ftp.");

   // finally detect the email addresses
   addNewKeyword("@");

   computeBackArcs();
}

/*
   When we're called, p points to the "\r\n" and so p+2 is the start of the
   next line. What we try to do here is to detect the case when there is an
   extension somewhere near the end of the line -- if it's incomplete, we can
   be (almost) sure that the URL continues on the next line. OTOH, if we have
   the full extension here, chances are that the URL is not wrapped.
 */
static bool CanBeWrapped(const wxChar *p)
{
   // we consider any alphanumeric string of 3 characters an extension
   // but we have separate arrays of known extensions of other lengths
   static const wxChar *extensions1 =
   {
      // this one is actually a string and not an array as like this we can use
      // strchr() below
      _T("cCfhZ"),
   };

   static const wxChar *extensions2[] =
   {
      _T("cc"), _T("gz"),
   };

   static const wxChar *extensions4[] =
   {
      _T("html"), _T("jpeg"), _T("tiff"),
   };

   if ( !IsAlnum(p[-1]) )
   {
      // can't be part of an extension, so, by default, consider that the URL
      // can be wrapped
      return true;
   }

   if ( p[-2] == '.' )
   {
      if ( wxStrchr(extensions1, p[-1]) )
      {
         // we seem to have a one letter extension at the end
         return false;
      }
   }
   else if ( !IsAlnum(p[-2]) )
   {
      // as above -- we don't know, assume it can be wrapped
      return true;
   }
   else if ( p[-3] == '.' )
   {
      for ( size_t n = 0; n < WXSIZEOF(extensions2); n++ )
      {
         if ( p[-2] == extensions2[n][0] &&
               p[-1] == extensions2[n][1] )
         {
            // line ends with a full 2 letter extension
            return false;
         }
      }
   }
   else if ( !IsAlnum(p[-3]) )
   {
      // as above -- we don't know, assume it can be wrapped
      return true;
   }
   else if ( p[-4] == '.' )
   {
      for ( size_t n = 0; n < WXSIZEOF(extensions4); n++ )
      {
         const wxChar * const ext = extensions4[n];
         if ( wxStrncmp(p - 3, ext, 3) == 0 && p[2] == ext[3] )
         {
            // looks like a long extension got wrapped
            return true;
         }
      }

      // it doesn't look that extension is continued on the next line,
      // consider this to be the end of the URL
      return false;
   }
   else if ( p[-5] == '.' )
   {
      for ( size_t n = 0; n < WXSIZEOF(extensions4); n++ )
      {
         if ( wxStrncmp(p - 4, extensions4[n], 4) == 0 )
         {
            // line ends with an extension, shouldn't wrap normally
            return false;
         }
      }

      return true;
   }
   else // no periods at all anywhere in sight
   {
      return true;
   }

   // we get here if we had a period near the end of the string but we didn't
   // recognize the extension following it -- so try to understand whether this
   // is a wrapped extension by checking if the next char is an alnum one
   return IsAlnum(p[2]);
}

int URLDetector::FindURL(const wxChar *text, int& len)
{
   // offset of the current value of text from the initial one
   int offset = 0;

match:
   int pos = scan(text, len);
   if ( !len )
      return -1;

   // the provisional start and end of the URL, will be changed below
   const wxChar *start = text + pos;
   const wxChar *p = start + len;

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
         size_t lenURL = 0;
         while ( IsURLChar(*p) )
         {
            lenURL++;
            p++;
         }

         // URLs are frequently so long that they're spread across multiple
         // lines, so try to see if this might be the case here
         //
         // first of all: is it at the end of line and can it be continued on
         // the next one? also check if it's really long enough to be wrapped:
         // the short URLs normally shouldn't be wrapped
         static const size_t URL_WRAP_LEN = 30; // min len of wrapped URL
         if ( p[0] != '\r' || p[1] != '\n'
               || lenURL < URL_WRAP_LEN
               || !IsURLChar(p[2]) )
         {
            // it isn't
            break;
         }

         // heuristic text for end of URL detection
         if ( p - start > 5 && !CanBeWrapped(p) )
         {
            // it seems that the URL ends here
            break;
         }

         p += 2; // go to the start of next line

         // Check that the beginning of next line is not the start of
         // another URL.
         //
         // Note that although '@' alone is recognized as the beginning
         // of an URL: here it should not be the case.
         int nextlen = 0;
         int nextpos = scan(p, nextlen);
         if ( nextlen && nextpos == 0 && *p != '@')
         {
            p -= 2;

            // The start of the next line being the start of an URL on its own,
            // do not join the two.
            break;
         }

         // check whether the next line starts with a word -- this is a good
         // indication that the URL hasn't wrapped
         const wxChar *q = p;
         while ( wxIsalpha(*q) )
            q++;

         if ( *q == _T(' ') || (wxStrchr(_T(".,:;"), *q) && q[1] == _T(' ')) )
         {
            // looks like we've a word (i.e. sequence of letters terminated by
            // space or punctuation) at the start of the next line
            p -= 2;
            break;
         }

         // it might be a wrapped URL but it might be not: it seems like we
         // get way too many false positives if we suppose that it's always
         // the case... so restrict the wrapped URLs detection to the case
         // when they occur at the beginning of the line, possibly after some
         // white space as this is how people usually format them
         q = start;
         while ( q >= text && *q != '\n' )
         {
            q--;

            if ( !wxIsspace(*q) )
               break;
         }

         // Does the URL start at the beginning of the line, or does it have
         // a '<' just in front?
         if ( q >= text && *q != '\n' && *q != '<')
            break;

         // it did occur at the start (or after '<'), suppose the URL is
         // wrapped and so we continue on the next line (and no need to test
         // the first character, it had been already done above)
         p++;
      }
   }

   // truncate any punctuation at the end
   while ( strchr(".:,)]!?", *(p - 1)) )
      p--;

   // additional checks for the matches which didn't have an explicit scheme
   if ( isMail || text[pos + len - 3 /* len of "://" */ ] != _T(':') )
   {
      // '@' matches may result in false positives, as not every '@' character
      // is inside a mailto URL so try to weed them out by requiring that the
      // mail address has a reasonable minimal length ("ab@cd.fr" and
      // "www.xy.fr" are probably the shortest ones we can have, hence 8)
      // which at least avoids matching the bare '@'s
      bool good = (p - start) >= 8;

      if ( good )
      {
         // also check that we have at least one dot in the domain part for the
         // mail addresses
         const char *
            pDot = (char *)memchr(text + pos + 1, '.', p - text - pos - 1);
         if ( !pDot )
         {
            good = false;
         }
         else if ( !isMail )
         {
            // and has either two dots or at least a slash the other URLs,
            // otherwise it probably isn't an address/URL neither (stuff like
            // "... using ftp.If you ... " shouldn't be recognized as an URL)
            good = memchr(pDot + 1, '.', p - pDot - 1) != NULL ||
                     memchr(pDot + 1, '/', p - pDot - 1) != NULL;
         }
      }

      if ( !good )
      {
         const int offDiff = pos + len;
         offset += offDiff;
         text += offDiff;

         // slightly more efficient than recursion...
         goto match;
      }
   }

   // return the length of the match
   len = p - start;

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

extern
int FindURL(const wxChar *s, int& len)
{
   static URLDetector s_detector;

   return s_detector.FindURL(s, len);
}

