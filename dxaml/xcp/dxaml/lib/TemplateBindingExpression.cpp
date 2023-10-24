// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TemplateBindingExpression.h"
#include "Control.g.h"
#include "ContentControl.g.h"

using namespace DirectUI;

// Destroys an instance of the TemplateBindingExpression class.
TemplateBindingExpression::~TemplateBindingExpression()
{
    ReleaseInterface(m_pSource);
    m_pTarget = nullptr;
    ReleaseInterface(m_pCustomHandler);
}

// Initializes a new instance of the TemplateBindingExpression class.
_Check_return_ HRESULT TemplateBindingExpression::Create(
    _In_ Control* pSource,
    _In_ const CDependencyProperty* pSourceProperty,
    _In_ bool bRequiresRuntimeTypeCheck,
    _Out_ TemplateBindingExpression** ppExpression)
{
    HRESULT hr = S_OK;
    TemplateBindingExpression* pExpression = NULL;

    IFCPTR(pSource);
    IFCPTR(pSourceProperty);
    IFCPTR(ppExpression);

    IFC(ctl::ComObject<TemplateBindingExpression>::CreateInstance(&pExpression));

    // Initialize template binding expression.
    IFC(pExpression->put_Source(pSource));
    pExpression->m_pSourceProperty = pSourceProperty;
    pExpression->m_bRequiresRuntimeTypeCheck = !!bRequiresRuntimeTypeCheck;

    *ppExpression = pExpression;
    pExpression = NULL;

Cleanup:
    ctl::release_interface(pExpression);
    RRETURN(hr);
}

// Gets a value indicating whether the value can be set.  TemplateBindings are
// always removed if a new value is set, so this always returns FALSE.
_Check_return_ HRESULT TemplateBindingExpression::GetCanSetValue(_Out_ bool *pValue)
{
    *pValue = false;

    return S_OK;
}

// Gets a value indicating whether the expression has been associated with a
// target.
bool TemplateBindingExpression::GetIsAssociated()
{
    // We've been associated when a target has been attached
    return !!m_pTarget;
}

// Attach the expression to a specific property on the target object.
_Check_return_ HRESULT TemplateBindingExpression::OnAttach(
    _In_ DependencyObject* pTarget,
    _In_ const CDependencyProperty* pTargetProperty)
{
    HRESULT hr = S_OK;
    IDPChangedEventSource* pCustomEventSource = NULL;
    TemplateBindingExpressionCustomPropertyChangedHandler* pCustomHandler = NULL;
    Control* pSource = NULL;

    IFCPTR(pTarget);
    IFCPTR(pTargetProperty);

    // We shouldn't have any event handlers at this point
    IFCEXPECT(!m_pCustomHandler);

    m_pTarget = pTarget;    // m_pTarget is a weak reference so we don't AddRef
    m_pTargetProperty = pTargetProperty;

    // Let the ContentControl know its Content property has been bound
    if (m_pTargetProperty->GetIndex() == KnownPropertyIndex::ContentControl_Content)
    {
        IFC(CoreImports::ContentControl_SetContentIsTemplateBoundManaged(static_cast<CContentControl*>(m_pTarget->GetHandle()), true));
    }

    // Attach the custom property changed handler
    IFC(get_Source(&pSource));
    IFC(pSource->GetDPChangedEventSource(&pCustomEventSource));

    pCustomHandler = new TemplateBindingExpressionCustomPropertyChangedHandler();
    IFC( pCustomHandler->Initialize(this));

    m_pCustomHandler = pCustomHandler;
    pCustomHandler = NULL;
    IFC(pCustomEventSource->AddHandler(m_pCustomHandler));

Cleanup:
    if (FAILED(hr))
    {
        // m_pTarget is a weak reference so we don't need to Release
        m_pTarget = NULL;
    }
    ReleaseInterface(pCustomEventSource);
    ReleaseInterface(pCustomHandler);
    ctl::release_interface(pSource);
    RRETURN(hr);
}

