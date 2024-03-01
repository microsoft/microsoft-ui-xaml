// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Provides a framework of common APIs for objects that participate in
//      layout. FrameworkElement also defines APIs related to data binding,
//      object tree, and object lifetime feature areas.

#include "precomp.h"
#include "FrameworkElement.g.h"
#include "FrameworkElementAutomationPeer.g.h"
#include "CollectionViewGroup.g.h"
#include "Binding.g.h"
#include "ToolTip.g.h"
#include "ToolTipService.g.h"
#include "VisualStateManager.g.h"
#include <DependencyLocator.h>
#include <DeferredElement.h>
#include "VisualTreeHelper.h"
#include "DefaultValueConverter.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

FrameworkElement::~FrameworkElement()
{
    if (m_pChainedGoToElementStateSourceControls)
    {
        ASSERT(m_pChainedGoToElementStateSourceControls->empty());
        delete m_pChainedGoToElementStateSourceControls;
    }
    ctl::release_interface(m_pDataContextChangedSource);
}

// Customized methods
_Check_return_ HRESULT
FrameworkElement::FindNameImpl(
    _In_ HSTRING name,
    _Out_ IInspectable** ppReturnValue)
{
    *ppReturnValue = nullptr;

    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFCEXPECT_RETURN(pCore);

    auto namedObject = pCore->GetHandle()->TryGetElementByName(xephemeral_string_ptr(name), GetHandle());
    if (!namedObject) return S_OK;

    ctl::ComPtr<DependencyObject> dxamlLayerObject;
    IFC_RETURN(pCore->GetPeer(namedObject.get(), &dxamlLayerObject));
    CValueBoxer::UnwrapExternalObjectReferenceIfPresent(ctl::as_iinspectable(dxamlLayerObject.Get()), ppReturnValue);
    return S_OK;
}

_Check_return_ HRESULT
FrameworkElement::SetBinding(_In_ const CDependencyProperty* pProperty, _In_ xaml_data::IBindingBase* binding)
{
    HRESULT hr = S_OK;
    Binding *pBinding = NULL;
    xaml_data::IBinding *pBindingIface = NULL;

    IFC(ctl::do_query_interface(pBindingIface, binding));
    pBinding  = static_cast<Binding*>(pBindingIface);
    IFC(SetBindingCore(pProperty, pBinding));

Cleanup:
    ReleaseInterface(pBindingIface);
    RRETURN(hr);
}

_Check_return_ HRESULT
FrameworkElement::SetBindingImpl(_In_ IDependencyProperty* pDP, _In_ xaml_data::IBindingBase* binding)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<Binding> spBinding;
    ctl::ComPtr<DependencyPropertyHandle> spDP;

    IFC(ctl::do_query_interface(spBinding, binding));
    IFC(ctl::do_query_interface(spDP, pDP));

    IFC(SetBindingCore(spDP->GetDP(), spBinding.Get()));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
FrameworkElement::GetBindingExpression(_In_ const CDependencyProperty* pProperty, _In_ xaml_data::IBindingExpression **ppReturnValue)
{
    HRESULT hr = S_OK;
    IInspectable* pValue = NULL;
    xaml_data::IBindingExpression* pBindingExpressionIface = NULL;

    ARG_VALIDRETURNPOINTER(ppReturnValue);

    // Get the unevaluated expression value
    IFC(ReadLocalValue(pProperty, &pValue));

    // Attempt to query the IBindingExpression interface
    // This is not IFC since unsupported interface should be returned with a NULL pointer
    // and not an exception (which matches Silverlight behavior and documentation)
    if (SUCCEEDED(ctl::do_query_interface(pBindingExpressionIface, pValue)))
    {
        *ppReturnValue = pBindingExpressionIface;
    }
    else
    {
        *ppReturnValue = NULL;
    }

Cleanup:
    ReleaseInterface(pValue);
    RRETURN(hr);
}

_Check_return_ HRESULT
FrameworkElement::GetBindingExpressionImpl(_In_ IDependencyProperty *pdp, _In_ xaml_data::IBindingExpression **ppReturnValue)
{
    HRESULT hr = S_OK;
    const CDependencyProperty* pUnderlyingDP = NULL;
    IInspectable* pValue = NULL;
    xaml_data::IBindingExpression* pBindingExpressionIface = NULL;

    ARG_VALIDRETURNPOINTER(ppReturnValue);

    pUnderlyingDP = static_cast<DependencyPropertyHandle*>(pdp)->GetDP();

    // Get the unevaluated expression value
    IFC(ReadLocalValue(pUnderlyingDP, &pValue));

    // Attempt to query the IBindingExpression interface
    // This is not IFC since unsupported interface should be returned with a NULL pointer
    // and not an exception (which matches Silverlight behavior and documentation)
    if (SUCCEEDED(ctl::do_query_interface(pBindingExpressionIface, pValue)))
    {
        *ppReturnValue = pBindingExpressionIface;
    }
    else
    {
        *ppReturnValue = NULL;
    }

Cleanup:
    ReleaseInterface(pValue);
    RRETURN(hr);
}

_Check_return_ HRESULT
FrameworkElement::get_BaseUriImpl(_Outptr_ wf::IUriRuntimeClass **ppBaseUri)
{
    HRESULT hr = S_OK;
    CValue boxedValue;

    IFC(CoreImports::DependencyObject_GetBaseUri(static_cast<CFrameworkElement*>(GetHandle()), &boxedValue));

    if (boxedValue.IsNull())
    {
        *ppBaseUri = nullptr;
    }
    else
    {
        IFC(CValueBoxer::UnboxValue(&boxedValue, ppBaseUri));
    }

Cleanup:
    RRETURN(hr);
}

// Apply a template to the element.
_Check_return_ HRESULT FrameworkElement::OnApplyTemplateFromCore(
    _In_ CFrameworkElement* nativeTarget)
{
    HRESULT hr = S_OK;
    DependencyObject* pPeer = NULL;
    IFrameworkElement* pPeerAsFE = NULL;

    IFCPTR(nativeTarget);

    // Get the framework peer
    IFC(DXamlCore::GetCurrent()->GetPeer(nativeTarget, &pPeer));

    // Call OnApplyTemplate
    IFC(ctl::do_query_interface(pPeerAsFE, pPeer));
    IFC(static_cast<FrameworkElement*>(pPeerAsFE)->OnApplyTemplateProtected());

Cleanup:
    ctl::release_interface(pPeer);
    ReleaseInterface(pPeerAsFE);
    RRETURN(hr);
}

