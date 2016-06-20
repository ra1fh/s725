/* conf.l - configuration file parser */

/*
 * Copyright (C) 2016  Ralf Horstmann
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

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
irda      { return TOKIRDA;      }
format    { return TOKFORMAT;    }
hrm       { return TOKHRM;       }
srd       { return TOKSRD;       }
tcx       { return TOKTCX;       }
txt       { return TOKTXT;       }
.         { return *yytext;      }

\"[^\"\n]+\" { yylval.sval = yytext; return STRING; }
'[^'\n]+'    { yylval.sval = yytext; return STRING; }
