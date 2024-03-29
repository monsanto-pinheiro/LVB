/*
 * ReadFile.cpp
 *
 *  Created on: Jun 24, 2014
 *      Author: mmp
 */
#include "ReadFile.h"

void read_file(char *file_name, DataStructure *p_lvbmat){

	CReadFiles readFiles = CReadFiles();
	/// read file
	std::string sz_file_name = std::string(file_name);
	readFiles.read_file(file_name);

    /* scalars */
	//cout << (long) readFiles.get_length_sequences() << endl;
    p_lvbmat->m = (long) readFiles.get_length_sequences();
    p_lvbmat->n = (long) readFiles.get_number_seqs();

    /* array for row title strings */
    p_lvbmat->rowtitle = (char **) malloc((size_t) (p_lvbmat->n) * sizeof(char *));

    /* array for row strings */
    p_lvbmat->row = (char **) malloc((size_t) (p_lvbmat->n) * sizeof(char *));

	/* initialize unallocated pointers to NULL */
	for (int i = 0; i < p_lvbmat->n; ++i){
		p_lvbmat->rowtitle[i] = NULL;
		p_lvbmat->row[i] = NULL;
	}
    /* we want null-terminated strings, so we cannot simply point to
     * the same, non-null-terminated arrays as are found in PHYLIP's
     * data structures */
    for (int i = 0; i < p_lvbmat->n; i++){
    	p_lvbmat->rowtitle[i] = (char*) malloc(sizeof(char) * (readFiles.get_max_length_seq_name() + 1));
    	p_lvbmat->row[i] = (char*) malloc(sizeof(char) * (p_lvbmat->m + 1));
    }
    for (int i = 0; i < p_lvbmat->n; i++) {
        for (int j = 0; j < readFiles.get_length_seq_name(i); j++) p_lvbmat->rowtitle[i][j] = readFiles.get_char_seq_name(i, j);
        p_lvbmat->rowtitle[i][readFiles.get_length_seq_name(i)] = '\0';
    }
    for (int i = 0; i < p_lvbmat->n; i++) {
        for (int j = 0; j < p_lvbmat->m; j++) p_lvbmat->row[i][j] = readFiles.get_char_sequences(i, j);
        p_lvbmat->row[i][p_lvbmat->m] = '\0';
    }
//	std::string file_name_out = "/home/mmp/workspace/LVB_READ_FILES/aln_files/file_nexus_nex_out.fas";
//	readFiles.save_file(file_name_out);
}


void phylip_mat_dims_in_external(char *file_name, long *species_ptr, long *sites_ptr){

	CReadFiles readFiles = CReadFiles();
	/// read file
	std::string sz_file_name = std::string(file_name);
	readFiles.read_file(file_name);

	*species_ptr = (long) readFiles.get_number_seqs();
	*sites_ptr = (long) readFiles.get_length_sequences();
//	std::string file_name_out = "/home/mmp/workspace/LVB_READ_FILES/aln_files/file_nexus_nex_out.fas";
//	readFiles.save_file(file_name_out);

}


void free_lvbmat_structure(DataStructure *p_lvbmat){

	if (p_lvbmat->row != NULL){
		for(int i = 0; i < p_lvbmat->n; ++i) free(p_lvbmat->row[i]);
		free(p_lvbmat->row);
		p_lvbmat->row = NULL;
	}
	if (p_lvbmat->rowtitle != NULL){
		for(int i = 0; i < p_lvbmat->n; ++i) free(p_lvbmat->rowtitle[i]);
		free(p_lvbmat->rowtitle);
		p_lvbmat->rowtitle = NULL;
	}
//	free(p_lvbmat);
//	p_lvbmat = NULL;
}

