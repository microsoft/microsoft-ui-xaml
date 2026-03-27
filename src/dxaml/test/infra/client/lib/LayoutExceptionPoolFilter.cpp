// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "LayoutExceptionPoolFilter.h"
#include "XamlTailored.h"
#include "HostingDispatcher.h"

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace WEX::Common;
using namespace WEX::Logging;

namespace Private { namespace Infrastructure {

bool LayoutExceptionPoolFilter::IsDirty()
{
    bool isExceptionElementNull = false;

    wrl::ComPtr<xaml_primitives::ILayoutInformationStatics> spLayoutInfoStatics;
    LogThrow_IfFailed(wf::GetActivationFactory(
        wrl::Wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Controls_Primitives_LayoutInformation).Get(),
        &spLayoutInfoStatics));

    RunOnUIThread([&]()
    {
        auto spDispatcherQueue = HostingDispatcher::Get()->GetDispatcher();

        wrl::ComPtr<xaml::IUIElement> spUIElement;
        LogThrow_IfFailed(spLayoutInfoStatics->GetLayoutExceptionElement(spDispatcherQueue.Get(), spUIElement.ReleaseAndGetAddressOf()));

        isExceptionElementNull = (spUIElement == nullptr);
    });

    if (!isExceptionElementNull)
    {
        Log::Error(L"Layout exception element is not null.");
    }

    return !isExceptionElementNull;
}

} }

