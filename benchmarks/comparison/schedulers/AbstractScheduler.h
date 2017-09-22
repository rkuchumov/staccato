#ifndef ABSTRACTSCHEDULER_H
#define ABSTRACTSCHEDULER_H

template <typename Task>
class abstact_sheduler
{
public:
	virtual void runRoot(Task *task) = 0;

	virtual unsigned long get_steals() {
		return 0;
	}

	virtual unsigned long get_delay() {
		return 0;
	}
};

#endif /* end of include guard: ABSTRACTSCHEDULER_H */

