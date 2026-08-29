// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "XamlTailored.h"
#include "SafeEventRegistration.h"
#include "TestEvent.h"
#include "SystemBackdropTests.h"
#include "TestCleanupWrapper.h"
#include "WUCRenderingScopeGuard.h"
#include <Collection.h>
#include "CommonInputHelper.h"
#include "FlyoutHelper.h"
#include "WindowAutoCloser.h"
#include "VisualDebugTags.h"

using namespace Platform;
using namespace Microsoft::UI::Composition;
using namespace Microsoft::UI::Composition::SystemBackdrops;
using namespace Microsoft::UI::Content;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Hosting;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace MockDComp;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

bool SystemBackdropTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool SystemBackdropTests::ClassCleanup()
{
    return true;
}

bool SystemBackdropTests::TestSetup()
{
    TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool SystemBackdropTests::TestCleanup()
{
    TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void SystemBackdropTests::APITest()
{
    TestCleanupWrapper cleanup;

    const auto& wh = TestServices::WindowHelper;

    RunOnUIThread([&]()
    {
        Canvas^ root = ref new Canvas();
        wh->WindowContent = root;

        LOG_OUTPUT(L"> Creating DesktopAcrylicBackdrop from code.");
        Microsoft::UI::Xaml::Media::SystemBackdrop^ systemDABackdropCode = ref new DesktopAcrylicBackdrop();
        VERIFY_IS_NOT_NULL(systemDABackdropCode);

        LOG_OUTPUT(L"> Creating MicaBackdrop from code.");
        Microsoft::UI::Xaml::Media::SystemBackdrop^ systemMicaBackdropCode = ref new MicaBackdrop();
        VERIFY_IS_NOT_NULL(systemMicaBackdropCode);

        LOG_OUTPUT(L"> Creating DesktopAcrylicBackdrop from markup.");
        Microsoft::UI::Xaml::Media::SystemBackdrop^ systemDABackdropMarkup = safe_cast<SystemBackdrop^>(XamlReader::Load(
            L"<DesktopAcrylicBackdrop xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' />"
            ));
        VERIFY_IS_NOT_NULL(systemDABackdropMarkup);

        LOG_OUTPUT(L"> Creating MicaBackdrop from markup.");
        Microsoft::UI::Xaml::Media::SystemBackdrop^ systemMicaBackdropMarkup = safe_cast<SystemBackdrop^>(XamlReader::Load(
            L"<MicaBackdrop xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' />"
            ));
        VERIFY_IS_NOT_NULL(systemMicaBackdropMarkup);
    });
    wh->WaitForIdle();
}

void SystemBackdropTests::DesktopAcrylicInWindowedPopup()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Popup^ windowedPopupCode;
    Popup^ windowedPopupMarkup;

    MenuFlyout^ menuFlyout;
    CommandBarFlyout^ commandBarFlyout;
    CommandBarFlyout^ commandBarFlyout2;

    RunOnUIThread([&]()
    {
        Canvas^ canvas = ref new Canvas();
        canvas->Width = 150;
        canvas->Height = 150;
        canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Green);

        LOG_OUTPUT(L"> Creating Popup from code.");
        windowedPopupCode = ref new Popup();
        windowedPopupCode->ShouldConstrainToRootBounds = false;
        windowedPopupCode->SystemBackdrop = ref new DesktopAcrylicBackdrop();
        windowedPopupCode->Child = canvas;

        LOG_OUTPUT(L"> Creating Popup from markup.");
        windowedPopupMarkup = safe_cast<Popup^>(XamlReader::Load(
            L"<Popup xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' ShouldConstrainToRootBounds='False'>\
                <Popup.SystemBackdrop>\
                    <DesktopAcrylicBackdrop />\
                </Popup.SystemBackdrop>\
                <Canvas Width='100' Height='100' Background='Red' />\
            </Popup>"));

        Canvas^ root = ref new Canvas();
        root->Children->Append(windowedPopupCode);
        root->Children->Append(windowedPopupMarkup);
        wh->WindowContent = root;

        LOG_OUTPUT(L"> Creating MenuFlyout from code.");
        menuFlyout = ref new MenuFlyout();
        menuFlyout->ShouldConstrainToRootBounds = false;
        menuFlyout->SystemBackdrop = ref new DesktopAcrylicBackdrop();

        LOG_OUTPUT(L"> Creating DesktopAcrylicBackdrop from code.");
        DesktopAcrylicBackdrop^ desktopAcrylicBackdrop = ref new DesktopAcrylicBackdrop();

        LOG_OUTPUT(L"> Creating CommandBarFlyout from code.");
        commandBarFlyout = ref new CommandBarFlyout();
        commandBarFlyout->ShouldConstrainToRootBounds = false;
        commandBarFlyout->SystemBackdrop = desktopAcrylicBackdrop;

        LOG_OUTPUT(L"> Attempting to share DesktopAcrylicBackdrop. Expecting success.");
        commandBarFlyout2 = ref new CommandBarFlyout();
        commandBarFlyout2->ShouldConstrainToRootBounds = false;
        commandBarFlyout2->SystemBackdrop = desktopAcrylicBackdrop;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Opening Popups.");
        windowedPopupCode->IsOpen = true;
        windowedPopupMarkup->IsOpen = true;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        VisualTreeVerifier::CreateFromElement(windowedPopupCode)
            ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::WindowedPopup_AnimationRootVisual))
            ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::WindowedPopup_SystemBackdropPlacementVisual))
            ->VerifyVisualSize(150, 150);

        VisualTreeVerifier::CreateFromElement(windowedPopupMarkup)
            ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::WindowedPopup_AnimationRootVisual))
            ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::WindowedPopup_SystemBackdropPlacementVisual))
            ->VerifyVisualSize(100, 100);
    });
}

