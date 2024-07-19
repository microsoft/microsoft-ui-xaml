// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PropertySetListener.h"
#include <SimpleProperties.h>
#include <UIElement.h>
#include <corep.h>

using namespace Microsoft::WRL;

// Establishes a mapping for the given facadeID.  The convention that must be followed is:
// Call this method just after inserting any property into the owning PropertySet being tracked.
bool FacadePropertyMapper::EstablishMappingForFacadeID(KnownPropertyIndex facadeID)
{
    // First search for the presence of this facadeID in our little array
    for (size_t i = 0; i < m_facadeIDs.size(); i++)
    {
        if (m_facadeIDs[i] == facadeID)
        {
            // The facadeID is already present.  We already know its index in the array.
            return false;
        }
    }

    // If we get here, we haven't found the facadeID in our array, insert it to the end now.
    // This yields the behavior that the order of each facadeID in the array establishes its DComp property ID.
    m_facadeIDs.push_back(facadeID);

    return true;
}

// Maps a DComp propertyID to a facadeID
KnownPropertyIndex FacadePropertyMapper::GetFacadeID(ixp::ExpExpressionNotificationProperty propertyID) const
{
    ASSERT(propertyID < static_cast<int>(m_facadeIDs.size()));

    return m_facadeIDs[propertyID];
}

// Maps a facadeID to a DComp Property ID
int FacadePropertyMapper::GetPropertyID(KnownPropertyIndex facadeID) const
{
    for (size_t i = 0; i < m_facadeIDs.size(); i++)
    {
        if (m_facadeIDs[i] == facadeID)
        {
            return static_cast<int>(i);
        }
    }

    ASSERT(FALSE);
    return -1;
}

PropertySetListener::PropertySetListener(_In_ CUIElement* uielement)
: m_pUIElementNoRef(uielement)
{
}

PropertySetListener::~PropertySetListener()
{
}

// Helper function to determine if we care about the given facadeID
bool PropertySetListener::ShouldTrackFacadeProperty(KnownPropertyIndex facadeID) const
{
    // UIElement.Opacity is never stored in the PropertySet as it's already a DependencyProperty, no need to track it.
    return (facadeID != KnownPropertyIndex::UIElement_Opacity);
}

// Called whenever the given facade has a value set on the underlying PropertySet
void PropertySetListener::OnFacadePropertyInserted(KnownPropertyIndex facadeID)
{
    if (ShouldTrackFacadeProperty(facadeID))
    {
        m_propertyMapper.EstablishMappingForFacadeID(facadeID);
    }
}

// Called whenever an animation is started on the given facadeID
void PropertySetListener::OnAnimationStarted(
    _In_ WUComp::ICompositionObject* backingPropertySet,
    KnownPropertyIndex facadeID)
{
    if (ShouldTrackFacadeProperty(facadeID))
    {
        // There is a delay before we hear back about the initial value.  Pre-populate our shadow property with current value now.
        PopulateAnimatedPropertyWithCurrentValue(facadeID);

        wrl::ComPtr<ixp::IExpCompositionPropertyChanged> notifier;
        VERIFYHR(backingPropertySet->QueryInterface(IID_PPV_ARGS(&notifier)));
        IFCFAILFAST(notifier->SetPropertyChangedListener(static_cast<ixp::ExpExpressionNotificationProperty>(m_propertyMapper.GetPropertyID(facadeID)), this));
    }
}

// Called whenever an animation completes on the given facadeID
void PropertySetListener::OnAnimationCompleted(
    _In_ WUComp::ICompositionObject* backingPropertySet,
    KnownPropertyIndex facadeID)
{
    if (ShouldTrackFacadeProperty(facadeID))
    {
        wrl::ComPtr<ixp::IExpCompositionPropertyChanged> notifier;
        VERIFYHR(backingPropertySet->QueryInterface(IID_PPV_ARGS(&notifier)));
        IFCFAILFAST(notifier->SetPropertyChangedListener(static_cast<ixp::ExpExpressionNotificationProperty>(m_propertyMapper.GetPropertyID(facadeID)), nullptr));
    }
}

