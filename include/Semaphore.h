#pragma once

#include "Config.h"

WT_BEGIN

class CSemaphore
{
public:
	CSemaphore()
	{
		m_hSem = NULL;
		Create();
	}
	~CSemaphore()
	{
		::CloseHandle(m_hSem);
		m_hSem = NULL;
	}

	bool Create(long initCount = 0) {
		Close();

		m_hSem = ::CreateSemaphore(0, initCount, 0x7fffffff, 0);
		return m_hSem != NULL;
	}

	bool Wait(unsigned int timeoutMs)
	{
		DWORD r = ::WaitForSingleObject(m_hSem, timeoutMs);
		return r == WAIT_OBJECT_0;
	}

	void Post()
	{
		::ReleaseSemaphore(m_hSem, 1, 0);
	}

	void Close() {
		if (m_hSem != NULL) {
			CloseHandle(m_hSem);
			m_hSem = NULL;
		}
	}

private:
	HANDLE m_hSem;
};

WT_END