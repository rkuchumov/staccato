#ifndef TOPOLOGY_HPP_B9DMY2XJ
#define TOPOLOGY_HPP_B9DMY2XJ

#include <cstdlib>
#include <vector>
#include <iostream>
#include <thread>

namespace staccato
{

class topology
{
public:
	topology (
		unsigned ncores = std::thread::hardware_concurrency(),
		unsigned nthreads = 1,
		unsigned nsockets = 1
	);

	virtual ~topology ();

	struct worker_t {
		size_t id;
		unsigned core;
		int victim;
	};

	virtual const std::vector<worker_t> &get() const;

protected:
	std::vector<worker_t> m_data;
};

topology::topology(unsigned ncores, unsigned nthreads, unsigned nsockets)
{
	size_t w = 0;
	int v = -1;

	for (unsigned s = 0; s < nsockets; ++s) {
		if (s > 0)
			v = (s - 1) * ncores;

		for (unsigned t = 0; t < nthreads; ++t) {
			for (unsigned c = 0; c < ncores; ++c) {
				unsigned i = t * nsockets * ncores + s * ncores + c;
				m_data.push_back({w, i, v});
				std::cout << "#" << w << " core " << i << " victim " << v << std::endl;
				v = w;
				w++;
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
