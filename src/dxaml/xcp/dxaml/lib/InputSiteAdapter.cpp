// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <windows.ui.viewmanagement.h>
#include "JupiterWindow.h"
#include "JupiterControl.h"
#include <windows.ui.core.h>
#include "Window.g.h"
#include "CoreWindowWrapper.h"
#include "DropOperationTarget.h"
#include "FrameworkApplication_Partial.h"
#include "DragDropInternal.h"
#include "focusmgr.h"
#include <AKExport.h>
#include "TouchHitTestingHandler.h"
#include "InternalDebugInteropModel.h"
#include <winpal.h>
#include <CoreWindow.h>
#include <FeatureFlags.h>
#include <DXamlServices.h>
#include <Microsoft.UI.Content.Private.h>

#include <XamlTraceLogging.h>

#include <InputServices.h>
#include <KeyboardUtility.h>
#include <TextBoxBase.h>
#include <DesktopUtility.h>

#include "TextCommon.h"
#include "MUX-ETWEvents.h"
#include "KeyboardAcceleratorUtility.h"
#include <RootScale.h>

#include "FocusObserver.h"

#include <windows.ui.core.corewindow-defs.h>

#include "WrlHelper.h"
#include "WindowRenderTarget.h"
#include "WindowsGraphicsDeviceManager.h"
#include "DcompTreeHost.h"
#include "InputSiteAdapter.h"
#include "PreTranslateHandler.h"
#include "LoadLibraryAbs.h"

InputSiteAdapter::InputSiteAdapter()
{

}

InputSiteAdapter::~InputSiteAdapter()
{
    if (m_isIslandInputSiteRegisteredWithInputServicesForDManip)
    {
        // If we were registered with input services we MUST have an island input site.
        FAIL_FAST_IF(nullptr == m_islandInputSite);

        // Before destruction we must unregister with input services for proper cleanup.
        UnregisterIslandInputSiteWithInputServicesForDManip();
        m_islandInputSite = nullptr;
    }

    UnsubscribeToInputEvents();
    m_jupiterWindow = nullptr;
}

void InputSiteAdapter::Initialize(_In_ ixp::IContentIsland* contentIsland, _In_ CContentRoot* contentRoot, _In_ CJupiterWindow* jupiterWindow, bool connectActivationListener)
{
    m_jupiterWindow = jupiterWindow;
    m_contentRootWeak = xref::get_weakref(contentRoot);
    InitInputObjects(contentIsland);
    SubscribeToInputKeyboardSourceEvents();
    SubscribeToInputPointerSourceEvents();
    RegisterIslandInputSiteWithInputServicesForDManip();

    // InputActivationListener
    if (connectActivationListener && !m_activationChangedToken.value)
    {
        IFCFAILFAST(m_inputActivationListener->add_InputActivationChanged(
            WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::InputActivationListener*, ixp::InputActivationListenerActivationChangedEventArgs*>>(
                [contentRootWeak=m_contentRootWeak](ixp::IInputActivationListener* inputActivationListener, ixp::IInputActivationListenerActivationChangedEventArgs* args) -> HRESULT
        {
            auto contentRootStrong = contentRootWeak.lock();
            if (contentRootStrong)
            {
                ixp::InputActivationState state = {};
                IFC_RETURN(inputActivationListener->get_State(&state));
                contentRootStrong->SetIsInputActive(ixp::InputActivationState::InputActivationState_Activated == state);
            }
            return S_OK;
        }).Get(), &m_activationChangedToken));

        if (nullptr != contentRoot)
        {
            ixp::InputActivationState state = {};
            IFCFAILFAST(m_inputActivationListener->get_State(&state));
            contentRoot->SetIsInputActive(ixp::InputActivationState::InputActivationState_Activated == state);
        }
    }
}

