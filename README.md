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

## 2022/03/14
- Finish #8 memory_map
- Loader needs symbol link for memory_map.h because Main.c needs.
- Check Loader side and build again when change Loader codes

## 2022/03/24
- Finish #11 timer
- ACPI is Advanced Configuration and Power Interface
- Frequency of ACPI is able to be count, so we use it as a timer.

## 2022/03/29
- #13 Context Switch
- PML4 Table is described in #8
- To prevent some program forgets switch, make it automation

## 2022/04/04
- Finish #16 Commands
- I found a bug that initialize active window
- The relationships of layer ids is sus
- There some diffs from originas : framebuffer, layer, mouse, window cpp/hpp

## 2022/04/05
```
~ ❯❯❯ dd if=/dev/zero of=fat_disk bs=1M count=128
128+0 records in
128+0 records out
134217728 bytes (134 MB, 128 MiB) copied, 0.0438862 s, 3.1 GB/s
~ ❯❯❯ mkfs.fat -n 'UHYOKO OS' -s 2 -f 2 -R 32 -F 32 fat_disk
mkfs.fat 4.2 (2021-01-31)
~ ❯❯❯ hexdump -C fat_disk
00000000  eb 58 90 6d 6b 66 73 2e  66 61 74 00 02 02 20 00  |.X.mkfs.fat... .|
00000010  02 00 00 00 00 f8 00 00  20 00 08 00 00 00 00 00  |........ .......|
00000020  00 00 04 00 f8 03 00 00  00 00 00 00 02 00 00 00  |................|
00000030  01 00 06 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
00000040  80 00 29 89 ea 36 b2 55  48 59 4f 4b 4f 20 4f 53  |..)..6.UHYOKO OS|
00000050  20 20 46 41 54 33 32 20  20 20 0e 1f be 77 7c ac  |  FAT32   ...w|.|
00000060  22 c0 74 0b 56 b4 0e bb  07 00 cd 10 5e eb f0 32  |".t.V.......^..2|
00000070  e4 cd 16 cd 19 eb fe 54  68 69 73 20 69 73 20 6e  |.......This is n|
00000080  6f 74 20 61 20 62 6f 6f  74 61 62 6c 65 20 64 69  |ot a bootable di|
00000090  73 6b 2e 20 20 50 6c 65  61 73 65 20 69 6e 73 65  |sk.  Please inse|
000000a0  72 74 20 61 20 62 6f 6f  74 61 62 6c 65 20 66 6c  |rt a bootable fl|
000000b0  6f 70 70 79 20 61 6e 64  0d 0a 70 72 65 73 73 20  |oppy and..press |
000000c0  61 6e 79 20 6b 65 79 20  74 6f 20 74 72 79 20 61  |any key to try a|
000000d0  67 61 69 6e 20 2e 2e 2e  20 0d 0a 00 00 00 00 00  |gain ... .......|
000000e0  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
*
000001f0  00 00 00 00 00 00 00 00  00 00 00 00 00 00 55 aa  |..............U.|
00000200  52 52 61 41 00 00 00 00  00 00 00 00 00 00 00 00  |RRaA............|
00000210  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
*
000003e0  00 00 00 00 72 72 41 61  f7 fb 01 00 02 00 00 00  |....rrAa........|
000003f0  00 00 00 00 00 00 00 00  00 00 00 00 00 00 55 aa  |..............U.|
00000400  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
*
00000c00  eb 58 90 6d 6b 66 73 2e  66 61 74 00 02 02 20 00  |.X.mkfs.fat... .|
00000c10  02 00 00 00 00 f8 00 00  20 00 08 00 00 00 00 00  |........ .......|
00000c20  00 00 04 00 f8 03 00 00  00 00 00 00 02 00 00 00  |................|
00000c30  01 00 06 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
00000c40  80 00 29 89 ea 36 b2 55  48 59 4f 4b 4f 20 4f 53  |..)..6.UHYOKO OS|
00000c50  20 20 46 41 54 33 32 20  20 20 0e 1f be 77 7c ac  |  FAT32   ...w|.|
00000c60  22 c0 74 0b 56 b4 0e bb  07 00 cd 10 5e eb f0 32  |".t.V.......^..2|
00000c70  e4 cd 16 cd 19 eb fe 54  68 69 73 20 69 73 20 6e  |.......This is n|
00000c80  6f 74 20 61 20 62 6f 6f  74 61 62 6c 65 20 64 69  |ot a bootable di|
00000c90  73 6b 2e 20 20 50 6c 65  61 73 65 20 69 6e 73 65  |sk.  Please inse|
00000ca0  72 74 20 61 20 62 6f 6f  74 61 62 6c 65 20 66 6c  |rt a bootable fl|
00000cb0  6f 70 70 79 20 61 6e 64  0d 0a 70 72 65 73 73 20  |oppy and..press |
00000cc0  61 6e 79 20 6b 65 79 20  74 6f 20 74 72 79 20 61  |any key to try a|
00000cd0  67 61 69 6e 20 2e 2e 2e  20 0d 0a 00 00 00 00 00  |gain ... .......|
00000ce0  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
*
00000df0  00 00 00 00 00 00 00 00  00 00 00 00 00 00 55 aa  |..............U.|
00000e00  52 52 61 41 00 00 00 00  00 00 00 00 00 00 00 00  |RRaA............|
00000e10  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
*
00000fe0  00 00 00 00 72 72 41 61  f7 fb 01 00 02 00 00 00  |....rrAa........|
00000ff0  00 00 00 00 00 00 00 00  00 00 00 00 00 00 55 aa  |..............U.|
00001000  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
*
00004000  f8 ff ff 0f ff ff ff 0f  f8 ff ff 0f 00 00 00 00  |................|
00004010  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
*
00083000  f8 ff ff 0f ff ff ff 0f  f8 ff ff 0f 00 00 00 00  |................|
00083010  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
*
00102000  55 48 59 4f 4b 4f 20 4f  53 20 20 08 00 00 55 ae  |UHYOKO OS  ...U.|
00102010  85 54 85 54 00 00 55 ae  85 54 00 00 00 00 00 00  |.T.T..U..T......|
00102020  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
*
08000000
~ ❯❯❯ hexdump -C -s 16k fat_disk
00004000  f8 ff ff 0f ff ff ff 0f  f8 ff ff 0f 00 00 00 00  |................|
00004010  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
*
00083000  f8 ff ff 0f ff ff ff 0f  f8 ff ff 0f 00 00 00 00  |................|
00083010  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
*
00102000  55 48 59 4f 4b 4f 20 4f  53 20 20 08 00 00 55 ae  |UHYOKO OS  ...U.|
00102010  85 54 85 54 00 00 55 ae  85 54 00 00 00 00 00 00  |.T.T..U..T......|
00102020  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
*
08000000
~ ❯❯❯ mkdir mnt
~ ❯❯❯ sudo mount -o loop fat_disk mnt
~ ❯❯❯ ls -a mnt
.  ..
~ ❯❯❯ echo uhyokodayo > test.txt
~ ❯❯❯ sudo cp test.txt mnt/test.txt                                                                                                                                                      ✘ 1 
~ ❯❯❯ ls -a mnt
.  ..  test.txt
~ ❯❯❯ sudo umount mnt                                                                                                                                                                    ✘ 1 
~ ❯❯❯ hexdump -C -s 16k fat_disk
00004000  f8 ff ff 0f ff ff ff 0f  f8 ff ff 0f ff ff ff 0f  |................|
00004010  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
*
00083000  f8 ff ff 0f ff ff ff 0f  f8 ff ff 0f ff ff ff 0f  |................|
00083010  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
*
00102000  55 48 59 4f 4b 4f 20 4f  53 20 20 08 00 00 55 ae  |UHYOKO OS  ...U.|
00102010  85 54 85 54 00 00 55 ae  85 54 00 00 00 00 00 00  |.T.T..U..T......|
00102020  41 74 00 65 00 73 00 74  00 2e 00 0f 00 8f 74 00  |At.e.s.t......t.|
00102030  78 00 74 00 00 00 ff ff  ff ff 00 00 ff ff ff ff  |x.t.............|
00102040  54 45 53 54 20 20 20 20  54 58 54 20 00 7e 2c 6a  |TEST    TXT .~,j|
00102050  85 54 85 54 00 00 2c 6a  85 54 03 00 0b 00 00 00  |.T.T..,j.T......|
00102060  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
*
00102400  75 68 79 6f 6b 6f 64 61  79 6f 0a 00 00 00 00 00  |uhyokodayo......|
00102410  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
*
08000000
~ ❯❯❯ sudo mount -o loop fat_disk mnt
~ ❯❯❯ echo MofuMofu! > HelloWorld.data
~ ❯❯❯ sudo cp HelloWorld.data mnt/HeloWorld.data
~ ❯❯❯ sudo umount mnt
~ ❯❯❯ hexdump -C -s 16k fat_disk
00004000  f8 ff ff 0f ff ff ff 0f  f8 ff ff 0f ff ff ff 0f  |................|
00004010  ff ff ff 0f 00 00 00 00  00 00 00 00 00 00 00 00  |................|
00004020  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
*
00083000  f8 ff ff 0f ff ff ff 0f  f8 ff ff 0f ff ff ff 0f  |................|
00083010  ff ff ff 0f 00 00 00 00  00 00 00 00 00 00 00 00  |................|
00083020  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
*
00102000  55 48 59 4f 4b 4f 20 4f  53 20 20 08 00 00 55 ae  |UHYOKO OS  ...U.|
00102010  85 54 85 54 00 00 55 ae  85 54 00 00 00 00 00 00  |.T.T..U..T......|
00102020  41 74 00 65 00 73 00 74  00 2e 00 0f 00 8f 74 00  |At.e.s.t......t.|
00102030  78 00 74 00 00 00 ff ff  ff ff 00 00 ff ff ff ff  |x.t.............|
00102040  54 45 53 54 20 20 20 20  54 58 54 20 00 7e 2c 6a  |TEST    TXT .~,j|
00102050  85 54 85 54 00 00 2c 6a  85 54 03 00 0b 00 00 00  |.T.T..,j.T......|
00102060  42 61 00 00 00 ff ff ff  ff ff ff 0f 00 e9 ff ff  |Ba..............|
00102070  ff ff ff ff ff ff ff ff  ff ff 00 00 ff ff ff ff  |................|
00102080  01 48 00 65 00 6c 00 6f  00 57 00 0f 00 e9 6f 00  |.H.e.l.o.W....o.|
00102090  72 00 6c 00 64 00 2e 00  64 00 00 00 61 00 74 00  |r.l.d...d...a.t.|
001020a0  48 45 4c 4f 57 4f 7e 31  44 41 54 20 00 c4 d4 6a  |HELOWO~1DAT ...j|
001020b0  85 54 85 54 00 00 d4 6a  85 54 04 00 0a 00 00 00  |.T.T...j.T......|
001020c0  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
*
00102400  75 68 79 6f 6b 6f 64 61  79 6f 0a 00 00 00 00 00  |uhyokodayo......|
00102410  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
*
00102800  4d 6f 66 75 4d 6f 66 75  21 0a 00 00 00 00 00 00  |MofuMofu!.......|
00102810  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
*
08000000
```
- [File Allocation Table Wiki](https://en.wikipedia.org/wiki/File_Allocation_Table)
- The current ls shows the name of volume which is set in osbook/devenv/make_image.sh

## 2022/04/22
- #18 Application
```
$ make
nasm -f bin -o onlyhlt onlyhlt.asm
$ hexdump -C onlyhlt
00000000  f4 eb fd                                          |...|
00000003
$ objdump -D -m i386:x86-64 -b binary onlyhlt

onlyhlt:     file format binary


Disassembly of section .data:

0000000000000000 <.data>:
   0:	f4                   	hlt    
   1:	eb fd                	jmp    0x0

```
- eb fd is relative short jump