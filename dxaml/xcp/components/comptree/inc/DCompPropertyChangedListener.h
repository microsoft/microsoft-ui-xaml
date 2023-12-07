// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <NamespaceAliases.h>
#include <microsoft.ui.composition.h>
#include <microsoft.ui.composition.experimental.h>

class CUIElement;

// Listens to a UIElement HandOff visual's DComp property changes. When the app changes the
// the DComp properties using WUComp::IVisual, the corresponding hit-test
// transforms are changed.
class DCompPropertyChangedListener :
      public wrl::RuntimeClass<ixp::IExpCompositionPropertyChangedListener>
{
    InspectableClass(nullptr /* this class is internal */, BaseTrust);

public:
    DCompPropertyChangedListener(_In_ CUIElement* uiElement);
    ~DCompPropertyChangedListener() override;

    void AttachToHandOffVisual(_In_ IInspectable* handOffVisual);
    void DetachFromHandOffVisual();
    void AttachToPrependVisual(_In_ IInspectable* prependVisual);
    void DetachFromPrependVisual();
    void AttachToWUCClip(_In_ IInspectable* wucInsetClip);
    void DetachFromWUCClip();

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

    void ForceUpdateFromLayoutOffset();

private:
    _Check_return_ HRESULT UpdateOverallOffset(_In_ CUIElement* uielement);
    _Check_return_ HRESULT UpdateOverallCenterPoint(_In_ CUIElement* uielement);
    void UpdateClip(_In_ CUIElement* uiElement);

    // The clip object on the visual changed. Detach from the old clip, attach to the new clip.
    void OnVisualClipChanged(_In_ ixp::ICompositionObject* visual, _In_ CUIElement* uiElement);

    wrl::ComPtr<WUComp::IInsetClip> GetInsetClipFromVisual(_In_ ixp::ICompositionObject* visual);

private:
    // Weak Reference to UIElement, whose HandOff visual property changes are being listened to. Weak to prevent cycle. UIElement will set this.
    xref::weakref_ptr<CUIElement> m_uiElementWeakRef;

    wrl::ComPtr<IInspectable> m_handOffVisual;
    wrl::ComPtr<IInspectable> m_prependVisual;
    wrl::ComPtr<IInspectable> m_wucInsetClip;

    wfn::Vector2 m_size;
    wfn::Vector3 m_offset;
    wfn::Vector3 m_translation;
    wfn::Vector3 m_centerPoint;
    wfn::Vector2 m_anchorPoint;
    XRECTF_RB m_clipInsets;
};


