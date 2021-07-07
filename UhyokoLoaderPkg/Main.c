#include  <Uefi.h>
#include  <Library/UefiLib.h>
#include  <Library/UefiBootServicesTableLib.h>
#include  <Library/PrintLib.h>
#include  <Protocol/LoadedImage.h>
#include  <Protocol/SimpleFileSystem.h>
#include  <Protocol/DiskIo2.h>
#include  <Protocol/BlockIo.h>
#include  <Guid/FileInfo.h>

/* メモリマップという構造体を作り、メモリの状態を保管する */
struct MemoryMap {
  UINTN buffer_size;
  VOID* buffer;
  UINTN map_size;
  UINTN map_key;
  UINTN descriptor_size;
  UINT32 descriptor_version;
};

/*
   メモリマップを習得
   UEFIは大きく分けてブートサービス(gBS)とラインタイムサービス(gRT)に分かれる
   #include  <Library/UefiBootServicesTableLib.h>
   メモリ管理はブートサービスが管理する
   UEFIのSpecification
*/

EFI_STATUS GetMemoryMap(struct MemoryMap* map) {
  /*
    https://uefi.org/sites/default/files/resources/UEFI_Spec_2_9_2021_03_18.pdf
    If the MemoryMap buffer is too small, the EFI_BUFFER_TOO_SMALL error code is returned and the MemoryMapSize value contains the size of the buffer needed to contain the current memory map. The actual size of the buffer allocated for the consequent call to GetMemoryMap() should be bigger then the value returned in MemoryMapSize, since allocation of the new buffer may potentially increase memory map size.
  */
  if (map->buffer == NULL) {
    return EFI_BUFFER_TOO_SMALL;
  }

  map->map_size = map->buffer_size;
  return gBS->GetMemoryMap(
      &map->map_size,
      (EFI_MEMORY_DESCRIPTOR*)map->buffer,
      &map->map_key,
      &map->descriptor_size,
      &map->descriptor_version);
  return EFI_SUCCESS;
}

/*
  メモリマップをファイル保存するときの型から文字列への翻訳
*/
const CHAR16* GetMemoryTypeUnicode(EFI_MEMORY_TYPE type) {
  switch (type) {
    case EfiReservedMemoryType: return L"EfiReservedMemoryType";
    case EfiLoaderCode: return L"EfiLoaderCode";
    case EfiLoaderData: return L"EfiLoaderData";
    case EfiBootServicesCode: return L"EfiBootServicesCode";
    case EfiBootServicesData: return L"EfiBootServicesData";
    case EfiRuntimeServicesCode: return L"EfiRuntimeServicesCode";
    case EfiRuntimeServicesData: return L"EfiRuntimeServicesData";
    case EfiConventionalMemory: return L"EfiConventionalMemory";
    case EfiUnusableMemory: return L"EfiUnusableMemory";
    case EfiACPIReclaimMemory: return L"EfiACPIReclaimMemory";
    case EfiACPIMemoryNVS: return L"EfiACPIMemoryNVS";
    case EfiMemoryMappedIO: return L"EfiMemoryMappedIO";
    case EfiMemoryMappedIOPortSpace: return L"EfiMemoryMappedIOPortSpace";
    case EfiPalCode: return L"EfiPalCode";
    case EfiPersistentMemory: return L"EfiPersistentMemory";
    case EfiMaxMemoryType: return L"EfiMaxMemoryType";
    default: return L"InvalidMemoryType";
  }
}


/* メモリマップのファイル保存
   EFI_FILE_PROTOCOL：UEFIではFATファイルシステムを扱える
*/
EFI_STATUS SaveMemoryMap(struct MemoryMap* map, EFI_FILE_PROTOCOL* outFile) {
  CHAR8 buf[256];
  UINTN len;

  CHAR8* header =
    "Index, Type, Type(name), PhysicalStart, NumberOfPages, Attribute\n";
  /*
    AsciiStrLen関数は、~/edk2/MdePkg/Library/BaseLib/String.cに居た
  */
  len = AsciiStrLen(header);
  outFile->Write(outFile, &len, header);

  Print(L"map->buffer = %08lx, map->map_size = %08lx\n",
      map->buffer, map->map_size);

  EFI_PHYSICAL_ADDRESS iter;
  int i;
  for (iter = (EFI_PHYSICAL_ADDRESS)map->buffer, i = 0;
       iter < (EFI_PHYSICAL_ADDRESS)map->buffer + map->map_size;
       iter += map->descriptor_size, i++) {
    EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)iter;

    // AsciiSPrint関数は #include  <Library/PrintLib.h> がないとビルドできない P.60
    len = AsciiSPrint(
        buf, sizeof(buf),
        "%u, %x, %-ls, %08lx, %lx, %lx\n",
        i, desc->Type, GetMemoryTypeUnicode(desc->Type),
        desc->PhysicalStart, desc->NumberOfPages,
        desc->Attribute & 0xffffflu);
    outFile->Write(outFile, &len, buf);
  }

  return EFI_SUCCESS;
}

