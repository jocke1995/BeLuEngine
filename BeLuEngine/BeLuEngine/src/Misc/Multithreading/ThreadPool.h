#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "Core.h"
#include "Thread.h"
#include <vector>

class MultiThreadedTask;

class ThreadPool
{
public:
	~ThreadPool();

	void WaitForThreads(unsigned int flag);

	void AddTask(MultiThreadedTask* task);

	static ThreadPool& GetInstance(unsigned int nrOfThreads = 0);
	unsigned int GetNrOfThreads() const;
private:
	ThreadPool(unsigned int nrOfThreads);
	std::vector<Thread*> m_Threads;

	std::queue<MultiThreadedTask*> m_JobQueue;
	std::mutex m_Mutex;
	std::condition_variable m_conditionVariable;

	unsigned int m_NrOfThreads;
	unsigned int m_ThreadCounter = 0;

	void exitThreads();
	bool isAllThreadsWaiting();
};

#endif
