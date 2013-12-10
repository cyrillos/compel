#include <stdlib.h>
#include <stdio.h>

#include "common/log.h"

#include "uapi/libcompel.h"

#include "version.h"
#include "compiler.h"
#include "types.h"


void lib_version(unsigned int *major, unsigned int *minor)
{
	*major = COMPEL_VERSION_MAJOR;
	*minor = COMPEL_VERSION_MINOR;
}
