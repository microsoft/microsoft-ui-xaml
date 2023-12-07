// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DCompPropertyChangedListener.h"
#include <UIElement.h>

using namespace Microsoft::WRL;

DCompPropertyChangedListener::DCompPropertyChangedListener(_In_ CUIElement* uiElement)
    : m_size({0.0f, 0.0f})
    , m_offset({0.0f, 0.0f, 0.0f})
    , m_translation({0.0f, 0.0f, 0.0f})
    , m_centerPoint({0.0f, 0.0f, 0.0f})
    , m_anchorPoint({0.0f, 0.0f})
    , m_clipInsets({0,0,0,0})
{
    m_uiElementWeakRef = xref::get_weakref(uiElement);
}

DCompPropertyChangedListener::~DCompPropertyChangedListener()
{
    DetachFromHandOffVisual();
    DetachFromWUCClip();
}

void DCompPropertyChangedListener::AttachToHandOffVisual(_In_ IInspectable* handOffVisual)
{
    if (handOffVisual != m_handOffVisual.Get())
    {
        // We don't expect any scenarios where the visual on the element changes.
        ASSERT(m_handOffVisual == nullptr);

        wrl::ComPtr<ixp::IExpCompositionPropertyChanged> visual;
        IFCFAILFAST(handOffVisual->QueryInterface(IID_PPV_ARGS(&visual)));

        IFCFAILFAST(visual->SetPropertyChangedListener(ixp::ExpExpressionNotificationProperty_Size, this));
        IFCFAILFAST(visual->SetPropertyChangedListener(ixp::ExpExpressionNotificationProperty_Offset, this));
        IFCFAILFAST(visual->SetPropertyChangedListener(ixp::ExpExpressionNotificationProperty_Scale, this));
        IFCFAILFAST(visual->SetPropertyChangedListener(ixp::ExpExpressionNotificationProperty_RotationAngle, this));
        IFCFAILFAST(visual->SetPropertyChangedListener(ixp::ExpExpressionNotificationProperty_CenterPoint, this));
        IFCFAILFAST(visual->SetPropertyChangedListener(ixp::ExpExpressionNotificationProperty_AnchorPoint, this));
        IFCFAILFAST(visual->SetPropertyChangedListener(ixp::ExpExpressionNotificationProperty_TransformMatrix, this));
        IFCFAILFAST(visual->SetPropertyChangedListener(ixp::ExpExpressionNotificationProperty_Clip, this));

        m_handOffVisual = handOffVisual;
    }
}

void DCompPropertyChangedListener::AttachToPrependVisual(_In_ IInspectable* prependVisual)
{
    // The Translation property, which allows the app to translate the HandOff visual, needs to be
    // incorporated into hit-testing.  This property is handled purely in XAML, by setting it on the
    // Prepend visual (see more details in HWCompTreeNodeWinRT::UpdatePrependTransform()).
    // When the Translate property is in use, we listen to the Prepend visual's Offset property,
    // which is used exclusively for carrying the Translate property.  We simply add this to Offset
    // when computing the hit-testing transform (see UpdateOverallOffset()).
    if (prependVisual != m_prependVisual.Get())
    {
        ASSERT(m_prependVisual == nullptr);

        wrl::ComPtr<ixp::IExpCompositionPropertyChanged> visual;
        IFCFAILFAST(prependVisual->QueryInterface(IID_PPV_ARGS(&visual)));
        IFCFAILFAST(visual->SetPropertyChangedListener(ixp::ExpExpressionNotificationProperty_Offset, this));

        m_prependVisual = prependVisual;
    }
}

void DCompPropertyChangedListener::DetachFromPrependVisual()
{
    if (m_prependVisual)
    {
        wrl::ComPtr<ixp::IExpCompositionPropertyChanged> visual;
        VERIFYHR(m_prependVisual->QueryInterface(IID_PPV_ARGS(&visual)));
        VERIFYHR(visual->SetPropertyChangedListener(ixp::ExpExpressionNotificationProperty_Offset, nullptr));

        m_prependVisual = nullptr;
        m_translation = {0.0f, 0.0f, 0.0f};
        const auto& uiElement = m_uiElementWeakRef.lock();
        if (uiElement != nullptr)
        {
            IFCFAILFAST(UpdateOverallOffset(uiElement));
        }
    }
}

