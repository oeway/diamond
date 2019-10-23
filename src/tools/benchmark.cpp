#include <chrono>
#include <utility>
#include "../basic/sequence.h"
#include "../basic/score_matrix.h"
#include "../dp/score_vector.h"

#ifdef __SSE2__
#include "../util/simd/transpose.h"
#endif

#include "../dp/swipe/swipe.h"
#include "../dp/dp.h"

using std::vector;
using std::chrono::high_resolution_clock;
using std::chrono::nanoseconds;
using std::chrono::duration_cast;
using std::list;

namespace Benchmark {

#ifdef __SSE2__
__m128i global_128;
#endif
int global_int;

int xdrop_window2(const Letter *query, const Letter *subject)
{
	static const int window = 40;
	int score(0), st(0), n = 0, i = 0;

	const Letter *q(query), *s(subject);

	st = score;
	while (n < window)
	{
		st += score_matrix(*q, *s);
		if (st > score) {
			score = st;
			i = n;
		}
		++q;
		++s;
		++n;

		st += score_matrix(*q, *s);
		if (st > score) {
			score = st;
			i = n;
		}
		++q;
		++s;
		++n;

		st += score_matrix(*q, *s);
		if (st > score) {
			score = st;
			i = n;
		}
		++q;
		++s;
		++n;
	}
	return st * i;
}

void benchmark_ungapped(const sequence &s1, const sequence &s2)
{
	static const size_t n = 10000000llu;
	high_resolution_clock::time_point t1 = high_resolution_clock::now();

	const Letter *q = s1.data(), *s = s2.data();
	int score = 0;

	for (size_t i = 0; i < n; ++i) {

		score += xdrop_window2(q, s);

	}
	high_resolution_clock::time_point t2 = high_resolution_clock::now();
	std::chrono::nanoseconds time_span = duration_cast<std::chrono::nanoseconds>(t2 - t1);

	global_int = score;
	cout << "Scalar ungapped extension:\t" << (double)time_span.count() / (n*40) * 1000 << " ps/Cell" << endl;
}

#ifdef __SSE2__
void benchmark_ungapped_sse(const sequence &s1, const sequence &s2)
{
	static const size_t n = 100000000llu;
	high_resolution_clock::time_point t1 = high_resolution_clock::now();

	const Letter *q = s1.data(), *s = s2.data();
	int score = 0;
	score_vector<uint8_t> sv, sum;
	unsigned a = (unsigned)global_int & 15;
	__m128i seq = _mm_loadu_si128((const __m128i*)s1.data());

	for (size_t i = 0; i < n; ++i) {		
		sv.set_ssse3(i & 15, seq);
		sum = sum + sv;
	}

	_mm_storeu_si128(&global_128, sum.data_);
	cout << "SSE score shuffle:\t\t" << (double)duration_cast<std::chrono::nanoseconds>(high_resolution_clock::now() - t1).count() / (n * 16) * 1000 << " ps/Letter" << endl;
}

void benchmark_transpose() {
	static const size_t n = 100000000llu;
	static char in[256], out[256];

	high_resolution_clock::time_point t1 = high_resolution_clock::now();
	for (size_t i = 0; i < n; ++i) {
		transpose(in, out, 0);
		in[0] = out[0];
	}
	cout << "Matrix transpose 16x16 bytes:\t" << (double)duration_cast<std::chrono::nanoseconds>(high_resolution_clock::now() - t1).count() / (n * 256) * 1000 << " ps/Letter" << endl;
}


void swipe_cell_update() {
	static const size_t n = 1000000000llu;
	high_resolution_clock::time_point t1 = high_resolution_clock::now();
	{
		score_vector<uint8_t> diagonal_cell, scores, gap_extension, gap_open, horizontal_gap, vertical_gap, best, vbias;
		for (size_t i = 0; i < n; ++i) {
			diagonal_cell = cell_update(diagonal_cell, scores, gap_extension, gap_open, horizontal_gap, vertical_gap, best, vbias);
		}
		_mm_storeu_si128(&global_128, diagonal_cell.data_);
	}
	cout << "SWIPE cell update (uint8_t):\t" << (double)duration_cast<std::chrono::nanoseconds>(high_resolution_clock::now() - t1).count() / (n * 16) * 1000 << " ps/Cell" << endl;

	t1 = high_resolution_clock::now();
	{
		score_vector<int8_t> diagonal_cell, scores, gap_extension, gap_open, horizontal_gap, vertical_gap, best;
		for (size_t i = 0; i < n; ++i) {
			diagonal_cell = cell_update(diagonal_cell, scores, gap_extension, gap_open, horizontal_gap, vertical_gap, best);
		}
		_mm_storeu_si128(&global_128, diagonal_cell.data_);
	}
	cout << "SWIPE cell update (int8_t):\t" << (double)duration_cast<std::chrono::nanoseconds>(high_resolution_clock::now() - t1).count() / (n * 16) * 1000 << " ps/Cell" << endl;
}
#endif
void swipe(const sequence &s1, const sequence &s2) {
	sequence target[16];
	std::fill(target, target + 16, s2);
	static const size_t n = 10000llu;
	high_resolution_clock::time_point t1 = high_resolution_clock::now();
	for (size_t i = 0; i < n; ++i) {
		vector<int> v = DP::Swipe::swipe(s1, target, target + 16);
		global_int = v[0];
	}
	cout << "SWIPE:\t\t\t\t" << (double)duration_cast<std::chrono::nanoseconds>(high_resolution_clock::now() - t1).count() / (n * s1.length() * s2.length() * 16) * 1000 << " ps/Cell" << endl;
}

void banded_swipe(const sequence &s1, const sequence &s2) {
	vector<DpTarget> target;
	list<Hsp> out;
	for (size_t i = 0; i < 8; ++i)
		target.emplace_back(s2, -32, 32, &out);
	static const size_t n = 10000llu;
	high_resolution_clock::time_point t1 = high_resolution_clock::now();
	for (size_t i = 0; i < n; ++i) {
		DP::BandedSwipe::swipe(s1, target.begin(), target.end());
		out.clear();
	}
	cout << "Banded SWIPE:\t\t\t" << (double)duration_cast<std::chrono::nanoseconds>(high_resolution_clock::now() - t1).count() / (n * s1.length() * 65 * 8) * 1000 << " ps/Cell" << endl;
}

}

