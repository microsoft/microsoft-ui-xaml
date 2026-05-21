// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <XamlTailored.h>
#include <wincodec.h>

#include "Utilities.h"
#include "TestServices.h"
#include "SmartStackLogger.h"
#include "WindowHelper.h"
#include "DPISetter.h"

using namespace WEX::Common;
using namespace WEX::TestExecution;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace ::Private::Infrastructure;

namespace Private { namespace Infrastructure {

DPISetter::DPISetter(LUID displayAdapterId, UINT displayAdapterTargetId, test_infra::DisplayDPIRange newDpi)
    : m_displayAdapterTargetId(displayAdapterTargetId),
    m_displayAdapterId(displayAdapterId),
    m_newDpi(newDpi)
{
}

HRESULT DPISetter::Init()
{
    int oldDpi = 0;
    LogThrow_IfFailed(RpcSetDpi(m_displayAdapterId, m_displayAdapterTargetId, m_newDpi, &oldDpi));
    m_oldDpi = static_cast<test_infra::DisplayDPIRange>(oldDpi);
    return S_OK;
}

HRESULT DPISetter::Close()
{
    int oldDpi = 0;
    LogThrow_IfFailed(RpcSetDpi(m_displayAdapterId, m_displayAdapterTargetId, m_oldDpi, &oldDpi));
    return S_OK;
}

} }

