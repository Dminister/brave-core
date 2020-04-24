/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_conversions.h"

#include <stdint.h>

#include <deque>
#include <map>
#include <memory>
#include <string>

#include "bat/ads/internal/ads_client_mock.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/client.h"
#include "bat/ads/internal/unittest_utils.h"

#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=Confirmations*

using ::testing::_;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;

namespace ads {

class BraveAdsAdConversionsTest : public ::testing::Test {
 protected:
  BraveAdsAdConversionsTest()
      : ads_client_mock_(std::make_unique<NiceMock<AdsClientMock>>()),
        ads_(std::make_unique<AdsImpl>(ads_client_mock_.get())),
        client_(std::make_unique<Client>(ads_.get(), ads_client_mock_.get())),
        ad_conversions_(std::make_unique<AdConversions>(ads_.get(),
            ads_client_mock_.get(), client_.get())) {
    // You can do set-up work for each test here
  }

  ~BraveAdsAdConversionsTest() override {
    // You can do clean-up work that doesn't throw exceptions here
  }

  // If the constructor and destructor are not enough for setting up and
  // cleaning up each test, you can use the following methods

  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test)

    MockLoad(ads_client_mock_.get());
    MockSave(ads_client_mock_.get());

    Initialize(client_.get());
    Initialize(ad_conversions_.get());
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor)
  }

  // Objects declared here can be used by all tests in the test case

  std::deque<uint64_t> GetAdConversionHistoryForCreativeSet(
      const std::string& creative_set_id) {
    std::deque<uint64_t> creative_set_history;

    const std::map<std::string, std::deque<uint64_t>> history =
        client_->GetAdConversionHistory();

    const auto iter = history.find(creative_set_id);
    if (iter == history.end()) {
      return creative_set_history;
    }

    creative_set_history = iter->second;
    return creative_set_history;
  }

  void AdEvent(
      const std::string& creative_set_id,
      const ConfirmationType confirmation_type,
      const base::Time time = base::Time::Now()) {
    AdHistory history;

    history.ad_content.creative_instance_id =
        "7a3b6d9f-d0b7-4da6-8988-8d5b8938c94f";
    history.ad_content.creative_set_id = creative_set_id;
    history.ad_content.ad_action = confirmation_type;
    history.timestamp_in_seconds = time.ToDoubleT();

    client_->AppendAdHistoryToAdsHistory(history);
  }

  base::test::TaskEnvironment task_environment;

  std::unique_ptr<AdsClientMock> ads_client_mock_;
  std::unique_ptr<AdsImpl> ads_;
  std::unique_ptr<Client> client_;

  std::unique_ptr<AdConversions> ad_conversions_;
};

TEST_F(BraveAdsAdConversionsTest,
    ShouldNotAllowAdConversionTracking) {
  // Arrange
  const std::string creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";

  ON_CALL(*ads_client_mock_, ShouldAllowAdConversionTracking())
      .WillByDefault(Return(false));

  EXPECT_CALL(*ads_client_mock_, GetAdConversions(_))
      .Times(0);

  // Act
  ad_conversions_->Check("https://www.brave.com/signup");

  // Assert
  const std::deque<uint64_t> creative_set_history =
      GetAdConversionHistoryForCreativeSet(creative_set_id);

  EXPECT_EQ(0UL, creative_set_history.size());
}

TEST_F(BraveAdsAdConversionsTest,
    ConvertViewedAd) {
  // Arrange
  const std::string creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";

  ON_CALL(*ads_client_mock_, ShouldAllowAdConversionTracking())
      .WillByDefault(Return(true));

  ON_CALL(*ads_client_mock_, GetAdConversions(_))
      .WillByDefault(Invoke([&creative_set_id](
          GetAdConversionsCallback callback) {
        AdConversionList ad_conversions;

        AdConversionInfo ad_conversion;
        ad_conversion.creative_set_id = creative_set_id;
        ad_conversion.type = "postview";
        ad_conversion.url_pattern = "https://www.brave.com/*";
        ad_conversion.observation_window = 3;

        ad_conversions.push_back(ad_conversion);

        callback(Result::SUCCESS, ad_conversions);
      }));

  EXPECT_CALL(*ads_client_mock_, GetAdConversions(_))
      .Times(1);

  AdEvent(creative_set_id, ConfirmationType::kViewed);

  // Act
  ad_conversions_->Check("https://www.brave.com/signup");

  // Assert
  const std::deque<uint64_t> creative_set_history =
      GetAdConversionHistoryForCreativeSet(creative_set_id);

  EXPECT_EQ(1UL, creative_set_history.size());
}

