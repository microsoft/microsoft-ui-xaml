// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "MetadataAPI.h"
#include "ThemeResource.h"
#include <OptimizedStyle.h>
#include "Theme.h"
#include <CVisualStateManager2.h>
using namespace DirectUI;


CSetter::~CSetter()
{
    ReleaseInterface(m_pDependencyPropertyProxy);
    ReleaseInterface(m_target);

    if (m_vValue.AsObject())
    {
        // Make sure the DependencyObject m_vValue object will not attempt to access
        // this deleted setter anymore. This is needed because CDependencyObject::ResetReferencesFromChildren()
        // does not clean up this member.
        IGNOREHR(ResetReferenceFromChild(m_vValue.AsObject()));
    }
}

_Check_return_ HRESULT
CSetter::GetProperty(_In_ KnownTypeIndex typeIndex, _Out_ KnownPropertyIndex *puiPropertyId)
{
    *puiPropertyId = KnownPropertyIndex::UnknownType_UnknownProperty;

    if (m_pDependencyPropertyProxy)
    {
        *puiPropertyId = m_pDependencyPropertyProxy->GetDP()->GetIndex();
    }
    else if (m_target)
    {
        if (typeIndex != KnownTypeIndex::UnknownType)
        {
            IFC_RETURN(m_target->ResolveTargetForStyleSetter(typeIndex, puiPropertyId));
        }
        else
        {
            const CDependencyProperty* pDP;
            xref_ptr<CDependencyObject> targetObject;
            xref_ptr<CDependencyObject> targetPropertyOwner;
            IGNOREHR(m_target->ResolveTargetForVisualStateSetter(this, targetObject, targetPropertyOwner, &pDP));
            if (pDP)
            {
                *puiPropertyId = pDP->GetIndex();
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
CSetter::SetValue(_In_ const SetValueParams& args)
{
    if (   m_bIsSealed
        && (   args.m_pDP->GetIndex() == KnownPropertyIndex::Setter_Property
            || (args.m_pDP->GetIndex() == KnownPropertyIndex::Setter_Value && !m_isValueMutable)
            || args.m_pDP->GetIndex() == KnownPropertyIndex::Setter_Target))
    {
        IFC_RETURN(E_ACCESSDENIED);
    }

    // Can't style the Style property
    if (args.m_pDP->GetIndex() == KnownPropertyIndex::FrameworkElement_Style)
    {
        IFC_RETURN(E_FAIL);
    }

    if (args.m_pDP->GetIndex() == KnownPropertyIndex::Setter_Value)
    {
        // since a new value is being set, unmark as resolved
        m_valueResolved = false;
    }

    IFC_RETURN(__super::SetValue(args));

    return S_OK;
}

_Check_return_ HRESULT
CSetter::GetValue(
    _In_ const CDependencyProperty *pdp,
    _Out_ CValue *pValue)
{
    // delegate to baseclass implementation except for our Setter_Value
    // m_DependencyPropertyProxy is not guaranteed to be set yet
    if (   pdp->GetIndex() == KnownPropertyIndex::Setter_Value
        && (m_pDependencyPropertyProxy || m_target)
        && !IsPropertyDefault(pdp))
    {
        IFC_RETURN(GetSetterValue(pValue));
    }
    else
    {
        IFC_RETURN(__super::GetValue(pdp, pValue));
    }

    return S_OK;
}

void CSetter::SetTemplatedParentImpl(_In_ CDependencyObject* parent)
{
    m_templatedParent = xref::get_weakref(parent);
}

CDependencyObject* CSetter::GetTemplatedParent()
{
    if (!m_templatedParent.expired())
    {
        // Apparently GetTemplatedParent() isn't expected to addref its return value...
        return m_templatedParent.lock_noref();
    }
    return nullptr;
}

_Check_return_ HRESULT
CSetter::GetSetterValue(_Out_ CValue *pValue)
{
    if (!m_valueResolved && !m_vValue.IsUnset())
    {
        const CDependencyProperty* pDP = nullptr;
        IFC_RETURN(ResolveDependencyProperty(&pDP));
        if (pDP)
        {
            IFC_RETURN(ResolveValueUsingProperty(pDP));
        }
    }

    if (m_vValue.IsUnset())
    {
        pValue->Unset();
    }
    else
    {
        IFC_RETURN(pValue->CopyConverted(m_vValue));
    }

    return S_OK;
}

// Get the target DP from either the DP proxy or target property path.
_Check_return_ HRESULT
CSetter::ResolveDependencyProperty(_Outptr_result_maybenull_ const CDependencyProperty** ppDP)
{
    // The 'Property' and 'Target' properties should not both be set; only one or the other
    if (m_pDependencyPropertyProxy && m_target)
    {
        IFC_RETURN(SetAndOriginateError(E_NER_INVALID_OPERATION, RuntimeError, AG_E_SETTER_AMBIGUOUS_TARGET));
    }

    *ppDP = nullptr;

    if (m_pDependencyPropertyProxy)
    {
        // If Setter.Property was set
        // Note: when a Style Setter is processed by XBF, Setter.Target is effectively transformed
        // into Setter.Property, which allows us to follow this fast path.
        *ppDP = m_pDependencyPropertyProxy->GetDP();
    }
    else if (m_target)
    {
        // If Setter.Target was set
        // If this isn't a Style setter (i.e. m_target has a target name), then we don't want
        // to try to resolve the target DP during a theme change notification. This is because
        // for a VSM setter, resolving the target DP implicitly resolves the target object as well.
        // If that target object is still deferred, then the act of resolving it will cause it to
        // unnecessarily undefer.
        if (!m_themeChanging || m_target->IsForStyleSetter())
        {
            // Do a best effort here, since we don't have enough information to determine if we should resolve
            // as a Style Setter or a VisualState Setter. If the Setter/TargetPropertyPath were created in XAML,
            // then that's OK because the required information would've been determined during type conversion.
            // If the Setter was created in code-behind, then Setter.Value shouldn't need to be type converted
            // anyway.
            IFC_RETURN(m_target->GenericResolveTargetProperty(ppDP));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
CSetter::ResolveValueUsingProperty(_In_ const CDependencyProperty* dp)
{
    if (!m_valueResolved && !m_vValue.IsUnset())
    {
        bool createdObject = false;
        IFC_RETURN(OptimizedStyle::ResolveValue(GetContext(), dp, m_vValue, &createdObject));
        m_valueResolved = true;
    }

    return S_OK;
}

_Check_return_ HRESULT
CSetter::NotifyThemeChangedCore(_In_ Theming::Theme theme, _In_ bool forceRefresh)
{
    m_themeChanging = true;
    auto scopeGuard = wil::scope_exit([this]
        {
            this->m_themeChanging = false;
        });
    IFC_RETURN(NotifyThemeChangedCoreImpl(theme, forceRefresh, /* ignoreGetValueFailures */ true));

    return S_OK;
}

_Check_return_ HRESULT
CSetter::OnPropertyChanged(_In_ const PropertyChangedParams& args)
{
    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::Setter_Property:
        {
            IFC_RETURN(OnSetterPropertyChanged(args));
        }
        break;

        case KnownPropertyIndex::Setter_Target:
        {
            IFC_RETURN(OnSetterTargetChanged(args));
        }
        break;

        case KnownPropertyIndex::Setter_Value:
        {
            IFC_RETURN(OnSetterValueChanged(args));
        }
        break;
    }

    return S_OK;
}

_Check_return_ HRESULT
CSetter::OnSetterPropertyChanged(_In_ const PropertyChangedParams& args)
{
    auto oldProperty = (!args.m_pOldValue->IsNull())
        ? static_cast<CDependencyPropertyProxy*>(args.m_pOldValue->AsObject())->GetDP()->GetIndex()
        : KnownPropertyIndex::UnknownType_UnknownProperty;

    auto newProperty = (!args.m_pNewValue->IsNull())
            ? static_cast<CDependencyPropertyProxy*>(args.m_pNewValue->AsObject())->GetDP()->GetIndex()
            : KnownPropertyIndex::UnknownType_UnknownProperty;

    auto& subscribers = EnsureSetterValueChangedNoficationSubscribersStorage();
    for(auto& subscriber : subscribers)
    {
        auto style = subscriber.lock();
        if (style)
        {
            if (oldProperty != KnownPropertyIndex::UnknownType_UnknownProperty)
            {
                GetContext()->NotifyMutableStyleValueChangedListeners(style, oldProperty);
            }
            if (newProperty != KnownPropertyIndex::UnknownType_UnknownProperty)
            {
                GetContext()->NotifyMutableStyleValueChangedListeners(style, newProperty);
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
CSetter::OnSetterTargetChanged(_In_ const PropertyChangedParams& args)
{
    auto resolveTargetHelper = [&](CTargetPropertyPath* target, KnownTypeIndex targetTypeIndex) -> KnownPropertyIndex
    {
        KnownPropertyIndex propertyIndex = KnownPropertyIndex::UnknownType_UnknownProperty;

        if (target != nullptr)
        {
            IGNOREHR(target->ResolveTargetForStyleSetter(targetTypeIndex, &propertyIndex));
        }

        return propertyIndex;
    };

    auto oldTarget = (!args.m_pOldValue->IsNull())
        ? static_cast<CTargetPropertyPath*>(args.m_pOldValue->AsObject())
        : nullptr;

    auto newTarget = (!args.m_pNewValue->IsNull())
        ? static_cast<CTargetPropertyPath*>(args.m_pNewValue->AsObject())
        : nullptr;

    auto& subscribers = EnsureSetterValueChangedNoficationSubscribersStorage();
    for(auto& subscriber : subscribers)
    {
        auto style = subscriber.lock();
        if (style)
        {
            const auto oldProperty = resolveTargetHelper(oldTarget, style->m_targetTypeIndex);
            if (oldProperty != KnownPropertyIndex::UnknownType_UnknownProperty)
            {
                GetContext()->NotifyMutableStyleValueChangedListeners(style, oldProperty);
            }

            const auto newProperty = resolveTargetHelper(newTarget, style->m_targetTypeIndex);
            if (newProperty != KnownPropertyIndex::UnknownType_UnknownProperty)
            {
                GetContext()->NotifyMutableStyleValueChangedListeners(style, newProperty);
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
CSetter::OnSetterValueChanged(_In_ const PropertyChangedParams& args)
{
    // Re-apply Setter for VisualState Setters in active VisualStates if Value is unset
    if (!IsStyleSetter() && args.m_pOldValue && args.m_pOldValue->IsUnset())
    {
        CVisualState* visualState = VisualStateSetterHelper::GetVisualStateSetterVisualState(this);

        if (visualState)
        {
            VisualStateSetterHelper::PerformSetterOperationIfStateActive(
                        visualState, this, VisualStateSetterHelper::SetterOperation::Set);
        }
    }

    auto& subscribers = EnsureSetterValueChangedNoficationSubscribersStorage();
    for(auto& subscriber : subscribers)
    {
        auto style = subscriber.lock();
        if (style)
        {
            IFC_RETURN(style->NotifyMutableSetterValueChanged(this));
        }
    }

    return S_OK;
}

void CSetter::SubscribeToValueChangedNotification(_In_ CStyle* const subscriber)
{
    auto& subscribers = EnsureSetterValueChangedNoficationSubscribersStorage();
    subscribers.emplace(xref::get_weakref(subscriber));
}

void CSetter::UnsubscribeFromValueChangedNotification(_In_ CStyle* const subscriber)
{
    const auto subscribers = GetSetterValueChangedNoficationSubscribersStorage();
    if (subscribers)
    {
        subscribers->erase(xref::get_weakref(subscriber));
    }
}

xref_ptr<CTargetPropertyPath> CSetter::GetTargetPropertyPath()
{
    return xref_ptr<CTargetPropertyPath>(m_target);
}

bool CSetter::IsStyleSetter() const
{
    return (!m_target || m_target->IsForStyleSetter());
}

