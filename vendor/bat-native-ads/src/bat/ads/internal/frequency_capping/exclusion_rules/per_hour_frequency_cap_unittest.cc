/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/per_hour_frequency_cap.h"

#include <memory>
#include <string>

#include "bat/ads/creative_ad_info.h"
#include "bat/ads/internal/ads_client_mock.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/client.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_unittest_utils.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_utils.h"
#include "bat/ads/internal/unittest_utils.h"

#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

using ::testing::NiceMock;

namespace ads {

namespace {

const char kCreativeInstanceId[] = "9aea9a47-c6a0-4718-a0fa-706338bb2156";

}  // namespace

class BatAdsPerHourFrequencyCapTest : public ::testing::Test {
 protected:
  BatAdsPerHourFrequencyCapTest()
      : ads_client_mock_(std::make_unique<NiceMock<AdsClientMock>>()),
        ads_(std::make_unique<AdsImpl>(ads_client_mock_.get())),
        client_(std::make_unique<Client>(ads_.get(), ads_client_mock_.get())),
        frequency_cap_(std::make_unique<PerHourFrequencyCap>(client_.get())) {
    // You can do set-up work for each test here
  }

  ~BatAdsPerHourFrequencyCapTest() override {
    // You can do clean-up work that doesn't throw exceptions here
  }

  // If the constructor and destructor are not enough for setting up and
  // cleaning up each test, you can use the following methods

  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test)

    Initialize(ads_.get());
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor)
  }

  std::unique_ptr<AdsClientMock> ads_client_mock_;
  std::unique_ptr<AdsImpl> ads_;
  std::unique_ptr<Client> client_;
  std::unique_ptr<PerHourFrequencyCap> frequency_cap_;
};

TEST_F(BatAdsPerHourFrequencyCapTest,
    AdAllowedWhenNoAds) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_instance_id = kCreativeInstanceId;

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsPerHourFrequencyCapTest,
    AdAllowedOverTheHour) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_instance_id = kCreativeInstanceId;

  GeneratePastAdHistoryFromNow(client_.get(), kCreativeInstanceId,
      base::Time::kSecondsPerHour, 1);

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsPerHourFrequencyCapTest,
    AdExcludedWithPastAdsJustWithinTheHour) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_instance_id = kCreativeInstanceId;

  GeneratePastAdHistoryFromNow(client_.get(), kCreativeInstanceId,
      base::Time::kSecondsPerHour - 1, 1);

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

TEST_F(BatAdsPerHourFrequencyCapTest,
    AdExcludedWithPastAdWithinTheHour) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_instance_id = kCreativeInstanceId;

  GeneratePastAdHistoryFromNow(client_.get(), kCreativeInstanceId, 0, 1);

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

}  // namespace ads
