#pragma once

#include "Config.h"

WT_BEGIN

class NetCore {
public:
	NetCore();

	~NetCore();

	/**
	* @brief start net service
	*
	* @param threadCount            if pass 0, will use cpu processor count
	*/
	int Initialize(int workThreadCount = 0);

	void Uninitialize();

	void* GetCore();

private:
	void* mCore;
};

WT_END
