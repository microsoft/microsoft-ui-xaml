// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "LinearGradientBrush.h"
#include "ColorUtil.h"
#include <DependencyObjectDCompRegistry.h>
#include <Transform.h>
#include <BrushTypeUtils.h>
#include <windows.ui.composition.interop.h>
#include <corep.h>
#include <dcomptreehost.h>

void CGradientStop::AddWUCStop(_In_ WUComp::ICompositor4* compositor, _In_ wfc::IVector<WUComp::CompositionColorGradientStop*>* stops)
{
    wrl::ComPtr<WUComp::ICompositionColorGradientStop> stop;
    IFCFAILFAST(compositor->CreateColorGradientStopWithOffsetAndColor(m_stop.rPosition, ColorUtils::GetWUColor(m_rgb), &stop));

    IFCFAILFAST(stops->Append(stop.Get()));
}

void CGradientStopCollection::AddWUCStops(_In_ WUComp::ICompositor4* compositor, _In_ WUComp::ICompositionGradientBrush* wucBrush)
{
    wrl::ComPtr<WUComp::ICompositionColorGradientStopCollection> stopCollection;
    IFCFAILFAST(wucBrush->get_ColorStops(stopCollection.ReleaseAndGetAddressOf()));

    wrl::ComPtr<wfc::IVector<WUComp::CompositionColorGradientStop*>> stops;
    IFCFAILFAST(stopCollection.As(&stops));

    stops->Clear();
    for (auto item : *this)
    {
        CGradientStop* stop = static_cast<CGradientStop*>(item);
        stop->AddWUCStop(compositor, stops.Get());
    }
}

CLinearGradientBrush::~CLinearGradientBrush()
{
    ReleaseInterface(m_pRenderingCache);
    SimpleProperty::Property::NotifyDestroyed<CLinearGradientBrush>(this);
}

void CLinearGradientBrush::NWPropagateDirtyFlag(DirtyFlags flag)
{
    ReleaseRenderingCache();
    CGradientBrush::NWPropagateDirtyFlag(flag);
    SetDCompResourceDirty();
}

void CLinearGradientBrush::ReleaseDCompResources()
{
    __super::ReleaseDCompResources();

    m_wucBrush.Reset();
    m_wucBrushMap.ReleaseDCompResources();
    m_isWUCBrushDirty = true;
}

void CLinearGradientBrush::SetDCompResourceDirty()
{
    if (!m_isWUCBrushDirty)
    {
        m_isWUCBrushDirty = true;
    }
}
_Check_return_ HRESULT CLinearGradientBrush::EnsurePrimaryWUCBrush(_In_ WUComp::ICompositor* compositor)
{
    wrl::ComPtr<WUComp::ICompositor4> compositor4;
    IFC_RETURN(compositor->QueryInterface(IID_PPV_ARGS(&compositor4)));
    return EnsurePrimaryWUCBrush(compositor4.Get());
}

_Check_return_ HRESULT CLinearGradientBrush::EnsurePrimaryWUCBrush(_In_ WUComp::ICompositor4* compositor)
{
    // The primary WUC brush is the one that we share (when we can share) and that is used
    // as the backing object for any animations that take place.
    if (m_wucBrush != nullptr) return S_OK;

    wrl::ComPtr<WUComp::ICompositionLinearGradientBrush> linearGradientBrush;
    IFC_RETURN(compositor->CreateLinearGradientBrush(linearGradientBrush.ReleaseAndGetAddressOf()));
    IFC_RETURN(linearGradientBrush.As(&m_wucBrush));

    if (GetDCompObjectRegistry() != nullptr)
    {
        GetDCompObjectRegistry()->EnsureObjectWithDCompResourceRegistered(this);
    }
    return S_OK;
}