TEST_F(BraveAdsAdConversionsTest,
    ConvertClickedAd) {
  // Arrange
  const std::string creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";

  ON_CALL(*ads_client_mock_, ShouldAllowAdConversionTracking())
      .WillByDefault(Return(true));

  ON_CALL(*ads_client_mock_, GetAdConversions(_))
      .WillByDefault(Invoke([&creative_set_id](
          GetAdConversionsCallback callback) {
        AdConversionList ad_conversions;

        AdConversionInfo ad_conversion;
        ad_conversion.creative_set_id = creative_set_id;
        ad_conversion.type = "postclick";
        ad_conversion.url_pattern = "https://www.brave.com/*";
        ad_conversion.observation_window = 3;

        ad_conversions.push_back(ad_conversion);

        callback(Result::SUCCESS, ad_conversions);
      }));

  EXPECT_CALL(*ads_client_mock_, GetAdConversions(_))
      .Times(1);

  AdEvent(creative_set_id, ConfirmationType::kClicked);

  // Act
  ad_conversions_->Check("https://www.brave.com/signup");

  // Assert
  const std::deque<uint64_t> creative_set_history =
      GetAdConversionHistoryForCreativeSet(creative_set_id);

  EXPECT_EQ(1UL, creative_set_history.size());
}

TEST_F(BraveAdsAdConversionsTest,
    DoNotConvertNonViewedOrClickedAds) {
  // Arrange
  const std::string creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";

  ON_CALL(*ads_client_mock_, ShouldAllowAdConversionTracking())
      .WillByDefault(Return(true));

  ON_CALL(*ads_client_mock_, GetAdConversions(_))
      .WillByDefault(Invoke([&creative_set_id](
          GetAdConversionsCallback callback) {
        AdConversionList ad_conversions;

        AdConversionInfo ad_conversion;
        ad_conversion.creative_set_id = creative_set_id;
        ad_conversion.type = "postclick";
        ad_conversion.url_pattern = "https://www.brave.com/*";
        ad_conversion.observation_window = 3;

        ad_conversions.push_back(ad_conversion);

        callback(Result::SUCCESS, ad_conversions);
      }));

  EXPECT_CALL(*ads_client_mock_, GetAdConversions(_))
      .Times(1);

  AdEvent(creative_set_id, ConfirmationType::kNone);
  AdEvent(creative_set_id, ConfirmationType::kDismissed);
  AdEvent(creative_set_id, ConfirmationType::kLanded);
  AdEvent(creative_set_id, ConfirmationType::kFlagged);
  AdEvent(creative_set_id, ConfirmationType::kUpvoted);
  AdEvent(creative_set_id, ConfirmationType::kDownvoted);
  AdEvent(creative_set_id, ConfirmationType::kConversion);

  // Act
  ad_conversions_->Check("https://www.brave.com/signup");

  // Assert
  const std::deque<uint64_t> creative_set_history =
      GetAdConversionHistoryForCreativeSet(creative_set_id);

  EXPECT_EQ(0UL, creative_set_history.size());
}

TEST_F(BraveAdsAdConversionsTest,
    DoNotConvertAdWhenThereIsAdConversionHistoryForTheSameCreativeSet) {
  // Arrange
  const std::string creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";

  ON_CALL(*ads_client_mock_, ShouldAllowAdConversionTracking())
      .WillByDefault(Return(true));

  ON_CALL(*ads_client_mock_, GetAdConversions(_))
      .WillByDefault(Invoke([&creative_set_id](
          GetAdConversionsCallback callback) {
        AdConversionList ad_conversions;

        AdConversionInfo ad_conversion;
        ad_conversion.creative_set_id = creative_set_id;
        ad_conversion.type = "postview";
        ad_conversion.url_pattern = "https://www.brave.com/*";
        ad_conversion.observation_window = 3;

        ad_conversions.push_back(ad_conversion);

        callback(Result::SUCCESS, ad_conversions);
      }));

  EXPECT_CALL(*ads_client_mock_, GetAdConversions(_))
      .Times(2);

  AdEvent(creative_set_id, ConfirmationType::kViewed);

  ad_conversions_->Check("https://www.brave.com/signup");

  // Act
  ad_conversions_->Check("https://www.brave.com/signup");

  // Assert
  const std::deque<uint64_t> creative_set_history =
      GetAdConversionHistoryForCreativeSet(creative_set_id);

  EXPECT_EQ(1UL, creative_set_history.size());
}

