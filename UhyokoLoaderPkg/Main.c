#include  <Uefi.h>
#include  <Library/UefiLib.h>

/* メモリマップという構造体を作り、メモリの状態を保管する */
struct MemoryMap {
  UINTN buffer_size;
  VOID* buffer;
  UINTN map_size;
  UINTN map_key;
  UINTN descriptor_size;
  UINT32 descriptor_version;
};

/* メモリマップを習得
   UEFIは大きく分けてブートサービス(gBS)とラインタイムサービス(gRT)に分かれる
   メモリ管理はブートサービスが管理する
*/
EFI_STATUS GetMemoryMap(struct MemoryMap* map) {
  return EFI_SUCCESS;
}

/* メモリマップのファイル保存
   EFI_FILE_PROTOCOL：UEFIではFATファイルシステムを扱える
*/
EFI_STATUS SaveMemoryMap(struct MemoryMap* map, EFI_FILE_PROTOCOL* file) {
  return EFI_SUCCESS;
}

/* UEFI:OSとファームウェアを仲介するソフトウェアインターフェース */
EFI_STATUS EFIAPI UefiMain(
    EFI_HANDLE image_handle,
    EFI_SYSTEM_TABLE *system_table) {
  Print(L"Hello, Mofumofu World! (:3 \n");
  while (1);
  return EFI_SUCCESS;
}
