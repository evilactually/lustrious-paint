
#include "cbpad.h"

extern void* cb_ptrs[];
extern void* cb_data_ptrs[];
extern int cb_step;
extern void cb_pad_0();

extern int CB_PAD_COUNT;

void set_callback(int id, void* callback) {
    cb_ptrs[id] = callback;
}

void* get_callback(int id) {
	return cb_ptrs[id];
}

void set_userdata(int id, void* data) {
	cb_data_ptrs[id] = data;
}

void* get_userdata(int id) {
	return cb_data_ptrs[id];	
}

void* get_callback_pad(int id) {
	return cb_pad_0 + id*cb_step;
}

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

typedef int (callback_pad_t)(int multiplier);
typedef int (callback_t)(struct user_data_t* data, int multiplier);
typedef int (variadic_callback_pad_t)(int m, ...);


int main(int argc, char const *argv[])
{
	// regular callback test
	for (int i = 0; i < CB_PAD_COUNT; ++i)
	{
		struct user_data_t data = {a:1,b:1,c:1};

        // callback setting test
		set_callback(i, callback);
		assert(get_callback(i) == callback);

		// user data setting test
		set_userdata(i, &data);
		assert(((struct user_data_t*)get_userdata(0))->a == 1);
		assert(((struct user_data_t*)get_userdata(0))->b == 1);
		assert(((struct user_data_t*)get_userdata(0))->c == 1);
	    
		callback_pad_t* callback_pad = (callback_pad_t*)(&cb_pad_0);

		// return value test
		assert(callback_pad(2) == 2*3);

		// callback effect on user data test
		assert(((struct user_data_t*)get_userdata(0))->a == 2);
		assert(((struct user_data_t*)get_userdata(0))->b == 2);
		assert(((struct user_data_t*)get_userdata(0))->c == 2);

	    // second call, return address corruption test
		assert(callback_pad(2) == 4*3);
		assert(((struct user_data_t*)get_userdata(0))->a == 4);
		assert(((struct user_data_t*)get_userdata(0))->b == 4);
		assert(((struct user_data_t*)get_userdata(0))->c == 4);
    }

    // variadic callback test
    for (int i = 0; i < CB_PAD_COUNT; ++i)
	{
		struct user_data_t data = {a:1,b:1,c:1};

		set_callback(i, variadic_callback);
		set_userdata(i, &data);
	    
		variadic_callback_pad_t* callback_pad = (variadic_callback_pad_t*)(cb_pad_0);
		
		// return value test
		assert(callback_pad(2, NULL) == 2*3);

		// callback effect on user data test
		assert(((struct user_data_t*)get_userdata(0))->a == 2);
		assert(((struct user_data_t*)get_userdata(0))->b == 2);
		assert(((struct user_data_t*)get_userdata(0))->c == 2);

	    // second call, return address corruption test
		assert(callback_pad(2, NULL) == 4*3);
		assert(((struct user_data_t*)get_userdata(0))->a == 4);
		assert(((struct user_data_t*)get_userdata(0))->b == 4);
		assert(((struct user_data_t*)get_userdata(0))->c == 4);
    }

	return 0;
}
#endif