void SystemBackdropTests::MicaInWindowedPopup()
{
    const auto& wh = TestServices::WindowHelper;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Popup^ windowedPopupCode;
    Popup^ windowedPopupMarkup;

    MenuFlyout^ menuFlyout;
    CommandBarFlyout^ commandBarFlyout;
    CommandBarFlyout^ commandBarFlyout2;

    RunOnUIThread([&]()
    {
        Canvas^ canvas = ref new Canvas();
        canvas->Width = 150;
        canvas->Height = 150;
        canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Green);

        LOG_OUTPUT(L"> Creating Popup from code.");
        windowedPopupCode = ref new Popup();
        windowedPopupCode->ShouldConstrainToRootBounds = false;
        windowedPopupCode->SystemBackdrop = ref new MicaBackdrop();
        windowedPopupCode->Child = canvas;

        LOG_OUTPUT(L"> Creating Popup from markup.");
        windowedPopupMarkup = safe_cast<Popup^>(XamlReader::Load(
            L"<Popup xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' ShouldConstrainToRootBounds='False'>\
                <Popup.SystemBackdrop>\
                    <MicaBackdrop />\
                </Popup.SystemBackdrop>\
                <Canvas Width='100' Height='100' Background='Red' />\
            </Popup>"));

        Canvas^ root = ref new Canvas();
        root->Children->Append(windowedPopupCode);
        root->Children->Append(windowedPopupMarkup);
        wh->WindowContent = root;

        LOG_OUTPUT(L"> Creating MenuFlyout from code.");
        menuFlyout = ref new MenuFlyout();
        menuFlyout->ShouldConstrainToRootBounds = false;
        menuFlyout->SystemBackdrop = ref new MicaBackdrop();

        LOG_OUTPUT(L"> Creating MicaBackdrop from code.");
        MicaBackdrop^ micaBackdrop = ref new MicaBackdrop();

        LOG_OUTPUT(L"> Creating CommandBarFlyout from code.");
        commandBarFlyout = ref new CommandBarFlyout();
        commandBarFlyout->ShouldConstrainToRootBounds = false;
        commandBarFlyout->SystemBackdrop = micaBackdrop;

        LOG_OUTPUT(L"> Attempting to share MicaBackdrop. Expecting success.");
        commandBarFlyout2 = ref new CommandBarFlyout();
        commandBarFlyout2->ShouldConstrainToRootBounds = false;
        commandBarFlyout2->SystemBackdrop = micaBackdrop;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Opening Popups.");
        windowedPopupCode->IsOpen = true;
        windowedPopupMarkup->IsOpen = true;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        VisualTreeVerifier::CreateFromElement(windowedPopupCode)
            ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::WindowedPopup_AnimationRootVisual))
            ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::WindowedPopup_SystemBackdropPlacementVisual))
            ->VerifyVisualSize(150, 150);

        VisualTreeVerifier::CreateFromElement(windowedPopupMarkup)
            ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::WindowedPopup_AnimationRootVisual))
            ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::WindowedPopup_SystemBackdropPlacementVisual))
            ->VerifyVisualSize(100, 100);
    });
}

ref class CustomBackdrop_TrackOnConnected : public SystemBackdrop
{
public:
    void OnTargetConnected(ICompositionSupportsSystemBackdrop^ connectedTarget, XamlRoot^ xamlRoot) override
    {
        LOG_OUTPUT(L"    > SystemBackdrop::OnTargetConnected called.");
        __super::OnTargetConnected(connectedTarget, xamlRoot);

        m_connectedCount++;
        m_ConnectedTarget = connectedTarget;
        m_XamlRoot = xamlRoot;
    }

    void OnTargetDisconnected(ICompositionSupportsSystemBackdrop^ disconnectedTarget) override
    {
        LOG_OUTPUT(L"    > SystemBackdrop::OnTargetDisconnected called.");
        __super::OnTargetDisconnected(disconnectedTarget);

        m_disconnectedCount++;
        m_DisconnectedTarget = disconnectedTarget;
        m_XamlRoot = nullptr;
    }

    void OnDefaultSystemBackdropConfigurationChanged(ICompositionSupportsSystemBackdrop^ target, XamlRoot^ xamlRoot) override
    {
        LOG_OUTPUT(L"    > SystemBackdrop::OnDefaultSystemBackdropConfigurationChanged called.");
        m_changedCount++;
    }

    int GetConnectedCount() { return m_connectedCount; }
    ICompositionSupportsSystemBackdrop^ GetConnectedTarget() { return m_ConnectedTarget; }

    int GetDisconnectedCount() { return m_disconnectedCount; }
    ICompositionSupportsSystemBackdrop^ GetDisconnectedTarget() { return m_DisconnectedTarget; }

    int GetChangedCount() { return m_changedCount; }

    XamlRoot^ GetXamlRoot() { return m_XamlRoot; }

    ixp::SystemBackdrops::SystemBackdropTheme GetConfiguration_Theme()
    {
        return SystemBackdrop::GetDefaultSystemBackdropConfiguration(m_ConnectedTarget, m_XamlRoot)->Theme;
    }

    bool GetConfiguration_IsHighContrast()
    {
        return SystemBackdrop::GetDefaultSystemBackdropConfiguration(m_ConnectedTarget, m_XamlRoot)->IsHighContrast;
    }

private:
    int m_connectedCount {0};
    ICompositionSupportsSystemBackdrop^ m_ConnectedTarget;

    int m_disconnectedCount {0};
    ICompositionSupportsSystemBackdrop^ m_DisconnectedTarget;

    XamlRoot^ m_XamlRoot;

    int m_changedCount {0};
};

void SystemBackdropTests::DesktopAcrylicInNonWindowedPopup()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Popup^ nonwindowedPopupCode;
    Popup^ nonwindowedPopupMarkup;


    RunOnUIThread([&]()
    {
        Canvas^ canvas = ref new Canvas();
        canvas->Width = 150;
        canvas->Height = 150;
        canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Green);

        LOG_OUTPUT(L"> Creating Popup from code.");
        nonwindowedPopupCode = ref new Popup();
        nonwindowedPopupCode->ShouldConstrainToRootBounds = true;
        nonwindowedPopupCode->Child = canvas;

        LOG_OUTPUT(L"> Creating Popup from markup.");
        nonwindowedPopupMarkup = safe_cast<Popup^>(XamlReader::Load(
            L"<Popup xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' ShouldConstrainToRootBounds='True'>\
                <Popup.SystemBackdrop>\
                    <DesktopAcrylicBackdrop />\
                </Popup.SystemBackdrop>\
                <Canvas Width='100' Height='100' Background='Red' />\
            </Popup>"));

        Canvas^ root = ref new Canvas();
        root->Children->Append(nonwindowedPopupCode);
        root->Children->Append(nonwindowedPopupMarkup);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        CustomBackdrop_TrackOnConnected^ customBackdrop = ref new CustomBackdrop_TrackOnConnected();
        ICompositionSupportsSystemBackdrop^ nonwindowedPopupCodeICSSB = dynamic_cast<ICompositionSupportsSystemBackdrop^>(nonwindowedPopupCode);
        nonwindowedPopupCode->SystemBackdrop = customBackdrop;

        LOG_OUTPUT(L"> Opening Popups.");
        nonwindowedPopupCode->IsOpen = true;
        nonwindowedPopupMarkup->IsOpen = true;
        VERIFY_ARE_EQUAL(0, customBackdrop->GetConnectedCount());
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        VisualTreeVerifier::CreateFromElement(nonwindowedPopupMarkup)
            ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::CompNode_PrimaryVisual))
            ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::CompNode_ContentVisual))
            ->WalkThroughSimpleCompNode()
            ->WalkToChildAtIndex(0, 1)
            ->VerifyVisualSize(100, 100);

        VisualTreeVerifier::CreateFromElement(nonwindowedPopupCode)
            ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::CompNode_PrimaryVisual))
            ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::CompNode_ContentVisual))
            ->WalkThroughSimpleCompNode()
            ->WalkToChildAtIndex(0, 1)
            ->VerifyVisualSize(150, 150);

        VisualTreeVerifier::CreateFromElement(nonwindowedPopupMarkup)
            ->VerifyNoTaggedChild(static_cast<UINT>(VisualDebugTags::WindowedPopup_AnimationRootVisual));

        VisualTreeVerifier::CreateFromElement(nonwindowedPopupCode)
            ->VerifyNoTaggedChild(static_cast<UINT>(VisualDebugTags::WindowedPopup_AnimationRootVisual));
    });
}

