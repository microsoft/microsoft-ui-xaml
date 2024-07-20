// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <Indexes.g.h> // uses KnownTypeIndex::UnknownType
#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>

using namespace DirectUI;

_Check_return_ HRESULT ctl::ValidateFactoryCreateInstanceWithBetterAggregableCoreObjectActivationFactory(_In_opt_ IInspectable* const pOuter, _In_ IInspectable** const ppInner, _Inout_ IUnknown** ppInstance, _In_ KnownTypeIndex typeIndex, bool isFreeThreaded)
{
    ARG_VALIDRETURNPOINTER(ppInstance);
    IFCEXPECT_RETURN(pOuter == nullptr || ppInner != nullptr);

    const GUID typeIID = MetadataAPI::GetClassInfoByIndex(typeIndex)->GetGuid();

    ComPtr<IInspectable> spInner;
    ComPtr<IInspectable> spInstance;
    IFC_RETURN(ctl::BetterAggregableCoreObjectActivationFactory::ActivateInstanceStatic(pOuter, spInner.ReleaseAndGetAddressOf(), typeIndex, true /*shouldCheckActivationAllowed*/, isFreeThreaded));
    IFC_RETURN(iunknown_cast(spInner.Get())->QueryInterface(typeIID, reinterpret_cast<void**>(spInstance.ReleaseAndGetAddressOf())));

    if (ppInner)
    {
        *ppInner = spInner.Detach();
    }

    *ppInstance = spInstance.Detach();

    return S_OK;
}

_Check_return_ HRESULT ctl::ValidateFactoryCreateInstanceWithBetterAggregableAbstractCoreObjectActivationFactory(_In_opt_ IInspectable* const pOuter, _In_ IInspectable** const ppInner, _Inout_ IUnknown** ppInstance, _In_ KnownTypeIndex typeIndex, bool isFreeThreaded)
{
    ARG_VALIDRETURNPOINTER(ppInstance);
    IFCEXPECT_RETURN(pOuter == nullptr || ppInner != nullptr);

    const GUID typeIID = MetadataAPI::GetClassInfoByIndex(typeIndex)->GetGuid();

    ComPtr<IInspectable> spInner;
    ComPtr<IInspectable> spInstance;
    IFC_RETURN(ctl::BetterAggregableAbstractCoreObjectActivationFactory::ActivateInstanceStatic(pOuter, spInner.ReleaseAndGetAddressOf(), typeIndex, true /* shouldCheckActivationAllowed */, isFreeThreaded));
    IFC_RETURN(iunknown_cast(spInner.Get())->QueryInterface(typeIID, reinterpret_cast<void**>(spInstance.ReleaseAndGetAddressOf())));

    if (ppInner)
    {
        *ppInner = spInner.Detach();
    }

    *ppInstance = spInstance.Detach();

    return S_OK;
}

/*static*/ _Check_return_ HRESULT ctl::BaseActivationFactory::CheckActivationAllowedStatic(bool isFreeThreaded)
{
    if (isFreeThreaded)
    {
        return S_OK;
    }

    if (DirectUI::DXamlServices::IsDXamlCoreInitialized())
    {
        return S_OK;
    }

    // Make sure this features is enabled before checking that is OK.
    auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();
    if (runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnableCoreShutdown) && DXamlCore::IsIdleStatic())
    {
        return S_OK;
    }

    return RPC_E_WRONG_THREAD;
}

_Check_return_ HRESULT ctl::BaseActivationFactory::CheckActivationAllowed()
{
    return CheckActivationAllowedStatic(false /* isFreeThreaded */);
}

IActivationFactory* ctl::BetterActivationFactoryCreator::GetForDO(KnownTypeIndex eTypeIndex)
{
    ctl::ComPtr<ctl::DynamicCoreObjectActivationFactory> spFactory;

    // Ignore leaks for activation factories. Those stay around for the lifetime of the process
    // and their lifetime is handled by the com module which we don't want to mess with.
    if (FAILED(ctl::make_ignoreleak(eTypeIndex, &spFactory)))
    {
        return nullptr;
    }

    return ctl::interface_cast<IActivationFactory>(spFactory.Detach());
}

IFACEMETHODIMP ctl::BetterCoreObjectActivationFactory::ActivateInstance(_Outptr_ IInspectable** ppInstance)
{
    HRESULT hr = S_OK;
    IFC(CheckActivationAllowed());
    IFC(ActivationAPI::ActivateInstance(GetTypeIndex(), ppInstance));
Cleanup:
    return hr;
}

KnownTypeIndex ctl::BetterCoreObjectActivationFactory::GetTypeIndex() const
{
    return KnownTypeIndex::UnknownType;
}