// Provides the behavior for the Measure pass of layout. Classes can override
// this method to define their own Measure pass behavior.
_Check_return_ HRESULT
FrameworkElement::MeasureOverrideImpl(
    _In_ wf::Size availableSize,
    _Out_ wf::Size* returnValue)
{
    HRESULT hr = S_OK;
    FLOAT width = 0.0;
    FLOAT height = 0.0;

    IFC(CoreImports::FrameworkElement_MeasureOverride(
        static_cast<CFrameworkElement*>(GetHandle()),
        availableSize.Width,
        availableSize.Height,
        &width,
        &height));

    returnValue->Width = width;
    returnValue->Height = height;

Cleanup:
    RRETURN(hr);
}


// Provides the behavior for the Measure pass of layout.
_Check_return_ HRESULT
FrameworkElement::MeasureOverrideFromCore(
    _In_ CFrameworkElement* nativeTarget,
    _In_ XFLOAT inWidth,
    _In_ XFLOAT inHeight,
    _Out_ XFLOAT* outWidth,
    _Out_ XFLOAT* outHeight)
{
    HRESULT hr = S_OK;
    DependencyObject* pPeer = NULL;
    wf::Size availableSize = {};
    wf::Size desiredSize = {};

    IFCPTR(nativeTarget);
    IFCPTR(outWidth);
    IFCPTR(outHeight);
    *outWidth = 0.0;
    *outHeight = 0.0;

    // Get the framework peer
    IFC(DXamlCore::GetCurrent()->GetPeer(nativeTarget, &pPeer));

    availableSize.Width = inWidth;
    availableSize.Height = inHeight;

    // Invoke the override
    IFCEXPECT(ctl::is<IFrameworkElement>(pPeer));
    IFC(static_cast<FrameworkElement*>(pPeer)->MeasureOverrideProtected(availableSize, &desiredSize));

    // Enforce that MeasureOverride cannot return Infinity or NaN size even
    // if given Infinite available size.
    if (DoubleUtil::IsInfinity(desiredSize.Width) ||
        DoubleUtil::IsInfinity(desiredSize.Height) ||
        DoubleUtil::IsNaN(desiredSize.Width) ||
        DoubleUtil::IsNaN(desiredSize.Height))
    {
        // TODO: Throw exception
        IFC(E_FAIL);
    }

    *outWidth = desiredSize.Width;
    *outHeight = desiredSize.Height;

Cleanup:
    ctl::release_interface(pPeer);
    RRETURN(hr);
}

// Provides the behavior for the Arrange pass of layout. Classes can override
// this method to define their own Arrange pass behavior.
_Check_return_ HRESULT
FrameworkElement::ArrangeOverrideImpl(
    _In_ wf::Size finalSize,
    _Out_ wf::Size* returnValue)
{
    HRESULT hr = S_OK;
    FLOAT width = 0.0;
    FLOAT height = 0.0;

    IFC(CoreImports::FrameworkElement_ArrangeOverride(
        static_cast<CFrameworkElement*>(GetHandle()),
        finalSize.Width,
        finalSize.Height,
        &width,
        &height));

    returnValue->Width = width;
    returnValue->Height = height;

Cleanup:
    RRETURN(hr);
}

// Provides the behavior for the Arrange pass of layout.
_Check_return_ HRESULT
FrameworkElement::ArrangeOverrideFromCore(
    _In_ CFrameworkElement* nativeTarget,
    _In_ XFLOAT inWidth,
    _In_ XFLOAT inHeight,
    _Out_ XFLOAT* outWidth,
    _Out_ XFLOAT* outHeight)
{
    HRESULT hr = S_OK;
    DependencyObject* pPeer = NULL;
    wf::Size finalSize = {};
    wf::Size usedSize = {};

    IFCPTR(nativeTarget);
    IFCPTR(outWidth);
    IFCPTR(outHeight);
    *outWidth = 0.0;
    *outHeight = 0.0;

    // Get the framework peer
    IFC(DXamlCore::GetCurrent()->GetPeer(nativeTarget, &pPeer));

    finalSize.Width = inWidth;
    finalSize.Height = inHeight;

    // Invoke the override
    IFCEXPECT(ctl::is<IFrameworkElement>(pPeer));
    IFC(static_cast<FrameworkElement*>(pPeer)->ArrangeOverrideProtected(finalSize, &usedSize));

    // Enforce that ArrangeOverride cannot return Infinity or NaN size even
    // if given Infinite available size.
    if (DoubleUtil::IsInfinity(usedSize.Width) ||
        DoubleUtil::IsInfinity(usedSize.Height) ||
        DoubleUtil::IsNaN(usedSize.Width) ||
        DoubleUtil::IsNaN(usedSize.Height))
    {
        // TODO: Throw exception
        IFC(E_FAIL);
    }

    *outWidth = usedSize.Width;
    *outHeight = usedSize.Height;

Cleanup:
    ctl::release_interface(pPeer);
    RRETURN(hr);
}

