#include "stdafx.h"
#include "ThreadPool.h"
#include "Thread.h"
#include "MultiThreadedTask.h"

ThreadPool::ThreadPool(unsigned int nrOfThreads)
{
	m_NrOfThreads = nrOfThreads;
	
	// Create Threads
	for (int i = 0; i < m_NrOfThreads; i++)
	{
		m_Threads.push_back(new Thread(
								&m_JobQueue, &m_Mutex,
								&m_workerThreadConditionVariable,
								&m_MainThreadConditionVariable,
								i));
	}
}

ThreadPool::~ThreadPool()
{
	// Make sure threads exit the thread-loop
	exitThreads();

	std::vector<HANDLE> handles;
	for (Thread* thread : m_Threads)
	{
		handles.push_back(thread->m_ThreadHandle);
	}

	// Make sure all threads have exited the thread-func
	WaitForMultipleObjects(m_NrOfThreads, handles.data(), true, INFINITE);

	// Delete thread objects
	for (Thread* thread : m_Threads)
	{
		delete thread;
	}
}

// FLAG_THREAD::
void ThreadPool::WaitForThreads(unsigned int flag)
{
	{
		// Lock
		std::unique_lock<std::mutex> lock(m_Mutex);

		// Wait until jobQueue is empty of jobs with specified flag
		m_MainThreadConditionVariable.wait(lock,
			[=]
		{
			bool isJobQueueEmpty = isQueueEmpty(flag);
			return (isJobQueueEmpty == true);
		});


		// Wait until all threads are done with their final tasks with the specified flag
		m_MainThreadConditionVariable.wait(lock,
			[=]
		{
			bool isAllThreadsSleeping = isAllThreadsWaiting(flag);
			return (isAllThreadsSleeping == true);
		});
	}
}

void ThreadPool::AddTask(MultiThreadedTask* task)
{
	// Adds a m_pTask to a m_Thread
	{
		m_Mutex.lock();
		m_JobQueue.push_back(task);
		m_workerThreadConditionVariable.notify_one();
		m_Mutex.unlock();
	}
}

ThreadPool& ThreadPool::GetInstance(unsigned int nrOfThreads)
{
	static ThreadPool instance(nrOfThreads);
	return instance;
}

unsigned int ThreadPool::GetNrOfThreads() const
{
	return m_NrOfThreads;
}

void ThreadPool::exitThreads()
{
	m_Mutex.lock();
	for (unsigned int i = 0; i < m_Threads.size(); i++)
	{
		m_Threads[i]->m_IsExiting = true;
	}
	m_Mutex.unlock();

	m_workerThreadConditionVariable.notify_all();
	WaitForThreads(FLAG_THREAD::ALL);
}

bool ThreadPool::isQueueEmpty(unsigned int flag)
{
	// Is there no tasks in queue?
	if (m_JobQueue.empty() == true)
	{
		return true;
	}

	// If there is tasks in queue, check for jobs with specified flag
	for (MultiThreadedTask* task : m_JobQueue)
	{
		if (task->GetThreadFlags() & flag)
		{
			// We found a job with specified flag thats yet not completed.. still task to do.
			return false;
		}
	}
	return true;
}

bool ThreadPool::isAllThreadsWaiting(unsigned int flag)
{

	for (unsigned int i = 0; i < m_Threads.size(); i++)
	{
		// If the thread isn't finished with its current job
		if (m_Threads[i]->m_pActiveTask != nullptr)
		{
			// If the task if the type specified by the incoming flag
			if (m_Threads[i]->m_pActiveTask->GetThreadFlags() & flag)
			{
				return false;
			}
		}
	}
	return true;
}
