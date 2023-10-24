// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "BindingExpression.g.h"
#include "TextBox.g.h"
#include "UserControl.g.h"
#include "PropertyPath.h"
#include "PropertyPathStep.h"
#include "ResourceDictionary.g.h"
#include "DefaultValueConverter.h"
#include "DynamicValueConverter.h"
#include "AutomationProperties.h"
#include "focusmgr.h"
#include <XStringUtils.h>

using namespace DirectUI;
using namespace DirectUISynonyms;

BindingExpression::BindingExpression()
{
}

IFACEMETHODIMP BindingOperations::SetBinding(
    _In_ xaml::IDependencyObject *target,
    _In_ IDependencyProperty *property,
    _In_ xaml_data::IBindingBase *binding)
{
    return SetBindingImpl(target, property, binding);
}

_Check_return_ HRESULT BindingOperations::SetBindingImpl(
    _In_ xaml::IDependencyObject *target,
    _In_ IDependencyProperty *property,
    _In_ xaml_data::IBindingBase *binding)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DependencyObject> spTarget;
    ctl::ComPtr<DependencyPropertyHandle> spDP;
    ctl::ComPtr<Binding> spBinding;

    IFC(ctl::do_query_interface(spTarget, target));
    IFC(ctl::do_query_interface(spDP, property));
    IFC(ctl::do_query_interface(spBinding, binding));

    IFC(spTarget->SetBindingCore(spDP->GetDP(), spBinding.Get()));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
BindingOperations::SetBinding(
    _In_ xaml::IDependencyObject* pTarget,
    _In_ const CDependencyProperty* pProperty,
    _In_ xaml_data::IBindingBase* pBinding)
{
    HRESULT hr = S_OK;
    xaml_data::IBinding *pBindingIface = NULL;

    IFCPTR(pTarget);
    IFCPTR(pProperty);
    IFCPTR(pBinding);

    IFC(ctl::do_query_interface(pBindingIface, pBinding));
    IFC(static_cast<DependencyObject*>(pTarget)->SetBindingCore(pProperty, static_cast<Binding*>(pBindingIface)));

Cleanup:
    ReleaseInterface(pBindingIface);
    RRETURN(hr);
}

BindingExpression::~BindingExpression()
{
    ASSERT(m_spPeggedTarget == NULL);

    delete[] m_szTraceString;
}

_Check_return_
HRESULT
BindingExpression::CreateObject(_In_ Binding *pBinding, _Out_ BindingExpression **ppExpr)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<BindingExpression> spNewExpr;

    IFCPTR(pBinding);

    IFC(ctl::make<BindingExpression>(&spNewExpr));

    // Initialize binding expression.
    spNewExpr->SetBinding(pBinding);
    pBinding->Freeze();

    *ppExpr = spNewExpr.Detach();

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
BindingExpression::GetValue(_In_ DependencyObject* pObject, _In_ const CDependencyProperty* pDP, _Out_ IInspectable** ppValue)
{
    IFC_RETURN(PegTargetReference());
    auto pegReferenceGuard = wil::scope_exit([&]
    {
        UnpegTargetReference();
    });

    m_ignoreSourcePropertyChanges = true;
    auto ignoreSourcePropertyChangesGuard = wil::scope_exit([&]
    {
        m_ignoreSourcePropertyChanges = false;
    });

    m_bindingValueSource = BindingValueSource::Other;
    auto valueSourceGuard = wil::scope_exit([&]
    {
        m_bindingValueSource = BindingValueSource::Unknown;
    });

    // If we're not connected to a source object yet or the binding path could
    // not be resolved then return the default value, TargetNullValue or FallbackValue
    if (!m_tpListener || !m_tpListener->FullPathExists())
    {
        IFC_RETURN(GetConvertedFallbackOrDefaultValue(ppValue));
    }
    else
    {
        ctl::ComPtr<IInspectable> spExpValue;

        IFC_RETURN(m_tpListener->GetValue(&spExpValue));
        IFC_RETURN(ConvertToTarget(spExpValue.Get(), true /* useTargetNullValue */, ppValue));
    }

    valueSourceGuard.release();
    return S_OK;
}

_Check_return_
HRESULT
BindingExpression::OnAttach(_In_ DependencyObject* pTarget, _In_ const CDependencyProperty* pDP)
{
    HRESULT hr = S_OK;

    // The target and property should never be null
    IFCPTR(pTarget);
    IFCPTR(pDP);

    // We should have a binding already assigned to us
    IFCEXPECT(m_tpBinding); // We should have a binding assigned to us

    // Store the reference to the target element and property
    IFC(ctl::AsWeak(pTarget, &m_spTargetRef));

    m_pDP = pDP; // This is a strong reference, we want the DP to stay alive

    // Ensure that the target is pegged at this stage
    IFC(PegTargetReference());

    {
        // If the target property is the DC property record so
        if (pDP->GetIndex() == KnownPropertyIndex::FrameworkElement_DataContext)
        {
            FrameworkElement *pFETarget = static_cast<FrameworkElement *>(GetTargetNoRef());

            // The DC property can only be bound once
            IFCEXPECT(!pFETarget->IsDataContextBound());
            pFETarget->SetIsDataContextBound(TRUE);
        }
    }

    // Connect to the target itself if needed
    IFC(ConnectToTarget());

    // Calculate the effective source
    IFC(CalculateEffectiveSource());

    // Connects to the effective source
    IFC(ConnectToEffectiveSource());

Cleanup:
    UnpegTargetReference();

    if (FAILED(hr))
    {
        IGNOREHR(OnDetach());
    }

    RRETURN(hr);
}

_Check_return_
HRESULT
BindingExpression::OnDetach()
{
    HRESULT hr = S_OK;

    if (!m_spTargetRef || !TryPegTargetReference())
    {
        // target has already been detached or it has been collected
        goto Cleanup;
    }

    {
        if (m_pDP->GetIndex() == KnownPropertyIndex::FrameworkElement_DataContext)
        {
            FrameworkElement *pFETarget = static_cast<FrameworkElement *>(GetTargetNoRef());

            // The DC property can only be bound once
            ASSERT(pFETarget->IsDataContextBound());
            pFETarget->SetIsDataContextBound(FALSE);
        }
    }

    // Detach from the target if needed
    IFC(DisconnectFromTarget());

    // Detach from the source
    IFC(DisconnectFromEffectiveSource());

Cleanup:

    UnpegTargetReference();

    // Forget the reference to the target
    m_spTargetRef = nullptr;

    RRETURN(hr);
}

_Check_return_
HRESULT
BindingExpression::GetCanSetValue(_Out_ bool *pValue)
{
    HRESULT hr = S_OK;
    xaml_data::BindingMode mode = xaml_data::BindingMode_OneTime;

    IFC(PegTargetReference());

    IFC(m_tpBinding->get_Mode(&mode));

    *pValue = mode == xaml_data::BindingMode_TwoWay ? TRUE : FALSE;

Cleanup:

    UnpegTargetReference();

    RRETURN(hr);
}

bool
BindingExpression::GetIsAssociated()
{
    return (m_spTargetRef != NULL);
}

