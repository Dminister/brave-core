/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_LOCALE_HELPER_WIN_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_LOCALE_HELPER_WIN_H_

#include <windows.h>
#include <string>

#include "brave/components/brave_ads/browser/locale_helper.h"

namespace brave_ads {

class LocaleHelperWin : public LocaleHelper {
 public:
  LocaleHelperWin(const LocaleHelperWin&) = delete;
  LocaleHelperWin& operator=(const LocaleHelperWin&) = delete;

  static LocaleHelperWin* GetInstanceImpl();

 private:
  friend struct base::DefaultSingletonTraits<LocaleHelperWin>;

  LocaleHelperWin();
  ~LocaleHelperWin() override;

  // LocaleHelper impl
  std::string GetLocale() const override;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_LOCALE_HELPER_WIN_H_
