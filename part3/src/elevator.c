// COP4610 Project 2: pet elevator kernel module.
// Requires a kernel with the three exported STUB_* syscall handlers.
#include <linux/atomic.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/wait.h>

#define FLOORS 5
#define CAPACITY 5
#define MAX_WEIGHT 50

MODULE_LICENSE("GPL");
MODULE_AUTHOR("COP4610 Project 2 Team");
MODULE_DESCRIPTION("Pet elevator with FIFO queues, kernel thread, and mutex");

// Exported by syscalls/syscalls.c in the group's modified Linux kernel.
extern int (*STUB_start_elevator)(void);
extern int (*STUB_issue_request)(int, int, int);
extern int (*STUB_stop_elevator)(void);

enum elevator_state {
    ELEVATOR_OFFLINE,
    ELEVATOR_IDLE,
    ELEVATOR_LOADING,
    ELEVATOR_UP,
    ELEVATOR_DOWN
};

struct pet {
    struct list_head link;
    int destination;
    int type;
    unsigned int weight;
    u64 arrival_order;
};

static const unsigned int pet_weights[] = { 3, 14, 10, 16 };
static const char pet_symbols[] = { 'C', 'P', 'H', 'D' };
static const char *const state_names[] = {
    "OFFLINE", "IDLE", "LOADING", "UP", "DOWN"
};

static DEFINE_MUTEX(elevator_lock);
static DECLARE_WAIT_QUEUE_HEAD(elevator_wq);
static atomic_t notification = ATOMIC_INIT(0);
static struct list_head waiting[FLOORS + 1];
static LIST_HEAD(riding);
static struct task_struct *elevator_task;
static struct proc_dir_entry *elevator_proc;

static enum elevator_state state = ELEVATOR_OFFLINE;
static bool running;
static bool stopping;
static unsigned int generation;
static int current_floor = 1;
static unsigned int riding_count;
static unsigned int waiting_count;
static unsigned int load_weight;
static unsigned long serviced_count;
static u64 next_order;

static void notify_thread(void)
{
    atomic_inc(&notification);
    wake_up_interruptible(&elevator_wq);
}

static bool waiting_exists_locked(void)
{
    return waiting_count != 0;
}

/* Oldest queued pet determines the next pickup when the car is empty. */
static int next_target_locked(void)
{
    struct pet *pet;
    u64 oldest = U64_MAX;
    int floor;
    int selected = -1;

    if (!list_empty(&riding)) {
        pet = list_first_entry(&riding, struct pet, link);
        return pet->destination;
    }

    if (stopping)
        return -1;

    for (floor = 1; floor <= FLOORS; floor++) {
        if (list_empty(&waiting[floor]))
            continue;
        pet = list_first_entry(&waiting[floor], struct pet, link);
        if (pet->arrival_order < oldest) {
            oldest = pet->arrival_order;
            selected = floor;
        }
    }
    return selected;
}

static bool should_service_here_locked(void)
{
    struct pet *pet;

    list_for_each_entry(pet, &riding, link) {
        if (pet->destination == current_floor)
            return true;
    }

    if (stopping || list_empty(&waiting[current_floor]))
        return false;

    pet = list_first_entry(&waiting[current_floor], struct pet, link);
    return riding_count < CAPACITY &&
           load_weight + pet->weight <= MAX_WEIGHT;
}

/* Caller holds elevator_lock. Removes all riders destined for this floor. */
static void unload_here_locked(void)
{
    struct pet *pet;
    struct pet *tmp;

    list_for_each_entry_safe(pet, tmp, &riding, link) {
        if (pet->destination != current_floor)
            continue;
        list_del(&pet->link);
        load_weight -= pet->weight;
        riding_count--;
        serviced_count++;
        kfree(pet);
    }
}

