#ifndef TOPOLOGY_HPP_B9DMY2XJ
#define TOPOLOGY_HPP_B9DMY2XJ

#include <cstdlib>
#include <vector>
#include <iostream>
#include <thread>
#include <map>

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

	virtual size_t get_nsockets() const;
	virtual size_t get_nthreads() const;
	virtual size_t get_ncores() const;

	virtual size_t get_core_id(
		size_t soc,
		size_t thr,
		size_t cor
	) const;

	struct worker_t {
		size_t soc;
		size_t thr;
		size_t cor;

		size_t id;
		int victim;
		size_t flags;
	};

	virtual const std::map<size_t, worker_t> &get() const;

	size_t get_nworkers() const;

	// void print() const;

protected:
	void build_map();
	void link_thiefs();
	bool link_thief(size_t victim, size_t thief, size_t flags = 0);
	bool link_core_thief(size_t id, size_t s, size_t t, size_t c);
	bool link_thread_thief(size_t id, size_t s, size_t t, size_t c);

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
: m_nworkers(nworkers)
, m_max_thieves(max_thieves)
, m_thr_threshold(threads_threshold)
{
	using namespace internal;

	m_nsockets = 1;
	m_nthreads = 2;
	m_ncores = std::thread::hardware_concurrency() /
		(m_nsockets * m_nthreads);

	if (m_nworkers == 0)
		m_nworkers = m_ncores * m_nsockets * m_nthreads;

	Debug() << "nsockets: " << m_nsockets;
	Debug() << "nthreads: " << m_nthreads;
	Debug() << "ncores: " << m_ncores;

	build_map();
	link_thiefs();
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

				auto core = get_core_id(s, t, c);
				m_workers[core] = {s, t, c, id, -1, 0};

				id++;
				left--;
			}

			if (left <= m_thr_threshold && t + 1 < m_nthreads)
				break;
		}
	}

	m_nworkers -= left;
}

void topology::link_thiefs()
{
	for (auto &p : m_workers) {
		auto &w = p.second;

		size_t left = m_max_thieves;

		if (w.thr == 0 && w.cor == 0 && w.soc + 1 < m_nsockets) {
			auto t = get_core_id(w.soc + 1, 0, 0);
			if (link_thief(w.id, t, 2))
				left = 1;
		}

		if (w.thr > 0 && w.cor > 0 && left == 1) {
			auto t = get_core_id(w.soc, w.thr, w.cor - 1);
			link_thief(w.id, t, 0);
			continue;
		}

		if (left > 0 && link_core_thief(w.id, w.soc, w.thr, w.cor))
			left--;

		while (left > 0 && link_thread_thief(w.id, w.soc, w.thr, w.cor))
			left--;

		while (left > 0 && link_core_thief(w.id, w.soc, w.thr, w.cor))
			left--;
	}
}

bool topology::link_thief(size_t victim, size_t thief, size_t flags)
{
	auto t = m_workers.find(thief);
	if (t == m_workers.end())
		return false;

	if (t->second.victim >= 0)
		return false;

	t->second.victim = victim;
	t->second.flags = flags;

	return true;
}

bool topology::link_thread_thief(size_t id, size_t s, size_t t, size_t c)
{
	for (size_t n = t + 1; n < m_nthreads; ++n) {
		auto thief = get_core_id(s, n, c);
		if (link_thief(id, thief, 1))
			return true;
	}

	return false;
}

bool topology::link_core_thief(size_t id, size_t s, size_t t, size_t c)
{
	for (size_t n = c + 1; n < m_ncores; ++n) {
		auto thief = get_core_id(s, t, n);
		if (link_thief(id, thief, 0))
			return true;
	}

	return false;
}


size_t topology::get_nsockets() const
{
	return m_nsockets;
}

size_t topology::get_nthreads() const
{
	return m_nthreads;
}

size_t topology::get_ncores() const
{
	return m_ncores;
}

size_t topology::get_core_id(
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

// void topology::print() const
// {
// 	using namespace std;
// 	for (auto &p : m_workers) {
// 		auto w = p.second;
//
// 		cout << "#" << w.id << " \t " <<  w.soc << " " << w.thr << " " << w.cor << " : ";
// 		cout << " f" << w.flags;
// 		cout << " #" << w.victim;
//
// 		cout << "\n";
// 	}
// }

} /* staccato */ 

#endif /* end of include guard: TOPOLOGY_HPP_B9DMY2XJ */
