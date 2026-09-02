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
//
package org.yb.client;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.yb.YBTestRunner;
import org.yb.cdc.CdcService;

import static org.yb.AssertionWrappers.*;

@RunWith(value=YBTestRunner.class)
public class TestTabletClient {
  @Test
  public void testFatalCDCErrorsFailFast() {
    assertEquals(
        TabletClient.CDCErrorHandling.FAIL,
        TabletClient.getCDCErrorHandling(CdcService.CDCErrorPB.Code.INVALID_REQUEST));
    assertEquals(
        TabletClient.CDCErrorHandling.FAIL,
        TabletClient.getCDCErrorHandling(CdcService.CDCErrorPB.Code.TABLE_NOT_FOUND));
    assertEquals(
        TabletClient.CDCErrorHandling.FAIL,
        TabletClient.getCDCErrorHandling(CdcService.CDCErrorPB.Code.CHECKPOINT_TOO_OLD));
    assertEquals(
        TabletClient.CDCErrorHandling.FAIL,
        TabletClient.getCDCErrorHandling(CdcService.CDCErrorPB.Code.TABLET_SPLIT));
    assertEquals(
        TabletClient.CDCErrorHandling.FAIL,
        TabletClient.getCDCErrorHandling(CdcService.CDCErrorPB.Code.OPERATION_DISALLOWED));
    assertEquals(
        TabletClient.CDCErrorHandling.FAIL,
        TabletClient.getCDCErrorHandling(
            CdcService.CDCErrorPB.Code.AUTO_FLAGS_CONFIG_VERSION_MISMATCH));
  }

  @Test
  public void testTransientCDCErrorsRetryAsNotLeader() {
    assertEquals(
        TabletClient.CDCErrorHandling.RETRY_NOT_LEADER,
        TabletClient.getCDCErrorHandling(CdcService.CDCErrorPB.Code.TABLET_NOT_RUNNING));
    assertEquals(
        TabletClient.CDCErrorHandling.RETRY_NOT_LEADER,
        TabletClient.getCDCErrorHandling(CdcService.CDCErrorPB.Code.LEADER_NOT_READY));
    assertEquals(
        TabletClient.CDCErrorHandling.RETRY_NOT_LEADER,
        TabletClient.getCDCErrorHandling(CdcService.CDCErrorPB.Code.NOT_LEADER));
    assertEquals(
        TabletClient.CDCErrorHandling.RETRY_NOT_LEADER,
        TabletClient.getCDCErrorHandling(CdcService.CDCErrorPB.Code.NOT_RUNNING));
  }

  @Test
  public void testTabletNotFoundInvalidatesTabletCacheAndRetries() {
    assertEquals(
        TabletClient.CDCErrorHandling.RETRY_TABLET_NOT_FOUND,
        TabletClient.getCDCErrorHandling(CdcService.CDCErrorPB.Code.TABLET_NOT_FOUND));
  }

  @Test
  public void testUnknownCDCErrorFailsFast() {
    assertEquals(
        TabletClient.CDCErrorHandling.FAIL,
        TabletClient.getCDCErrorHandling(CdcService.CDCErrorPB.Code.UNKNOWN_ERROR));
  }
}
