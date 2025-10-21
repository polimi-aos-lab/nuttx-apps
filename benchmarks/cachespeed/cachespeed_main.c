/****************************************************************************
 * apps/benchmarks/cachespeed/cachespeed_main.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/arch.h>
#include <nuttx/cache.h>
#include <nuttx/config.h>
#include <nuttx/irq.h>

#include <inttypes.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CACHESPEED_PREFIX "CACHE Speed: "
#define REPEAT_NUM 1000

extern int get_current_timer_nanoseconds(clockid_t, struct timespec *time);


#define TIME time_t

#define CONVERT(cost)

#define TIMESTAMP(x) \
do \
{ \
  struct timespec ts; \
  get_current_timer_nanoseconds(CLOCK_MONOTONIC, &ts); \
  x = ts.tv_sec * 1000000000 + ts.tv_nsec; \
} while (0)

#define GET_DCACHE_LINE up_get_dcache_linesize()
#define GET_ICACHE_LINE up_get_icache_linesize()
#define GET_DCACHE_SIZE up_get_dcache_size()
#define GET_ICACHE_SIZE up_get_icache_size()

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct cachespeed_s
{
  uintptr_t addr;
  size_t alloc;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: setup
 ****************************************************************************/

static void setup(FAR struct cachespeed_s *cs)
{
  struct mallinfo info = mallinfo();

  /* Get the currently available memory from the system. We want the
   * memset range to be as large as possible in our tests to ensure
   * that the cache is filled with our dirty data
   */

  // we want to allocate 0x40000 byte of memory
  info.fordblks = (1UL << 18);

  cs->alloc = info.fordblks / 2;
  cs->addr = (uintptr_t)malloc(cs->alloc);
  if (cs->addr == 0)
    {
      printf(CACHESPEED_PREFIX "Unable to request memory.\n");
      exit(EXIT_FAILURE);
    }

  /* Let's export the test message */

  printf(CACHESPEED_PREFIX "address src: %" PRIxPTR "\n", cs->addr);
}

/****************************************************************************
 * Name: teardown
 ****************************************************************************/

static void teardown(FAR struct cachespeed_s *cs)
{
  free((void *)cs->addr);
  printf(CACHESPEED_PREFIX "Done!\n");
}

/****************************************************************************
 * Name: report_line
 ****************************************************************************/

static void report_line(const char *name, size_t bytes, TIME cost)
{
  printf("[cachespeed] %s %lu %lu\n", name, bytes, cost);
}

/****************************************************************************
 * Name: test_skeleton
 ****************************************************************************/

static void test_skeleton(FAR struct cachespeed_s *cs,
                          const size_t cache_size,
                          const size_t cache_line_size, 
                          int align,
                          void (*func)(uintptr_t, uintptr_t),
                          const char *name)
{
  size_t update_size;

  if (!align)
    {
      update_size = cache_line_size - 1;
    }
  else
    {
      update_size = cache_line_size;
    }

  for (size_t bytes = update_size;
       bytes <= cache_size; bytes = 2 * bytes)
    {
      irqstate_t irq;
      TIME start;
      TIME end;
      TIME cost = 0;

      /* Make sure that test with all the contents
       * of our address in the cache.
       */

      up_flush_dcache_all();

      for (int i = 0; i < REPEAT_NUM; i++)
        {
          irq = enter_critical_section();
          memset((void *)cs->addr, 1, cs->alloc);
          TIMESTAMP(start);
          func(cs->addr, (uintptr_t)(cs->addr + bytes));
          TIMESTAMP(end);
          leave_critical_section(irq);
          report_line(name, bytes, end - start);
          cost += end - start;
        }

      //report_line(bytes, cost);
    }
}

/****************************************************************************
 * Name: cachespeed_common
 ****************************************************************************/

 extern void reboot_test_finished(void);
static void cachespeed_common(struct cachespeed_s *cs)
{
  printf("---- start test ----\n");
  test_skeleton(cs, GET_DCACHE_SIZE, GET_DCACHE_LINE, 1,
                up_invalidate_dcache, "dcache-invalidate-align");
  
  test_skeleton(cs, GET_DCACHE_SIZE, GET_DCACHE_LINE, 0,
                up_invalidate_dcache, "dcache-invalidate-noalign");
  
  test_skeleton(cs, GET_DCACHE_SIZE, GET_DCACHE_LINE, 1,
                up_clean_dcache, "dcache-clean-align");

  test_skeleton(cs, GET_DCACHE_SIZE, GET_DCACHE_LINE, 0,
                up_clean_dcache, "dcache-clean-noalign");
  
  test_skeleton(cs, GET_DCACHE_SIZE, GET_DCACHE_LINE, 1,
                up_flush_dcache, "dcache-flush-align");
  
  test_skeleton(cs, GET_DCACHE_SIZE, GET_DCACHE_LINE, 0,
                up_flush_dcache, "dcache-flush-noalign");
  
  test_skeleton(cs, GET_ICACHE_SIZE, GET_ICACHE_LINE, 1,
                up_invalidate_icache, "icache-invalidate-align");
  
  test_skeleton(cs, GET_ICACHE_SIZE, GET_ICACHE_LINE, 0,
                up_invalidate_icache, "icache-invalidate-noalign");
  printf("---- stop test ----\n");
  reboot_test_finished();

}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cachespeed_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct cachespeed_s cs =
    {
      .addr = 0,
      .alloc = 0
    };

  printf("start setup\n");
  setup(&cs);
  printf("start cachespeed_common\n");
  cachespeed_common(&cs);
  teardown(&cs);
  return 0;
}
