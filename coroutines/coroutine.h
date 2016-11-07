#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif
	typedef struct _context context;
	typedef struct _coroutine
	{
		size_t stack_size;
		void(*funcptr)(void* datap);
	} coroutine;

	// Yields the coroutine and returns control to the caller
	// passing datap back to the caller. The return value is
	// the pointer that was passed into the coroutine.
	void* yield(context* ctx, void* datap);

	// Starts a coroutine with a given context but doesn't begin executing it.
	context* start(coroutine initdata, void* datap);
	// Performs context cleanup and completes execution
	// of the coroutine
	void destroy(context* ctx, void* datap);
	// Performs context cleanup without completing
	// execution of the coroutine.
	// WARNING: Calling this method will cause the
	// coroutine to never finish executing, this
	// can cause memory leaks and essential cleanup
	// to never be executed.
	void abort(context* ctx);

	// Executes the coroutine to the next yield call
	// with a provided data pointer. If the coroutine
	// is completed then it returns the last yielded
	// value instead.
	void* next(context* ctx, void* datap);

	// Returns 1 if the coroutine is complete, 0 otherwise.
	char is_complete(const context* ctx);
#ifdef __cplusplus
}
#endif