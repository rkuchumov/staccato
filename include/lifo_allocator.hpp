#ifndef LIFO_ALLOCATOR_HPP_PXMFXMQN
#define LIFO_ALLOCATOR_HPP_PXMFXMQN

#include <cstdint>
#include <cstdlib>
#include <memory>
#include <new>

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
		page(size_t size = m_alignment);

		~page();

		void *alloc(size_t alignment, size_t size);

		void set_next(page *p);

		page *get_next() const;

	private:
		static const size_t m_alignment = 4 * (1 << 10);

		page *m_next;
		size_t m_size_left;
		void *m_stack;
		void *m_base;
	};

	void inc_tail(size_t required_size);

	void *alloc(size_t alignment, size_t size);

	const size_t m_page_size;

	page *m_head;
	page *m_tail;
};

lifo_allocator::page::page(size_t size)
: m_next(nullptr)
, m_size_left(round_align(m_alignment, size))
{
	// TODO: Nope, don't allocate like that. I should init page class over
	// allocated memory
	m_stack = aligned_alloc(m_alignment, m_size_left);
	m_base = m_stack;
}

lifo_allocator::page::~page()
{
	std::free(m_stack);
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

lifo_allocator::lifo_allocator(size_t page_size)
: m_page_size(page_size)
{
	m_head = new page(page_size);
	m_tail = m_head;
}

lifo_allocator::~lifo_allocator()
{
	auto n = m_head;

	while (n) {
		auto p = n;
		n = n->get_next();
		delete p;
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

	auto p = new page(sz);

	m_tail->set_next(p);
	m_tail = p;
}

} /* internal */ 
} /* staccato */ 

#endif /* end of include guard: LIFO_ALLOCATOR_HPP_PXMFXMQN */
