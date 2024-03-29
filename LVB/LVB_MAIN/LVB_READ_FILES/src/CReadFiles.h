/*
 * CReadFiles.h
 *
 *  Created on: Jun 17, 2014
 *      Author: mmp
 */

#ifndef CREADFILES_H_
#define CREADFILES_H_

#include <string>
#include <string.h>
#include <vector>
#include <sstream>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <sstream>
#include <algorithm>

using namespace std;

class CReadFiles {

public:
	/// this is because some compilers has a problem with to_string method
	template < typename T > std::string to_string_( const T& n )
	{
		std::ostringstream stm ;
		stm << n ;
		return stm.str() ;
	}

public:
	CReadFiles();
	virtual ~CReadFiles();
	static int exit_error(int ierr, std::string sz_error);
	static bool is_file_exist(std::string file_name);

	void save_file(std::string sz_file_name_temp);
	void read_file(std::string file_name_out);
	int get_length_sequences() {
		if ((int) lst_sequences.size() > 0) return lst_sequences[0].length();
		return 0;
	}
	int get_number_seqs() { return (int) lst_names_seq.size(); }
	int get_max_length_seq_name(){ return n_max_length_name_seq; }
	int get_length_seq_name(int n_seq) { return (int) lst_names_seq[n_seq].length(); }
	char get_char_seq_name(int n_seq, int n_pos_char) { return lst_names_seq[n_seq][n_pos_char]; }
	char get_char_sequences(int n_seq, int n_pos_char) { return lst_sequences[n_seq][n_pos_char]; }

/// data structure
private:
	std::vector< std::string > lst_sequences;
	std::vector< std::string > lst_names_seq;
	std::string sz_extension;						/// extension of the file
	std::string sz_file_name;					/// file name possible all path
	std::string sz_only_file_name;				/// only yhe file name
	int n_max_length_name_seq;

	void clean_data();

private:
	/// several read file methods
	void read_clustal(int filetype);
	void read_phylip();
	void read_fasta();
	void read_nexus();

	bool is_only_one_sequence_in_array(std::vector< std::string > lst_strings);
	std::string get_string_from_list(std::vector< std::string > lst_strings, bool b_last_one);

	std::string trim(std::string const& str);
	std::string trim(std::string const& str, char c_char);
	std::vector<std::string> split(const std::string &s, char delim);
	std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems);

	/// methods used in phylip files
	bool is_phylip_interleaved();
	bool is_phylip_line_sequential(int &n_total_lines);
	std::string clean_phylip_dna_sequence(std::string sz_sequence);

	/// used for phylip files, is the max length of the namess
	int n_nmlngth_phylip_names;
	std::string sz_phylip_accept_chars;

	//// trim chars form the strings
	std::vector<char> vect_trim_char;
	bool b_debug;

	/// members for nexus format
	void get_dimensions_nexus_format(std::string sz_line, int &n_seqs, int &n_length_seq);

};

#endif /* CREADFILES_H_ */
