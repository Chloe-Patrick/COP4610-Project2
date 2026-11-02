# COP4610 Project 2 — GitHub Submission Checklist

## Required source and documentation

- [ ] `part1/empty.c` and `part1/part1.c`.
- [ ] `part1/empty.trace` and `part1/part1.trace` generated on Linux.
- [ ] `part1/Makefile`.
- [ ] `part2/src/timer.c` and `part2/Makefile` (builds `my_timer.ko`).
- [ ] `part3/src/elevator.c` and `part3/Makefile` (builds `elevator.ko`).
- [ ] Custom syscall wrapper source and Makefile, plus reproducing the kernel
      source modifications in `part3/kernel_source/INSTALL.md`.
- [ ] Starter user-space tests used for the project demo.
- [ ] Root README with file listing and build/run instructions.
- [ ] Final division of labor records actual work, not just initial plan.
- [ ] Development log for *each* member and actual meeting/coordination notes.
- [ ] Final tests and 15-minute demonstration practiced.

## Check for compiled files that must not be submitted

From the project root (`~/elevator`), run:

```bash
git status --short
find . -type f \( -name '*.ko' -o -name '*.o' -o -name '*.mod.c' \)
```

It is normal for these compiled files to exist locally. They should be
ignored by `.gitignore` and excluded from commits. The required `.trace`
files are *not* compiled binaries and should be committed.

To see what Git would stage, run:

```bash
git add -n .
```

If the preview looks right, stage and check it:

```bash
git add .
git status --short
```

Before committing, review the list for binaries or files from the entire
kernel build. Do not add `/usr/src/linux-6.16.12` or the kernel tarball.

If the directory isn't a Git repository yet, initialize it or clone the
team's actual GitHub repository before staging. Check `git remote -v` to
confirm the destination before pushing. Do not invent a remote URL.

## Important project details

- The specification allows floors **1-5**. The provided producer may generate
  floor 6 requests; they should be rejected.
- The elevator must have a maximum of 5 pets and 50 lbs at any time.
- State output must include movement state, floor, load, rider list,
  per-floor waiting lists, total waiting, and total serviced.
- Stop must let onboard pets finish their rides before becoming OFFLINE.
- The demo must show successful compilation, loading, use, and unloading.
- All code used in the demo will be pulled from the team's GitHub repository.
- Validate `README.md` placeholders and your course's policies on assistance.
