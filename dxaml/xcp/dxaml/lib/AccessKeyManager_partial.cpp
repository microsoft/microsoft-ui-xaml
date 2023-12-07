// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "InputServices.h"
#include "XamlRoot.g.h"
#include "AccessKeymanager.g.h"
#include "InputServices.h"
#include <FocusMgr.h>

using namespace DirectUI;

// Callback from AKModeContainer
namespace AccessKeys {
    _Check_return_ HRESULT UpdateIsDisplayModeEnabled(_Out_ bool* didDisplayModeForThreadChange)
    {
        bool isDisplayModeEnabled {false};
        const auto contentRootCoordinator = DXamlCore::GetCurrent()->GetHandle()->GetContentRootCoordinator();

        // New lifted behavior:
        // AccessKeyManager.IsDisplayModeEnabled returns true if *any* Xaml content on the thread is in AccessKey mode.
        // AccessKeyManager.IsDisplayModeEnabledChanged fires when IsDisplayModeEnabled changes.
        if (DXamlCore::GetCurrent()->GetHandle()->GetInitializationType() == InitializationType::IslandsOnly)
        {
            for (xref_ptr<CContentRoot> contentRoot : contentRootCoordinator->GetContentRoots())
            {
                if (contentRoot)
                {
                    const bool isActive = contentRoot->GetAKExport().GetModeContainer().GetIsActive();
                    if (isActive)
                    {
                        isDisplayModeEnabled = true;
                        break;
                    }
                }
            }
        }
        else
        {
            // UWP and legacy behavior (unsupported):
            // This matches System Xaml behavior.  Only the AccessKey mode of the Xaml content on the MainWindow
            // matters.  This was for compat, so earlier apps weren't broken by the new behavior.
            if (const auto contentRoot = contentRootCoordinator->Unsafe_IslandsIncompatible_CoreWindowContentRoot())
            {
                isDisplayModeEnabled = contentRoot->GetAKExport().GetModeContainer().GetIsActive();
            }
        }

        AKEvents& accessKeyEvents {DXamlCore::GetCurrent()->AccessKeyEvents()};
        const bool previousIsDisplayModeEnabled {accessKeyEvents.GetIsDisplayModeEnabledForCurrentThread()};
        const bool didDisplayModeForThreadChangeLocal {previousIsDisplayModeEnabled != isDisplayModeEnabled};

        if (didDisplayModeForThreadChangeLocal == true)
        {
            accessKeyEvents.SetIsDisplayModeEnabledForCurrentThread(isDisplayModeEnabled);
        }

        *didDisplayModeForThreadChange = didDisplayModeForThreadChangeLocal;
        return S_OK;
    }

    _Check_return_ HRESULT AKOnIsActiveChanged(_In_ CFocusManager* focusManager,_In_ IInspectable* sender, _Out_ IInspectable* args)
    {
        DXamlCore* const dxamlCore = DXamlCore::GetCurrent();
        CCoreServices* const core = dxamlCore->GetHandle();
        bool didDisplayModeForThreadChange {false};
        
        IFC_RETURN(UpdateIsDisplayModeEnabled(&didDisplayModeForThreadChange));

        IFC_RETURN(focusManager->OnAccessKeyDisplayModeChanged());

        core->GetInputServices()->GetKeyTipManager().AccessKeyModeChanged();

        if (didDisplayModeForThreadChange)
        {
            // We fire IsDisplayModeEnabledChanged only when IsDisplayModeEnabled changes.
            IFC_RETURN(dxamlCore->AccessKeyEvents().RaiseIsActiveChanged(sender, args));
        }
        return S_OK;
    }
}