_Check_return_
HRESULT
BindingExpression::get_DataItemImpl(_Outptr_ IInspectable **ppReturnValue)
{
    HRESULT hr = S_OK;

    // The return value should never be null
    IFCPTR(ppReturnValue);

    *ppReturnValue = NULL;
    if (m_tpEffectiveSource.Get() != NULL)
    {
        *ppReturnValue = ValueWeakReference::get_value_as<IInspectable>(m_tpEffectiveSource.Get());
    }

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
BindingExpression::get_ParentBindingImpl(_Outptr_ xaml_data::IBinding **ppReturnValue)
{
    HRESULT hr = S_OK;
    xaml_data::IBinding* pBindingIface = NULL;

    ARG_VALIDRETURNPOINTER(ppReturnValue);

    IFC(ctl::do_query_interface(pBindingIface, m_tpBinding.Get()));

    *ppReturnValue = pBindingIface;

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
BindingExpression::UpdateSourceImpl()
{
    HRESULT hr = S_OK;
    xaml_data::BindingMode mode = xaml_data::BindingMode_OneTime;

    IFC(PegTargetReference());

    // If there's no source to be updated then just return since the source cannot be updated
    if (!m_tpListener || !m_tpListener->FullPathExists())
    {
        goto Cleanup;
    }

    // Only update the source if it is a two-way binding
    IFC(m_tpBinding->get_Mode(&mode));
    if(mode == xaml_data::BindingMode_TwoWay)
    {
        IFC(UpdateSourceInternal());
    }

Cleanup:

    UnpegTargetReference();

    RRETURN(hr);
}

_Check_return_
HRESULT
BindingExpression::ConnectToTarget()
{
    // Attach to the property changed event on the target, that's how we will receive
    // the fact that the property value is new
    xaml_data::BindingMode bindingMode = xaml_data::BindingMode_OneWay;
    IFC_RETURN(m_tpBinding->get_Mode(&bindingMode));
    if (bindingMode == xaml_data::BindingMode_TwoWay)
    {
        // Determine if the target is a textbox
        bool fTargetIsTextBox = GetTargetNoRef()->GetTypeIndex() == KnownTypeIndex::TextBox;

        // At this point we should not have handlers setup
        IFCEXPECT_RETURN(!m_tpSyncHandler);

        xaml_data::UpdateSourceTrigger updateSourceTrigger = xaml_data::UpdateSourceTrigger_Default;
        IFC_RETURN(m_tpBinding->get_UpdateSourceTrigger(&updateSourceTrigger));
        if(updateSourceTrigger != xaml_data::UpdateSourceTrigger_Explicit)
        {
            // First connect to the sync event source
            ctl::ComPtr<IDPChangedEventSource> spDPChangedEventSource;
            ctl::ComPtr<BindingExpressionDPChangedHandler> spSyncHandler;

            spSyncHandler.Attach(new BindingExpressionDPChangedHandler(this));
            SetPtrValue(m_tpSyncHandler, spSyncHandler);
            IFC_RETURN(GetTargetNoRef()->GetDPChangedEventSource(&spDPChangedEventSource));
            IFC_RETURN(spDPChangedEventSource->AddHandler(m_tpSyncHandler.Get()));

            // We need to listen for the LostFocus event to propagate the value from the
            // target to the source if:
            // 1) The target is TextBox.Text
            // 2) The Binding's UpdateSourceTrigger is LostFocus
            if (   (updateSourceTrigger == xaml_data::UpdateSourceTrigger_LostFocus)
                || (   updateSourceTrigger == xaml_data::UpdateSourceTrigger_Default
                    && fTargetIsTextBox && m_pDP->GetIndex() == KnownPropertyIndex::TextBox_Text))
            {
                // Only try to set up the LostFocus handler if the target is a UIElement. Otherwise, throw
                // an error
                ctl::ComPtr<xaml::IUIElement> spUIElement;
                ctl::ComPtr<BindingExpressionLostFocusHandler> spLostFocusHandler;

                if (SUCCEEDED(ctl::do_query_interface(spUIElement, GetTargetNoRef())))
                {
                    spLostFocusHandler.Attach(new BindingExpressionLostFocusHandler(this));
                    SetPtrValue(m_tpLostFocusHandler, spLostFocusHandler);
                    IFC_RETURN(spUIElement->add_LostFocus(m_tpLostFocusHandler.Get(), &m_lostFocusHandlerToken));
                    m_fListeningToLostFocus = true;
                }
                else
                {
                    IFC_RETURN(ErrorHelper::OriginateErrorUsingResourceID(E_FAIL, AG_E_LOSTFOCUS_BINDING_REQUIRES_UIELEMENT));
                }
            }
        }
    }

    // Listen for changes on the inheritance context for the target
    // this only needs to be done for objects that are not FE
    if (!GetTargetNoRef()->GetHandle()->OfTypeByIndex<KnownTypeIndex::FrameworkElement>())
    {
        ctl::ComPtr<IInheritanceContextChangedEventSource> spSource;

        IFC_RETURN(GetTargetNoRef()->GetInheritanceContextChangedEventSource(&spSource));
        if (!m_tpInheritanceContextChangedHandler)
        {
            ctl::ComPtr<BindingExpressionInheritanceContextChangedHandler> spHandler;
            spHandler.Attach(new BindingExpressionInheritanceContextChangedHandler(this));
            SetPtrValue(m_tpInheritanceContextChangedHandler, spHandler);
        }
        IFC_RETURN(spSource->AddHandler(m_tpInheritanceContextChangedHandler.Get()));
    }

    return S_OK;
}

_Check_return_
HRESULT
BindingExpression::DisconnectFromTarget()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IDPChangedEventSource> spSyncSource;
    ctl::ComPtr<xaml::IUIElement> spTargetAsUIElement;

    // Undo the registration of the event handlers - ensures that we're disconnected from the target
    if (m_tpSyncHandler)
    {
        IFC(GetTargetNoRef()->GetDPChangedEventSource(&spSyncSource));

        IFC(spSyncSource->RemoveHandler(m_tpSyncHandler.Get()));

        if (m_fListeningToLostFocus)
        {
            IFCEXPECT(m_tpLostFocusHandler);

            IFC(ctl::do_query_interface(spTargetAsUIElement, GetTargetNoRef()));
            IFC(spTargetAsUIElement->remove_LostFocus(m_lostFocusHandlerToken));
            m_fListeningToLostFocus = false;
        }
    }

    IFC(StopListeningForInheritanceContextChanges());

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
BindingExpression::StopListeningForInheritanceContextChanges()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInheritanceContextChangedEventSource> spSource;

    if (!m_tpInheritanceContextChangedHandler)
    {
        goto Cleanup;
    }

    // Undo listening for inheritance context
    IFC(GetTargetNoRef()->GetInheritanceContextChangedEventSource(&spSource));
    IFC(spSource->RemoveHandler(m_tpInheritanceContextChangedHandler.Get()));

    m_tpInheritanceContextChangedHandler.Clear();

Cleanup:

    RRETURN(hr);
}


_Check_return_
HRESULT
BindingExpression::AttachToEffectiveSource()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IRelativeSource> spRelativeSource;
    xaml_data::RelativeSourceMode relativeSourceMode = xaml_data::RelativeSourceMode_None;
    wrl_wrappers::HString strElementName;

    // Check to see if we're binding to element name
    IFC(m_tpBinding->get_ElementName(strElementName.GetAddressOf()));
    if (strElementName.Get() != NULL)
    {
        IFC(AttachToElementName());
        goto Cleanup;
    }

    // Only look for the DataContext if there's no relative source
    // specified
    IFC(m_tpBinding->get_RelativeSource(&spRelativeSource));
    if (spRelativeSource)
    {
        IFC(spRelativeSource->get_Mode(&relativeSourceMode));
    }

    switch (relativeSourceMode)
    {
    case xaml_data::RelativeSourceMode_None:
        IFC(AttachToDataContext());
        break;

    case xaml_data::RelativeSourceMode_Self:
        IFC(AttachToTarget());
        break;

    case xaml_data::RelativeSourceMode_TemplatedParent:
        IFC(AttachToTemplatedParent());
        break;

    default:
        // Unknown relative source mode
        ASSERT(FALSE);
        IFC(E_NOTIMPL);
        break;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
BindingExpression::DetachFromEffectiveSource()
{
    HRESULT hr = S_OK;

    switch (m_effectiveSourceType)
    {
    case xaml_data::EffectiveSourceType_DataContext:
    case xaml_data::EffectiveSourceType_Mentor_DataContext:
        IFC(DetachFromDataContext());
        break;

    case xaml_data::EffectiveSourceType_Target:
        // Nothing special to do for Self
        break;

    case xaml_data::EffectiveSourceType_TemplatedParent:
    case xaml_data::EffectiveSourceType_Mentor_TemplatedParent:
        IFC(DetachFromTemplatedParent());
        break;

    case xaml_data::EffectiveSourceType_Binding_Source:
        IFC(DetachFromMentor());
        break;

    case xaml_data::EffectiveSourceType_ElementName:
    case xaml_data::EffectiveSourceType_Mentor_ElementName:
        IFC(DetachFromElementName());
        break;

    default:
        // Unknown effective source type
        ASSERT(FALSE);
        IFC(E_NOTIMPL);
        break;
    }

    // Done with the effective source
    m_tpEffectiveSource.Clear();

    // Reset the effective source type
    m_effectiveSourceType = xaml_data::EffectiveSourceType_None;

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
BindingExpression::AttachToDataContext()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IFrameworkElement> spFE;
    FrameworkElement *pDCSource = NULL; // Raw pointer, no need to release
    ctl::ComPtr<IDataContextChangedEventSource> spEventSource;
    ctl::ComPtr<IInspectable> spDataContext;

    // If the target itself is not an FE then we need to find
    // the mentor and get the DC from it
    spFE = ctl::query_interface_cast<IFrameworkElement>(GetTargetNoRef());
    if (!spFE)
    {
        IFC(AttachToMentor());
        if (m_spMentorRef)
        {
            IFC(m_spMentorRef.As(&spFE));
            pDCSource = spFE.Cast<FrameworkElement>();
        }
        m_effectiveSourceType = xaml_data::EffectiveSourceType_Mentor_DataContext;
    }
    else
    {
        pDCSource = spFE.Cast<FrameworkElement>();
        m_effectiveSourceType = xaml_data::EffectiveSourceType_DataContext;
    }

    // Only fetch the DataContext if we have one
    if (pDCSource)
    {
        if (!m_tpDataContextChangedChangedHandler)
        {
            ctl::ComPtr<BindingExpressionDataContextChangedHandler> spHandler;

            spHandler.Attach(new BindingExpressionDataContextChangedHandler(this));
            SetPtrValue(m_tpDataContextChangedChangedHandler, spHandler);
        }

        IFC(pDCSource->GetDataContextChangedSource(&spEventSource));
        IFC(spEventSource->AddHandler(m_tpDataContextChangedChangedHandler.Get()));

        m_tpEffectiveSource.Clear();
        IFC(GetDataContext(pDCSource, &spDataContext));
        if( spDataContext )
        {
            SetPtrValue(m_tpEffectiveSource, spDataContext);
        }
    }

Cleanup:

    RRETURN(hr);
}

 _Check_return_
HRESULT
BindingExpression::DetachFromDataContext()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IFrameworkElement> spFE;
    FrameworkElement *pDCSource = NULL; // Raw pointer, no release
    ctl::ComPtr<IDataContextChangedEventSource> spEventSource;

    // If there's no handler then there's nothing todo
    if (!m_tpDataContextChangedChangedHandler)
    {
        goto Cleanup;
    }

    switch (m_effectiveSourceType)
    {
    case xaml_data::EffectiveSourceType_DataContext:
        IFC(ctl::do_query_interface(spFE, GetTargetNoRef()));
        pDCSource = spFE.Cast<FrameworkElement>();
        break;

    case xaml_data::EffectiveSourceType_Mentor_DataContext:
        if (m_spMentorRef)
        {
            IFC(m_spMentorRef.As(&spFE));
        }
        pDCSource = spFE.Cast<FrameworkElement>();
        break;

    default:
        ASSERT(FALSE);
        IFC(E_UNEXPECTED);
        break;
    }

    // Remove our handler from the DC source, if we have one
    if (pDCSource)
    {
        IFC(pDCSource->GetDataContextChangedSource(&spEventSource));
        IFC(spEventSource->RemoveHandler(m_tpDataContextChangedChangedHandler.Get()));
    }

    // After we disconnected from the DC changes disconnect from the mentor
    // we do not need it anymore
    if (m_effectiveSourceType == xaml_data::EffectiveSourceType_Mentor_DataContext)
    {
        IFC(DetachFromMentor());
    }

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
BindingExpression::AttachToTarget()
{
    HRESULT hr = S_OK;

    // We do not want to have a ref-cycle so we will keep in this case
    // just a weak reference back to the target.
    ctl::ComPtr<IInspectable> spWeakReference;
    IFC(ValueWeakReference::Create(ctl::as_iinspectable(GetTargetNoRef()), &spWeakReference));
    // IFC( m_tpEffectiveSource.Set( pWeakReference ));
    SetPtrValue(m_tpEffectiveSource, spWeakReference);
    m_effectiveSourceType = xaml_data::EffectiveSourceType_Target;

    // No need to listen for inheritance context changes anymore since we have our source
    IFC(StopListeningForInheritanceContextChanges());

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
BindingExpression::AttachToTemplatedParent()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IFrameworkElement> spFE;
    FrameworkElement *pTemplatedParentSource = NULL;    // Is going to be the same as pFE no need to release
    ctl::ComPtr<DependencyObject> spTemplatedParent;
    ctl::ComPtr<IInspectable> spEffectiveSource;

    // If the target itself is not an FE then we need to find
    // the mentor and get the DC from it
    spFE = ctl::query_interface_cast<IFrameworkElement>(GetTargetNoRef());
    if (!spFE)
    {
        IFC(AttachToMentor());
        if (m_spMentorRef)
        {
            IFC(m_spMentorRef.As(&spFE));
            pTemplatedParentSource = spFE.Cast<FrameworkElement>();
        }
        m_effectiveSourceType = xaml_data::EffectiveSourceType_Mentor_TemplatedParent;
    }
    else
    {
        pTemplatedParentSource = spFE.Cast<FrameworkElement>();
        m_effectiveSourceType = xaml_data::EffectiveSourceType_TemplatedParent;
    }

    // Only fetch the TemplatedParent if we have one
    if (pTemplatedParentSource)
    {
        IFC(pTemplatedParentSource->get_TemplatedParent(&spTemplatedParent));
        if (spTemplatedParent)
        {
            IFC(ValueWeakReference::Create(ctl::as_iinspectable(spTemplatedParent.Get()), &spEffectiveSource));
        }
        else
        {
            IFC(PropertyValue::CreateEmpty(&spEffectiveSource));
        }

        // m_tpEffectiveSource.Set( pEffectiveSource );
        SetPtrValue(m_tpEffectiveSource, spEffectiveSource);

        // If we have a mentor then this mentor was either created in a template or it wasn't
        // in any case we do not need to listen for the InheritanceContextChanged event anymore
        // as we found our source
        IFC(StopListeningForInheritanceContextChanges());
        IFC(DetachFromMentor());
    }

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
BindingExpression::DetachFromTemplatedParent()
{
    HRESULT hr = S_OK;

    // After we're disconnected we can remove our connector to the mentor
    if (m_effectiveSourceType == xaml_data::EffectiveSourceType_Mentor_TemplatedParent)
    {
        IFC(DetachFromMentor());
    }

Cleanup:

    RRETURN(hr);
}


_Check_return_
HRESULT
BindingExpression::AttachToMentor()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<FrameworkElement> spMentor;

    // If already have a mentor then we're done
    if (m_spMentorRef)
    {
        goto Cleanup;
    }

    IFC(GetTargetNoRef()->GetMentor(&spMentor));
    if (spMentor)
    {
        IFC(ctl::AsWeak(ctl::iinspectable_cast(spMentor.Get()), &m_spMentorRef));
        // Mentor is being used in binding, so has state
        IFC(spMentor->MarkHasState());
    }

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
BindingExpression::AttachToMentor(_In_ FrameworkElement *pMentor)
{
    HRESULT hr = S_OK;

    m_spMentorRef = nullptr;

    if (pMentor)
    {
        IFC(ctl::AsWeak(ctl::iinspectable_cast(pMentor), &m_spMentorRef));
        // Mentor is being used in binding, so has state
        IFC(pMentor->MarkHasState());
    }

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
BindingExpression::DetachFromMentor()
{
    HRESULT hr = S_OK;

    if (!m_spMentorRef)
    {
        goto Cleanup;
    }

    m_spMentorRef = nullptr;

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
BindingExpression::AttachToElementName()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IFrameworkElement> spFE;
    FrameworkElement *pElementNameSource = NULL;    // Is the same reference as pFE no need to release
    ctl::ComPtr<IInspectable> spSource;
    ctl::ComPtr<IInspectable> spWeakReference;

    // If the target itself is not an FE then we need to find
    // the mentor and get the name from it
    spFE = ctl::query_interface_cast<IFrameworkElement>(GetTargetNoRef());
    if (!spFE)
    {
        IFC(AttachToMentor());
        if (m_spMentorRef)
        {
            IFC(m_spMentorRef.As(&spFE));
            pElementNameSource = spFE.Cast<FrameworkElement>();
        }
        m_effectiveSourceType = xaml_data::EffectiveSourceType_Mentor_ElementName;
    }
    else
    {
        pElementNameSource = spFE.Cast<FrameworkElement>();
        m_effectiveSourceType = xaml_data::EffectiveSourceType_ElementName;
    }

    if (pElementNameSource)
    {
        IFC(GetSourceElement(pElementNameSource, &spSource));

        if (spSource)
        {
            IFC(ValueWeakReference::Create(spSource.Get(), &spWeakReference));
            SetPtrValue(m_tpEffectiveSource, spWeakReference);
            IFC(DetachFromMentor());    // We have a source we do not need the mentor anymore
            IFC(StopListeningForInheritanceContextChanges());
        }
        else
        {
            if (!pElementNameSource->IsInLiveTree())
            {
                ctl::ComPtr<BindingExpressionTargetLoadedHandler> spHandler;

                spHandler.Attach(new BindingExpressionTargetLoadedHandler(this));
                SetPtrValue(m_tpTargetLoadedHandler, spHandler);

                FrameworkElement::LoadedEventSourceType* pLoadedEventSource = nullptr;
                IFC(pElementNameSource->GetLoadedEventSourceNoRef(&pLoadedEventSource));
                IFC(pLoadedEventSource->AddHandler(m_tpTargetLoadedHandler.Get()));
            }
        }
    }

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
BindingExpression::DetachFromElementName()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IFrameworkElement> spFE;
    FrameworkElement *pElementNameSource = NULL;    // Raw pointer, no need to release

    if (m_tpTargetLoadedHandler)
    {
        switch (m_effectiveSourceType)
        {
        case xaml_data::EffectiveSourceType_ElementName:
            IFC(ctl::do_query_interface(spFE, GetTargetNoRef()));
            pElementNameSource = spFE.Cast<FrameworkElement>();
            break;

        case xaml_data::EffectiveSourceType_Mentor_ElementName:
            if (m_spMentorRef)
            {
                IFC(m_spMentorRef.As(&spFE));
            }
            pElementNameSource = spFE.Cast<FrameworkElement>();
            break;

        default:
            // Unknown type of ElementName
            ASSERT(FALSE);
            IFC(E_UNEXPECTED);
            break;
        }

        if (pElementNameSource)
        {
            FrameworkElement::LoadedEventSourceType* pTargetLoadedSource = nullptr;
            IFC(pElementNameSource->GetLoadedEventSourceNoRef(&pTargetLoadedSource));
            IFC(pTargetLoadedSource->RemoveHandler(m_tpTargetLoadedHandler.Get()));
            m_tpTargetLoadedHandler.Clear();
        }
    }

    // After we're disconnected we can remove our connector to the mentor
    if (m_effectiveSourceType == xaml_data::EffectiveSourceType_Mentor_ElementName)
    {
        IFC(DetachFromMentor());
    }

Cleanup:

    RRETURN(hr);
}


_Check_return_
HRESULT
BindingExpression::AttachToCollectionViewSource()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ICollectionViewSource> spCVS;
    ctl::ComPtr<ICollectionView> spView;
    ctl::ComPtr<ICVSViewChangedEventSource> spSource;

    // Abandon the existing CVS
    IFC(DetachFromCollectionViewSource());

    // If we do not have an effective source yet
    // then nothing to do
    if (m_tpEffectiveSource.Get() == NULL)
    {
        goto Cleanup;
    }

    // it is a DO and hence we should get its IDO interface directly
    spCVS = ctl::query_interface_cast<ICollectionViewSource>(m_tpEffectiveSource.GetAsDO());
    if (!spCVS)
    {
        goto Cleanup;
    }

    // At this point we got a CVS, we want the value of it's View property
    // to be the effective value so
    SetPtrValue(m_tpCVS, spCVS);

    // Now that we have the CVS, we can set the effective source to the
    // value of the view property
    m_tpEffectiveSource.Clear();
    IFC(m_tpCVS->get_View(&spView));
    SetPtrValue(m_tpEffectiveSource, spView);

    // Connect to the CVS view changed event to be notified
    // of view changes
    if (!m_tpCVSViewChangedHandler)
    {
        ctl::ComPtr<BindingExpressionCVSViewChangedHandler> spHandler;

        spHandler.Attach(new BindingExpressionCVSViewChangedHandler(this));
        SetPtrValue(m_tpCVSViewChangedHandler, spHandler);
    }

    IFC(m_tpCVS->GetCVSViewChangedSource(&spSource));
    IFC(spSource->AddHandler(m_tpCVSViewChangedHandler.Get()));

Cleanup:

    RRETURN(hr);
}


_Check_return_
HRESULT
BindingExpression::DetachFromCollectionViewSource()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ICVSViewChangedEventSource> spSource;

    // We have no CVS so nothing to do
    if (m_tpCVS.Get() == NULL)
    {
        goto Cleanup;
    }

    // We might not have a change handler at this stage if
    // we failed to connect to the collection view source
    if (m_tpCVSViewChangedHandler)
    {
        IFC(m_tpCVS->GetCVSViewChangedSource(&spSource));
        IFC(spSource->RemoveHandler(m_tpCVSViewChangedHandler.Get()));
    }

    // Now abandon the CVS
    m_tpCVS.Clear();

Cleanup:

    RRETURN(hr);
}


_Check_return_
HRESULT
BindingExpression::GetSourceElement(
    _In_ FrameworkElement *pMentor,
    _Outptr_ IInspectable **ppSource)
{
    wrl_wrappers::HString strElementName;
    bool searchUserControlDefinitionNamescope = false;

    ASSERT(m_effectiveSourceType == xaml_data::EffectiveSourceType_Mentor_ElementName ||
           m_effectiveSourceType == xaml_data::EffectiveSourceType_ElementName);

    IFC_RETURN(m_tpBinding->get_ElementName(strElementName.GetAddressOf()));
    ASSERT(strElementName.Get() != nullptr);     // We know we're processing an ElementName binding

    // Consider the case of a mentor being a UserControl and if the target
    // element is in the resources of that UserControl
    if (m_effectiveSourceType == xaml_data::EffectiveSourceType_Mentor_ElementName)
    {
        if (ctl::is<IUserControl>(pMentor))
        {
            UserControl *pUC = static_cast<UserControl *>(pMentor);
            ResourceDictionary *pInternalResourceDictionary = nullptr;

            // By definition, if the target is a resource inside the UserControl.Resources ResourceDictionary,
            // that ResourceDictionary is the parent.
            //
            // MSFT:10964346
            // This logic doesn't work for theme dictionaries (and has never worked, likely due to
            // oversight/ignorance of its existence when theme resources were first added).
            // There are likely other ways to get a non-FE target inside a UserControl's content with an ElementName Binding
            // to rely on the UserControl as the mentor, in which case we would need some other way to determine whether we
            // need to search the namescope owned by the UserControl, or the namescope that the UserControl instance belongs to.
            ctl::ComPtr<IResourceDictionary> spResourceDictionary;
            IFC_RETURN(pUC->get_Resources(&spResourceDictionary));
            pInternalResourceDictionary = spResourceDictionary.Cast<ResourceDictionary>();

            if (GetTargetNoRef()->GetHandle()->GetParentInternal(false) == pInternalResourceDictionary->GetHandle())
            {
                // If the target object is a resource for the mentor, i.e. it is a DO in the resource dictionary, then
                // we need to do the lookup inside the UserControl's definition namescope, not on the namescope where the UserControl lives
                searchUserControlDefinitionNamescope = TRUE;
            }
        }
    }

    IFC_RETURN(pMentor->FindNameInPage(strElementName.Get(), searchUserControlDefinitionNamescope /*fIsCalledFromUserControl*/, ppSource));

    return S_OK;
}


_Check_return_
HRESULT
BindingExpression::GetDataContext(_Outptr_ IInspectable **ppValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IFrameworkElement> spFE;
    FrameworkElement *pDCSource = NULL; // Raw pointer, no need to release

    ASSERT(m_effectiveSourceType == xaml_data::EffectiveSourceType_DataContext ||
           m_effectiveSourceType == xaml_data::EffectiveSourceType_Mentor_DataContext);

    if (m_effectiveSourceType == xaml_data::EffectiveSourceType_DataContext)
    {
        IFC(ctl::do_query_interface(spFE, GetTargetNoRef()));
    }
    else
    {
        if (m_spMentorRef)
        {
            IFC(m_spMentorRef.As(&spFE));
        }
    }

    pDCSource = spFE.Cast<FrameworkElement>();

    IFC(GetDataContext(pDCSource, ppValue));

Cleanup:

    RRETURN(hr);
}


_Check_return_
HRESULT
BindingExpression::GetDataContext(_In_ FrameworkElement *pDCSource, _Outptr_ IInspectable **ppValue)
{
    HRESULT hr = S_OK;

    ASSERT(m_effectiveSourceType == xaml_data::EffectiveSourceType_DataContext ||
           m_effectiveSourceType == xaml_data::EffectiveSourceType_Mentor_DataContext);

    // If we don't have yet a source of DC then skip the operation
    // This can happen if we do not yet have a mentor
    if (pDCSource == NULL)
    {
        ASSERT(m_effectiveSourceType == xaml_data::EffectiveSourceType_Mentor_DataContext);
        goto Cleanup;
    }

    // If the target DP is the DataContext property then we need to get the
    // DC from the parent, so we don't have a loop.
    {
        if (m_pDP->GetIndex() == KnownPropertyIndex::FrameworkElement_DataContext)
        {
            // To bind to the DC the target must be a FrameworkElement
            ASSERT(m_effectiveSourceType == xaml_data::EffectiveSourceType_DataContext);

            IFC(pDCSource->GetEffectiveDataContext(ppValue));
        }
        else
        {
            IFC(pDCSource->get_DataContext(ppValue));
        }
    }

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
BindingExpression::GetConvertedFallbackOrDefaultValue(
    _Outptr_ IInspectable **ppConvertedValue)
{
    ctl::ComPtr<IInspectable> spFallbackValue;

    IFC_RETURN(m_tpBinding->get_FallbackValue(&spFallbackValue));
    if (spFallbackValue)
    {
        IFC_RETURN(ConvertValue(spFallbackValue.Get(), ppConvertedValue));
        m_bindingValueSource = BindingValueSource::FallbackValue;
    }
    else
    {
        // Use the default value of the property until we get a valid source
        IFC_RETURN(GetTargetNoRef()->GetDefaultValueInternal(m_pDP, ppConvertedValue));
        m_bindingValueSource = BindingValueSource::Unknown;
    }

    return S_OK;
}

_Check_return_
HRESULT
BindingExpression::GetConvertedTargetNullOrDefaultValue(
    _Outptr_ IInspectable **ppConvertedValue)
{
    ctl::ComPtr<IInspectable> spTargetNullValue;

    IFC_RETURN(m_tpBinding->get_TargetNullValue(&spTargetNullValue));
    if (spTargetNullValue)
    {
        IFC_RETURN(ConvertToTarget(spTargetNullValue.Get(), false /* useTargetNullValue */, ppConvertedValue));
        m_bindingValueSource = BindingValueSource::TargetNullValue;
    }
    else
    {
        // Use the default value of the property until we get a valid source
        IFC_RETURN(GetTargetNoRef()->GetDefaultValueInternal(m_pDP, ppConvertedValue));
        m_bindingValueSource = BindingValueSource::Unknown;
    }

    return S_OK;
}

_Check_return_
HRESULT
BindingExpression::CalculateEffectiveSource()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spSource;

    // If the binding specifies a source use it now
    IFC(m_tpBinding->get_Source(&spSource));
    if (spSource)
    {
        SetPtrValue(m_tpEffectiveSource, spSource);
        m_effectiveSourceType = xaml_data::EffectiveSourceType_Binding_Source;

        // No need to calculate the effective source anymore since we have the source
        IFC(StopListeningForInheritanceContextChanges());
    }
    else
    {
        // Attach to the target to get to the source
        // This covers:
        // * DataContext
        // * RelativeMode.Self
        // * RelativeMode.TemplateBinding
        IFC(AttachToEffectiveSource());
    }

    // Process effective source to see if it is
    // a CVS and if so bind to the View property
    IFC(AttachToCollectionViewSource());

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
BindingExpression::ConnectToEffectiveSource()
{
    HRESULT hr = S_OK;
    PropertyPathParser *pPropertyPathParser = NULL;     // Owned by the binding

    // By this point we should have calculated the effective source
    ASSERT(m_effectiveSourceType != xaml_data::EffectiveSourceType_None);

    if( !m_tpListener  )
    {
        bool fListenToChanges = false;
        xaml_data::BindingMode bindingMode = xaml_data::BindingMode_OneTime;
        ctl::ComPtr<PropertyPathListener> spListener;

        IFC(m_tpBinding->get_Mode(&bindingMode));
        IFC(m_tpBinding->GetPropertyPathParser(&pPropertyPathParser));

        fListenToChanges = bindingMode == xaml_data::BindingMode_OneWay || bindingMode == xaml_data::BindingMode_TwoWay;

        // Create and initialize the listener before setting into m_tpListener, so we don't have a race with ReferenceTrackerWalk.
        IFC(ctl::make<PropertyPathListener>(this, pPropertyPathParser, fListenToChanges, m_effectiveSourceType == xaml_data::EffectiveSourceType_Target, &spListener));
        SetPtrValue(m_tpListener, spListener);
    }

    // Update the source of the listener
    IFC(m_tpListener->SetSource(m_tpEffectiveSource.Get()));

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
BindingExpression::DisconnectFromEffectiveSource()
{
    // If the effective source type is None, then that means we haven't
    // actually connected to an effective source yet (e.g. error early during
    // initial attach), and therefore don't need to do anything
    if (m_effectiveSourceType != xaml_data::EffectiveSourceType_None)
    {
        // Make sure the listener no longer tries to use us.
        if (m_tpListener != nullptr)
        {
            m_tpListener->ClearOwner();
        }

        m_tpListener.Clear();

        IFC_RETURN(DetachFromEffectiveSource());

        IFC_RETURN(DetachFromCollectionViewSource());
    }

    return S_OK;
}



_Check_return_
HRESULT
BindingExpression::BindingSourceChanged()
{
    HRESULT hr = S_OK;

    // If we can't get the target pegged then ignore the change
    if (!TryPegTargetReference())
    {
        goto Cleanup;

    }

    // Connect to the CVS::View property if any
    IFC(AttachToCollectionViewSource());

    IFC(RefreshBindingValue());

Cleanup:

    UnpegTargetReference();

    RRETURN(hr);
}

// This method is to be called once the final value of the
// effective source is determined and we need to update the
// target of the binding with the result of the binding
_Check_return_
HRESULT
BindingExpression::RefreshBindingValue()
{
    HRESULT hr = S_OK;

    IFC(ConnectToEffectiveSource());

    // Update the target with the new value
    IFC(UpdateTarget());

Cleanup:

    RRETURN(hr);
}


_Check_return_
HRESULT
BindingExpression::UpdateTarget()
{
    if (EventEnabledUpdateTargetBindingBegin())
    {
        TraceUpdateBindingStart(true/*updateTarget*/);
    }

    auto guard = wil::scope_exit([&]
    {
        m_state = BEState::Normal;
        if (EventEnabledUpdateTargetBindingEnd())
        {
            TraceUpdateBindingEnd(true/*updateTarget*/);
        }
    });

    // The binding must be set by now
    IFCEXPECT_RETURN(GetTargetNoRef());
    IFCEXPECT_RETURN(m_pDP);

    // Update the target with the new value
    m_state = BEState::SettingTarget;
    IFC_RETURN(GetTargetNoRef()->RefreshExpression(m_pDP));

    return S_OK;
}

_Check_return_
HRESULT
BindingExpression::UpdateSourceInternal()
{
    HRESULT hr = S_OK;
    HRESULT xr = S_OK;
    ctl::ComPtr<IInspectable> spValue;
    ctl::ComPtr<IInspectable> spValueConverted;
    bool fIsValidValue = FALSE;
    const CClassInfo *pSourceType = NULL;
    ctl::ComPtr<xaml_data::IValueConverter> spValueConverter;
    ctl::ComPtr<IInspectable> spValueConverterParameter;
    ctl::ComPtr<IInspectable> spValueConverterResult;
    wxaml_interop::TypeName sTargetType = {0};
    wrl_wrappers::HString strConverterLanguage;
    ctl::ComPtr<IInspectable> spTargetNullValue;

    if (EventEnabledUpdateSourceBindingBegin())
    {
        TraceUpdateBindingStart(false/*updateTarget*/);
    }

    IFC(GetTargetNoRef()->GetValue(m_pDP, &spValue));

    //When there is a TargetNullValue and the Target value is equal to the TargetNullValue
    //then the Source value should be set to null
    IFC(m_tpBinding->get_TargetNullValue(&spTargetNullValue));
    if (spTargetNullValue)
    {
        ctl::ComPtr<IInspectable> spConvertedValue;
        bool areEqual = false;

        IFC(ConvertToTarget(spTargetNullValue.Get(), true /* useTargetNullValue */, &spConvertedValue));

        IFC(PropertyValue::AreEqual(spConvertedValue.Get(), spValue.Get(), &areEqual));

        if (areEqual)
        {
            spValue = nullptr;
        }
    }

    IFC(m_tpListener->GetLeafType(&pSourceType));

    IFC(m_tpBinding->get_Converter(&spValueConverter ));
    if (spValueConverter)
    {
        BOOLEAN isUnsetValue = FALSE;
        IFC(m_tpBinding->get_ConverterParameter(&spValueConverterParameter));
        IFC(m_tpBinding->get_ConverterLanguage(strConverterLanguage.GetAddressOf()));

        // TODO: If no converter culture is specified then calculate the ambient culture

        IFC(MetadataAPI::GetTypeNameByClassInfo(pSourceType, &sTargetType));

        IFC(spValueConverter->ConvertBack(spValue.Get(),
            sTargetType,
            spValueConverterParameter.Get(),
            strConverterLanguage.Get(),
            &spValueConverterResult));

        // If the converter returned UnsetValue, meaning no conversion could happen
        // we stop the transfer process here silently.
        IFC(DependencyPropertyFactory::IsUnsetValue(spValueConverterResult.Get(), isUnsetValue));
        if (isUnsetValue)
        {
            goto Cleanup;
        }

        // Now the value is the converted value
        spValue = spValueConverterResult;
    }

    IFC(IsValidValueForUpdate(spValue.Get(), pSourceType, &fIsValidValue));
    if (!fIsValidValue)
    {
        IFC(EnsureValueConverter());
        xr = m_tpValueConverter->ConvertBack(spValue.Get(), pSourceType, NULL, &spValueConverted);
        if (FAILED(xr))
        {
            bool fShouldFailTransfer = false;

            IFC(ErrorHelper::ClearError());

            fShouldFailTransfer = pSourceType->IsBuiltinType();

            if (!fShouldFailTransfer)
            {
                const CClassInfo* pValueType = nullptr;
                IFC(MetadataAPI::GetClassInfoFromObject_SkipWinRTPropertyOtherType(spValue.Get(), &pValueType));

                // See comment in ConvertToTarget for why we check for built-in type in both the source type (type of the source property) and
                // the type of the value being transferred
                fShouldFailTransfer = pValueType->IsBuiltinType();
            }

            if (fShouldFailTransfer)
            {
                // The source type to which we're converting is a built-in type
                // this means that we have determine with certainty that the value is not
                // a valid value for the source
                TraceConvertFailed(spValue.Get(), pSourceType);

                // Converter errors are swallowed here
                goto Cleanup;
            }
            else
            {
                // Since the type is not a built-in type we cannot fully determine if the
                // value is a valid value for the source, we will let it go here and the
                // setter will do the checks for us
                spValueConverted = spValue;
            }
        }
    }
    else
    {
        spValueConverted = spValue;
    }

    // TODO: Add all of the validation logic
    m_state = BEState::SettingSource;
    xr = m_tpListener->SetValue(spValueConverted.Get());
    if (FAILED(xr))
    {
        TraceSetterFailed();
        IFC(ErrorHelper::ClearError());
    }

Cleanup:
    if (EventEnabledUpdateSourceBindingEnd())
    {
        TraceUpdateBindingEnd(false/*updateTarget*/);
    }
    m_state = BEState::Normal;
    m_targetPropertyState = TargetPropertyState::Clean;
    DELETE_STRING(sTargetType.Name);
    RRETURN(hr);
}

void BindingExpression::TraceSetterFailed()
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES
    const WCHAR *szExpressionTrace = NULL;
    wrl_wrappers::HString strErrorString;
    LPCWSTR szErrorString = NULL;

    if (!DebugOutput::IsLoggingForBindingEnabled())
    {
        goto Cleanup;
    }

    IFC(DXamlCore::GetCurrentNoCreate()->GetNonLocalizedErrorString(TEXT_BINDINGTRACE_SETTER_FAILED, strErrorString.GetAddressOf()));
    szErrorString = strErrorString.GetRawBuffer(NULL);

    IFC(GetTraceString(&szExpressionTrace));
    DebugOutput::LogBindingErrorMessage(StringCchPrintfWWrapper(
        const_cast<WCHAR*>(szErrorString), 
        szExpressionTrace));

Cleanup:
    return;
}

_Check_return_
HRESULT
BindingExpression::SourceChanged()
{
    // WinBlue #149347:  Hold a reference on this during this call, to protect against the
    // potential of someone to call DependencyObject::ClearValue() to remove the binding
    // during the resulting property change notification.
    ctl::ComPtr<BindingExpression> spSelfRef(this);

    // Ignore the source change if we can't peg the target
    if (!TryPegTargetReference())
    {
        return S_OK;
    }
    auto pegReferenceGuard = wil::scope_exit([&]
    {
        UnpegTargetReference();
    });

    // If this binding expression is the one setting the value
    // or we are explicitly ignoring source change notifications
    // (see comment on m_ignoreSourcePropertyChanges)
    // then ignore this change notification
    if (m_state != BEState::Normal || m_ignoreSourcePropertyChanges)
    {
        return S_OK;
    }

    IFC_RETURN(UpdateTarget());

    return S_OK;
}

_Check_return_
HRESULT
BindingExpression::TargetChanged()
{
    HRESULT hr = S_OK;

    // Ignore the target change if we can't peg the target
    if (!TryPegTargetReference())
    {
        goto Cleanup;
    }

    // If there's no source to be updated then return
    if (!m_tpListener || !m_tpListener->FullPathExists())
    {
        goto Cleanup;
    }

    // If the binding expression is the one responsible for
    // the change in the target property then return
    if (m_state != BEState::Normal)
    {
        goto Cleanup;
    }

    // If we're listening for the lost focus event then
    // remember that the target property changed
    if (m_fListeningToLostFocus)
    {
        m_targetPropertyState = TargetPropertyState::Dirty;

        const auto focusManager = VisualTree::GetFocusManagerForElement(GetTargetNoRef()->GetHandle());

        if (focusManager->GetFocusedElementNoRef() == GetTargetNoRef()->GetHandle())
        {
            // Since the target element is the focused element
            // any changes to the Text property will be postponed
            // until after the focus is lost
            goto Cleanup;
        }

        // The target element is not the focused element therefore
        // any changes to the Text property will be sent to the source
        // immediately
    }

    IFC(UpdateSourceInternal());

Cleanup:

    UnpegTargetReference();

    RRETURN(hr);
}

_Check_return_
HRESULT
BindingExpression::TargetLostFocus()
{
    // Ignore the target event change if we can't peg the target
    if (!TryPegTargetReference())
    {
        return S_OK;
    }
    auto pegReferenceGuard = wil::scope_exit([&]
    {
        UnpegTargetReference();
    });

    // Only update the source if the target property was updated
    if (m_targetPropertyState == TargetPropertyState::Dirty
        && m_tpListener != nullptr
        && m_tpListener->FullPathExists())
    {
        IFC_RETURN(UpdateSourceInternal());
    }

    return S_OK;
}

_Check_return_ HRESULT BindingExpression::DataContextChanged(_In_ const DataContextChangedParams* pArgs)
{
    ctl::ComPtr<IInspectable> spNewDataContext;
    ctl::ComPtr<FrameworkElement> spPeggedTargetAsFE;
    bool areEqual = false;

    auto guard = wil::scope_exit([this]()
    {
        UnpegTargetReference();
    });

    IFCPTR_RETURN(pArgs);

    // Ignore the DataContext change if we can't peg the target
    if (!TryPegTargetReference())
    {
        return S_OK;
    }

    // Re-calculate the DataContext if we have an explicit DataContext binding.

    // Is the target a FrameworkElement with a DataContext="{Binding}", or a ContentPresenter
    // with a Content="{Binding}"?
    if (m_spPeggedTarget)
    {
        m_spPeggedTarget.As<FrameworkElement>(&spPeggedTargetAsFE);
    }

    const BOOLEAN bIsDataContextBound =
         spPeggedTargetAsFE &&
         spPeggedTargetAsFE->IsDataContextBound();

    if (pArgs->m_fResolvedNewDataContext && !bIsDataContextBound)
    {
        IFC_RETURN(pArgs->GetNewDataContext(&spNewDataContext));
    }
    else
    {
        IFC_RETURN(GetDataContext(&spNewDataContext));
    }

    IFC_RETURN(PropertyValue::AreEqual(m_tpEffectiveSource.Get(), spNewDataContext.Get(), &areEqual));
    if (!areEqual)
    {
        // Update the data source
        SetPtrValue(m_tpEffectiveSource, spNewDataContext);

        // Recalculate the binding
        IFC_RETURN(BindingSourceChanged());
    }

    return S_OK;
}

_Check_return_
HRESULT
BindingExpression::InheritanceContextChanged()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<FrameworkElement> spNewMentor;
    ctl::ComPtr<IFrameworkElement> spOldMentorAsFE;
    ctl::ComPtr<FrameworkElement> spOldMentor;

    if (!TryPegTargetReference())
    {
        goto Cleanup;
    }

    IFC(GetTargetNoRef()->GetMentor(&spNewMentor));

    if (m_spMentorRef)
    {
        IFC(m_spMentorRef.As(&spOldMentorAsFE));
        spOldMentor = spOldMentorAsFE.Cast<FrameworkElement>();
    }

    // We only act on loosing or getting a new mentor, changing mentor has no change
    // TODO: Is this still valid?
    if (spNewMentor != spOldMentor && (!spOldMentor || !spNewMentor))
    {
        IFC(DisconnectFromEffectiveSource());

        IFC(AttachToMentor(spNewMentor.Get()));

        IFC(CalculateEffectiveSource());

        IFC(RefreshBindingValue());
    }

    // If we finally found a mentor then we're done listenting for
    // more mentor changes
    if (spNewMentor)
    {
        IFC(StopListeningForInheritanceContextChanges());
    }

Cleanup:

    UnpegTargetReference();

    RRETURN(hr);
}

_Check_return_
HRESULT
BindingExpression::TargetLoaded()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IFrameworkElement> spFE;
    FrameworkElement *pElementNameSource = NULL;    // Is the same reference as pFE no need to release
    ctl::ComPtr<IInspectable> spSource;
    FrameworkElement::LoadedEventSourceType* pLoadedEventSource = nullptr;
    ctl::ComPtr<IInspectable> spWeakReference;

    // The binding expression could have been detached(WinBlue 399022),
    // then we no longer have a reference in our target element.
    // We have to use TryPegTargetReference and ignore the failure here.
    // TODO: We could try removing the TargetLoaded handler from the Loaded event on the source element when we detach a binding expression.
    if (!TryPegTargetReference())
    {
        goto Cleanup;

    }

    // If the target itself is not an FE then we need to find
    // the mentor and get the name from it
    spFE = ctl::query_interface_cast<IFrameworkElement>(GetTargetNoRef());
    if (!spFE)
    {
        if (m_spMentorRef)
        {
            IFC(m_spMentorRef.As(&spFE));
            pElementNameSource = spFE.Cast<FrameworkElement>();
        }
    }
    else
    {
        pElementNameSource = spFE.Cast<FrameworkElement>();
    }

    IFCEXPECT(pElementNameSource);

    // Disconnect the events, we only need to load once
    IFC(pElementNameSource->GetLoadedEventSourceNoRef(&pLoadedEventSource));
    IFC(pLoadedEventSource->RemoveHandler(m_tpTargetLoadedHandler.Get()));
    m_tpTargetLoadedHandler.Clear();

    // Disconnect from the mentor, it is no longer needed since we aquired
    // a source for the name lookup and the name lookup happens exactly once
    if (m_effectiveSourceType == xaml_data::EffectiveSourceType_Mentor_ElementName)
    {
        IFC(DetachFromMentor());
        IFC(StopListeningForInheritanceContextChanges());
    }

    IFC(GetSourceElement(pElementNameSource, &spSource));

    if (spSource)
    {
        IFC(ValueWeakReference::Create(spSource.Get(), &spWeakReference));
        SetPtrValue(m_tpEffectiveSource, spWeakReference);
        IFC(BindingSourceChanged());
    }

Cleanup:

    UnpegTargetReference();

    RRETURN(hr);
}

_Check_return_
HRESULT
BindingExpression::CVSViewChanged()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ICollectionView> spView;

    // Ignore the source CVS change if we can't peg the target
    if (!TryPegTargetReference())
    {
        goto Cleanup;
    }

    // At this point we should have a CVS because
    // it just called us
    ASSERT(m_tpCVS.Get() != NULL);

    IFC(m_tpCVS->get_View(&spView));

    // Transfer the view to be the effective source
    SetPtrValue(m_tpEffectiveSource, spView);

    // And update the binding
    IFC(RefreshBindingValue());

Cleanup:

    UnpegTargetReference();

    RRETURN(hr);
}