/* Never skip an oversized front-of-queue pet: strict per-floor FIFO. */
static void board_here_locked(void)
{
    struct pet *pet;

    while (!stopping && !list_empty(&waiting[current_floor])) {
        pet = list_first_entry(&waiting[current_floor], struct pet, link);
        if (riding_count >= CAPACITY ||
            load_weight + pet->weight > MAX_WEIGHT)
            break;
        list_move_tail(&pet->link, &riding);
        waiting_count--;
        riding_count++;
        load_weight += pet->weight;
    }
}

static int elevator_worker(void *unused)
{
    int seen;
    int target;
    int direction;
    unsigned int trip_generation;

    while (!kthread_should_stop()) {
        mutex_lock(&elevator_lock);

        if (!running) {
            state = ELEVATOR_OFFLINE;
            seen = atomic_read(&notification);
            mutex_unlock(&elevator_lock);
            wait_event_interruptible(elevator_wq,
                kthread_should_stop() ||
                atomic_read(&notification) != seen);
            continue;
        }

        /* A stop drains existing riders but does not pick up waiting pets. */
        if (stopping && riding_count == 0) {
            stopping = false;
            running = false;
            state = ELEVATOR_OFFLINE;
            generation++;
            mutex_unlock(&elevator_lock);
            continue;
        }

        if (should_service_here_locked()) {
            state = ELEVATOR_LOADING;
            trip_generation = generation;
            mutex_unlock(&elevator_lock);
            msleep(1000);
            mutex_lock(&elevator_lock);
            if (running && generation == trip_generation) {
                unload_here_locked();
                if (!stopping)
                    board_here_locked();
            }
            mutex_unlock(&elevator_lock);
            continue;
        }

        target = next_target_locked();
        if (target < 0 ||
            (target == current_floor && !waiting_exists_locked())) {
            state = ELEVATOR_IDLE;
            seen = atomic_read(&notification);
            mutex_unlock(&elevator_lock);
            wait_event_interruptible(elevator_wq,
                kthread_should_stop() ||
                atomic_read(&notification) != seen);
            continue;
        }

        if (target == current_floor) {
            /* An unusual state: re-evaluate instead of spinning. */
            seen = atomic_read(&notification);
            mutex_unlock(&elevator_lock);
            wait_event_interruptible_timeout(elevator_wq,
                kthread_should_stop() ||
                atomic_read(&notification) != seen, HZ);
            continue;
        }

        direction = target > current_floor ? 1 : -1;
        state = direction > 0 ? ELEVATOR_UP : ELEVATOR_DOWN;
        trip_generation = generation;
        mutex_unlock(&elevator_lock);

        msleep(2000);  /* Exactly one floor of travel per movement step. */

        mutex_lock(&elevator_lock);
        if (running && generation == trip_generation)
            current_floor += direction;
        mutex_unlock(&elevator_lock);
    }
    return 0;
}

static int elevator_start(void)
{
    mutex_lock(&elevator_lock);
    if (running) {
        mutex_unlock(&elevator_lock);
        return 1;
    }

    /* Existing floor queues survive a stop/start cycle. */
    running = true;
    stopping = false;
    generation++;
    current_floor = 1;
    state = ELEVATOR_IDLE;
    mutex_unlock(&elevator_lock);
    notify_thread();
    return 0;
}

static int elevator_request(int start_floor, int destination_floor, int type)
{
    struct pet *pet;

    if (start_floor < 1 || start_floor > FLOORS ||
        destination_floor < 1 || destination_floor > FLOORS ||
        start_floor == destination_floor || type < 0 || type > 3)
        return 1;

    pet = kmalloc(sizeof(*pet), GFP_KERNEL);
    if (!pet)
        return -ENOMEM;
    pet->destination = destination_floor;
    pet->type = type;
    pet->weight = pet_weights[type];

    mutex_lock(&elevator_lock);
    pet->arrival_order = ++next_order;
    list_add_tail(&pet->link, &waiting[start_floor]);
    waiting_count++;
    mutex_unlock(&elevator_lock);
    notify_thread();
    return 0;
}