/*
  ファイルオープンのために必要そうに見える関数
  Protocol系ライブラリをincludeする必要がある
  OpenRootDirは教科書にはないけど実装にあった。
*/
EFI_STATUS OpenRootDir(EFI_HANDLE image_handle, EFI_FILE_PROTOCOL** root) {
  EFI_LOADED_IMAGE_PROTOCOL* loaded_image;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs;

  /*
    gEfiLoadedImageProtocolGuid等、プロトコルはLoader.infに書き込まないとビルドできない
   */
  gBS->OpenProtocol(
      image_handle,
      &gEfiLoadedImageProtocolGuid,
      (VOID**)&loaded_image,
      image_handle,
      NULL,
      EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

  gBS->OpenProtocol(
      loaded_image->DeviceHandle,
      &gEfiSimpleFileSystemProtocolGuid,
      (VOID**)&fs,
      image_handle,
      NULL,
      EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

  fs->OpenVolume(fs, root);

  return EFI_SUCCESS;
}



/* UEFI:OSとファームウェアを仲介するソフトウェアインターフェース */
EFI_STATUS EFIAPI UefiMain(
    EFI_HANDLE image_handle,
    EFI_SYSTEM_TABLE *system_table) {

  // (:3 とりあえずprint begin
  Print(L"Hello, Mofumofu World! (:3 \n");
  // (:3 とりあえずprint end

  // (:3 メモリマップのプリント begin
  // memmap_bufを確保してみる
  CHAR8 memmap_buf[4096 * 4];
  struct MemoryMap memmap = {sizeof(memmap_buf), memmap_buf, 0, 0, 0, 0};

  // その時のgBSの状況を取ってくる
  GetMemoryMap(&memmap);

  // ファイルを開いて書き込む
  EFI_FILE_PROTOCOL* root_dir;
  OpenRootDir(image_handle, &root_dir);

  EFI_FILE_PROTOCOL* memmap_file;
  root_dir->Open(
      root_dir, &memmap_file, L"\\memmap",
      EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);

  SaveMemoryMap(&memmap, memmap_file);
  memmap_file->Close(memmap_file);
  // (:3 メモリマップのプリント end

  // (:3 kernel.elf 読み込み begin
  EFI_FILE_PROTOCOL* kernel_file;
  root_dir->Open(
                 root_dir, &kernel_file, L"\\kernel.elf",
                 EFI_FILE_MODE_READ, 0);

  // char16の12個分は\kernel.elf\0の12文字
  UINTN file_info_size = sizeof(EFI_FILE_INFO) + sizeof(CHAR16) * 12;

  UINT8 file_info_buffer[file_info_size];
  kernel_file->GetInfo(
                       kernel_file, &gEfiFileInfoGuid,
                       &file_info_size, file_info_buffer);
  EFI_FILE_INFO* file_info = (EFI_FILE_INFO*) file_info_buffer;
  UINTN kernel_file_size = file_info->FileSize;

  EFI_PHYSICAL_ADDRESS kernel_base_addr = 0x100000;

  // メモリ確保のモードは3種類ある
  // AllocateAddressは指定したアドレスに確保する
  // UEFIにおける1ページの大きさは4KiB=0x1000 byte
  // ページ数=(kernel_file_size + 0xfff) / 0x1000
  gBS->AllocatePages(
                     AllocateAddress, EfiLoaderData,
                     (kernel_file_size + 0xfff) / 0x1000, &kernel_base_addr);
  // メモリ確保ができたので、読み込み
  kernel_file->Read(kernel_file, &kernel_file_size, (VOID*)kernel_base_addr);
  Print(L"Kernel: 0x%0lx (%lu bytes)\n", kernel_base_addr, kernel_file_size);
  // (:3 kernel.elf 読み込み end

  // (:3 UEFI BIOSのブートサービスを停止 begin
  EFI_STATUS status;
  status = gBS->ExitBootServices(image_handle, memmap.map_key);
  if (EFI_ERROR(status)) {
    status = GetMemoryMap(&memmap);
    if (EFI_ERROR(status)) {
      Print(L"Failed to get memory map: %r\n", status);
      while(1);
    }
    status = gBS->ExitBootServices(image_handle, memmap.map_key);
    if (EFI_ERROR(status)) {
      Print(L"Could not exit boot service: %r\n", status);
      while(1);
    }
  }
  // (:3 UEFI BIOSのブートサービスを停止 end

  // (:3 カーネル呼び出し begin
  UINT64 entry_addr = *(UINT64*)(kernel_base_addr + 24);

  typedef void EntryPointType(void);
  EntryPointType* entry_point = (EntryPointType*)entry_addr;
  entry_point();
  // (:3 カーネル呼び出し end

  Print(L"Kernel is called (:3 \n");

  while (1);
  return EFI_SUCCESS;
}
