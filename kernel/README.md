# Kernel
## Day3 : 最小カーネルの作り方

$ cat main.cpp
```c
extern "C" void KernelMain() {
  while (1) __asm__("hlt");
}
```
$ clang++ -O2 -Wall -g --target=x86_64-elf -ffreestanding -mno-red-zone -fno-expections -fno-rtti -std=c++17 -c main.cpp
$ ld.lld --entry KernelMain -z norelro --image-base 0x100000 --static -o kernel.elf main.o
