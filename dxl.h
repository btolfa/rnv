/* $Id$ */

#ifndef DXL_H
#define DXL_H 1

#define DXL_URL "http://davidashen.net/relaxng/pluggable-datatypes"

extern char *dxl_cmd;

extern int dxl_allows(char *typ,char *ps,char *s,int n);
extern int dxl_equal(char *typ,char *val,char *s,int n);

#endif
