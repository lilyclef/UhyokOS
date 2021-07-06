/*
  KernelMain()がブートローダから呼び出される
  エントリポイントと呼ぶ
*/
extern "C" void KernelMain() {
  while (1) __asm__("hlt");
}
