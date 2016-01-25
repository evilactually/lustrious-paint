
#include "cbpad.h"

extern void* cb_ptrs[];
extern void* cb_data_ptrs[];
extern int cb_cdecl_step;
extern int cb_cdecl_rbr_step;
extern void cb_pad_cdecl_0();
extern void cb_pad_cdecl_rbr_0();

extern int CB_PAD_COUNT;

void set_argc(int id, int argc) {
	//cb_argcs[id] = argc;
}

void set_cb(int id, void* callback) {
    cb_ptrs[id] = callback;
}

void* get_cb(int id) {
	return cb_ptrs[id];
}

void set_ud(int id, void* data) {
	cb_data_ptrs[id] = data;
}

void* get_ud(int id) {
	return cb_data_ptrs[id];	
}

#if CPU_TYPE == x86

void* get_cb_pad_cdecl(int id) {
	return cb_pad_cdecl_0 + id*cb_cdecl_step;
}

void* get_cb_pad_cdecl_rbr(int id) {
	return cb_pad_cdecl_rbr_0 + id*cb_cdecl_rbr_step;
}

#endif

#if CPU_TYPE == x86-64

#if OS_TYPE == windows

void* get_cb_pad_win64(int id) {};

void* get_cb_pad_win64_rbr(int id) {};

#endif

void* get_cb_pad_sysv64(int id) {

}

void* get_cb_pad_sysv64_rbr(int id) {

}

#endif


#define FULL

#ifdef TEST
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>

struct user_data_t {
	int a, b, c;
};

int callback(struct user_data_t* data, int multiplier) {
	data->a *= multiplier;
	data->b *= multiplier;
	data->c *= multiplier;
	return data->a + data->b + data->c;
}

long long large_callback(struct user_data_t* data, long long multiplier) {
	if(data) {
		data->a *= 2;
		data->b *= 2;
		data->c *= 2;
	}
	return multiplier*multiplier;
}

int variadic_callback(struct user_data_t* data, ...) {
	va_list ap;
    va_start(ap,data);
    int multiplier;
	while (multiplier = va_arg(ap, int)) {
		data->a *= multiplier;
		data->b *= multiplier;
		data->c *= multiplier;
    }
	va_end(ap);
	return data->a + data->b + data->c;
}

struct user_data_t callback_struct_return(struct user_data_t* data, int multiplier, int addition_1, int addition_2) {
	struct user_data_t x = {a:data->a*multiplier + addition_1 + addition_2,
	                        b:data->b*multiplier + addition_1 + addition_2,
	                        c:data->c*multiplier + addition_1 + addition_2};
	return x;
}

typedef int (callback_pad_t)(int multiplier);
typedef int (callback_t)(struct user_data_t* data, int multiplier);
typedef int (variadic_callback_pad_t)(int m, ...);
typedef struct user_data_t (callback_pad_struct_return_t)(int multiplier, int addition_1, int addition_2);
typedef struct user_data_t (callback_struct_return_t)(struct user_data_t* data, int multiplier, int addition_1, int addition_2);
typedef long long (callback_pad_large_return_t)(int multiplier);
typedef long long (callback_large_return_t)(struct user_data_t* data, int multiplier);

#ifdef FULL

#if CPU_TYPE == x86

#define get_cb_pad     get_cb_pad_cdecl
#define get_cb_pad_rbr get_cb_pad_cdecl_rbr

#else

#define get_cb_pad     get_cb_pad_sysv64
#define get_cb_pad_rbr get_cb_pad_sysv64_rbr

#endif

