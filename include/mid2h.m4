/*
 * $Id$
 *
 * M4 definitions for converting .mid to ,idl and .h
*/
m4_define(m4_concat,`$1$2')

m4_define(INTERFACE,`/* Interface $1*/
{
m4_define(PREFIX,$1)
$2
}')


m4_define(STRING, char *)
m4_define(INT, int)
m4_define(ARG, const $1 $2)
m4_define(FUNCTION, $2 PREFIX``_''$1)))
m4_define(BODY, `{
$1
}')
