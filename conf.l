
%option yylineno noyywrap

%{

#include "conf.tab.h"

%}
						
%%

#[^\n]*\n { /* ignore */       }
[ \t]+    { /* ignore */       }
\n        { return EOL;        }
device    { return TOKDEVICE;  }
driver    { return TOKDRIVER;  }
serial    { return TOKSERIAL;  }
usb       { return TOKUSB;     }
ir        { return TOKIR;      }
.         { return *yytext;    }

\"[^\"\n]+\" { yylval.sval = yytext; return STRING; }
'[^'\n]+'    { yylval.sval = yytext; return STRING; }
