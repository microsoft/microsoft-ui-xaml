// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
//------------------------------------------------------------------------
//
//  Abstract:
//
//      XAML types.
//      NOTE: This file was generated by a tool.
//
//------------------------------------------------------------------------

#include "InputValidationCommand.g.h"
#include "XamlTelemetry.h"

// Constructors/destructors.
DirectUI::InputValidationCommand::InputValidationCommand()
{
}

DirectUI::InputValidationCommand::~InputValidationCommand()
{
}

HRESULT DirectUI::InputValidationCommand::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::InputValidationCommand)))
    {
        *ppObject = static_cast<DirectUI::InputValidationCommand*>(this);
    }
#if WI_IS_FEATURE_PRESENT(Feature_InputValidation)
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::IInputValidationCommand)) && Feature_InputValidation::IsEnabled())
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::IInputValidationCommand*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::IInputValidationCommandOverrides)) && Feature_InputValidation::IsEnabled())
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::IInputValidationCommandOverrides*>(this);
    }
#endif
    else
    {
        RRETURN(DirectUI::DependencyObject::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Properties.
IFACEMETHODIMP DirectUI::InputValidationCommand::get_InputValidationKind(_Out_ ABI::Microsoft::UI::Xaml::Controls::InputValidationKind* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::InputValidationCommand_InputValidationKind, pValue));
}
IFACEMETHODIMP DirectUI::InputValidationCommand::put_InputValidationKind(ABI::Microsoft::UI::Xaml::Controls::InputValidationKind value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::InputValidationCommand_InputValidationKind, value));
}
IFACEMETHODIMP DirectUI::InputValidationCommand::get_InputValidationMode(_Out_ ABI::Microsoft::UI::Xaml::Controls::InputValidationMode* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::InputValidationCommand_InputValidationMode, pValue));
}
IFACEMETHODIMP DirectUI::InputValidationCommand::put_InputValidationMode(ABI::Microsoft::UI::Xaml::Controls::InputValidationMode value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::InputValidationCommand_InputValidationMode, value));
}

// Events.

// Methods.
IFACEMETHODIMP DirectUI::InputValidationCommand::CanValidate(_In_ ABI::Microsoft::UI::Xaml::Controls::IInputValidationControl* pValidationControl, _Out_ BOOLEAN* pResult)
{
    HRESULT hr = S_OK;
    ABI::Microsoft::UI::Xaml::Controls::IInputValidationCommandOverrides* pVirtuals = NULL;

    if (IsComposed())
    {
        IFC(ctl::do_query_interface(pVirtuals, this));

        // SYNC_CALL_TO_APP DIRECT - This next line may directly call out to app code.
        IFC(pVirtuals->CanValidateCore(pValidationControl, pResult));
    }
    else
    {
        IFC(CanValidateCore(pValidationControl, pResult));
    }

Cleanup:
    ReleaseInterfaceNoNULL(pVirtuals);
    RRETURN(hr);
}

IFACEMETHODIMP DirectUI::InputValidationCommand::CanValidateCore(_In_ ABI::Microsoft::UI::Xaml::Controls::IInputValidationControl* pValidationControl, _Out_ BOOLEAN* pResult)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "InputValidationCommand_CanValidate", 0);
    }
    ARG_NOTNULL(pValidationControl, "validationControl");
    ARG_VALIDRETURNPOINTER(pResult);
    *pResult={};
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<InputValidationCommand*>(this)->CanValidateCoreImpl(pValidationControl, pResult));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "InputValidationCommand_CanValidate", hr);
    }
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::InputValidationCommand::Validate(_In_ ABI::Microsoft::UI::Xaml::Controls::IInputValidationControl* pValidationControl)
{
    HRESULT hr = S_OK;
    ABI::Microsoft::UI::Xaml::Controls::IInputValidationCommandOverrides* pVirtuals = NULL;

    if (IsComposed())
    {
        IFC(ctl::do_query_interface(pVirtuals, this));

        // SYNC_CALL_TO_APP DIRECT - This next line may directly call out to app code.
        IFC(pVirtuals->ValidateCore(pValidationControl));
    }
    else
    {
        IFC(ValidateCore(pValidationControl));
    }

Cleanup:
    ReleaseInterfaceNoNULL(pVirtuals);
    RRETURN(hr);
}

IFACEMETHODIMP DirectUI::InputValidationCommand::ValidateCore(_In_ ABI::Microsoft::UI::Xaml::Controls::IInputValidationControl* pValidationControl)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "InputValidationCommand_Validate", 0);
    }
    ARG_NOTNULL(pValidationControl, "validationControl");
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<InputValidationCommand*>(this)->ValidateCoreImpl(pValidationControl));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "InputValidationCommand_Validate", hr);
    }
    RRETURN(hr);
}

HRESULT DirectUI::InputValidationCommandFactory::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
#if WI_IS_FEATURE_PRESENT(Feature_InputValidation)
    if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::IInputValidationCommandFactory)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::IInputValidationCommandFactory*>(this);
    }
    else
#endif
    {
        RRETURN(ctl::BetterAggregableCoreObjectActivationFactory::QueryInterfaceImpl(iid, ppObject));
    }

#if WI_IS_FEATURE_PRESENT(Feature_InputValidation)
    AddRefOuter();
    RRETURN(S_OK);
#endif
}


// Factory methods.
IFACEMETHODIMP DirectUI::InputValidationCommandFactory::CreateInstance(_In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ ABI::Microsoft::UI::Xaml::Controls::IInputValidationCommand** ppInstance)
{
    if (!Feature_InputValidation::IsEnabled()) IFC_RETURN(E_NOTIMPL);

#if DBG
    // We play some games with reinterpret_cast and assuming that the GUID type table is accurate - which is somewhat sketchy, but
    // really good for binary size.  This code is a sanity check that the games we play are ok.
    const GUID uuidofGUID = __uuidof(ABI::Microsoft::UI::Xaml::Controls::IInputValidationCommand);
    const GUID metadataAPIGUID = MetadataAPI::GetClassInfoByIndex(GetTypeIndex())->GetGuid();
    const KnownTypeIndex typeIndex = GetTypeIndex();

    if(uuidofGUID != metadataAPIGUID)
    {
        XAML_FAIL_FAST();
    }
#endif

    // Can't just IFC(_RETURN) this because for some validate calls (those with multiple template parameters), the
    // preprocessor gets confused at the "," in the template type-list before the function's opening parenthesis.
    // So we'll use IFC_RETURN syntax with a local hr variable, kind of weirdly.
    const HRESULT hr = ctl::ValidateFactoryCreateInstanceWithBetterAggregableCoreObjectActivationFactory(pOuter, ppInner, reinterpret_cast<IUnknown**>(ppInstance), GetTypeIndex(), false /*isFreeThreaded*/);
    IFC_RETURN(hr);
    return S_OK;
}

// Dependency properties.



// Attached properties.

// Static properties.

// Static methods.

namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_InputValidationCommand()
    {
        RRETURN(ctl::ActivationFactoryCreator<InputValidationCommandFactory>::CreateActivationFactory());
    }
}
