# About SPI Flash
## make flash.bin

```
cd out/bin
dd if=/dev/zero of=flash.bin bs=1024 count=8192
```

## qemu run

```
../../qemu/build/aarch64-softmmu/qemu-system-aarch64 -machine virgo,acpi=off,secure=on,mte=off,gic-version=3,virtualization=false -nographic -smp 2 -serial mon:stdio -bios bl1.bin -semihosting-config enable=on,target=native -d unimp -m 1057 -mtdblock flash.bin -cpu cortex-a55
```

## info

* IRQ: 10
* ADDR: 0x090c0000
* Flash Type: w25q64 8M
