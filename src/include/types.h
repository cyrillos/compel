#ifndef __COMPEL_TYPES_H__
#define __COMPEL_TYPES_H__

#include <linux/types.h>
#include <sys/types.h>

#include <stdbool.h>
#include <stdint.h>

#include "asm-generic/int.h"

#define PAGE_SHIFT		(12)
#define PAGE_SIZE		(1 << PAGE_SHIFT)
#define PAGES(len)		((len) >> PAGE_SHIFT)

#endif /* __COMPEL_TYPES_H__ */
