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

#include "aregister_transaction.h"
#include "../../core/base_tx_builder.h"
#include "core/block_crypt.h"
#include "wallet/core/strings_resources.h"
#include "wallet/core/wallet.h"
#include "utility/logger.h"

namespace beam::wallet
{
    BaseTransaction::Ptr AssetRegisterTransaction::Creator::Create(const TxContext& context)
    {
        return BaseTransaction::Ptr(new AssetRegisterTransaction(context));
    }

    TxParameters AssetRegisterTransaction::Creator::CheckAndCompleteParameters(const TxParameters& params)
    {
        if(params.GetParameter<WalletID>(TxParameterID::PeerID))
        {
            throw InvalidTransactionParametersException("Asset registration: unexpected PeerID");
        }

        if(params.GetParameter<WalletID>(TxParameterID::MyID))
        {
            throw InvalidTransactionParametersException("Asset registration: unexpected MyID");
        }

        const auto isSenderO = params.GetParameter<bool>(TxParameterID::IsSender);
        if (!isSenderO || !isSenderO.get())
        {
            throw InvalidTransactionParametersException("Asset registration: non-sender transaction");
        }

        const auto isInitiatorO = params.GetParameter<bool>(TxParameterID::IsInitiator);
        if (!isInitiatorO || !isInitiatorO.get())
        {
            throw InvalidTransactionParametersException("Asset registration: non-initiator transaction");
        }

        TxParameters result{params};
        result.SetParameter(TxParameterID::IsSelfTx, true);
        result.SetParameter(TxParameterID::MyID, WalletID(Zero)); // Mandatory parameter
        return result;
    }

    AssetRegisterTransaction::AssetRegisterTransaction(const TxContext& context)
        : AssetTransaction(context)
    {
    }

    struct AssetRegisterTransaction::MyBuilder
        :public AssetTransaction::Builder
    {
        MyBuilder(AssetRegisterTransaction& tx)
            :Builder(tx, kDefaultSubTxID)
        {
            const auto amount = GetParameterStrict<Amount>(TxParameterID::Amount);
            if (amount < Rules::get().CA.DepositForList)
            {
                throw TransactionFailedException(!m_Tx.IsInitiator(), TxFailureReason::RegisterAmountTooSmall);
            }
        }

        void Sign()
        {
            if (m_pKrn)
                return;

            std::unique_ptr<TxKernelAssetCreate> pKrn = std::make_unique<TxKernelAssetCreate>();
            pKrn->m_MetaData = m_Md;
            AddKernel(std::move(pKrn));

            FinalyzeTx();
        }
    };

    void AssetRegisterTransaction::UpdateImpl()
    {
        if (!AssetTransaction::BaseUpdate())
            return;

        if (!_builder)
            _builder = std::make_shared<MyBuilder>(*this);

        auto& builder = *_builder;

        if (builder.m_Coins.IsEmpty())
        {
            builder.MakeInputsAndChange(Rules::get().CA.DepositForList + builder.m_Fee, 0);
            builder.SaveCoins();

            UpdateTxDescription(TxStatus::InProgress);
            SetState(State::Making);
        }

        builder.GenerateInOuts();
        if (builder.IsGeneratingInOuts())
            return; // Sign() would verify the tx, but it can't be verified until all in/outs are prepared

        if (!builder.m_pKrn)
        {
            builder.Sign();

            LOG_INFO() << GetTxID() << " Registering asset with the owner ID " << builder.m_pKrn->CastTo_AssetCreate().m_Owner
                << ". Cost is " << PrintableAmount(Rules::get().CA.DepositForList, false)
                << ". Fee is " << PrintableAmount(builder.m_Fee, false);
        }

        auto registered = proto::TxStatus::Unspecified;
        if (!GetParameter(TxParameterID::TransactionRegistered, registered))
        {
            if(const auto ainfo = GetWalletDB()->findAsset(builder.m_pKrn->CastTo_AssetCreate().m_Owner))
            {
                OnFailed(TxFailureReason::AssetExists);
                return;
            }

			SetState(State::Registration);
            GetGateway().register_tx(GetTxID(), builder.m_pTransaction);
            return;
        }

        if (proto::TxStatus::Ok != registered)
        {
            OnFailed(TxFailureReason::FailedToRegister);
            return;
        }

        Height kpHeight = 0;
        GetParameter(TxParameterID::KernelProofHeight, kpHeight);
        if (!kpHeight)
        {
            SetState(State::KernelConfirmation);
            ConfirmKernel(builder.m_pKrn->m_Internal.m_ID);
            return;
        }

        if (GetState() == State::KernelConfirmation)
        {
            LOG_INFO() << GetTxID() << " Asset with the owner ID " << builder.m_pKrn->CastTo_AssetCreate().m_Owner << " successfully registered";
            SetState(State::AssetConfirmation);
            ConfirmAsset();
            return;
        }

        if (GetState() == State::AssetConfirmation)
        {
            Height auHeight = 0;
            GetParameter(TxParameterID::AssetUnconfirmedHeight, auHeight);
            if (auHeight)
            {
                OnFailed(TxFailureReason::AssetConfirmFailed);
                return;
            }

            Height acHeight = 0;
            GetParameter(TxParameterID::AssetConfirmedHeight, acHeight);
            if (!acHeight)
            {
                ConfirmAsset();
                return;
            }

            SetState(State::AssetCheck);
        }

        if(GetState() == State::AssetCheck)
        {
            Asset::Full info;
            if (!GetParameter(TxParameterID::AssetInfoFull, info) || !info.IsValid())
            {
                OnFailed(TxFailureReason::NoAssetInfo);
                return;
            }

            if(builder.m_pKrn->CastTo_AssetCreate().m_Owner != info.m_Owner)
            {
                OnFailed(TxFailureReason::InvalidAssetOwnerId);
                return;
            }

            SetParameter(TxParameterID::AssetID, info.m_ID);
        }

        SetState(State::Finalizing);
        SetCompletedTxCoinStatuses(kpHeight);
        CompleteTx();
    }

    void AssetRegisterTransaction::ConfirmAsset()
    {
        GetGateway().confirm_asset(GetTxID(), _builder->m_pKrn->CastTo_AssetCreate().m_Owner, kDefaultSubTxID);
    }

    bool AssetRegisterTransaction::ShouldNotifyAboutChanges(TxParameterID paramID) const
    {
        switch (paramID)
        {
        case TxParameterID::Fee:
        case TxParameterID::MinHeight:
        case TxParameterID::CreateTime:
        case TxParameterID::IsSender:
        case TxParameterID::Status:
        case TxParameterID::TransactionType:
        case TxParameterID::KernelID:
            return true;
        default:
            return false;
        }
    }

    TxType AssetRegisterTransaction::GetType() const
    {
        return TxType::AssetReg;
    }

    AssetRegisterTransaction::State AssetRegisterTransaction::GetState() const
    {
        State state = State::Initial;
        GetParameter(TxParameterID::State, state);
        return state;
    }

    bool AssetRegisterTransaction::IsInSafety() const
    {
        State txState = GetState();
        return txState >= State::KernelConfirmation;
    }
}
