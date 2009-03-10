#ifndef __stdcsv_h
#define __stdcsv_h

#include <cat/cat.h>

#if CAT_USE_STDLIB

#include <cat/csv.h>

struct csv_record {
  unsigned long	cr_nfields;
  char **	cr_fields;
};

int  csv_fopen(struct csv_state *csv, const char *filename);
int  csv_fclose(struct csv_state *csv);
int  csv_read_field(struct csv_state *csv, char **field);
int  csv_read_rec(struct csv_state *csv, struct csv_record *cr);
void csv_free_rec(struct csv_record *cr);

#endif /* CAT_USE_STDLIB */

#endif /* __stdcsv_h */
