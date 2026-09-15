// Copyright (c) YugabyteDB, Inc.
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
#include "yb/master/master-test_base.h"

#include "yb/util/backoff_waiter.h"
#include "yb/util/flags.h"

DECLARE_int32(cdc_cleanup_bg_task_wait_ms);
DECLARE_int32(cdc_state_table_num_tablets);

namespace yb::master {

TEST(CatalogManagerCdcBgTasksTest, DefaultCleanupIntervalIs60Seconds) {
  const auto default_wait_ms = std::stoll(
      gflags::GetCommandLineFlagInfoOrDie("cdc_cleanup_bg_task_wait_ms").default_value);
  ASSERT_EQ(default_wait_ms, 60 * 1000);
}

class MasterTestCdcCleanupBgTask : public MasterTestBase {
 protected:
  void SetUp() override {
    ANNOTATE_UNPROTECTED_WRITE(FLAGS_cdc_cleanup_bg_task_wait_ms) = 50;
    MasterTestBase::SetUp();
    ANNOTATE_UNPROTECTED_WRITE(FLAGS_cdc_state_table_num_tablets) = 1;
  }
};

TEST_F(MasterTestCdcCleanupBgTask, CdcCleanupBgTaskRunsOnMasterLeader) {
  ASSERT_OK(WaitFor(
      []() -> Result<bool> {
        return TEST_catalog_manager_cdc_bg_task_run_count() >= 2;
      },
      MonoDelta::FromSeconds(10), "Timed out waiting for CDC cleanup background task iterations"));
}

}  // namespace yb::master
