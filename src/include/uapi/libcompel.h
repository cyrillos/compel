#ifndef __LIBCOMPEL_H__
#define __LIBCOMPEL_H__

#define LIB_VERSION_SYM	"lib_version"
typedef void (*lib_version_t)(unsigned int *major, unsigned int *minor);
extern void lib_version(unsigned int *major, unsigned int *minor);

#endif /* __LIBCOMPEL_H__ */
