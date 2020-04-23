/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_rewards/checkout_dialog_message_handler.h"

#include "brave/browser/brave_rewards/checkout_dialog.h"
#include "brave/components/brave_rewards/browser/rewards_service_factory.h"
#include "chrome/browser/profiles/profile.h"

namespace brave_rewards {

using base::Bind;
using base::BindOnce;
using base::BindRepeating;

CheckoutDialogMessageHandler::CheckoutDialogMessageHandler(
    CheckoutDialogController* controller)
    : controller_(controller),
      weak_factory_(this) {
  DCHECK(controller_);
  controller_->AddObserver(this);
}

CheckoutDialogMessageHandler::~CheckoutDialogMessageHandler() {
  controller_->RemoveObserver(this);
  if (rewards_service_) {
    rewards_service_->RemoveObserver(this);
  }
}

RewardsService* CheckoutDialogMessageHandler::GetRewardsService() {
  // TODO(zenparsing): Consider whether we can do this in constructor
  // instead.
  if (!rewards_service_) {
    Profile* profile = Profile::FromWebUI(web_ui());
    rewards_service_ = RewardsServiceFactory::GetForProfile(profile);
    if (rewards_service_) {
      rewards_service_->AddObserver(this);
    }
  }
  return rewards_service_;
}

void CheckoutDialogMessageHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback("getWalletBalance", BindRepeating(
      &CheckoutDialogMessageHandler::OnGetWalletBalance,
      base::Unretained(this)));

  web_ui()->RegisterMessageCallback("getAnonWalletStatus", BindRepeating(
      &CheckoutDialogMessageHandler::OnGetAnonWalletStatus,
      base::Unretained(this)));

  web_ui()->RegisterMessageCallback("getExternalWallet", BindRepeating(
      &CheckoutDialogMessageHandler::OnGetExternalWallet,
      base::Unretained(this)));

  web_ui()->RegisterMessageCallback("getRewardsEnabled", BindRepeating(
      &CheckoutDialogMessageHandler::OnGetRewardsEnabled,
      base::Unretained(this)));

  web_ui()->RegisterMessageCallback("enableRewards", BindRepeating(
      &CheckoutDialogMessageHandler::OnEnableRewards,
      base::Unretained(this)));

  web_ui()->RegisterMessageCallback("createWallet", BindRepeating(
      &CheckoutDialogMessageHandler::OnCreateWallet,
      base::Unretained(this)));

  web_ui()->RegisterMessageCallback("cancelPayment", BindRepeating(
      &CheckoutDialogMessageHandler::OnCancelPayment,
      base::Unretained(this)));

  web_ui()->RegisterMessageCallback("getOrderInfo", BindRepeating(
      &CheckoutDialogMessageHandler::OnGetOrderInfo,
      base::Unretained(this)));
}

void CheckoutDialogMessageHandler::OnWalletInitialized(
    RewardsService* service,
    int32_t result) {
  if (IsJavascriptAllowed()) {
    base::Value response(base::Value::Type::DICTIONARY);
    response.SetIntKey("status", result);
    FireWebUIListener("walletInitialized", std::move(response));
  }
}

void CheckoutDialogMessageHandler::OnRewardsMainEnabled(
    RewardsService* service,
    bool enabled) {
  GetRewardsMainEnabledCallback(enabled);
}

void CheckoutDialogMessageHandler::OnPaymentAborted() {
  // TODO(zenparsing): Perform same functionality as OnCancelPayment
}

void CheckoutDialogMessageHandler::OnPaymentCompleted() {
  // TODO(zenparsing): Update flow state to complete
}

void CheckoutDialogMessageHandler::OnGetWalletBalance(
    const base::ListValue* args) {
  if (auto* service = GetRewardsService()) {
    AllowJavascript();
    service->FetchBalance(BindOnce(
        &CheckoutDialogMessageHandler::FetchBalanceCallback,
        weak_factory_.GetWeakPtr()));
  }
}

void CheckoutDialogMessageHandler::OnGetAnonWalletStatus(
    const base::ListValue* args) {
  if (auto* service = GetRewardsService()) {
    AllowJavascript();
    service->GetAnonWalletStatus(BindOnce(
        &CheckoutDialogMessageHandler::GetAnonWalletStatusCallback,
        weak_factory_.GetWeakPtr()));
  }
}