void SystemBackdropTests::OnTargetConnectedDisconnected()
{
    const auto& wh = TestServices::WindowHelper;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ root;
    Popup^ parentedPopup1;
    Popup^ parentedPopup2;
    Popup^ inlineParentedPopup;
    Popup^ parentlessPopup;

    LOG_OUTPUT(L">>> Testing parented popup.");
    RunOnUIThread([&]()
    {
        Canvas^ canvas;

        canvas = ref new Canvas();
        canvas->Width = 150;
        canvas->Height = 150;
        canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Green);

        LOG_OUTPUT(L"  > Creating parented Popups.");
        parentedPopup1 = ref new Popup();
        parentedPopup1->ShouldConstrainToRootBounds = false;
        parentedPopup1->Child = canvas;

        canvas = ref new Canvas();
        canvas->Width = 125;
        canvas->Height = 125;
        canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Lime);

        parentedPopup2 = ref new Popup();
        parentedPopup2->ShouldConstrainToRootBounds = false;
        parentedPopup2->Child = canvas;

        root = ref new Canvas();
        root->Children->Append(parentedPopup1);
        root->Children->Append(parentedPopup2);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        ICompositionSupportsSystemBackdrop^ parentedPopup1ICSSB = dynamic_cast<ICompositionSupportsSystemBackdrop^>(parentedPopup1);
        ICompositionSupportsSystemBackdrop^ parentedPopup2ICSSB = dynamic_cast<ICompositionSupportsSystemBackdrop^>(parentedPopup2);

        LOG_OUTPUT(L"  > Creating custom SystemBackdrop.");
        CustomBackdrop_TrackOnConnected^ customBackdrop = ref new CustomBackdrop_TrackOnConnected();

        LOG_OUTPUT(L"\n> Attach-Detach-Open-Attach-Detach-Close");

        LOG_OUTPUT(L"  > Attaching to popup1. OnTargetConnected should not be called since popup is not open.");
        parentedPopup1->SystemBackdrop = customBackdrop;
        VERIFY_ARE_EQUAL(0, customBackdrop->GetConnectedCount());
        VERIFY_ARE_EQUAL(0, customBackdrop->GetDisconnectedCount());

        LOG_OUTPUT(L"  > Detaching from popup1. OnTargetDisconnected should not be called since popup is still not open.");
        parentedPopup1->SystemBackdrop = nullptr;
        VERIFY_ARE_EQUAL(0, customBackdrop->GetConnectedCount());
        VERIFY_ARE_EQUAL(0, customBackdrop->GetDisconnectedCount());

        LOG_OUTPUT(L"  > Opening popup1 and attaching. Expect an OnTargetConnected call.");
        parentedPopup1->IsOpen = true;
        parentedPopup1->SystemBackdrop = customBackdrop;
        VERIFY_ARE_EQUAL(1, customBackdrop->GetConnectedCount());
        VERIFY_ARE_EQUAL(parentedPopup1ICSSB, customBackdrop->GetConnectedTarget());
        VERIFY_ARE_EQUAL(0, customBackdrop->GetDisconnectedCount());

        LOG_OUTPUT(L"  > Detaching from popup1. Expect an OnTargetDisconnected call.");
        parentedPopup1->SystemBackdrop = nullptr;
        VERIFY_ARE_EQUAL(1, customBackdrop->GetConnectedCount());
        VERIFY_ARE_EQUAL(1, customBackdrop->GetDisconnectedCount());
        VERIFY_ARE_EQUAL(parentedPopup1ICSSB, customBackdrop->GetDisconnectedTarget());

        LOG_OUTPUT(L"  > Closing popup1. This shouldn't deliver a duplicate OnTargetDisconnected call.");
        parentedPopup1->IsOpen = false;
        VERIFY_ARE_EQUAL(1, customBackdrop->GetConnectedCount());
        VERIFY_ARE_EQUAL(1, customBackdrop->GetDisconnectedCount());

        LOG_OUTPUT(L"\n> Open-Attach-Close-Unparent-Detach");

        LOG_OUTPUT(L"  > Opening popup2.");
        parentedPopup2->IsOpen = true;

        LOG_OUTPUT(L"  > Attaching to popup2. Expect an OnTargetConnected call.");
        parentedPopup2->SystemBackdrop = customBackdrop;
        VERIFY_ARE_EQUAL(2, customBackdrop->GetConnectedCount());
        VERIFY_ARE_EQUAL(parentedPopup2ICSSB, customBackdrop->GetConnectedTarget());
        VERIFY_ARE_EQUAL(1, customBackdrop->GetDisconnectedCount());

        LOG_OUTPUT(L"  > Attaching the same object to popup2. Expect no OnTargetConnected or OnTargetDisconnected calls.");
        parentedPopup2->SystemBackdrop = customBackdrop;
        VERIFY_ARE_EQUAL(2, customBackdrop->GetConnectedCount());
        VERIFY_ARE_EQUAL(1, customBackdrop->GetDisconnectedCount());

        LOG_OUTPUT(L"  > Removing popup2 from the tree. This should implicitly close the popup and deliver an OnTargetDisconnected call.");
        root->Children->RemoveAt(1);
        VERIFY_ARE_EQUAL(2, customBackdrop->GetConnectedCount());
        VERIFY_ARE_EQUAL(2, customBackdrop->GetDisconnectedCount());
        VERIFY_ARE_EQUAL(false, parentedPopup2->IsOpen);

        LOG_OUTPUT(L"  > Detaching from popup2. This shouldn't deliver an OnTargetDisconnected call.");
        parentedPopup2->SystemBackdrop = nullptr;
        VERIFY_ARE_EQUAL(2, customBackdrop->GetConnectedCount());
        VERIFY_ARE_EQUAL(2, customBackdrop->GetDisconnectedCount());
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L">>> Testing inline parented popup.");
    RunOnUIThread([&]()
    {
        Canvas^ canvas = ref new Canvas();
        canvas->Width = 100;
        canvas->Height = 100;
        canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);

        LOG_OUTPUT(L"  > Creating inline parented Popup.");
        inlineParentedPopup = ref new Popup();
        inlineParentedPopup->Child = canvas;

        root->Children->Append(inlineParentedPopup);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Creating custom SystemBackdrop.");
        CustomBackdrop_TrackOnConnected^ customBackdrop = ref new CustomBackdrop_TrackOnConnected();

        LOG_OUTPUT(L"\n> Attach-Open-Detach-Close");

        LOG_OUTPUT(L"  > Attaching to inline parented popup. OnTargetConnected should not be called since popup is neither open nor windowed.");
        inlineParentedPopup->SystemBackdrop = customBackdrop;
        VERIFY_ARE_EQUAL(0, customBackdrop->GetConnectedCount());
        VERIFY_ARE_EQUAL(0, customBackdrop->GetDisconnectedCount());

        LOG_OUTPUT(L"  > Opening inline parented popup. Don't expect an OnTargetConnected call, since popup is unwindowed.");
        inlineParentedPopup->IsOpen = true;
        VERIFY_ARE_EQUAL(0, customBackdrop->GetConnectedCount());
        VERIFY_ARE_EQUAL(0, customBackdrop->GetDisconnectedCount());

        LOG_OUTPUT(L"  > Detaching from inline parented popup. Don't expect an OnTargetDisconnected call, since popup is unwindowed.");
        inlineParentedPopup->SystemBackdrop = nullptr;
        VERIFY_ARE_EQUAL(0, customBackdrop->GetConnectedCount());
        VERIFY_ARE_EQUAL(0, customBackdrop->GetDisconnectedCount());

        LOG_OUTPUT(L"  > Closing inline parented popup. Don't expect an OnTargetDisconnected call, since popup is unwindowed.");
        inlineParentedPopup->IsOpen = false;
        VERIFY_ARE_EQUAL(0, customBackdrop->GetConnectedCount());
        VERIFY_ARE_EQUAL(0, customBackdrop->GetDisconnectedCount());
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L">>> Testing parentless popup.");
    RunOnUIThread([&]()
    {
        Canvas^ canvas = ref new Canvas();
        canvas->Width = 100;
        canvas->Height = 100;
        canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Red);

        LOG_OUTPUT(L"  > Creating parentless Popup.");
        parentlessPopup = ref new Popup();
        parentlessPopup->ShouldConstrainToRootBounds = false;
        parentlessPopup->Child = canvas;
        parentlessPopup->XamlRoot = parentedPopup1->XamlRoot;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        ICompositionSupportsSystemBackdrop^ parentlessPopupICSSB = dynamic_cast<ICompositionSupportsSystemBackdrop^>(parentlessPopup);

        LOG_OUTPUT(L"  > Creating custom SystemBackdrop.");
        CustomBackdrop_TrackOnConnected^ customBackdrop = ref new CustomBackdrop_TrackOnConnected();

        LOG_OUTPUT(L"\n> Attach-Open-Detach-Close");

        LOG_OUTPUT(L"  > Attaching to popup. Don't expect an OnTargetConnected call because the popup isn't open yet.");
        parentlessPopup->SystemBackdrop = customBackdrop;
        VERIFY_ARE_EQUAL(0, customBackdrop->GetConnectedCount());
        VERIFY_ARE_EQUAL(0, customBackdrop->GetDisconnectedCount());

        LOG_OUTPUT(L"  > Opening popup. Expect an OnTargetConnected call.");
        parentlessPopup->IsOpen = true;
        VERIFY_ARE_EQUAL(1, customBackdrop->GetConnectedCount());
        VERIFY_ARE_EQUAL(parentlessPopupICSSB, customBackdrop->GetConnectedTarget());
        VERIFY_ARE_EQUAL(0, customBackdrop->GetDisconnectedCount());

        LOG_OUTPUT(L"  > Detaching from popup. Expect an OnTargetDisconnected call.");
        parentlessPopup->SystemBackdrop = nullptr;
        VERIFY_ARE_EQUAL(1, customBackdrop->GetConnectedCount());
        VERIFY_ARE_EQUAL(1, customBackdrop->GetDisconnectedCount());
        VERIFY_ARE_EQUAL(parentlessPopupICSSB, customBackdrop->GetDisconnectedTarget());

        LOG_OUTPUT(L"  > Closing popup. This shouldn't deliver any duplicate OnTargetDisconnected calls.");
        parentlessPopup->IsOpen = false;
        VERIFY_ARE_EQUAL(1, customBackdrop->GetConnectedCount());
        VERIFY_ARE_EQUAL(1, customBackdrop->GetDisconnectedCount());
    });
    wh->WaitForIdle();
}