void InputSiteAdapter::InitInputObjects(_In_ ixp::IContentIsland* const contentIsland)
{
    typedef HRESULT(WINAPI* PFN_GetActivationFactory)(HSTRING activatableClassId, IActivationFactory** ppFactory);
    PFN_GetActivationFactory pfnGetActivationFactory = nullptr;

    wil::unique_hmodule inputModule;
    inputModule.reset(LoadLibraryExWAbs(L"Microsoft.UI.Input.dll", nullptr, LOAD_WITH_ALTERED_SEARCH_PATH));
    ASSERT(inputModule != nullptr, L"error: NULL handle to module Microsoft.UI.Input.dll!");

    pfnGetActivationFactory = (PFN_GetActivationFactory)GetProcAddress(inputModule.get(), "DllGetActivationFactory");
    ASSERT(pfnGetActivationFactory != nullptr, L"error: nullptr to pfnGetActivationFactory!");

    // IIslandInputSitePartner
    {
        // DManip initialization requires the ContentIsland's IslandInputSite.
        ComPtr<ixp::IContentIsland> island{ contentIsland };
        ComPtr<ixp::IContentIslandPartner> contentIslandPartner;
        IFCFAILFAST(island.As(&contentIslandPartner));
        IFCFAILFAST(contentIslandPartner->get_IslandInputSite(&m_islandInputSite));
    }

    // InputFocusController
    {
        ComPtr<ixp::IInputFocusControllerStatics> inputFocusControllerStatics;
        ComPtr<IActivationFactory> factory;
        pfnGetActivationFactory(Microsoft::WRL::Wrappers::HStringReference(RuntimeClass_Microsoft_UI_Input_InputFocusController).Get(), &factory);
        IFCFAILFAST(factory.As(&inputFocusControllerStatics));
        IFCFAILFAST(inputFocusControllerStatics->GetForIsland(contentIsland, &m_inputFocusController));
    }

    // InputKeyboardSource
    ComPtr<ixp::IInputKeyboardSourceStatics2> inputKeyboardSourceStatics2;
    {
        ComPtr<IActivationFactory> factory;
        pfnGetActivationFactory(Microsoft::WRL::Wrappers::HStringReference(RuntimeClass_Microsoft_UI_Input_InputKeyboardSource).Get(), &factory);
        IFCFAILFAST(factory.As(&inputKeyboardSourceStatics2));
        ComPtr<ixp::IInputKeyboardSource> inputKeyboardSource;
        IFCFAILFAST(inputKeyboardSourceStatics2->GetForIsland(contentIsland, &inputKeyboardSource));
        IFCFAILFAST(inputKeyboardSource.As(&m_inputKeyboardSource2));
    }

    // InputPreTranslateKeyboardSource
    {
        ComPtr<ixp::IInputPreTranslateKeyboardSourceStatics> inputPreTranslateKeyboardSourceStatics;
        ComPtr<IActivationFactory> factory;
        pfnGetActivationFactory(Microsoft::WRL::Wrappers::HStringReference(RuntimeClass_Microsoft_UI_Input_InputPreTranslateKeyboardSource).Get(), &factory);
        IFCFAILFAST(factory.As(&inputPreTranslateKeyboardSourceStatics));
        ComPtr<ixp::IInputPreTranslateKeyboardSource> inputPreTranslateKeyboardSource;
        IFCFAILFAST(inputPreTranslateKeyboardSourceStatics->GetForIsland(contentIsland, &inputPreTranslateKeyboardSource));
        IFCFAILFAST(inputPreTranslateKeyboardSource.As(&m_inputPreTranslateKeyboardSourceInterop));
    }

    // PreTranslateHandler
    m_preTranslateHandler.Attach(new DirectUI::PreTranslateHandler<InputSiteAdapter>(this));

    // InputPointerSource
    ComPtr<ixp::IInputPointerSourceStatics> inputPointerSourceStatics;
    {
        ComPtr<IActivationFactory> factory;
        pfnGetActivationFactory(Microsoft::WRL::Wrappers::HStringReference(RuntimeClass_Microsoft_UI_Input_InputPointerSource).Get(), &factory);
        IFCFAILFAST(factory.As(&inputPointerSourceStatics));
        IFCFAILFAST(inputPointerSourceStatics->GetForIsland(contentIsland, &m_inputPointerSource));
        IFCFAILFAST(m_inputPointerSource.As(&m_inputPointerSource2));
    }

    // InputActivationListener
    ComPtr<ixp::IInputActivationListenerStatics> inputActivationListenerStatics;
    {
        ComPtr<IActivationFactory> factory;
        pfnGetActivationFactory(Microsoft::WRL::Wrappers::HStringReference(RuntimeClass_Microsoft_UI_Input_InputActivationListener).Get(), &factory);
        IFCFAILFAST(factory.As(&inputActivationListenerStatics));
        ComPtr<ixp::IInputActivationListenerStatics2> inputActivationListenerStatics2;
        IFCFAILFAST(inputActivationListenerStatics.As(&inputActivationListenerStatics2));
        IFCFAILFAST(inputActivationListenerStatics2->GetForIsland(contentIsland, &m_inputActivationListener));
    }
}

void InputSiteAdapter::RegisterIslandInputSiteWithInputServicesForDManip()
{
    FAIL_FAST_IF(m_isIslandInputSiteRegisteredWithInputServicesForDManip);
    FAIL_FAST_IF(nullptr == m_islandInputSite);

    auto coreServices = DirectUI::DXamlServices::GetHandle();
    auto inputServices = coreServices->GetInputServices();
    inputServices->RegisterIslandInputSite(m_islandInputSite.Get());
    m_isIslandInputSiteRegisteredWithInputServicesForDManip = true;
}

