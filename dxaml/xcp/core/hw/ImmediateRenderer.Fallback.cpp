// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// S11/S16: per-window (island) fallback to Composition. When the immediate walk finds content that only Composition can
// present (app Composition visuals and animations, swap chains, media), the island permanently switches to
// HWWalk::RenderRoot; other islands keep the immediate backend. See docs/design-notes/
// immediate-rendering-backends-proposal.md, "Full-backend decisions (S11-S23)".

#include "precomp.h"
#include "ImmediateRenderer.h"
#include <ImmediateRenderingPolicy.h>
#include <XamlProfilerTracing.h>
#include "XamlIslandRoot.h"
#include <FxCallbacks.h>
#include <MetadataAPI.h>

using namespace RenderingBackend;
using Microsoft::WRL::ComPtr;

namespace
{
    bool FallbackDisabled() noexcept
    {
        static const bool disabled = []
        {
            wchar_t value[8] = {};
            return GetEnvironmentVariableW(L"WINUI_IMMEDIATE_NO_FALLBACK", value, ARRAYSIZE(value)) > 0 && value[0] == L'1';
        }();
        return disabled;
    }

    bool IsPeerOfType(_In_opt_ CDependencyObject* object, _In_ const xstring_ptr& typeName) noexcept
    {
        return object != nullptr && object->HasManagedPeer()
            && SUCCEEDED(FxCallbacks::FrameworkCallbacks_CheckPeerType(object, typeName, FALSE));
    }

    // Reads a custom (non-built-in) dependency property such as MUXC ProgressRing.IndeterminateSource. Fails safe:
    // false when the type or property cannot be resolved.
    bool TryGetCustomValue(_In_ CDependencyObject* object, _In_ const xstring_ptr& typeName, _In_ const xstring_ptr& propertyName, _Out_ CValue* value) noexcept
    {
        const CClassInfo* type = nullptr;
        const CDependencyProperty* property = nullptr;
        return SUCCEEDED(DirectUI::MetadataAPI::GetClassInfoByFullName(typeName, &type)) && type != nullptr
            && SUCCEEDED(DirectUI::MetadataAPI::TryGetDependencyPropertyByName(type, propertyName, &property)) && property != nullptr
            && SUCCEEDED(object->GetValue(property, value));
    }

    bool IsNullObject(const CValue& value) noexcept
    {
        return value.IsNullOrUnset()
            || (value.GetType() == valueObject && value.AsObject() == nullptr)
            || (value.GetType() == valueIInspectable && value.AsIInspectable() == nullptr);
    }

    DECLARE_CONST_XSTRING_PTR_STORAGE(c_progressRingStorage, L"Microsoft.UI.Xaml.Controls.ProgressRing");
}

bool ImmediateRenderer::IsBuiltInProgressRingPlayer(_In_ CUIElement* element) noexcept
{
    // ProgressRing's template hosts an AnimatedVisualPlayer ("LottiePlayer") whose source is a built-in animation
    // unless the app sets DeterminateSource/IndeterminateSource. The ring is drawn natively instead.
    const xstring_ptr c_progressRing = XSTRING_PTR_FROM_STORAGE(c_progressRingStorage);
    CDependencyObject* templatedParent = element->GetTemplatedParent();
    if (!IsPeerOfType(templatedParent, c_progressRing))
    {
        return false;
    }
    DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_determinateSource, L"DeterminateSource");
    DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_indeterminateSource, L"IndeterminateSource");
    CValue value;
    if (TryGetCustomValue(templatedParent, c_progressRing, c_determinateSource, &value) && !IsNullObject(value))
    {
        return false;
    }
    value.SetNull();
    if (TryGetCustomValue(templatedParent, c_progressRing, c_indeterminateSource, &value) && !IsNullObject(value))
    {
        return false;
    }
    return true;
}

bool ImmediateRenderer::IsHandOffVisualTouched(_In_ CUIElement* element) noexcept
{
    ComPtr<WUComp::IVisual> visual;
    element->GetStoredHandOffVisual(visual.ReleaseAndGetAddressOf());
    if (visual == nullptr)
    {
        return false;
    }

    ImmediateRenderingPolicy::HandOffVisualState state;
    IGNOREHR(visual->get_Opacity(&state.opacity));
    IGNOREHR(visual->get_RotationAngle(&state.rotationAngle));
    wfn::Vector3 v{};
    if (SUCCEEDED(visual->get_Offset(&v)))
    {
        state.offset[0] = v.X; state.offset[1] = v.Y; state.offset[2] = v.Z;
    }
    if (SUCCEEDED(visual->get_Scale(&v)))
    {
        state.scale[0] = v.X; state.scale[1] = v.Y; state.scale[2] = v.Z;
    }
    wfn::Matrix4x4 m{};
    if (SUCCEEDED(visual->get_TransformMatrix(&m)))
    {
        const float values[16] = { m.M11, m.M12, m.M13, m.M14, m.M21, m.M22, m.M23, m.M24, m.M31, m.M32, m.M33, m.M34, m.M41, m.M42, m.M43, m.M44 };
        std::copy(std::begin(values), std::end(values), std::begin(state.transformMatrix));
    }

    // Translation set by the app through the visual's property set (SetIsTranslationEnabled). XAML's own
    // UIElement.Translation (painted by the walk) is excluded.
    ComPtr<WUComp::ICompositionObject> object;
    ComPtr<WUComp::ICompositionPropertySet> properties;
    if (SUCCEEDED(visual.As(&object)) && SUCCEEDED(object->get_Properties(&properties)) && properties != nullptr)
    {
        WUComp::CompositionGetValueStatus status{};
        wfn::Vector3 translation{};
        if (SUCCEEDED(properties->TryGetVector3(wrl_wrappers::HStringReference(L"Translation").Get(), &translation, &status))
            && status == WUComp::CompositionGetValueStatus_Succeeded)
        {
            const wfn::Vector3 xamlTranslation = element->GetTranslation();
            state.translation[0] = translation.X - xamlTranslation.X;
            state.translation[1] = translation.Y - xamlTranslation.Y;
            state.translation[2] = translation.Z - xamlTranslation.Z;
        }
    }

    ComPtr<WUComp::ICompositionClip> clip;
    if (SUCCEEDED(visual->get_Clip(&clip)) && clip != nullptr)
    {
        ComPtr<WUComp::IInsetClip> insetClip;
        state.hasNonInsetClip = FAILED(clip.As(&insetClip));
    }

    return ImmediateRenderingPolicy::IsHandOffVisualTouched(state);
}