void SystemBackdropTests::SharedSystemBackdrop()
{
    const auto& wh = TestServices::WindowHelper;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ root;
    Popup^ popup1;
    Popup^ popup2;
    MenuFlyout^ menuFlyout1;
    MenuFlyout^ menuFlyout2;
    MenuFlyoutSubItem^ nestedMenu;
    CommandBarFlyout^ commandBarFlyout1;
    CommandBarFlyout^ commandBarFlyout2;
    CustomBackdrop_TrackOnConnected^ customBackdrop;
    LOG_OUTPUT(L">>> Creating tree.");
    RunOnUIThread([&]()
    {
        Canvas^ canvas;

        canvas = ref new Canvas();
        canvas->Width = 150;
        canvas->Height = 150;
        canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Green);

        LOG_OUTPUT(L"  > Creating Popups.");
        popup1 = ref new Popup();
        popup1->ShouldConstrainToRootBounds = false;
        popup1->Child = canvas;

        canvas = ref new Canvas();
        canvas->Width = 125;
        canvas->Height = 125;
        canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Lime);

        popup2 = ref new Popup();
        popup2->ShouldConstrainToRootBounds = false;
        popup2->Child = canvas;

        root = ref new Canvas();
        root->Children->Append(popup1);
        root->Children->Append(popup2);
        wh->WindowContent = root;

        LOG_OUTPUT(L"> Creating MenuFlyouts.");
        MenuFlyoutItem^ nestedMenuItem = ref new MenuFlyoutItem();
        nestedMenuItem->Text = "Nested menu item";

        nestedMenu = ref new MenuFlyoutSubItem();
        nestedMenu->Text = L"Open nested menu";
        nestedMenu->Items->Append(nestedMenuItem);

        menuFlyout1 = ref new MenuFlyout();
        menuFlyout1->ShouldConstrainToRootBounds = false;
        menuFlyout2 = ref new MenuFlyout();
        menuFlyout2->ShouldConstrainToRootBounds = false;
        menuFlyout2->Items->Append(nestedMenu);

        LOG_OUTPUT(L"> Creating CommandBarFlyouts.");
        commandBarFlyout1 = ref new CommandBarFlyout();
        commandBarFlyout1->ShouldConstrainToRootBounds = false;
        commandBarFlyout2 = ref new CommandBarFlyout();
        commandBarFlyout2->ShouldConstrainToRootBounds = false;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        ICompositionSupportsSystemBackdrop^ popup1ICSSB = dynamic_cast<ICompositionSupportsSystemBackdrop^>(popup1);
        ICompositionSupportsSystemBackdrop^ popup2ICSSB = dynamic_cast<ICompositionSupportsSystemBackdrop^>(popup2);

        LOG_OUTPUT(L"  > Creating custom SystemBackdrop.");
        customBackdrop = ref new CustomBackdrop_TrackOnConnected();

        LOG_OUTPUT(L"  > Opening popup1 and attaching SystemBackdrop. Expect an OnTargetConnected call.");
        popup1->IsOpen = true;
        popup1->SystemBackdrop = customBackdrop;
        XamlRoot^ popupXamlRoot = customBackdrop->GetXamlRoot();    // Both popups reference same XamlRoot throughout test
        VERIFY_ARE_EQUAL(1, customBackdrop->GetConnectedCount());
        VERIFY_ARE_EQUAL(popup1ICSSB, customBackdrop->GetConnectedTarget());
        VERIFY_ARE_EQUAL(0, customBackdrop->GetDisconnectedCount());

        LOG_OUTPUT(L"  > Opening popup2 and attaching SystemBackdrop. Expect an OnTargetConnected call.");
        popup2->IsOpen = true;
        popup2->SystemBackdrop = customBackdrop;
        VERIFY_ARE_EQUAL(2, customBackdrop->GetConnectedCount());
        VERIFY_ARE_EQUAL(popup2ICSSB, customBackdrop->GetConnectedTarget());
        VERIFY_ARE_EQUAL(0, customBackdrop->GetDisconnectedCount());

        LOG_OUTPUT(L"  > Checking for separate config objects.");
        SystemBackdropConfiguration^ popup1Configuration = customBackdrop->GetDefaultSystemBackdropConfiguration(popup1ICSSB, popupXamlRoot);
        SystemBackdropConfiguration^ popup2Configuration = customBackdrop->GetDefaultSystemBackdropConfiguration(popup2ICSSB, popupXamlRoot);
        VERIFY_IS_NOT_NULL(popup1Configuration);
        VERIFY_IS_NOT_NULL(popup2Configuration);
        VERIFY_ARE_NOT_EQUAL(popup1Configuration, popup2Configuration);

        LOG_OUTPUT(L"  > Detaching from popup1. Expect an OnTargetDisconnected call.");
        popup1->SystemBackdrop = nullptr;
        VERIFY_ARE_EQUAL(2, customBackdrop->GetConnectedCount());
        VERIFY_ARE_EQUAL(1, customBackdrop->GetDisconnectedCount());
        VERIFY_ARE_EQUAL(popup1ICSSB, customBackdrop->GetDisconnectedTarget());

        LOG_OUTPUT(L"  > Checking for deleted config object.");
        SystemBackdropConfiguration^ popup1DiscardedConfiguration = customBackdrop->GetDefaultSystemBackdropConfiguration(popup1ICSSB, popupXamlRoot);
        VERIFY_IS_NULL(popup1DiscardedConfiguration);

        LOG_OUTPUT(L"  > Reattaching to popup1. Expect an OnTargetConnected call.");
        popup1->SystemBackdrop = customBackdrop;
        VERIFY_ARE_EQUAL(3, customBackdrop->GetConnectedCount());
        VERIFY_ARE_EQUAL(popup1ICSSB, customBackdrop->GetConnectedTarget());
        VERIFY_ARE_EQUAL(1, customBackdrop->GetDisconnectedCount());

        LOG_OUTPUT(L"  > Checking for new config object.");
        SystemBackdropConfiguration^ newPopup1Configuration = customBackdrop->GetDefaultSystemBackdropConfiguration(popup1ICSSB, popupXamlRoot);
        VERIFY_IS_NOT_NULL(newPopup1Configuration);
        VERIFY_ARE_NOT_EQUAL(newPopup1Configuration, popup1Configuration);
        VERIFY_ARE_NOT_EQUAL(newPopup1Configuration, popup2Configuration);

        LOG_OUTPUT(L"  > Attaching to flyouts. Expect no OnConnected call because the flyouts aren't open yet.");
        menuFlyout1->SystemBackdrop = customBackdrop;
        menuFlyout2->SystemBackdrop = customBackdrop;
        commandBarFlyout1->SystemBackdrop = customBackdrop;
        commandBarFlyout2->SystemBackdrop = customBackdrop;

        LOG_OUTPUT(L"  > Detaching from popup2. Expect an OnTargetDisconnected call.");
        popup2->SystemBackdrop = nullptr;
        VERIFY_ARE_EQUAL(3, customBackdrop->GetConnectedCount());
        VERIFY_ARE_EQUAL(2, customBackdrop->GetDisconnectedCount());
        VERIFY_ARE_EQUAL(popup2ICSSB, customBackdrop->GetDisconnectedTarget());

        LOG_OUTPUT(L"  > Detaching from popup1. Expect an OnTargetDisconnected call.");
        popup1->SystemBackdrop = nullptr;
        VERIFY_ARE_EQUAL(3, customBackdrop->GetConnectedCount());
        VERIFY_ARE_EQUAL(3, customBackdrop->GetDisconnectedCount());
        VERIFY_ARE_EQUAL(popup1ICSSB, customBackdrop->GetDisconnectedTarget());

        LOG_OUTPUT(L"  > Open flyouts. Expect OnConnected calls as the flyouts open.");
        menuFlyout1->ShowAt(root);
        VERIFY_ARE_EQUAL(4, customBackdrop->GetConnectedCount());

        LOG_OUTPUT(L"  > Closing popups.");
        popup1->IsOpen = false;
        popup2->IsOpen = false;
    });
    wh->WaitForIdle();
    // Only one flyout is allowed to open at a time, and a flyout is considered fully closed after
    // FlyoutBase::OnPresenterUnloaded, so use FlyoutHelper which waits for everything to be completed.
    FlyoutHelper::HideFlyout(menuFlyout1);

    RunOnUIThread([&]()
    {
        menuFlyout2->ShowAt(root);
        VERIFY_ARE_EQUAL(5, customBackdrop->GetConnectedCount());

        nestedMenu->Focus(FocusState::Keyboard);
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"  > Expanding submenu. Expect OnConnected call from the submenu.");
    CommonInputHelper::Right(InputDevice::Keyboard);
    VERIFY_ARE_EQUAL(6, customBackdrop->GetConnectedCount());
    FlyoutHelper::HideFlyout(menuFlyout2);

    RunOnUIThread([&]()
    {
        commandBarFlyout1->ShowAt(root);
        VERIFY_ARE_EQUAL(7, customBackdrop->GetConnectedCount());
    });
    wh->WaitForIdle();
    FlyoutHelper::HideFlyout(commandBarFlyout1);

    RunOnUIThread([&]()
    {
        commandBarFlyout2->ShowAt(root);
        VERIFY_ARE_EQUAL(8, customBackdrop->GetConnectedCount());
    });
    wh->WaitForIdle();
    FlyoutHelper::HideFlyout(commandBarFlyout2);
}

