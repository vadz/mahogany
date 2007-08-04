#include <wx/init.h>
#include <wx/string.h>

typedef wxString String;

#include "mail/MimeDecode.h"

extern "C" {

void *fs_get (size_t size) { return malloc(size); }
void fs_resize (void **p, size_t size) { *p = realloc(*p, size); }
void fs_give (void **p) { free(*p); *p = NULL; }

char *cpystr (const char *string)
{
  return string ? strcpy ((char *) fs_get (1 + strlen (string)),string) : NULL;
}

void mm_log (char *string,long errflg)
{
    printf("mm_log[%ld]: %s\n", errflg, string);
}

void fatal (char *string)
{
    mm_log(string, -1);
    abort();
}

#define WARN (long) 1		/* mm_log warning type */
#define ERROR (long) 2		/* mm_log error type */
#define PARSE (long) 3		/* mm_log parse error type */
#define BYE (long) 4		/* mm_notify stream dying */

#define NIL 0			/* convenient name */

#define MAILTMPLEN 1024		/* size of a temporary buffer */

/* Convert two hex characters into byte
 * Accepts: char for high nybble
 *	    char for low nybble
 * Returns: byte
 *
 * Arguments must be isxdigit validated
 */

unsigned char hex2byte (unsigned char c1,unsigned char c2)
{
				/* merge the two nybbles */
  return ((c1 -= (isdigit (c1) ? '0' : ((c1 <= 'Z') ? 'A' : 'a') - 10)) << 4) +
    (c2 - (isdigit (c2) ? '0' : ((c2 <= 'Z') ? 'A' : 'a') - 10));
}
/* Convert QUOTED-PRINTABLE contents to 8BIT
 * Accepts: source
 *	    length of source
 * 	    pointer to return destination length
 * Returns: destination as 8-bit text or NIL if error
 */

unsigned char *rfc822_qprint (unsigned char *src,unsigned long srcl,
			      unsigned long *len)
{
  char tmp[MAILTMPLEN];
  unsigned int bogon = NIL;
  unsigned char *ret = (unsigned char *) fs_get ((size_t) srcl + 1);
  unsigned char *d = ret;
  unsigned char *t = d;
  unsigned char *s = src;
  unsigned char c,e;
  *len = 0;			/* in case we return an error */
				/* until run out of characters */
  while (((unsigned long) (s - src)) < srcl) {
    switch (c = *s++) {		/* what type of character is it? */
    case '=':			/* quoting character */
      if (((unsigned long) (s - src)) < srcl) switch (c = *s++) {
      case '\0':		/* end of data */
	s--;			/* back up pointer */
	break;
      case '\015':		/* non-significant line break */
	if ((((unsigned long) (s - src)) < srcl) && (*s == '\012')) s++;
      case '\012':		/* bare LF */
	t = d;			/* accept any leading spaces */
	break;
      default:			/* two hex digits then */
	if (!(isxdigit (c) && (((unsigned long) (s - src)) < srcl) &&
	      (e = *s++) && isxdigit (e))) {
	  /* This indicates bad MIME.  One way that it can be caused is if
	     a single-section message was QUOTED-PRINTABLE encoded and then
	     something (e.g. a mailing list processor) appended text.  The
	     problem is that there is no way to determine where the encoded
	     data ended and the appended crud began.  Consequently, prudent
	     software will always encapsulate a QUOTED-PRINTABLE segment
	     inside a MULTIPART.
	   */
	  if (!bogon++) {	/* only do this once */
	    sprintf (tmp,"Invalid quoted-printable sequence: =%.80s",
		   (char *) s - 1);
	    mm_log (tmp,PARSE);
	  }
	  *d++ = '=';		/* treat = as ordinary character */
	  *d++ = c;		/* and the character following */
	  t = d;		/* note point of non-space */
	  break;
	}
	*d++ = hex2byte (c,e);	/* merge the two hex digits */
	t = d;			/* note point of non-space */
	break;
      }
      break;
    case ' ':			/* space, possibly bogus */
      *d++ = c;			/* stash the space but don't update s */
      break;
    case '\015':		/* end of line */
    case '\012':		/* bare LF */
      d = t;			/* slide back to last non-space, drop in */
    default:
      *d++ = c;			/* stash the character */
      t = d;			/* note point of non-space */
    }      
  }
  *d = '\0';			/* tie off results */
  *len = d - ret;		/* calculate length */
  return ret;			/* return the string */
}
/* Convert BASE64 contents to binary
 * Accepts: source
 *	    length of source
 *	    pointer to return destination length
 * Returns: destination as binary or NIL if error
 */

#define WSP 0176		/* NUL, TAB, LF, FF, CR, SPC */
#define JNK 0177
#define PAD 0100

void *rfc822_base64 (unsigned char *src,unsigned long srcl,unsigned long *len)
{
  char c,*s,tmp[MAILTMPLEN];
  void *ret = fs_get ((size_t) ((*len = 4 + ((srcl * 3) / 4))) + 1);
  char *d = (char *) ret;
  int e;
  static char decode[256] = {
   WSP,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,WSP,WSP,JNK,WSP,WSP,JNK,JNK,
   JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,
   WSP,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,076,JNK,JNK,JNK,077,
   064,065,066,067,070,071,072,073,074,075,JNK,JNK,JNK,PAD,JNK,JNK,
   JNK,000,001,002,003,004,005,006,007,010,011,012,013,014,015,016,
   017,020,021,022,023,024,025,026,027,030,031,JNK,JNK,JNK,JNK,JNK,
   JNK,032,033,034,035,036,037,040,041,042,043,044,045,046,047,050,
   051,052,053,054,055,056,057,060,061,062,063,JNK,JNK,JNK,JNK,JNK,
   JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,
   JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,
   JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,
   JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,
   JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,
   JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,
   JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,
   JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK,JNK
  };
				/* initialize block */
  memset (ret,0,((size_t) *len) + 1);
  *len = 0;			/* in case we return an error */

				/* simple-minded decode */
  for (e = 0; srcl--; ) switch (c = decode[*src++]) {
  default:			/* valid BASE64 data character */
    switch (e++) {		/* install based on quantum position */
    case 0:
      *d = c << 2;		/* byte 1: high 6 bits */
      break;
    case 1:
      *d++ |= c >> 4;		/* byte 1: low 2 bits */
      *d = c << 4;		/* byte 2: high 4 bits */
      break;
    case 2:
      *d++ |= c >> 2;		/* byte 2: low 4 bits */
      *d = c << 6;		/* byte 3: high 2 bits */
      break;
    case 3:
      *d++ |= c;		/* byte 3: low 6 bits */
      e = 0;			/* reinitialize mechanism */
      break;
    }
    break;
  case WSP:			/* whitespace */
    break;
  case PAD:			/* padding */
    switch (e++) {		/* check quantum position */
    case 3:			/* one = is good enough in quantum 3 */
				/* make sure no data characters in remainder */
      for (; srcl; --srcl) switch (decode[*src++]) {
				/* ignore space, junk and extraneous padding */
      case WSP: case JNK: case PAD:
	break;
      default:			/* valid BASE64 data character */
	/* This indicates bad MIME.  One way that it can be caused is if
	   a single-section message was BASE64 encoded and then something
	   (e.g. a mailing list processor) appended text.  The problem is
	   that in 1 out of 3 cases, there is no padding and hence no way
	   to detect the end of the data.  Consequently, prudent software
	   will always encapsulate a BASE64 segment inside a MULTIPART.
	   */
	sprintf (tmp,"Possible data truncation in rfc822_base64(): %.80s",
		 (char *) src - 1);
	if (s = strpbrk (tmp,"\015\012")) *s = NIL;
	mm_log (tmp,PARSE);
	srcl = 1;		/* don't issue any more messages */
	break;
      }
      break;
    case 2:			/* expect a second = in quantum 2 */
      if (srcl && (*src == '=')) break;
    default:			/* impossible quantum position */
      fs_give (&ret);
      return NIL;
    }
    break;
  case JNK:			/* junk character */
    fs_give (&ret);
    return NIL;
  }
  *len = d - (char *) ret;	/* calculate data length */
  *d = '\0';			/* NUL terminate just in case */
  return ret;			/* return the string */
}
}

int main()
{
    wxInitializer init;

    String s;

    s = MIME::DecodeHeader("=?koi8-r?B?79TXxdTZIM7BINfP0NLP09k=?=");
    s = MIME::DecodeHeader("=?koi8-r?B?99jA1sHOyc4g68/O09TBztTJziBcKENvbnN0YW50aW5lIFZ5dXpoYW5pblwp?= <vycon@udmnet.ru>");
    s = MIME::DecodeHeader("Ludovic =?ISO-8859-1?Q?P=E9net?= <ludovic@penet.org>");
    printf(s.utf8_str());

    return 0;
}

