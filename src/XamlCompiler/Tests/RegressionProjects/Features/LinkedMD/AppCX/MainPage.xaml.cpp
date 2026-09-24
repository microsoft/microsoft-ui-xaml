// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"
#include "App.xaml.h"

using namespace AppCX;

using namespace Platform;
using namespace ::Windows::Foundation;
using namespace ::Windows::Foundation::Collections;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Data;
using namespace Microsoft::UI::Xaml::Input;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Navigation;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

IXamlType^ EnsureTypeExists(String^ typeName)
{
    auto ixmp = dynamic_cast<App^>(Application::Current);
    auto t = ixmp->GetXamlType(typeName);
    if (!t)
    {
        throw ref new Platform::InvalidArgumentException(typeName);
    }
    return t;
}

IXamlMember^ EnsureMemberExists(IXamlType^ t, String^ memberName)
{
    auto m = t->GetMember(memberName);
    if (!m)
    {
        throw ref new Platform::InvalidArgumentException(memberName);
    }
    return m;
}


MainPage::MainPage()
{
    InitializeComponent();

    IXamlType^ t;

    t = EnsureTypeExists("AppCX.MainPage");

    // CX
    t = EnsureTypeExists("ControlsCX.A");
    EnsureMemberExists(t, "StringPropertyOnA");
    EnsureMemberExists(t, "BPropertyOnA");
    t = EnsureTypeExists("ControlsCX.B");
    EnsureMemberExists(t, "StringPropertyOnB");
    t = EnsureTypeExists("SubControlsCX.S");
    EnsureMemberExists(t, "StringPropertyOnS");
    EnsureMemberExists(t, "TPropertyOnS");
    t = EnsureTypeExists("SubControlsCX.T");
    EnsureMemberExists(t, "StringPropertyOnT");

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
}
