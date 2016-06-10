
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
stir      { return TOKSTIR;      }
irda      { return TOKIRDA;      }
format    { return TOKFORMAT;    }
hrm       { return TOKHRM;       }
srd       { return TOKSRD;       }
tcx       { return TOKTCX;       }
txt       { return TOKTXT;       }
.         { return *yytext;      }

\"[^\"\n]+\" { yylval.sval = yytext; return STRING; }
'[^'\n]+'    { yylval.sval = yytext; return STRING; }
