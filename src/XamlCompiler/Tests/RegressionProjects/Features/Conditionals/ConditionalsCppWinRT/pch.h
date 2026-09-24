// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// pch.h
// Header for platform projection include files
//

#pragma once

#include "hstring.h"
#include "windows.h"

#include "winrt/Windows.ApplicationModel.Activation.h"
#include "winrt/Windows.Foundation.h"
#include "winrt/Windows.Foundation.Metadata.h"
#include "winrt/Windows.UI.h"
#include "winrt/Windows.UI.Popups.h"
#include "winrt/Microsoft.UI.Xaml.h"
#include "winrt/Microsoft.UI.Xaml.Controls.h"
#include "winrt/Microsoft.UI.Xaml.Controls.Primitives.h"
#include "winrt/Microsoft.UI.Xaml.Data.h"
#include "winrt/Microsoft.UI.Xaml.Documents.h"
#include "winrt/Microsoft.UI.Xaml.Input.h"
#include "winrt/Microsoft.UI.Xaml.Interop.h"
#include "winrt/Microsoft.UI.Xaml.Markup.h"
#include "winrt/Microsoft.UI.Xaml.Media.h"
#include "winrt/Microsoft.UI.Xaml.Navigation.h"
#include "winrt/Microsoft.UI.Xaml.Shapes.h"

#include "winrt/ConditionalControls.h"
#include "winrt/ConditionalControls.ConditionalsModel_XamlTypeInfo.h"

namespace winrt::ConditionalsCppWinRT::implementation
{
    namespace wa = ::Windows::ApplicationModel;
    namespace wf = ::Windows::Foundation;
    namespace wfc = ::Windows::Foundation::Collections;
    namespace wux = Microsoft::UI::Xaml;
    namespace wuxc = Microsoft::UI::Xaml::Controls;
}
