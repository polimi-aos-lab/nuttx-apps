/****************************************************************************
 * apps/benchmarks/spinlock_bench/spinlock_bench.c
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

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <nuttx/spinlock.h>
#include <time.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct thread_parmeter_s
{
  FAR int *result;
  FAR spinlock_t *lock;
  FAR int index;
};

static int latencys[CONFIG_SPINLOCK_ITERATIONS];

/****************************************************************************
 * Private Functions
 ****************************************************************************/
extern unsigned long get_current_nanosecond(void);

static FAR void *thread_spinlock(FAR void *parameter)
{
  FAR int *result = ((FAR struct thread_parmeter_s *)parameter)->result;
  FAR spinlock_t *lock = ((FAR struct thread_parmeter_s *)parameter)->lock;
  FAR int index = ((FAR struct thread_parmeter_s *)parameter)->index;

  int i;
  unsigned diff;

  for (i = 0; i < CONFIG_SPINLOCK_ITERATIONS; i++)
    {
      diff = get_current_nanosecond();
      spin_lock(lock);
      (*result)++;
      spin_unlock(lock);
      diff = get_current_nanosecond() - diff;
      if (index == 0)
        latencys[i] = diff;
    }

  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/


void main(void)
{
  spinlock_t lock = SP_UNLOCKED;
  int result = 0;
  pthread_t thread[CONFIG_SPINLOCK_MULTITHREAD];
  struct thread_parmeter_s para[CONFIG_SPINLOCK_MULTITHREAD];
  clock_t start;
  clock_t end;

  int status;
  int i;

  for (i = 0; i < CONFIG_SPINLOCK_MULTITHREAD; ++i)
    {
      para[i].result = &result;
      para[i].lock = &lock;
      para[i].index = i;
    }

  start = get_current_nanosecond();
  for (i = 0; i < CONFIG_SPINLOCK_MULTITHREAD; ++i)
    {
      status = pthread_create(&thread[i], NULL,
                              thread_spinlock, &para[i]);
      if (status != 0)
        {
          printf("spinlock_test: ERROR pthread_create failed, status=%d\n",
                 status);
          ASSERT(false);
        }
    }

  for (i = 0; i < CONFIG_SPINLOCK_MULTITHREAD; ++i)
    {
      pthread_join(thread[i], NULL);
    }

  end = get_current_nanosecond();

  printf("total_time: %lu\n", end - start);

  for (i = 0; i < CONFIG_SPINLOCK_ITERATIONS; i++)
    printf("latency: %u\n", latencys[i]);
}
