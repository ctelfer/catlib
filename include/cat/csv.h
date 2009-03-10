#ifndef __cat_csv_h
#define __cat_csv_h
#include <cat/cat.h>

enum {
  CSV_ERR = -1,
  CSV_EOF = 0,
  CSV_OK  = 1,
  CSV_FLD = 2,
  CSV_REC = 3,
  CSV_CNT = 4,
  CSV_GETC_EOF = -1,
  CSV_GETC_ERR = -2
};

typedef int (*getchar_f)(void *aux);

struct csv_state {
  getchar_f	csv_getc;
  void *	csv_gcctx;
  int		csv_done;
  int		csv_inquote;
  int		csv_sawquote;
  int		csv_last;
};

void csv_init(struct csv_state *csv, getchar_f gc, void *gcctx);
int  csv_next(struct csv_state *csv, char *buf, size_t len, size_t *rlen);
int  csv_clear_field(struct csv_state *csv);

#endif /* __cat_csv_h */
