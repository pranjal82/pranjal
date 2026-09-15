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

#include "yb/master/catalog_manager_cdc_bg_tasks.h"

#include "yb/master/catalog_manager.h"
#include "yb/master/cdcsdk_manager.h"
#include "yb/master/master.h"
#include "yb/master/scoped_leader_shared_lock.h"

#include "yb/util/callsite_profiling.h"
#include "yb/util/flag_validators.h"
#include "yb/util/status_log.h"
#include "yb/util/thread.h"

using namespace std::literals;

DEFINE_RUNTIME_int32(cdc_cleanup_bg_task_wait_ms, 60 * 1000,
    "Amount of time the CDC cleanup background task thread waits between runs");
DEFINE_validator(cdc_cleanup_bg_task_wait_ms, FLAG_GT_VALUE_VALIDATOR(0));

DECLARE_bool(TEST_cdcsdk_disable_deleted_stream_cleanup);
DECLARE_bool(TEST_cdcsdk_disable_drop_table_cleanup);

namespace yb::master {

namespace {

std::atomic<int32_t> TEST_catalog_manager_cdc_bg_task_run_counter{0};

}  // namespace

int32_t TEST_catalog_manager_cdc_bg_task_run_count() {
  return TEST_catalog_manager_cdc_bg_task_run_counter.load();
}

CatalogManagerCdcBgTasks::CatalogManagerCdcBgTasks(Master* master)
    : closing_(false),
      cond_(&lock_),
      thread_(nullptr),
      catalog_manager_(master->catalog_manager_impl()) {}

CatalogManagerCdcBgTasks::~CatalogManagerCdcBgTasks() = default;

void CatalogManagerCdcBgTasks::Wake() {
  MutexLock lock(lock_);
  YB_PROFILE(cond_.Broadcast());
}

void CatalogManagerCdcBgTasks::Wait(int msec) {
  MutexLock lock(lock_);
  if (closing_.load()) {
    return;
  }
  cond_.TimedWait(MonoDelta::FromMilliseconds(msec));
}

Status CatalogManagerCdcBgTasks::Init() {
  RETURN_NOT_OK(yb::Thread::Create(
      "catalog manager", "cdc-bgtasks", &CatalogManagerCdcBgTasks::Run, this, &thread_));
  return Status::OK();
}

void CatalogManagerCdcBgTasks::Shutdown() {
  {
    bool closing_expected = false;
    if (!closing_.compare_exchange_strong(closing_expected, true)) {
      VLOG(2) << "CatalogManagerCdcBgTasks already shut down";
      return;
    }
  }

  Wake();
  if (thread_ != nullptr) {
    CHECK_OK(ThreadJoiner(thread_.get()).Join());
  }
}

void CatalogManagerCdcBgTasks::RunOnceAsLeader(const LeaderEpoch& epoch) {
  if (!FLAGS_TEST_cdcsdk_disable_deleted_stream_cleanup) {
    WARN_NOT_OK(
        catalog_manager_->CleanUpDeletedCDCSDKStreams(epoch),
        "Failed cleaning deleted CDCSDK streams");
  }

  if (!FLAGS_TEST_cdcsdk_disable_drop_table_cleanup) {
    WARN_NOT_OK(
        catalog_manager_->CleanUpCDCSDKStreamsMetadata(epoch),
        "Failed cleanup CDCSDK streams metadata");
  }

  catalog_manager_->cdcsdk_manager_->RunBgTasks(epoch);
}

void CatalogManagerCdcBgTasks::Run() {
  while (!closing_.load()) {
    SCOPED_LEADER_SHARED_LOCK(l, catalog_manager_);
    if (!l.catalog_status().ok()) {
      LOG(WARNING) << "CDC cleanup background task thread going to sleep: "
                   << l.catalog_status().ToString();
    } else if (l.leader_status().ok()) {
      RunOnceAsLeader(l.epoch());
    }
    l.Unlock();
    TEST_catalog_manager_cdc_bg_task_run_counter.fetch_add(1);
    Wait(FLAGS_cdc_cleanup_bg_task_wait_ms);
  }
  VLOG(1) << "CDC cleanup background task thread shutting down";
}

}  // namespace yb::master
