#ifndef STACCATO_UTILS_H
#define STACCATO_UTILS_H

#include <iostream>
#include <cassert>
#include <iostream>
#include <sstream>
#include <thread>
#include <iomanip>

#include "constants.hpp"


namespace staccato {
namespace internal {

inline uint32_t xorshift_rand() {
	STACCATO_TLS static uint32_t x = 2463534242;

	x ^= x >> 13;
	x ^= x << 17;
	x ^= x >> 5;
	return x;
}

class Debug
{
public:
	Debug(size_t indent = 0)
	: m_printed(false)
	, m_indent(indent)
	{
		m_buffer << "[STACCATO]";

		m_buffer << "[";
		m_buffer << std::setfill('0') << std::setw(5)
			<< std::hash<std::thread::id>()(std::this_thread::get_id()) % 100000;
		m_buffer << "] ";

		for (size_t i = 0; i < m_indent; ++i) {
			m_buffer << "   ";
		}
	}

	~Debug()
	{
		if (!m_printed)
			print();
	}

	void print()
	{
	// TODO: make it thread safe
#if STACCATO_DEBUG
		m_buffer << std::endl;
		std::cerr << m_buffer.str();
#endif
		m_printed = true;
	}

	template <typename T>
	Debug & operator<<(const T &value)
	{
		m_buffer << value;
		return *this;
	}

private:
	std::ostringstream m_buffer;
	bool m_printed;
	size_t m_indent;
};
	
} /* internal */ 
} /* staccato */ 

#endif /* end of include guard: STACCATO_UTILS_H */