_Check_return_
HRESULT
BindingExpression::EnsureValueConverter()
{
    HRESULT hr = S_OK;
    xaml_data::BindingMode mode = xaml_data::BindingMode_OneWay;

    if (!m_tpValueConverter)
    {
        ctl::ComPtr<IValueConverterInternal> spValueConverter;

        IFC(m_tpBinding->get_Mode(&mode));

        IFC(DynamicValueConverter::CreateConverter(&spValueConverter));
        SetPtrValue(m_tpValueConverter, spValueConverter);
    }
Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
BindingExpression::ConvertValue(
    _In_ IInspectable *pInValue,
    _Outptr_ IInspectable **ppConvertedValue)
{
    HRESULT hr = S_OK;

    IFC(EnsureValueConverter());

    if (FAILED(m_tpValueConverter->Convert(pInValue, m_pDP->GetPropertyType(), NULL, ppConvertedValue)))
    {
        // Return default value for type if failed
        IFC(GetTargetNoRef()->GetDefaultValueInternal(m_pDP, ppConvertedValue));
    }

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
BindingExpression::ConvertToTarget(
    _In_ IInspectable *pInValue,
    _In_ const bool useTargetNullValue,
    _Outptr_ IInspectable **ppConvertedValue)
{
    HRESULT hr = S_OK;
    HRESULT xr = S_OK;
    bool fIsValidValue = false;
    ctl::ComPtr<xaml_data::IValueConverter> spValueConverter;
    ctl::ComPtr<IInspectable> spValueConverterParameter;
    ctl::ComPtr<IInspectable> spConvertedValue;
    wxaml_interop::TypeName sTargetType = {0};
    wrl_wrappers::HString strConverterLanguage;
    const CClassInfo* pTargetType = m_pDP->GetPropertyType();

    // First perform the custom value conversion as requested
    IFC(m_tpBinding->get_Converter(&spValueConverter));
    if (spValueConverter)
    {
        BOOLEAN isUnsetValue = FALSE;
        IFC(m_tpBinding->get_ConverterParameter(&spValueConverterParameter));
        IFC(m_tpBinding->get_ConverterLanguage(strConverterLanguage.GetAddressOf()));

        // TODO: If no converter culture is provided calculate the ambient culture

        IFC(MetadataAPI::GetTypeNameByClassInfo(pTargetType, &sTargetType));

        IFC(spValueConverter->Convert(pInValue,
            sTargetType,
            spValueConverterParameter.Get(),
            strConverterLanguage.Get(),
            &spConvertedValue));

        // If UnsetValue is returned then we need the default
        // value of the target property
        IFC(DependencyPropertyFactory::IsUnsetValue(spConvertedValue.Get(), isUnsetValue));
        if (isUnsetValue)
        {
            IFC(GetConvertedFallbackOrDefaultValue(&spConvertedValue));
        }
    }
    else
    {
        // No conversion to the converted value is the incoming value itself
        spConvertedValue = pInValue;
    }

    // Now see if the converted value is appropiated for the target property
    // performing a built-in conversion if not
    if (!PropertyValue::IsNullOrEmpty(spConvertedValue.Get()))
    {
        IFC(MetadataAPI::IsInstanceOfType(spConvertedValue.Get(), pTargetType, &fIsValidValue));

        // The value is valid for the target, nothing else to do
        if (fIsValidValue)
        {
            *ppConvertedValue = spConvertedValue.Detach();
            goto Cleanup;
        }
    }
    else if (useTargetNullValue)
    {
        IFC(GetConvertedTargetNullOrDefaultValue(&spConvertedValue));
    }

    // Try to do the conversion if it fails then use the default value
    IFC(EnsureValueConverter());
    xr = m_tpValueConverter->Convert(spConvertedValue.Get(), pTargetType, NULL, ppConvertedValue);
    if (FAILED(xr))
    {
        bool fShouldFailTransfer = false;

        IFC(ErrorHelper::ClearError());

        // If either the type of the target DP or the value type are built-in types then
        // we know that if they're not compatible that they will not be compatibles because:
        // 1) If the DP type is a built-in type then the value type is not of the right type as we are
        //    able to check for built-in types.
        // 2) If the type of the value being converted is a built-in type then we also know that the value
        //    is not valid for the property because the only DPs that will accept the value are DPs of built-in types.
        fShouldFailTransfer = pTargetType->IsBuiltinType();

        if (!fShouldFailTransfer)
        {
            const CClassInfo* pType = nullptr;
            IFC(MetadataAPI::GetClassInfoFromObject_SkipWinRTPropertyOtherType(spConvertedValue.Get(), &pType));
            fShouldFailTransfer = pType->IsBuiltinType();
        }

        if (fShouldFailTransfer)
        {
            // The target type is a built-in type therefore we can determine with certainty
            // that the value that we're trying to convert is not a valid value
            // We will use the same behavior as before for this
            TraceConvertFailed(spConvertedValue.Get(), pTargetType);

            IFC(GetConvertedFallbackOrDefaultValue(ppConvertedValue));
        }
        else
        {
            // The target type is a custom type therefore we can't determine with certainty
            // whether the type is valid or not, we will let the value go as is
            *ppConvertedValue = spConvertedValue.Detach();
        }
    }

Cleanup:
    DELETE_STRING(sTargetType.Name);
    RRETURN(hr);
}

void BindingExpression::TraceConvertFailed(_In_ IInspectable* pValue, _In_ const CClassInfo* pTargetType)
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES
    xstring_ptr strValueClassName;
    LPCWSTR szValueClassName = NULL;
    const WCHAR* szTraceString = NULL;
    wrl_wrappers::HString strErrorString;
    LPCWSTR szErrorString = NULL;

    if (!DebugOutput::IsLoggingForBindingEnabled())
    {
        goto Cleanup;
    }

    if (pValue == NULL)
    {
        szValueClassName = L"null";
    }
    else
    {
        IFC(MetadataAPI::GetFriendlyRuntimeClassName(pValue, &strValueClassName));
        szValueClassName = strValueClassName.GetBuffer();
    }

    IFC(GetTraceString(&szTraceString));

    IFC(DXamlCore::GetCurrentNoCreate()->GetNonLocalizedErrorString(TEXT_BINDINGTRACE_CONVERT_FAILED, strErrorString.GetAddressOf()));
    szErrorString = strErrorString.GetRawBuffer(NULL);

    DebugOutput::LogBindingErrorMessage(StringCchPrintfWWrapper(
        const_cast<WCHAR*>(szErrorString), 
        const_cast<WCHAR*>(szValueClassName),
        pTargetType->GetName().GetBuffer(),szTraceString));

Cleanup:

    return;
}


_Check_return_
HRESULT
BindingExpression::IsValidValueForUpdate(
    _In_ IInspectable* pInValue,
    _In_ const CClassInfo* pSourceType,
    _Out_ bool* pfIsValidForUpdate)
{
    HRESULT hr = S_OK;
    if (PropertyValue::IsNullOrEmpty(pInValue))
    {
        // The value is NULL, it is only valid if the type of the source
        // property accepts NULL values
        *pfIsValidForUpdate = !pSourceType->IsValueType();
    }
    else
    {
        IFC(MetadataAPI::IsInstanceOfType(pInValue, pSourceType, pfIsValidForUpdate));
    }

Cleanup:

    RRETURN(hr);
}


_Check_return_
HRESULT
BindingExpression::GetTraceString(_Outptr_result_z_ const WCHAR **pszTraceString)
{
    HRESULT hr = S_OK;
    int iStringSize = 0;
    wrl_wrappers::HString strFormat;
    LPCWSTR szFormat = NULL;
    xstring_ptr strSource;
    LPCWSTR szSource = NULL;
    wrl_wrappers::HString strPropertyPath;
    LPCWSTR szPropertyPath = NULL;
    xstring_ptr strElementClassName;
    LPCWSTR szElementClassName = NULL;
    wrl_wrappers::HString strElementName;
    LPCWSTR szElementName = NULL;
    wrl_wrappers::HString strAutomationId;
    wrl_wrappers::HString strAutomationName;
    wrl_wrappers::HString strTemplatedParentAutomationId;
    ctl::ComPtr<IFrameworkElement> spFE;
    ctl::ComPtr<DependencyObject> spTemplatedParent;
    const CClassInfo* pTargetPropertyType = NULL;
    WCHAR *szTraceString = NULL;
    bool fTargetPegged = false;

    // If we have the string already then just return it
    if (m_szTraceString == NULL)
    {
        // Collect the strings that will be part of the tracing string
        if (m_tpEffectiveSource.Get() != NULL)
        {
            ctl::ComPtr<IInspectable> spEffectiveSource;
            spEffectiveSource.Attach(ValueWeakReference::get_value_as<IInspectable>(m_tpEffectiveSource.Get()));

            IFC(MetadataAPI::GetFriendlyRuntimeClassName(spEffectiveSource.Get(), &strSource));
            szSource = strSource.GetBuffer();
        }
        else
        {
            szSource = L"null";
        }

        IFC(m_tpBinding->GetPathString(strPropertyPath.GetAddressOf()));
        szPropertyPath = strPropertyPath.GetRawBuffer(NULL);

        if (TryPegTargetReference())
        {
            fTargetPegged = TRUE;
            IFC(MetadataAPI::GetFriendlyRuntimeClassName(ctl::as_iinspectable(GetTargetNoRef()), &strElementClassName));
            szElementClassName = strElementClassName.GetBuffer();
        }
        else
        {
            szElementClassName = L"null";
        }

        spFE = ctl::query_interface_cast<IFrameworkElement>(GetTargetNoRef());
        if (spFE)
        {
            IFC(spFE->get_Name(strElementName.GetAddressOf()));
        }

        if (strElementName.Get() != NULL)
        {
            szElementName = strElementName.GetRawBuffer(NULL);
        }
        else
        {
            szElementName = L"null";
        }

        pTargetPropertyType = m_pDP->GetPropertyType();

        IFC(DXamlCore::GetCurrentNoCreate()->GetNonLocalizedErrorString(TEXT_BINDINGTRACE_BINDINGEXPRESSION_TRACE, strFormat.GetAddressOf()));
        szFormat = strFormat.GetRawBuffer(NULL);

        // Create the string
        iStringSize = _scwprintf(szFormat,
            szPropertyPath,
            szSource,
            szElementClassName,
            szElementName,
            m_pDP->GetName().GetBuffer(),
            pTargetPropertyType->GetName().GetBuffer()) + 1;
        szTraceString = new WCHAR[iStringSize];

        IFCEXPECT(swprintf_s(szTraceString, iStringSize,
            szFormat,
            szPropertyPath,
            szSource,
            szElementClassName,
            szElementName,
            m_pDP->GetName().GetBuffer(),
            pTargetPropertyType->GetName().GetBuffer()) >= 0);

        m_szTraceString = szTraceString;
        szTraceString = NULL;
    }

    *pszTraceString = m_szTraceString;

Cleanup:

    if (fTargetPegged)
    {
        UnpegTargetReference();
    }

    delete[] szTraceString;

    RRETURN(hr);
}

_Check_return_
HRESULT
BindingExpression::PegTargetReference()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IDependencyObject> spObj;

    // No need to use the AutoReentrantReferenceLock here, as we're not changing any
    // data that's used by ReferenceTrackerWalk.

    // If there's no target ref then this means that
    // the binding expression is innefective which means
    // that we're done
    // The caller is responsible for dealing with this situation
    ASSERT(m_spTargetRef != NULL || m_spPeggedTarget == NULL);
    IFCCHECK_NOTRACE(m_spTargetRef);

    if (!m_spPeggedTarget)
    {
        ASSERT(m_iTargetPegCount == 0);

        IFC(m_spTargetRef.As(&spObj));
        IFCCHECK_NOTRACE(spObj);

        m_spPeggedTarget = spObj.Cast<DependencyObject>();
        m_spPeggedTarget->UpdatePeg(true);
        m_iTargetPegCount = 1;
    }
    else
    {
        m_iTargetPegCount++;
    }

Cleanup:

    RRETURN(hr);
}

