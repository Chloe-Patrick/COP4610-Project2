// Built into the patched kernel. Elevator handlers are supplied by a module.
#include <linux/compiler.h>
#include <linux/errno.h>
#include <linux/export.h>
#include <linux/syscalls.h>

int (*STUB_start_elevator)(void);
int (*STUB_issue_request)(int, int, int);
int (*STUB_stop_elevator)(void);

EXPORT_SYMBOL(STUB_start_elevator);
EXPORT_SYMBOL(STUB_issue_request);
EXPORT_SYMBOL(STUB_stop_elevator);

SYSCALL_DEFINE0(start_elevator)
{
    int (*handler)(void) = READ_ONCE(STUB_start_elevator);

    if (!handler)
        return -ENOSYS;
    return handler();
}

SYSCALL_DEFINE3(issue_request, int, start_floor,
                int, destination_floor, int, type)
{
    int (*handler)(int, int, int) = READ_ONCE(STUB_issue_request);

    if (!handler)
        return -ENOSYS;
    return handler(start_floor, destination_floor, type);
}

SYSCALL_DEFINE0(stop_elevator)
{
    int (*handler)(void) = READ_ONCE(STUB_stop_elevator);

    if (!handler)
        return -ENOSYS;
    return handler();
}