void InputSiteAdapter::UnregisterIslandInputSiteWithInputServicesForDManip()
{
    ASSERT(m_isIslandInputSiteRegisteredWithInputServicesForDManip);
    FAIL_FAST_IF(nullptr == m_islandInputSite);

    // We can end up here under both orderly and unorderly shutdown.
    // That means we could either be first cleaning ourselves up before all the core services have gone down.
    // Or core services themselves have been torn down, in which case we no longer need to update input services.
    auto coreServices = DirectUI::DXamlServices::GetHandle();
    if (nullptr != coreServices)
    {
        auto inputServices = coreServices->GetInputServices();
        if (nullptr != inputServices)
        {
            inputServices->UnregisterIslandInputSite(m_islandInputSite.Get());
        }
    }

    m_isIslandInputSiteRegisteredWithInputServicesForDManip = false;
}

bool InputSiteAdapter::HasFocus() const
{
    if (nullptr != m_inputFocusController)
    {
        boolean hasFocus{ false };
        if (SUCCEEDED(m_inputFocusController->get_HasFocus(&hasFocus)))
        {
            return static_cast<bool>(hasFocus);
        }
    }

    return false;
}

HRESULT InputSiteAdapter::PreTranslateMessage(
    mui::IInputPreTranslateKeyboardSourceInterop* source,
    const MSG* msg,
    UINT keyboardModifiers,
    bool focusPass,
    bool* handled)
{
    xref_ptr<CContentRoot> contentRootStrongRef { GetContentRoot() };

    wrl::ComPtr<mui::IInputKeyboardSourceInterop> inputKeyboardSourceInterop;
    IFCFAILFAST(m_inputKeyboardSource2.As(&inputKeyboardSourceInterop));

    return DirectUI::DXamlCore::GetCurrent()->GetControl()->GetJupiterWindow()->PreTranslateMessage(
        contentRootStrongRef.get(),
        source,
        inputKeyboardSourceInterop.Get(),
        msg,
        keyboardModifiers,
        focusPass,
        handled);
}

void InputSiteAdapter::SubscribeToInputKeyboardSourceEvents()
{
    // PreTranslateHandler for accelerator support
    {
        m_inputPreTranslateKeyboardSourceInterop->SetPreTranslateHandler(m_preTranslateHandler.Get());
    }

    if (!m_characterReceivedToken.value)
    {
        IFCFAILFAST(m_inputKeyboardSource2->add_CharacterReceived(
            WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::InputKeyboardSource*, ixp::CharacterReceivedEventArgs*>>(
                [this](ixp::IInputKeyboardSource* inputKeyboardSource, ixp::ICharacterReceivedEventArgs* args) -> HRESULT
                {
                    return OnCharacterReceived(args);
                }).Get(), &m_characterReceivedToken));
    }

    if (!m_keyDownToken.value)
    {
        IFCFAILFAST(m_inputKeyboardSource2->add_KeyDown(
            WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::InputKeyboardSource*, ixp::KeyEventArgs*>>(
                [this](ixp::IInputKeyboardSource* inputKeyboardSource, ixp::IKeyEventArgs* args) -> HRESULT
                {
                    return OnKeyDown(args);
                }).Get(), &m_keyDownToken));
    }

    if (!m_keyUpToken.value)
    {
        IFCFAILFAST(m_inputKeyboardSource2->add_KeyUp(
            WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::InputKeyboardSource*, ixp::KeyEventArgs*>>(
                [this](ixp::IInputKeyboardSource* inputKeyboardSource, ixp::IKeyEventArgs* args) -> HRESULT
                {
                    return OnKeyUp(args);
                }).Get(), &m_keyUpToken));
    }

    if (!m_sysKeyDownToken.value)
    {
        IFCFAILFAST(m_inputKeyboardSource2->add_SystemKeyDown(
            WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::InputKeyboardSource*, ixp::KeyEventArgs*>>(
                [this](ixp::IInputKeyboardSource* inputKeyboardSource, ixp::IKeyEventArgs* args) -> HRESULT
                {
                    return OnSysKeyDown(args);
                }).Get(), &m_sysKeyDownToken));
    }

    if (!m_sysKeyUpToken.value)
    {
        IFCFAILFAST(m_inputKeyboardSource2->add_SystemKeyUp(
            WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::InputKeyboardSource*, ixp::KeyEventArgs*>>(
                [this](ixp::IInputKeyboardSource* inputKeyboardSource, ixp::IKeyEventArgs* args) -> HRESULT
                {
                    return OnSysKeyUp(args);
                }).Get(), &m_sysKeyUpToken));
    }

    if (!m_contextMenuToken.value)
    {
        IFCFAILFAST(m_inputKeyboardSource2->add_ContextMenuKey(
            WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::InputKeyboardSource*, ixp::ContextMenuKeyEventArgs*>>(
                [this](ixp::IInputKeyboardSource* inputKeyboardSource, ixp::IContextMenuKeyEventArgs* args) -> HRESULT
                {
                    return OnContextMenuKey(args);
                }).Get(), &m_contextMenuToken));
    }

    if (!m_gotFocusToken.value)
    {
        IFCFAILFAST(m_inputFocusController->add_GotFocus(
            WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::InputFocusController*, ixp::FocusChangedEventArgs*>>(
                [this](ixp::IInputFocusController* inputKeyboardSource, ixp::IFocusChangedEventArgs* args) -> HRESULT
                {
                    args->put_Handled(true);
                    return OnGotFocus();
                }).Get(), &m_gotFocusToken));
    }

    if (!m_lostFocusToken.value)
    {
        IFCFAILFAST(m_inputFocusController->add_LostFocus(
            WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::InputFocusController*, ixp::FocusChangedEventArgs*>>(
                [this](ixp::IInputFocusController* inputKeyboardSource, ixp::IFocusChangedEventArgs* args) -> HRESULT
                {
                    args->put_Handled(true);
                    return OnLostFocus();
                }).Get(), &m_lostFocusToken));
    }
}

