define(FILE_HEADER, `')
define(FILE_FOOTER,`')

define(INCLUDE,`#include $1')
define(CLASSINCLUDE,`$2')
define(INTERFACE,`/* Interface $1*/
define(PREFIX,$1)
class PREFIX
{
public:
$2
};')

define(STRING, const char *)
define(INT, int)
define(ARG, $1 $2 $3)
define(FUNCTION, `
virtual $2 $1 $3 = 0;
')