wrl::ComPtr<WUComp::ICompositionBrush> CLinearGradientBrush::GetWUCBrush(
    _In_ const XRECTF& brushBounds,
    _In_opt_ const CMILMatrix* brushTransform,
    _In_ WUComp::ICompositor4* compositor)
{
    if (brushTransform == nullptr)
    {
        if (CanUseSingleWUCBrush())
        {
            m_wucBrushMap.ReleaseDCompResources();

            if (m_isWUCBrushDirty || m_wucBrush == nullptr)
            {
                IFCFAILFAST(EnsurePrimaryWUCBrush(compositor));

                ApplyWUCBrushProperties(compositor, m_wucBrush.Get(), brushBounds, brushTransform);

                m_isWUCBrushDirty = false;
            }

            return m_wucBrush;
        }
        else
        {
            wrl::ComPtr<WUComp::ICompositionBrush> brush = m_wucBrushMap.GetBrush(brushBounds, compositor);

            if (GetDCompObjectRegistry() != nullptr)
            {
                GetDCompObjectRegistry()->EnsureObjectWithDCompResourceRegistered(this);
            }

            ApplyWUCBrushProperties(compositor, brush.Get(), brushBounds, brushTransform);

            return brush;
        }
    }
    else
    {
        //
        // WUC's linear gradient brush will stretch to fit the visual that references it. In nearly all Xaml scenarios, that's
        // the behavior that we want, so we let WUC handle the stretching. The one exception is RichTextBlock, which uses different
        // bounds for the gradient and for the text.
        //
        // We have a map of WUC brushes that correspond to this Xaml brush, but that map is meant to handle other scenarios. WUC's
        // linear gradient brush works in relative coordinates, so if we have an absolute mapping mode we'll need to transform it
        // to relative coordinates based on the (different) sizes of the elements that share this brush. Similarly, WUC's linear
        // gradient transforms use absolute coordinates, which means if we have a relative transform, then we'll need to convert it
        // to absolute coordinates using the (different) sizes of the elements that share this brush. In both these cases we can expect
        // the elements using this element to stay the same size, and for this brush to be shared between multiple elements with the
        // same size (e.g. in an item template), so we cache the brush.
        //
        // The extra stretch transform depends on both the size of the brush bounds and the size of the text inside the RichTextBlock.
        // It's less likely that we'll have multiple blocks of text with the same size. It's also not likely that a block of text will
        // stay the same size after any text property changes (e.g. font, font size, or the text itself). So there's less of a chance
        // that the WUC linear gradient brush can ever be reused. Given that, we'll make a new brush each time and not cache any of
        // them.
        //

        wrl::ComPtr<WUComp::ICompositionLinearGradientBrush> linearGradientBrush;
        wrl::ComPtr<WUComp::ICompositionBrush> brush;
        IFCFAILFAST(compositor->CreateLinearGradientBrush(linearGradientBrush.ReleaseAndGetAddressOf()));
        IFCFAILFAST(linearGradientBrush.As(&brush));
        ApplyWUCBrushProperties(compositor, brush.Get(), brushBounds, brushTransform);
        return brush;
    }
}

bool CLinearGradientBrush::CanUseSingleWUCBrush() const
{
        // WUC linear gradient brushes have relative coordinates for their start and end points, which are stretched to the bounds of
        // their visuals. If the Xaml LinearGradientBrush uses absolute start and end points, we'll have to scale them to the visual.
        // Since Xaml brushes can be shared between multiple elements with different sizes, we'll need to create multiple WUC linear
        // gradients if the mapping mode is absolute.
    return m_nMapping == XcpRelative
        // WUC linear gradient brush transforms use absolute coordinates, which corresponds to Transform. RelativeTransform is relative
        // to the bounding box of the element using this LinearGradientBrush. Since this brush can be shared between multiple elements
        // with different sizes, we'll need to create multiple WUC linear gradients if there's a relative transform involved.
        && GetRelativeTransform() == nullptr;
}