void InputSiteAdapter::SubscribeToInputPointerSourceEvents()
{
    if (!m_pointerCaptureLostToken.value)
    {
        IFCFAILFAST(m_inputPointerSource->add_PointerCaptureLost(
            WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::InputPointerSource*, ixp::PointerEventArgs*>>(
                [this](ixp::IInputPointerSource* inputPointerSource, ixp::IPointerEventArgs* args) -> HRESULT
                {
                    return OnPointerCaptureLost(args);
                }).Get(), &m_pointerCaptureLostToken));
    }

    if (!m_pointerEnteredToken.value)
    {
        IFCFAILFAST(m_inputPointerSource->add_PointerEntered(
            WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::InputPointerSource*, ixp::PointerEventArgs*>>(
                [this](ixp::IInputPointerSource* inputPointerSource, ixp::IPointerEventArgs* args) -> HRESULT
                {
                    return OnPointerEntered(args);
                }).Get(), &m_pointerEnteredToken));
    }

    if (!m_pointerExitedToken.value)
    {
        IFCFAILFAST(m_inputPointerSource->add_PointerExited(
            WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::InputPointerSource*, ixp::PointerEventArgs*>>(
                [this](ixp::IInputPointerSource* inputPointerSource, ixp::IPointerEventArgs* args) -> HRESULT
                {
                    return OnPointerExited(args);
                }).Get(), &m_pointerExitedToken));
    }

    if (!m_pointerMovedToken.value)
    {
        IFCFAILFAST(m_inputPointerSource->add_PointerMoved(
            WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::InputPointerSource*, ixp::PointerEventArgs*>>(
                [this](ixp::IInputPointerSource* inputPointerSource, ixp::IPointerEventArgs* args) -> HRESULT
                {
                    return OnPointerMoved(args);
                }).Get(), &m_pointerMovedToken));
    }

    if (!m_pointerPressedToken.value)
    {
        IFCFAILFAST(m_inputPointerSource->add_PointerPressed(
            WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::InputPointerSource*, ixp::PointerEventArgs*>>(
                [this](ixp::IInputPointerSource* inputPointerSource, ixp::IPointerEventArgs* args) -> HRESULT
                {
                    return OnPointerPressed(args);
                }).Get(), &m_pointerPressedToken));
    }

    if (!m_pointerReleasedToken.value)
    {
        IFCFAILFAST(m_inputPointerSource->add_PointerReleased(
            WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::InputPointerSource*, ixp::PointerEventArgs*>>(
                [this](ixp::IInputPointerSource* inputPointerSource, ixp::IPointerEventArgs* args) -> HRESULT
                {
                    return OnPointerReleased(args);
                }).Get(), &m_pointerReleasedToken));
    }

    if (!m_pointerWheelChangedToken.value)
    {
        IFCFAILFAST(m_inputPointerSource->add_PointerWheelChanged(
            WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::InputPointerSource*, ixp::PointerEventArgs*>>(
                [this](ixp::IInputPointerSource* inputPointerSource, ixp::IPointerEventArgs* args) -> HRESULT
                {
                    return OnPointerWheelChanged(args);
                }).Get(), &m_pointerWheelChangedToken));
    }

    if (!m_pointerRoutedAwayToken.value)
    {
        IFCFAILFAST(m_inputPointerSource->add_PointerRoutedAway(
            WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::InputPointerSource*, ixp::PointerEventArgs*>>(
                [this](ixp::IInputPointerSource* inputPointerSource, ixp::IPointerEventArgs* args) -> HRESULT
                {
                    return OnPointerRoutedAway(args);
                }).Get(), &m_pointerRoutedAwayToken));
    }

    if (!m_pointerRoutedReleasedToken.value)
    {
        IFCFAILFAST(m_inputPointerSource->add_PointerRoutedReleased(
            WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::InputPointerSource*, ixp::PointerEventArgs*>>(
                [this](ixp::IInputPointerSource* inputPointerSource, ixp::IPointerEventArgs* args) -> HRESULT
                {
                    return OnPointerRoutedReleased(args);
                }).Get(), &m_pointerRoutedReleasedToken));
    }

    if (!m_pointerRoutedToToken.value)
    {
        IFCFAILFAST(m_inputPointerSource->add_PointerRoutedTo(
            WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::InputPointerSource*, ixp::PointerEventArgs*>>(
                [this](ixp::IInputPointerSource* inputPointerSource, ixp::IPointerEventArgs* args) -> HRESULT
                {
                    return OnPointerRoutedTo(args);
                }).Get(), &m_pointerRoutedToToken));
    }

    if (!m_directManipulationHitTestToken.value)
    {
        IFCFAILFAST(m_inputPointerSource2->add_DirectManipulationHitTest(
            WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::InputPointerSource*, ixp::PointerEventArgs*>>(
                [this](ixp::IInputPointerSource* inputPointerSource, ixp::IPointerEventArgs* args)->HRESULT
                {
                    return OnDirectManipulationHitTest(args);
                }).Get(), &m_directManipulationHitTestToken));
    }

    // Temporarily disable touch hit testing pending ADO 38140531.
    //
    // if (!m_touchHitTestRequestedToken.value)
    // {
    //     IFCFAILFAST(m_inputPointerSource2->add_TouchHitTesting(
    //         WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::InputPointerSource*, ixp::TouchHitTestingEventArgs*>>(
    //             [this](ixp::IInputPointerSource* inputPointerSource, ixp::ITouchHitTestingEventArgs* args) -> HRESULT
    //     {
    //         return OnTouchHitTesting(args);
    //     }).Get(), &m_touchHitTestRequestedToken));
    // }
}

