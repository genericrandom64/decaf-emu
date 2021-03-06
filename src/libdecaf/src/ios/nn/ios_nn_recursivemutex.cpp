#include "ios_nn_recursivemutex.h"

#include "ios/kernel/ios_kernel_thread.h"

#include <common/decaf_assert.h>

using namespace ios::kernel;

namespace nn
{

void
RecursiveMutex::lock()
{
   auto threadId = IOS_GetCurrentThreadId();
   if (mRecursionCount == 0) {
      mOwnerThread = threadId;
   }

   if (mOwnerThread != static_cast<ThreadId>(threadId)) {
      mCriticalSection.lock();
      mOwnerThread = threadId;
      decaf_check(mRecursionCount == 0);
   }

   ++mRecursionCount;
}

bool
RecursiveMutex::try_lock()
{
   auto threadId = IOS_GetCurrentThreadId();
   if (mRecursionCount == 0) {
      mOwnerThread = threadId;
   }

   if (mOwnerThread != static_cast<ThreadId>(threadId)) {
      return false;
   }

   ++mRecursionCount;
   return true;
}

void
RecursiveMutex::unlock()
{
   decaf_check(locked());
   --mRecursionCount;

   if (mRecursionCount == 0) {
      mOwnerThread = static_cast<ThreadId>(-1);
      mCriticalSection.unlock();
   }
}

bool
RecursiveMutex::locked()
{
   return mRecursionCount > 0 && mOwnerThread == static_cast<ThreadId>(IOS_GetCurrentThreadId());
}

} // namespace nn
