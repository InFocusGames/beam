// Copyright 2018 The Beam Team
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "wallet/client/wallet_client.h"

class WalletModel
    : public beam::wallet::WalletClient
{
public:

    using Ptr = std::shared_ptr<WalletModel>;

    WalletModel(beam::wallet::IWalletDB::Ptr walletDB, const std::string& nodeAddr, beam::io::Reactor::Ptr reactor);
    ~WalletModel() override;

    void callMyFunction();
    std::function<void()> myFunction;

private:
    void doFunction(const std::function<void()>& func);

    void onStatus(const beam::wallet::WalletStatus& status) override;
    void onTxStatus(beam::wallet::ChangeAction, const std::vector<beam::wallet::TxDescription>& items) override;
    void onSyncProgressUpdated(int done, int total) override;
    void onChangeCalculated(beam::Amount changeAsset, beam::Amount changeBeam, beam::Asset::ID assetId) override;
    void onShieldedCoinsSelectionCalculated(const beam::wallet::ShieldedCoinsSelectionInfo& selectionRes) override;
    void onNeedExtractShieldedCoins(bool val) override;
    void onAllUtxoChanged(beam::wallet::ChangeAction, const std::vector<beam::wallet::Coin>& utxos) override;
    void onAddressesChanged(beam::wallet::ChangeAction, const std::vector<beam::wallet::WalletAddress>& addresses) override;
    void onAddresses(bool own, const std::vector<beam::wallet::WalletAddress>& addrs) override;
#ifdef BEAM_ATOMIC_SWAP_SUPPORT
    void onSwapOffersChanged(beam::wallet::ChangeAction action, const std::vector<beam::wallet::SwapOffer>& offers) override;
#endif  // BEAM_ATOMIC_SWAP_SUPPORT
    void onGeneratedNewAddress(const beam::wallet::WalletAddress& walletAddr) override;
    void onSwapParamsLoaded(const beam::ByteBuffer& params) override;
    void onNewAddressFailed() override;
    void onNodeConnectionChanged(bool isNodeConnected) override;
    void onWalletError(beam::wallet::ErrorType error) override;
    void FailedToStartWallet() override;
    void onSendMoneyVerified() override;
    void onCantSendToExpired() override;
    void onPaymentProofExported(const beam::wallet::TxID& txID, const beam::ByteBuffer& proof) override;
    void onCoinsByTx(const std::vector<beam::wallet::Coin>& coins) override;
    void onAddressChecked(const std::string& addr, bool isValid) override;
    void onImportRecoveryProgress(uint64_t done, uint64_t total) override;
    void onNoDeviceConnected() override {}
    void onImportDataFromJson(bool isOk) override;
    void onExportDataToJson(const std::string& data) override;
    void onPostFunctionToClientContext(MessageFunction&& func) override;
    void onExportTxHistoryToCsv(const std::string& data) override {};
    void onNotificationsChanged(beam::wallet::ChangeAction action, const std::vector<beam::wallet::Notification>&) override;
    void onExchangeRates(const std::vector<beam::wallet::ExchangeRate>&) override;
    void onGetAddress(const beam::wallet::WalletID& wid, const boost::optional<beam::wallet::WalletAddress>& address, size_t offlinePayments) override;
    void onShieldedCoinChanged(beam::wallet::ChangeAction action, const std::vector<beam::wallet::ShieldedCoin>& items) override;
    void onPublicAddress(const std::string& publicAddr) override;
};
