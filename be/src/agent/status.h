// This file is made available under Elastic License 2.0.
// This file is based on code available under the Apache license here:
//   https://github.com/apache/incubator-doris/blob/master/be/src/agent/status.h

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

#pragma once

namespace starrocks {

enum AgentStatus {
    STARROCKS_SUCCESS = 0,
    STARROCKS_ERROR = -1,
    STARROCKS_TASK_REQUEST_ERROR = -101,
    STARROCKS_FILE_DOWNLOAD_INVALID_PARAM = -201,
    STARROCKS_FILE_DOWNLOAD_INSTALL_OPT_FAILED = -202,
    STARROCKS_FILE_DOWNLOAD_CURL_INIT_FAILED = -203,
    STARROCKS_FILE_DOWNLOAD_FAILED = -204,
    STARROCKS_FILE_DOWNLOAD_GET_LENGTH_FAILED = -205,
    STARROCKS_FILE_DOWNLOAD_NOT_EXIST = -206,
    STARROCKS_FILE_DOWNLOAD_LIST_DIR_FAIL = -207,
    STARROCKS_CREATE_TABLE_EXIST = -301,
    STARROCKS_CREATE_TABLE_DIFF_SCHEMA_EXIST = -302,
    STARROCKS_CREATE_TABLE_NOT_EXIST = -303,
    STARROCKS_DROP_TABLE_NOT_EXIST = -401,
    STARROCKS_PUSH_INVALID_TABLE = -501,
    STARROCKS_PUSH_INVALID_VERSION = -502,
    STARROCKS_PUSH_TIME_OUT = -503,
    STARROCKS_PUSH_HAD_LOADED = -504,
    STARROCKS_TIMEOUT = -901,
    STARROCKS_INTERNAL_ERROR = -902,
    STARROCKS_DISK_REACH_CAPACITY_LIMIT = -903,
};
} // namespace starrocks
