/*
 * $Id$
 *
 * M4 definitions for converting .mid to ,idl and .h
*/

define(INTERFACE,`interface $1
{
$2
};')

define(STRING, string)
define(INT, long)
define(UIDTYPE, unsigned long)
define(ARG, in $1 $2)
define(FUNCTION, $2 $1 $3;)

define(FILE_HEADER,`')
define(FILE_FOOTER,`')
define(INCLUDE,`#include $1')
define(CLASSINCLUDE,`$include $1')
