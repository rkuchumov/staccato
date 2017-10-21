#include "task.hpp"

#include "lambda_task.hpp"

namespace staccato
{
namespace internal
{

lambda_task::lambda_task(std::function<void ()> fn)
: m_fn(fn)
{ }

lambda_task::~lambda_task()
{ }

void lambda_task::execute()
{
	if (m_fn)
		m_fn();
}

} /* internal */ 
} /* staccato */ 
