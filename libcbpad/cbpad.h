
void set_argc(int id, int argc);

void set_cb(int id, void* callback);

void* get_cb(int id);

void set_ud(int id, void* userdata);

void* get_ud(int id);

#if CPU_TYPE == x86

void* get_cb_pad_cdecl(int id);

void* get_cb_pad_cdecl_rbr(int id);

#endif

#if CPU_TYPE == x86-64

#if OS_TYPE == windows

void* get_cb_pad_win64(int id);

void* get_cb_pad_win64_rbr(int id);

#endif

void* get_cb_pad_sysv64(int id);

void* get_cb_pad_sysv64_rbr(int id);

#endif

/* DESIGN:
   
   - One pad per thread. No sharing.
   - Argument count needed for realignment.
   - RBR used for functions returning values by hidden reference argument 

*/