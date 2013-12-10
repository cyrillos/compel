#ifndef __UAPI_LIBCOMPEL_H__
#define __UAPI_LIBCOMPEL_H__

#include <sys/types.h>

#define LIB_VERSION_SYM		"lib_version"
typedef void (*lib_version_t)(unsigned int *major, unsigned int *minor);
extern void lib_version(unsigned int *major, unsigned int *minor);

#define LIB_SEIZE_TASK_SYM	"lib_seize_task"
typedef void *(*lib_seize_task_t)(pid_t pid);
extern void *lib_seize_task(pid_t pid);

#define LIB_UNSEIZE_TASK_SYM	"lib_unseize_task"
typedef int (*lib_unseize_task_t)(void *compel_handler);
extern int lib_unseize_task(void *compel_handler);

#define LIB_INFECT_TASK_SYM	"lib_infect_task"
typedef int (*lib_infect_task_t)(void *compel_handler);
extern int lib_infect_task(void *compel_handler);

#define LIB_CURE_TASK_SYM	"lib_cure_task"
typedef int(*lib_cure_task_t)(void *compel_handler);
extern int lib_cure_task(void *compel_handler);

#define LIB_EXEC_SYM		"lib_exec"
typedef int (*lib_exec_t)(void *compel_handler);
extern int lib_exec(void *compel_handler);

#endif /* __UAPI_LIBCOMPEL_H__ */
