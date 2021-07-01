# UhyokOS
- OS of Uhyoko, by Uhyoko, for Uhyoko from MikanOS (¦3[▓▓]

## Environment
macOS Big Sur ver. 11.4 2021/07/01
MacBook (Retina, 12-inch, Early 2015)
CPU:1.3 GHz デュアルコアIntel Core M

Thanks!
[Mac で始める「ゼロからのOS自作入門」 - Qiita](https://qiita.com/yamoridon/items/4905765cc6e4f320c9b5)

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
