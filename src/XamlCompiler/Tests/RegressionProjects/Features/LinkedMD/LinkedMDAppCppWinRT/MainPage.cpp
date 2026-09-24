// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "MainPage.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Markup;

namespace winrt::LinkedMDAppCppWinRT::implementation
{
    static IXamlType EnsureTypeExists(hstring typeName)
    {
        auto ixmp = Application::Current().as<IXamlMetadataProvider>();
        IXamlType t = ixmp.GetXamlType(typeName);
        if (!t)
        {
            throw hresult_invalid_argument(typeName);
        }
        return t;
    }

    static IXamlMember EnsureMemberExists(IXamlType t, hstring memberName)
    {
        auto m = t.GetMember(memberName);
        if (!m)
        {
            throw hresult_invalid_argument(memberName);
        }
        return m;
    }

    MainPage::MainPage()
    {
        InitializeComponent();
        IXamlType t;

        t = EnsureTypeExists(L"LinkedMDAppCppWinRT.MainPage");
        
        // C++
        t = EnsureTypeExists(L"LinkedMDControlsCppWinRT.A");
        EnsureMemberExists(t, L"StringPropertyOnA");
        EnsureMemberExists(t, L"BPropertyOnA");
        t = EnsureTypeExists(L"LinkedMDControlsCppWinRT.B");
        EnsureMemberExists(t, L"StringPropertyOnB");
        t = EnsureTypeExists(L"LinkedMDSubControlsCppWinRT.S");
        EnsureMemberExists(t, L"StringPropertyOnS");
        EnsureMemberExists(t, L"TPropertyOnS");
        t = EnsureTypeExists(L"LinkedMDSubControlsCppWinRT.T");
        EnsureMemberExists(t, L"StringPropertyOnT");

        // CX
        t = EnsureTypeExists(L"ControlsCX.A");
        EnsureMemberExists(t, L"StringPropertyOnA");
        EnsureMemberExists(t, L"BPropertyOnA");
        t = EnsureTypeExists(L"ControlsCX.B");
        EnsureMemberExists(t, L"StringPropertyOnB");
        t = EnsureTypeExists(L"SubControlsCX.S");
        EnsureMemberExists(t, L"StringPropertyOnS");
        EnsureMemberExists(t, L"TPropertyOnS");
        t = EnsureTypeExists(L"SubControlsCX.T");
        EnsureMemberExists(t, L"StringPropertyOnT");
    }
}