void CLinearGradientBrush::ApplyWUCBrushProperties(
    _In_ WUComp::ICompositor4* compositor,
    _In_ WUComp::ICompositionBrush* brush,
    _In_ const XRECTF& brushBounds,
    _In_ const CMILMatrix* brushTransform)
{
    wrl::ComPtr<WUComp::ICompositionLinearGradientBrush> linearGradientBrush;
    wrl::ComPtr<WUComp::ICompositionGradientBrush> gradientBrush;

    IFCFAILFAST(brush->QueryInterface(IID_PPV_ARGS(&linearGradientBrush)));
    IFCFAILFAST(brush->QueryInterface(IID_PPV_ARGS(&gradientBrush)));

    wfn::Vector2 startPoint;
    startPoint.X = m_ptStart.x;
    startPoint.Y = m_ptStart.y;

    wfn::Vector2 endPoint;
    endPoint.X = m_ptEnd.x;
    endPoint.Y = m_ptEnd.y;

    if (m_nMapping == XcpAbsolute)
    {
        if (brushBounds.Width != 0)
        {
            startPoint.X /= brushBounds.Width;
            endPoint.X /= brushBounds.Width;
        }
        if (brushBounds.Height != 0)
        {
            startPoint.Y /= brushBounds.Height;
            endPoint.Y /= brushBounds.Height;
        }
    }

    IFCFAILFAST(linearGradientBrush->put_StartPoint(startPoint));
    IFCFAILFAST(linearGradientBrush->put_EndPoint(endPoint));

    switch (m_nSpread)
    {
    case XcpGradientWrapModeExtend:
        IFCFAILFAST(gradientBrush->put_ExtendMode(WUComp::CompositionGradientExtendMode_Clamp));
        break;

    case XcpGradientWrapModeFlip:
        IFCFAILFAST(gradientBrush->put_ExtendMode(WUComp::CompositionGradientExtendMode_Mirror));
        break;

    case XcpGradientWrapModeTile:
        IFCFAILFAST(gradientBrush->put_ExtendMode(WUComp::CompositionGradientExtendMode_Wrap));
        break;
    }

    IFCFAILFAST(gradientBrush->put_InterpolationSpace(
        m_nInterpolate == XcpColorInterpolationModeScRgbLinearInterpolation ?
        WUComp::CompositionColorSpace_RgbLinear : WUComp::CompositionColorSpace_Rgb
    ));

    if (m_pStops != nullptr)
    {
        m_pStops->AddWUCStops(compositor, gradientBrush.Get());
    }

    // If we aren't using facades, then just set the transform matrix on the brush as we have done historically.
#if WI_IS_FEATURE_PRESENT(Feature_XamlMotionSystemHoldbacks)
    if (GetObjectStrictness() != ObjectStrictness::StrictOnly)
#endif
    {
        // Neither the transform nor the relative transform is independently animatable. Just get the static 3x2 matrix and set it.
        CMILMatrix relativeTransform(true);
        const auto& rt = GetRelativeTransform();
        if (rt != nullptr)
        {
            rt->GetTransform(&relativeTransform);
        }

        CMILMatrix transform(true);
        const auto& t = GetTransform();
        if (t != nullptr)
        {
            t->GetTransform(&transform);
        }

        CMILMatrix transformMatrix(true);
        if (relativeTransform.IsIdentity())
        {
            transformMatrix = transform;
        }
        else
        {
            CBrushTypeUtils::GetBrushTransform(
                &relativeTransform,
                &transform,
                &brushBounds,
                &transformMatrix);
        }

        if (brushTransform != nullptr && !brushTransform->IsIdentity())
        {
            transformMatrix.Prepend(*brushTransform);
        }

        C_ASSERT(sizeof(wfn::Matrix3x2) == sizeof(transformMatrix));
        IFCFAILFAST(gradientBrush->put_TransformMatrix(*reinterpret_cast<wfn::Matrix3x2*>(&transformMatrix)));
        return;
    }

#if WI_IS_FEATURE_PRESENT(Feature_XamlMotionSystemHoldbacks)
    // We are dealing with facade properties which are treated differently depending on whether they are animated
    // or not and whether it is the primary brush.
    //
    //  If we are the primary brush
    //      push all non-animated properties into the brush
    //      ignore animated properties (as the animations would have been set when the animation started)
    //  If we are not the primary brush.
    //      push all non-animated properties into the brush
    //      create an expression animation refering back to the primary brush for all animated properties.
    //
    //  If we have a brush transform, this will only contain scale and offset and it needs to be applied after
    //  all other transform affecting properties (offset, scale, rotation, etc.).  Since DComp doesn't give us an
    //  additional post transform, we will manually push this through to the offset and scale properties.

    bool isPrimaryBrush = brush == m_wucBrush.Get(); // Both are ICompositionBrushes from CompositionLinearGradientBrush so we can compare them.

    if (!GetContext()->GetFacadeStorage().HasAnimation(this, KnownPropertyIndex::LinearGradientBrush_Translation))
    {
        wfn::Vector2 vector = SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_Translation>::Get(this);
        if (brushTransform)
        {
            brushTransform->Transform(reinterpret_cast<XPOINTF*>(&vector), reinterpret_cast<XPOINTF*>(&vector), 1);
        }
        IFCFAILFAST(gradientBrush->put_Offset(vector));
    }
    else if (!isPrimaryBrush)
    {
        IFCFAILFAST(SetExpressionAnimation(compositor, gradientBrush.Get(), FacadeProperty_Translation_Comp, L"Transform(PrimaryBrush.Offset, BrushTransform)", brushTransform));
    }

    if (!GetContext()->GetFacadeStorage().HasAnimation(this, KnownPropertyIndex::LinearGradientBrush_Rotation))
    {
        IFCFAILFAST(gradientBrush->put_RotationAngleInDegrees(static_cast<float>(SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_Rotation>::Get(this))));
    }
    else if (!isPrimaryBrush)
    {
        IFCFAILFAST(SetExpressionAnimation(compositor, gradientBrush.Get(), FacadeProperty_Rotation_Comp, nullptr /* expression */));
    }

    if (!GetContext()->GetFacadeStorage().HasAnimation(this, KnownPropertyIndex::LinearGradientBrush_Scale))
    {
        wfn::Vector2 vector = SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_Scale>::Get(this);
        if (brushTransform)
        {
            vector.X = vector.X * brushTransform->GetM11();
            vector.Y = vector.Y * brushTransform->GetM22();
        }
        IFCFAILFAST(gradientBrush->put_Scale(vector));
    }
    else if (!isPrimaryBrush)
    {
        IFCFAILFAST(SetExpressionAnimation(compositor, gradientBrush.Get(), FacadeProperty_Scale, L"PrimaryBrush.Scale * Vector2(BrushTransform._11, BrushTransform._22)", brushTransform));
    }

    if (!GetContext()->GetFacadeStorage().HasAnimation(this, KnownPropertyIndex::LinearGradientBrush_CenterPoint))
    {
        IFCFAILFAST(gradientBrush->put_CenterPoint(SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_CenterPoint>::Get(this)));
    }
    else if (!isPrimaryBrush)
    {
        IFCFAILFAST(SetExpressionAnimation(compositor, gradientBrush.Get(), FacadeProperty_CenterPoint, nullptr /* expression */));
    }

    if (!GetContext()->GetFacadeStorage().HasAnimation(this, KnownPropertyIndex::LinearGradientBrush_TransformMatrix))
    {
        IFCFAILFAST(gradientBrush->put_TransformMatrix(SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_TransformMatrix>::Get(this)));
    }
    else if (!isPrimaryBrush)
    {
        IFCFAILFAST(SetExpressionAnimation(compositor, gradientBrush.Get(), FacadeProperty_TransformMatrix, nullptr /* expression */));
    }
#endif
}

