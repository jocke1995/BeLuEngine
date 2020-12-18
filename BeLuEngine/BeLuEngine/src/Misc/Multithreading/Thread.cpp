#include "stdafx.h"
#include "Thread.h"
#include "MultiThreadedTask.h"
#include "../Misc/Timer.h"

unsigned int __stdcall Thread::threadFunc(void* threadParam)
{
	Thread* t = static_cast<Thread*>(threadParam);
	while (t->m_IsExiting == false)
	{
		{
			// Lock
			std::unique_lock<std::mutex> lock(*(t->m_Mutex));
			
			// Wait until jobQueue isn't empty, or until we are exiting the program
			t->m_conditionVariable->wait(lock,
				[=]
			{
				return (t->m_JobQueue->empty() == false || t->m_IsExiting == true);
			});

			if (t->m_IsExiting == true)
			{
				continue;
			}
			
			t->m_pActiveTask = t->m_JobQueue->front();
			t->m_JobQueue->pop();
		}

		t->m_pActiveTask->Execute();
		t->m_pActiveTask = nullptr;
	}

	Log::Print("Engine thread with id:%d Exiting!\n", t->m_ThreadId);
	return 0;
}

Thread::Thread(
	std::queue<MultiThreadedTask*>* jobQueue,
	std::mutex* mutex,
	std::condition_variable* conditionVariable,
	unsigned int threadId)
{
	m_JobQueue = jobQueue;
	m_Mutex = mutex;
	m_conditionVariable = conditionVariable;

	m_ThreadId = threadId;
	// Create and start the thread function
	m_ThreadHandle = reinterpret_cast<HANDLE>(_beginthreadex(0, 0, threadFunc, static_cast<void*>(this), 0, 0));

	// Set Thread Priority
	if (SetThreadPriority(m_ThreadHandle, THREAD_PRIORITY_TIME_CRITICAL) == false)
	{
		Log::PrintSeverity(Log::Severity::CRITICAL, "Failed to 'SetThreadPriority' belonging to a thread with id: %d\n", m_ThreadId);
	}
}

Thread::~Thread()
{
	if (CloseHandle(m_ThreadHandle) == false)
	{
		Log::PrintSeverity(Log::Severity::WARNING, "Failed to 'CloseHandle' belonging to a thread with id: %d\n", m_ThreadId);
	}
}