bool BindingExpression::TryPegTargetReference()
{
    // If there is no target ref, then there is nothing to peg.  Check this before calling
    // PegTargetReference() to avoid it IFC-ing a failure.
    ASSERT(m_spTargetRef != NULL || m_spPeggedTarget == NULL);
    return (m_spTargetRef != NULL && SUCCEEDED(PegTargetReference()));
}

void BindingExpression::UnpegTargetReference()
{
    if (m_iTargetPegCount == 0)
    {
        // We should not have a pegged target if we are at ref count 0
        ASSERT(m_spPeggedTarget == NULL);
        return;
    }

    ASSERT(m_iTargetPegCount > 0);

    // We must have a pegged target at this point
    ASSERT(m_spPeggedTarget.Get());
    if (m_spPeggedTarget == NULL)
    {
        m_iTargetPegCount = 0;
        return;
    }

    m_iTargetPegCount--;
    if (m_iTargetPegCount == 0)
    {
        m_spPeggedTarget->UpdatePeg(false);
        m_spPeggedTarget = nullptr;
    }
}

DependencyObject *BindingExpression::GetTargetNoRef()
{
    ASSERT(m_spPeggedTarget != NULL && m_iTargetPegCount > 0);

    return m_spPeggedTarget.Get();
}
// The section below contains functions specifically for use by XAML debug tools.

