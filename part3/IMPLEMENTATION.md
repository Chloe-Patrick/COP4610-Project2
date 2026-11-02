# Elevator module: Parts 3c-3f

This source is an initial complete implementation to test against your group's
patched Linux 6.16.12 kernel. **Do not load on the stock Ubuntu 6.8 kernel:**
the required STUB_* symbols have not been exported there.

Files:
- `part3/src/elevator.c` — kernel module, queue management, scheduling and procfs.
- `part3/Makefile` — produces `elevator.ko` in the `part3` directory.

## Project behavior

- Five floors (1-5). Requests outside that range, invalid types, or same
  start/destination are rejected with `1`.
- Four pet types: C=3 lbs, P=14 lbs, H=10 lbs, D=16 lbs.
- Queues per floor and a riding queue use `linux/list.h`; pets use `kmalloc`.
- An `elevator_worker` kernel thread updates the car; a mutex guards all queues
  and shared state. Work notifications use a wait queue to avoid busy waiting.
- Boarding respects FIFO on each floor, a capacity of 5 and a 50-lb limit.
- The car takes 2 seconds for each one-floor trip and 1 second at a stop for
  loading/unloading; timing is approximate under system load.
- When riders are aboard, the next target is the *oldest onboard rider's*
  destination. When empty, the car travels to the floor containing the oldest
  queued request. It may board new pets at intermediate floors.
- `stop_elevator()` stops new boarding but allows existing riders to exit;
  waiting pets remain queued for a future restart.
- `/proc/elevator` contains movement state, location, load, riding pets,
  waiting counts and queue contents for each floor, and serviced total.
- Requests can arrive while the elevator is offline and will wait until started.

## Copy to the VM without touching the running kernel build

On the Mac terminal after downloading the ZIP to Downloads:

    scp ~/Downloads/Part3_Elevator_Module.zip chloe@192.168.64.2:/home/chloe/

In a *second* terminal connected to Ubuntu:

    cd ~/elevator
    unzip -o ~/Part3_Elevator_Module.zip
    ls -l part3/src/elevator.c part3/Makefile

Do not run `make` for this module in parallel with the very slow full kernel
build if your VM is already struggling. This is just copying source files.

## Build and test once the custom kernel is installed and booted

    uname -r   # Must show your patched 6.16.12 kernel, not 6.8.0-generic
    cd ~/elevator/part3
    make
    sudo insmod elevator.ko
    cat /proc/elevator
    cd tests/elevator-test
    make
    ./consumer --start
    ./producer 10
    cat /proc/elevator
    ./consumer --stop
    cat /proc/elevator
    cd ../..
    sudo rmmod elevator

The starter producer requests floor 6 sometimes despite the assignment's
five-floor requirement. Such requests must return 1. `syscheck.ko` must be
unloaded before loading `elevator.ko` because both use the same stubs.

Additional recommended tests:
- Request a pet from floor 1 to 5; observe four 2-second moves and 1-second
  load/unload stops.
- Send >5 pets from one floor; confirm maximum rider count of 5.
- Send multiple 16-lb dachshunds; confirm the car never exceeds 50 lbs.
- Stop with riders onboard; confirm state reaches OFFLINE only after dropoff.
- Call start twice and stop twice; verify return values 0 then 1.
- Remove module; confirm /proc/elevator disappears.

## Notes and limitations

This code can be syntax-checked against available kernel headers, but its full
functionality and runtime behavior must be verified using the custom kernel.
Because the course's built-in syscall wrapper calls a mutable module function
pointer, do not unload the module while user programs are concurrently issuing
syscalls; this project design does not implement a safe module-reference handoff.

The user-space starter test assumes the kernel already has calls 548-550. Do
not assume that having a compiled `elevator.ko` proves syscall integration.
