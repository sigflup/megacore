
O   [0-7]
D   [0-9]
NZ  [1-9]
L   [a-zA-Z_]
A   [a-zA-Z_0-9]
H   [a-fA-F0-9]
HP  (0[xX])
E   ([Ee][+-]?{D}+)
P   ([Pp][+-]?{D}+)
FS  (f|F|l|L)
CP  (u|U|L)
SP  (u8|u|U|L)
ES  (\\(['"\?\\abfnrtv]|[0-7]{1,3}|x[a-fA-F0-9]+))
WS  [ \t\v\f\n]

%{
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <SDL.h>
 #include "link.h"
 #include "fifo.h"

 #include "vt100.h"
 #include "async.h"
 #include "main.h"
 #include "app.h"
 #include "Z80.h"

 #include "y.tab.h"
 #include "lang.h"


 extern YYSTYPE yylval;

 const char hex_chars[] = "0123456789ABCDEF";
 const char oct_chars[] = "01234567";

#define YY_INPUT(buf,result,max_size) \
  { \
   int c = lang_getchar(); \
   result = (c == EOF) ? YY_NULL : (buf[0] = c,1); \
  }

 int row=0,col=0,file_pos=0;
 int last_line_start = 0;

 int line_pos = 0;

 value_t *new;

%}

%option noyywrap
%x comment

%%

"//".*                       { return(T_COMMENT); }
"watch"		             { return(T_WATCH); }
"wait"			     { return(T_WAIT); }
"xb"			     { return(T_8BIT); }
"s"			     { return(T_S); }
"hard"			     { return(T_HARD); }
"version"                    { return(T_VERSION); }
"script"		     { return(T_SCRIPT); }
"bank"			     { return(T_BANK); }
"systype"		     { return(T_SYSTYPE); }
"pal"		             { return(T_PAL); }
"ntsc"			     { return(T_NTSC); }
"regs"			     { return(T_REGS); }
"68k"			     { return(T_68K); }
"fm"		             { return(T_FM); }
"psg"                        { return(T_PSG); }
"z80"			     { return(T_Z80); }
"break"                      { return(T_BREAK); }
"stop"			     { return(T_STOP); }
"cont"			     { return(T_CONT); }
"reset"	                     { return(T_RESET); }
"int"                        { return(T_INT); }
"jump"		             { return(T_JUMP); }
"dump"	                     { return(T_DUMP); }
"dis"                        { return(T_DIS); }
"load"			     { return(T_LOAD); }
"poke"			     { return(T_POKE); }
"clear"			     { return(T_CLEAR); }
"help"                       { return(T_HELP); }
"exit"			     { return(T_EXIT); }
"-"			     { return('-'); }
":"			     { return(':'); }
","			     { return(','); }

{HP}{H}+                { 
 new = (value_t *)malloc(sizeof(value_t));
 new->type = INT;
 sscanf(yytext, "0x%x", &new->datum.ivalue);
 yylval.p = new;
 return CONSTANT; 
}

{NZ}{D}*               { 
 new = (value_t *)malloc(sizeof(value_t));
 new->type=INT;
 new->datum.ivalue = atoi(yytext);
 yylval.p=new;
 return CONSTANT; 
}

"0"{O}*               { 
 new = (value_t *)malloc(sizeof(value_t));
 new->type=INT;
 sscanf(yytext, "%o", &new->datum.ivalue);
 yylval.p = new;
 return CONSTANT; 
}

({SP}?\"([^"\\\n]|{ES})*\"{WS}*)+       { 
 {
  int i,j=0, k,w, d, close = 0, found = 0;;
  char *number;
  new = (value_t *)malloc(sizeof(value_t));
  new->type = STRING;
  new->datum.string = (char *)malloc(yyleng);
  for(i=1;i<yyleng;i++) {
   if(close == 0) {
    switch(yytext[i]) {
     case '\"': 
      close = 1;
      break;
     case '\\':
      if((i+1)<=yyleng) {
       switch(yytext[i+1]) {
        case 'a': new->datum.string[j++] = '\x07'; i++; break;
        case 'b': new->datum.string[j++] = '\x08'; i++; break;
        case 'f': new->datum.string[j++] = '\x0c'; i++; break;
        case 'n': new->datum.string[j++] = '\x0a'; i++; break;
        case 'r': new->datum.string[j++] = '\x0d'; i++; break;
        case 't': new->datum.string[j++] = '\x09'; i++; break;
        case 'v': new->datum.string[j++] = '\x0b'; i++; break;
        case '\\': new->datum.string[j++] ='\x5c'; i++; break;
        case '\'': new->datum.string[j++] ='\x27'; i++; break;
        case '\"': new->datum.string[j++] ='\x22'; i++; break;
        case 'x':
	 if((i+2)<=yyleng) {
	  number = (char *)malloc(yyleng);
	  for(k=0;k<2;k++) {
	   if((i+2+k)>yyleng) {
	    number[0] = 0;
	    break;
	   }
	   found = 0;
	   for(w=0;w<16;w++)
	    if(toupper(yytext[i+2+k]) == hex_chars[w])
	     found = 1;
	   if(found == 0) {
	    number[k] = 0;
	    break;
	   } else
	    number[k] = yytext[i+2+k];
	  }
	  number[2] = 0;
	  if(number[0] != 0) {
	   sscanf(number, "%x", &d);
	   i+=1+strlen(number);
	   new->datum.string[j++] = d;
	  }
	  free(number);
  	 } 
         break;

	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	 if((i+1)<=yyleng) {
          number = (char *)malloc(yyleng);
          for(k=0;k<3;k++) {
	   if((i+1+k)>yyleng) {
	    number[0] =0;
	    break;
	   }
	   found = 0;
	   for(w=0;w<8;w++)
	    if(yytext[i+1+k]==oct_chars[w])
	     found = 1;

	   if(found==0) {
	    number[k] = 0;
	    break;
	   } else 
	    number[k] = yytext[i+1+k];
	  }
	  number[3] = 0;
	  if(number[0] != 0) {
	   sscanf(number, "%o", &d);
	   i+=strlen(number);
	   new->datum.string[j++] = d;
	  }	   
	  free(number);
	 }

       }
      }  
      break;
     default:
      new->datum.string[j++] = yytext[i];
    }
   } else 
    if(yytext[i] == '\"') 
     close = 0;
  }
  new->datum.string[j] = 0;
  yylval.p = new; 
  return STRING_LITERAL; 
 }
}

{WS}                         {  }
. { return T_ERROR; }



%%

int lang_getchar() {
 int c;
 if((c=line[line_pos])==0) return EOF;
 line_pos++;
 return c;
}