void ImmediateRenderer::CheckFallback(_In_ CUIElement* element) noexcept
{
    if (m_pendingFallback != nullptr)
    {
        return;
    }

    ImmediateRenderingPolicy::ElementInterop interop;
    const KnownTypeIndex type = element->GetTypeIndex();
    interop.swapChainPanel = type == KnownTypeIndex::SwapChainPanel || element->HasSwapChainContent();
    interop.media = type == KnownTypeIndex::MediaPlayerPresenter
        || (type == KnownTypeIndex::FullWindowMediaRoot && element->GetFirstChildNoAddRef() != nullptr);
    interop.connectedAnimation = element->HasActiveConnectedAnimation()
        || (type == KnownTypeIndex::ConnectedAnimationRoot && element->GetFirstChildNoAddRef() != nullptr);
    interop.implicitShowHideAnimation = element->HasImplicitShowAnimation() || element->HasImplicitHideAnimation();
    if (element->IsUsingHandInVisual())
    {
        interop.handInVisual = true;
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_animatedVisualPlayer, L"Microsoft.UI.Xaml.Controls.AnimatedVisualPlayer");
        if (IsBuiltInProgressRingPlayer(element))
        {
            interop.handInIsBuiltInProgressRing = true;
        }
        else if (IsPeerOfType(element, c_animatedVisualPlayer))
        {
            interop.appAnimatedVisualPlayer = true;
        }
    }
    // App animations on UIElement facades (StartAnimation on Translation/Scale/...) run in Composition.
    interop.handOffVisualTouched = element->HasFacadeAnimation()
        || (element->IsUsingHandOffVisual() && IsHandOffVisualTouched(element));

    if (const wchar_t* reason = ImmediateRenderingPolicy::GetFallbackReason(interop))
    {
        if (FallbackDisabled())
        {
            ReportUnsupported(element, reason);
            return;
        }
        m_pendingFallback = reason;
        const xstring_ptr className = element->GetClassName();
        m_pendingFallbackClass = className.IsNullOrEmpty() ? L"" : className.GetBuffer();
    }
}

void ImmediateRenderer::FallBackIsland(_In_ DCompTreeHost* dcompTreeHost, _In_ CXamlIslandRoot* island) noexcept
{
    std::wstring reason = m_pendingFallbackClass;
    reason += L' ';
    reason += m_pendingFallback;
    m_pendingFallback = nullptr;

    TraceWindowFallback(m_mode, reinterpret_cast<uint64_t>(island), reason.c_str());
    if (FILE* statsFile = StatsFile())
    {
        fwprintf(statsFile, L"# fallback %s\n", reason.c_str());
        fflush(statsFile);
    }

    m_fallenBack[island] = true; // the next HWWalk pass must force a full redraw

    // Unhook the island's immediate sprite so DCompTreeHost::ConnectXamlIslandTargetRoots hosts the island's comp
    // node visual (created by the HWWalk pass that follows) as the ContentIsland root.
    auto& renderDataMap = dcompTreeHost->GetXamlIslandRenderData();
    auto found = renderDataMap.find(island);
    if (found != renderDataMap.end())
    {
        found->second.contentConnected = false;
        found->second.m_islandRootVisual.Reset();
    }
}

bool ImmediateRenderer::IsFallenBack(_In_ CXamlIslandRoot* island) const noexcept
{
    return m_fallenBack.find(island) != m_fallenBack.end();
}

bool ImmediateRenderer::ConsumeFallbackRedraw(_In_ CXamlIslandRoot* island) noexcept
{
    auto found = m_fallenBack.find(island);
    if (found == m_fallenBack.end() || !found->second)
    {
        return false;
    }
    found->second = false;
    return true;
}

void ImmediateRenderer::PruneFallenBack(_In_ DCompTreeHost* dcompTreeHost) noexcept
{
    auto& renderData = dcompTreeHost->GetXamlIslandRenderData();
    for (auto it = m_fallenBack.begin(); it != m_fallenBack.end();)
    {
        it = (renderData.find(it->first) == renderData.end()) ? m_fallenBack.erase(it) : std::next(it);
    }
}
