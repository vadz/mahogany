/*
 * $Id$
 *
 * M4 definitions for converting .mid to ,idl and .h
*/

m4_define(INTERFACE,`interface $1
{
$2
};')

m4_define(STRING, string)
m4_define(INT, int)
m4_define(ARG, in $1 $2)
m4_define(FUNCTION, $2 $1 `(' `)')
m4_define(BODY,`;')