// Helper function to populate our "shadow property" with the current static value for a property we're tracking
void PropertySetListener::PopulateAnimatedPropertyWithCurrentValue(KnownPropertyIndex facadeID)
{
    switch(facadeID)
    {
    case KnownPropertyIndex::UIElement_Translation:
        SimpleProperty::Property::id<KnownPropertyIndex::UIElement_AnimatedTranslation>::Set(
            m_pUIElementNoRef,
            SimpleProperty::Property::id<KnownPropertyIndex::UIElement_Translation>::Get(m_pUIElementNoRef));
        break;
    case KnownPropertyIndex::UIElement_Rotation:
        SimpleProperty::Property::id<KnownPropertyIndex::UIElement_AnimatedRotation>::Set(
            m_pUIElementNoRef,
            SimpleProperty::Property::id<KnownPropertyIndex::UIElement_Rotation>::Get(m_pUIElementNoRef));
        break;
    case KnownPropertyIndex::UIElement_Scale:
        SimpleProperty::Property::id<KnownPropertyIndex::UIElement_AnimatedScale>::Set(
            m_pUIElementNoRef,
            SimpleProperty::Property::id<KnownPropertyIndex::UIElement_Scale>::Get(m_pUIElementNoRef));
        break;
    case KnownPropertyIndex::UIElement_TransformMatrix:
        SimpleProperty::Property::id<KnownPropertyIndex::UIElement_AnimatedTransformMatrix>::Set(
            m_pUIElementNoRef,
            SimpleProperty::Property::id<KnownPropertyIndex::UIElement_TransformMatrix>::Get(m_pUIElementNoRef));
        break;
    case KnownPropertyIndex::UIElement_RotationAxis:
        SimpleProperty::Property::id<KnownPropertyIndex::UIElement_AnimatedRotationAxis>::Set(
            m_pUIElementNoRef,
            SimpleProperty::Property::id<KnownPropertyIndex::UIElement_RotationAxis>::Get(m_pUIElementNoRef));
        break;
    case KnownPropertyIndex::UIElement_CenterPoint:
        SimpleProperty::Property::id<KnownPropertyIndex::UIElement_AnimatedCenterPoint>::Set(
            m_pUIElementNoRef,
            SimpleProperty::Property::id<KnownPropertyIndex::UIElement_CenterPoint>::Get(m_pUIElementNoRef));
        break;
    }
}

IFACEMETHODIMP PropertySetListener::NotifySinglePropertyChanged(
    _In_ ixp::ICompositionObject* target,
    _In_ ixp::ExpExpressionNotificationProperty propertyId,
    _In_ float value)
{
    KnownPropertyIndex facadeID = m_propertyMapper.GetFacadeID(propertyId);

    switch(facadeID)
    {
    case KnownPropertyIndex::UIElement_Rotation:
        SimpleProperty::Property::id<KnownPropertyIndex::UIElement_AnimatedRotation>::Set(m_pUIElementNoRef, value);
        break;
    }
    m_pUIElementNoRef->GetContext()->DecrementPendingAnimatedFacadePropertyChangeCount();

    CUIElement::NWSetTransformDirty(m_pUIElementNoRef, DirtyFlags::Independent | DirtyFlags::Bounds);

    m_pUIElementNoRef->UpdateHas3DDepth();

    return S_OK;
}

IFACEMETHODIMP PropertySetListener::NotifyVector2PropertyChanged(
    _In_ ixp::ICompositionObject* pTarget,
    _In_ ixp::ExpExpressionNotificationProperty propertyId,
    _In_ wfn::Vector2 value)
{
    return S_OK;
}

