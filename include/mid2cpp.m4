define(FILE_HEADER, `')
define(FILE_FOOTER,`')

define(INCLUDE,`#include $1')
define(CLASSINCLUDE,`#include $1')

define(INTERFACE,`/* Interface $1*/
define(PREFIX,$1)
class PREFIX`'Impl : public PREFIX
{
public:
$2
};

static PREFIX`'Impl gs_`'PREFIX;')

define(STRING, const wxChar *)
define(STRINGA, const char *)
define(INT, long int)
define(UIDTYPE, UIdType)
define(ARG, $1 $2)
define(FUNCTION, `
virtual $2 $1 $3
{
$4
}
')

