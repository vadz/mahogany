m4_define(FILE_HEADER, `#ifdef __cplusplus
extern "C" {
#endif')
m4_define(FILE_FOOTER,`#ifdef __cplusplus
}
#endif
')

m4_define(INCLUDE,`#include $1')
m4_define(m4_concat,`$1$2')

m4_define(INTERFACE,`/* Interface $1*/
m4_define(PREFIX,$1)
$2
')

m4_define(STRING, char *)
m4_define(INT, int)
m4_define(ARG, const $1 $2)
m4_define(FUNCTION, `
#ifdef MINTERFACE_IMPLEMENTATION
extern $2 PREFIX`'_$1 $3
{
$4
}
#else
typedef $2 (* PREFIX`'_$1_Type) $3;
PREFIX`'_$1_Type PREFIX`'_$1 = (PREFIX`'_$1_Type)MModule_GetSymbol(`"'PREFIX_$1`"');
#endif')
m4_define(VARIABLE, `extern $2 PREFIX`'_$1;')

