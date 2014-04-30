//          Copyright John R. Bandela 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#ifndef INCLUDE_GUARD_7f279f29_bae1_46e9_8aa9_846e6390f844
#define INCLUDE_GUARD_7f279f29_bae1_46e9_8aa9_846e6390f844

#include <cppcomponents/cppcomponents.hpp>
#include <cppcomponents/function.hpp>

namespace cppcomponents_async_coroutine_wrapper{
	struct ICoroutineVoidPtr : public cppcomponents::define_interface <cppcomponents::uuid<0xfd6ff1f1, 0x332c, 0x49ff, 0x889c, 0x8181868ee543>>{

	void* Get();
	void Call(void*);

	cppcomponents::use<cppcomponents::InterfaceUnknown> GetOtherCoroutine();

	CPPCOMPONENTS_CONSTRUCT(ICoroutineVoidPtr, Get, Call, GetOtherCoroutine);

	CPPCOMPONENTS_INTERFACE_EXTRAS(ICoroutineVoidPtr){
		typedef cppcomponents::use<ICoroutineVoidPtr> CallerType;
		void operator()(void* v){
			this->get_interface().Call(v);
		}


	};


};

typedef cppcomponents::delegate < void(cppcomponents::use<ICoroutineVoidPtr>), cppcomponents::uuid < 0x4b9d05cd , 0xbafb , 0x4dc8 , 0x8fca , 0x13f0f41125a3>> CoroutineHandler;

struct ICoroutineVoidPtrFactory : public cppcomponents::define_interface < cppcomponents::uuid < 0x23dd8862 , 0x038f , 0x4eea , 0x898b , 0x1c4ce25a6d45>> {
	cppcomponents::use<cppcomponents::InterfaceUnknown> Create(cppcomponents::use<CoroutineHandler>,void*);

	CPPCOMPONENTS_CONSTRUCT(ICoroutineVoidPtrFactory, Create);
};

inline std::string CoroutineVoidPtrId(){ return "cppcomponents_concurrency!CoroutineVoidPtr"; }

typedef cppcomponents::runtime_class<CoroutineVoidPtrId, cppcomponents::object_interfaces<ICoroutineVoidPtr>, cppcomponents::factory_interface<ICoroutineVoidPtrFactory>> CoroutineVoidPtr_t;
typedef cppcomponents::use_runtime_class<CoroutineVoidPtr_t> CoroutineVoidPtr;




struct ICoroutineStatic : public cppcomponents::define_interface<cppcomponents::uuid<0xa12a71a6, 0x0cd3, 0x4645, 0xa284, 0x9e0d99d1ff7e>>
{
	void* GetThreadLocalAwaiter();
	void SetThreadLocalAwaiter(void* v);

	CPPCOMPONENTS_CONSTRUCT(ICoroutineStatic,GetThreadLocalAwaiter,SetThreadLocalAwaiter)
};
inline std::string CoroutineStaticId(){ return "cppcomponents_concurrency!CoroutineStatic"; }
typedef cppcomponents::runtime_class<CoroutineStaticId, cppcomponents::static_interfaces<ICoroutineStatic>, cppcomponents::factory_interface<cppcomponents::NoConstructorFactoryInterface>> Coroutine_t;
typedef cppcomponents::use_runtime_class<Coroutine_t> Coroutine;
}
#endif