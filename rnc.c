/* $Id$ */

#include <fcntl.h> /* open, read, close */
#include <string.h> /* memcpy */
#include <stdlib.h> /* calloc,malloc,free */
#include <stdio.h> /*stderr*/

#include "u.h"
#include "rn.h"
#include "er.h"
#include "rnc.h"

/* taken from the specification:

topLevel ::= decl* (pattern | grammarContent*)
decl ::= "namespace" identifierOrKeyword "=" namespaceURILiteral
| "default" "namespace" [identifierOrKeyword] "=" namespaceURILiteral
| "datatypes" identifierOrKeyword "=" literal
pattern ::= "element" nameClass "{" pattern "}"
| "attribute" nameClass "{" pattern "}"
| pattern ("," pattern)+
| pattern ("&" pattern)+
| pattern ("|" pattern)+
| pattern "?"
| pattern "*"
| pattern "+"
| "list" "{" pattern "}"
| "mixed" "{" pattern "}"
| identifier
| "parent" identifier
| "empty"
| "text"
| [datatypeName] datatypeValue
| datatypeName ["{" param* "}"] [exceptPattern]
| "notAllowed"
| "external" anyURILiteral [inherit]
| "grammar" "{" grammarContent* "}"
| "(" pattern ")"
param ::= identifierOrKeyword "=" literal
exceptPattern ::= "-" pattern
grammarContent ::= start
| define
| "div" "{" grammarContent* "}"
| "include" anyURILiteral [inherit] ["{" includeContent* "}"]
includeContent ::= define
| start
| "div" "{" includeContent* "}"
start ::= "start" assignMethod pattern
define ::= identifier assignMethod pattern
assignMethod ::= "="
| "|="
| "&="
nameClass ::= name
| nsName [exceptNameClass]
| anyName [exceptNameClass]
| nameClass "|" nameClass
| "(" nameClass ")"
name ::= identifierOrKeyword
| CName
exceptNameClass ::= "-" nameClass
datatypeName ::= CName
| "string"
| "token"
datatypeValue ::= literal
anyURILiteral ::= literal
namespaceURILiteral ::= literal
| "inherit"
inherit ::= "inherit" "=" identifierOrKeyword
identifierOrKeyword ::= identifier
| keyword
identifier ::= (NCName - keyword)
| quotedIdentifier
quotedIdentifier ::= "\" NCName
CName ::= NCName ":" NCName
nsName ::= NCName ":*"
anyName ::= "*"
literal ::= literalSegment ("~" literalSegment)+
literalSegment ::= '"' (Char - ('"' | newline))* '"'
| "'" (Char - ("'" | newline))* "'"
| '"""' (['"'] ['"'] (Char - '"'))* '"""'
| "'''" (["'"] ["'"] (Char - "'"))* "'''"
keyword ::= "attribute"
| "default"
| "datatypes"
| "div"
| "element"
| "empty"
| "external"
| "grammar"
| "include"
| "inherit"
| "list"
| "mixed"
| "namespace"
| "notAllowed"
| "parent"
| "start"
| "string"
| "text"
| "token"
*/

