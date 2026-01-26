#pragma once

#include <functional>

namespace wt {
namespace th {
	class ThreadHelper;
	struct FunctorHolder
	{
		virtual ~FunctorHolder() {};
		virtual void call() = 0;
	};
	
	template<class T, typename Func>
	struct FunctorHolderArg0: public FunctorHolder
	{
		T* obj;
		Func func;
		FunctorHolderArg0(T* obj, Func func)
		{
			this->obj = obj;
			this->func = func;
		}
		void call()
		{
			(obj->*func)();
		}
	};

	template<class T, typename Func, class T1>
	struct FunctorHolderArg1: public FunctorHolder
	{
		T* obj;
		Func func;
		T1 arg;
		FunctorHolderArg1(T* obj, Func func, T1 arg)
		{
			this->obj = obj;
			this->func = func;
			this->arg = arg;
		}
		void call()
		{
			(obj->*func)(arg);
		}
	};
}

class Thread
{
public:
	friend class wt::th::ThreadHelper;
	typedef unsigned int ThreadIdType;
	Thread(void);

	template<class T, typename Func>
	Thread(T* obj, Func func);

	template<class T, typename Func, class T1>
	Thread(T* obj, Func func, T1 arg);

	~Thread(void);

	bool Start();

	template<class T, typename Func>
	bool Start(T* obj, Func func);

	template<class T, typename Func, class T1>
	bool Start(T* obj, Func func, T1 arg);

	bool Join();

	bool Join(unsigned int wait);
private:
	Thread(const Thread&);
	Thread& operator=(const Thread&);
	wt::th::FunctorHolder* mHolder;
	void* mHandle;
	ThreadIdType mThreadID;
};



template<class T, typename Func>
Thread::Thread(T* obj, Func func)
	:mHolder(0)
	,mHandle(0)
	,mThreadID(0)
{
	mHolder = new wt::th::FunctorHolderArg0<T, Func>(obj, func);
}

template<class T, typename Func, class T1>
Thread::Thread(T* obj, Func func, T1 arg)
	:mHolder(0)
	,mHandle(0)
	,mThreadID(0)
{
	mHolder = new wt::th::FunctorHolderArg1<T, Func, T1>(obj, func, arg);
}




template<class T, typename Func>
bool Thread::Start(T* obj, Func func)
{
	if( mHandle != 0 )
		return false;
	if( mHolder ) delete mHolder;
	mHolder = new wt::th::FunctorHolderArg0<T, Func>(obj, func);
	return Start();
}

template<class T, typename Func, class T1>
bool Thread::Start(T* obj, Func func, T1 arg)
{
	if( mHandle != 0 )
		return false;
	if( mHolder ) delete mHolder;
	mHolder = new wt::th::FunctorHolderArg1<T, Func, T1>(obj, func, arg);
	return Start();
}

class ThreadLocal
{
public:
	ThreadLocal();
	~ThreadLocal();
	bool Store(void* ptr);
	void* Get();
private:
	ThreadLocal(const ThreadLocal&);
	ThreadLocal& operator=(const ThreadLocal&);
	void* mRep;

};
}