_Check_return_ HRESULT
FrameworkElement::GetLogicalParentForAPCore(
    _In_ CDependencyObject* nativeTarget,
    _Outptr_ CDependencyObject** ppLogicalParentForAP)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<DependencyObject> spPeer;
    ctl::ComPtr<DependencyObject> spLogicalParentForAP;

    IFCPTR(ppLogicalParentForAP);
    *ppLogicalParentForAP = nullptr;

    if (SUCCEEDED(DXamlCore::GetCurrent()->TryGetPeer(nativeTarget, &spPeer)) && spPeer)
    {
        ctl::ComPtr<IFrameworkElement> spPeerAsFE = spPeer.AsOrNull<IFrameworkElement>();
        if (spPeerAsFE)
        {
            IFC(spPeerAsFE.Cast<FrameworkElement>()->GetLogicalParentForAPProtected(&spLogicalParentForAP));
            if (spLogicalParentForAP)
            {
                *ppLogicalParentForAP = spLogicalParentForAP->GetHandleAddRef();
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT FrameworkElement::GetPlainText(_Out_ HSTRING* strPlainText)
{
    HRESULT hr = S_OK;

    IFC(wrl_wrappers::HStringReference(L"",0).CopyTo(strPlainText));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT FrameworkElement::GetStringFromObject(
        _In_ IInspectable* pObject,
        _Out_ HSTRING* returnValue)
{
    IFCPTR_RETURN(returnValue);
    *returnValue = NULL;

    // First, try IFrameworkElement
    ctl::ComPtr<xaml::IFrameworkElement> spFrameworkElement;
    spFrameworkElement.Attach(ctl::query_interface<xaml::IFrameworkElement>(pObject));
    if (spFrameworkElement)
    {
        IFC_RETURN(static_cast<FrameworkElement*>(spFrameworkElement.Get())->GetPlainText(returnValue));
        return S_OK;
    }

    // Try IPropertyValue
    ctl::ComPtr<wf::IPropertyValue> spPropertyValue;
    spPropertyValue.Attach(ctl::query_interface<wf::IPropertyValue>(pObject));
    if (spPropertyValue)
    {
        wf::PropertyType propertyType;
        IFC_RETURN(spPropertyValue->get_Type(&propertyType));

        if (ValueConversionHelpers::CanConvertValueToString(propertyType))
        {
            IFC_RETURN(ValueConversionHelpers::ConvertValueToString(spPropertyValue.Get(), propertyType, returnValue));
        }

        return S_OK;
    }

    // Try ICustomPropertyProvider
    ctl::ComPtr<xaml_data::ICustomPropertyProvider> spCustomPropertyProvider;
    spCustomPropertyProvider.Attach(ctl::query_interface<xaml_data::ICustomPropertyProvider>(pObject));
    if (spCustomPropertyProvider)
    {
        IFC_RETURN(spCustomPropertyProvider->GetStringRepresentation(returnValue));
        return S_OK;
    }

    // Finally, Try IStringable
    ctl::ComPtr<wf::IStringable> spStringable;
    spStringable.Attach(ctl::query_interface<wf::IStringable>(pObject));
    if (spStringable)
    {
        IFC_RETURN(spStringable->ToString(returnValue));
    }


    return S_OK;
}

void FrameworkElement::NotifyOfDataContextChange(_In_ const DataContextChangedParams& args)
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES
    ctl::ComPtr<IToolTip> spToolTip;
    ctl::ComPtr<xaml::IDataContextChangedEventArgs> spArgs;
    DataContextChangedEventSourceType* pEventSource = nullptr;
    CValue newDataContext;
    DataContextChangedParams tempArgs(args.m_pOriginalSourceNoRef, args.m_dataContextChangedReason);
    const DataContextChangedParams* pArgsToForward = &args;
    BOOLEAN bPropagationHandled = FALSE;

    // Raise the internal DataContextChanged event, notifying the expressions interested in the local DC.
    // If this FrameworkElement has a DataContext with a {Binding}, the following call will
    // update the value of FrameworkElement.DataContext.
    IFC(NotifyBindingExpressions(args));

    // Avoid walking children twice when an element has a DataContext with a live {Binding}. We make an exception
    // when entering the tree, because we need all DataContexts to get re-evaluated.
    BOOLEAN fIsRelevant = (!m_pDataContextChangedSource || !IsDataContextBound() || args.m_pOriginalSourceNoRef == this || args.m_dataContextChangedReason == EnteringLiveTree);
    if (fIsRelevant)
    {
        // If the DataContext property has a {Binding}, pick up the new value of our DataContext, instead of
        // continuing to pass on the DataContext from our parent.
        if (m_fIsDataContextBound && args.m_fResolvedNewDataContext && (args.m_pOriginalSourceNoRef != this))
        {
            IFC(GetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_DataContext, newDataContext));
            tempArgs.m_pNewDataContext = &newDataContext;
            tempArgs.m_fResolvedNewDataContext = TRUE;
            pArgsToForward = &tempArgs;
        }

        // Raise the public DataContextChanged event if there are listeners.
        if (ShouldRaiseEvent(KnownEventIndex::FrameworkElement_DataContextChanged))
        {
            DXamlCore* pCore = DXamlCore::GetCurrent();
            ctl::ComPtr<IInspectable> spNewDataContext;

            if (pArgsToForward->m_fResolvedNewDataContext)
            {
                IFC(pArgsToForward->GetNewDataContext(&spNewDataContext));
            }
            else
            {
                IFC(get_DataContext(&spNewDataContext));
            }

            IFC(pCore->GetDataContextChangedEventArgsFromPool(spNewDataContext.Get(), &spArgs));

            IFC(GetDataContextChangedEventSourceNoRef(&pEventSource));
            {
                SuspendFailFastOnStowedException suspender; // ensure we don't FailFast due to an error within Raise
                IFC(pEventSource->Raise(this, spArgs.Get()));
            }
            IFC(spArgs->get_Handled(&bPropagationHandled));

            IFC(pCore->ReleaseDataContextChangedEventArgsToPool(spArgs.Get()));
        }

        if (!bPropagationHandled)
        {
            // Propagate the notification down the visual tree
            IFC(PropagateDataContextChanged(*pArgsToForward));

            // If there is a ToolTip registered for this element, we need to push the new DataContext to it.
            IFC(ToolTipServiceFactory::GetToolTipObjectStatic(this, &spToolTip));
            if (spToolTip != nullptr)
            {
                if (pArgsToForward->m_fResolvedNewDataContext)
                {
                    IFC(spToolTip.Cast<ToolTip>()->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_DataContext, *pArgsToForward->m_pNewDataContext));
                }
                else
                {
                    IFC(GetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_DataContext, newDataContext));
                    IFC(spToolTip.Cast<ToolTip>()->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_DataContext, newDataContext));
                }
            }
        }
    }

Cleanup:
    return;
}

_Check_return_ HRESULT
FrameworkElement::GetEffectiveDataContext(_Outptr_ IInspectable** ppValue)
{
    ctl::ComPtr<DependencyObject> spParentAsDO;
    ctl::ComPtr<xaml::IDependencyObject> spParentAsIDO;
    ctl::ComPtr<FrameworkElement> spParentAsFE;

    *ppValue = NULL;

    // First try to use the logical parent to get the DC
    IFC_RETURN(GetInheritanceParent(&spParentAsDO));
    if (spParentAsDO)
    {
        spParentAsFE = spParentAsDO.AsOrNull<FrameworkElement>();
        if (spParentAsFE)
        {
            IFC_RETURN(spParentAsFE->get_DataContext(ppValue));
        }
    }

    // If nothing was found this way then try the visual tree.
    if (!spParentAsFE)
    {
        IFC_RETURN(VisualTreeHelper::GetParentStatic(this, &spParentAsIDO));
        if (spParentAsIDO)
        {
            spParentAsFE = spParentAsIDO.AsOrNull<FrameworkElement>();
            if (spParentAsFE)
            {
                IFC_RETURN(spParentAsFE->get_DataContext(ppValue));
            }
        }
    }

    // If we haven't found anything yet then give up.
    return S_OK;
}

