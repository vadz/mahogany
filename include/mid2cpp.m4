define(FILE_HEADER, `')
define(FILE_FOOTER,`')

define(INCLUDE,`#include $1')

define(INTERFACE,`/* Interface $1*/
define(PREFIX,$1)
$2
')

define(STRING, const char *)
define(INT, int)
define(ARG, $1 $2)
define(FUNCTION, `
$2 PREFIX::$1 $3
{
$4
}
')

