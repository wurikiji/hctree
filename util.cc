#include <sys/types.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>

int __memalign(void **user, size_t base, size_t size) 
{
	return posix_memalign(user, base, size);
}

void __free(void *user) 
{
	free(user);
}