int main(int argc, char const *argv[])
{
	printf("%s\n", "Callback setting...");
	for (int i = 0; i < CB_PAD_COUNT; ++i)
	{
		set_cb(i, callback);
		assert(get_cb(i) == callback);
	}
	
	printf("%s\n", "User data setting...");
	for (int i = 0; i < CB_PAD_COUNT; ++i)
	{
		struct user_data_t data = {a:1,b:2,c:3};
    	set_ud(i, &data);
		assert(((struct user_data_t*)get_ud(i))->a == 1);
		assert(((struct user_data_t*)get_ud(i))->b == 2);
		assert(((struct user_data_t*)get_ud(i))->c == 3);
	}

	printf("%s\n", "Printing callback addresses...");
	for (int i = 0; i < CB_PAD_COUNT; ++i)
	{
        //printf("0x%x\n", get_cb_pad_cdecl(i));
	}

	printf("%s\n", "Callback effects on user data...");
	for (int i = 0; i < CB_PAD_COUNT; ++i)
	{
		struct user_data_t data = {a:1,b:1,c:1};
        set_cb(i, callback);
		set_ud(i, &data);
		
		callback_pad_t* callback_pad = (callback_pad_t*)get_cb_pad(i);

		int r = callback_pad(2);
		//printf("%d\n", r);
		assert(r == 2*3);

		r = callback_pad(2);
		//printf("%d\n", r);
		assert(r == 4*3);

		r = callback_pad(2);
		//printf("%d\n", r);
		assert(r == 8*3);
	}

	printf("%s\n", "Variadic callback effects on user data...");
	for (int i = 0; i < CB_PAD_COUNT; ++i)
	{
		struct user_data_t data = {a:1,b:1,c:1};
        set_cb(i, variadic_callback);
		set_ud(i, &data);
		
		variadic_callback_pad_t* callback_pad = (variadic_callback_pad_t*)get_cb_pad(i); 

		int r = callback_pad(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, NULL);
		//printf("%d\n", r);
		assert(r == 3);

		r = callback_pad(1, 1, 1, 1, 1, 2, 1, 1, 1, 1, NULL);
		//printf("%d\n", r);
		assert(r == 2*3);

		r = callback_pad(3, 4, 5, 6, 7, NULL);
		//printf("%d\n", r);
		assert(r == 3*(1*2*3*4*5*6*7));

	}

	printf("%s\n", "Normal struct return test(no callback)...");
    {
    	struct user_data_t data = {a:1,b:2,c:3};
    	struct user_data_t data_ret = callback_struct_return(&data, 2, 1, 3);
        assert(data_ret.a == (data.a*2 + 1 + 3));
        assert(data_ret.b == (data.b*2 + 1 + 3));
        assert(data_ret.c == (data.c*2 + 1 + 3));
	}
	
	printf("%s\n", "Struct return through callback pad test...");
	{
		struct user_data_t data = {a:1,b:2,c:3};
		set_cb(0, callback_struct_return);
		set_ud(0, &data);
		callback_pad_struct_return_t* callback_pad = (callback_pad_struct_return_t*)get_cb_pad_rbr(0);
        //printf("%d\n", (data.a*2 + 1 + 3));
        struct user_data_t data_ret = callback_pad(2, 1, 3);
        //printf("%d\n", data_ret.a);
        //printf("%d\n", (data.a*2 + 1 + 3));
        assert(data_ret.a == (data.a*2 + 1 + 3));
	}
	
	printf("%d\n", sizeof(long long));

	printf("%s\n", "Large primitive type return...");
	{
		struct user_data_t data = {a:1,b:2,c:3};
		set_cb(0, large_callback);
		set_ud(0, &data);
		callback_pad_large_return_t* callback_pad = (callback_pad_large_return_t*)get_cb_pad(0);
        long long a = callback_pad(2);
        long long b = callback_pad(3);
        assert(data.a == 4);
        assert(data.b == 8);
        assert(data.c == 12);
        assert(a = 2*2);
        assert(b = 3*3);
	}

	return 0;
}
#else

int main(int argc, char const *argv[])
{
	long long a = large_callback(NULL, (((unsigned long long)1)<<41) + 1);
	return 0;
}
#endif

#endif