/* $Id$ */

#ifndef DRV_H
#define DRV_H 1

extern void drv_init(void);
extern void drv_clear(void);

/* Expat passes character data unterminated.  Hence functions that can deal with cdata expect the length of the data */

extern void drv_add_dtl(char *suri,int (*equal)(char *typ,char *val,char *s,int n),int (*allows)(char *typ,char *ps,char *s,int n));

extern int drv_start_tag_open(int p,char *suri,char *sname);
extern int drv_start_tag_open_recover(int p,char *suri,char *sname);
extern int drv_attribute_open(int p,char *suri,char *s);
extern int drv_attribute_open_recover(int p,char *suri,char *s);
extern int drv_attribute_close(int p);
extern int drv_attribute_close_recover(int p);
extern int drv_start_tag_close(int p);
extern int drv_start_tag_close_recover(int p);
extern int drv_text(int p,char *s,int n);
extern int drv_text_recover(int p,char *s,int n);
extern int drv_mixed_text(int p);
extern int drv_mixed_text_recover(int p);
extern int drv_end_tag(int p);
extern int drv_end_tag_recover(int p);

#endif
