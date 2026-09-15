// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// The following only applies to changes made to this file as part of YugabyteDB development.
//
// Portions Copyright (c) YugabyteDB, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
// in compliance with the License.  You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
// or implied.  See the License for the specific language governing permissions and limitations
// under the License.

#pragma once

#include <atomic>

#include "yb/gutil/ref_counted.h"
#include "yb/master/leader_epoch.h"
#include "yb/master/master_fwd.h"
#include "yb/util/condition_variable.h"
#include "yb/util/mutex.h"
#include "yb/util/status_fwd.h"

namespace yb {

class Thread;

namespace master {

class CatalogManager;

int32_t TEST_catalog_manager_cdc_bg_task_run_count();

class CatalogManagerCdcBgTasks final {
 public:
  explicit CatalogManagerCdcBgTasks(Master* master);
  ~CatalogManagerCdcBgTasks();

  Status Init();
  void Shutdown();

  void Wake();

 private:
  void Run();
  void RunOnceAsLeader(const LeaderEpoch& epoch);
  void Wait(int msec);

  std::atomic<bool> closing_;
  mutable Mutex lock_;
  ConditionVariable cond_;
  scoped_refptr<yb::Thread> thread_;
  CatalogManager* catalog_manager_;
};

}  // namespace master
}  // namespace yb
