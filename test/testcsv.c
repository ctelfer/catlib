#include <stdio.h>
#include <cat/csv.h>
#include <cat/list.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	char farr[32] = { 0 };
	int i, j;
	char buf[20];
	int code;
	struct csv_state csv;

	for ( i = 1 ; i < argc ; ++i ) {
		j = atoi(argv[i]);
		if ( j <= 0 || j > sizeof(farr) )
			exit(-1);
		farr[j-1] = 1;
	}

	csv_init(&csv, (int (*)())getchar, NULL);

	i = 0;
	while ( (code = csv_next(&csv, buf, sizeof(buf), 0)) != CSV_EOF ) {
		if ( code == CSV_CNT )
			code = csv_clear_field(&csv);
		if ( i < sizeof(farr) && farr[i++] )
			printf("%19s ", buf);
		if ( code == CSV_REC ) {
			printf("\n");
			i = 0;
		}
	}

	return 0;
}