#define SYM_EOF 0
#define SYM_DEFAULT 1
#define SYM_DATATYPE 2
#define SYM_DIV 3
#define SYM_ELEMENT 4
#define SYM_EMPTY 5
#define SYM_EXTERNAL 6
#define SYM_GRAMMAR 7
#define SYM_INCLUDE 8
#define SYM_INHERIT 9
#define SYM_LIST 10
#define SYM_MIXED 11
#define SYM_NAMESPACE 12
#define SYM_NOT_ALLOWED 13
#define SYM_PARENT 14
#define SYM_START 15
#define SYM_STRING 16
#define SYM_TEXT 17
#define SYM_TOKEN 18
#define SYM_ASGN 19
#define SYM_ASGN_ILEAVE 20
#define SYM_ASGN_CHOICE 21
#define SYM_SEQ 22 /* , */
#define SYM_CHOICE 23 
#define SYM_ILEAVE 24
#define SYM_OPTIONAL 25
#define SYM_ZERO_OR_MORE 26
#define SYM_ONE_OR_MORE 27
#define SYM_LPAR 28
#define SYM_RPAR 29
#define SYM_LCUR 30
#define SYM_RCUR 31
#define SYM_LSQU 32
#define SYM_RSQU 33
#define SYM_EXCEPT 34
#define SYM_QNAME 35   /* : */
#define SYM_NS_NAME 36 /* :* */
#define SYM_ANY_NAME 37 /* * */
#define SYM_QUOTE 38  /* \ */
#define SYM_ANNOTATION 39 /* >> */
#define SYM_COMMENT 40 
#define SYM_DOCUMENTATION 41 /* ## */
#define SYM_IDENT 42
#define SYM_LITERAL 43

#define BUFSIZE 1024
#define CUFSIZE 1
#define BUFTAIL 6

#define SRC_FREE 1
#define SRC_CLOSE 2

struct utf_source {
  char *fn; int fd;
  char *buf; int i,n;
  int complete;
  int line,col;
  int u,v,w; int nx;
  int flags;
};

static void rnc_init(struct utf_source *sp);
static int rnc_read(struct utf_source *sp);

int rnc_stropen(struct utf_source *sp,char *fn,char *s,int len) {
  rnc_init(sp);
  sp->buf=s; sp->n=len; sp->complete=1; 
  return 0;
}

int rnc_bind(struct utf_source *sp,char *fn,int fd) {
  rnc_init(sp);
  sp->fn=fn; sp->fd=fd;
  sp->buf=(char*)calloc(BUFSIZE,sizeof(char));
  sp->complete=sp->fd==-1;
  sp->flags=SRC_FREE;
  rnc_read(sp);
  return sp->fd;
}

int rnc_open(struct utf_source *sp,char *fn) {
  sp->flags|=SRC_CLOSE;
  return rnc_bind(sp,fn,open(fn,O_RDONLY));
}

int rnc_close(struct utf_source *sp) {
  int ret=0;
  if(sp->flags&SRC_FREE) free(sp->buf); 
  sp->buf=NULL;
  sp->complete=-1;
  if(sp->flags&SRC_CLOSE) {
    if(sp->fd!=-1) {
      ret=close(sp->fd); sp->fd=-1;
    }
  }
  return ret;
}

static void rnc_init(struct utf_source *sp) {
  sp->fn=sp->buf=NULL;
  sp->i=sp->n=0; 
  sp->complete=sp->fd=-1;
  sp->u=sp->v=0; sp->nx=-1;
  sp->flags=0;
}

static int rnc_read(struct utf_source *sp) {
  int ni;
  memcpy(sp->buf,sp->buf+sp->i,sp->n-=sp->i);
  sp->i=0;
  for(;;) {
    ni=read(sp->fd,sp->buf+sp->n,BUFSIZE-sp->n);
    if(ni>0) {
      sp->n+=ni;
      if(sp->n>=BUFTAIL) break;
    } else {
      close(sp->fd); sp->fd=-1;
      sp->complete=1;
      break;
    }
  }
  return ni;
}

/* read utf8 */
static void getu(struct utf_source *sp) {
  int n,u0=sp->u;
  for(;;) {
    if(!sp->complete&&sp->i>sp->n-BUFTAIL) {
      if(rnc_read(sp)==-1) (*er_handler)(ER_IO,sp->fn);
    }
    if(sp->i==sp->n) {
      sp->u=u0=='\n'?-1:'\n'; 
      return;
    } /* eof */
    n=u_get(&sp->u,sp->buf+sp->i);
    if(n==0) { 
      (*er_handler)(ER_UTF,sp->fn,sp->line,sp->col);
      ++sp->i;
      continue;
    } else if(n+sp->i>sp->n) { 
      (*er_handler)(ER_UTF,sp->fn,sp->line,sp->col);
      sp->i=sp->n;
      continue;
    } else {
      sp->i+=n;
      if(u0=='\r'&&sp->u=='\n') continue;
    }
    return;
  }
}