_Check_return_ HRESULT CLinearGradientBrush::SetExpressionAnimation(_In_ WUComp::ICompositor4* compositor4, WUComp::ICompositionGradientBrush* brush, _In_ LPCWSTR propertyName, _In_opt_ LPCWSTR expressionString, _In_opt_ const CMILMatrix* brushTransform)
{
    ASSERT(m_wucBrush.Get());

    wrl::ComPtr<WUComp::ICompositor> compositor;
    IFC_RETURN(compositor4->QueryInterface(IID_PPV_ARGS(&compositor)));

    wrl_wrappers::HString expression;
    if (expressionString == nullptr)
    {
        IFC_RETURN(expression.Set(wrl_wrappers::HStringReference(L"PrimaryBrush.").Get()));
        IFC_RETURN(expression.Concat(wrl_wrappers::HStringReference(propertyName), expression));
    }
    else
    {
        IFC_RETURN(expression.Set(expressionString));
    }

    // Create a new Expression Animation
    wrl::ComPtr<WUComp::IExpressionAnimation> expressionAnimation;
    IFC_RETURN(compositor->CreateExpressionAnimation(expressionAnimation.GetAddressOf()));

    // Set the expression and the reference to the primary brush
    IFC_RETURN(expressionAnimation->put_Expression(expression.Get()));
    Microsoft::WRL::ComPtr<WUComp::ICompositionAnimation> compositionAnimation;
    IFC_RETURN(expressionAnimation.As(&compositionAnimation));
    Microsoft::WRL::ComPtr<WUComp::ICompositionObject> brushObject;
    IFC_RETURN(brush->QueryInterface(IID_PPV_ARGS(&brushObject)));
    IFC_RETURN(compositionAnimation->SetReferenceParameter(wrl_wrappers::HStringReference(L"PrimaryBrush").Get(), brushObject.Get()));
    if (brushTransform != nullptr)
    {
        IFCFAILFAST(compositionAnimation->SetMatrix3x2Parameter(wrl_wrappers::HStringReference(L"BrushTransform").Get(), *(reinterpret_cast<const wfn::Matrix3x2*>(brushTransform))));
    }
    return brushObject->StartAnimation(wrl_wrappers::HStringReference(propertyName).Get(), compositionAnimation.Get());
}

