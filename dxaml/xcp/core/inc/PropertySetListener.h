// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <NamespaceAliases.h>
#include <FacadeStorage.h>

class CUIElement;

// Helper class that maps DComp PropertySet IDs to facade IDs
// When listening for PropertySet properties, DComp has the convention that a property ID
// maps to the order the property was inserted into the PropertySet.
// This class helps manage that dynamic relationship by building a mapping as properties are inserted.
class FacadePropertyMapper
{
public:
    FacadePropertyMapper() {}
    ~FacadePropertyMapper() {}

    bool EstablishMappingForFacadeID(KnownPropertyIndex facadeID);
    KnownPropertyIndex GetFacadeID(ixp::ExpExpressionNotificationProperty propertyID) const;
    int GetPropertyID(KnownPropertyIndex facadeID) const;

private:
    std::vector<KnownPropertyIndex> m_facadeIDs;
};

// Class to listen for PropertySet changes, currently specialized to suit CUIElement's needs
class PropertySetListener :
      public wrl::RuntimeClass<IFacadePropertyListener, ixp::IExpCompositionPropertyChangedListener>
{
    InspectableClass(nullptr /* this class is internal */, BaseTrust);

public:
    PropertySetListener(_In_ CUIElement* uielement);
    ~PropertySetListener() override;

    bool ShouldTrackFacadeProperty(KnownPropertyIndex facadeID) const;

    void OnFacadePropertyInserted(KnownPropertyIndex facadeID) override;

    void OnAnimationStarted(
        _In_ WUComp::ICompositionObject* backingPropertySet,
        KnownPropertyIndex facadeID) override;

    void OnAnimationCompleted(
        _In_ WUComp::ICompositionObject* backingPropertySet,
        KnownPropertyIndex facadeID) override;

    void PopulateAnimatedPropertyWithCurrentValue(KnownPropertyIndex facadeID);

    // IExpCompositionPropertyChangedListener
    IFACEMETHOD(NotifyColorPropertyChanged)(
        _In_ ixp::ICompositionObject* pTarget,
        _In_ ixp::ExpExpressionNotificationProperty propertyId,
        _In_ wu::Color value);

    IFACEMETHOD(NotifyMatrix3x2PropertyChanged)(
        _In_ ixp::ICompositionObject* pTarget,
        _In_ ixp::ExpExpressionNotificationProperty propertyId,
        _In_ wfn::Matrix3x2 value);

    IFACEMETHOD(NotifyMatrix4x4PropertyChanged)(
        _In_ ixp::ICompositionObject* pTarget,
        _In_ ixp::ExpExpressionNotificationProperty propertyId,
        _In_ wfn::Matrix4x4 value);

    IFACEMETHOD(NotifyReferencePropertyChanged)(
        _In_ ixp::ICompositionObject* pTarget,
        _In_ ixp::ExpExpressionNotificationProperty propertyId);

    IFACEMETHOD(NotifySinglePropertyChanged)(
        _In_ ixp::ICompositionObject* pTarget,
        _In_ ixp::ExpExpressionNotificationProperty propertyId,
        _In_ float value);

    IFACEMETHOD(NotifyVector2PropertyChanged)(
        _In_ ixp::ICompositionObject* pTarget,
        _In_ ixp::ExpExpressionNotificationProperty propertyId,
        _In_ wfn::Vector2 value);

    IFACEMETHOD(NotifyVector3PropertyChanged)(
        _In_ ixp::ICompositionObject* pTarget,
        _In_ ixp::ExpExpressionNotificationProperty propertyId,
        _In_ wfn::Vector3 value);

    IFACEMETHOD(NotifyVector4PropertyChanged)(
        _In_ ixp::ICompositionObject* pTarget,
        _In_ ixp::ExpExpressionNotificationProperty propertyId,
        _In_ wfn::Vector4 value);

    IFACEMETHOD(NotifyBooleanPropertyChanged)(
        _In_ ixp::ICompositionObject* pTarget,
        _In_ ixp::ExpExpressionNotificationProperty propertyId,
        _In_ boolean value);

private:
    CUIElement* m_pUIElementNoRef = nullptr;
    FacadePropertyMapper m_propertyMapper;
};


