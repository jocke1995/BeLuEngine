#ifndef THREAD_H
#define THREAD_H

class MultiThreadedTask;

#include <queue>
#include <condition_variable>
#include <mutex>

#include "../Misc/EngineStatistics.h"

class Thread
{
public:
	Thread(
		std::deque<MultiThreadedTask*>* jobQueue,
		std::mutex* mutex,
		std::condition_variable* workerThreadCV,
		std::condition_variable* mainThreadCV,
		unsigned int threadId);
	~Thread();

private:
	friend class ThreadPool;
	HANDLE m_ThreadHandle;

	// Reference to the same queue in threadpool, all threads share the same jobqueue
	std::deque<MultiThreadedTask*>* m_JobQueue;
	std::mutex* m_Mutex;
	std::condition_variable* m_pWorkerThreadCV;
	std::condition_variable* m_pMainThreadCV;

	static unsigned int __stdcall threadFunc(void* lpParameter);

	MultiThreadedTask* m_pActiveTask = nullptr;

	bool m_IsExiting = false;
	unsigned int m_ThreadId = 0;

	TODO("Wrap into editorMode");
	// Used for debug statistics
	IM_ThreadStats* m_pStatistics = nullptr;
};
#endif
