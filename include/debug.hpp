#ifndef DEBUG_HPP_0BRUWGOK
#define DEBUG_HPP_0BRUWGOK

#include <cstddef>
#include <iomanip>
#include <thread>
#include <iostream>
#include <sstream>

#include "utils.hpp"

namespace staccato {
namespace internal {

#if STACCATO_DEBUG

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

	template <typename T>
	Debug & operator<<(const T &value)
	{
		m_buffer << value;
		return *this;
	}

private:
	void print()
	{
		m_buffer << std::endl;
		std::cerr << m_buffer.str();
		m_printed = true;
	}

	std::ostringstream m_buffer;
	bool m_printed;
	size_t m_indent;
};

#else

class Debug
{
public:
	Debug(size_t n = 0) { }
	~Debug() { }

	template <typename T>
	Debug & operator<<(const T &) { return *this;}
};

#endif

} /* internal */ 
} /* staccato */ 

#endif /* end of include guard: DEBUG_HPP_0BRUWGOK */