wfn::Vector2 CLinearGradientBrush::GetTranslation(bool preferAnimatingValue) const
{
    if (preferAnimatingValue && GetContext()->GetFacadeStorage().HasAnimation(this, KnownPropertyIndex::LinearGradientBrush_Translation))
    {
        return SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_AnimatedTranslation>::Get(this);
    }
    return SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_Translation>::Get(this);
}

void CLinearGradientBrush::SetTranslation(const wfn::Vector2& translation)
{
    FacadeAnimationHelper::CancelAnimation(this, KnownPropertyIndex::LinearGradientBrush_Translation);
    SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_Translation>::Set(this, translation);
    CDependencyObject::NWSetRenderDirty(this, DirtyFlags::Render);
}

DOUBLE CLinearGradientBrush::GetRotation(bool preferAnimatingValue) const
{
    if (preferAnimatingValue && GetContext()->GetFacadeStorage().HasAnimation(this, KnownPropertyIndex::LinearGradientBrush_Rotation))
    {
        return SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_AnimatedRotation>::Get(this);
    }
    return SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_Rotation>::Get(this);
}

void CLinearGradientBrush::SetRotation(DOUBLE rotation)
{
    FacadeAnimationHelper::CancelAnimation(this, KnownPropertyIndex::LinearGradientBrush_Rotation);
    SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_Rotation>::Set(this, rotation);
    CDependencyObject::NWSetRenderDirty(this, DirtyFlags::Render);
}

wfn::Vector2 CLinearGradientBrush::GetScale(bool preferAnimatingValue) const
{
    if (preferAnimatingValue && GetContext()->GetFacadeStorage().HasAnimation(this, KnownPropertyIndex::LinearGradientBrush_Scale))
    {
        return SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_AnimatedScale>::Get(this);
    }
    return SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_Scale>::Get(this);
}

void CLinearGradientBrush::SetScale(const wfn::Vector2& scale)
{
    FacadeAnimationHelper::CancelAnimation(this, KnownPropertyIndex::LinearGradientBrush_Scale);
    SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_Scale>::Set(this, scale);
    CDependencyObject::NWSetRenderDirty(this, DirtyFlags::Render);
}

wfn::Matrix3x2 CLinearGradientBrush::GetTransformMatrix(bool preferAnimatingValue) const
{
    if (preferAnimatingValue && GetContext()->GetFacadeStorage().HasAnimation(this, KnownPropertyIndex::LinearGradientBrush_TransformMatrix))
    {
        return SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_AnimatedTransformMatrix>::Get(this);
    }
    return SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_TransformMatrix>::Get(this);
}

void CLinearGradientBrush::SetTransformMatrix(const wfn::Matrix3x2& transformMatrix)
{
    FacadeAnimationHelper::CancelAnimation(this, KnownPropertyIndex::LinearGradientBrush_TransformMatrix);
    SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_TransformMatrix>::Set(this, transformMatrix);
    CDependencyObject::NWSetRenderDirty(this, DirtyFlags::Render);
}

wfn::Vector2 CLinearGradientBrush::GetCenterPoint(bool preferAnimatingValue) const
{
    if (preferAnimatingValue && GetContext()->GetFacadeStorage().HasAnimation(this, KnownPropertyIndex::LinearGradientBrush_CenterPoint))
    {
        return SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_AnimatedCenterPoint>::Get(this);
    }
    return SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_CenterPoint>::Get(this);
}

