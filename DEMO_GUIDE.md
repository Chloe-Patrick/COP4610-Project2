# COP4610 Project 2 — 15-Minute Demonstration Guide

Run on the existing Ubuntu VM with `uname -r` showing
`6.16.12-cop4610`. Your course requires demonstrating the elevator running
with arbitrary tests for **three minutes**. Practice with a timer beforehand.

## Before the demo

- Check that `~/elevator` matches the team's committed GitHub source.
- Verify `uname -r` is `6.16.12-cop4610`.
- Verify that `syscheck` and `elevator` are not already loaded.
- Make sure your VM has enough storage and is responsive.
- Know where `part3/src/elevator.c` and syscall wrapper code live.
- Never load `syscheck.ko` and `elevator.ko` simultaneously.

## Suggested presentation sequence

**0:00–2:00 — Overview.** Describe Parts 1, 2, and 3, each member's
verified contribution, and the floor/weight/capacity rules.

**2:00–4:00 — Source walkthrough.** Point out the syscall wrappers, pet
linked lists, mutex, `elevator_worker` kernel thread, and proc output.

**4:00–5:00 — Part 1 and timer.** Show trace files and `make verify` output.
Briefly show `my_timer.ko` load/read/unload if requested.

**5:00–6:30 — Syscall verification.** With elevator unloaded:

```bash
cd ~/elevator/part3/tests/system-calls-test
make
sudo insmod syscheck.ko
./test-syscalls
sudo rmmod syscheck
```

**6:30–8:00 — Compile and load the elevator.**

```bash
cd ~/elevator/part3
make
sudo insmod elevator.ko
cat /proc/elevator
```

**8:00–11:30 — Elevator live demonstration (at least three minutes).**

In one terminal:

```bash
cd ~/elevator/part3/tests/elevator-test
make
./consumer --start
./producer 30
watch -n 1 cat /proc/elevator
```

In a second terminal, run additional valid or provided random requests
while the display is running. Observe changes in state, boarding, dropping
off, waiting count, and serviced count. Press Ctrl+C to exit `watch` after
at least three minutes. Floor 6 requests from the provided producer are
invalid and should return 1.

**11:30–13:30 — Stop, restart, and cleanup.**

```bash
./consumer --stop
cat /proc/elevator
```

If riders remain onboard, keep checking until the elevator is OFFLINE.
Then, with no producer/consumer process still running:

```bash
cd ~/elevator/part3
sudo rmmod elevator
ls /proc/elevator
```

The final command should report that the proc entry no longer exists.

**13:30–15:00 — Explain results and answer questions.** Describe why a
mutex is needed, how linked lists preserve FIFO, why the worker sleeps,
and how stopping differs from unloading.

## Short talking points

- **Why syscall stubs?** The modified kernel offers fixed syscall numbers;
  the elevator module supplies the handlers when loaded.
- **Why linked lists?** Floors can hold dynamically allocated, variable-length
  queues, and each floor can board in FIFO order.
- **Why mutex?** System calls, a kernel worker, and proc reads access shared
  elevator data and must not race.
- **Why a kernel thread?** It runs the elevator while users keep submitting
  requests without blocking user-space callers for entire trips.
- **What happens on stop?** No new boarding; existing riders are dropped
  off; queued pets remain for a later restart.

## Important unfinished verification

FIFO ordering and precise movement/load timing have not yet been
independently measured in the recorded tests. Verify them before the demo.
