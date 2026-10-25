// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "SolidColorBrush.h"
#include "ColorUtil.h"
#include <DependencyObjectDCompRegistry.h>
#include <ExpressionHelper.h>
#include <TimeMgr.h>
#include <DCompTreeHost.h>
#include <WinRTExpressionConversionContext.h>
#include "WRLHelper.h"
#include <WinUIProfilerResourceHooks.h>
#include <memory>

namespace
{
    void ReleaseWUCBrush(wrl::ComPtr<WUComp::ICompositionBrush>& brush)
    {
        if (brush != nullptr)
        {
            brush.Reset();
            const auto owner = std::addressof(brush);
            WinUIProfiler::Hooks::GraphicsResourceChanged(owner, nullptr,
                WinUIProfiler::Hooks::GraphicsResourceKind::CompositionBrush,
                WinUIProfiler::Hooks::GraphicsSurfaceOperation::OwnershipRelease, 0, 0);
            WinUIProfiler::Hooks::GraphicsResourceChanged(owner, nullptr,
                WinUIProfiler::Hooks::GraphicsResourceKind::CompositionBrush,
                WinUIProfiler::Hooks::GraphicsSurfaceOperation::Destroy, 0, 0);
        }
    }
}

CSolidColorBrush::CSolidColorBrush(
    _In_ const CSolidColorBrush& original,
    _Out_ HRESULT& hr
    )
    : CBrush(original, hr)
    , m_rgb(original.m_rgb)
    , m_isWUCColorAnimationDirty(true)
    , m_isWUCBrushDirty(true)
{
}

CSolidColorBrush::~CSolidColorBrush()
{
    if (GetDCompObjectRegistry() != nullptr)
    {
        GetDCompObjectRegistry()->UnregisterObject(this);
    }
    ReleaseWUCBrush(m_wucBrush);
}

void CSolidColorBrush::ReleaseDCompResources()
{
    __super::ReleaseDCompResources();

    SetDCompAnimation(nullptr, KnownPropertyIndex::SolidColorBrush_ColorAAnimation);
    SetDCompAnimation(nullptr, KnownPropertyIndex::SolidColorBrush_ColorRAnimation);
    SetDCompAnimation(nullptr, KnownPropertyIndex::SolidColorBrush_ColorGAnimation);
    SetDCompAnimation(nullptr, KnownPropertyIndex::SolidColorBrush_ColorBAnimation);

    ReleaseWUCBrush(m_wucBrush);
    m_isWUCBrushDirty = true;
    SetDCompAnimation(nullptr, KnownPropertyIndex::SolidColorBrush_ColorAnimation);
}

void CSolidColorBrush::SetDCompResourceDirty()
{
    if (!m_isWUCBrushDirty)
    {
        m_isWUCBrushDirty = true;
    }
}

void CSolidColorBrush::NWSetRenderDirty(
    _In_ CDependencyObject* pTarget,
    DirtyFlags flags
    )
{
    ASSERT(pTarget->OfTypeByIndex<KnownTypeIndex::SolidColorBrush>());

    CSolidColorBrush *pBrush = static_cast<CSolidColorBrush*>(pTarget);

    if (!pBrush->m_isWUCBrushDirty
        && !flags_enum::is_set(flags, DirtyFlags::Independent))
    {
        // TODO: DCompAnim: This will currently over-dirty the animated color. Xaml will evaluate the transform one
        // last time when the storyboard expires, which goes through SetAnimatedValue with a dependent value, which then
        // gets here. This will mark the (currently animating in DComp) resource dirty, which will cause us to regenerate
        // the DComp color without an animation. The result is that the DComp animation ends prematurely.
        // We'll have the same problem any time the animation evaluates on the Xaml UI thread and calls SetValue with
        // something dependent, which shows up as the DComp animation jumping forward or backwards. This is a temporary
        // problem - when all animations are converted to DComp, we can stop setting dependent values for DComp animations.
        pBrush->SetDCompResourceDirty();
    }

    __super::NWSetRenderDirty(pTarget, flags);
}

WUComp::ICompositionBrush* CSolidColorBrush::GetWUCBrush(_In_ WUComp::ICompositor* compositor)
{
    if (m_isWUCBrushDirty || m_wucBrush == nullptr)
    {
        wrl::ComPtr<WUComp::ICompositionColorBrush> colorBrush;

        if (m_wucBrush == nullptr)
        {
            IFCFAILFAST(compositor->CreateColorBrush(colorBrush.ReleaseAndGetAddressOf()));
            IFCFAILFAST(colorBrush.As(&m_wucBrush));

            // Initialize the WUC brush with this brush's color. If this brush is animating, and the animation has a start
            // delay, then this static color will show up until the animation starts for real. If we don't initialize the
            // start color, then the WUC brush will show its default (transparent black) until the animation starts for real,
            // which will make the element disappear.
            IFCFAILFAST(colorBrush->put_Color(ColorUtils::GetWUColor(m_rgb)));
            WinUIProfiler::Hooks::GraphicsResourceChanged(std::addressof(m_wucBrush), nullptr,
                WinUIProfiler::Hooks::GraphicsResourceKind::CompositionBrush,
                WinUIProfiler::Hooks::GraphicsSurfaceOperation::Create, 0, 0, S_OK,
                static_cast<CDependencyObject*>(this));

            if (GetDCompObjectRegistry() != nullptr)
            {
                GetDCompObjectRegistry()->EnsureObjectWithDCompResourceRegistered(this);
            }
        }
        else
        {
            IFCFAILFAST(m_wucBrush.As(&colorBrush));
        }

        const auto& animation = GetWUCDCompAnimation(KnownPropertyIndex::SolidColorBrush_ColorAnimation);

        if (animation == nullptr)
        {
            IFCFAILFAST(colorBrush->put_Color(ColorUtils::GetWUColor(m_rgb)));
        }
        else if (m_isWUCColorAnimationDirty)
        {
            const auto& timeManager = GetTimeManager();

            wrl::ComPtr<WUComp::ICompositionObject> colorBrushICO;
            IFCFAILFAST(colorBrush.As(&colorBrushICO));

            CTimeManager::StartWUCAnimation(
                compositor,
                colorBrushICO.Get(),
                ExpressionHelper::sc_propertyName_Color,
                animation.get(),
                this,
                KnownPropertyIndex::SolidColorBrush_Color,
                timeManager);
        }

        m_isWUCBrushDirty = false;
        m_isWUCColorAnimationDirty = false;
    }

    WinUIProfiler::Hooks::GraphicsResourceChanged(std::addressof(m_wucBrush), nullptr,
        WinUIProfiler::Hooks::GraphicsResourceKind::CompositionBrush,
        WinUIProfiler::Hooks::GraphicsSurfaceOperation::BaselinePresent, 0, 0, S_OK,
        static_cast<CDependencyObject*>(this));
    return m_wucBrush.Get();
}

void CSolidColorBrush::EnsureWUCAnimationStarted(_Inout_ WinRTExpressionConversionContext* context)
{
    GetWUCBrush(context->GetCompositorNoRef());
}

void CSolidColorBrush::SetIsColorAnimationDirty(bool value)
{
    m_isWUCColorAnimationDirty = value;
}
