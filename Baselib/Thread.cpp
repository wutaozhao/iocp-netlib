#include "Thread.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <sys/time.h>
#include <stdio.h>
#endif 

namespace wt
{
	namespace th
	{
		//FunctorHolder::~FunctorHolder() {}

		class ThreadHelper
		{
		public:
			ThreadHelper(Thread* thread)
				:mThread(thread)
			{
			}
			void Run() 
			{
				if( mThread && mThread->mHolder)
					mThread->mHolder->call();
			}
		private:
			Thread* mThread;
		};
	}




#ifdef _WIN32
static DWORD WINAPI Thread_ThreadProc(LPVOID lpParam)
{
	using namespace wt::th;
	ThreadHelper t( reinterpret_cast<Thread*>(lpParam));
	t.Run();
	return 0;
}
#else
struct PThreadContext
{
	pthread_t thread_;
	pthread_mutex_t mutex_;
	pthread_cond_t cv_;
	bool active_;
	Thread* arg_;
	PThreadContext()
	{
		pthread_mutex_init(&mutex_, NULL);
		pthread_cond_init(&cv_, NULL);
		active_ = false;
	}

	~PThreadContext()
	{
		pthread_cond_destroy(&cv_);
		pthread_mutex_destroy(&mutex_);
	}

	bool CreateThread(Thread* arg)
	{
		if( active_ )
			return false;
		arg_ = arg;
		active_ = true;
		int status = pthread_create(&thread_, NULL, &PThreadContext::Thread_ThreadProc, this);
		if( status != 0 )
		{
			active_ = false;
			return false;
		}
		return true;
	}

	void Join()
	{
		if( active_ )
			pthread_join(thread_, NULL);
	}

	bool Join(unsigned int wait)
	{
		pthread_mutex_lock(&mutex_);
		if( !active_ )
		{
			pthread_mutex_unlock(&mutex_);
			return true;
		}
		struct timespec ts;
		struct timeval tp;
		gettimeofday(&tp, NULL);

		 ts.tv_sec  = tp.tv_sec + wait / 1000;
		 ts.tv_nsec = tp.tv_usec * 1000 + (wait % 1000) * 1000 * 1000;

		int rc = pthread_cond_timedwait(&cv_, &mutex_, &ts);
		pthread_mutex_unlock(&mutex_);
		if( !active_ )
			return true;
		pthread_cancel(thread_);
		active_ = false;
		return true;
	}

	static void* Thread_ThreadProc(void* lpParam)
	{
		PThreadContext* context = (PThreadContext*)lpParam;
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

		using namespace wt::th;
		ThreadHelper t( context->arg_);
		t.Run();
		pthread_mutex_lock(&context->mutex_);
		context->active_ = false;
		pthread_mutex_unlock(&context->mutex_);
		pthread_cond_broadcast(&context->cv_);
	}


};
#endif

Thread::Thread(void)
:mHolder(0)
,mHandle(0)
,mThreadID(0)

{
}

Thread::~Thread(void)
{
	if( mHandle )
	{
		Join(1);
#ifndef _WIN32
		delete (PThreadContext*)mHandle;
#endif
	}
	delete mHolder;
}

bool Thread::Start()
{
	if( mHolder == 0 || mHandle != 0 )
		return false;

#ifdef _WIN32
	mHandle = CreateThread(NULL, 0, Thread_ThreadProc, this, 0, 0);
#else
	 PThreadContext* ctx = new PThreadContext;
	 if( !ctx )
		 return false;
	 if( !ctx->CreateThread(this) )
	 {
		 delete ctx;
		 return false;
	 }
	 mHandle = ctx;
#endif

	return true;
}



bool Thread::Join()
{
	if( mHandle )
	{
#ifdef _WIN32
		WaitForSingleObject(mHandle, INFINITE);
		CloseHandle(mHandle);
		mHandle = NULL;
		mThreadID = 0;
#else
		PThreadContext* ctx = (PThreadContext*)mHandle;
		ctx->Join();
#endif
		
	}
	return true;
}

bool Thread::Join(unsigned int wait)
{
	bool bOk = true;
	if( mHandle )
	{
#ifdef _WIN32
		if( WAIT_OBJECT_0 != WaitForSingleObject(mHandle, wait) )
		{
			TerminateThread(mHandle, 0);
			bOk = false;
		}
		CloseHandle(mHandle);
		mHandle = NULL;
		mThreadID = 0;
#else
		PThreadContext* ctx = (PThreadContext*)mHandle;
		ctx->Join(wait);
#endif
	}
	return bOk;
}

void globalDestructor(void *value)
{
}

ThreadLocal::ThreadLocal()
{
#ifdef _WIN32
	mTlsIndex = TlsAlloc();
	if (mTlsIndex == TLS_OUT_OF_INDEXES)
		mTlsIndex = TLS_OUT_OF_INDEXES;
#else
	mKey = new pthread_key_t;
	pthread_key_create(mKey, globalDestructor);
#endif
}

ThreadLocal::~ThreadLocal()
{
#ifdef _WIN32
	if (mTlsIndex != TLS_OUT_OF_INDEXES)
		TlsFree(mTlsIndex);
#else
	pthread_key_delete(*mKey);
	delete mKey;
#endif
}

bool ThreadLocal::Store(void* ptr)
{
#ifdef _WIN32
	if (mTlsIndex == TLS_OUT_OF_INDEXES)
		return false;
	return TlsSetValue(mTlsIndex, ptr) == TRUE;
#else
	return pthread_setspecific(*mKey, ptr) == 0;
#endif
}

void* ThreadLocal::Get()
{
#ifdef _WIN32
	if (mTlsIndex == TLS_OUT_OF_INDEXES)
		return NULL;
	return TlsGetValue(mTlsIndex);
#else
	return pthread_getspecific(*mKey);
#endif
}

}