// MakePtr is a macro that allows you to easily add two values (including
// pointers) together without dealing with C's pointer arithmetic.  It
// essentially treats the last two parameters as DWORDs.  The first
// parameter is used to typecast the result to the appropriate pointer type.
#ifdef _WIN64
#define MakePtr(cast, ptr, addValue) (cast)( (DWORD64)(ptr) + (DWORD64)(addValue))
#else
#define MakePtr(cast, ptr, addValue) (cast)( (DWORD)(ptr) + (DWORD)(addValue))
#endif

