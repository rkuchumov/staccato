#ifndef TOPOLOGY_HPP_B9DMY2XJ
#define TOPOLOGY_HPP_B9DMY2XJ

#include <cstdlib>
#include <vector>
#include <iostream>
#include <thread>

#include "utils.hpp"

namespace staccato
{

class topology
{
public:
	topology (
		unsigned ncores = std::thread::hardware_concurrency() / 2,
		unsigned nthreads = 2,
		unsigned nsockets = 1
	);

	virtual ~topology ();

	struct worker_t {
		unsigned core;
		int victim;
		size_t flags;
	};

	virtual const std::vector<worker_t> &get() const;

protected:
	std::vector<worker_t> m_data;
};

topology::topology(unsigned ncores, unsigned nthreads, unsigned nsockets)
{
	int v = -1;

	for (unsigned t = 0; t < nthreads; ++t) {
		for (unsigned s = 0; s < nsockets; ++s) {
			if (s > 0)
				v = (s - 1) * ncores;

			for (unsigned c = 0; c < ncores; ++c) {
				unsigned i = t * nsockets * ncores + s * ncores + c;

				size_t f = 0;

				if (t > 0)
					f |= internal::worker_flags_e::virtual_thread;
				else if (c == 0)
					f |= internal::worker_flags_e::socket_master;

				m_data.push_back({i, v, f});
				v++;
			}
		}
	}

}

topology::~topology()
{ }

const std::vector<topology::worker_t> &topology::get() const
{
	return m_data;
}

} /* staccato */ 

#endif /* end of include guard: TOPOLOGY_HPP_B9DMY2XJ */