void SystemBackdropTests::DefaultConfiguration()
{
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    const auto& wh = TestServices::WindowHelper;

    Canvas^ root;
    Popup^ parentedPopup1;
    CustomBackdrop_TrackOnConnected^ customBackdrop;

    LOG_OUTPUT(L">>> Testing theme via Default Configuration on Popup.");
    RunOnUIThread([&]()
    {
        Canvas^ canvas;

        canvas = ref new Canvas();
        canvas->Width = 150;
        canvas->Height = 150;
        canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Green);

        LOG_OUTPUT(L"  > Creating default configuration.");
        parentedPopup1 = ref new Popup();
        parentedPopup1->ShouldConstrainToRootBounds = false;
        parentedPopup1->Child = canvas;

        root = ref new Canvas();
        root->Children->Append(parentedPopup1);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Opening popup.");
        parentedPopup1->IsOpen = true;
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Creating custom SystemBackdrop.");
        customBackdrop = ref new CustomBackdrop_TrackOnConnected();

        LOG_OUTPUT(L"\n> Attach-ChangeTheme(Light)-Detach-Attach-ChangeTheme(Dark)");

        LOG_OUTPUT(L"  > Attaching to popup1 and change Theme to Light.");
        parentedPopup1->SystemBackdrop = customBackdrop;
        VERIFY_ARE_EQUAL(0, customBackdrop->GetChangedCount());
        parentedPopup1->RequestedTheme = xaml::ElementTheme::Light;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Validate Light reflected in IXP Controller.");
        VERIFY_ARE_EQUAL(ixp::SystemBackdrops::SystemBackdropTheme::Light, customBackdrop->GetConfiguration_Theme());
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Detaching from popup1. Expect an OnTargetDisconnected call.");
        parentedPopup1->SystemBackdrop = nullptr;

        LOG_OUTPUT(L"  > Reattaching to popup1. Expect an OnTargetConnected call.");
        parentedPopup1->SystemBackdrop = customBackdrop;

        LOG_OUTPUT(L"  > Validate Light still reflected in IXP controller.");
        VERIFY_ARE_EQUAL(ixp::SystemBackdrops::SystemBackdropTheme::Light, customBackdrop->GetConfiguration_Theme());

        LOG_OUTPUT(L"  > Change Theme to Dark.");
        parentedPopup1->RequestedTheme = xaml::ElementTheme::Dark;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Validate Dark reflected in IXP Controller.");
        VERIFY_ARE_EQUAL(ixp::SystemBackdrops::SystemBackdropTheme::Dark, customBackdrop->GetConfiguration_Theme());

        LOG_OUTPUT(L"  > Validate we had 2 OnDefaultSystemBackdropConfigurationChanged() calls.");
        VERIFY_ARE_EQUAL(2, customBackdrop->GetChangedCount());
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"\n> ChangeHighContrast(True)-Detach-Attach-ChangeHighContrast(False)");

        LOG_OUTPUT(L"  > Set high contrast theme to Black.");
        TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::Black;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Validate IsHighContrast = True reflected in IXP Controller.");
        VERIFY_ARE_EQUAL(true, customBackdrop->GetConfiguration_IsHighContrast());
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Detaching from popup1.");
        parentedPopup1->SystemBackdrop = nullptr;

        LOG_OUTPUT(L"  > Reattaching to popup1.");
        parentedPopup1->SystemBackdrop = customBackdrop;

        LOG_OUTPUT(L"  > Validate IsHighContrast = True still reflected in IXP Controller.");
        VERIFY_ARE_EQUAL(true, customBackdrop->GetConfiguration_IsHighContrast());

        LOG_OUTPUT(L"  > Turn off high contrast theme.");
        TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::None;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Validate IsHighContrast = False reflected in IXP Controller.");
        VERIFY_ARE_EQUAL(false, customBackdrop->GetConfiguration_IsHighContrast());

        LOG_OUTPUT(L"  > Validate we had 4 OnDefaultSystemBackdropConfigurationChanged() calls.");
        VERIFY_ARE_EQUAL(4, customBackdrop->GetChangedCount());
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Closing popup.");
        parentedPopup1->IsOpen = false;
    });
}