void InputSiteAdapter::UnsubscribeToInputEvents()
{
    if (m_preTranslateHandler != nullptr)
    {
        m_inputPreTranslateKeyboardSourceInterop->SetPreTranslateHandler(nullptr);
        m_preTranslateHandler = nullptr;
    }

    if (m_characterReceivedToken.value != 0)
    {
        IGNOREHR(m_inputKeyboardSource2->remove_CharacterReceived(m_characterReceivedToken));
        m_characterReceivedToken.value = 0;
    }

    if (m_keyDownToken.value != 0)
    {
        IGNOREHR(m_inputKeyboardSource2->remove_KeyDown(m_keyDownToken));
        m_keyDownToken.value = 0;
    }

    if (m_keyUpToken.value != 0)
    {
        IGNOREHR(m_inputKeyboardSource2->remove_KeyUp(m_keyUpToken));
        m_keyUpToken.value = 0;
    }

    if (m_sysKeyDownToken.value != 0)
    {
        IGNOREHR(m_inputKeyboardSource2->remove_SystemKeyDown(m_sysKeyDownToken));
        m_sysKeyDownToken.value = 0;
    }

    if (m_sysKeyUpToken.value != 0)
    {
        IGNOREHR(m_inputKeyboardSource2->remove_SystemKeyUp(m_sysKeyUpToken));
        m_sysKeyUpToken.value = 0;
    }

    if (m_contextMenuToken.value != 0)
    {
        IGNOREHR(m_inputKeyboardSource2->remove_ContextMenuKey(m_contextMenuToken));
        m_contextMenuToken.value = 0;
    }

    if (m_gotFocusToken.value != 0)
    {
        IGNOREHR(m_inputFocusController->remove_GotFocus(m_gotFocusToken));
        m_gotFocusToken.value = 0;
    }

    if (m_lostFocusToken.value != 0)
    {
        IGNOREHR(m_inputFocusController->remove_LostFocus(m_lostFocusToken));
        m_lostFocusToken.value = 0;
    }

    if (m_pointerCaptureLostToken.value != 0)
    {
        IGNOREHR(m_inputPointerSource->remove_PointerCaptureLost(m_pointerCaptureLostToken));
        m_pointerCaptureLostToken.value = 0;
    }

    if (m_pointerEnteredToken.value != 0)
    {
        IGNOREHR(m_inputPointerSource->remove_PointerEntered(m_pointerEnteredToken));
        m_pointerEnteredToken.value = 0;
    }

    if (m_pointerExitedToken.value != 0)
    {
        IGNOREHR(m_inputPointerSource->remove_PointerExited(m_pointerExitedToken));
        m_pointerExitedToken.value = 0;
    }

    if (m_pointerMovedToken.value != 0)
    {
        IGNOREHR(m_inputPointerSource->remove_PointerMoved(m_pointerMovedToken));
        m_pointerMovedToken.value = 0;
    }

    if (m_pointerPressedToken.value != 0)
    {
        IGNOREHR(m_inputPointerSource->remove_PointerPressed(m_pointerPressedToken));
        m_pointerPressedToken.value = 0;
    }

    if (m_pointerReleasedToken.value != 0)
    {
        IGNOREHR(m_inputPointerSource->remove_PointerReleased(m_pointerReleasedToken));
        m_pointerReleasedToken.value = 0;
    }

    if (m_pointerRoutedAwayToken.value != 0)
    {
        IGNOREHR(m_inputPointerSource->remove_PointerRoutedAway(m_pointerRoutedAwayToken));
        m_pointerRoutedAwayToken.value = 0;
    }

    if (m_pointerRoutedReleasedToken.value != 0)
    {
        IGNOREHR(m_inputPointerSource->remove_PointerRoutedReleased(m_pointerRoutedReleasedToken));
        m_pointerRoutedReleasedToken.value = 0;
    }

    if (m_pointerRoutedToToken.value != 0)
    {
        IGNOREHR(m_inputPointerSource->remove_PointerRoutedTo(m_pointerRoutedToToken));
        m_pointerRoutedToToken.value = 0;
    }

    if (m_pointerWheelChangedToken.value != 0)
    {
        IGNOREHR(m_inputPointerSource->remove_PointerWheelChanged(m_pointerWheelChangedToken));
        m_pointerWheelChangedToken.value = 0;
    }

    if (m_touchHitTestRequestedToken.value != 0)
    {
        IGNOREHR(m_inputPointerSource2->remove_TouchHitTesting(m_touchHitTestRequestedToken));
        m_touchHitTestRequestedToken.value = 0;
    }

    if (m_directManipulationHitTestToken.value != 0)
    {
        IGNOREHR(m_inputPointerSource2->remove_DirectManipulationHitTest(m_directManipulationHitTestToken));
        m_directManipulationHitTestToken.value = 0;
    }

    if (m_activationChangedToken.value != 0)
    {
        IGNOREHR(m_inputActivationListener->remove_InputActivationChanged(m_activationChangedToken));
        m_activationChangedToken.value = 0;
    }

    m_inputFocusController = nullptr;
    m_inputKeyboardSource2 = nullptr;
    m_inputPointerSource = nullptr;
    m_inputPointerSource2 = nullptr;
    m_inputActivationListener = nullptr;
    m_inputPreTranslateKeyboardSourceInterop = nullptr;
}

