/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BINANCE_BROWSER_BINANCE_SERVICE_H_
#define BRAVE_COMPONENTS_BINANCE_BROWSER_BINANCE_SERVICE_H_

#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/containers/queue.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observer.h"
#include "components/keyed_service/core/keyed_service.h"
#include "url/gurl.h"

namespace base {
class FilePath;
class SequencedTaskRunner;
}  // namespace base

namespace content {
class BrowserContext;
}  // namespace content

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

class Profile;

const char oauth_path_access_token[] = "/oauth/token";
const char oauth_path_account_balances[] = "/oauth-api/v1/balance";
const char oauth_path_convert_assets[] = "/oauth-api/v1/ocbs/support-coins";
const char oauth_path_convert_quote[] = "/oauth-api/v1/ocbs/quote";
const char oauth_path_convert_confirm[] = "/oauth-api/v1/ocbs/confirm";
const char oauth_path_deposit_info[] = "/oauth-api/v1/get-charge-address";
const char oauth_path_revoke_token[] = "/oauth-api/v1/revoke-token";

const char api_path_ticker_price[] = "/api/v3/ticker/price";
const char api_path_ticker_volume[] = "/api/v3/ticker/24hr";

class BinanceService : public KeyedService {
 public:
  explicit BinanceService(content::BrowserContext* context);
  ~BinanceService() override;

  using GetAccessTokenCallback = base::OnceCallback<void(bool)>;
  using GetConvertQuoteCallback = base::OnceCallback<void(const std::string&,
                                                          const std::string&,
                                                          const std::string&,
                                                          const std::string&)>;
  using GetAccountBalancesCallback = base::OnceCallback<
      void(const std::map<std::string, std::string>&, bool success)>;
  using GetDepositInfoCallback = base::OnceCallback<void(const std::string&,
                                                         const std::string&,
                                                         bool success)>;
  using ConfirmConvertCallback = base::OnceCallback<void(bool,
                                                         const std::string&)>;
  using GetConvertAssetsCallback = base::OnceCallback<
      void(const std::map<std::string, std::vector<std::string>>&)>;
  using GetTickerPriceCallback = base::OnceCallback<void(const std::string&)>;
  using GetTickerVolumeCallback = base::OnceCallback<void(const std::string&)>;
  using RevokeTokenCallback = base::OnceCallback<void(bool)>;

  bool GetAccessToken(const std::string& code,
      GetAccessTokenCallback callback);
  bool GetConvertQuote(const std::string& from,
      const std::string& to,
      const std::string& amount,
      GetConvertQuoteCallback callback);
  bool GetAccountBalances(GetAccountBalancesCallback callback);
  bool GetDepositInfo(const std::string& symbol,
      GetDepositInfoCallback callback);
  bool ConfirmConvert(const std::string& quote_id,
      ConfirmConvertCallback callback);
  bool GetConvertAssets(GetConvertAssetsCallback callback);
  bool GetTickerPrice(const std::string& symbol_pair,
      GetTickerPriceCallback callback);
  bool GetTickerVolume(const std::string& symbol_pair,
      GetTickerVolumeCallback callback);
  bool RevokeToken(RevokeTokenCallback callback);

  std::string GetBinanceTLD();
  std::string GetOAuthClientUrl();
  static std::string GetCodeChallenge(const std::string& code_verifier);

 private:
  static GURL oauth_endpoint_;
  static GURL api_endpoint_;
  using SimpleURLLoaderList =
      std::list<std::unique_ptr<network::SimpleURLLoader>>;
  bool SetAccessTokens(const std::string& access_token,
                       const std::string& refresh_token);

  using URLRequestCallback =
      base::OnceCallback<void(const int, const std::string&,
                              const std::map<std::string, std::string>&)>;

  base::SequencedTaskRunner* io_task_runner();
  void OnGetAccessToken(GetAccessTokenCallback callback,
                           const int status, const std::string& body,
                           const std::map<std::string, std::string>& headers);
  void OnGetConvertQuote(GetConvertQuoteCallback callback,
                           const int status, const std::string& body,
                           const std::map<std::string, std::string>& headers);
  void OnGetAccountBalances(GetAccountBalancesCallback callback,
                           const int status, const std::string& body,
                           const std::map<std::string, std::string>& headers);
  void OnGetTickerPrice(GetTickerPriceCallback callback,
                        const int status, const std::string& body,
                        const std::map<std::string, std::string>& headers);
  void OnGetTickerVolume(GetTickerVolumeCallback callback,
                        const int status, const std::string& body,
                        const std::map<std::string, std::string>& headers);
  void OnGetDepositInfo(GetDepositInfoCallback callback,
                        const int status, const std::string& body,
                        const std::map<std::string, std::string>& headers);
  void OnConfirmConvert(ConfirmConvertCallback callback,
                        const int status, const std::string& body,
                        const std::map<std::string, std::string>& headers);
  void OnGetConvertAssets(GetConvertAssetsCallback callback,
                          const int status, const std::string& body,
                          const std::map<std::string, std::string>& headers);
  void OnRevokeToken(RevokeTokenCallback callback,
                     const int status, const std::string& body,
                     const std::map<std::string, std::string>& headers);
  bool OAuthRequest(const GURL& url, const std::string& method,
      const std::string& post_data, URLRequestCallback callback);
  bool LoadTokensFromPrefs();
  void OnURLLoaderComplete(
      SimpleURLLoaderList::iterator iter,
      URLRequestCallback callback,
      const std::unique_ptr<std::string> response_body);
  void SetClientIdForTest(const std::string& client_id);
  void SetOAuthHostForTest(const std::string& oauth_host);
  void SetAPIHostForTest(const std::string& api_host);

  scoped_refptr<base::SequencedTaskRunner> io_task_runner_;
  std::string access_token_;
  std::string refresh_token_;
  std::string code_challenge_;
  std::string code_verifier_;
  std::string client_id_;
  std::string oauth_host_;
  std::string api_host_;

  content::BrowserContext* context_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  SimpleURLLoaderList url_loaders_;
  base::WeakPtrFactory<BinanceService> weak_factory_;

  FRIEND_TEST_ALL_PREFIXES(BinanceAPIBrowserTest, GetOAuthClientURL);
  friend class BinanceAPIBrowserTest;

  DISALLOW_COPY_AND_ASSIGN(BinanceService);
};

#endif  // BRAVE_COMPONENTS_BINANCE_BROWSER_BINANCE_SERVICE_H_
