#pragma once

#include <assert.h>
template <class T>
class Singleton
{
public:
	Singleton()
	{
		assert(msInstance == 0);

		msInstance = static_cast<T*>(this);
	}

	virtual ~Singleton()
	{
		msInstance = 0;
	}

	static T* Instance()
	{
		assert(msInstance != 0);
		return msInstance;
	}
private:
	static T* msInstance;
};

template<class T> T* Singleton<T>::msInstance = 0;
