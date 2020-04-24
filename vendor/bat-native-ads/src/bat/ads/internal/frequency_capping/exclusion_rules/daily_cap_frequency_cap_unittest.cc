/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/daily_cap_frequency_cap.h"

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

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

const std::vector<std::string> kCampaignIds = {
  "60267cee-d5bb-4a0d-baaf-91cd7f18e07e",
  "90762cee-d5bb-4a0d-baaf-61cd7f18e07e"
};

const uint64_t kSecondsPerDay = base::Time::kSecondsPerHour *
    base::Time::kHoursPerDay;

}  // namespace

class BatAdsDailyCapFrequencyCapTest : public ::testing::Test {
 protected:
  BatAdsDailyCapFrequencyCapTest()
      : ads_client_mock_(std::make_unique<NiceMock<AdsClientMock>>()),
        ads_(std::make_unique<AdsImpl>(ads_client_mock_.get())),
        client_(std::make_unique<Client>(ads_.get(), ads_client_mock_.get())),
        frequency_cap_(std::make_unique<DailyCapFrequencyCap>(client_.get())) {
    // You can do set-up work for each test here
  }

  ~BatAdsDailyCapFrequencyCapTest() override {
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
  std::unique_ptr<DailyCapFrequencyCap> frequency_cap_;
};

TEST_F(BatAdsDailyCapFrequencyCapTest,
    AdAllowedWhenNoAds) {
  // Arrange
  CreativeAdInfo ad;
  ad.campaign_id = kCampaignIds.at(0);
  ad.daily_cap = 2;

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsDailyCapFrequencyCapTest,
    AdAllowedWithAds) {
  // Arrange
  CreativeAdInfo ad;
  ad.campaign_id = kCampaignIds.at(0);
  ad.daily_cap = 2;

  GeneratePastCampaignHistoryFromNow(client_.get(), ad.campaign_id, 0, 1);

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsDailyCapFrequencyCapTest,
    AdAllowedWithAdsWithinTheDay) {
  // Arrange
  CreativeAdInfo ad;
  ad.campaign_id = kCampaignIds.at(0);
  ad.daily_cap = 2;

  // 23hrs 59m 59s ago
  GeneratePastCampaignHistoryFromNow(client_.get(), ad.campaign_id,
      kSecondsPerDay - 1, 1);

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsDailyCapFrequencyCapTest,
    AdAllowedWithAdsOverTheDay) {
  // Arrange
  CreativeAdInfo ad;
  ad.campaign_id = kCampaignIds.at(0);
  ad.daily_cap = 2;

  // 24hs ago
  GeneratePastCampaignHistoryFromNow(client_.get(), ad.campaign_id,
      kSecondsPerDay, 1);

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsDailyCapFrequencyCapTest,
    AdExcludedWithMatchingCampaignAds) {
  // Arrange
  CreativeAdInfo ad;
  ad.campaign_id = kCampaignIds.at(0);
  ad.daily_cap = 2;

  GeneratePastCampaignHistoryFromNow(client_.get(), ad.campaign_id, 0, 2);

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

TEST_F(BatAdsDailyCapFrequencyCapTest,
    AdNotExcludedWithNoMatchingCampaignAds) {
  // Arrange
  CreativeAdInfo ad;
  ad.campaign_id = kCampaignIds.at(1);
  ad.daily_cap = 1;

  GeneratePastCampaignHistoryFromNow(client_.get(), kCampaignIds.at(0), 0, 2);

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsDailyCapFrequencyCapTest,
    AdExcludedForIssue4207) {
  // Arrange
  const uint64_t ads_per_day = 20;
  const uint64_t ads_per_hour = 5;
  const uint64_t ad_interval = base::Time::kSecondsPerHour / ads_per_hour;

  CreativeAdInfo ad;
  ad.campaign_id = kCampaignIds.at(0);
  ad.daily_cap = ads_per_day;

  GeneratePastCampaignHistoryFromNow(client_.get(), ad.campaign_id, ad_interval,
      ads_per_day);

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

}  // namespace ads