_Check_return_ HRESULT InputSiteAdapter::OnCharacterReceived(
    _Inout_ ixp::ICharacterReceivedEventArgs *args)
{
    IFC_RETURN(m_jupiterWindow->ProcessCharEvents(args, GetContentRoot()));
    return S_OK;
}

_Check_return_ HRESULT InputSiteAdapter::OnKeyDown(_In_ ixp::IKeyEventArgs *pArgs)
{
    UINT32 uMsg = WM_KEYDOWN;
    ixp::PhysicalKeyStatus keyStatus;
    IFC_RETURN(pArgs->get_KeyStatus(&keyStatus));
    if (keyStatus.IsMenuKeyDown)
    {
        uMsg = WM_SYSKEYDOWN;
    }

    IFC_RETURN(m_jupiterWindow->ProcessKeyEvents(pArgs, uMsg, GetContentRoot(), false));
    return S_OK;
}

_Check_return_ HRESULT InputSiteAdapter::OnKeyUp(_In_ ixp::IKeyEventArgs *pArgs)
{
    UINT32 uMsg = WM_KEYUP;
    ixp::PhysicalKeyStatus keyStatus;
    IFC_RETURN(pArgs->get_KeyStatus(&keyStatus));
    if (keyStatus.IsMenuKeyDown)
    {
        uMsg = WM_SYSKEYUP;
    }

    IFC_RETURN(m_jupiterWindow->ProcessKeyEvents(pArgs, uMsg, GetContentRoot(), false));

    return S_OK;
}

_Check_return_ HRESULT InputSiteAdapter::OnSysKeyUp(
    _Inout_ ixp::IKeyEventArgs * pArgs)
{
   return m_jupiterWindow->ProcessKeyEvents(pArgs, WM_SYSKEYUP, GetContentRoot());
}

