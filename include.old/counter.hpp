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
		
		nr_events,
	};

	void count(event_e e);

	static void print_header();

	void print_row(size_t id) const;

	void print_totals() const;

	void join(const counter &other);

private:
	static const int m_cell_width = 11;

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

	unsigned long m_counters[nr_events];
};

const constexpr char* const counter::m_events[];

counter::counter()
{
	memset(m_counters, 0, nr_events * sizeof(m_counters[0]));
}

void counter::count(event_e e)
{
	auto i = static_cast<size_t>(e);
	m_counters[i]++;
}

void counter::join(const counter &other)
{
	for (size_t i = 0; i < nr_events; ++i)
		m_counters[i] += other.m_counters[i];
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

void counter::print_row(size_t id) const
{
	FILE *fp = stdout;

	fprintf(fp, "[STACCATO]");
	fprintf(fp, "%3lu |", id);

	for (size_t i = 0; i < nr_events; ++i)
		fprintf(fp, "%*lu |", m_cell_width, m_counters[i]);

	fprintf(fp, "\n");
}

void counter::print_totals() const
{
	FILE *fp = stdout;

	fprintf(fp, "[STACCATO]");
	fprintf(fp, "tot |");

	for (size_t i = 0; i < nr_events; ++i)
		fprintf(fp, "%*lu |", m_cell_width, m_counters[i]);

	fprintf(fp, "\n");

	unsigned long nr_steals = 0;
	nr_steals += m_counters[steal];
	nr_steals += m_counters[steal_race];
	nr_steals += m_counters[steal_empty];
	nr_steals += m_counters[steal2];
	nr_steals += m_counters[steal2_race];
	nr_steals += m_counters[steal2_empty];
	fprintf(fp, "steal attempts:    %lu\n", nr_steals);
	fprintf(fp, "steal migartions:  %lu\n", m_counters[steal] + m_counters[steal2]);
	fprintf(fp, "take empty:   %lu\n", m_counters[take_empty]);
	fprintf(fp, "take stolen:  %lu\n", m_counters[take_stolen]);
	fprintf(fp, "\n");
}

} /* internal */ 
} /* staccato */ 

#endif /* end of include guard: COUNTER_HPP_6COIEFOP */