void benchmark() {
	vector<Letter> s1, s2;
	
	s1 = sequence::from_string("mpeeeysefkelilqkelhvvyalshvcgqdrtllasillriflhekleslllctlndreismedeattlfrattlastlmeqymkatatqfvhhalkdsilkimeskqscelspskleknedvntnlthllnilselvekifmaseilpptlryiygclqksvqhkwptnttmrtrvvsgfvflrlicpailnprmfniisdspspiaartlilvaksvqnlanlvefgakepymegvnpfiksnkhrmimfldelgnvpelpdttehsrtdlsrdlaalheicvahsdelrtlsnergaqqhvlkkllaitellqqkqnqyt");
	s2 = sequence::from_string("erlvelvtmmgdqgelpiamalanvvpcsqwdelarvlvtlfdsrhllyqllwnmfskeveladsmqtlfrgnslaskimtfcfkvygatylqklldpllrivitssdwqhvsfevdptrlepsesleenqrnllqmtekffhaiissssefppqlrsvchclyqvvsqrfpqnsigavgsamflrfinpaivspyeagildkkpppiierglklmskilqsianhvlftkeehmrpfndfvksnfdaarrffldiasdcptsdavnhslsfisdgnvlalhrllwnnqekigqylssnrdhkavgrrpfdkmatllaylgppe");
	
	Benchmark::benchmark_ungapped(s1, s2);
	#ifdef __SSE2__
	Benchmark::benchmark_ungapped_sse(s1, s2);
	Benchmark::benchmark_transpose();
	Benchmark::swipe_cell_update();
	#endif
	Benchmark::swipe(s1, s2);
	Benchmark::banded_swipe(s1, s2);
}