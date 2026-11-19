// Leak detection is off for every fuzz/standalone binary (leaks are not the bug class fuzzed here);
// ASan's memory-error checks and UBSan stay fully on. Default visibility: upstream compiles with
// -fvisibility=hidden, which would otherwise turn the hook into a local symbol.
extern "C" __attribute__((visibility("default"))) int __lsan_is_turned_off() { return 1; }
