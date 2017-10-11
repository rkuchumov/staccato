#ifndef LAMBDA_TASK_HPP_PJHRXCSC
#define LAMBDA_TASK_HPP_PJHRXCSC

#include "task.hpp"
#include <functional>

namespace staccato
{
namespace internal
{

class worker;

class lambda_task: public task
{
public:
	lambda_task(std::function<void ()> fn);

	~lambda_task();

	void execute(); 

private:
	std::function<void ()> m_fn;
};

} /* internal */ 
} /* staccato */ 


#endif /* end of include guard: LAMBDA_TASK_HPP_PJHRXCSC */