ref class CustomBackdrop_BackedByController sealed : public SystemBackdrop
{
public:
    CustomBackdrop_BackedByController()
    {
        m_controller = ref new DesktopAcrylicController();
        m_controller->SetSystemBackdropConfiguration(ref new SystemBackdropConfiguration());
    }

    void OnTargetConnected(ICompositionSupportsSystemBackdrop^ connectedTarget, XamlRoot^ XamlRoot) override
    {
        LOG_OUTPUT(L"    > SystemBackdrop::OnTargetConnected called.");
        __super::OnTargetConnected(connectedTarget, XamlRoot);

        Popup^ targetAsPopup = dynamic_cast<Popup^>(connectedTarget);
        VERIFY_IS_NOT_NULL(targetAsPopup);  // For now only support being called on popups
        VERIFY_IS_TRUE(m_controller->IsSupported());
        m_controller->AddSystemBackdropTarget(connectedTarget);
    }

    void OnTargetDisconnected(ICompositionSupportsSystemBackdrop^ disconnectedTarget) override
    {
        LOG_OUTPUT(L"    > SystemBackdrop::OnTargetDisconnected called.");
        __super::OnTargetDisconnected(disconnectedTarget);

        m_controller->RemoveSystemBackdropTarget(disconnectedTarget);
    }

private:
    DesktopAcrylicController^ m_controller;
};