void CLinearGradientBrush::SetCenterPoint(const wfn::Vector2& centerPoint)
{
    FacadeAnimationHelper::CancelAnimation(this, KnownPropertyIndex::LinearGradientBrush_CenterPoint);
    SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_CenterPoint>::Set(this, centerPoint);
    CDependencyObject::NWSetRenderDirty(this, DirtyFlags::Render);
}

wfn::Vector2 CLinearGradientBrush::GetAnimatedTranslation() const
{
    return SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_AnimatedTranslation>::Get(this);
}

void CLinearGradientBrush::SetAnimatedTranslation(const wfn::Vector2& translation)
{
    SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_AnimatedTranslation>::Set(this, translation);
}

DOUBLE CLinearGradientBrush::GetAnimatedRotation() const
{
    return SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_AnimatedRotation>::Get(this);
}

void CLinearGradientBrush::SetAnimatedRotation(DOUBLE rotation)
{
    SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_AnimatedRotation>::Set(this, rotation);
}

wfn::Vector2 CLinearGradientBrush::GetAnimatedScale() const
{
    return SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_AnimatedScale>::Get(this);
}

void CLinearGradientBrush::SetAnimatedScale(const wfn::Vector2& scale)
{
    SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_AnimatedScale>::Set(this, scale);
}

wfn::Matrix3x2 CLinearGradientBrush::GetAnimatedTransformMatrix() const
{
    return SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_AnimatedTransformMatrix>::Get(this);
}

void CLinearGradientBrush::SetAnimatedTransformMatrix(const wfn::Matrix3x2& transformMatrix)
{
    SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_AnimatedTransformMatrix>::Set(this, transformMatrix);
}

wfn::Vector2 CLinearGradientBrush::GetAnimatedCenterPoint() const
{
    return SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_AnimatedCenterPoint>::Get(this);
}

void CLinearGradientBrush::SetAnimatedCenterPoint(const wfn::Vector2& centerPoint)
{
    SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_AnimatedCenterPoint>::Set(this, centerPoint);
}

_Check_return_ HRESULT CLinearGradientBrush::StartAnimation(_In_ WUComp::ICompositionAnimationBase* animation)
{
    IFC_RETURN(FacadeAnimationHelper::StartAnimation(this, animation, false /* isImplicitAnimation */));
    CDependencyObject::NWSetRenderDirty(this, DirtyFlags::Render);
    return S_OK;
}

_Check_return_ HRESULT CLinearGradientBrush::StopAnimation(_In_ WUComp::ICompositionAnimationBase* animation)
{
    return FacadeAnimationHelper::StopAnimation(this, animation);
}

_Check_return_ HRESULT CLinearGradientBrush::PullFacadePropertyValueFromCompositionObject(_In_ WUComp::ICompositionObject* backingCO, KnownPropertyIndex facadeID)
{
    wrl::ComPtr<WUComp::ICompositionGradientBrush> backingPS;
    IFC_RETURN(backingCO->QueryInterface(IID_PPV_ARGS(&backingPS)));

    switch (facadeID)
    {
    case KnownPropertyIndex::LinearGradientBrush_Translation:
        wfn::Vector2 translation;
        IFCFAILFAST(backingPS->get_Offset(&translation));
        SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_Translation>::Set(this, translation);
        CDependencyObject::NWSetRenderDirty(this, DirtyFlags::Render);
        break;
    case KnownPropertyIndex::LinearGradientBrush_Rotation:
        FLOAT rotation;
        IFCFAILFAST(backingPS->get_RotationAngleInDegrees(&rotation));
        SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_Rotation>::Set(this, static_cast<double>(rotation));
        CDependencyObject::NWSetRenderDirty(this, DirtyFlags::Render);
        break;
    case KnownPropertyIndex::LinearGradientBrush_Scale:
        wfn::Vector2 scale;
        IFCFAILFAST(backingPS->get_Scale(&scale));
        SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_Scale>::Set(this, scale);
        CDependencyObject::NWSetRenderDirty(this, DirtyFlags::Render);
        break;
    case KnownPropertyIndex::LinearGradientBrush_TransformMatrix:
        wfn::Matrix3x2 transformMatrix;
        IFCFAILFAST(backingPS->get_TransformMatrix(&transformMatrix));
        SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_TransformMatrix>::Set(this, transformMatrix);
        CDependencyObject::NWSetRenderDirty(this, DirtyFlags::Render);
        break;
    case KnownPropertyIndex::LinearGradientBrush_CenterPoint:
        wfn::Vector2 centerPoint;
        IFCFAILFAST(backingPS->get_CenterPoint(&centerPoint));
        SimpleProperty::Property::id<KnownPropertyIndex::LinearGradientBrush_CenterPoint>::Set(this, centerPoint);
        CDependencyObject::NWSetRenderDirty(this, DirtyFlags::Render);
        break;
    }

    return S_OK;
}

