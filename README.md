# COP4610 Project 2: Pet Elevator

**Team members:** Chloe Patrick and Alexander Petrov  
**Project group number:** 23
**Environment tested:** Ubuntu Server 24.04.5 LTS, x86-64 VM, custom Linux
6.16.12-cop4610

## Project overview

This project explores system calls and kernel programming through three tasks:

1. Trace five additional system calls in a C program with `strace`.
2. Implement `my_timer.ko` and `/proc/timer` to report current and elapsed time.
3. Modify Linux 6.16.12 to add elevator system calls and implement `elevator.ko`.

The pet elevator serves floors 1-5. It supports at most five pets and 50 lbs
at one time, queues waiting pets using linked lists, runs a kernel thread,
and uses a mutex to protect shared state. A proc entry shows elevator status.

## Source files and directory organization

```text
elevator/
├── README.md
├── DEVELOPMENT_LOG.md
├── DEMO_GUIDE.md
├── SUBMISSION_CHECKLIST.md
├── .gitignore
├── Makefile
├── part1/
│   ├── empty.c
│   ├── empty.trace
│   ├── part1.c
│   ├── part1.trace
│   ├── Makefile
│   └── TESTING.md
├── part2/
│   ├── src/timer.c
│   ├── Makefile
│   └── TESTING.md
└── part3/
    ├── src/elevator.c
    ├── Makefile
    ├── IMPLEMENTATION.md
    ├── kernel_source/
    │   ├── INSTALL.md
    │   └── syscalls/
    │       ├── syscalls.c
    │       └── Makefile
    └── tests/
        ├── elevator-test/       # Provided producer and consumer tests
        └── system-calls-test/   # Provided syscall test and syscheck module
```

Note: The two `.trace` files are generated on Linux and must be included in
Git. Executables, `.ko`, `.o`, and other build artifacts must not be committed.
The patched *entire* Linux kernel source lives at `/usr/src/linux-6.16.12`
on the VM, not in this project repository. `kernel_source/INSTALL.md` explains
how to reproduce those modifications.

## Build and run

### Part 1: System call tracing

From the project root:

```bash
cd part1
make verify
```

This compiles both programs, records the traces, and checks that `part1`
produces five additional system calls compared with `empty`.

### Part 2: Timer kernel module

Run on Linux with the matching headers and sudo privileges:

```bash
cd part2
make
sudo insmod my_timer.ko
cat /proc/timer
sleep 1
cat /proc/timer
sudo rmmod my_timer
```

The first read displays current time. Later reads also display the time
elapsed since the previous read. `/proc/timer` disappears after removal.

### Part 3a and 3b: Custom kernel and system calls

See `part3/kernel_source/INSTALL.md` to apply the source changes, configure,
compile, install, and boot Linux 6.16.12. The syscall numbers are:

- 548: `start_elevator(void)`
- 549: `issue_request(int start_floor, int destination_floor, int type)`
- 550: `stop_elevator(void)`

The booted kernel was checked with `uname -r`, which reported:

```text
6.16.12-cop4610
```

The syscall wrappers dispatch through exported `STUB_*` function pointers.
To test using the provided stub module, first ensure the elevator module is
not loaded:

```bash
cd part3/tests/system-calls-test
make
sudo insmod syscheck.ko
./test-syscalls
sudo rmmod syscheck
```

Do not load `syscheck.ko` and `elevator.ko` simultaneously.

### Part 3c-3f: Pet elevator module

Run under the booted custom kernel (`uname -r` should be
`6.16.12-cop4610`):

```bash
cd part3
make
sudo insmod elevator.ko
cat /proc/elevator
cd tests/elevator-test
make
./consumer --start
./producer 30
watch -n 1 cat /proc/elevator
```

Press Ctrl+C to exit `watch`. After the desired testing, use:

```bash
./consumer --stop
cat /proc/elevator
cd ../..
sudo rmmod elevator
ls /proc/elevator  # Expected: No such file or directory
```

Only unload the module after user-space tests have stopped and the elevator
is fully offline. `elevator.ko` is generated in `part3/`.

## Elevator implementation

