
%option yylineno noyywrap

%{

#include "conf.tab.h"

%}
						
%%

#[^\n]*\n { /* ignore */         }
[ \t]+    { /* ignore */         }
\n        { return EOL;          }
device    { return TOKDEVICE;    }
driver    { return TOKDRIVER;    }
directory { return TOKDIRECTORY; }
serial    { return TOKSERIAL;    }
ir        { return TOKIR;        }
stir      { return TOKSTIR;      }
.         { return *yytext;      }

\"[^\"\n]+\" { yylval.sval = yytext; return STRING; }
'[^'\n]+'    { yylval.sval = yytext; return STRING; }