_Check_return_
HRESULT
BindingExpression::GetBindingString(_Out_ HSTRING *phDebugString)
{
    HRESULT hr = S_OK;
    int iStringSize = 0;
    wrl_wrappers::HString strPropertyPath;
    LPCWSTR szPropertyPath = NULL;
    wrl_wrappers::HString strElementName;
    WCHAR szElementName[256] = {};
    WCHAR *szDebugString = NULL;

    // Get the path for the binding
    IFC(m_tpBinding->GetPathString(strPropertyPath.GetAddressOf()));
    szPropertyPath = strPropertyPath.GetRawBuffer(NULL);

    // Check to see if we're binding to element name
    IFC(m_tpBinding->get_ElementName(strElementName.GetAddressOf()));
    if (strElementName.Get() != NULL)
    {
        // If the PropertyPath is non-empty, then include a comma at the start
        // of the element name string.
        LPCWSTR szComma = (szPropertyPath[0] != L'\0') ? L", " : L"";
        swprintf_s(szElementName, ARRAYSIZE(szElementName), L"%sElementName=%s", szComma, strElementName.GetRawBuffer(NULL));
    }

    // Create the string
    iStringSize = _scwprintf(L"{Binding %s%s}",
        szPropertyPath,
        szElementName) + 1;
    szDebugString = new WCHAR[iStringSize];

    IFCEXPECT(swprintf_s(szDebugString, iStringSize,
        L"{Binding %s%s}",
        szPropertyPath,
        szElementName) >= 0);

    IFC(wrl_wrappers::HStringReference(szDebugString, iStringSize-1).CopyTo(phDebugString));

Cleanup:

    delete[] szDebugString;

    RRETURN(hr);
}

