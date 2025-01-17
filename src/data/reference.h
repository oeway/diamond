/****
DIAMOND protein aligner
Copyright (C) 2013-2018 Benjamin Buchfink <buchfink@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
****/

#ifndef REFERENCE_H_
#define REFERENCE_H_

#include <vector>
#include <string>
#include <string.h>
#include <stdint.h>
#include "../util/io/serializer.h"
#include "../util/io/input_file.h"
#include "../data/seed_histogram.h"
#include "sequence_set.h"
#include "metadata.h"

using std::vector;
using std::string;

struct ReferenceHeader
{
	ReferenceHeader() :
		magic_number(MAGIC_NUMBER),
		build(Const::build_version),
		db_version(current_db_version),
		sequences(0),
		letters(0)
	{ }
	uint64_t magic_number;
	uint32_t build, db_version;
	uint64_t sequences, letters, pos_array_offset;
	enum { current_db_version = 3 };
	static constexpr uint64_t MAGIC_NUMBER = 0x24af8a415ee186dllu;
};

struct ReferenceHeader2
{
	ReferenceHeader2():
		taxon_array_offset(0),
		taxon_array_size(0),
		taxon_nodes_offset(0),
		taxon_names_offset(0)
	{
		memset(hash, 0, sizeof(hash));
	}
	char hash[16];
	uint64_t taxon_array_offset, taxon_array_size, taxon_nodes_offset, taxon_names_offset;

	friend Serializer& operator<<(Serializer &s, const ReferenceHeader2 &h);
	friend Deserializer& operator>>(Deserializer &d, ReferenceHeader2 &h);
};

struct Database_format_exception : public std::exception
{
	virtual const char* what() const throw()
	{ return "Database file is not a DIAMOND database."; }
};

struct DatabaseFile : public InputFile
{

	DatabaseFile(const string &file_name);
	DatabaseFile(TempFile &tmp_file);
	static void read_header(InputFile &stream, ReferenceHeader &header);
	static DatabaseFile* auto_create_from_fasta();
	static bool is_diamond_db(const string &file_name);
	void rewind();
	bool load_seqs(vector<unsigned> &block_to_database_id, size_t max_letters, Sequence_set **dst_seq, String_set<0> **dst_id, bool load_ids = true, const vector<bool> *filter = NULL);
	void get_seq();
	void read_seq(string &id, vector<char> &seq);
	bool has_taxon_id_lists();
	bool has_taxon_nodes();
	bool has_taxon_scientific_names();
	void close();
	void seek_seq(size_t i);
	size_t tell_seq() const;
	void seek_direct();

	enum { min_build_required = 74, MIN_DB_VERSION = 2 };

	bool temporary;
	size_t pos_array_offset;
	ReferenceHeader ref_header;
	ReferenceHeader2 header2;

private:
	void init();

};

void make_db(TempFile **tmp_out = nullptr);

struct ref_seqs
{
	static const Sequence_set& get()
	{ return *data_; }
	static Sequence_set& get_nc()
	{ return *data_; }
	static Sequence_set *data_;
};

struct ref_ids
{
	static const String_set<0>& get()
	{ return *data_; }
	static String_set<0> *data_;
};

extern Partitioned_histogram ref_hst;
extern unsigned current_ref_block;
extern bool blocked_processing;

inline size_t max_id_len(const String_set<0> &ids)
{
	size_t max (0);
	for(size_t i=0;i<ids.get_length(); ++i)
		max = std::max(max, find_first_of(ids[i].c_str(), Const::id_delimiters));
	return max;
}

inline vector<string> seq_titles(const char *title)
{
	return tokenize(title, "\1");
}

#endif /* REFERENCE_H_ */