void DCompPropertyChangedListener::AttachToWUCClip(_In_ IInspectable* wucInsetClip)
{
    if (wucInsetClip != m_wucInsetClip.Get())
    {
        // We don't expect any scenarios where the clip on the visual changes. It could only go from null (no clip when the app
        // requested a hand off visual) to non-null (when the app sets a Xaml clip afterwards).
        ASSERT(m_wucInsetClip == nullptr);

        wrl::ComPtr<ixp::IExpCompositionPropertyChanged> insetClip;
        IFCFAILFAST(wucInsetClip->QueryInterface(IID_PPV_ARGS(&insetClip)));

        // Note: There's also anchor/center/offset/rotation/scale on the base clip which Xaml should also care about.

        IFCFAILFAST(insetClip->SetPropertyChangedListener(ixp::ExpExpressionNotificationProperty_TransformMatrix, this));
        IFCFAILFAST(insetClip->SetPropertyChangedListener(ixp::ExpExpressionNotificationProperty_LeftInset, this));
        IFCFAILFAST(insetClip->SetPropertyChangedListener(ixp::ExpExpressionNotificationProperty_TopInset, this));
        IFCFAILFAST(insetClip->SetPropertyChangedListener(ixp::ExpExpressionNotificationProperty_RightInset, this));
        IFCFAILFAST(insetClip->SetPropertyChangedListener(ixp::ExpExpressionNotificationProperty_BottomInset, this));

        m_wucInsetClip = wucInsetClip;
    }
}

void DCompPropertyChangedListener::DetachFromHandOffVisual()
{
    m_uiElementWeakRef.reset();

    if (m_handOffVisual)
    {
        wrl::ComPtr<ixp::IExpCompositionPropertyChanged> visual;
        VERIFYHR(m_handOffVisual->QueryInterface(IID_PPV_ARGS(&visual)));

        VERIFYHR(visual->SetPropertyChangedListener(ixp::ExpExpressionNotificationProperty_Size, nullptr));
        VERIFYHR(visual->SetPropertyChangedListener(ixp::ExpExpressionNotificationProperty_Offset, nullptr));
        VERIFYHR(visual->SetPropertyChangedListener(ixp::ExpExpressionNotificationProperty_Scale, nullptr));
        VERIFYHR(visual->SetPropertyChangedListener(ixp::ExpExpressionNotificationProperty_RotationAngle, nullptr));
        VERIFYHR(visual->SetPropertyChangedListener(ixp::ExpExpressionNotificationProperty_CenterPoint, nullptr));
        VERIFYHR(visual->SetPropertyChangedListener(ixp::ExpExpressionNotificationProperty_AnchorPoint, nullptr));
        VERIFYHR(visual->SetPropertyChangedListener(ixp::ExpExpressionNotificationProperty_TransformMatrix, nullptr));
        IFCFAILFAST(visual->SetPropertyChangedListener(ixp::ExpExpressionNotificationProperty_Clip, nullptr));

        m_handOffVisual = nullptr;
    }
}

void DCompPropertyChangedListener::DetachFromWUCClip()
{
    if (m_wucInsetClip)
    {
        wrl::ComPtr<ixp::IExpCompositionPropertyChanged> insetClip;
        VERIFYHR(m_wucInsetClip->QueryInterface(IID_PPV_ARGS(&insetClip)));

        // Note: There's also anchor/center/offset/rotation/scale on the base clip which Xaml should also care about.

        VERIFYHR(insetClip->SetPropertyChangedListener(ixp::ExpExpressionNotificationProperty_TransformMatrix, nullptr));
        VERIFYHR(insetClip->SetPropertyChangedListener(ixp::ExpExpressionNotificationProperty_LeftInset, nullptr));
        VERIFYHR(insetClip->SetPropertyChangedListener(ixp::ExpExpressionNotificationProperty_TopInset, nullptr));
        VERIFYHR(insetClip->SetPropertyChangedListener(ixp::ExpExpressionNotificationProperty_RightInset, nullptr));
        VERIFYHR(insetClip->SetPropertyChangedListener(ixp::ExpExpressionNotificationProperty_BottomInset, nullptr));

        m_wucInsetClip = nullptr;
    }
}