_Check_return_ HRESULT FrameworkElement::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;

    IFC(FrameworkElementGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::FrameworkElement_DataContext:
            {
                DataContextChangedParams dcArgs(this, NewDataContext, *args.m_pNewValue, args.m_pNewValueOuterNoRef);
                NotifyOfDataContextChange(dcArgs);
            }
            break;
        case KnownPropertyIndex::FrameworkElement_Style:
            // Track whether the FrameworkElement.Style property has been set by
            // the ItemsControl or the end user.  ItemsControl::ApplyItemContainerStyle will
            // explicitly set this to TRUE after setting the ItemContainerStyle on a container.
            SetIsStyleSetFromItemsControl(FALSE);
            break;
    }

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP FrameworkElement::get_Parent(
    _Outptr_ xaml::IDependencyObject** pValue)
{
    HRESULT hr = S_OK;

    // Don't return the parent unless we're in the live tree.
    if (!IsInLiveTree())
    {
        *pValue = NULL;
        goto Cleanup;
    }

    IFC(FrameworkElementGenerated::get_Parent(pValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT FrameworkElement::HasParent(_Out_ BOOLEAN* pHasParent)
{
    HRESULT hr = S_OK;
    CDependencyObject* returnValueNative = NULL;

    *pHasParent = FALSE;
    IFC(CFrameworkElement::get_Parent(static_cast<CFrameworkElement*>(GetHandle()), &returnValueNative));
    *pHasParent = returnValueNative != NULL;

Cleanup:
    ReleaseInterface(returnValueNative);
    RRETURN(hr);
}

_Check_return_ HRESULT FrameworkElement::TryGetParent(_Outptr_ xaml::IDependencyObject** pValue)
{
    HRESULT hr = S_OK;

    DependencyObject* pReturnValuePeer = NULL;
    CDependencyObject* returnValueNative = NULL;

    *pValue = NULL;
    IFC(CFrameworkElement::get_Parent(static_cast<CFrameworkElement*>(GetHandle()), &returnValueNative));

    if (returnValueNative)
    {
        IFC(DXamlCore::GetCurrent()->TryGetPeer(returnValueNative, &pReturnValuePeer));
        *pValue = pReturnValuePeer;
    }

Cleanup:
    ReleaseInterface(returnValueNative);
    RRETURN(hr);
}

// Public methods not on the public interface
_Check_return_
HRESULT
FrameworkElement::GetDataContextChangedSource(_Outptr_ IDataContextChangedEventSource **ppSource)
{
    HRESULT hr = S_OK;

    if (m_pDataContextChangedSource == NULL)
    {
        IFC(MarkHasState());
        IFC(ctl::ComObject<DataContextChangedEventSource>::CreateInstance(&m_pDataContextChangedSource));
    }

    *ppSource = m_pDataContextChangedSource;
    ctl::addref_interface(m_pDataContextChangedSource);

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
FrameworkElement::NotifyBindingExpressions(_In_ const DataContextChangedParams& args)
{
    HRESULT hr = S_OK;

    if (m_pDataContextChangedSource)
    {
        IFC(m_pDataContextChangedSource->Raise(this, &args));
    }

Cleanup:
    RRETURN(hr);
}

// static
// Propagate DataContext down the core tree, because corresponding peers are not available
_Check_return_
HRESULT
FrameworkElement::PropagateDataContextChangedInCoreTree(
    _In_ CFrameworkElement *pElementCore,
    _In_ const DataContextChangedParams& args)
{
    HRESULT hr = S_OK;
    unsigned int cChildren = 0;
    CCollection* pChildrenCoreNoRef = NULL;
    CUIElement* pChildCore = NULL;
    DependencyObject* pChild = NULL;
    DXamlCore* pCore = DXamlCore::GetCurrent();

    pChildrenCoreNoRef = pElementCore->GetChildren();
    if (pChildrenCoreNoRef)
    {
        // Propagate DataContext to children
        cChildren = pChildrenCoreNoRef->GetCount();
        for (unsigned int i =  0; i < cChildren; i++)
        {
            pChildCore = static_cast<CUIElement*>(pChildrenCoreNoRef->GetItemWithAddRef(i));
            if (do_pointer_cast<CFrameworkElement>(pChildCore))
            {
                IFC(pCore->TryGetPeer(pChildCore, &pChild));
                if (pChild)
                {
                    // Peer is available, so use peer to propagate
                    IGNOREHR(static_cast<FrameworkElement*>(pChild)->OnAncestorDataContextChanged(args));
                }
                else
                {
                    // Peer is not available, so use the core tree to propagate
                    IGNOREHR(PropagateDataContextChangedInCoreTree(static_cast<CFrameworkElement*>(pChildCore), args));
                }
            }

            ctl::release_interface(pChild);
            ReleaseInterface(pChildCore);
        }
    }

Cleanup:
    ReleaseInterface(pChildCore);
    ctl::release_interface(pChild);
    RRETURN(hr);
}

_Check_return_
HRESULT
FrameworkElement::PropagateDataContextChanged(_In_ const DataContextChangedParams& args)
{
    HRESULT hr = S_OK;
    INT nChildCount = 0;
    CFrameworkElement* pFECoreNoRef = NULL;
    CCollection* pChildrenCoreNoRef = NULL;
    CUIElement* pChildCore = NULL;
    DependencyObject* pChild = NULL;
    IFrameworkElement* pFE = NULL;

    DXamlCore* pCore = DXamlCore::GetCurrent();

    pFECoreNoRef = static_cast<CFrameworkElement*>(GetHandle());
    pChildrenCoreNoRef = pFECoreNoRef->GetChildren();
    if (pChildrenCoreNoRef)
    {
        nChildCount = pChildrenCoreNoRef->GetCount();
        for (INT nCurrent = 0; nCurrent < nChildCount; nCurrent++)
        {
            pChildCore = static_cast<CUIElement*>(pChildrenCoreNoRef->GetItemWithAddRef(nCurrent));

            // If the child doesn't already have a peer, it doesn't have a {Binding} anyway, so skip.
            IFC(pCore->TryGetPeer(pChildCore, &pChild));
            if (pChild)
            {
                // See if this is a framework element
                // TODO: Check the type internally perhaps?
                pFE = ctl::query_interface<IFrameworkElement>(pChild);
                if (pFE)
                {
                    // As we propagate down the tree we might encounter errors because
                    // not all of the objects are, yet, creatable in the DirectUI layer.
                    // Ignore those errors as those objects will not have bindings on them.
                    // NOTE: This will happen on
                    IGNOREHR(static_cast<FrameworkElement*>(pFE)->OnAncestorDataContextChanged(args));
                }
                ReleaseInterface(pFE);
            }
            else
            {
                // This child doesn't participate in peer tree, but could have descendants
                // that do participate, so propagate down the core tree
                if (do_pointer_cast<CFrameworkElement>(pChildCore))
                {
                    IGNOREHR(PropagateDataContextChangedInCoreTree(static_cast<CFrameworkElement*>(pChildCore), args));
                }
            }

            ctl::release_interface(pChild);
            ReleaseInterface(pChildCore);
        }
    }

Cleanup:
    ReleaseInterface(pChildCore);
    ctl::release_interface(pChild);
    ReleaseInterface(pFE);
    RRETURN(hr);
}


_Check_return_
HRESULT
FrameworkElement::OnAncestorDataContextChanged(_In_ const DataContextChangedParams& args)
{
    HRESULT hr = S_OK;
    BOOLEAN fIsContextChangeRelevant = FALSE;

    // Only process the change if we care about it
    IFC(IsDataContextChangeRelevant(args, &fIsContextChangeRelevant));
    if (!fIsContextChangeRelevant)
    {
        goto Cleanup;
    }

    NotifyOfDataContextChange(args);

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
FrameworkElement::IsDataContextChangeRelevant(_In_ const DataContextChangedParams& args, _Out_ BOOLEAN *pfIsDataContextChangeRelevant)
{
    HRESULT hr = S_OK;
    BOOLEAN fIsPropertyLocal = FALSE;

    if (m_fIsDataContextBound)
    {
        // If the data context is data bound then any changes to the
        // ancestor data context is relevant to us, the local one will be
        // set to the expression and would otherwise prevent the
        // DC from being propagated down the tree
        *pfIsDataContextChangeRelevant = TRUE;
    }
    else if (args.m_dataContextChangedReason == NewDataContext)
    {
        IFC(IsPropertyLocal(
            MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::FrameworkElement_DataContext),
            &fIsPropertyLocal));

        // We only care about the DC change if the local DC is not set
        *pfIsDataContextChangeRelevant = !fIsPropertyLocal;
    }
    else if (args.m_dataContextChangedReason == EnteringLiveTree)
    {
        *pfIsDataContextChangeRelevant = TRUE;
    }
    else
    {
        *pfIsDataContextChangeRelevant = FALSE;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
FrameworkElement::OnTreeParentUpdated(_In_opt_ CDependencyObject *pNewParent, _In_ BOOLEAN isParentAlive)
{
    HRESULT hr = S_OK;
    DataContextChangedParams dcArgs(this, EnteringLiveTree);

    IFC(FrameworkElementGenerated::OnTreeParentUpdated(pNewParent, isParentAlive));

    // We're only interested on the parent if we're entring the live
    // tree, avoid spurious change notifications as the tree is being built
    if (!isParentAlive)
    {
        goto Cleanup;
    }

    NotifyOfDataContextChange(dcArgs);

Cleanup:

    RRETURN(hr);
}

//static
_Check_return_ HRESULT FrameworkElement::PropagateDataContextChange(
    _In_ CFrameworkElement* pChildCore)
{
    // pOriginalSource == nullptr because pChildCore doesn't have a peer
    DataContextChangedParams args(nullptr, EnteringLiveTree);

    return PropagateDataContextChangedInCoreTree(pChildCore, args);
}

// Gets a reference to the template parent of this element. This
// property is not relevant if the element was not created through a
// template.
_Check_return_ HRESULT FrameworkElement::get_TemplatedParent(
    _Outptr_ DependencyObject** ppTemplatedParent)
{
    IFCPTR_RETURN(ppTemplatedParent);
    *ppTemplatedParent = nullptr;

    CDependencyObject* parent = GetHandle()->GetTemplatedParent();

    if (parent)
    {
        IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(
            parent,
            ppTemplatedParent));
    }

    return S_OK;
}

IFACEMETHODIMP FrameworkElement::add_LayoutUpdated(
        _In_ wf::IEventHandler<IInspectable*>* pHandler,
    _Out_ EventRegistrationToken* ptToken)
{
    HRESULT hr = S_OK;
    LayoutUpdatedEventSourceType* pLayoutUpdatedEventSource = nullptr;

    // Let the core know we're listening to LayoutUpdated
    IFC(GetLayoutUpdatedEventSourceNoRef(&pLayoutUpdatedEventSource));
    if (!pLayoutUpdatedEventSource->HasHandlers())
    {
        // Tell the core we're interested in LayoutUpdated callbacks for this object.
        IFC(CoreImports::WantsEvent(GetHandle(), ManagedEvent::ManagedEventLayoutUpdated, true));

        // Get a weak reference to this event source and register it as one which needs to
        // be invoked when we get the LayoutUpdated callback from the core.
        IFC(DXamlCore::GetCurrent()->RegisterLayoutUpdatedEventSource(this));
    }

    // Add the handler
    IFC(FrameworkElementGenerated::add_LayoutUpdated(pHandler, ptToken));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP FrameworkElement::remove_LayoutUpdated(
    _In_ EventRegistrationToken tToken)
{
    ctl::ComPtr<LayoutUpdatedEventSourceType> layoutUpdatedEventSource;

    {
        LayoutUpdatedEventSourceType* layoutUpdatedEventSourceNoRef = nullptr;
        IFC_RETURN(GetLayoutUpdatedEventSourceNoRef(&layoutUpdatedEventSourceNoRef));
        layoutUpdatedEventSource = layoutUpdatedEventSourceNoRef;
    }

    BOOLEAN hadHandlers = layoutUpdatedEventSource->HasHandlers();

    // Remove the handler
    IFC_RETURN(FrameworkElementGenerated::remove_LayoutUpdated(tToken));

    // Check if we can tell the core we're no longer listening to LayoutUpdated
    if (hadHandlers && !layoutUpdatedEventSource->HasHandlers() && GetHandle() != nullptr) // Note: GetHandle can return NULL during shutdown.
    {
        // Tell the core we're no longer interested in LayoutUpdated callbacks for this object.
        IFC_RETURN(CoreImports::WantsEvent(GetHandle(), ManagedEvent::ManagedEventLayoutUpdated, false));

        // Remove self from global LayoutUpdated callback queue.
        DXamlCore::GetCurrent()->UnregisterLayoutUpdatedEventSource(this);
    }

    return S_OK;
}

_Check_return_ HRESULT FrameworkElement::RaiseLayoutUpdated(
    _In_ IInspectable* pSender,
    _In_ IInspectable* pArgs)
{
    HRESULT hr = S_OK;

    // Get the LayoutUpdated event source on this element and raise the event

    LayoutUpdatedEventSourceType* pLayoutUpdatedEventSource = nullptr;
    IFC(GetLayoutUpdatedEventSourceNoRef(&pLayoutUpdatedEventSource));
    IFC(pLayoutUpdatedEventSource->UntypedRaise(pSender, pArgs));

Cleanup:
    RRETURN(hr);
}

// Used by VisualStateManager to allow the FrameworkElement to react to calls to
// VisualStateManager.GoToState on its templated parent (or itself, if this is a UserControl).
// Don't call this method directly - use InvokeGoToElementStateWithControl.
_Check_return_ HRESULT FrameworkElement::GoToElementStateCoreImpl(
    _In_ HSTRING stateName,
    _In_ BOOLEAN useTransitions,
    _Out_ BOOLEAN* returnValue)
{
    HRESULT hr = S_OK;
    if (m_pChainedGoToElementStateSourceControls && !m_pChainedGoToElementStateSourceControls->empty())
    {
        // Invoke the default VSM processing, using the IControl provided by InvokeGoToElementStateWithControl.
        IFC(VisualStateManager::GoToStateWithDefaultVSM(m_pChainedGoToElementStateSourceControls->back(), this, stateName, useTransitions, returnValue));
    }
    else
    {
        // Someone called the method directly on us. This isn't supported.
        IFC(E_UNEXPECTED);
    }

Cleanup:
    RRETURN(hr);
}

// Our default implementation of GoToElementStateCoreImpl needs to know the control that GoToState is
// being called on, so it can eventually raise events on the VSGs. However, this control isn't part of the
// public API (by design). This method allows us to hack in knowledge of this control by acting as a
// wrapper around GoToElementStateCoreImpl calls. Sadly we need this behavior so delvelopers can call base
// in GoToElementStateCoreImpl and have it work as expected.
_Check_return_ HRESULT FrameworkElement::InvokeGoToElementStateWithControl(
    _In_ IControl* pControl,
    _In_ HSTRING stateName,
    _In_ BOOLEAN bUseTransitions,
    _Out_ BOOLEAN* retval)
{
    HRESULT hr = S_OK;
    HRESULT hr2 = S_OK;

    if (!m_pChainedGoToElementStateSourceControls)
    {
        m_pChainedGoToElementStateSourceControls = new std::vector<xaml_controls::IControl*>();
    }

    // Save off the control in case someone calls base in GoToElementStateCore (or there isn't any override).
    m_pChainedGoToElementStateSourceControls->push_back(pControl);

    hr2 = GoToElementStateCoreProtected(stateName, bUseTransitions, retval);

    // Get rid of the stored IControl - it's only valid for this call to InvokeGoToElementStateWithControl.
    // Not guaranteeing the release of this IControl will cause this list to potentially grow without bound
    // if we keep getting errors.
    m_pChainedGoToElementStateSourceControls->pop_back();


    // Give any failure to GoToElementStateCoreProtected priority.
    if (!SUCCEEDED(hr2))
    {
        RRETURN(hr2);
    }

    RRETURN(hr);//RRETURN_REMOVAL
}

// Occurs when either the ActualHeight or the ActualWidth properties change
// value on a FrameworkElement.
IFACEMETHODIMP FrameworkElement::add_SizeChanged(
    _In_ xaml::ISizeChangedEventHandler* pHandler,
    _Out_ EventRegistrationToken* ptToken)
{
    HRESULT hr = S_OK;
    SizeChangedEventSourceType* pSizeChangedEventSource = nullptr;

    // Let the core know we're listening to SizeChanged
    IFC(GetSizeChangedEventSourceNoRef(&pSizeChangedEventSource));
    if (!pSizeChangedEventSource->HasHandlers())
    {
        IFC(CoreImports::WantsEvent(GetHandle(), ManagedEvent::ManagedEventSizeChanged, true));
    }

    // Add the handler
    IFC(FrameworkElementGenerated::add_SizeChanged(pHandler, ptToken));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP FrameworkElement::remove_SizeChanged(
    _In_ EventRegistrationToken tToken)
{
    HRESULT hr = S_OK;
    SizeChangedEventSourceType* pSizeChangedEventSource = nullptr;

    // Remove the handler
    IFC(FrameworkElementGenerated::remove_SizeChanged(tToken));

    // Check if we can tell the core we're no longer listening to SizeChanged
    IFC(GetSizeChangedEventSourceNoRef(&pSizeChangedEventSource));
    if (!pSizeChangedEventSource->HasHandlers() && GetHandle() != NULL) // Note: GetHandle can return NULL during shutdown.
    {
        IFC(CoreImports::WantsEvent(GetHandle(), ManagedEvent::ManagedEventSizeChanged, false));
    }

Cleanup:
    RRETURN(hr);
}

// Raise the SizeChanged event
_Check_return_ HRESULT FrameworkElement::OnSizeChanged(
    _In_ IInspectable* pSender,
    _In_ xaml::ISizeChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    SizeChangedEventSourceType* pSizeChangedEventSource = nullptr;

    IFC(GetSizeChangedEventSourceNoRef(&pSizeChangedEventSource));
    IFC(pSizeChangedEventSource->Raise(pSender, pArgs));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT FrameworkElement::InvalidateViewportImpl()
{
    // If we're in the process of shutting down, this UIElement might have
    // already been disconnected from the core. If that is the case, GetHandle
    // will return null, and the subsequent call to InvalidateViewport will
    // result in an access violation. In order to prevent this, we're
    // explicitly checking for null here. Note: This is not intended to
    // establish a precedent and, if similar issues arise in the future, we
    // will need a full-fledged solution that would work across the board.
    // We're opting for the current approach because InvalidateViewport
    // in particular does not have any externally visible consequences.
    // (See RS Bug #1563794).
    CUIElement* element = static_cast<CUIElement*>(GetHandle());

    if (element)
    {
        // InvalidateViewport can only be called on elements that have been
        // registered as scroll ports.
        if (element->IsScroller())
        {
            element->InvalidateViewport();
        }
        else
        {
            IFC_RETURN(ErrorHelper::OriginateErrorUsingResourceID(E_FAIL, ERROR_INVALIDATEVIEWPORT_REQUIRES_SCROLLER));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT FrameworkElement::GetInheritanceParent(_Outptr_ DependencyObject **ppParent)
{
    CDependencyObject* pInheritanceParent = GetHandle()->GetInheritanceParentInternal(TRUE);

    if (pInheritanceParent)
    {
        IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(pInheritanceParent, ppParent));
    }

    return S_OK;
}

_Check_return_ HRESULT FrameworkElement::FindNameInPage(
    _In_ HSTRING strElementName,
    _In_ bool fIsCalledFromUserControl,
    _Outptr_ IInspectable **ppObj)
{
    ctl::ComPtr<FrameworkElement> current;
    ctl::ComPtr<DependencyObject> templatedParentAsDO;
    BOOLEAN fIsItemsHost = false;
    ctl::ComPtr<IInspectable> foundElementValue;

    current = this;

    IFC_RETURN(current->FindName(strElementName, &foundElementValue));

    while (!foundElementValue && current)
    {
        IFC_RETURN(current->get_TemplatedParent(&templatedParentAsDO));
        if ((templatedParentAsDO) && templatedParentAsDO.AsOrNull<IFrameworkElement>())
        {
            templatedParentAsDO.As(&current);
        }
        else
        {
            // ListBoxItem(s) inside of an ItemsControl do not have a
            // templated parent, so the TemplatedParent chain is broken.
            // To work around this, we get their parent and if it is a panel
            // serving as an ItemsHost in an ItemsControl then we use that panel
            // as the next step in the chain. The Panel will have the TemplatedParent
            // set since it is created from a Template by the ItemsPresenter.
            // A MUXC ItemsRepeater may not have a TemplatedParent either, so that
            // panel is used as the next step in the chain as well. This allows its
            // ItemTemplate to use ElementName-based bindings to elements outside its scope.
            IFC_RETURN(current->GetInheritanceParent(&templatedParentAsDO));
            if (!templatedParentAsDO)
            {
                return S_OK;
            }

            ctl::ComPtr<IPanel> panel;
            templatedParentAsDO.As(&panel);
            current = nullptr;

            if (panel)
            {
                // Two kinds of panels get special treatment:
                // - items host for an ItemsControl,
                // - MUXC ItemsRepeater
                // In both cases, ElementName-based bindings need to be able to successfully find elements outside the ItemsControl or ItemsRepeater respectively.
                IFC_RETURN(panel->get_IsItemsHost(&fIsItemsHost));
                if (fIsItemsHost ||
                    static_cast<CFrameworkElement*>(templatedParentAsDO->GetHandle())->GetClassName().Equals(XSTRING_PTR_EPHEMERAL(L"Microsoft.UI.Xaml.Controls.ItemsRepeater")))
                {
                    current = templatedParentAsDO.Cast<FrameworkElement>();
                }
            }
        }

        if (current)
        {
            IFC_RETURN(current->FindName(strElementName, &foundElementValue));
        }
    }

    foundElementValue.CopyTo<IInspectable>(ppObj);
    return S_OK;
}

_Check_return_
HRESULT
FrameworkElement::HasFocus(_Out_ BOOLEAN *pbHasFocus)
{
    HRESULT hr = S_OK;
    DependencyObject *pFocused = NULL;
    xaml::IDependencyObject *pCurrentAsDO = NULL;
    xaml::IDependencyObject *pThisAsDO = NULL;
    xaml::IDependencyObject *pParentAsDO = NULL;

    IFCPTR(pbHasFocus);
    if(GetHandle()->IsActive())
    {
        IFC(GetFocusedElement(&pFocused));

        // TODO: handle hyperlink
        //Hyperlink hyperlink = focused as Hyperlink;

        //// If a hyperlink has focus, we will start our search at its owner FE.
        //if (hyperlink != null)
        //{
        //    focused = hyperlink.HostFrameworkElement;
        //}

        pThisAsDO = static_cast<xaml::IDependencyObject *>(this);
        pCurrentAsDO = pFocused;

        // Walk up the visual tree to see if you, or one of your visual children, have the focus.  This test is often required
        // because the focus events occur asynchronously and do not accurately reflect whether or not you actually have the focus.
        while (pCurrentAsDO && pCurrentAsDO != pThisAsDO)
        {
            IFC(VisualTreeHelper::GetParentStatic(pCurrentAsDO, &pParentAsDO));
            ReleaseInterface(pCurrentAsDO);
            pCurrentAsDO = pParentAsDO;
        }

        pParentAsDO = NULL;
        *pbHasFocus = (pCurrentAsDO == pThisAsDO);
    }
    else
    {
        *pbHasFocus = FALSE;
    }

Cleanup:
    ReleaseInterface(pCurrentAsDO);
    ReleaseInterface(pParentAsDO);
    RRETURN(hr);
}

_Check_return_
HRESULT
FrameworkElement::GetValueFromStyle(
    _In_ const CDependencyProperty* pDP,
    _Out_opt_ IInspectable** ppValue,
    _Out_ bool* pbGotValue)
{
    HRESULT hr = S_OK;
    CValue boxedValue;

    // Effective value of core properties is updated by native code,
    // so this method should be used only for managed properties.
    ASSERT(pDP->IsSparse());

    // Style is implemented in native code. If property is in current style,
    // get it.
    IFC(CoreImports::GetManagedPropertyValueFromStyle(
        FALSE, // bUseBuiltInStyle
        GetHandle(),
        pDP,
        &boxedValue,
        pbGotValue));

    if (*pbGotValue)
    {
        IFC(CValueBoxer::UnboxObjectValue(&boxedValue, pDP->GetPropertyType(), __uuidof(IInspectable), reinterpret_cast<void**>(ppValue)));
    }
    else
    {
        *ppValue = NULL;
    }

Cleanup:
    RRETURN(hr);
}



//-----------------------------------------------------------------------------
//
//  OnDisconnectVisualChildren
//
//  During a DisconnectVisualChildrenRecursive tree walk, also clear the DataContext and
//  Tag properties, as well as un-applying the template.
//
//-----------------------------------------------------------------------------

IFACEMETHODIMP
FrameworkElement::OnDisconnectVisualChildren()
{
    HRESULT hr = S_OK;

    // Clear the DataContext
    IFC(ClearValue(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::FrameworkElement_DataContext)));

    // Clear Tag
    IFC(ClearValue(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::FrameworkElement_Tag)));

    // Clear the visual children
    IFC(FrameworkElementGenerated::OnDisconnectVisualChildren());

    // Now that the template has been cleared, let the control clear any references it got
    // from GetTemplateChild()
    // (Call the version of OnApplyTemplate that calls out to the outer object first.)
    IFC(OnApplyTemplateProtected());

Cleanup:
    RRETURN(hr);
}


// Check whether this FrameworkElement has a Style set by an ItemsControl. This typically happens when the user provides an explicit container
// in XAML, but does not set a local style for the container.
BOOLEAN
FrameworkElement::GetIsStyleSetFromItemsControl()
{
    return m_isStyleSetFromItemsControl;
}

// Set a flag showing whether this FrameworkElement had a style set by an ItemsControl.
void
FrameworkElement::SetIsStyleSetFromItemsControl(BOOLEAN value)
{
    VERIFYHR(MarkHasState());
    m_isStyleSetFromItemsControl = value;
}

// Clips the given point (which is relative to the specified FrameworkElement)
// to the bounds of this FrameworkElement.
_Check_return_
HRESULT
FrameworkElement::ClipPointToElementBounds (
    _In_ FrameworkElement* pElementPointRelativeTo,
    _Inout_ wf::Point* pPoint)
{
    HRESULT hr = S_OK;
    wf::Point pointRelativeToBoundsElement = {0,0};
    ctl::ComPtr<IGeneralTransform> spToBoundsElement = NULL;
    ctl::ComPtr<IGeneralTransform> spFromBoundsElement = NULL;
    DOUBLE boundsItemWidth = 0;
    DOUBLE boundsItemHeight = 0;

    // Get the transforms and sizes we need.
    IFC(pElementPointRelativeTo->TransformToVisual(this, &spToBoundsElement));
    IFC(TransformToVisual(pElementPointRelativeTo, &spFromBoundsElement));
    IFC(get_ActualWidth(&boundsItemWidth));
    IFC(get_ActualHeight(&boundsItemHeight));

    // Transform point to bounds element.
    IFC(spToBoundsElement->TransformPoint(*pPoint, &pointRelativeToBoundsElement));

    // Clip
    pointRelativeToBoundsElement.X = MAX(0, MIN(static_cast<FLOAT>(boundsItemWidth), pointRelativeToBoundsElement.X));
    pointRelativeToBoundsElement.Y = MAX(0, MIN(static_cast<FLOAT>(boundsItemHeight), pointRelativeToBoundsElement.Y));

    // Transform back to source element.
    IFC(spFromBoundsElement->TransformPoint(pointRelativeToBoundsElement, pPoint));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
FrameworkElement::SetHasOpenToolTip(
    _In_ bool bHasOpenToolTip)
{
    HRESULT hr = S_OK;

    IFC(MarkHasState());
    m_bHasOpenToolTip = bHasOpenToolTip;

    if (!m_bListenForUnloaded)
    {
        ctl::ComPtr<xaml::IRoutedEventHandler> spUnloadedEventHandler;
        EventRegistrationToken unloadedToken;

        spUnloadedEventHandler.Attach(
            new ClassMemberEventHandler<
                FrameworkElement,
                IFrameworkElement,
                xaml::IRoutedEventHandler,
                IInspectable,
                xaml::IRoutedEventArgs>(
                    this,
                    &FrameworkElement::OnUnloaded,
                    true /* subscribingToSelf */ ));
        IFC(add_Unloaded(spUnloadedEventHandler.Get(), &unloadedToken));
        m_bListenForUnloaded = TRUE;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
FrameworkElement::OnUnloaded(
    _In_ IInspectable* pSender,
    _In_ IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    // Close any open automatic ToolTip if it exists. ToolTipService.ToolTip is registered on type DO, but for
    // automatic ToolTips they are only opened by PointerEntered and GotFocus events on type FE, so we can safely
    // scope this check to FE.
    if (m_bHasOpenToolTip)
    {
        IFC(ToolTipService::CancelAutomaticToolTip());
    }

Cleanup:
    RRETURN(hr);
}

// Returns True when either the Width, MinWidth or MaxWidth property was set to a non-default value.
bool FrameworkElement::IsWidthSpecified()
{
    return static_cast<CFrameworkElement*>(GetHandle())->IsWidthSpecified();
}

// Returns True when either the Height, MinHeight or MaxHeight property was set to a non-default value.
bool FrameworkElement::IsHeightSpecified()
{
    return static_cast<CFrameworkElement*>(GetHandle())->IsHeightSpecified();
}

IFACEMETHODIMP FrameworkElement::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_FALSE;
    INT nAutomationPeerFactoryIndex = 0;
    IFC_RETURN(get_AutomationPeerFactoryIndex(&nAutomationPeerFactoryIndex));
    if (nAutomationPeerFactoryIndex != 0)
    {
        ctl::ComPtr<IInspectable> spInner;
        ActivationAPI::ActivateAutomationInstance(static_cast<KnownTypeIndex>(nAutomationPeerFactoryIndex), GetHandle(), nullptr, &spInner);
        if (spInner)
        {
            ctl::ComPtr<xaml_automation_peers::IFrameworkElementAutomationPeer> spFEAutomationPeer;
            spFEAutomationPeer = spInner.AsOrNull<xaml_automation_peers::IFrameworkElementAutomationPeer>();
            if (spFEAutomationPeer)
            {
                IFC_RETURN(spFEAutomationPeer.Cast<FrameworkElementAutomationPeer>()->put_Owner(this));
                IFC_RETURN(spFEAutomationPeer.CopyTo(ppAutomationPeer));
                hr = S_OK;
            }
        }
    }
    return hr;
}

_Check_return_ HRESULT FrameworkElement::get_AutomationPeerFactoryIndex(_Out_ INT* pValue)
{
    *pValue = 0;
    if (static_cast<CFrameworkElement*>(GetHandle())->IsAutomationPeerFactorySet())
    {
        IFC_RETURN(GetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_AutomationPeerFactoryIndex, pValue));
    }
    return S_OK;
}

_Check_return_ HRESULT FrameworkElement::put_AutomationPeerFactoryIndex(_In_ INT value)
{
    IFC_RETURN(SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_AutomationPeerFactoryIndex, value));
    return S_OK;
}

_Check_return_ HRESULT FrameworkElement::get_FocusVisualPrimaryBrushImpl(_Outptr_result_maybenull_ xaml_media::IBrush** ppValue)
{
    IFC_RETURN(GetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_FocusVisualPrimaryBrush, ppValue));
    return S_OK;
}

_Check_return_ HRESULT FrameworkElement::put_FocusVisualPrimaryBrushImpl(_In_opt_ xaml_media::IBrush* pValue)
{
    ctl::ComPtr<DependencyObject> asObj = nullptr;
    IFC_RETURN(ExternalObjectReference::ConditionalWrap(pValue, &asObj));

    if (MetadataAPI::IsAssignableFrom<KnownTypeIndex::SolidColorBrush>(asObj->GetTypeIndex()))
    {
        IFC_RETURN(SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_FocusVisualPrimaryBrush, pValue));
    }
    else
    {
        IFC_RETURN(ErrorHelper::OriginateErrorUsingResourceID(E_INVALIDARG, ERROR_FOCUSRECT_NOSOLIDCOLORBRUSH))
    }

    return S_OK;
}

_Check_return_ HRESULT FrameworkElement::get_FocusVisualSecondaryBrushImpl(_Outptr_result_maybenull_ xaml_media::IBrush** ppValue)
{
    IFC_RETURN(GetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_FocusVisualSecondaryBrush, ppValue));
    return S_OK;
}

_Check_return_ HRESULT FrameworkElement::put_FocusVisualSecondaryBrushImpl(_In_opt_ xaml_media::IBrush* pValue)
{
    ctl::ComPtr<DependencyObject> asObj = nullptr;
    IFC_RETURN(ExternalObjectReference::ConditionalWrap(pValue, &asObj));

    if (MetadataAPI::IsAssignableFrom<KnownTypeIndex::SolidColorBrush>(asObj->GetTypeIndex()))
    {
        IFC_RETURN(SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_FocusVisualSecondaryBrush, pValue));
    }
    else
    {
        IFC_RETURN(ErrorHelper::OriginateErrorUsingResourceID(E_INVALIDARG, ERROR_FOCUSRECT_NOSOLIDCOLORBRUSH))
    }

    return S_OK;
}

_Check_return_ HRESULT FrameworkElement::get_IsLoadedImpl(_Out_ BOOLEAN* pValue)
{
    *pValue = FALSE;
    CFrameworkElement* frameworkObj = static_cast<CFrameworkElement*>(GetHandle());
    if (frameworkObj != nullptr)
    {
        // if the Loaded event is still pending in the queue, we should report IsLoaded as false until event fires.
        if (frameworkObj->IsActive() && !frameworkObj->IsLoadedEventPending())
        {
            *pValue = TRUE;
        }
    }
    return S_OK;
}

_Check_return_ HRESULT FrameworkElementFactory::DeferTreeImpl(
    _In_ xaml::IDependencyObject* element)
{
    IFC_RETURN(CDeferredElement::TryDefer(
        static_cast<DependencyObject*>(element)->GetHandle()));

    return S_OK;
}