bool BindingExpression::IsBindingValid()
{
    return (m_tpListener && m_tpListener->FullPathExists());
}

PropertyPathStep* BindingExpression::GetFirstPropertyPathStep()
{
    PropertyPathStep *pStep = nullptr;

    if (m_tpListener)
    {
        pStep = m_tpListener->DebugGetFirstStep();
    }
    return pStep;
}

_Check_return_ HRESULT
BindingExpression::GetPath(_Out_ HSTRING *phPath)
{
    RRETURN(m_tpBinding->GetPathString(phPath));
}

_Check_return_ HRESULT
BindingExpression::GetBindingMode(_Out_ xaml_data::BindingMode* bindingMode)
{
    RRETURN(m_tpBinding->get_Mode(bindingMode));
}

_Check_return_ HRESULT
BindingExpression::GetSource(_Outptr_opt_ IInspectable **ppSource)
{
    HRESULT hr = S_OK;

    IFC(m_tpEffectiveSource ? S_OK : E_FAIL);

    *ppSource = ValueWeakReference::get_value_as<IInspectable>(m_tpEffectiveSource.Get());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
BindingExpression::GetElementName(_Out_ HSTRING *phElementName)
{
    RRETURN(m_tpBinding->get_ElementName(phElementName));
}

_Check_return_ HRESULT
BindingExpression::GetConverter(_Outptr_opt_ void **ppConverter)
{
    *ppConverter = m_tpValueConverter.Get();

    RRETURN(*ppConverter ? S_OK : E_FAIL);
}

_Check_return_ HRESULT
BindingExpression::GetConverterParameter(_Outptr_opt_ IInspectable **ppConverterParameter)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<IInspectable> spValue;

    IFC(m_tpBinding->get_ConverterParameter(&spValue));
    IFC(spValue ? S_OK : E_FAIL);

    *ppConverterParameter = ValueWeakReference::get_value_as<IInspectable>(spValue.Get());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
BindingExpression::GetConverterLanguage(_Out_ HSTRING *phConverterLanguage)
{
    RRETURN(m_tpBinding->get_ConverterLanguage(phConverterLanguage));
}

_Check_return_ HRESULT
BindingExpression::GetFallbackValue(_Outptr_opt_ IInspectable **ppFallbackValue)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<IInspectable> spValue;

    IFC(m_tpBinding->get_FallbackValue(&spValue));
    IFC(spValue ? S_OK : E_FAIL);

    *ppFallbackValue = ValueWeakReference::get_value_as<IInspectable>(spValue.Get());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
BindingExpression::GetTargetNullValue(_Outptr_opt_ IInspectable **ppTargetNullValue)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<IInspectable> spValue;

    IFC(m_tpBinding->get_TargetNullValue(&spValue));
    IFC(spValue ? S_OK : E_FAIL);

    *ppTargetNullValue = ValueWeakReference::get_value_as<IInspectable>(spValue.Get());

Cleanup:
    RRETURN(hr);
}

