# Part 2 testing (VM with root access)

```sh
cd ~/elevator/part2
make
sudo insmod my_timer.ko
lsmod | grep my_timer
cat /proc/timer          # first read: current time only
sleep 1
cat /proc/timer          # now also prints elapsed time near 1 second
sleep 3
cat /proc/timer          # prints elapsed time near 3 seconds
sudo rmmod my_timer
test ! -e /proc/timer && echo 'proc entry removed'
make clean
```

The module uses `ktime_get_real_ts64()` to retrieve the wall-clock time;
it stores the previous `timespec64` and subtracts it from the current one.
The read offset prevents a single `cat` invocation from updating the
saved timestamp multiple times. A mutex protects concurrent reads.
Installation/loading requires an appropriately privileged VM; do not try
`insmod` on Linprog.
