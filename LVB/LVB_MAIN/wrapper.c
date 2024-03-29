/* LVB
 * (c) Copyright 2003-2012 by Daniel Barker.
 * (c) Copyright 2013, 2014 by Daniel Barker and Maximilian Strobl.
 * Permission is granted to copy and use this program provided that no fee is
 * charged for it and provided that this copyright notice is not removed. */

/**********

=head1 NAME

wrapper.c - LVB to PHYLIP interface

=cut

**********/

#include "lvb.h"

void read_file(char *file_name, DataStructure *p_lvbmat);
void phylip_mat_dims_in_external(char *file_name, long *species_ptr, long *sites_ptr);


/**********

=head1 phylip_dna_matrin - READ PHYLIP-FORMAT DNA DATA MATRIX

=head2 SYNOPSIS

    Dataptr phylip_dna_matrin(Lvb_bool ileaved);

=head2 DESCRIPTION

Read a DNA data matrix in PHYLIP 3.6 format from file. The file name is
given by the macro MATFNAM in F<lvb.h>.

=head2 PARAMETERS

=head3 INPUT

=over 4

=item ileaved

C<LVB_TRUE> indicates that the matrix is in PHYLIP interleaved format.
Otherwise, it is assumed to be in PHYLIP sequential format.

=back

=head2 RETURN

Returns a pointer to a new, dynamically allocated LVB data matrix
structure containing the data matrix.

=cut

**********/

void phylip_dna_matrin(char *p_file_name, Dataptr lvbmat)
{
	read_file(p_file_name, lvbmat);

    /* check number of sequences is in range for LVB */
    if (lvbmat->n < MIN_N) crash("The data matrix must have at least %ld sequences.", MIN_N);
    else if (lvbmat->n > MAX_N) crash("The data matrix must have no more than %ld sequences.", MAX_N);
    /* check number of sites is in range for LVB */
    else if (lvbmat->m < MIN_M) crash("The data matrix must have at least %ld sites.", MIN_N);
    else if (lvbmat->m > MAX_M) crash("The data matrix must have no more than %ld sites.", MAX_M);

} /* end phylip_dna_matrin() */

/**********

=head1 phylip_mat_dims_in - READ PHYLIP MATRIX DIMENSIONS

=head2 SYNOPSIS

void phylip_mat_dims_in(long *species_ptr, long *sites_ptr);

=head2 DESCRIPTION

Reads the number of sequences and number of sites per sequence in a
PHYLIP-format matrix file.

=head2 PARAMETERS

=head3 OUTPUT

=over 4

=item species_ptr

On return, *C<species_ptr> is set to the number of sequences.

=item sites_ptr

On return, *C<sites_ptr> is set to the number of sites per
sequence.

=back

=head2 BUGS

C<phylip_mat_dims_in()> should not be called if the matrix file is
open.

=cut

**********/

void phylip_mat_dims_in(char *p_file_name, long *species_ptr, long *sites_ptr){

	phylip_mat_dims_in_external(p_file_name, species_ptr, sites_ptr);
}
