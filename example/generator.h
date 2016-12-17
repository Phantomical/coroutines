#pragma once

#include <tuple>
#include <functional>
#include <cassert>
#include <memory>

template<typename T>
class generator;
typedef struct _coroutine coroutine;


namespace impl
{
	template<typename T>
	struct gen_wrapper
	{
#if __cplusplus <= 199711
		static generator<T>* _generator;
#else 
		thread_local generator<T>* _generator;
#endif
	};
	template<typename T>
	generator<T>* gen_wrapper<T>::_generator = NULL;

	void* yield(coroutine* ctx, void* ptr);
	void* next(coroutine* ctx, void* ptr);
	bool is_complete(const coroutine* ctx);

	coroutine* start(void(*func)(void*), size_t stack_size);
	coroutine* start(void(*func)(void*), size_t stack_size, void* stackmem);
}

template<typename T>
class generator// : std::enable_shared_from_this<generator<T>>
{
private:
	struct init_args
	{
		generator* gen;
		std::function<void()> func;
	};

	coroutine* ctx;
	const T* ptr;

	static void coroutine_base(void* ptr)
	{
		init_args* args = (init_args*)ptr;
		auto gen = args->gen;
		auto fn = args->func;

		args->gen->yield(T());

		impl::gen_wrapper<T>::_generator = gen;

		fn();
	}

public:
	void yield(const T& val)
	{
		T* ptr = const_cast<T*>(&val);

		auto gen = impl::gen_wrapper<T>::_generator;
		impl::gen_wrapper<T>::_generator = nullptr;
		impl::yield(ctx, ptr);
		impl::gen_wrapper<T>::_generator = gen;
	}
	bool next()
	{
		auto gen = impl::gen_wrapper<T>::_generator;
		ptr = (const T*)impl::next(ctx, nullptr);
		impl::gen_wrapper<T>::_generator = gen;
		return !complete();
	}
	const T& value() const
	{
		return *ptr;
	}

	bool complete() const
	{
		return impl::is_complete(ctx);
	}
	
	generator(const std::function<void()>& func, size_t stack_size) :
		ptr(nullptr)
	{
		init_args args = {
			this,
			func
		};
		ctx = impl::start(&coroutine_base, stack_size);

		impl::next(ctx, &args);
		next();
	}
	generator(const std::function<void()>& func, size_t stack_size, void* stackmem) :
		ptr(nullptr)
	{
		init_args args = {
			this,
			func
		};
		ctx = impl::start(&coroutine_base, stack_size, stackmem);

		impl::next(ctx, &args);
		next();
	}
};

template<typename T>
struct generator_iterator
{
private:
	std::shared_ptr<generator<T>> gen;

	bool complete;

public:
	generator_iterator& operator++()
	{
		complete = !gen->next();

		return *this;
	}

	const T& operator*() const
	{
		return gen->value();
	}

	bool operator==(const generator_iterator& it) const
	{
		return it.complete == complete && it.gen == gen;
	}
	bool operator!=(const generator_iterator& it) const
	{
		return !(it == *this);
	}

	generator_iterator(const std::shared_ptr<generator<T>>& gen, bool complete = false) :
		gen(gen),
		complete(complete || gen->complete())
	{

	}
};

template<typename T>
struct gen_wrapper
{
	std::shared_ptr<generator<T>> gen;

	typename generator_iterator<T> begin()
	{
		return generator_iterator<T>{ gen, false };
	}
	typename generator_iterator<T> end()
	{
		return generator_iterator<T>{ gen, true };
	}
};

template<typename T>
gen_wrapper<T> make_generator(const std::function<void()>& func, size_t stack_size = 1024 * 128)
{
	return{ std::make_shared<generator<T>>(func, stack_size) };
}
template<typename T>
gen_wrapper<T> make_generator(const std::function<void()>& func, size_t stack_size, void* mem)
{
	return{ std::make_shared<generator<T>>(func, stack_size, mem) };
}

#ifndef GENERATOR_IMPL
template<typename T>
void yield(const T& val)
{
	// Check to make sure that we are executing a coroutine
	assert(impl::gen_wrapper<T>::_generator != nullptr);

	impl::gen_wrapper<T>::_generator->yield(val);
}
#endif
