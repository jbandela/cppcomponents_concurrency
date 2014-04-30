
// Copyright (c) 2013 John R. Bandela
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once
#ifndef INCLUDE_GUARD_CPPCOMPONENTS_RESUMABLE_AWAIT_HPP_
#define INCLUDE_GUARD_CPPCOMPONENTS_RESUMABLE_AWAIT_HPP_

#include <cppcomponents/future.hpp>
#include "implementation/cppcomponents_async_coroutine_wrapper.hpp"

#include <memory>
#include <exception>
#include <utility>
#include <assert.h>
#include <type_traits>
#include <functional>


#ifdef PPL_HELPER_OUTPUT_ENTER_EXIT
#include <cstdio>
#include <atomic>
#define PPL_HELPER_ENTER_EXIT ::cppcomponents::detail::EnterExit ppl_helper_enter_exit_var;
#else
#define PPL_HELPER_ENTER_EXIT
#endif

namespace cppcomponents{
	namespace detail{
#ifdef PPL_HELPER_OUTPUT_ENTER_EXIT
		// Used for debugging to make sure all functions are exiting correctly
		struct EnterExit{
			EnterExit() : n_(++number()){ increment(); std::printf("==%d Entering\n", n_); }
			int n_;
			std::string s_;
			static int& number(){ static int number = 0; return number; }
			static std::atomic<int>& counter(){ static std::atomic<int> counter_ = 0; return counter_; }
			static void increment(){ ++counter(); }
			static void decrement(){ --counter(); }
			static void check_all_destroyed(){ assert(counter() == 0); if (counter())throw std::exception("Not all EnterExit destroyed"); };
			~EnterExit(){ decrement(); std::printf("==%d Exiting\n", n_); }

		};
#endif
#undef PPL_HELPER_OUTPUT_ENTER_EXIT

#pragma pack(push, 1)
		struct ret_type{
			std::int32_t error_;
			void* pv_;

			template<class T>
			T get(){
				if (error_){
					cppcomponents::error_mapper::exception_from_error_code(error_);
				}
				auto p = static_cast<cppcomponents::portable_base*>(pv_);
				T ret{ cppcomponents::reinterpret_portable_base<typename T::interface_t>(p), true };
				return ret;
			}
		};

		struct co_prev_holder{
			co_prev_holder* prev_;
			co_prev_holder* next_;
			cppcomponents::portable_base* p_;
		};
#pragma pack(pop)

		typedef cppcomponents::function < void ()> awaiter_func_type;

		void execute_awaiter_func(void* v){
			if (!v) return;
			auto p = static_cast<cppcomponents::portable_base*>(v);
			awaiter_func_type f{ cppcomponents::reinterpret_portable_base<awaiter_func_type::interface_t>(p), true };
			f();
		}


	}



	class awaiter{
		typedef detail::awaiter_func_type func_type;
		typedef use<cppcomponents_async_coroutine_wrapper::ICoroutineVoidPtr> CoType;
		detail::co_prev_holder pholder;

		template<class R>
		static func_type get_function(CoType co1,  use < IFuture < R >> t){
			func_type  retfunc = cppcomponents::make_function<func_type>([co1, t]()mutable{
				auto co = co1;
				co1 = nullptr;
				t.Then([co](use < IFuture < R >> et)mutable{
					detail::ret_type ret;
					ret.error_ = 0;
					ret.pv_ = nullptr;
					ret.pv_ = et.get_portable_base();
					co(&ret);
					detail::execute_awaiter_func(co.Get());
					
				});
			});
			return retfunc;
		}
		template<class R>
		static func_type get_function(CoType co1, use<IExecutor> executor, use < IFuture < R >> t){
			func_type retfunc = cppcomponents::make_function<func_type>([co1, t,executor]()mutable{
				auto co = co1;
				co1 = nullptr;
				t.Then(executor,[co](use < IFuture < R >> et)mutable{
					detail::ret_type ret;
					ret.error_ = 0;
					ret.pv_ = nullptr;
					ret.pv_ = et.get_portable_base();
					co(&ret);
					auto p = co.Get();
					detail::execute_awaiter_func(p);
				});
			});

			return retfunc;

		}
	public:
		static detail::co_prev_holder* get_tls(){
			return static_cast<detail::co_prev_holder*>(cppcomponents_async_coroutine_wrapper::Coroutine::GetThreadLocalAwaiter());;
		}
		static void set_tls(detail::co_prev_holder* ph){
			ph->prev_ = get_tls();
			if (ph->prev_){
				ph->next_ = ph->prev_->next_;
				ph->prev_->next_ = ph;
			}
			if (ph->next_){
				ph->next_->prev_ = ph;
			}
			cppcomponents_async_coroutine_wrapper::Coroutine::SetThreadLocalAwaiter(ph);
		}

		static void reset_tls(detail::co_prev_holder* ph){
			auto ph_old = get_tls();
			if (ph_old == ph){
				cppcomponents_async_coroutine_wrapper::Coroutine::SetThreadLocalAwaiter(ph->prev_);
				if (ph->prev_){
					ph->prev_->next_ = nullptr;
				}
				ph->prev_ = nullptr;
			}
	
		}


