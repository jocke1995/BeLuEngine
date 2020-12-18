#include "stdafx.h"
#include "ThreadPool.h"
#include "Thread.h"
ThreadPool::ThreadPool(unsigned int nrOfThreads)
{
	m_NrOfThreads = nrOfThreads;
	
	// Create Threads
	for (int i = 0; i < m_NrOfThreads; i++)
	{
		m_Threads.push_back(new Thread(&m_JobQueue, &m_Mutex, &m_conditionVariable, i));
	}
}

ThreadPool::~ThreadPool()
{
	exitThreads();

	for (Thread* thread : m_Threads)
	{
		delete thread;
	}
}

void ThreadPool::WaitForThreads(unsigned int flag)
{
	while (true)
	{
		m_Mutex.lock();
		bool isJobQueueEmpty = m_JobQueue.empty();
		bool isAllThreadsWaiting = this->isAllThreadsWaiting();
		m_Mutex.unlock();

		if (isJobQueueEmpty == true)
		{
			if (isAllThreadsWaiting == true)
			{
				break;
			}
		}
	}
}

void ThreadPool::AddTask(MultiThreadedTask* task)
{
	// Adds a m_pTask to a m_Thread
	{
		m_Mutex.lock();
		m_JobQueue.push(task);
		m_conditionVariable.notify_one();
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

	WaitForThreads(FLAG_THREAD::ALL);
	m_conditionVariable.notify_all();
}

bool ThreadPool::isAllThreadsWaiting()
{
	for (unsigned int i = 0; i < m_Threads.size(); i++)
	{
		// If the thread isn't finished with its current job
		if (m_Threads[i]->m_pActiveTask != nullptr)
		{
			return false;
		}
	}
	return true;
}