IFACEMETHODIMP DCompPropertyChangedListener::NotifySinglePropertyChanged(
    _In_ ixp::ICompositionObject* target,
    _In_ ixp::ExpExpressionNotificationProperty propertyId,
    _In_ float value)
{
    const auto& uiElement = m_uiElementWeakRef.lock();
    if (uiElement != nullptr)
    {
        wrl::ComPtr<IInspectable> targetInspectable;
        IFCFAILFAST(target->QueryInterface(IID_PPV_ARGS(targetInspectable.ReleaseAndGetAddressOf())));

        if (targetInspectable.Get() == m_handOffVisual.Get())
        {
            switch (propertyId)
            {
                case ixp::ExpExpressionNotificationProperty_RotationAngle:
                    // RotationAngle is provided in radians, but CRotateTransform uses degrees
                    IFC_RETURN(uiElement->SetHandOffVisualRotationAngle(180.0f / float(M_PI) * value));
                    break;
            }
        }
        else if (targetInspectable.Get() == m_wucInsetClip.Get())
        {
            switch (propertyId)
            {
                case ixp::ExpExpressionNotificationProperty_LeftInset:
                    m_clipInsets.left = value;
                    break;

                case ixp::ExpExpressionNotificationProperty_TopInset:
                    m_clipInsets.top = value;
                    break;

                case ixp::ExpExpressionNotificationProperty_RightInset:
                    m_clipInsets.right = value;
                    break;

                case ixp::ExpExpressionNotificationProperty_BottomInset:
                    m_clipInsets.bottom = value;
                    break;
            }

            UpdateClip(uiElement);
        }
    }

    return S_OK;
}

IFACEMETHODIMP DCompPropertyChangedListener::NotifyVector2PropertyChanged(
    _In_ ixp::ICompositionObject* pTarget,
    _In_ ixp::ExpExpressionNotificationProperty propertyId,
    _In_ wfn::Vector2 value)
{
    const auto& uiElement = m_uiElementWeakRef.lock();
    if (uiElement != nullptr)
    {
        switch (propertyId)
        {
        case ixp::ExpExpressionNotificationProperty_Size:
            // Size affects both the overall offset and center-point due to AnchorPoint.
            m_size = value;
            IFC_RETURN(UpdateOverallOffset(uiElement));
            IFC_RETURN(UpdateOverallCenterPoint(uiElement));
            UpdateClip(uiElement);
            break;

        case ixp::ExpExpressionNotificationProperty_AnchorPoint:
            // AnchorPoint affects both the overall offset and center-point.
            m_anchorPoint = value;
            IFC_RETURN(UpdateOverallOffset(uiElement));
            IFC_RETURN(UpdateOverallCenterPoint(uiElement));
            break;
        }
    }

    return S_OK;
}

IFACEMETHODIMP DCompPropertyChangedListener::NotifyVector3PropertyChanged(
    _In_ ixp::ICompositionObject* pTarget,
    _In_ ixp::ExpExpressionNotificationProperty propertyId,
    _In_ wfn::Vector3 value)
{
    const auto& uiElement = m_uiElementWeakRef.lock();
    if (uiElement != nullptr)
    {
        switch (propertyId)
        {
            case ixp::ExpExpressionNotificationProperty_Offset:
            {
                wrl::ComPtr<IInspectable> targetInspectable;
                IFCFAILFAST(pTarget->QueryInterface(IID_PPV_ARGS(targetInspectable.ReleaseAndGetAddressOf())));

                if (targetInspectable.Get() == m_handOffVisual.Get())
                {
                    m_offset = value;
                }
                else if (targetInspectable.Get() == m_prependVisual.Get())
                {
                    m_translation = value;
                }
                else
                {
                    ASSERT(FALSE);
                }

                // The overall offset is a function of AnchorPoint.
                IFC_RETURN(UpdateOverallOffset(uiElement));
                break;
            }

            case ixp::ExpExpressionNotificationProperty_Scale:
                IFC_RETURN(uiElement->SetHandOffVisualScale(value.X, value.Y));
                break;

            case ixp::ExpExpressionNotificationProperty_CenterPoint:
                // The overall center-point is a function of AnchorPoint.
                m_centerPoint = value;
                IFC_RETURN(UpdateOverallCenterPoint(uiElement));
                break;
        }
    }

    return S_OK;
}