void SystemBackdropTests::VerifyPopupHelper(WindowHelper^ wh, Popup^ popup, bool expectedIsWindowedPopupOpen, bool expectedIsPlacementVisualParented)
{
    if (expectedIsWindowedPopupOpen)
    {
        if (expectedIsPlacementVisualParented)
        {
            RunOnUIThread([&]()
            {
                VisualTreeVerifier::CreateFromElement(popup)
                    ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::WindowedPopup_AnimationRootVisual))
                    ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::WindowedPopup_SystemBackdropPlacementVisual));
            });
        }
        else
        {
            RunOnUIThread([&]()
            {
                VisualTreeVerifier::CreateFromElement(popup)
                    ->WalkToTaggedChild(static_cast<UINT>(VisualDebugTags::WindowedPopup_AnimationRootVisual))
                    ->VerifyNoTaggedChild(static_cast<UINT>(VisualDebugTags::WindowedPopup_SystemBackdropPlacementVisual));
            });
        }
    }
    else
    {
        auto visuals = ref new Platform::Collections::Vector<Object^>();
        wh->GetElementRenderedVisuals(popup, visuals);
        VERIFY_IS_TRUE(visuals->Size == 0);
    }
}

void SystemBackdropTests::AddRemoveSystemBackdropTarget()
{
    const auto& wh = TestServices::WindowHelper;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ root;
    Popup^ parentedPopup;
    Popup^ parentlessPopup;
    CustomBackdrop_BackedByController^ customBackdrop;

    LOG_OUTPUT(L"> Creating tree.");
    RunOnUIThread([&]()
    {
        Canvas^ canvas;

        canvas = ref new Canvas();
        canvas->Width = 150;
        canvas->Height = 150;
        canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Green);

        LOG_OUTPUT(L"  > Creating popups.");
        parentedPopup = ref new Popup();
        parentedPopup->ShouldConstrainToRootBounds = false;
        parentedPopup->Child = canvas;

        root = ref new Canvas();
        root->Children->Append(parentedPopup);
        wh->WindowContent = root;

        canvas = ref new Canvas();
        canvas->Width = 125;
        canvas->Height = 125;
        canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Lime);

        parentlessPopup = ref new Popup();
        parentlessPopup->ShouldConstrainToRootBounds = false;
        parentlessPopup->Child = canvas;
        parentlessPopup->XamlRoot = parentedPopup->XamlRoot;

        LOG_OUTPUT(L"  > Creating custom SystemBackdrop.");
        customBackdrop = ref new CustomBackdrop_BackedByController();
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"\n> Attach-Open-Detach-Close");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Attaching to popup. There should be no popup visual tree yet.");
        parentedPopup->SystemBackdrop = customBackdrop;
    });
    wh->WaitForIdle();
    VerifyPopupHelper(wh, parentedPopup, false /* expectedIsWindowedPopupOpen */, false /* expectedIsPlacementVisualParented */);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Opening popup. There should be a popup visual tree with the placement visual attached.");
        parentedPopup->IsOpen = true;
    });
    wh->WaitForIdle();
    Sleep(100);     // Have the test thread wait - there's a worker thread that queues calling Xaml's put_SystemBackdrop
    VerifyPopupHelper(wh, parentedPopup, true /* expectedIsWindowedPopupOpen */, true /* expectedIsPlacementVisualParented */);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Detaching from popup. The popup visual tree should no longer have the placement visual attached.");
        parentedPopup->SystemBackdrop = nullptr;
    });
    wh->WaitForIdle();
    Sleep(100);     // Have the test thread wait - there's a worker thread that queues calling Xaml's put_SystemBackdrop
    VerifyPopupHelper(wh, parentedPopup, true /* expectedIsWindowedPopupOpen */, false /* expectedIsPlacementVisualParented */);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Closing popup. There should be no popup visual tree anymore.");
        parentedPopup->IsOpen = false;
    });
    wh->WaitForIdle();
    VerifyPopupHelper(wh, parentedPopup, false /* expectedIsWindowedPopupOpen */, false /* expectedIsPlacementVisualParented */);

    LOG_OUTPUT(L"\n> Open-Attach-Close-Detach");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Opening popup. There should be a popup visual tree without the placement visual attached.");
        parentlessPopup->IsOpen = true;
    });
    wh->WaitForIdle();
    VerifyPopupHelper(wh, parentlessPopup, true /* expectedIsWindowedPopupOpen */, false /* expectedIsPlacementVisualParented */);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Attaching to popup. The popup visual tree should now have the placement visual attached.");
        parentlessPopup->SystemBackdrop = customBackdrop;
    });
    wh->WaitForIdle();
    Sleep(100);     // Have the test thread wait - there's a worker thread that queues calling Xaml's put_SystemBackdrop
    VerifyPopupHelper(wh, parentlessPopup, true /* expectedIsWindowedPopupOpen */, true /* expectedIsPlacementVisualParented */);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Closing popup. There should be no popup visual tree anymore.");
        parentlessPopup->IsOpen = false;
    });
    wh->WaitForIdle();
    VerifyPopupHelper(wh, parentlessPopup, false /* expectedIsWindowedPopupOpen */, false /* expectedIsPlacementVisualParented */);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Detaching from popup. There should still be no popup visual tree.");
        parentlessPopup->SystemBackdrop = nullptr;
    });
    wh->WaitForIdle();
    VerifyPopupHelper(wh, parentlessPopup, false /* expectedIsWindowedPopupOpen */, false /* expectedIsPlacementVisualParented */);
}

