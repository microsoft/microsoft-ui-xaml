// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Private { namespace Infrastructure {

    class VisualTreeVerifierStatics
        : public wrl::ActivationFactory<test_infra::IVisualTreeVerifierStatics>
    {
    public:
        IFACEMETHOD(CreateFromElement)(_In_ xaml::IUIElement* element, _Outptr_ test_infra::IVisualTreeVerifier** value);

        IFACEMETHOD(CreateFromVisual)(_In_ ixp::IVisual* visual, _Outptr_ test_infra::IVisualTreeVerifier** value);
    };

    // MockDComp replacement
    class VisualTreeVerifier
        : public wrl::RuntimeClass<test_infra::IVisualTreeVerifier>
    {
        InspectableClass(RuntimeClass_Private_Infrastructure_VisualTreeVerifier, TrustLevel::BaseTrust);

    public:
        HRESULT RuntimeClassInitialize(_In_ ixp::IVisual* visual, _In_ msy::IDispatcherQueue* dispatcherQueue);

        IFACEMETHOD(WalkToTaggedChild)(UINT tag, _Outptr_ IVisualTreeVerifier** value) override;
        IFACEMETHOD(WalkThroughSimpleCompNode)(_Outptr_ IVisualTreeVerifier** value) override;
        IFACEMETHOD(WalkToChildAtIndex)(UINT childIndex, UINT expectedChildCount, _Outptr_ IVisualTreeVerifier** value) override;

        IFACEMETHOD(VerifyNoTaggedChild)(UINT tag) override;
        IFACEMETHOD(VerifyChildCount)(UINT childCount) override;

        IFACEMETHOD(VerifyVisualOffset)(const float x, const float y) override;
        IFACEMETHOD(VerifyVisualSize)(const float width, const float height) override;
        IFACEMETHOD(VerifyTransformMatrix_Scale)(const float x, const float y) override;
        IFACEMETHOD(VerifyRoundedCornerClip)(const float left, const float top, const float right, const float bottom, const float cornerRadius) override;
        IFACEMETHOD(VerifyRoundedCornerClipCorners)(const float topLeft, const float topRight, const float bottomRight, const float bottomLeft) override;

        IFACEMETHOD(GetVisualOffset)(_Out_ wfn::Vector3* offset) override;

    private:
        ~VisualTreeVerifier();
        void VerifyIsUIThread();
        bool WalkToFirstTaggedChildHelper(UINT tag);
        wrl::ComPtr<wfc::IIterator<ixp::Visual*>> GetChildVisualIterator();
        bool DoesVisualHaveTag(const wrl::ComPtr<ixp::IVisual>& visual, UINT tag);
        bool AreEqualWithinEpsilon(const float expected, const float actual);

        wrl::ComPtr<ixp::IVisual> m_visual;
        wrl::ComPtr<msy::IDispatcherQueue> m_dispatcherQueue;
    };

} }
