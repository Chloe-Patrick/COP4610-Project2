# COP4610 Project 2 — Development Log

Record actual team contributions. The entries below summarize results
reported during this project; edit dates/details if your personal records
show differences. Do not claim unverified work or meetings.

## Chloe Patrick

| Date | Work and results |
| --- | --- |
| October 25-26, 2026 | Set up the x86-64 Ubuntu VM on an M2 Mac using UTM; transferred project sources. |
| October 26, 2026 | Built `my_timer.ko`; loaded and read `/proc/timer`; unloaded module successfully. |
| October 30-November 2, 2026 | Extracted Linux 6.16.12, added syscalls 548-550 and their wrapper code, updated the syscall table, header, and kernel Makefile; configured and compiled the kernel. |
| November 2, 2026 | Installed and rebooted into Linux `6.16.12-cop4610`; verified the running version using `uname -r`. |
| November 2, 2026 | Integrated the elevator module source and Makefile into the VM project, built and loaded `elevator.ko`, and tested `/proc/elevator` and the starter consumer/producer programs. |
| November 2, 2026 | Ran Part 1 tracing check: 30 baseline calls and 35 modified-program calls, for a difference of 5. |
| November 2, 2026 | Tested 30 requests: 19 accepted, 11 invalid requests rejected; all 19 accepted pets serviced. |
| November 2, 2026 | Stopped elevator with queued pets, confirmed no onboard pets and 19 waiting, restarted and serviced all 19; cumulative serviced count reached 42. |
| November 2, 2026 | Confirmed successful start, stop, removal, and disappearance of `/proc/elevator`; reported no observed violation of 5-pet/50-lb limits. |

### Debugging issues and resolutions

- Linprog lacked the running kernel's module build headers and did not grant
  the privileges needed for the module project; switched to Ubuntu VM.
- M2 host required x86-64 emulation for the course's syscall-table changes.
- VM experienced CPU stalls; rebooted and confirmed project files survived.
- The first kernel build was very slow under one emulated CPU; completed and
  verified installation of `6.16.12-cop4610`.
- Starter producer occasionally sent floor 6 requests; the elevator correctly
  rejected those requests as invalid under the five-floor specification.

## Alexander Petrov

**Assigned implementation completed:**

- Part 3c — Threads: elevator kernel worker thread.
- Part 3d — Linked Lists: per-floor waiting queues and onboard pet list.
- Part 3e — Mutexes: synchronization of shared elevator data.
- Part 3f — Scheduling Algorithm: floor movement, boarding and unloading,
  capacity and weight restrictions, and service decisions.

**Implementation dates, debugging decisions, and commits:**
[ADD ALEXANDER'S ACTUAL DEVELOPMENT NOTES; DATES HAVE NOT BEEN PROVIDED]

Chloe performed the recorded compilation, integration, and runtime tests of the
complete elevator module on the custom Linux kernel.

## Group meetings / coordination

| Date | Participants | What was discussed / decided |
| --- | --- | --- |
| [DATE] | [NAMES] | [REAL MEETING OR COORDINATION] |

If there were no formal meetings, replace this table with an accurate account
of asynchronous coordination (if any). Do not invent meeting records.

## Remaining checks

- [ ] Independently verify strict FIFO boarding from controlled inputs.
- [ ] Verify travel and load/unload delays against timestamps.
- [ ] Run concurrent producer tests while checking module stability.
- [ ] Add Alexander's actual development dates and group communication notes.
- [ ] Run the full demonstration from a clean Git checkout.
