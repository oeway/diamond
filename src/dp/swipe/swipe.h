/****
DIAMOND protein aligner
Copyright (C) 2013-2018 Benjamin Buchfink <buchfink@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
****/

#ifndef SWIPE_H_
#define SWIPE_H_

#include "../score_vector.h"
#include "../../basic/value.h"

template<typename _score>
inline score_vector<_score> cell_update(const score_vector<_score> &diagonal_cell,
	const score_vector<_score> &scores,
	const score_vector<_score> &gap_extension,
	const score_vector<_score> &gap_open,
	score_vector<_score> &horizontal_gap,
	score_vector<_score> &vertical_gap,
	score_vector<_score> &best,
	const score_vector<_score> &vbias)
{
	score_vector<_score> current_cell = diagonal_cell + scores;
	current_cell -= vbias;
	current_cell.max(vertical_gap).max(horizontal_gap);
	best.max(current_cell);
	vertical_gap -= gap_extension;
	horizontal_gap -= gap_extension;
	const score_vector<_score> open = current_cell - gap_open;
	vertical_gap.max(open);
	horizontal_gap.max(open);
	return current_cell;
}

template<typename _score>
inline score_vector<_score> cell_update(const score_vector<_score> &diagonal_cell,
	const score_vector<_score> &scores,
	const score_vector<_score> &gap_extension,
	const score_vector<_score> &gap_open,
	score_vector<_score> &horizontal_gap,
	score_vector<_score> &vertical_gap,
	score_vector<_score> &best)
{
	score_vector<_score> current_cell = diagonal_cell + scores;
	current_cell.max(vertical_gap).max(horizontal_gap);
	best.max(current_cell);
	vertical_gap -= gap_extension;
	horizontal_gap -= gap_extension;
	const score_vector<_score> open = current_cell - gap_open;
	vertical_gap.max(open);
	horizontal_gap.max(open);
	return current_cell;
}

template<typename _score>
inline score_vector<_score> cell_update(const score_vector<_score> &diagonal_cell,
	const score_vector<_score> &reverse_cell,
	const score_vector<_score> &forward_cell,
	const score_vector<_score> &scores,
	const score_vector<_score> &gap_extension,
	const score_vector<_score> &gap_open,
	const score_vector<_score> &frame_shift,
	score_vector<_score> &horizontal_gap,
	score_vector<_score> &vertical_gap,
	score_vector<_score> &best)
{
	score_vector<_score> current_cell = diagonal_cell + scores;
	current_cell.max(reverse_cell + scores - frame_shift);
	current_cell.max(forward_cell + scores - frame_shift);
	current_cell.max(vertical_gap).max(horizontal_gap);
	best.max(current_cell);
	vertical_gap -= gap_extension;
	horizontal_gap -= gap_extension;
	const score_vector<_score> open = current_cell - gap_open;
	vertical_gap.max(open);
	horizontal_gap.max(open);
	return current_cell;
}

template<typename _score>
struct SwipeProfile
{
#ifdef __SSSE3__
	inline void set(const __m128i &seq)
	{
		assert(sizeof(data_) / sizeof(score_vector<_score>) >= value_traits.alphabet_size);
		score_vector<_score> bias(score_matrix.bias());
		for (unsigned j = 0; j < AMINO_ACID_COUNT; ++j)
			data_[j] = score_vector<_score>(j, seq, bias);
	}
#else
	inline void set(uint64_t seq)
	{
		assert(sizeof(data_) / sizeof(score_vector<_score>) >= value_traits.alphabet_size);
		for (unsigned j = 0; j < AMINO_ACID_COUNT; ++j)
			data_[j] = score_vector<_score>(j, seq);
	}
#endif
	inline const score_vector<_score>& get(Letter i) const
	{
		return data_[(int)i];
	}
	score_vector<_score> data_[AMINO_ACID_COUNT];
};

template<typename _score>
struct LazySwipeProfile
{
	LazySwipeProfile():
		set(0)
	{ }
	inline const score_vector<_score>& get(Letter i, const __m128i &seq)
	{
		if ((set & (1 << i)) == 0) {
			data_[(int)i] = score_vector<_score>(i, seq);
			set |= 1 << i;
		}
		return data_[(int)i];
	}
	int set;
	score_vector<_score> data_[AMINO_ACID_COUNT];
};


#endif