IFACEMETHODIMP PropertySetListener::NotifyVector3PropertyChanged(
    _In_ ixp::ICompositionObject* pTarget,
    _In_ ixp::ExpExpressionNotificationProperty propertyId,
    _In_ wfn::Vector3 value)
{
    KnownPropertyIndex facadeID = m_propertyMapper.GetFacadeID(propertyId);

    switch(facadeID)
    {
    case KnownPropertyIndex::UIElement_Translation:
        SimpleProperty::Property::id<KnownPropertyIndex::UIElement_AnimatedTranslation>::Set(m_pUIElementNoRef, value);
        break;
    case KnownPropertyIndex::UIElement_Scale:
        SimpleProperty::Property::id<KnownPropertyIndex::UIElement_AnimatedScale>::Set(m_pUIElementNoRef, value);
        break;
    case KnownPropertyIndex::UIElement_CenterPoint:
        SimpleProperty::Property::id<KnownPropertyIndex::UIElement_AnimatedCenterPoint>::Set(m_pUIElementNoRef, value);
        break;
    case KnownPropertyIndex::UIElement_RotationAxis:
        SimpleProperty::Property::id<KnownPropertyIndex::UIElement_AnimatedRotationAxis>::Set(m_pUIElementNoRef, value);
        break;
    }
    m_pUIElementNoRef->GetContext()->DecrementPendingAnimatedFacadePropertyChangeCount();

    CUIElement::NWSetTransformDirty(m_pUIElementNoRef, DirtyFlags::Independent | DirtyFlags::Bounds);

    m_pUIElementNoRef->UpdateHas3DDepth();

    return S_OK;
}

IFACEMETHODIMP PropertySetListener::NotifyVector4PropertyChanged(
    _In_ ixp::ICompositionObject* pTarget,
    _In_ ixp::ExpExpressionNotificationProperty propertyId,
    _In_ wfn::Vector4 value)
{
    return S_OK;
}

IFACEMETHODIMP PropertySetListener::NotifyColorPropertyChanged(
    _In_ ixp::ICompositionObject* pTarget,
    _In_ ixp::ExpExpressionNotificationProperty propertyId,
    _In_ wu::Color value)
{
    return S_OK;
}

IFACEMETHODIMP PropertySetListener::NotifyMatrix3x2PropertyChanged(
    _In_ ixp::ICompositionObject* target,
    _In_ ixp::ExpExpressionNotificationProperty propertyId,
    _In_ wfn::Matrix3x2 value)
{
    return S_OK;
}

IFACEMETHODIMP PropertySetListener::NotifyMatrix4x4PropertyChanged(
    _In_ ixp::ICompositionObject *pTarget,
    _In_ ixp::ExpExpressionNotificationProperty propertyId,
    _In_ wfn::Matrix4x4 value)
{
    wfn::Matrix4x4 valueMatrix4x4 = *reinterpret_cast<wfn::Matrix4x4*>(&value);
    KnownPropertyIndex facadeID = m_propertyMapper.GetFacadeID(propertyId);

    switch(facadeID)
    {
    case KnownPropertyIndex::UIElement_TransformMatrix:
        SimpleProperty::Property::id<KnownPropertyIndex::UIElement_AnimatedTransformMatrix>::Set(m_pUIElementNoRef, valueMatrix4x4);
        m_pUIElementNoRef->OnTransformMatrixForHitTestingChanged(valueMatrix4x4);
        break;
    }
    m_pUIElementNoRef->GetContext()->DecrementPendingAnimatedFacadePropertyChangeCount();

    CUIElement::NWSetTransformDirty(m_pUIElementNoRef, DirtyFlags::Independent | DirtyFlags::Bounds);

    m_pUIElementNoRef->UpdateHas3DDepth();

    return S_OK;
}

IFACEMETHODIMP PropertySetListener::NotifyBooleanPropertyChanged(
    _In_ ixp::ICompositionObject* target,
    _In_ ixp::ExpExpressionNotificationProperty propertyId,
    _In_ boolean value)
{
    return S_OK;
}

IFACEMETHODIMP PropertySetListener::NotifyReferencePropertyChanged(
    _In_ ixp::ICompositionObject* pTarget,
    _In_ ixp::ExpExpressionNotificationProperty propertyId)
{
    return S_OK;
}
