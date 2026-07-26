#include <XEngine/Test/TestSupport.h>

// Intentionally empty for now. Real helpers (path helpers, GPU validation
// message collectors, Vulkan test contexts, readback helpers) will land here
// in later stages. Keeping one TU so the target produces a real library rather
// than an INTERFACE stub, which keeps downstream CMake target existence
// checks (if(TARGET XEngineTestSupport)) simple.