void CheckoutDialogMessageHandler::OnGetExternalWallet(
    const base::ListValue* args) {
  if (auto* service = GetRewardsService()) {
    AllowJavascript();
    service->GetExternalWallet("uphold", BindOnce(
        &CheckoutDialogMessageHandler::GetExternalWalletCallback,
        weak_factory_.GetWeakPtr()));
  }
}

void CheckoutDialogMessageHandler::OnGetRewardsEnabled(
    const base::ListValue* args) {
  if (auto* service = GetRewardsService()) {
    AllowJavascript();
    service->GetRewardsMainEnabled(Bind(
        &CheckoutDialogMessageHandler::GetRewardsMainEnabledCallback,
        weak_factory_.GetWeakPtr()));
  }
}

void CheckoutDialogMessageHandler::OnEnableRewards(
    const base::ListValue* args) {
  if (auto* service = GetRewardsService()) {
    AllowJavascript();
    service->SetRewardsMainEnabled(1);
  }
}

void CheckoutDialogMessageHandler::OnCreateWallet(
    const base::ListValue* args) {
  if (auto* service = GetRewardsService()) {
    AllowJavascript();
    service->CreateWallet(Bind(
        &CheckoutDialogMessageHandler::CreateWalletCallback,
        weak_factory_.GetWeakPtr()));
  }
}

void CheckoutDialogMessageHandler::OnCancelPayment(
    const base::ListValue* args) {
  // TODO(zenparsing): Cancel the potential order via the
  // rewards service, but do not wait to close the dialog.
  AllowJavascript();
  FireWebUIListener("orderCanceled");
}

void CheckoutDialogMessageHandler::OnGetOrderInfo(
    const base::ListValue* args) {
  AllowJavascript();
  base::Value response(base::Value::Type::DICTIONARY);
  response.SetDoubleKey("total", controller_->params().total);
  response.SetStringKey("description", controller_->params().description);
  FireWebUIListener("orderInfoUpdated", response);
}

void CheckoutDialogMessageHandler::FetchBalanceCallback(
    int32_t status,
    std::unique_ptr<brave_rewards::Balance> balance) {
  if (!IsJavascriptAllowed())
    return;

  if (status == 0 && balance) {
    base::Value response(base::Value::Type::DICTIONARY);
    response.SetDoubleKey("total", balance->total);

    base::Value rates_dict(base::Value::Type::DICTIONARY);
    for (auto& pair : balance->rates) {
      rates_dict.SetDoubleKey(pair.first, pair.second);
    }
    response.SetKey("rates", std::move(rates_dict));

    FireWebUIListener("walletBalanceUpdated", response);
  }

  // TODO(zenparsing): Fire error event if status != 0
}

void CheckoutDialogMessageHandler::GetAnonWalletStatusCallback(
    uint32_t status) {
  if (!IsJavascriptAllowed())
    return;

  std::string status_text;
  switch (status) {
    case 12: status_text = "created"; break;
    case 17: status_text = "corrupted"; break;
    default: status_text = "not-created"; break;
  }

  base::Value response(base::Value::Type::DICTIONARY);
  response.SetStringKey("status", status_text);
  FireWebUIListener("anonWalletStatusUpdated", response);
}

void CheckoutDialogMessageHandler::GetExternalWalletCallback(
    int32_t status,
    std::unique_ptr<brave_rewards::ExternalWallet> wallet) {
  if (!IsJavascriptAllowed())
    return;

  if (status == 0 && wallet) {
    bool verified = wallet->status == 1; // ledger::WalletStatus::VERIFIED

    base::Value response(base::Value::Type::DICTIONARY);
    response.SetBoolKey("verified", verified);

    FireWebUIListener("externalWalletUpdated", response);
  }

  // TODO(zenparsing): Fire error event if status != 0
}

void CheckoutDialogMessageHandler::GetRewardsMainEnabledCallback(
    bool enabled) {
  if (!IsJavascriptAllowed())
    return;

  base::Value response(base::Value::Type::DICTIONARY);
  response.SetBoolKey("rewardsEnabled", enabled);

  FireWebUIListener("rewardsEnabledUpdated", response);
}

void CheckoutDialogMessageHandler::CreateWalletCallback(int32_t result) {
  // JS will be informed of the wallet creation result via
  // the WalletInitialized callback.
}

}  // namespace brave_rewards
