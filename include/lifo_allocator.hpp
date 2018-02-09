#ifndef LIFO_ALLOCATOR_HPP_PXMFXMQN
#define LIFO_ALLOCATOR_HPP_PXMFXMQN

#include <cstdint>
#include <cstdlib>
#include <memory>
#include <new>

#include "utils.hpp"

namespace staccato
{
namespace internal
{

class lifo_allocator
{
public:
	lifo_allocator(size_t page_size);

	virtual ~lifo_allocator();

	template <typename T>
	T *alloc();

	template <typename T>
	T *alloc_array(size_t lenght);

	static inline size_t round_align(size_t to, size_t x) {
		return (x + (to - 1)) & ~(to - 1);
	}

private:
	class page {
	public:
		page(void *mem, size_t size);

		~page();

		void *alloc(size_t alignment, size_t size);

		void set_next(page *p);

		page *get_next() const;

		static page *allocate_page(size_t alignment, size_t size);

	private:
		page *m_next;
		size_t m_size_left;
		void *m_stack;
		void *m_base;
	};

	void inc_tail(size_t required_size);

	void *alloc(size_t alignment, size_t size);

	static const size_t m_page_alignment = 4 * (1 << 10);

	const size_t m_page_size;

	page *m_head;
	page *m_tail;
};

lifo_allocator::page::page(void *mem, size_t size)
: m_next(nullptr)
, m_size_left(size)
, m_stack(mem)
, m_base(reinterpret_cast<uint8_t *>(mem) + sizeof(page))
{ }

lifo_allocator::page::~page()
{
}

void lifo_allocator::page::set_next(lifo_allocator::page *p)
{
	m_next = p;
}

lifo_allocator::page *lifo_allocator::page::get_next() const
{
	return m_next;
}

void *lifo_allocator::page::alloc(size_t alignment, size_t size)
{
	auto p = std::align(alignment, size, m_base, m_size_left);

	if (!p)
		return nullptr;

	m_base = reinterpret_cast<uint8_t *>(m_base) + size;
	m_size_left -= size;

	return p;
}

lifo_allocator::page *lifo_allocator::page::allocate_page(
	size_t alignment,
	size_t size
) {
	auto sz = round_align(alignment, size);
	auto p = aligned_alloc(alignment, sz);

	new(p) page(p, sz - sizeof(page));

	return reinterpret_cast<page *>(p);
}

lifo_allocator::lifo_allocator(size_t page_size)
: m_page_size(page_size)
{
	m_head = page::allocate_page(m_page_alignment, m_page_size);
	m_tail = m_head;
}

lifo_allocator::~lifo_allocator()
{
	auto n = m_head;

	while (n) {
		auto p = n;
		n = n->get_next();
		std::free(p);
	}
}

template <typename T>
T *lifo_allocator::alloc()
{
	auto p = alloc(alignof(T), sizeof(T));
	return reinterpret_cast<T *>(p);
}

template <typename T>
T *lifo_allocator::alloc_array(size_t lenght)
{
	auto p = alloc(alignof(T), sizeof(T) * lenght);
	return reinterpret_cast<T *>(p);
}

void *lifo_allocator::alloc(size_t alignment, size_t size)
{
	void *ptr = m_tail->alloc(alignment, size);
	if (ptr)
		return ptr;

	inc_tail(size);

	ptr = m_tail->alloc(alignment, size);
	if (ptr)
		return ptr;

	throw std::bad_alloc();
}

void lifo_allocator::inc_tail(size_t required_size)
{
	auto sz = m_page_size;
	if (required_size > sz)
		sz = required_size;

	auto p = page::allocate_page(m_page_alignment, sz);

	m_tail->set_next(p);
	m_tail = p;
}

} /* internal */ 
} /* staccato */ 

#endif /* end of include guard: LIFO_ALLOCATOR_HPP_PXMFXMQN */
