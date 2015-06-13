#include <algorithm>
#include "coreinit.h"
#include "coreinit_dynload.h"
#include "coreinit_memory.h"
#include "coreinit_expheap.h"
#include "interpreter.h"
#include "thread.h"

static wfunc_ptr<int, int, int, be_val<uint32_t>*>
pOSDynLoad_MemAlloc;

static wfunc_ptr<void, p32<void>>
pOSDynLoad_MemFree;

template<unsigned N>
static inline uint32_t
OSExecuteCallback(uint32_t addr, uint32_t (&args)[N])
{
   auto state = Thread::getCurrentThread()->getThreadState();
   uint32_t save[N];
   uint32_t result;

   for (auto i = 0u; i < N; ++i) {
      save[i] = state->gpr[3 + i];
      state->gpr[3 + i] = args[i];
   }

   gInterpreter.execute(state, addr);

   result = state->gpr[3];

   for (auto i = 0u; i < N; ++i) {
      state->gpr[3 + i] = save[i];
   }

   return result;
}

int MEM_DynLoad_DefaultAlloc(int size, int alignment, be_val<uint32_t> *outPtr)
{
   if (!outPtr) {
      return 0xBAD10017;
   }

   if (alignment < 0) {
      alignment = std::min(alignment, -4);
   } else {
      alignment = std::max(alignment, 4);
   }

   uint32_t args[] = {
      static_cast<uint32_t>(size),
      static_cast<uint32_t>(alignment)
   };

   *outPtr = OSExecuteCallback(pMEMAllocFromDefaultHeapEx->value, args);
   return 0;
}

void MEM_DynLoad_DefaultFree(p32<void> addr)
{
   uint32_t args[] = {
      static_cast<uint32_t>(addr)
   };

   OSExecuteCallback(pMEMFreeToDefaultHeap->value, args);
}

int
OSDynLoad_SetAllocator(uint32_t allocFn, uint32_t freeFn)
{
   if (!allocFn || !freeFn) {
      return 0xBAD10017;
   }

   pOSDynLoad_MemAlloc = allocFn;
   pOSDynLoad_MemFree = freeFn;
   return 0;
}

int
OSDynLoad_GetAllocator(be_val<uint32_t> *outAllocFn, be_val<uint32_t> *outFreeFn)
{
   *outAllocFn = static_cast<uint32_t>(pOSDynLoad_MemAlloc);
   *outFreeFn = static_cast<uint32_t>(pOSDynLoad_MemFree);
   return 0;
}

// Wrapper func to call function pointer set by OSDynLoad_SetAllocator
int
OSDynLoad_MemAlloc(int size, int alignment, uint32_t *outPtr)
{
   auto handle = MEMGetBaseHeapHandle(BaseHeapType::MEM2);
   *outPtr = static_cast<uint32_t>(MEMAllocFromExpHeapEx(handle, size, alignment));
   return 0;

   // TODO: Fix user dynload callback
   /*
   auto state = Thread::getCurrentThread()->getThreadState();
   state->gpr[1] -= 4;

   auto stack = state->gpr[1];

   uint32_t args[] = {
      size,
      alignment,
      stack
   };

   auto result = OSExecuteCallback(pOSDynLoad_MemAlloc->value, args);
   *outPtr = gMemory.read<uint32_t>(stack);
   state->gpr[1] += 4;
   return result;
   */
}

// Wrapper func to call function pointer set by OSDynLoad_SetAllocator
void
OSDynLoad_MemFree(p32<void> addr)
{
   auto handle = MEMGetBaseHeapHandle(BaseHeapType::MEM2);
   return MEMFreeToExpHeap(handle, addr);

   // TODO: Fix user dynload callback
   /*
   uint32_t args[] = {
      addr.value
   };

   OSExecuteCallback(pOSDynLoad_MemFree->value, args);
   */
}

int
OSDynLoad_Acquire(char const *name, be_val<uint32_t> *outHandle)
{
   *outHandle = 0;
   return -1;
}

void
OSDynLoad_Release(DynLoadModuleHandle handle)
{
}

void
CoreInit::registerDynLoadFunctions()
{
   RegisterSystemFunction(OSDynLoad_Acquire);
   RegisterSystemFunction(OSDynLoad_Release);
   RegisterSystemFunction(OSDynLoad_SetAllocator);
   RegisterSystemFunction(OSDynLoad_GetAllocator);
   RegisterSystemFunction(MEM_DynLoad_DefaultAlloc);
   RegisterSystemFunction(MEM_DynLoad_DefaultFree);
}

void
CoreInit::initialiseDynLoad()
{
   pOSDynLoad_MemAlloc = findExportAddress("MEM_DynLoad_DefaultAlloc");
   pOSDynLoad_MemFree = findExportAddress("MEM_DynLoad_DefaultFree");
}