- **Threads (3c):** `elevator_worker` runs as a kernel thread. It waits for
  requests and controls movement, pickup, and drop-off.
- **Linked lists (3d):** Linux `list_head` queues represent waiting pets on
  each floor and pets currently riding. Pet nodes use `kmalloc` and `kfree`.
- **Mutexes (3e):** `elevator_lock` guards queue changes and shared counters
  and state for syscalls, the worker, and `/proc/elevator` reads.
- **Scheduling (3f):** The module serves pets using per-floor FIFO boarding,
  a maximum of 5 pets / 50 lbs, two-second floor travel, and one-second
  loading/unloading stops. The target favors the oldest onboard rider's
  destination, or the oldest waiting request when empty.
- **Stopping:** New boarding stops; onboard pets finish their rides. Waiting
  pets remain queued so they can be serviced after a restart.

Pet types: `C` = Chihuahua (3 lbs), `P` = Pug (14 lbs), `H` = Pughuahua
(10 lbs), and `D` = Dachshund (16 lbs).

## Tests and observed results

These results were observed on November 2, 2026. They describe the tested
runs, not exhaustive proof for all inputs.

| Test | Observed outcome |
| --- | --- |
| Part 1 `make verify` | `empty`: 30 calls; `part1`: 35 calls; difference: 5 |
| Timer module | Compiled, loaded, returned time and elapsed time, unloaded |
| Custom kernel boot | `uname -r` returned `6.16.12-cop4610` |
| Valid/invalid requests | 30-request run: 19 accepted, 11 rejected for floor 6 |
| 30-request run | 19 pets serviced; 0 waiting; 0 aboard; OFFLINE after stop |
| Stop with pets waiting | OFFLINE, 0 aboard, 19 still waiting on floors |
| Restart after stop | All 19 waiting pets serviced; cumulative total: 42 |
| Restart / stop / unload | Start and stop returned 0; `/proc/elevator` removed |
| Capacity observation | No observed run exceeded 5 pets or 50 lbs |


## Division of labor

### Original assignment

| Section | Originally assigned |
| --- | --- |
| Part 1: System Call Tracing | Chloe Patrick |
| Part 2: Timer Kernel Module | Chloe Patrick |
| Part 3a: Adding System Calls | Chloe Patrick |
| Part 3b: Kernel Compilation | Chloe Patrick |
| Part 3c: Threads | Alexander Petrov |
| Part 3d: Linked Lists | Alexander Petrov |
| Part 3e: Mutexes | Alexander Petrov |
| Part 3f: Scheduling Algorithm | Alexander Petrov |

### Final division of labor

**Chloe Patrick:** Part 1 (System Call Tracing), Part 2 (Timer Kernel Module),
Part 3a (Adding System Calls), and Part 3b (Kernel Compilation). Chloe set up
and tested the Linux VM, installed the custom kernel, and integrated and tested
the elevator module with the provided programs.

**Alexander Petrov:** Part 3c (Threads), Part 3d (Linked Lists), Part 3e
(Mutexes), and Part 3f (Scheduling Algorithm). These cover the elevator worker
thread, dynamic pet queues, synchronization, and elevator scheduling behavior.

The team retained its original division of labor. Chloe performed the recorded
integration and runtime tests for the completed elevator module.

## Development log

See `DEVELOPMENT_LOG.md`. It records Chloe's development and integration tests
and Alexander's assigned implementation areas. Add Alexander's actual dates,
debugging notes, and any group meeting details before submission.

## Group meetings and communication

### Group Communication

Chloe Patrick and Alexander Petrov met online as needed throughout the project to discuss progress, coordinate assigned tasks, and address any issues that came up during development.

## Known limitations and notes

- The kernel build was performed in a single-core emulated x86-64 Ubuntu VM;
  a full first build was unusually slow. Incremental module builds are faster.
- The generated user-space programs occasionally request invalid floor 6.
- Build artifacts should not be checked into Git.
- `syscheck.ko` must be unloaded before loading `elevator.ko`.
- Follow the project's demonstration requirements and any course policy on
  disclosing assistance used during development.
