/* $Id$ */

#include <stdarg.h>

#ifndef RNV_H
#define RNV_H 1

#define RNV_ER_ELEM 0
#define RNV_ER_AKEY 1
#define RNV_ER_AVAL 2
#define RNV_ER_EMIS 3
#define RNV_ER_AMIS 4
#define RNV_ER_UFIN 5
#define RNV_ER_TEXT 6
#define RNV_ER_MIXT 7

extern void (*rnv_verror_handler)(int erno,va_list ap);

extern void rnv_init(void);
extern void rnv_clear(void);

extern int rnv_flush_text(int current,char *text,int n_t,int mixed);
extern int rnv_start_element(int current,char *name,char **attrs);
extern int rnv_end_element(int current,char *name);

#endif