void BindingExpression::TraceUpdateBindingStart(_In_ bool updateTarget)
{
    // Ensure that this is a UI element
    if (ctl::is<IUIElement>(GetTargetNoRef()))
    {
        CUIElement* pUIElement = ctl::ComPtr<DependencyObject>(GetTargetNoRef()).Cast<UIElement>()->GetHandle();
        auto propertyName = m_pDP->GetName();
        CUIElement* pParent = pUIElement->GetUIElementParentInternal();
        xstring_ptr sourceClassName;

        // Go through MetadataAPI::GetFriendlyRuntimeClassName to get a nice ouput string. Sometimes we can get really ugly looking
        // strings like: MyApp.MyType, MyApp, Version=abc.easyas.123, Culture=neutral, etc. This API will strip the stuff after
        // MyApp.MyType from the class name.
        if (m_tpEffectiveSource.Get() != nullptr)
        {
            ctl::ComPtr<IInspectable> effectiveSource;
            effectiveSource.Attach(ValueWeakReference::get_value_as<IInspectable>(m_tpEffectiveSource.Get()));

            IGNOREHR(MetadataAPI::GetFriendlyRuntimeClassName(effectiveSource.Get(), &sourceClassName));
        }

        auto propertyPathStep = m_tpListener->DebugGetFirstStep();
        std::wstring modelPropertyName;

        wchar_t* tempName = nullptr;
        // If there are multiple steps in the property path we will want to include those
        while (propertyPathStep)
        {
            tempName = propertyPathStep->DebugGetPropertyName();
            if (tempName)
            {
                modelPropertyName.append(tempName);
            }
            propertyPathStep = propertyPathStep->GetNextStep();
            // If next step is not null, then append a "."
            if (tempName && propertyPathStep)
            {
                modelPropertyName.append(L".");
            }
        }

        if (updateTarget)
        {
            TraceUpdateTargetBindingBegin1(
                reinterpret_cast<UINT64>(static_cast<CDependencyObject*>(pUIElement)),
                reinterpret_cast<UINT64>(static_cast<CDependencyObject*>(pParent)),
                propertyName.GetBuffer(),
                sourceClassName.GetBuffer(),
                modelPropertyName.c_str(),
                pUIElement->IsPropertyTemplateBound(m_pDP),
                m_effectiveSourceType
                );
        }
        else
        {
            TraceUpdateSourceBindingBegin(
                reinterpret_cast<UINT64>(pUIElement),
                reinterpret_cast<UINT64>(pParent),
                propertyName.GetBuffer(),
                sourceClassName.GetBuffer(),
                modelPropertyName.c_str()
                );
        }
    }
}

