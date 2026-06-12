#pragma once

#undef yyFlexLexer
#define yyFlexLexer p6_scanner_FlexLexer
#include <FlexLexer.h>

typedef p6_scanner_FlexLexer p6_scanner;