TEST_F(BraveAdsAdConversionsTest,
    DoNotConvertAdWhenUrlDoesNotMatchAdConversionPattern) {
  // Arrange
  const std::string creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";

  ON_CALL(*ads_client_mock_, ShouldAllowAdConversionTracking())
      .WillByDefault(Return(true));

  ON_CALL(*ads_client_mock_, GetAdConversions(_))
      .WillByDefault(Invoke([&creative_set_id](
          GetAdConversionsCallback callback) {
        AdConversionList ad_conversions;

        AdConversionInfo ad_conversion;
        ad_conversion.creative_set_id = creative_set_id;
        ad_conversion.type = "postview";
        ad_conversion.url_pattern = "https://www.brave.com/signup/*";
        ad_conversion.observation_window = 3;

        ad_conversions.push_back(ad_conversion);

        callback(Result::SUCCESS, ad_conversions);
      }));

  EXPECT_CALL(*ads_client_mock_, GetAdConversions(_))
      .Times(1);

  AdEvent(creative_set_id, ConfirmationType::kViewed);

  // Act
  ad_conversions_->Check("https://www.brave.com/welcome");

  // Assert
  const std::deque<uint64_t> creative_set_history =
      GetAdConversionHistoryForCreativeSet(creative_set_id);

  EXPECT_EQ(0UL, creative_set_history.size());
}

TEST_F(BraveAdsAdConversionsTest,
    ConvertAdWhenTheAdConversionObservationWindowHasNotExpired) {
  // Arrange
  const std::string creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";

  ON_CALL(*ads_client_mock_, ShouldAllowAdConversionTracking())
      .WillByDefault(Return(true));

  ON_CALL(*ads_client_mock_, GetAdConversions(_))
      .WillByDefault(Invoke([&creative_set_id](
          GetAdConversionsCallback callback) {
        AdConversionList ad_conversions;

        AdConversionInfo ad_conversion;
        ad_conversion.creative_set_id = creative_set_id;
        ad_conversion.type = "postview";
        ad_conversion.url_pattern = "https://www.brave.com/*";
        ad_conversion.observation_window = 3;

        ad_conversions.push_back(ad_conversion);

        callback(Result::SUCCESS, ad_conversions);
      }));

  EXPECT_CALL(*ads_client_mock_, GetAdConversions(_))
      .Times(1);

  const base::Time time = base::Time::Now() - base::TimeDelta::FromHours(71);
  AdEvent(creative_set_id, ConfirmationType::kViewed, time);

  // Act
  ad_conversions_->Check("https://www.brave.com/");

  // Assert
  const std::deque<uint64_t> creative_set_history =
      GetAdConversionHistoryForCreativeSet(creative_set_id);

  EXPECT_EQ(1UL, creative_set_history.size());
}

TEST_F(BraveAdsAdConversionsTest,
    DoNotConvertAdWhenTheAdConversionObservationWindowHasExpired) {
  // Arrange
  const std::string creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";

  ON_CALL(*ads_client_mock_, ShouldAllowAdConversionTracking())
      .WillByDefault(Return(true));

  ON_CALL(*ads_client_mock_, GetAdConversions(_))
      .WillByDefault(Invoke([&creative_set_id](
          GetAdConversionsCallback callback) {
        AdConversionList ad_conversions;

        AdConversionInfo ad_conversion;
        ad_conversion.creative_set_id = creative_set_id;
        ad_conversion.type = "postview";
        ad_conversion.url_pattern = "https://www.brave.com/*";
        ad_conversion.observation_window = 3;

        ad_conversions.push_back(ad_conversion);

        callback(Result::SUCCESS, ad_conversions);
      }));

  EXPECT_CALL(*ads_client_mock_, GetAdConversions(_))
      .Times(1);

  const base::Time time = base::Time::Now() - base::TimeDelta::FromHours(73);
  AdEvent(creative_set_id, ConfirmationType::kViewed, time);

  // Act
  ad_conversions_->Check("https://www.foobar.com/");

  // Assert
  const std::deque<uint64_t> creative_set_history =
      GetAdConversionHistoryForCreativeSet(creative_set_id);

  EXPECT_EQ(0UL, creative_set_history.size());
}

}  // namespace ads
