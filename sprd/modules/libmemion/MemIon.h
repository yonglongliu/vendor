/*
 * Copyright 2015 The Android Open Source Project
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

#ifndef MEMION_H_
#define MEMION_H_

#include <stdlib.h>
#include <stdint.h>
#include <utils/SortedVector.h>
#include <utils/threads.h>

namespace android {

// ---------------------------------------------------------------------------

class MemIon : public RefBase {
 public:
  enum {
    READ_ONLY = 0x00000001,
    // memory won't be mapped locally, but will be mapped in the remote
    // process.
    DONT_MAP_LOCALLY = 0x00000100,
    NO_CACHING = 0x00000200
  };

  MemIon();
  MemIon(const char *, size_t, uint32_t, unsigned int);
  ~MemIon();
  int getHeapID() const;
  void *getBase() const;
  int getIonDeviceFd() const;

  static int Get_phy_addr_from_ion(int fd, unsigned long *phy_addr,
                                   size_t *size);
  int get_phy_addr_from_ion(unsigned long *phy_addr, size_t *size);
  static int Flush_ion_buffer(int buffer_fd, void *v_addr, void *p_addr,
                              size_t size);
  int flush_ion_buffer(void *v_addr, void *p_addr, size_t size);

  int Get_kaddr(int buffer_fd, uint64_t *kaddr, size_t *size);
  int get_kaddr(uint64_t *kaddr, size_t *size);

  int Free_kaddr(int buffer_fd);
  int free_kaddr();

 private:
  status_t mapIonFd(int fd, size_t size, unsigned int memory_type, int flags);

  int mIonDeviceFd; /*fd we get from open("/dev/ion")*/
  int mIonHandle;   /*handle we get from ION_IOC_ALLOC*/
  int mFD;
  size_t mSize;
  void *mBase;
};
// ---------------------------------------------------------------------------
};  // namespace android

#endif  // MEMION_H_