		void remove_pholder_links(){
			if (pholder.prev_){
				pholder.prev_->next_ = pholder.next_;
			}
			if (pholder.next_){
				pholder.next_->prev_ = pholder.prev_;
			}
			pholder.prev_ = nullptr;
			pholder.next_ = nullptr;
		}

	public:
		awaiter(CoType& c)
		{
			pholder.p_ = c.get_portable_base();
			pholder.next_ = nullptr;
			pholder.prev_ = nullptr;
			set_tls(&pholder);
		}

		~awaiter(){
				reset_tls(&pholder);
				remove_pholder_links();
		}

		// Non-copyable
		//awaiter(const awaiter&) = delete;
		awaiter& operator=(const awaiter&) = delete;

		// Ugly movable
		awaiter(awaiter& other){
			pholder.next_ = other.pholder.next_;
			pholder.prev_ = other.pholder.prev_;
			pholder.p_ = other.pholder.p_;

			other.remove_pholder_links();
			if (pholder.next_){
				pholder.next_->prev_ = &pholder;
			}

			if (pholder.prev_){
				pholder.prev_->next_ = &pholder;
			}

			other.pholder.p_ = nullptr;
			if (get_tls() == &other.pholder){
				
				cppcomponents_async_coroutine_wrapper::Coroutine::SetThreadLocalAwaiter(&pholder);
			}
		}

		// Move assignment
		awaiter& operator=(awaiter&& other) = delete;
	private:
		template<class R>
		friend use<IFuture<R>> await_as_future(use < IFuture < R >> t);
		template<class R>
		friend use<IFuture<R>> await_as_future(use<IExecutor> executor, use < IFuture < R >> t);

		template<class R>
		static use<IFuture<R>> as_future(CoType ca, use < IFuture < R >> t){
			if (t.Ready()){
				return t;
			}
			auto retfunc = get_function(ca.GetOtherCoroutine().QueryInterface<cppcomponents_async_coroutine_wrapper::ICoroutineVoidPtr>(),t);
			PPL_HELPER_ENTER_EXIT;
			assert(ca);
			auto ph = awaiter::get_tls();
			reset_tls(ph);
			ca(retfunc.get_portable_base());
			set_tls(ph);
			return static_cast<detail::ret_type*>(ca.Get())->get < use < IFuture<R >> >();
		}
		template<class R>
			static use<IFuture<R>> as_future(CoType ca, use<IExecutor> executor, use < IFuture < R >> t){
				if (t.Ready()){
					return t;
				}
				auto retfunc = get_function(ca.GetOtherCoroutine().QueryInterface<cppcomponents_async_coroutine_wrapper::ICoroutineVoidPtr>(), executor, t);
				PPL_HELPER_ENTER_EXIT;
				assert(ca);
				auto ph = awaiter::get_tls();
				reset_tls(ph);
				ca(retfunc.get_portable_base());
				set_tls(ph);
				return static_cast<detail::ret_type*>(ca.Get())->get < use < IFuture<R >> >();
		}

			template<class R>
			R operator()(use < IFuture < R >> t){
				return as_future(t).Get();
			}

			template<class R>
			R operator()(use<IExecutor> executor,use < IFuture < R >> t){
				return as_future(executor,t).Get();
			}

	};
	namespace detail{

		template<class T>
		struct ret_holder{
			T value_;
			template<class FT>
			ret_holder(FT& f, awaiter h) : value_(f(h)){}
			const T& get()const{ return value_; }

			use<IFuture<T>> get_ready_future()const{ return cppcomponents::make_ready_future(value_); }
		};
		template<>
		struct ret_holder<void>{
			template<class FT>
			ret_holder(FT& f, awaiter h){
				f(h);
			}
			void get()const{}
			use<IFuture<void>> get_ready_future()const{ return cppcomponents::make_ready_future(); }

		};

		template<class F>
		class simple_async_function_holder{


			F f_;
			typedef typename std::result_of<F(awaiter)>::type return_type;
			Future<return_type> future_;


			static void coroutine_function(cppcomponents::use<cppcomponents_async_coroutine_wrapper::ICoroutineVoidPtr> ca){
				PPL_HELPER_ENTER_EXIT;;

				auto p = ca.Get();
				auto pthis = reinterpret_cast<simple_async_function_holder*>(p);
				auto promise = make_promise<return_type>();
				pthis->future_ = promise.template QueryInterface < IFuture<return_type>>();
				try{
					PPL_HELPER_ENTER_EXIT;
					awaiter helper(ca);
					promise.SetResultOf(std::bind(pthis->f_, helper));
					ca(nullptr);
				}
				catch (std::exception& e){
					auto ec = cppcomponents::error_mapper::error_code_from_exception(e);
					promise.SetError(ec);
					ca(nullptr);
				}
			}
		public:
			simple_async_function_holder(F f) : f_(f){}

