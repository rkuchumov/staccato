#ifndef COUNTER_HPP_6COIEFOP
#define COUNTER_HPP_6COIEFOP

#include <cstddef>
#include <cstring>
#include <iostream>

#include "utils.hpp"

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
		take_failed  = 1,
		steal        = 2,
		steal_race   = 3,
		steal_null   = 4,
		steal_empty  = 5,
		steal2       = 6,
		steal2_race  = 7,
		steal2_null  = 8,
		steal2_empty = 9,
	};

	void count(event_e e);

	static void print_header();

	void print(size_t id) const;

private:
	static const size_t m_nconsters = 10;
	static const int m_cell_width = 9;

	static const constexpr char* const m_events[] = { 
		"take",
		"take!",
		"steal",
		"steal!r",
		"steal!n",
		"steal!e",
		"steal2",
		"steal2!r",
		"steal2!n",
		"steal2!e"
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

	fprintf(fp, "[STACCATO] ");
	fprintf(fp, " w# |");

	auto n = sizeof(m_events) / sizeof(m_events[0]);
	for (size_t i = 0; i < n; ++i)
		fprintf(fp, "%*s |", m_cell_width, m_events[i]);

	fprintf(fp, "\n");
}

void counter::print(size_t id) const
{
	FILE *fp = stdout;
	fprintf(fp, "[STACCATO] ");
	fprintf(fp, "%3lu |", id);

	for (size_t i = 0; i < m_nconsters; ++i)
		fprintf(fp, "%*lu |", m_cell_width, m_counters[i]);

	fprintf(fp, "\n");
}

} /* internal */ 
} /* staccato */ 

#endif /* end of include guard: COUNTER_HPP_6COIEFOP */
