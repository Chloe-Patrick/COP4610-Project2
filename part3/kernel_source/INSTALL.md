# Chloe's Part 3a / 3b: Patch and boot Linux 6.16.x

**Run ONLY on your group's disposable Linux VM, not on Linprog or a personal
host. Take a VM snapshot before changing or rebooting the kernel.**

The course specifies a **6.16.x kernel**, with new syscall numbers
**548**, **549**, and **550**. Example below uses `6.16.12`; adjust the
version if your instructor or VM already has another 6.16.x tree.

## 1. Install prerequisites and download the specified kernel

```sh
sudo apt update
sudo apt install -y build-essential libncurses-dev bison flex \
    libssl-dev libelf-dev bc dwarves wget xz-utils strace
cd /usr/src
sudo wget https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-6.16.12.tar.xz
sudo tar -xf linux-6.16.12.tar.xz
sudo chown -R "$(id -un):$(id -gn)" /usr/src/linux-6.16.12
ln -s /usr/src/linux-6.16.12 ~/linux-6.16.12
cd ~/linux-6.16.12
```

Keep the tarball and the original kernel in case you need to roll back.
If the link already exists, do not recreate it.

## 2. Install your syscall wrapper code into the kernel source

From your project's checked-out repository, replace `PROJECT` below with
the absolute directory containing `elevator`:

```sh
PROJECT="$HOME/elevator"
mkdir -p "$HOME/linux-6.16.12/syscalls"
cp "$PROJECT/part3/kernel_source/syscalls/syscalls.c" \
    "$HOME/linux-6.16.12/syscalls/syscalls.c"
cp "$PROJECT/part3/kernel_source/syscalls/Makefile" \
    "$HOME/linux-6.16.12/syscalls/Makefile"
```

Edit `arch/x86/entry/syscalls/syscall_64.tbl` and ADD these lines after
the x32-only syscall range (do not replace or renumber any existing lines):

```text
548 common start_elevator sys_start_elevator
549 common issue_request  sys_issue_request
550 common stop_elevator  sys_stop_elevator
```

Before the final `#endif` in `include/linux/syscalls.h`, ADD:

```c
asmlinkage long sys_start_elevator(void);
asmlinkage long sys_issue_request(int start_floor, int destination_floor,
                                 int type);
asmlinkage long sys_stop_elevator(void);
```

In the **kernel source's top-level** `Makefile`, find the `core-y` variable
and add `syscalls/` to its value (don't overwrite existing entries):

```make
core-y += syscalls/
```

Verify `grep -n '^54[89] \|^550 ' arch/x86/entry/syscalls/syscall_64.tbl`
shows all three entries, and ensure none already existed.

## 3. Configure and compile the modified kernel

```sh
cd ~/linux-6.16.12
cp /boot/config-$(uname -r) .config
make olddefconfig
make menuconfig
# Save and exit menuconfig; no custom menu changes are required.
scripts/config --set-str SYSTEM_TRUSTED_KEYS ''
scripts/config --set-str SYSTEM_REVOCATION_KEYS ''
make olddefconfig
make -j"$(nproc)"
sudo make modules_install
sudo make headers_install
sudo make install
sudo update-grub
sudo reboot
```

`make headers_install` is included because the course slides ask for it.
If you hit a BTF/pahole error, check that `dwarves` is installed. If you
hit certificate errors, inspect the two `SYSTEM_*_KEYS` strings in `.config`.
If the distro-specific `.config` refers to another missing certificate,
inspect the error and resolve it; do not delete unrelated security settings.

## 4. Confirm the booted kernel, compile the provided syscall tests

```sh
uname -r    # should contain 6.16.12, not the old distro kernel
cd "$HOME/elevator/part3/tests/system-calls-test"
make
sudo insmod syscheck.ko
./test-syscalls
sudo rmmod syscheck
```

Expect all three calls to say `... exists` with syscheck loaded.
`syscheck.ko` provides dummy successful handlers. Remove it before the
real elevator module is inserted; they must never own STUB pointers at
the same time. With neither module loaded, the three calls should
return -1 with errno=ENOSYS from userspace.

**NOTE:** The provided project description calls the test module
`syscalls.ko`, but the actual starter archive names it **`syscheck.ko`**
and the test executable is **`test-syscalls`**.

## 5. Handoff to Alexander

The elevator module must use the exported symbols (same signatures):

```c
extern int (*STUB_start_elevator)(void);
extern int (*STUB_issue_request)(int, int, int);
extern int (*STUB_stop_elevator)(void);
```

During `module_init`, assign the three pointers to the elevator's handler
functions. During module exit, clear all three pointers. Ensure no call
remains in flight when unloading; this simple class-provided pointer
interface has **no unload-race protection**. Do not unload while running
a producer or consumer. The elevator handlers, queues, mutex and thread
belong to Alexander's sections.

The provided `producer.c` currently generates floors **1 through 6**;
the specification's building has only floors **1 through 5**. Requests for
floor 6 must be rejected by the eventual `issue_request` handler.
