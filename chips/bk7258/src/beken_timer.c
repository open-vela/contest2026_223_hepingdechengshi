/****************************************************************************
 *
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
#include "syslog.h"
#include "systick.h"
#include <nuttx/arch.h>
#include <nuttx/timers/arch_timer.h>

void up_timer_initialize(void) {
  syslog(LOG_INFO, "up_timer_initialize\n");
  FAR struct timer_lowerhalf_s *lower;
  lower = systick_initialize(true, 120000000, -1);
  if (lower == NULL) {
    syslog(LOG_ERR, "ERROR: systick_initialize failed\n");
    return;
  }

  up_timer_set_lowerhalf(lower);
}