// Copied from MenuFlyoutIntegrationTests
xaml_controls::MenuFlyoutPresenter^ GetMenuFlyoutPresenter(xaml_controls::MenuFlyout^ menuFlyout)
{
    VERIFY_IS_TRUE(menuFlyout->Items->Size > 0);
    auto item = dynamic_cast<xaml::DependencyObject^>(menuFlyout->Items->GetAt(0));
    VERIFY_IS_NOT_NULL(item);

    return TreeHelper::FindAncestor<xaml_controls::MenuFlyoutPresenter^>(item);
}

void SystemBackdropTests::MenuFlyoutBackdrop()
{
    const auto& wh = TestServices::WindowHelper;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ root;
    MenuFlyout^ menuFlyout;
    MenuFlyoutPresenter^ menuFlyoutPresenter;

    CustomBackdrop_TrackOnConnected^ customBackdropFlyout;
    CustomBackdrop_TrackOnConnected^ customBackdropPresenter;

    LOG_OUTPUT(L">>> Creating tree.");
    RunOnUIThread([&]()
    {
        root = ref new Canvas();
        wh->WindowContent = root;

        LOG_OUTPUT(L"> Creating MenuFlyout.");
        MenuFlyoutItem^ menuItem = ref new MenuFlyoutItem();
        menuItem->Text = L"Menu item";

        menuFlyout = ref new MenuFlyout();
        menuFlyout->ShouldConstrainToRootBounds = false;
        menuFlyout->Items->Append(menuItem);

        LOG_OUTPUT(L"  > Creating custom SystemBackdrop.");
        customBackdropFlyout = ref new CustomBackdrop_TrackOnConnected();
        customBackdropPresenter = ref new CustomBackdrop_TrackOnConnected();
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Open flyout to make sure it has a presenter.");
        menuFlyout->ShowAt(root);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Getting presenter.");
        menuFlyoutPresenter = GetMenuFlyoutPresenter(menuFlyout);
    });
    wh->WaitForIdle();
    FlyoutHelper::HideFlyout(menuFlyout);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Setting SystemBackdrop on the presenter.");
        menuFlyoutPresenter->SystemBackdrop = customBackdropPresenter;

        LOG_OUTPUT(L"  > Opening flyout. Expecting the SystemBackdrop on the presenter to be used.");
        menuFlyout->ShowAt(root);
        VERIFY_ARE_EQUAL(0, customBackdropFlyout->GetConnectedCount());
        VERIFY_ARE_EQUAL(1, customBackdropPresenter->GetConnectedCount());
    });
    wh->WaitForIdle();
    FlyoutHelper::HideFlyout(menuFlyout);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Setting SystemBackdrop on the flyout.");
        menuFlyout->SystemBackdrop = customBackdropFlyout;

        LOG_OUTPUT(L"  > Opening flyout. Expecting the SystemBackdrop on the flyout to be used.");
        menuFlyout->ShowAt(root);
        VERIFY_ARE_EQUAL(1, customBackdropFlyout->GetConnectedCount());
        // The popup will open with the old SystemBackdrop (the one on the presenter). After the popup opens we update
        // it with the new SystemBackdrop (the one on the flyout), so the presenter SystemBackdrop will get another
        // OnConnected called on it.
        VERIFY_ARE_EQUAL(2, customBackdropPresenter->GetConnectedCount());
    });
    wh->WaitForIdle();
    FlyoutHelper::HideFlyout(menuFlyout);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Removing SystemBackdrop on the flyout.");
        menuFlyout->SystemBackdrop = nullptr;

        LOG_OUTPUT(L"  > Opening flyout. Expecting the SystemBackdrop on the presenter to be used again.");
        menuFlyout->ShowAt(root);
        // The popup will open with the old SystemBackdrop (the one on the flyout). After the popup opens we update
        // it with the new SystemBackdrop (the one on the presenter), so the flyout SystemBackdrop will get another
        // OnConnected called on it.
        VERIFY_ARE_EQUAL(2, customBackdropFlyout->GetConnectedCount());
        VERIFY_ARE_EQUAL(3, customBackdropPresenter->GetConnectedCount());
    });
    wh->WaitForIdle();
    FlyoutHelper::HideFlyout(menuFlyout);
}

void SystemBackdropTests::ChangeWindowContent()
{
    const auto& wh = TestServices::WindowHelper;

    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

    WindowAutoCloser window1;

    CustomBackdrop_TrackOnConnected^ customBackdrop;

    LOG_OUTPUT(L">>> Creating tree.");
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating Window with content element requesting Dark theme.");

        window1.Attach(safe_cast<xaml::Window^> (xaml_markup::XamlReader::Load(
            L"<Window xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"  >"
            L"  <StackPanel x:Name='rootPanel' RequestedTheme='Dark'>"
            L"    <Button x:Name='button' />"
            L"  </StackPanel>"
            L"</Window>")));

        window1->Activate();

        LOG_OUTPUT(L"  > Creating custom SystemBackdrop.");
        customBackdrop = ref new CustomBackdrop_TrackOnConnected();
        window1->SystemBackdrop = customBackdrop;
    });
    wh->WaitForIdle();
    RunOnUIThread([&]()
    {
        VERIFY_ARE_EQUAL(0, customBackdrop->GetChangedCount());
    });
    wh->WaitForIdle();

    UIElement^ newContent;
    LOG_OUTPUT(L">>>Creating new Content requesting Light theme.");
    RunOnUIThread([&]()
    {
        newContent = safe_cast<UIElement^> (xaml_markup::XamlReader::Load(
            L"<Grid x:Name='newContentGrid' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"  >"
            L"  <Button x:Name='button' />"
            L"</Grid>"));
        FrameworkElement^ newContentAsFE = dynamic_cast<FrameworkElement^>(newContent);

        XamlRoot^ xamlRoot = customBackdrop->GetXamlRoot();
        FrameworkElement^ xamlRootContentAsFE = dynamic_cast<FrameworkElement^>(xamlRoot->Content);

        newContentAsFE->RequestedTheme = xaml::ElementTheme::Light;
        window1->Content = newContentAsFE;
    });
    wh->WaitForIdle();
    RunOnUIThread([&]()
    {
        VERIFY_ARE_EQUAL(1, customBackdrop->GetChangedCount());
    });
    wh->WaitForIdle();

}

}}}}}}
