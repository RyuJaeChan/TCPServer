#pragma once

#include <mutex>
#include <iostream>

#define TEMPLATE_DEFINE(type)  template<typename T> type SingleTon<T>

template<typename T>
class SingleTon {
private:
	static T* instance;
	static std::mutex mtx;

	static void Release();
protected:
	SingleTon() = default;
	virtual ~SingleTon() = default;
public:
	SingleTon(const SingleTon&) = delete;
	SingleTon(const SingleTon&&) = delete;
	const SingleTon& operator=(const SingleTon&) = delete;
	const SingleTon& operator=(const SingleTon&&) = delete;

	static T* GetInstance();
};
TEMPLATE_DEFINE(T*)::instance = nullptr;
TEMPLATE_DEFINE(std::mutex)::mtx;

TEMPLATE_DEFINE(T*)::GetInstance() {
	mtx.lock();
	if (!instance) {
		instance = new T;
	}
	mtx.unlock();
	return instance;
}
TEMPLATE_DEFINE(void)::Release() {
	if (instance) {
		delete instance;
		instance = nullptr;
	}
}

#undef TEMPLATE_DEFINE