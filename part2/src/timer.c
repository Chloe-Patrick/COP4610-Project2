// my_timer: report current Unix time and the time since the preceding read.
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/timekeeping.h>

#define PROC_NAME "timer"

static struct proc_dir_entry *timer_entry;
static DEFINE_MUTEX(last_read_mutex);
static struct timespec64 last_read;
static bool has_last_read;

static ssize_t timer_read(struct file *file, char __user *user_buf,
                          size_t count, loff_t *offset)
{
    struct timespec64 now;
    s64 seconds;
    long nanoseconds;
    char output[160];
    int len;

    /* cat() reads to EOF. Do not treat the second read as a new visit. */
    if (*offset != 0)
        return 0;

    mutex_lock(&last_read_mutex);
    ktime_get_real_ts64(&now);

    if (!has_last_read) {
        len = scnprintf(output, sizeof(output),
                        "current time: %lld.%09ld\n",
                        (long long)now.tv_sec, now.tv_nsec);
    } else {
        seconds = now.tv_sec - last_read.tv_sec;
        nanoseconds = now.tv_nsec - last_read.tv_nsec;
        if (nanoseconds < 0) {
            seconds--;
            nanoseconds += NSEC_PER_SEC;
        }

        len = scnprintf(output, sizeof(output),
                        "current time: %lld.%09ld\n"
                        "elapsed time: %lld.%09ld\n",
                        (long long)now.tv_sec, now.tv_nsec,
                        (long long)seconds, nanoseconds);
    }

    last_read = now;
    has_last_read = true;
    mutex_unlock(&last_read_mutex);

    return simple_read_from_buffer(user_buf, count, offset, output, len);
}

static const struct proc_ops timer_proc_ops = {
    .proc_read = timer_read,
};

static int __init my_timer_init(void)
{
    has_last_read = false;
    timer_entry = proc_create(PROC_NAME, 0444, NULL, &timer_proc_ops);
    if (!timer_entry)
        return -ENOMEM;
    return 0;
}

static void __exit my_timer_exit(void)
{
    proc_remove(timer_entry);
}

module_init(my_timer_init);
module_exit(my_timer_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chloe Patrick");
MODULE_DESCRIPTION("Unix timestamp and elapsed-time proc module");
