// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "FocusTestPoolFilter.h"
#include "XamlTailored.h"
#include "Hosting.h"

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace WEX::Common;
using namespace WEX::Logging;

namespace Private {
    namespace Infrastructure {

        bool FocusTestPoolFilter::IsEnabled()
        {
            Hosting::HostingMode hostingMode = Hosting::HostingMode::UAP;
            LogThrow_IfFailed(GetHostingMode(&hostingMode));

            return (hostingMode == Hosting::HostingMode::UAP);
        }

        bool FocusTestPoolFilter::IsDirty()
        {
            wrl::ComPtr<IInspectable> focusedElement;

            wrl::ComPtr<xaml_input::IFocusManagerStatics> focusManager;
            LogThrow_IfFailed(wf::GetActivationFactory(wrl_wrappers::HStringReference(L"Microsoft.UI.Xaml.Input.FocusManager").Get(),
                &focusManager));

            RunOnUIThread([&]() {
                focusManager->GetFocusedElement(&focusedElement);
            });

            return focusedElement != nullptr;
        }

    }
}
