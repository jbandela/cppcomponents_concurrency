//          Copyright John R. Bandela 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include "cppcomponents_async_coroutine_wrapper.hpp"

using namespace cppcomponents;
using namespace cppcomponents_async_coroutine_wrapper;
#define BOOST_COROUTINES_OLD

#include <boost/coroutine/all.hpp>
#include <stdio.h>

typedef boost::coroutines::coroutine<void*(void*) > co_type;


// Visual C++ does not yet implement
// thread-safe static initializations
// boost::coroutines::detail::system_info looks like this
//SYSTEM_INFO system_info()
//{
//	static SYSTEM_INFO si = system_info_();
//	return si;
//}
// However, because Visual C++ does not have thread-safe
// static initialization, there is a race. However, if we
// call this function ourselves, before any other call to the function
// then si get initialized properly without a race
// We only need to do this with MSVC
#ifdef _MSC_VER
#include <windows.h>

namespace boost {
	namespace coroutines {
		namespace detail {



			SYSTEM_INFO system_info();

		}
	}
}

SYSTEM_INFO info = boost::coroutines::detail::system_info();

#endif

inline std::string CoroutineCallerId(){ return "cppcomponents_concurrency!CoroutineCaller"; }

typedef cppcomponents::runtime_class<CoroutineCallerId, object_interfaces<ICoroutineVoidPtr>, factory_interface<NoConstructorFactoryInterface>> Caller_t;

struct ImplementCaller : public implement_runtime_class < ImplementCaller, Caller_t>
{
	cppcomponents::portable_base* pco_;
	co_type* ca_;
	ImplementCaller(co_type* ca, cppcomponents::portable_base* p) : ca_(ca), pco_(p){};

	void* Get(){
		return ca_->get();
	}

	void Call(void* v){

		(*ca_)(v);
	}


	cppcomponents::use<cppcomponents::InterfaceUnknown> GetOtherCoroutine(){
		cppcomponents::use<cppcomponents::InterfaceUnknown> ret(cppcomponents::reinterpret_portable_base<cppcomponents::InterfaceUnknown>(pco_), true);
		return ret;
	}


};
CPPCOMPONENTS_REGISTER(ImplementCaller)

std::atomic<int> g_i = 0;
struct ImplementCoroutineVoidPtr : public implement_runtime_class < ImplementCoroutineVoidPtr, CoroutineVoidPtr_t>
{
	co_type co_;
	ImplementCoroutineVoidPtr(use<CoroutineHandler> h, void* v) : 
		co_(
		[h](co_type& ca)mutable{
		auto pthis = static_cast<ImplementCoroutineVoidPtr*>(ca.get());
		ca(nullptr);
		
		h(ImplementCaller::create(&ca, pthis->get_unknown_portable_base()).QueryInterface<ICoroutineVoidPtr>());
	}, this)
	{
		
		co_(v);
		printf("In ctor %d\n", ++g_i);
	};
	~ImplementCoroutineVoidPtr(){
		printf("In dtor %d\n", --g_i);
	}
	void* Get(){
		return co_.get();
	}

	void Call(void* v){
		co_(v);
	}

	cppcomponents::use<cppcomponents::InterfaceUnknown> GetOtherCoroutine(){
		return nullptr;
	}




};
CPPCOMPONENTS_REGISTER(ImplementCoroutineVoidPtr)


// From http://stackoverflow.com/questions/18298280/how-to-declare-a-variable-as-thread-local-portably
#ifdef __GNUC__
# define CPPCOMPONENTS_THREAD_LOCAL __thread
#elif defined(_MSC_VER)
# define CPPCOMPONENTS_THREAD_LOCAL __declspec(thread)
#else
#  CPPCOMPONENTS_THREAD_LOCAL thread_local
#endif

CPPCOMPONENTS_THREAD_LOCAL void* tls_awaiter = nullptr;
struct ImplementCoroutineStatic : public implement_runtime_class<ImplementCoroutineStatic, Coroutine_t>{
	static void* GetThreadLocalAwaiter(){
		return tls_awaiter;
	}
	static void SetThreadLocalAwaiter(void* v){
		tls_awaiter = v;
	}
};

CPPCOMPONENTS_REGISTER(ImplementCoroutineStatic)

CPPCOMPONENTS_DEFINE_FACTORY();