/* newlines are replaced with \0; \x{<hex>+} are unescaped.
the result is in sp->v
*/
static void getv(struct utf_source *sp) {
  if(sp->nx>0) {
    sp->v='x'; --sp->nx;
  } else if(sp->nx==0) {
    sp->v=sp->w;
    sp->nx=-1;
  } else {
    getu(sp);
    switch(sp->u) {
    case '\r': case '\n': sp->v=0; break;
    case '\\':
      getu(sp);
      if(sp->u=='x') {
	sp->nx=0;
	do {
	  ++sp->nx;
	  getu(sp);
	} while(sp->u=='x');
	if(sp->u=='{') { 
	  sp->nx=-1;
	  sp->v=0; 
	  for(;;) {
	    getu(sp);
	    if(sp->u=='}') goto END_OF_HEX_DIGITS;
	    sp->v<<=4;
	    switch(sp->u) {
            case '0': break;
            case '1': sp->v+=1; break;
            case '2': sp->v+=2; break;
            case '3': sp->v+=3; break;
            case '4': sp->v+=4; break;
            case '5': sp->v+=5; break;
            case '6': sp->v+=6; break;
            case '7': sp->v+=7; break;
            case '8': sp->v+=8; break;
            case '9': sp->v+=9; break;
	    case 'A': case 'a': sp->v+=10; break;
	    case 'B': case 'b': sp->v+=11; break;
	    case 'C': case 'c': sp->v+=12; break;
	    case 'D': case 'd': sp->v+=13; break;
	    case 'E': case 'e': sp->v+=14; break;
	    case 'F': case 'f': sp->v+=15; break;
            default: 
	      (*er_handler)(ER_XESC,sp->fn,sp->line,sp->col);
	      goto END_OF_HEX_DIGITS;
            }
	  } END_OF_HEX_DIGITS:;
	} else {
	  sp->v='\\'; sp->w=sp->u;
	}
      } else {
	sp->nx=0;
	sp->v='\\'; sp->w=sp->u;
      }
      break;
    default:
      sp->v=sp->u;
      break;
    }
  }
}

static int sym(struct utf_source *sp) {
  switch(sp->v) {
  case -1: return SYM_EOF;
  case '#':
    break;
  case '=':
    break;
  case ',':
    break;
  case '|':
    break;
  case '&':
    break;
  case '?':
    break;
  case '*':
    break;
  case '+':
    break;
  case '-':
    break;
  case '(':
    break;
  case ')':
    break;
  case '{':
    break;
  case '}':
    break;
  case '"':
    break;
  case '\'':
    break;
  case '~':	   
    break;
  case '[':
    break;
  case ']':
    break;
  case '>':
    break;
  default:
  }
}

int main(int argc,char **argv) {
  struct utf_source src;
  src.line=src.col=1;
  rnc_bind(&src,"stdin",0);
  for(;;) {
    if(src.v==-1) break;
    if(src.v==0) {
      printf("\n>> ");
      src.line++;
      src.col=1;
    } else {
      src.col++;
      putchar(src.v);
    }
    getv(&src);
  }
  rnc_close(&src);
}

/*
 * $Log$
 * Revision 1.5  2003/11/20 23:35:17  dvd
 * cleanups
 *
 * Revision 1.4  2003/11/20 23:28:50  dvd
 * getu,getv debugged
 *
 * Revision 1.3  2003/11/20 16:29:08  dvd
 * x escapes sketched
 *
 * Revision 1.2  2003/11/20 07:46:16  dvd
 * +er, rnc in progress
 *
 * Revision 1.1  2003/11/19 00:28:57  dvd
 * back to lists of ranges
 *
 */
