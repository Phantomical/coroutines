#include "coroutine.h"
#include "gtest\gtest.h"

#include <utility>

#define TEST_COROUTINE_STACK_SIZE 16384
#define DETERMINISTIC_REPEAT_TIMES 4

void deterministic_test(coroutine*, void* arg)
{
	coroutine* ctx = static_cast<coroutine*>(arg);
	for (uintptr_t i = 0; i < DETERMINISTIC_REPEAT_TIMES; ++i)
	{
		coroutine_yield(ctx, reinterpret_cast<void*>(i));
	}
}

void set_var(coroutine*, void* arg)
{
	std::pair<coroutine*, int*>* vals = (std::pair<coroutine*, int*>*)arg;

	(void)coroutine_yield(vals->first, nullptr);

	*vals->second = 0xFFF;
}
void set_var_before(coroutine*, void* arg)
{
	std::pair<coroutine*, int*>* vals = (std::pair<coroutine*, int*>*)arg;
	*vals->second = 0xFFF;
}

TEST(run, yield_unmodified)
{
	coroutine* ctx = coroutine_start({ TEST_COROUTINE_STACK_SIZE, &deterministic_test });

	EXPECT_EQ(0, reinterpret_cast<uintptr_t>(coroutine_next(ctx, ctx)));

	for (uintptr_t i = 1; i < DETERMINISTIC_REPEAT_TIMES; ++i)
	{
		EXPECT_EQ(i, reinterpret_cast<uintptr_t>(
			coroutine_next(ctx, reinterpret_cast<void*>(i))
		));
	}

	coroutine_abort(ctx);
}
TEST(run, destroy_completes_coroutine)
{
	int r = 0;
	std::pair<coroutine*, int*> pair;
	pair.first = coroutine_start({ TEST_COROUTINE_STACK_SIZE, &set_var });
	pair.second = &r;

	coroutine_next(pair.first, &pair);

	coroutine_destroy(pair.first, nullptr);

	ASSERT_EQ(r, 0xFFF);
}
TEST(run, abort_does_not_complete)
{
	int r = 0;
	std::pair<coroutine*, int*> pair;
	pair.first = coroutine_start({ TEST_COROUTINE_STACK_SIZE, &set_var });
	pair.second = &r;

	coroutine_next(pair.first, &pair);

	coroutine_abort(pair.first);

	ASSERT_NE(r, 0xFFF);
}
TEST(run, is_complete)
{
	coroutine* ctx = coroutine_start({ TEST_COROUTINE_STACK_SIZE, &deterministic_test });

	EXPECT_EQ(0, reinterpret_cast<uintptr_t>(coroutine_next(ctx, ctx)));

	for (uintptr_t i = 1; i < DETERMINISTIC_REPEAT_TIMES; ++i)
	{
		EXPECT_EQ(i, reinterpret_cast<uintptr_t>(
			coroutine_next(ctx, reinterpret_cast<void*>(i))
			));
		EXPECT_FALSE(coroutine_is_complete(ctx) != 0);
	}
	(void)coroutine_next(ctx, nullptr);

	EXPECT_TRUE(coroutine_is_complete(ctx) != 0);
	coroutine_destroy(ctx, nullptr);
}
TEST(run, start_does_not_call_function)
{
	int i = 0;
	coroutine* ctx = coroutine_start({ TEST_COROUTINE_STACK_SIZE, &set_var_before });
	// Make sure that null is not returned
	ASSERT_NE(ctx, nullptr);
	std::pair<coroutine*, int*> vals = { ctx, &i };
	// Make sure i has not changed
	ASSERT_EQ(i, 0);
	coroutine_next(ctx, &vals);
	// Make sure i was set
	ASSERT_EQ(i, 0xFFF);

	coroutine_abort(ctx);
}

TEST(start_with_mem, returns_null_on_null_memptr)
{
	coroutine* ctx = coroutine_start_with_mem({ TEST_COROUTINE_STACK_SIZE, &set_var }, nullptr);

	ASSERT_EQ(ctx, nullptr);
}
TEST(start_with_mem, returns_null_with_null_method)
{
	void* buffer = malloc(TEST_COROUTINE_STACK_SIZE);

	ASSERT_NE(buffer, nullptr);

	coroutine* ctx = coroutine_start_with_mem({ TEST_COROUTINE_STACK_SIZE, nullptr }, buffer);

	ASSERT_EQ(ctx, nullptr);

	free(buffer);
}
TEST(start_with_mem, returns_null_on_zero_stack_size)
{
	void* buffer = malloc(TEST_COROUTINE_STACK_SIZE);

	ASSERT_NE(buffer, nullptr);

	coroutine* ctx = coroutine_start_with_mem({ 0, &set_var_before }, buffer);

	ASSERT_EQ(ctx, nullptr);

	free(buffer);
}

TEST(start, returns_null_with_null_method)
{
	coroutine* ctx = coroutine_start({ TEST_COROUTINE_STACK_SIZE, nullptr });

	ASSERT_EQ(ctx, nullptr);
}
TEST(start, returns_null_on_zero_stack_size)
{
	coroutine* ctx = coroutine_start({ 0, &set_var_before });

	ASSERT_EQ(ctx, nullptr);
}

TEST(abort, succeeds_with_null)
{
	coroutine_abort(nullptr);

	// If we reached here then we didn't segfault
	SUCCEED();
}

TEST(destroy, succeeds_with_null)
{
	coroutine_destroy(nullptr, nullptr);

	// If we reached here then we didn't segfault
	SUCCEED();
}

TEST(next, succeeds_with_null)
{
	// Check to see if this crashes
	(void)coroutine_next(nullptr, nullptr);

	// If we get here it didn't crash
	SUCCEED();
}
TEST(next, returns_null_on_null)
{
	void* result = coroutine_next(nullptr, nullptr);

	ASSERT_EQ(nullptr, result);
}

TEST(yield, succeeds_with_null)
{
	// Check to see if this crashes
	(void)coroutine_yield(nullptr, nullptr);

	// Yield didn't crash
	SUCCEED();
}
TEST(yield, returns_null_on_null)
{
	void* result = coroutine_yield(nullptr, nullptr);

	ASSERT_EQ(nullptr, result);
}

TEST(is_complete, succeeds_with_null)
{
	// Check to see if this segfaults
	(void)coroutine_is_complete(nullptr);

	SUCCEED();
}
TEST(is_complete, returns_negative_one_on_null)
{
	ASSERT_EQ(-1, coroutine_is_complete(nullptr));
}
