define(FILE_HEADER, `
#ifndef _$1_MID_H_
#define _$1_MID_H_')
define(FILE_FOOTER,`
#endif
')

define(INCLUDE,`#include $1')
define(CLASSINCLUDE,`$2')
define(INTERFACE,`/* Interface $1*/
define(PREFIX,$1)
class PREFIX
{
public:
$2
};')

define(STRING, const wxChar *)
define(STRINGA, const char *)
define(INT, long int)
define(UIDTYPE, UIdType)
define(ARG, $1 $2 $3)
define(FUNCTION, `
virtual $2 $1 $3 = 0;
')

