// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <NamespaceAliases.h>
#include <microsoft.ui.composition.h>

namespace ExpComp
{
    // The enum and interfaces below are exact copies of the same enum/interfaces from the
    // Microsoft::UI::Composition::Experimental namespace. Ideally WinUI wouldn't depend
    // on experimental APIs, but some functionality depends on these APIs and these APIs
    // are not on a path to become stable/non-experimental APIs (because the API shape is
    // very undesirable and doesn't have a clear alternative which is both better and still
    // good for runtime performance).
    //
    // To ensure WinUI can build even on non-experimental builds, copy in these required
    // definitions. DCompPropertyChangedListener.cpp has some static_assert checks to catch
    // if the interfaces ever change (which is not anticpated).

    // Copied from "microsoft.ui.composition.experimental.h":

    enum ExpExpressionNotificationProperty : int
    {
        ExpExpressionNotificationProperty_Undefined = 0,
        ExpExpressionNotificationProperty_Clip = 1,
        ExpExpressionNotificationProperty_Offset = 2,
        ExpExpressionNotificationProperty_Opacity = 3,
        ExpExpressionNotificationProperty_Size = 4,
        ExpExpressionNotificationProperty_RelativeOffset = 5,
        ExpExpressionNotificationProperty_RelativeSize = 6,
        ExpExpressionNotificationProperty_AnchorPoint = 7,
        ExpExpressionNotificationProperty_CenterPoint = 8,
        ExpExpressionNotificationProperty_Orientation = 9,
        ExpExpressionNotificationProperty_RotationAngle = 10,
        ExpExpressionNotificationProperty_RotationAxis = 11,
        ExpExpressionNotificationProperty_Scale = 12,
        ExpExpressionNotificationProperty_TransformMatrix = 13,
        ExpExpressionNotificationProperty_BottomInset = 14,
        ExpExpressionNotificationProperty_LeftInset = 15,
        ExpExpressionNotificationProperty_RightInset = 16,
        ExpExpressionNotificationProperty_TopInset = 17,
        ExpExpressionNotificationProperty_LeftRadiusX = 18,
        ExpExpressionNotificationProperty_LeftRadiusY = 19,
        ExpExpressionNotificationProperty_BottomRightRadiusX = 20,
        ExpExpressionNotificationProperty_BottomRightRadiusY = 21,
        ExpExpressionNotificationProperty_TopLeftRadiusX = 22,
        ExpExpressionNotificationProperty_TopLeftRadiusY = 23,
        ExpExpressionNotificationProperty_TopRightRadiusX = 24,
        ExpExpressionNotificationProperty_TopRightRadiusY = 25,
    };

    MIDL_INTERFACE("5f9c3d96-1e77-5980-8b28-7a9b8614a863")
    IExpCompositionPropertyChangedListener : public IInspectable
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE NotifyBooleanPropertyChanged(
            ABI::Microsoft::UI::Composition::ICompositionObject* target,
            ExpComp::ExpExpressionNotificationProperty property,
            boolean value
            ) = 0;
        virtual HRESULT STDMETHODCALLTYPE NotifyColorPropertyChanged(
            ABI::Microsoft::UI::Composition::ICompositionObject* target,
            ExpComp::ExpExpressionNotificationProperty property,
            ABI::Windows::UI::Color value
            ) = 0;
        virtual HRESULT STDMETHODCALLTYPE NotifyMatrix3x2PropertyChanged(
            ABI::Microsoft::UI::Composition::ICompositionObject* target,
            ExpComp::ExpExpressionNotificationProperty property,
            ABI::Windows::Foundation::Numerics::Matrix3x2 value
            ) = 0;
        virtual HRESULT STDMETHODCALLTYPE NotifyMatrix4x4PropertyChanged(
            ABI::Microsoft::UI::Composition::ICompositionObject* target,
            ExpComp::ExpExpressionNotificationProperty property,
            ABI::Windows::Foundation::Numerics::Matrix4x4 value
            ) = 0;
        virtual HRESULT STDMETHODCALLTYPE NotifyReferencePropertyChanged(
            ABI::Microsoft::UI::Composition::ICompositionObject* target,
            ExpComp::ExpExpressionNotificationProperty property
            ) = 0;
        virtual HRESULT STDMETHODCALLTYPE NotifySinglePropertyChanged(
            ABI::Microsoft::UI::Composition::ICompositionObject* target,
            ExpComp::ExpExpressionNotificationProperty property,
            FLOAT value
            ) = 0;
        virtual HRESULT STDMETHODCALLTYPE NotifyVector2PropertyChanged(
            ABI::Microsoft::UI::Composition::ICompositionObject* target,
            ExpComp::ExpExpressionNotificationProperty property,
            ABI::Windows::Foundation::Numerics::Vector2 value
            ) = 0;
        virtual HRESULT STDMETHODCALLTYPE NotifyVector3PropertyChanged(
            ABI::Microsoft::UI::Composition::ICompositionObject* target,
            ExpComp::ExpExpressionNotificationProperty property,
            ABI::Windows::Foundation::Numerics::Vector3 value
            ) = 0;
        virtual HRESULT STDMETHODCALLTYPE NotifyVector4PropertyChanged(
            ABI::Microsoft::UI::Composition::ICompositionObject* target,
            ExpComp::ExpExpressionNotificationProperty property,
            ABI::Windows::Foundation::Numerics::Vector4 value
            ) = 0;
    };

    MIDL_INTERFACE("12b579a9-6a27-5cde-a2a1-c557bb7dfdb3")
    IExpCompositionPropertyChanged : public IInspectable
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetPropertyChangedListener(
            ExpComp::ExpExpressionNotificationProperty property,
            ExpComp::IExpCompositionPropertyChangedListener* listener
            ) = 0;
    };
}