namespace DirectUI
{

_Check_return_ HRESULT AccessKeyManagerFactory::add_IsDisplayModeEnabledChanged(
    _In_ wf::ITypedEventHandler<IInspectable*,IInspectable*>* value,
    _Out_ EventRegistrationToken* token)
{
    IFC_RETURN(CheckActivationAllowed());
    IFC_RETURN(DXamlCore::GetCurrent()->AccessKeyEvents().add_IsActiveChanged(value, token));
    return S_OK;
}

_Check_return_ HRESULT AccessKeyManagerFactory::remove_IsDisplayModeEnabledChanged(
    _In_ EventRegistrationToken token)
{
    IFC_RETURN(CheckActivationAllowed());
    IFC_RETURN(DXamlCore::GetCurrent()->AccessKeyEvents().remove_IsActiveChanged(token));
    return S_OK;
}

// New lifted behavior:
// IsDisplayModeEnabled/IsDisplayModeEnabledChanged don't take any parameters, so they must be true or false
// for the whole thread.  But internally to Xaml, any XamlRoot/ContentRoot may be in or out of AccessKey
// DisplayMode.  So:
//
// AccessKeyManager.IsDisplayModeEnabled returns true if *any* Xaml content on the thread is in AccessKey mode.
// AccessKeyManager.IsDisplayModeEnabledChanged fires when IsDisplayModeEnabled changes.
_Check_return_ HRESULT AccessKeyManagerFactory::get_IsDisplayModeEnabledImpl(_Out_ BOOLEAN* value)
{
    AccessKeys::AKEvents& accessKeyEvents {DXamlCore::GetCurrent()->AccessKeyEvents()};
    *value = !!(accessKeyEvents.GetIsDisplayModeEnabledForCurrentThread());
    return S_OK;
}

_Check_return_ HRESULT AccessKeyManagerFactory::EnterDisplayModeForXamlRootImpl(_In_ xaml::IXamlRoot* xamlRootInterface)
{
    ctl::ComPtr<DirectUI::XamlRoot> xamlRoot;    
    IFC_RETURN(xamlRootInterface->QueryInterface(IID_PPV_ARGS(&xamlRoot)));

    CContentRoot* contentRoot = xamlRoot->GetVisualTreeNoRef()->GetContentRootNoRef();
    IFC_RETURN(contentRoot->GetAKExport().EnterAccessKeyMode());
    return S_OK;
}


// New lifted behavior:
// ExitDisplayModeImpl doesn't take any parameters, so it must apply to the whole thread.  But internally to Xaml, any
// XamlRoot/ContentRoot may be in or out of AccessKey DisplayMode.  So:
//
// AccessKeyManager.ExitDisplayModeImpl now exits AccessKey display mode for all Xaml content on the thread.
_Check_return_ HRESULT AccessKeyManagerFactory::ExitDisplayModeImpl()
{
    const auto contentRootCoordinator = DXamlCore::GetCurrent()->GetHandle()->GetContentRootCoordinator();

    // This was first enabled for FileExplorer's AccessKeys.  We make ExitDisplayMode work for islands-based apps
    // by iterating over all the ContentRoots (which include islands), and exiting AccessKey mode for each one.
    // This effectively exits AccessKey mode for all Xaml islands on the thread.
    if (DXamlCore::GetCurrent()->GetHandle()->GetInitializationType() == InitializationType::IslandsOnly)
    {
        for (xref_ptr<CContentRoot> contentRoot : contentRootCoordinator->GetContentRoots())
        {
            if (contentRoot)
            {
                IGNOREHR(contentRoot->GetAKExport().ExitAccessKeyMode());
            }
        }
    }
    else
    {
        // UWP behavior: just exit AccessKeyMode for the Xaml content on the CoreWindow.
        const auto contentRoot = contentRootCoordinator->Unsafe_IslandsIncompatible_CoreWindowContentRoot();

        if (contentRoot == nullptr)
        {
            return S_OK;
        }

        IFC_RETURN(contentRoot->GetAKExport().ExitAccessKeyMode());
    }
    return S_OK;
}


_Check_return_ HRESULT AccessKeyManagerFactory::get_AreKeyTipsEnabledImpl(_Out_ BOOLEAN* value)
{
    auto inputServices = DXamlCore::GetCurrent()->GetHandle()->GetInputServices();
    if (inputServices != nullptr)
    {
        *value = inputServices->GetKeyTipManager().AreKeyTipsEnabled();
    }
    else
    {
        // Tests can call this during cleanup (via KeyTipTestPoolFilter::IsDirty) after shutdown. In that case there will
        // be no input manager. Return the default value.
        *value = true;
    }

    return S_OK;
}

_Check_return_ HRESULT AccessKeyManagerFactory::put_AreKeyTipsEnabledImpl(_In_ BOOLEAN value)
{
    auto inputServices = DXamlCore::GetCurrent()->GetHandle()->GetInputServices();
    IFCEXPECT_RETURN(inputServices);

    inputServices->GetKeyTipManager().SetAreKeyTipsEnabled(!!value);
    return S_OK;
}



}