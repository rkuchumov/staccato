#ifndef ROOT_TASK_HPP_V6TDOCST
#define ROOT_TASK_HPP_V6TDOCST

#include "task.hpp"

namespace staccato
{
namespace internal
{

class worker;

class root_task: public task
{
public:
	root_task();

	~root_task();

	void execute(); 
};

	
} /* internal */ 
} /* staccato */ 

#endif /* end of include guard: ROOT_TASK_HPP_V6TDOCST */