// Refresh the expression when the source property has been changed.
_Check_return_ HRESULT TemplateBindingExpression::OnSourcePropertyChanged()
{
    // Only respond to source property change if we're not explicitly ignoring them
    if (!m_ignoreSourcePropertyChanges)
    {
        IFCEXPECT_RETURN(m_pTarget);
        IFCEXPECT_RETURN(m_pTargetProperty);

        IFC_RETURN(m_pTarget->RefreshExpression(m_pTargetProperty));
    }

    return S_OK;
}

// Detach the expression from the target object.
_Check_return_ HRESULT TemplateBindingExpression::OnDetach()
{
    HRESULT hr = S_OK;
    IDPChangedEventSource* pCustomEventSource = NULL;
    Control* pSource = NULL;

    if (m_pTarget == NULL && m_pTargetProperty == NULL)
    {
        // Already detached.
        ASSERT(m_pCustomHandler == NULL);
        goto Cleanup;
    }

    // Let the ContentControl know its Content property has been bound
    if (m_pTargetProperty->GetIndex() == KnownPropertyIndex::ContentControl_Content)
    {
        IFC(CoreImports::ContentControl_SetContentIsTemplateBoundManaged(static_cast<CContentControl*>(m_pTarget->GetHandle()), false));
    }

    // m_pTarget is a weak reference so we don't need to Release
    m_pTarget = NULL;
    m_pTargetProperty = nullptr;

    // The event handlers should already be attached
    IFCEXPECT(m_pCustomHandler);

    // Detach the custom property changed event handler
    if (SUCCEEDED(get_Source(&pSource)))
    {
        IFC(pSource->TryGetDPChangedEventSource(&pCustomEventSource));
        if (pCustomEventSource != NULL)
        {
            IFC(pCustomEventSource->RemoveHandler(m_pCustomHandler));
        }
    }

Cleanup:
    ReleaseInterface(pCustomEventSource);
    ReleaseInterface(m_pCustomHandler);
    ctl::release_interface(pSource);
    RRETURN(hr);
}

// Determine whether a value is compatible with a given source type.
_Check_return_ HRESULT TemplateBindingExpression::IsValidValueForUpdate(
    _In_ IInspectable* pValue,
    _In_ const CClassInfo* pSourceType,
    _Out_ bool* pbIsValid)
{
    HRESULT hr = S_OK;

    IFCPTR(pSourceType);
    IFCPTR(pbIsValid);

    *pbIsValid = FALSE;

    // Value types can't receive null values
    if (!pValue && !pSourceType->IsNullable())
    {
        goto Cleanup;
    }

    // Otherwise null is a valid value
    if (!pValue)
    {
        *pbIsValid = TRUE;
        goto Cleanup;
    }

    // Otherwise check whether the source type is assignable from the value
    IFC(MetadataAPI::IsInstanceOfType(pValue, pSourceType, pbIsValid));

Cleanup:
    RRETURN(hr);
}

// Get the value of the target object's target property.
_Check_return_ HRESULT TemplateBindingExpression::GetValue(
    _In_ DependencyObject* pObject,
    _In_ const CDependencyProperty* pProperty,
    _Out_ IInspectable** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFCEXPECT_RETURN(m_pTargetProperty); // GetValue should only be called when attached

    m_ignoreSourcePropertyChanges = true;
    auto ignoreSourcePropertyChangesGuard = wil::scope_exit([&]
    {
        m_ignoreSourcePropertyChanges = false;
    });

    // Get the value of the source instance's source property
    ctl::ComPtr<IInspectable> spValue;
    ctl::ComPtr<Control> spSource;
    IFC_RETURN(get_Source(&spSource));

    if (   !m_pSourceProperty->ShouldBindingGetValueUseCheckOnDemandProperty()
        || !spSource->GetHandle()->CheckOnDemandProperty(m_pSourceProperty).IsNull())
    {
        IFC_RETURN(spSource->GetValue(m_pSourceProperty, &spValue));
    }

    // Use the value directly if we don't need a runtime check to ensure the
    // types are compatible
    if (!m_bRequiresRuntimeTypeCheck)
    {
        *ppValue = spValue.Detach();
        return S_OK;
    }

    // Otherwise perform a runtime check to determine if the types are
    // compatible
    bool bAreTypesCompatible = false;
    IFC_RETURN(IsValidValueForUpdate(spValue.Get(), m_pTargetProperty->GetPropertyType(), &bAreTypesCompatible));
    if (bAreTypesCompatible)
    {
        *ppValue = spValue.Detach();
        return S_OK;
    }

    // If the current value of the target property isn't valid, use the default
    // value of the target property instead
    IFC_RETURN(m_pTarget->GetDefaultValueInternal(m_pTargetProperty, &spValue));
    *ppValue = spValue.Detach();

    return S_OK;
}

