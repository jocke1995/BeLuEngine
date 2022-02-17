#include "stdafx.h"
#include "Thread.h"

#include "MultiThreadedTask.h"
#include "../Misc/Timer.h"

unsigned int __stdcall Thread::threadFunc(void* threadParam)
{
	Thread* t = static_cast<Thread*>(threadParam);

// Statistics info
	TODO("Wrap this in a EditorConfig");
#ifdef DEBUG
	{
		std::unique_lock<std::mutex> lock(*(t->m_Mutex));

		std::stringstream ss;
		ss << std::this_thread::get_id();
		t->m_pStatistics->m_Id = std::stoull(ss.str());
	}
	
#endif

	while (t->m_IsExiting == false)
	{
		{
			// Lock
			std::unique_lock<std::mutex> lock(*(t->m_Mutex));
			
			// Check if queue is empty, if it is, signal main thread so it wakes up
			t->m_pMainThreadCV->notify_one();

			// Wait until jobQueue isn't empty, or until we are exiting the program
			t->m_pWorkerThreadCV->wait(lock,
				[=]
			{
				return (t->m_JobQueue->empty() == false || t->m_IsExiting == true);
			});

			if (t->m_IsExiting == true)
			{
				continue;
			}
			
			t->m_pActiveTask = std::move(t->m_JobQueue->front());
			t->m_JobQueue->pop_front();
		}

		{
			// Run thread task
			t->m_pActiveTask->Execute();

			TODO("Wrap this in a EditorConfig");
			t->m_pStatistics->m_TasksCompleted++;

			// Lock
			std::unique_lock<std::mutex> lock(*(t->m_Mutex));
			t->m_pActiveTask = nullptr;

			// Notify main thread that the job is done
			t->m_pMainThreadCV->notify_one();
		}
	}

	Log::Print("Engine thread with id:%d Exiting!\n", t->m_ThreadId);
	return 0;
}

Thread::Thread(
	std::deque<MultiThreadedTask*>* jobQueue,
	std::mutex* mutex,
	std::condition_variable* workerThreadCV,
	std::condition_variable* mainThreadCV,
	unsigned int threadId)
{
	m_JobQueue = jobQueue;
	m_Mutex = mutex;
	m_pWorkerThreadCV = workerThreadCV;
	m_pMainThreadCV = mainThreadCV;
	m_ThreadId = threadId;

	TODO("Wrap this in a EditorConfig");
	m_pStatistics = new IM_ThreadStats();

	// Create and start the thread function
	m_ThreadHandle = reinterpret_cast<HANDLE>(_beginthreadex(0, 0, threadFunc, static_cast<void*>(this), 0, 0));

	// Set Thread Priority
	if (SetThreadPriority(m_ThreadHandle, THREAD_PRIORITY_TIME_CRITICAL) == false)
	{
		BL_LOG_CRITICAL("Failed to 'SetThreadPriority' belonging to a thread with id: %d\n", m_ThreadId);
	}

	TODO("Wrap this in a EditorConfig");
	EngineStatistics::GetIM_ThreadStats().push_back(m_pStatistics);
}

Thread::~Thread()
{
	if (CloseHandle(m_ThreadHandle) == false)
	{
		BL_LOG_WARNING("Failed to 'CloseHandle' belonging to a thread with id: %d\n", m_ThreadId);
	}

	TODO("Wrap this in a EditorConfig");
	delete m_pStatistics;
}
