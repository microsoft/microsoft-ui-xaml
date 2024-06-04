// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ElementSoundPlayer.g.h"
#include "Control.g.h"
#include "ElementSoundPlayerService_Partial.h"

using namespace DirectUI;

_Check_return_ HRESULT
ElementSoundPlayerFactory::get_VolumeImpl(
    _Out_ DOUBLE* pValue)
{
    IFCPTR_RETURN(pValue);
    *pValue = 0;

    IFCEXPECT_RETURN(DXamlServices::IsDXamlCoreInitialized());
    ElementSoundPlayerService* service = DXamlCore::GetCurrent()->GetElementSoundPlayerServiceNoRef();
    *pValue = service->GetVolume();

    return S_OK;
}

_Check_return_ HRESULT
ElementSoundPlayerFactory::put_VolumeImpl(
    _In_ DOUBLE value)
{
    IFCEXPECT_RETURN(DXamlServices::IsDXamlCoreInitialized());

    if (value < 0.0 || value > 1.0)
    {
        IFC_RETURN(ErrorHelper::OriginateErrorUsingResourceID(E_INVALIDARG, ERROR_ELEMENTSOUNDPLAYER_VOLUME_OUTOFRANGE))
    }
    else
    {
        ElementSoundPlayerService* service = DXamlCore::GetCurrent()->GetElementSoundPlayerServiceNoRef();
        IFC_RETURN(service->SetVolume(value));
    }

    return S_OK;
}

_Check_return_ HRESULT
ElementSoundPlayerFactory::get_StateImpl(
    _Out_ xaml::ElementSoundPlayerState* pValue)
{
    IFCPTR_RETURN(pValue);
    *pValue = xaml::ElementSoundPlayerState_Auto;

    IFCEXPECT_RETURN(DXamlServices::IsDXamlCoreInitialized());
    ElementSoundPlayerService* service = DXamlCore::GetCurrent()->GetElementSoundPlayerServiceNoRef();
    *pValue = service->GetPlayerState();

    return S_OK;
}

_Check_return_ HRESULT
ElementSoundPlayerFactory::put_StateImpl(
    _In_ xaml::ElementSoundPlayerState value)
{
    IFCEXPECT_RETURN(DXamlServices::IsDXamlCoreInitialized());
    ElementSoundPlayerService* service = DXamlCore::GetCurrent()->GetElementSoundPlayerServiceNoRef();
    IFC_RETURN(service->SetPlayerState(value));

    return S_OK;
}

_Check_return_ HRESULT
ElementSoundPlayerFactory::PlayImpl(
    _In_ xaml::ElementSoundKind sound)
{
    IFCEXPECT_RETURN(DXamlServices::IsDXamlCoreInitialized());
    ElementSoundPlayerService* service = DXamlCore::GetCurrent()->GetElementSoundPlayerServiceNoRef();
    IFC_RETURN(service->Play(sound));

    return S_OK;
}

_Check_return_ HRESULT
ElementSoundPlayerFactory::RequestInteractionSoundForElementImpl(
    _In_ xaml::ElementSoundKind sound,
    _In_ xaml::IDependencyObject* pElement)
{
    IFCEXPECT_RETURN(DXamlServices::IsDXamlCoreInitialized());
    ElementSoundPlayerService* service = DXamlCore::GetCurrent()->GetElementSoundPlayerServiceNoRef();
    IFC_RETURN(service->RequestInteractionSoundForElement(sound, static_cast<DependencyObject*>(pElement)));

    return S_OK;
}

_Check_return_ HRESULT
ElementSoundPlayerFactory::GetEffectiveSoundModeImpl(
    _In_ xaml::IDependencyObject* pElement,
    _Out_ xaml::ElementSoundMode* soundMode)
{
    IFCEXPECT_RETURN(DXamlServices::IsDXamlCoreInitialized());
    *soundMode = ElementSoundPlayerService::GetEffectiveSoundMode(static_cast<DependencyObject*>(pElement));
    return S_OK;
}

_Check_return_ HRESULT
ElementSoundPlayerFactory::get_SpatialAudioModeImpl(
    _Out_ xaml::ElementSpatialAudioMode* pValue)
{
    IFCPTR_RETURN(pValue);
    *pValue = xaml::ElementSpatialAudioMode_Off;

    IFCEXPECT_RETURN(DXamlServices::IsDXamlCoreInitialized());
    ElementSoundPlayerService* service = DXamlCore::GetCurrent()->GetElementSoundPlayerServiceNoRef();

    *pValue = service->GetSpatialAudioMode();

    return S_OK;
}

_Check_return_ HRESULT
ElementSoundPlayerFactory::put_SpatialAudioModeImpl(
    _In_ xaml::ElementSpatialAudioMode value)
{
    IFCEXPECT_RETURN(DXamlServices::IsDXamlCoreInitialized());

    ElementSoundPlayerService* service = DXamlCore::GetCurrent()->GetElementSoundPlayerServiceNoRef();

    IFC_RETURN(service->SetSpatialAudioMode(value));

    return S_OK;
}