void BindingExpression::TraceUpdateBindingEnd(_In_ bool updateTarget)
{
    // Ensure that this is a UI element
    if (ctl::is<IUIElement>(GetTargetNoRef()))
    {
        auto pUIElement = ctl::ComPtr<DependencyObject>(GetTargetNoRef()).Cast<UIElement>()->GetHandle();
        auto propertyName = m_pDP->GetName();
        auto pParent = pUIElement->GetUIElementParentInternal();

        if (updateTarget)
        {
            TraceUpdateTargetBindingEnd(
                reinterpret_cast<UINT64>(pUIElement),
                reinterpret_cast<UINT64>(pParent),
                propertyName.GetBuffer());
        }
        else
        {
            TraceUpdateSourceBindingEnd(
                reinterpret_cast<UINT64>(pUIElement),
                reinterpret_cast<UINT64>(pParent),
                propertyName.GetBuffer()
                );
        }
    }
}

void
BindingExpression::NotifyThemeChanged(
    _In_ Theming::Theme theme,
    _In_ bool forceRefresh,
    _Out_ bool& valueChanged)
{
    // Only Bindings with a {ThemeResource} on TargetNullValue/FallbackValue need to respond to the theme change.
    // Blindly refreshing every Binding on theme change can result in unexpected behavior if a converter that
    // returns a complex object is involved (because converters generally don't maintain reference equality).
    // We additionally do our best to test for equality to avoid an unnecessary refresh, since that can cause
    // subtle timing changes.
    //
    // Errors are swallowed because that's what Bindings do

    valueChanged = false;

    if (m_tpBinding)
    {
        xaml_data::BindingMode mode;
        if (FAILED(m_tpBinding->get_Mode(&mode)))
        {
            return;
        }

        auto bindingAsCoreDO = m_tpBinding->GetHandle();
        if (mode != xaml_data::BindingMode_OneTime && bindingAsCoreDO)
        {
            ASSERT(mode == xaml_data::BindingMode_OneWay || mode == xaml_data::BindingMode_TwoWay);

            bool shouldNotifyThemeChange = false;
            switch (m_bindingValueSource)
            {
                // This is the case where, for whatever reason, the Binding is using the target property's default value
                // (or maybe has not yet been evaluated). In those cases, we want to make sure we give the Binding a chance
                // to re-evaluate, in case a ThemeResource on FallbackValue/TargetNullValue will end up providing a valid value.
                case BindingValueSource::Unknown:
                {
                    shouldNotifyThemeChange = bindingAsCoreDO->HasThemeResourceBinding(KnownPropertyIndex::Binding_FallbackValue)
                        || bindingAsCoreDO->HasThemeResourceBinding(KnownPropertyIndex::Binding_TargetNullValue);
                }
                break;
                case BindingValueSource::FallbackValue:
                {
                    shouldNotifyThemeChange = bindingAsCoreDO->HasThemeResourceBinding(KnownPropertyIndex::Binding_FallbackValue);
                }
                break;
                case BindingValueSource::TargetNullValue:
                {
                    shouldNotifyThemeChange = bindingAsCoreDO->HasThemeResourceBinding(KnownPropertyIndex::Binding_TargetNullValue);
                }
                break;
            }

            if (shouldNotifyThemeChange)
            {
                ctl::ComPtr<IInspectable> oldValue;
                if (FAILED(GetValue(nullptr /* unused */, nullptr /* unused */, oldValue.ReleaseAndGetAddressOf())))
                {
                    return;
                }

                if (FAILED(bindingAsCoreDO->NotifyThemeChanged(theme, forceRefresh)))
                {
                    return;
                }

                ctl::ComPtr<IInspectable> newValue;
                if (FAILED(GetValue(nullptr /* unused */, nullptr /* unused */, newValue.ReleaseAndGetAddressOf())))
                {
                    return;
                }

                bool areEqual = false;
                if (FAILED(PropertyValue::AreEqual(oldValue.Get(), newValue.Get(), &areEqual)))
                {
                    return;
                }

                valueChanged = !areEqual;
            }
        }
    }
}

namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_BindingOperations()
    {
        RRETURN(ctl::ActivationFactoryCreator<DirectUI::BindingOperations>::CreateActivationFactory());
    }
}