// Determine whether two property types are compatible and whether they are
// potentially compatible but require a runtime type check.
_Check_return_ HRESULT TemplateBindingExpression::ArePropertyTypesCompatible(
    _In_ const CClassInfo* pSourceType,
    _In_ const CClassInfo* pTargetType,
    _Out_ bool* pbTypesAreCompatible,
    _Out_ bool* pbRequiresRuntimeTypeCheck)
{
    HRESULT hr = S_OK;
    bool bIsAssignable = false;

    IFCPTR(pSourceType);
    IFCPTR(pTargetType);
    IFCPTR(pbTypesAreCompatible);
    IFCPTR(pbRequiresRuntimeTypeCheck);

    // By default assume the type are not compatible
    *pbTypesAreCompatible = FALSE;
    *pbRequiresRuntimeTypeCheck = FALSE;

    // If the source type is the same type or derived from the target type, then
    // the types are fully compatible and don't require a runtime type check
    bIsAssignable = MetadataAPI::IsAssignableFrom(pTargetType, pSourceType);
    if (bIsAssignable)
    {
        *pbTypesAreCompatible = TRUE;
        goto Cleanup;
    }

    // If the target type is derived from the source type, then the types are
    // potentially compatible but will require a runtime type check (i.e., if
    // we have a source property of type UIElement then we should allow it to
    // be template bound to a target property of type Button but check at
    // runtime to make sure the value of the source property really is a Button)
    bIsAssignable = MetadataAPI::IsAssignableFrom(pSourceType, pTargetType);
    if (bIsAssignable)
    {
        *pbTypesAreCompatible = TRUE;
        *pbRequiresRuntimeTypeCheck = TRUE;
        goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}

// Create and set a template binding from the source property of the source
// Control to the target property of the target DependencyObject.  This method
// is called by the four callbacks used to set template bindings.
_Check_return_ HRESULT TemplateBindingExpression::SetTemplateBinding(
    _In_ Control* pSource,
    _In_ const CDependencyProperty* pSourceProperty,
    _In_ DependencyObject* pTarget,
    _In_ const CDependencyProperty* pTargetProperty)
{
    HRESULT hr = S_OK;

    bool bTypeAreCompatible = false;
    bool bRequiresRuntimeTypeCheck = false;
    TemplateBindingExpression* pExpression = NULL;

    IFCPTR(pSource);
    IFCPTR(pSourceProperty);
    IFCPTR(pTarget);
    IFCPTR(pTargetProperty);

    ASSERT(!pSourceProperty->Is<CCustomProperty>() && !pTargetProperty->Is<CCustomProperty>());

    // Ensure the property types are compatible
    IFC(ArePropertyTypesCompatible(pSourceProperty->GetPropertyType(), pTargetProperty->GetPropertyType(), &bTypeAreCompatible, &bRequiresRuntimeTypeCheck));
    if (!bTypeAreCompatible)
    {
        // If the types aren't compatible, then there's no template binding to
        // create (and it's not considered an error)
        goto Cleanup;
    }

    // Create the TemplateBindingExpression
    IFC(TemplateBindingExpression::Create(pSource, pSourceProperty, bRequiresRuntimeTypeCheck, &pExpression));

    // Assign the expression to the target DP
    IFC(pTarget->SetValueExpression(pTargetProperty, pExpression));

Cleanup:
    ctl::release_interface(pExpression);
    RRETURN(hr);
}

// Gets or sets the Source weak reference
_Check_return_ HRESULT TemplateBindingExpression::get_Source(
    _Outptr_ Control** ppSource)
{
    HRESULT hr = S_OK;
    xaml_controls::IControl* pSourceAsI = NULL;

    IFCPTR(ppSource);
    *ppSource = NULL;

    // There's a bug in the CLR (#470130) where we try to resolve a weak reference to a managed object, but the object
    // is in the middle of destructing itself, yet the resolve succeeds.  This happens when we're in a thread/core/window
    // shutdown.  As a workaround, detect the shutdown case, and simulate a failed weak reference resolution, which is
    // to fail this method.  That means we won't clean up the templated parent, but that's OK, because it's on this thread too,
    // and so is going away anyway.
    if (!DXamlCore::GetCurrent()->IsShuttingDown())
    {
        IFCEXPECT(m_pSource);
        IFC(ctl::resolve_weakref(m_pSource, pSourceAsI));
        *ppSource = static_cast<Control*>(pSourceAsI);
    }
    IFCCHECK_NOTRACE(*ppSource);

    pSourceAsI = NULL;

Cleanup:
    ReleaseInterface(pSourceAsI);
    RRETURN(hr);
}
_Check_return_ HRESULT TemplateBindingExpression::put_Source(
    _In_ Control* pSource)
{
    HRESULT hr = S_OK;

    ReleaseInterface(m_pSource);
    IFCEXPECT(pSource);
    IFC(ctl::as_weakref(m_pSource, ctl::as_iinspectable(pSource)));

Cleanup:
    RRETURN(hr);
}



// Set a template binding between two properties on two objects.  This is a
// callback from the core.
_Check_return_ HRESULT TemplateBindingExpression::SetTemplateBinding(
    _In_ CDependencyObject* source,
    _In_ const CDependencyProperty* sourceProperty,
    _In_ CDependencyObject* target,
    _In_ const CDependencyProperty* targetProperty)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<DependencyObject> sourcePeer;
    ctl::ComPtr<DependencyObject> targetPeer;
    const CDependencyProperty* sourceDP = nullptr;
    const CDependencyProperty* targetDP = nullptr;

    // Get the managed peers.
    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFC(pCore->GetPeer(source, &sourcePeer));
    IFC(pCore->GetPeer(target, &targetPeer));

    // Try to get the underlying DP so we can attach a binding.
    IFC(MetadataAPI::TryGetUnderlyingDependencyProperty(sourceProperty, &sourceDP));
    IFC(MetadataAPI::TryGetUnderlyingDependencyProperty(targetProperty, &targetDP));

    // Create and set the TemplateBindingExpression.
    IFC(SetTemplateBinding(sourcePeer.Cast<Control>(), sourceDP, targetPeer.Get(), targetDP));

Cleanup:
    RRETURN(hr);
}

// Handle the custom DependencyProperty changed event for the source property
// and refresh the TemplateBindingExpression.
IFACEMETHODIMP TemplateBindingExpressionCustomPropertyChangedHandler::Invoke(
    _In_ xaml::IDependencyObject* pSender,
    _In_ const CDependencyProperty* pDP)
{
    HRESULT hr = S_OK;
    TemplateBindingExpression *pExpression = NULL;
    IExpressionBase *pExpressionBase = NULL;

    IFCEXPECT(m_pExpressionRef);

    IFC(ctl::resolve_weakref(m_pExpressionRef, pExpressionBase));
    if (pExpressionBase == NULL)
    {
        goto Cleanup;
    }
    pExpression = static_cast<TemplateBindingExpression*>(pExpressionBase);

    if (pDP->GetIndex() == pExpression->m_pSourceProperty->GetIndex())
    {
        IFC(pExpression->OnSourcePropertyChanged());
    }

Cleanup:

    ReleaseInterface(pExpressionBase);
    RRETURN(hr);
}