void CLinearGradientBrush::FacadeAnimationComplete(KnownPropertyIndex animatedProperty)
{
    // LinearGradientBrush doesn't respond when a WUC animation completes.
}

void CLinearGradientBrush::AllFacadeAnimationsComplete()
{
    m_wucBrush.Reset();
    CDependencyObject::NWSetRenderDirty(this, DirtyFlags::Render);
}

void CLinearGradientBrush::GetFacadeEntries(_Out_ const FacadeMatcherEntry** entries, _Out_ size_t * count)
{
    static const FacadeMatcherEntry facadeEntries[] =
    {
        { FacadeProperty_Translation,     ARRAYSIZE(FacadeProperty_Translation),       KnownPropertyIndex::LinearGradientBrush_Translation,       true,     false, FacadeProperty_Translation_Comp },
        { FacadeProperty_Scale,           ARRAYSIZE(FacadeProperty_Scale),             KnownPropertyIndex::LinearGradientBrush_Scale,             true,     false, },
        { FacadeProperty_TransformMatrix, ARRAYSIZE(FacadeProperty_TransformMatrix),   KnownPropertyIndex::LinearGradientBrush_TransformMatrix,   true,     false, },
        { FacadeProperty_CenterPoint,     ARRAYSIZE(FacadeProperty_CenterPoint),       KnownPropertyIndex::LinearGradientBrush_CenterPoint,       true,     false, },
        { FacadeProperty_Rotation,        ARRAYSIZE(FacadeProperty_Rotation),          KnownPropertyIndex::LinearGradientBrush_Rotation,          false,    false, FacadeProperty_Rotation_Comp },
    };
    *entries = facadeEntries;
    *count = ARRAYSIZE(facadeEntries);
}

void CLinearGradientBrush::CreateBackingCompositionObjectForFacade(_In_ WUComp::ICompositor* compositor, _Out_ WUComp::ICompositionObject** backingCO, _Outptr_result_maybenull_ IFacadePropertyListener** listener)
{
    IFCFAILFAST(EnsurePrimaryWUCBrush(compositor));
    VERIFYHR(m_wucBrush.Get()->QueryInterface(IID_PPV_ARGS(backingCO)));

    // Create a PropertySetListener, which we'll use to listen for animating facade property values that affect hit-testing.
    // First pass this is going to be null since DComp doesn't support listenting to brushes right now anyway.
    // Note: This is still waiting on work from dcomp.
    *listener = nullptr;
}

void CLinearGradientBrush::PopulateBackingCompositionObjectWithFacade(WUComp::ICompositionObject* backingCO, KnownPropertyIndex facadeID)
{
    wrl::ComPtr<WUComp::ICompositionGradientBrush> backingPS;
    VERIFYHR(backingCO->QueryInterface(IID_PPV_ARGS(&backingPS)));

    switch (facadeID)
    {
    case KnownPropertyIndex::LinearGradientBrush_Translation:
        IFCFAILFAST(backingPS->put_Offset(GetTranslation()));
        break;
    case KnownPropertyIndex::LinearGradientBrush_Rotation:
        IFCFAILFAST(backingPS->put_RotationAngleInDegrees(static_cast<float>(GetRotation())));
        break;
    case KnownPropertyIndex::LinearGradientBrush_Scale:
        IFCFAILFAST(backingPS->put_Scale(GetScale()));
        break;
    case KnownPropertyIndex::LinearGradientBrush_TransformMatrix:
        IFCFAILFAST(backingPS->put_TransformMatrix(GetTransformMatrix()));
        break;
    case KnownPropertyIndex::LinearGradientBrush_CenterPoint:
        IFCFAILFAST(backingPS->put_CenterPoint(GetCenterPoint()));
        break;
    }
}