_Check_return_ HRESULT InputSiteAdapter::OnSysKeyDown(
    _Inout_ ixp::IKeyEventArgs * pArgs)
{
    return m_jupiterWindow->ProcessKeyEvents(pArgs, WM_SYSKEYDOWN, GetContentRoot());
}

_Check_return_ HRESULT InputSiteAdapter::OnContextMenuKey(
    _Inout_ ixp::IContextMenuKeyEventArgs* pArgs)
{
    bool handled = m_jupiterWindow->ProcessContextMenu(WM_CONTEXTMENU, 0, 0, /*wasKeyEventHandled*/ false, GetContentRoot());
    m_jupiterWindow->ResetContextMenuState();
    pArgs->put_Handled(handled);

    return S_OK;
}

_Check_return_ HRESULT InputSiteAdapter::OnGotFocus()
{
    m_jupiterWindow->ProcessFocusEvents(WM_SETFOCUS, GetContentRoot());
    // args has no put_handled method
    return S_OK;
}

_Check_return_ HRESULT InputSiteAdapter::OnLostFocus()
{
    m_jupiterWindow->ProcessFocusEvents(WM_KILLFOCUS, GetContentRoot());
    // args has no put_handled method
    return S_OK;
}

_Check_return_ HRESULT InputSiteAdapter::OnPointerCaptureLost(_In_ ixp::IPointerEventArgs* args)
{
    IFC_RETURN(OnPointerMessage(CInputServices::GetMessageFromPointerCaptureLostArgs(args), args));
    return S_OK;
}

_Check_return_ HRESULT InputSiteAdapter::OnPointerEntered(_In_ ixp::IPointerEventArgs* e)
{
    IFC_RETURN(OnPointerMessage(WM_POINTERENTER, e));
    return S_OK;
}

_Check_return_ HRESULT InputSiteAdapter::OnPointerExited(_In_ ixp::IPointerEventArgs* e)
{
    IFC_RETURN(OnPointerMessage(WM_POINTERLEAVE, e));
    return S_OK;
}

_Check_return_ HRESULT InputSiteAdapter::OnPointerMoved(_In_ ixp::IPointerEventArgs* e)
{
    IFC_RETURN(OnPointerMessage(WM_POINTERUPDATE, e));
    return S_OK;
}

_Check_return_ HRESULT InputSiteAdapter::OnPointerPressed(_In_ ixp::IPointerEventArgs* e)
{
    IFC_RETURN(OnPointerMessage(WM_POINTERDOWN, e));
    return S_OK;
}

_Check_return_ HRESULT InputSiteAdapter::OnPointerReleased(_In_ ixp::IPointerEventArgs* e)
{
    IFC_RETURN(OnPointerMessage(WM_POINTERUP, e));
    return S_OK;
}

_Check_return_ HRESULT InputSiteAdapter::OnPointerWheelChanged(_In_ ixp::IPointerEventArgs* e)
{
    IFC_RETURN(OnPointerMessage(WM_POINTERWHEEL, e));
    return S_OK;
}

_Check_return_ HRESULT InputSiteAdapter::OnPointerRoutedAway(_In_ ixp::IPointerEventArgs* e)
{
    IFC_RETURN(OnPointerMessage(WM_POINTERROUTEDAWAY, e));
    return S_OK;
}

_Check_return_ HRESULT InputSiteAdapter::OnPointerRoutedReleased(_In_ ixp::IPointerEventArgs* e)
{
    IFC_RETURN(OnPointerMessage(WM_POINTERROUTEDRELEASED, e));
    return S_OK;
}

_Check_return_ HRESULT InputSiteAdapter::OnPointerRoutedTo(_In_ ixp::IPointerEventArgs* e)
{
    IFC_RETURN(OnPointerMessage(WM_POINTERROUTEDTO, e));
    return S_OK;
}

_Check_return_ HRESULT InputSiteAdapter::OnDirectManipulationHitTest(_In_ ixp::IPointerEventArgs* args)
{
    IFC_RETURN(m_jupiterWindow->OnIslandDirectManipulationHitTest(GetContentRoot(), args));
    return S_OK;
}

_Check_return_ HRESULT InputSiteAdapter::OnPointerMessage(const UINT uMsg, _In_ ixp::IPointerEventArgs* args)
{
    wrl::ComPtr<ixp::IPointerPoint> pointerPoint;
    IFCFAILFAST(args->get_CurrentPoint(&pointerPoint));

    UpdateLastPointerPointForReplay(uMsg, pointerPoint.Get(), args);

    IFC_RETURN(m_jupiterWindow->OnCoreWindowPointerMessage(uMsg, args));

    return S_OK;
}

