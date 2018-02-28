#ifndef COUNTER_HPP_6COIEFOP
#define COUNTER_HPP_6COIEFOP

#include <cstddef>
#include <cstring>
#include <iostream>

#include "utils.hpp"

#define COUNT(e) m_counter.count(counter::e)

namespace staccato
{
namespace internal
{

class counter
{
public:
	counter();

	enum event_e { 
		take         = 0,
		take_empty   = 1,
		take_stolen  = 2,
		steal        = 3,
		steal_race   = 4,
		steal_empty  = 5,
		steal2       = 6,
		steal2_race  = 7,
		steal2_empty = 8,
		dbg1         = 9,
		dbg2         = 10,
	};

	void count(event_e e);

	static void print_header();

	void print(size_t id) const;

private:
	static const size_t m_nconsters = 11;
	static const int m_cell_width = 9;

	static const constexpr char* const m_events[] = { 
		"take",
		"take!e",
		"take!s",
		"steal",
		"steal!r",
		"steal!e",
		"steal2",
		"steal2!r",
		"steal2!e",
		"dbg1",
		"dbg2"
	};

	unsigned long m_counters[m_nconsters];
};

const constexpr char* const counter::m_events[];

counter::counter()
{
	memset(m_counters, 0, m_nconsters * sizeof(m_counters[0]));
}

void counter::count(event_e e)
{
	auto i = static_cast<size_t>(e);
	m_counters[i]++;
}

void counter::print_header()
{
	FILE *fp = stdout;

	fprintf(fp, "[STACCATO]");
	fprintf(fp, " w# |");

	auto n = sizeof(m_events) / sizeof(m_events[0]);
	for (size_t i = 0; i < n; ++i)
		fprintf(fp, "%*s |", m_cell_width, m_events[i]);

	fprintf(fp, "\n");
}

void counter::print(size_t id) const
{
	FILE *fp = stdout;

	fprintf(fp, "[STACCATO]");
	fprintf(fp, "%3lu |", id);

	for (size_t i = 0; i < m_nconsters; ++i)
		fprintf(fp, "%*lu |", m_cell_width, m_counters[i]);

	fprintf(fp, "\n");
}

} /* internal */ 
} /* staccato */ 

#endif /* end of include guard: COUNTER_HPP_6COIEFOP */
