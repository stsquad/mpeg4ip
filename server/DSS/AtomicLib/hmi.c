/*
 * to test use of timestamp()
 */

 /* cc hmi.c -o hmi libatomic.a */

#include <stdio.h>
#include "timestamp.h"

void
main(int argc, char **argv)
{
	int i, j;
	long long ts1, ts2;
	long long ts3, ts4;
	struct timescale tsc;
	double scale;

	if (argc <= 1) {
		fprintf(stderr, "Usage: %s loop-count\n", argv[0]);
		exit(1);
	}

	j = atoi(argv[1]);

	ts1 = timestamp();	/* START */

	/* Loop for the given loop-count */
	for (i=0; i < j; i++) {
		;
	}

	ts2 = timestamp();	/* END */

	printf("ts1 = %qd, ts2 = %qd\n", ts1, ts2);

	utimescale(&tsc);
	scale = (double)tsc.tsc_numerator / (double)tsc.tsc_denominator;

	ts1 = (long long)((double)ts1 * scale);
	ts2 = (long long)((double)ts2 * scale);

	printf("ts1 = %qd, ts2 = %qd, micro seconds = %qd\n",
			ts1, ts2, (ts2 - ts1));

	/* Use the scaledtimestamp() now */

	ts3 = scaledtimestamp(scale);	/* START */

	/* Loop for the given loop-count */
	for (i=0; i < j; i++) {
		;
	}

	ts4 = scaledtimestamp(scale);	/* END */

	printf("ts3 = %qd, ts4 = %qd, micro seconds = %qd\n",
			ts3, ts4, (ts4 - ts3));

}
