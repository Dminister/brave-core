/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/unittest_utils.h"

#include <string>

#include "bat/ads/internal/ads_client_mock.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/client.h"

#include "base/files/file_util.h"
#include "testing/gmock/include/gmock/gmock.h"

using ::testing::_;
using ::testing::Invoke;

namespace ads {

base::FilePath GetTestDataPath() {
  return base::FilePath("brave/vendor/bat-native-ads/test/data");
}

void MockLoad(
    AdsClientMock* mock) {
  ON_CALL(*mock, Load(_, _))
      .WillByDefault(Invoke([](
          const std::string& name,
          LoadCallback callback) {
        base::FilePath path = GetTestDataPath();
        path = path.AppendASCII(name);

        std::string value;
        if (!base::ReadFileToString(path, &value)) {
          callback(FAILED, value);
          return;
        }

        callback(SUCCESS, value);
      }));
}

void MockSave(
    AdsClientMock* mock) {
  ON_CALL(*mock, Save(_, _, _))
      .WillByDefault(Invoke([](
          const std::string& name,
          const std::string& value,
          ResultCallback callback) {
        callback(SUCCESS);
      }));
}

}  // namespace ads