IFACEMETHODIMP DCompPropertyChangedListener::NotifyVector4PropertyChanged(
    _In_ ixp::ICompositionObject* pTarget,
    _In_ ixp::ExpExpressionNotificationProperty propertyId,
    _In_ wfn::Vector4 value)
{
    return S_OK;
}

IFACEMETHODIMP DCompPropertyChangedListener::NotifyColorPropertyChanged(
    _In_ ixp::ICompositionObject* pTarget,
    _In_ ixp::ExpExpressionNotificationProperty propertyId,
    _In_ wu::Color value)
{
    return S_OK;
}

IFACEMETHODIMP DCompPropertyChangedListener::NotifyMatrix3x2PropertyChanged(
    _In_ ixp::ICompositionObject* target,
    _In_ ixp::ExpExpressionNotificationProperty propertyId,
    _In_ wfn::Matrix3x2 value)
{
    const auto& uiElement = m_uiElementWeakRef.lock();
    if (uiElement != nullptr)
    {
        wrl::ComPtr<IInspectable> targetInspectable;
        IFCFAILFAST(target->QueryInterface(IID_PPV_ARGS(targetInspectable.ReleaseAndGetAddressOf())));
        ASSERT(targetInspectable.Get() == m_wucInsetClip.Get());

        switch (propertyId)
        {
            case ixp::ExpExpressionNotificationProperty_TransformMatrix:
                uiElement->SetHandOffVisualClipTransform(value);
                break;
        }
    }

    return S_OK;
}

IFACEMETHODIMP DCompPropertyChangedListener::NotifyMatrix4x4PropertyChanged(
    _In_ ixp::ICompositionObject *pTarget,
    _In_ ixp::ExpExpressionNotificationProperty propertyId,
    _In_ wfn::Matrix4x4 value)
{
    const auto& uiElement = m_uiElementWeakRef.lock();
    if (uiElement != nullptr)
    {
        switch (propertyId)
        {
            case ixp::ExpExpressionNotificationProperty_TransformMatrix:
                uiElement->SetHandOffVisualTransformMatrix(value);
                break;
        }
    }

    return S_OK;
}

IFACEMETHODIMP DCompPropertyChangedListener::NotifyBooleanPropertyChanged(
    _In_ ixp::ICompositionObject *pTarget,
    _In_ ixp::ExpExpressionNotificationProperty propertyId,
    _In_ boolean value)
{
    return S_OK;
}

IFACEMETHODIMP DCompPropertyChangedListener::NotifyReferencePropertyChanged(
    _In_ ixp::ICompositionObject* pTarget,
    _In_ ixp::ExpExpressionNotificationProperty propertyId)
{
    const auto& uiElement = m_uiElementWeakRef.lock();
    if (uiElement != nullptr)
    {
        switch (propertyId)
        {
            case ixp::ExpExpressionNotificationProperty_Clip:
                OnVisualClipChanged(pTarget, uiElement);
                break;
        }
    }

    return S_OK;
}

void DCompPropertyChangedListener::OnVisualClipChanged(_In_ ixp::ICompositionObject* visual, _In_ CUIElement* uiElement)
{
    DetachFromWUCClip();

    // Get the new clip from the visual, attach the listener to it if it exists.
    const auto& wucInsetClip = GetInsetClipFromVisual(visual);
    if (wucInsetClip != nullptr)
    {
        wrl::ComPtr<IInspectable> wucInsetClipAsIInspectable;
        IFCFAILFAST(wucInsetClip.As(&wucInsetClipAsIInspectable));
        AttachToWUCClip(wucInsetClipAsIInspectable.Get());

        // Also read off the current values of the new clip. Xaml won't get change notifications for the initial values.
        // Note: If any of these are animated, we won't have the accurate value. In that case we're relying on a property
        // update after a (hopefully soon) future tick of the animation. If the animation or expression isn't changing the
        // value then we'll have the wrong clip for a while.
        IFCFAILFAST(wucInsetClip->get_LeftInset(&m_clipInsets.left));
        IFCFAILFAST(wucInsetClip->get_TopInset(&m_clipInsets.top));
        IFCFAILFAST(wucInsetClip->get_RightInset(&m_clipInsets.right));
        IFCFAILFAST(wucInsetClip->get_BottomInset(&m_clipInsets.bottom));
    }
    else
    {
        m_wucInsetClip = nullptr;   // AttachToWUCClip updates this already, if the new clip isn't null.
    }

    // We lost the old WUC clip, along with its previous transforms. Clear out the cached Xaml clip as well to reset
    // the clip transform. If there's a non-default (i.e. non-identity) transform on the new WUC clip, we'll be told
    // about it.
    uiElement->ClearHandOffVisualClip();

    UpdateClip(uiElement);
}

