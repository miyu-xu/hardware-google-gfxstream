/*
 * Copyright (C) 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <condition_variable>
#include <mutex>
#include <stdint.h>

namespace gfxstream {

class SimpleLatch {
   public:
    SimpleLatch(std::uint32_t count) : mCount(count) {}

    SimpleLatch(const SimpleLatch&) = delete;
    SimpleLatch& operator=(const SimpleLatch&) = delete;

    SimpleLatch(SimpleLatch&&) = delete;
    SimpleLatch& operator=(SimpleLatch&&) = delete;

    void count_down() {
        {
            std::unique_lock lock(mMutex);
            --mCount;
        }
        mConditionVariable.notify_all();
    }

    void wait() {
        std::unique_lock lock(mMutex);
        mConditionVariable.wait(lock, [this] { return mCount == 0; });
    }

   private:
    std::mutex mMutex;
    std::condition_variable mConditionVariable;
    std::uint32_t mCount;
};

}  // namespace gfxstream
