
#include "net/NetCore.h"
#include "NetCoreIOCP.h"
#include "tool/util.h"

WT_BEGIN

NetCore::NetCore():mCore(NULL)
{

}

NetCore::~NetCore()
{

}

int NetCore::Initialize(int workThreadCount)
{
	int ret = 0;

	do
	{
		mCore = new(std::nothrow) NetCoreIOCP();
		if (!mCore) {
			ret = NSE_SYSTEM_ERROR;
			break;
		}

		NetCoreIOCP* coreIOCP = reinterpret_cast<NetCoreIOCP*>(mCore);
		ret = coreIOCP->Initialize(workThreadCount);

	} while (false);

	if (ret != 0) {
		Uninitialize();
	}

	return ret;
}

void NetCore::Uninitialize()
{
	if (mCore != NULL) {
		NetCoreIOCP* coreIOCP = reinterpret_cast<NetCoreIOCP*>(mCore);
		if (coreIOCP) {
			coreIOCP->UnInitialize();
			SafeDelete(coreIOCP);
			mCore = NULL;
		}
	}
}

void* NetCore::GetCore()
{
	return mCore;
}

WT_END