wrl::ComPtr<WUComp::IInsetClip> DCompPropertyChangedListener::GetInsetClipFromVisual(_In_ ixp::ICompositionObject* visual)
{
    // Get the new clip from the visual, attach the listener to it if it exists.
    wrl::ComPtr<WUComp::IVisual> wucVisual;
    IFCFAILFAST(visual->QueryInterface(IID_PPV_ARGS(&wucVisual)));

    wrl::ComPtr<WUComp::ICompositionClip> wucClip;
    IFCFAILFAST(wucVisual->get_Clip(&wucClip));

    wrl::ComPtr<WUComp::IInsetClip> wucInsetClip;
    if (wucClip)
    {
        wucClip.As(&wucInsetClip);  // Allowed to fail - we won't attach a listener to it if it's not an inset clip.
    }

    return wucInsetClip;
}

void DCompPropertyChangedListener::ForceUpdateFromLayoutOffset()
{
    // Forcefully push the current layout offset from our UIElement into the Transform.
    const auto& uiElement = m_uiElementWeakRef.lock();
    if (uiElement != nullptr)
    {
        m_offset.X = uiElement->GetActualOffsetX();
        m_offset.Y = uiElement->GetActualOffsetY();
        m_offset.Z = 0.0f;
        IFCFAILFAST(UpdateOverallOffset(uiElement));
    }
}

HRESULT DCompPropertyChangedListener::UpdateOverallOffset(_In_ CUIElement* uielement)
{
    // Replicate the math DComp does to incorporate AnchorPoint into the overall offset.
    // From the API spec:
    // The AnchorPoint property on a visual defines the point on the visual to be positioned at the visual�s offset.
    // It is expressed as a normalized value with respect to the visual�s size, with the AnchorPoint (0, 0) referring to
    // the visual�s top-left corner and (1, 1) referring to the visual�s bottom right corner.
    float overallOffsetX = m_offset.X + m_translation.X - (m_size.X * m_anchorPoint.X);
    float overallOffsetY = m_offset.Y + m_translation.Y - (m_size.Y * m_anchorPoint.Y);
    IFC_RETURN(uielement->SetHandOffVisualOffset(overallOffsetX, overallOffsetY));

    return S_OK;
}

HRESULT DCompPropertyChangedListener::UpdateOverallCenterPoint(_In_ CUIElement* uielement)
{
    // Replicate the math DComp does to incorporate AnchorPoint into the overall center-point.
    // From the API spec:
    // Additionally, with the inclusion of AnchorPoint, the existing CenterPoint property,
    // which serves as the center of scale and rotate transforms, can now be defined as an offset from the AnchorPoint
    // rather than as an offset from the (0, 0, 0) point (i.e. top left corner of the visual).
    float overallCenterX = m_centerPoint.X + (m_size.X * m_anchorPoint.X);
    float overallCenterY = m_centerPoint.Y + (m_size.Y * m_anchorPoint.Y);
    IFC_RETURN(uielement->SetHandOffVisualCenterPoint(overallCenterX, overallCenterY));

    return S_OK;
}

void DCompPropertyChangedListener::UpdateClip(_In_ CUIElement* uiElement)
{
    if (m_wucInsetClip)
    {
        XRECTF clip = {
            // The left and top of the clip are the left and top insets.
            m_clipInsets.left,
            m_clipInsets.top,

            // Positive insets make the clip smaller. Subtract the insets from the clip's size.
            m_size.X - m_clipInsets.left - m_clipInsets.right,
            m_size.Y - m_clipInsets.top - m_clipInsets.bottom};

        uiElement->SetHandOffVisualClip(clip);
    }
    else
    {
        uiElement->ClearHandOffVisualClip();
    }
}