_Check_return_ HRESULT ctl::BetterAggregableCoreObjectActivationFactory::ActivateInstance(_In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInstance)
{
    IFC_RETURN(CheckActivationAllowed());
    IFC_RETURN(ctl::BetterAggregableCoreObjectActivationFactory::ActivateInstanceStatic(pOuter, ppInstance, GetTypeIndex(), false /* shouldCheckActivationAllowed */, false /* isFreeThreaded */));
    return S_OK;
}

/*static*/ _Check_return_ HRESULT ctl::BetterAggregableCoreObjectActivationFactory::ActivateInstanceStatic(_In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInstance, _In_ KnownTypeIndex typeIndex, bool shouldCheckActivationAllowed, bool isFreeThreaded)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spInstance;

    if (shouldCheckActivationAllowed)
    {
        IFC(ctl::BaseActivationFactory::CheckActivationAllowedStatic(isFreeThreaded));
    }

    IFC(ActivationAPI::ActivateInstance(typeIndex, pOuter, &spInstance));

    if (pOuter)
    {
        // Get a raw pointer to the inner IInspectable.
        *ppInstance = reinterpret_cast<IInspectable*>(spInstance.Cast<ComBase>()->AsNonDelegatingInspectable());

        // Don't release the inner object's reference, or else the object will get destroyed.
        spInstance.Detach();
    }
    else
    {
        *ppInstance = spInstance.Detach();
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT ctl::BetterAggregableCoreObjectActivationFactory::ActivateInstance(_In_opt_ IInspectable* pOuter, _In_ CDependencyObject* pValue, _Outptr_ IInspectable** ppInstance)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spInstance;

    IFC(CheckActivationAllowed());
    IFC(ActivationAPI::ActivateAutomationInstance(GetTypeIndex(), pValue, pOuter, &spInstance));

    if (pOuter)
    {
        // Get a raw pointer to the inner IInspectable.
        *ppInstance = reinterpret_cast<IInspectable*>(spInstance.Cast<ComBase>()->AsNonDelegatingInspectable());

        // Don't release the inner object's reference, or else the object will get destroyed.
        spInstance.Detach();
    }
    else
    {
        *ppInstance = spInstance.Detach();
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT ctl::BetterAggregableAbstractCoreObjectActivationFactory::ActivateInstance(_In_ IInspectable* pOuter, _Outptr_ IInspectable** ppInstance)
{
    IFC_RETURN(CheckActivationAllowed());
    IFC_RETURN(ActivateInstanceStatic(pOuter, ppInstance, GetTypeIndex(), false /* shouldCheckActivationAllowed */, false /* isFreeThreaded */));
    return S_OK;
}

/*static*/ _Check_return_ HRESULT ctl::BetterAggregableAbstractCoreObjectActivationFactory::ActivateInstanceStatic(_In_ IInspectable* pOuter, _Outptr_ IInspectable** ppInstance, KnownTypeIndex typeIndex, bool shouldCheckActivationAllowed, bool isFreeThreaded)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spInstance;

    IFCPTR(pOuter);

    if (shouldCheckActivationAllowed)
    {
        IFC(ctl::BaseActivationFactory::CheckActivationAllowedStatic(isFreeThreaded));
    }
    IFC(ActivationAPI::ActivateInstance(typeIndex, pOuter, &spInstance));

    // Get a raw pointer to the inner IInspectable.
    *ppInstance = reinterpret_cast<IInspectable*>(spInstance.Cast<ComBase>()->AsNonDelegatingInspectable());

    // Don't release the inner object's reference, or else the object will get destroyed.
    spInstance.Detach();

Cleanup:
    return hr;
}

_Check_return_ HRESULT ctl::BetterAggregableAbstractCoreObjectActivationFactory::ActivateInstance(_In_opt_ IInspectable* pOuter, _In_ CDependencyObject* pValue, _Outptr_ IInspectable** ppInstance)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spInstance;

    IFCPTR(pOuter);
    IFC(CheckActivationAllowed());
    IFC(ActivationAPI::ActivateAutomationInstance(GetTypeIndex(), pValue, pOuter, &spInstance));

    // Get a raw pointer to the inner IInspectable.
    *ppInstance = reinterpret_cast<IInspectable*>(spInstance.Cast<ComBase>()->AsNonDelegatingInspectable());

    // Don't release the inner object's reference, or else the object will get destroyed.
    spInstance.Detach();

Cleanup:
    return hr;
}

KnownTypeIndex ctl::AbstractActivationFactory::GetTypeIndex() const
{
    return KnownTypeIndex::UnknownType;
}

ctl::DynamicCoreObjectActivationFactory::DynamicCoreObjectActivationFactory()
    : m_eTypeIndex(KnownTypeIndex::UnknownType)
{}