			Future<return_type> run(){
				cppcomponents_async_coroutine_wrapper::CoroutineVoidPtr co(cppcomponents::make_delegate<cppcomponents_async_coroutine_wrapper::CoroutineHandler>(coroutine_function), this);
				detail::execute_awaiter_func(co.Get());

				return this->future_.template QueryInterface<IFuture<return_type>>();

			}
		};

		template<class F>
		struct return_helper{};

		template<class R>
		struct return_helper<R(awaiter)>{
			typedef R type;
		};


		template<class F>
		use<IFuture<typename std::result_of<F(awaiter)>::type>> do_async(F f){
			detail::simple_async_function_holder<F> async_func_holder(f);
			return async_func_holder.run();
		}


		template<class R, class F>
		struct do_async_functor{
			F f_;
			template<class... T>
			use<IFuture<R>> operator()(T && ... t){
				using namespace std::placeholders;
				return do_async(
					
					std::bind(f_, std::forward<T>(t)..., _1));
			}

			do_async_functor(F f) : f_{ f }{}


		};

		namespace return_type_calculator{

			// Calculate return type of callable
			// Adapted from http://stackoverflow.com/questions/11893141/inferring-the-call-signature-of-a-lambda-or-arbitrary-callable-for-make-functio
			template<class F>
			using typer = F;

			template<typename T> struct remove_class { };
			template<typename C, typename R, typename... A>
			struct remove_class<R(C::*)(A...)> { using type = typer<R(A...)>; using return_type = R; };
			template<typename C, typename R, typename... A>
			struct remove_class<R(C::*)(A...) const> { using type = typer<R(A...)>; using return_type = R; };
			template<typename C, typename R, typename... A>
			struct remove_class<R(C::*)(A...) volatile> { using type = typer<R(A...)>; using return_type = R; };
			template<typename C, typename R, typename... A>
			struct remove_class<R(C::*)(A...) const volatile> { using type = typer<R(A...)>; using return_type = R; };

			template<class T>
			using remove_reference_t = typename std::remove_reference<T>::type;
			template<typename T>
			struct get_signature_impl {
				using type = typename remove_class<
					decltype(&remove_reference_t<T>::operator())>::type;
				using return_type = typename remove_class<
					decltype(&remove_reference_t<T>::operator())>::return_type;
			};
			template<typename R, typename... A>
			struct get_signature_impl<R(A...)> { using type = typer<R(A...)>; using return_type = R; };
			template<typename R, typename... A>
			struct get_signature_impl<R(&)(A...)> { using type = typer<R(A...)>; using return_type = R; };
			template<typename R, typename... A>
			struct get_signature_impl<R(*)(A...)> { using type = typer<R(A...)>; using return_type = R; };
			template<typename T> using get_signature = typename get_signature_impl<T>::type;
			template<typename T> using return_type = typename get_signature_impl<T>::return_type;

		}
	}
	template<class F>
	auto  resumable(F f) -> detail::do_async_functor<detail::return_type_calculator::return_type<decltype(f)>, F>{
		return detail::do_async_functor<detail::return_type_calculator::return_type<F>, F>{f};
	}

	struct await_error :public std::runtime_error{
		await_error() :std::runtime_error("Await error"){}
	};

	template<class R>
	use<IFuture<R>> await_as_future(use < IFuture < R >> t){
		auto ph = awaiter::get_tls();
		assert(ph);
		if (!ph){
			throw await_error();
		}
		use<cppcomponents_async_coroutine_wrapper::ICoroutineVoidPtr> co{reinterpret_portable_base<cppcomponents_async_coroutine_wrapper::ICoroutineVoidPtr>(ph->p_),true};
		return awaiter::as_future(co, t);
		

	}
	template<class R>
	use<IFuture<R>> await_as_future(use<IExecutor> executor, use < IFuture < R >> t){
		auto ph = awaiter::get_tls();
		assert(ph);
		if (!ph){
			throw await_error();
		}
		use<cppcomponents_async_coroutine_wrapper::ICoroutineVoidPtr> co{ reinterpret_portable_base<cppcomponents_async_coroutine_wrapper::ICoroutineVoidPtr>(ph->p_), true };
		return awaiter::as_future(co,executor,t);
	}
	template<class R>
	R await(use < IFuture < R >> t){
		return await_as_future(t).Get();
	}

	template<class R>
	R await(use<IExecutor> executor, use < IFuture < R >> t){
		return await_as_future(executor, t).Get();
	}


	template<class F>
	use<IFuture<typename std::result_of<F()>::type>> co_async(use<IExecutor> e, F f){
		auto func = resumable([f](awaiter){return f(); });
		return async(e,func);


	}
	template<class F>
	use<IFuture<typename std::result_of<F()>::type>> co_async(use<IExecutor> e, use<IExecutor> then_executor, F f){
		auto func = resumable([f](awaiter){return f(); });
		return async(e,then_executor, func);
	}
	
}


#endif