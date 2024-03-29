/* LVB
 * (c) Copyright 2003-2012 by Daniel Barker.
 * (c) Copyright 2013, 2014 by Daniel Barker and Maximilian Strobl.
 * Permission is granted to copy and use this program provided that no fee is
 * charged for it and provided that this copyright notice is not removed. */

#include <lvb.h>


/* basic test of phylip_mat_dims_in() */

/* these constants are given in the infile */
#define EXPECTED_N 10
#define EXPECTED_M 63

int main(void)
{
    long m;				/* sites */
    long n;				/* sequences */
    Lvb_bool success = LVB_FALSE;	/* test passed */

    lvb_initialize();

    phylip_mat_dims_in(&n, &m);
    if ((n == EXPECTED_N) && (m == EXPECTED_M))
    {
    	/* try it again and check it still works */
	phylip_mat_dims_in(&n, &m);
	if ((n == EXPECTED_N) && (m == EXPECTED_M))
	    success = LVB_TRUE;
    }

    if (success == LVB_TRUE)
        printf("test passed\n");
    else
    	printf("test failed\n");

    return 0;
}