void InputSiteAdapter::ClearLastPointerPointForReplay()
{
    m_previousPointerPoint = nullptr;
    m_previousPointerEventArgs = nullptr;
}

void InputSiteAdapter::UpdateLastPointerPointForReplay(const UINT uMsg, _In_ ixp::IPointerPoint* pointerPoint, _In_ ixp::IPointerEventArgs* pointerEventArgs)
{
    // Clear any input cached by any open windowed popups and the CXamlIslandRoot. Those are no longer the most recent -
    // the most recent input is the one that we're about to cache here.
    CContentRoot* contentRoot = GetContentRoot();
    if (contentRoot && contentRoot->GetXamlIslandRootNoRef())   // UWPs use an InputSiteAdapter too
    {
        contentRoot->GetXamlIslandRootNoRef()->ClearLastPointerPointForReplay();
        contentRoot->GetVisualTreeNoRef()->GetPopupRoot()->ClearLastPointerPointForReplay();
    }

    mui::PointerDeviceType pointerDeviceType;
    IFCFAILFAST(pointerPoint->get_PointerDeviceType(&pointerDeviceType));

    // Save off the last pointer message so we can replay it.
    // Note that we will replay pointer down/up as updates.  This allows us to handle the case where there is
    // slight movement between the last update and the release as well as if the down/up cause
    // changes in the scene (e.g. clicking a button deletes that button).
    // Also only certain types of input (e.g. mouse) can be replayed.  Only save off the replay mesage if
    // we can replay it.
    if ((uMsg == WM_POINTERUPDATE ||
        uMsg == WM_POINTERDOWN ||
        uMsg == WM_POINTERUP) &&
        pointerDeviceType == mui::PointerDeviceType_Mouse)
    {
        m_previousPointerPoint = pointerPoint;
        m_previousPointerEventArgs = pointerEventArgs;
    }
    else if (uMsg == WM_POINTERLEAVE ||
        uMsg == WM_POINTERCAPTURECHANGED ||
        pointerDeviceType != mui::PointerDeviceType_Mouse)
    {
        m_previousPointerPoint = nullptr;
        m_previousPointerEventArgs = nullptr;
    }
}

_Check_return_ HRESULT InputSiteAdapter::OnTouchHitTesting(_In_ ixp::ITouchHitTestingEventArgs* args)
{
    IFC_RETURN(DirectUI::TouchHitTestingHandler::TouchHitTesting(GetPublicRootVisual(), args));
    return S_OK;
}

CUIElement* InputSiteAdapter::GetPublicRootVisual()
{
    if (xref_ptr<CContentRoot> contentRoot = GetContentRoot())
    {
        return contentRoot->GetVisualTreeNoRef()->GetPublicRootVisual();
    }
    return nullptr;
}

wrl::ComPtr<ixp::IPointerPoint> InputSiteAdapter::GetPreviousPointerPoint()
{
    return m_previousPointerPoint;
}

wrl::ComPtr<ixp::IInputPointerSource> InputSiteAdapter::GetInputPointerSource()
{
    return m_inputPointerSource;
}

_Check_return_ HRESULT InputSiteAdapter::SetFocus()
{
    boolean success = false;
    m_inputFocusController->TrySetFocus(&success);
    if(success == false)
    {
        IFC_RETURN(E_FAIL)
    }

    return S_OK;
}

_Check_return_ HRESULT InputSiteAdapter::SetPointerCapture()
{   // TODO: Remove this dead function
    m_hasCapture = true;
    return S_OK;
}

_Check_return_ HRESULT InputSiteAdapter::ReleasePointerCapture()
{   // TODO: Remove this dead function
    m_hasCapture = false;
    return S_OK;
}

bool InputSiteAdapter::HasPointerCapture()
{   // TODO: Remove this dead function
    return m_hasCapture;
}

bool InputSiteAdapter::ReplayPointerUpdate()
{
    ixp::IPointerPoint* previousPointerPoint = m_previousPointerPoint.Get();
    ixp::IPointerEventArgs* previousPointerEventArgs = m_previousPointerEventArgs.Get();

    if (previousPointerPoint != nullptr)
    {
        if (previousPointerEventArgs)
        {
            bool handled_dontCare;
            IFCFAILFAST(m_jupiterWindow->OnCoreWindowPointerMessage(
                WM_POINTERUPDATE,
                previousPointerPoint,
                previousPointerEventArgs,
                true /* isReplayedMessage */,
                &handled_dontCare));

            return true;
        }
    }

    return false;
}

xref_ptr<CContentRoot> InputSiteAdapter::GetContentRoot() const
{
    return m_contentRootWeak.lock();
}