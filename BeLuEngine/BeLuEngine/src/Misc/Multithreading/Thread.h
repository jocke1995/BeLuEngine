#ifndef THREAD_H
#define THREAD_H

class MultiThreadedTask;

#include <queue>
#include <condition_variable>
#include <mutex>

class Thread
{
public:
	Thread(
		std::queue<MultiThreadedTask*>* jobQueue,
		std::mutex* mutex,
		std::condition_variable* conditionVariable,
		unsigned int threadId);
	~Thread();

private:
	friend class ThreadPool;
	HANDLE m_ThreadHandle;

	std::queue<MultiThreadedTask*>* m_JobQueue;
	std::mutex* m_Mutex;
	std::condition_variable* m_conditionVariable;

	static unsigned int __stdcall threadFunc(void* lpParameter);

	MultiThreadedTask* m_pActiveTask = nullptr;

	bool m_IsExiting = false;
	unsigned int m_ThreadId = 0;
};
#endif