static int elevator_stop(void)
{
    mutex_lock(&elevator_lock);
    if (!running || stopping) {
        mutex_unlock(&elevator_lock);
        return 1;
    }

    if (riding_count == 0) {
        running = false;
        state = ELEVATOR_OFFLINE;
        generation++;
    } else {
        stopping = true;
    }
    mutex_unlock(&elevator_lock);
    notify_thread();
    return 0;
}

static int elevator_show(struct seq_file *out, void *unused)
{
    struct pet *pet;
    int floor;

    mutex_lock(&elevator_lock);
    seq_printf(out, "Elevator state: %s\n", state_names[state]);
    seq_printf(out, "Current floor: %d\n", current_floor);
    seq_printf(out, "Current load: %u lbs\n", load_weight);
    seq_puts(out, "Elevator status:");
    list_for_each_entry(pet, &riding, link)
        seq_printf(out, " %c%d", pet_symbols[pet->type], pet->destination);
    seq_putc(out, '\n');

    for (floor = FLOORS; floor >= 1; floor--) {
        unsigned int count = 0;
        list_for_each_entry(pet, &waiting[floor], link)
            count++;
        seq_printf(out, "[%c] Floor %d: %u", 
                   floor == current_floor ? '*' : ' ', floor, count);
        list_for_each_entry(pet, &waiting[floor], link)
            seq_printf(out, " %c%d", pet_symbols[pet->type], pet->destination);
        seq_putc(out, '\n');
    }

    seq_printf(out, "Number of pets: %u\n", riding_count);
    seq_printf(out, "Number of pets waiting: %u\n", waiting_count);
    seq_printf(out, "Number of pets serviced: %lu\n", serviced_count);
    mutex_unlock(&elevator_lock);
    return 0;
}

static int elevator_open(struct inode *inode, struct file *file)
{
    return single_open(file, elevator_show, NULL);
}

static const struct proc_ops elevator_proc_ops = {
    .proc_open = elevator_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

static int __init elevator_init(void)
{
    int floor;

    /* Avoid replacing the pointers owned by the provided syscheck module. */
    if (READ_ONCE(STUB_start_elevator) || READ_ONCE(STUB_issue_request) ||
        READ_ONCE(STUB_stop_elevator))
        return -EBUSY;

    for (floor = 1; floor <= FLOORS; floor++)
        INIT_LIST_HEAD(&waiting[floor]);

    elevator_proc = proc_create("elevator", 0444, NULL, &elevator_proc_ops);
    if (!elevator_proc)
        return -ENOMEM;

    elevator_task = kthread_run(elevator_worker, NULL, "pet_elevator");
    if (IS_ERR(elevator_task)) {
        int error = PTR_ERR(elevator_task);
        elevator_task = NULL;
        proc_remove(elevator_proc);
        return error;
    }

    WRITE_ONCE(STUB_start_elevator, elevator_start);
    WRITE_ONCE(STUB_issue_request, elevator_request);
    WRITE_ONCE(STUB_stop_elevator, elevator_stop);
    return 0;
}

static void __exit elevator_exit(void)
{
    struct pet *pet;
    struct pet *tmp;
    int floor;

    WRITE_ONCE(STUB_start_elevator, NULL);
    WRITE_ONCE(STUB_issue_request, NULL);
    WRITE_ONCE(STUB_stop_elevator, NULL);

    proc_remove(elevator_proc);
    kthread_stop(elevator_task);

    mutex_lock(&elevator_lock);
    list_for_each_entry_safe(pet, tmp, &riding, link) {
        list_del(&pet->link);
        kfree(pet);
    }
    for (floor = 1; floor <= FLOORS; floor++) {
        list_for_each_entry_safe(pet, tmp, &waiting[floor], link) {
            list_del(&pet->link);
            kfree(pet);
        }
    }
    mutex_unlock(&elevator_lock);
}

module_init(elevator_init);
module_exit(elevator_exit);
