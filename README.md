# UhyokOS
- OS of Uhyoko, by Uhyoko, for Uhyoko from MikanOS (¦3[▓▓]

# Environment
## MacBook (Retina, 12-inch, Early 2015)
OS: macOS Big Sur ver. 11.4
CPU:1.3 GHz デュアルコアIntel Core M

### How to run on Intel Mac
Thanks!
[Mac で始める「ゼロからのOS自作入門」 - Qiita](https://qiita.com/yamoridon/items/4905765cc6e4f320c9b5)

## Ubuntu 21.04
OS: Ubuntu 21.04 (Hirsute Hippo)
CPU: Intel Core i7-9700KF CPU @ 3.60GHz
Cores: 8

### How to run on Ubuntu 21.04
- Basically you can follow https://github.com/uchan-nos/mikanos-build
- Since llvm-7-dev isn't provided for Ubuntu 21.04, use $sudo apt-get install llvm-11-dev clang-11$
- Change -7 to -11 in the ~/osbook/devenv/ansible_provision.yml and build

# Diary
## Day 1
- [Unified_Extensible_Firmware_Interface : Wikipedia](https://ja.wikipedia.org/wiki/Unified_Extensible_Firmware_Interface)
- 教科書P46
- [UEFI Specification](https://uefi.org/sites/default/files/resources/UEFI_Spec_2_9_2021_03_18.pdf)
- mikanOS系の名前をuhyokOSへ変更する。

## Day 2
- P. 61「2.7 メモリマップの確認」で、出力されたメモリマップをイメージから見る方法。mountコマンドをmacOS用にhdiutilコマンドに翻訳する必要がある。
```sh
cd ~/edk2
mkdir mnt
hdiutil attach -mountpoint mnt disk.img
cat mnt/memmap
Index, Type, Type(name), PhysicalStart, NumberOfPages, Attribute
0, 3, EfiBootServicesCode, 00000000, 1, F
1, 7, EfiConventionalMemory, 00001000, 9F, F
2, 7, EfiConventionalMemory, 00100000, 700, F
3, A, EfiACPIMemoryNVS, 00800000, 8, F
4, 7, EfiConventionalMemory, 00808000, 8, F
5, A, EfiACPIMemoryNVS, 00810000, F0, F
...
```

## Day 3
- カーネル作成
```c
$ cat main.cpp
extern "C" void KernelMain() {
  while (1) __asm__("hlt");
}
$ clang++ -O2 -Wall -g --target=x86_64-elf -ffreestanding -mno-red-zone -fno-expections -fno-rtti -std=c++17 -c main.cpp
$ ld.lld --entry KernelMain -z norelro --image-base 0x100000 --static -o kernel.elf main.o
```

- QEMU起動コマンド変更
$ $HOME/osbook/devenv/run_qemu.sh $HOME/edk2/Build/UhyokoLoaderX64/DEBUG_CLANGPDB/X64/Loader.efi $HOME/workspace/UhyokOS/kernel/kernel.elf

- エントリーポイントが教科書と違うアドレスだが、大丈夫なのかな？？

- on Mac差分
- https://qiita.com/yamoridon/items/4905765cc6e4f320c9b5
- eval clang++ $CPPFLAGS -O2 --target=x86_64-elf -fno-exceptions -ffreestanding -c main.cpp
- eval ld.lld $LDFLAGS --entry=KernelMain -z norelro --image-base 0x100000 --static -o kernel.elf -z separate-code main.o


## Day 4
- Makefileを作る。
- https://kaworu.jpn.org/c/C%E8%A8%80%E8%AA%9E%E3%81%AE%E3%82%A4%E3%83%B3%E3%82%AF%E3%83%AB%E3%83%BC%E3%83%89%E3%82%AC%E3%83%BC%E3%83%89%E3%81%AFpragma_once%E3%82%92%E4%BD%BF%E3%81%86
- https://en.wikipedia.org/wiki/Pragma_once
- インクルードガード初めて知った

- 描画
  - ノリで円を描画させてみた
  - ブレゼンハムのアルゴリズムによる円周の描画というものがあるらしい。
  - https://www.kushiro-ct.ac.jp/yanagawa/cg-2015/1021/index.html


```
readelf -l kernel.elf

Elf file type is EXEC (Executable file)
Entry point 0x101020
There are 5 program headers, starting at offset 64

Program Headers:
  Type           Offset             VirtAddr           PhysAddr
                 FileSiz            MemSiz              Flags  Align
  PHDR           0x0000000000000040 0x0000000000100040 0x0000000000100040
                 0x0000000000000118 0x0000000000000118  R      0x8
  LOAD           0x0000000000000000 0x0000000000100000 0x0000000000100000
                 0x00000000000001a8 0x00000000000001a8  R      0x1000
  LOAD           0x0000000000001000 0x0000000000101000 0x0000000000101000
                 0x00000000000001d9 0x00000000000001d9  R E    0x1000
  LOAD           0x0000000000002000 0x0000000000102000 0x0000000000102000
                 0x0000000000000000 0x0000000000000018  RW     0x1000
  GNU_STACK      0x0000000000000000 0x0000000000000000 0x0000000000000000
                 0x0000000000000000 0x0000000000000000  RW     0x0

 Section to Segment mapping:
  Segment Sections...
   00
   01     .rodata
   02     .text
   03     .bss
   04
```
## Day 6
- asmfunc.asm ってなんなのか気になる。あまり良くわかっていないけれど、取り敢えず進んであとから戻る。

- undefined symbol: std::get_new_handler()が出たが、libcxx_support.cppを足したら問題なくなった。

## 2022/03/02
- Resume developping OS on Ubuntu
- Start Issue driven development
- When I delete return in the printk function, host OS ubuntu was broken.
- Now it resolves but awaking QEMU problem has occurred

## 2022/03/03
- Resolve buggy behavior
- Don't change the order of member variables of struct
- They're set to the struct in the order

## 2022/03/07
### Inline Assembers
- cli: Clear Interrupt flag
  - Interrupt Flag of the CPU is set 0
- sti: Set Interrupt flag
  - Interrupt Flag of the CPU is set 1
- hlt: CPU is set as save power mode since the next intterrupt
