#ifndef TOPOLOGY_HPP_B9DMY2XJ
#define TOPOLOGY_HPP_B9DMY2XJ

#include <cstdlib>
#include <vector>
#include <iostream>
#include <thread>
#include <map>
#include <limits>

#include <numa.h>

#include "utils.hpp"

namespace staccato
{

class topology
{
public:
	topology (
		size_t nworkers = 0,
		size_t max_thieves = 1,
		size_t threads_threshold = 4
	);

	virtual ~topology ();

	virtual size_t fetch_nsockets();
	virtual size_t fetch_nthreads();
	virtual size_t fetch_ncores();

	virtual size_t get_cpu_id(
		size_t soc,
		size_t thr,
		size_t cor
	) const;

	struct worker_t {
		size_t id;

		size_t soc;
		size_t thr;
		size_t cor;

		size_t victim;
		size_t flags;
	};

	virtual const std::map<size_t, worker_t> &get() const;

	size_t get_nworkers() const;

#if STACCATO_DEBUG
	void print(size_t cpu = 0, size_t depth = 1) const;
#endif

protected:
	void build_map();
	void link_thieves();
	bool link_thief(
		size_t victim,
		size_t thief,
		size_t flags = internal::worker_flags_e::none
	);
	bool set_thief_at_next_core(size_t victim, const worker_t &w);
	bool set_thief_at_next_thread(size_t victim, const worker_t &w);

	static const size_t UNDEF = std::numeric_limits<size_t>::max();

	size_t m_nsockets;
	size_t m_nthreads;
	size_t m_ncores;

	size_t m_nworkers;
	size_t m_max_thieves;
	size_t m_thr_threshold;

	std::map<size_t, worker_t> m_workers;
	std::vector<worker_t> m_data;
};

topology::topology(
	size_t nworkers,
	size_t max_thieves,
	size_t threads_threshold
)
: m_nsockets(0)
, m_nthreads(0)
, m_ncores(0)
, m_nworkers(nworkers)
, m_max_thieves(max_thieves)
, m_thr_threshold(threads_threshold)
{
	using namespace internal;

	fetch_nsockets();
	fetch_nthreads();
	fetch_ncores();

	if (m_nworkers == 0)
		m_nworkers = m_ncores * m_nsockets * m_nthreads;

	Debug() << "nsockets: " << m_nsockets;
	Debug() << "nthreads: " << m_nthreads;
	Debug() << "ncores: " << m_ncores;

	build_map();

	link_thieves();

#if STACCATO_DEBUG
	print();
#endif
}

topology::~topology()
{ }

void topology::build_map()
{
	size_t left = m_nworkers;
	size_t id = 0;

	for (size_t t = 0; t < m_nthreads; ++t) {
		for (size_t s = 0; s < m_nsockets; ++s) {

			for (size_t cc = 0; cc < m_ncores; ++cc) {
				if (left == 0)
					return;

				auto c = t ? m_ncores - cc - 1 : cc;

				if (t > 0 && c == 0 && s + 1 < m_nsockets)
					continue;

				auto cpu = get_cpu_id(s, t, c);
				m_workers[cpu] = {id, s, t, c, UNDEF, 0};

				id++;
				left--;
			}

			if (left <= m_thr_threshold && t + 1 < m_nthreads)
				break;
		}
	}

	m_nworkers -= left;
}

void topology::link_thieves()
{
	for (auto &p : m_workers) {
		auto &i = p.first;
		auto &w = p.second;

		size_t left = m_max_thieves;

		if (w.thr == 0 && w.cor == 0 && w.soc + 1 < m_nsockets) {
			auto t = get_cpu_id(w.soc + 1, 0, 0);
			if (link_thief(i, t, internal::worker_flags_e::distant_victim))
				left = 1;
		}

		if (w.thr > 0 && w.cor > 0 && left == 1) {
			auto t = get_cpu_id(w.soc, w.thr, w.cor - 1);
			link_thief(i, t);
			continue;
		}

		if (left > 0 && set_thief_at_next_core(i, w))
			left--;

		while (left > 0 && set_thief_at_next_thread(i, w))
			left--;

		while (left > 0 && set_thief_at_next_core(i, w))
			left--;
	}
}

bool topology::link_thief(size_t victim, size_t thief, size_t flags)
{
	auto t = m_workers.find(thief);
	if (t == m_workers.end())
		return false;

	if (t->second.victim != UNDEF)
		return false;

	t->second.victim = victim;
	t->second.flags = flags;

	return true;
}

bool topology::set_thief_at_next_thread(size_t victim, const worker_t &w)
{
	for (size_t n = w.thr + 1; n < m_nthreads; ++n) {
		auto thief = get_cpu_id(w.soc, n, w.cor);
		if (link_thief(victim, thief, internal::worker_flags_e::sibling_victim))
			return true;
	}

	return false;
}

bool topology::set_thief_at_next_core(size_t victim, const worker_t &w)
{
	for (size_t n = w.cor + 1; n < m_ncores; ++n) {
		auto thief = get_cpu_id(w.soc, w.thr, n);
		if (link_thief(victim, thief))
			return true;
	}

	return false;
}


size_t topology::fetch_nsockets()
{
	using namespace internal;

	if (m_nsockets)
		return m_nsockets;

	if (numa_available() < 0) {
		Debug() << "NUMA is not avaliable";
		return 1;
	}

	auto i = numa_max_node();
	Debug() << "NUMA max node: " << i;
	m_nsockets = i + 1;

	return m_nsockets;
}

size_t topology::fetch_nthreads()
{
	if (m_nthreads)
		return m_nthreads;

	// TODO: get number of threads per core
	m_nthreads = 2;
	return m_nthreads;
}

size_t topology::fetch_ncores()
{
	if (m_ncores)
		return m_ncores;

	m_ncores = std::thread::hardware_concurrency() /
		(fetch_nsockets() * fetch_nthreads());

	return m_ncores;
}

size_t topology::get_cpu_id(
	size_t soc,
	size_t thr,
	size_t cor
) const
{
	return thr * m_nsockets * m_ncores + soc * m_ncores + cor;
}

const std::map<size_t, topology::worker_t> &topology::get() const
{
	return m_workers;
}

size_t topology::get_nworkers() const
{
	return m_nworkers;
}

#if STACCATO_DEBUG
void topology::print(size_t cpu, size_t depth) const
{
	using namespace std;
	using namespace internal;

	auto fp = stderr;

	if (cpu == 0 && depth == 1) {
		Debug() << "Victim grapth:";
		fprintf(fp, "<cpu> <socket>:<thread>:<core> -- <worker_id> <flags>\n");
	}

	auto w = m_workers.at(cpu);
	fprintf(fp, "CPU%-3lu ", cpu);
	fprintf(fp, "%1lu:", w.soc);
	fprintf(fp, "%1lu:", w.thr);
	fprintf(fp, "%-2lu ", w.cor);

	for (size_t i = 0; i < depth; ++i)
		fprintf(fp, "--");

	fprintf(fp, " #%lu", w.id);
	if (w.flags & worker_flags_e::distant_victim)
		fprintf(fp, " distant_victim");
	if (w.flags & worker_flags_e::sibling_victim)
		fprintf(fp, " sibling_victim");

	fprintf(fp, "\n");

	for (auto &p : m_workers) {
		if (p.second.victim == cpu)
			print(p.first, depth + 1);
	}
}
#endif

} /* staccato */ 

#endif /* end of include guard: TOPOLOGY_HPP_B9DMY2XJ */
