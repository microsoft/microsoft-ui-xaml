// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "AppAnalysisClientHelper.h"
#include "RpcClient.h"

using namespace WEX::Common;
using namespace WEX::TestExecution;

namespace Private { namespace Infrastructure {

    HRESULT AppAnalysisClientHelperStatics::EnableRule(HSTRING ruleId, HSTRING testIdentifier, BOOLEAN shouldHaveSourceInfo)
    {
        COM_START
        {
            RpcClientEnsureConnected();
            LogThrow_IfFailed(RpcEnableRule(
                static_cast<UINT>(GetCurrentProcessId()), 
                WindowsGetStringRawBuffer(ruleId, nullptr),
                WindowsGetStringRawBuffer(testIdentifier, nullptr),
                shouldHaveSourceInfo));
        }
        COM_END
    }

    HRESULT AppAnalysisClientHelperStatics::VerifyRuleTriggered(_In_ unsigned int totalTimesTriggered)
    {
        COM_START
        {
            RpcClientEnsureConnected();
            LogThrow_IfFailed(RpcVerifyRuleTriggered(totalTimesTriggered));
        }
        COM_END
    }

    HRESULT AppAnalysisClientHelperStatics::VerifyRuleNotTriggered()
    {
        COM_START
        {
            RpcClientEnsureConnected();
            LogThrow_IfFailed(RpcVerifyRuleNotTriggered());
        }
        COM_END
    }

    HRESULT AppAnalysisClientHelperStatics::VerifyMeasurement(_In_  unsigned int info, _In_ appanalysis::Measurement measurement)
    {
        COM_START
        {
            RpcClientEnsureConnected();
            LogThrow_IfFailed(RpcVerifyMeasurement(info, measurement.Unit, measurement.Value));
        }
        COM_END
    }

    HRESULT AppAnalysisClientHelperStatics::VerifySourceInfo(_In_  unsigned int info, _In_ appanalysis::SourceInfo sourceInfo)
    {
        COM_START
        {
            wrl::Wrappers::HString fileName;
            LogThrow_IfFailed(fileName.Set(sourceInfo.FileName));
            RpcClientEnsureConnected();
            LogThrow_IfFailed(RpcVerifySourceInfo(
                info, fileName.GetRawBuffer(nullptr), 
                sourceInfo.LineNumber, sourceInfo.ColumnNumber)
                );
        }
        COM_END
    }

    HRESULT AppAnalysisClientHelperStatics::VerifyCanLinkToLVT(_In_  unsigned int info, _In_ unsigned long long lvtHandle)
    {
        COM_START
        {
            RpcClientEnsureConnected();
            LogThrow_IfFailed(RpcVerifyCanLinkToLVT(info, lvtHandle));
        }
        COM_END
    }

    HRESULT AppAnalysisClientHelperStatics::VerifyDescription(
        _In_ unsigned int triggeredRuleIndex,
        _In_ appanalysis::IResourceString* description
        )
    {
        COM_START
        {
            RpcClientEnsureConnected();
            wrl::ComPtr<wfc::IVector<HSTRING>> descriptionStrings;
            LogThrow_IfFailed(description->QueryInterface<wfc::IVector<HSTRING>>(&descriptionStrings));
            UINT32 size = 0;
            LogThrow_IfFailed(descriptionStrings->get_Size(&size));
            std::vector<LPCWSTR> stringArgs(size);
            for (UINT32 i = 0; i < size; ++i)
            {
                wrl::Wrappers::HString arg;
                LogThrow_IfFailed(descriptionStrings->GetAt(i, arg.ReleaseAndGetAddressOf()));
                stringArgs[i] = arg.GetRawBuffer(nullptr);
            }

            UINT32 descriptionId = 0;
            LogThrow_IfFailed(description->get_Identifier(&descriptionId));
            LogThrow_IfFailed(RpcVerifyDescription(triggeredRuleIndex, descriptionId, size, stringArgs.data()));
        }
        COM_END
    }

    HRESULT AppAnalysisClientHelperStatics::DisableCurrentRule()
    {
        COM_START
        {
            RpcClientEnsureConnected();
            LogThrow_IfFailed(RpcDisableCurrentRule());
        }
        COM_END
    }
} }
