// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Included at the end of ImmediateRenderer.cpp (shares its file-local helpers and display list types).
//
// S15 render thread: in immediate modes every animation ticks on the UI thread (CAnimation::ShouldTickOnUIThread), so a
// blocked UI thread freezes the island. While the UI thread walks, elements whose RenderTransform/Opacity is driven by
// an active DoubleAnimation (or an indeterminate ProgressRing, drawn as a rotating arc) get a pinned opacity layer and a
// snapshot of their animation curves (IndependentLayer). A per-renderer render thread re-evaluates those curves and
// replays the last display list with the layers moved/faded whenever the UI thread has not produced a frame recently.
//
// S21 device lost: WINUI_IMMEDIATE_SIMULATE_DEVICE_LOST ("every:N", "f1,f2", "limit:M") turns presented frames into
// DXGI_ERROR_DEVICE_REMOVED; real and simulated losses release all resources, request another frame, and report
// DeviceRecovered (elapsed ms) on the next successful present.
//
// S20 target changes: DPI changes rebuild the island (painter caches realized at the old scale), theme changes force a
// full repaint without subtree reuse; both are reported with TargetChanged.

#include "storyboard.h"
#include "DoubleAnimation.h"
#include "timemgr.h"
#include "timespan.h"
#include "EasingFunctions.h"
#include "TranslateTransform.h"
#include "RotateTransform.h"
#include "ScaleTransform.h"
#include "SkewTransform.h"
#include "CompositeTransform.h"
#include "TransformGroup.h"
#include "TransformCollection.h"
#include "MatrixTransform.h"
#include <FrameworkTheming.h>

namespace II = ImmediateIndependent;

namespace
{
    constexpr int64_t c_uiIdleMicroseconds = 34000;   // render thread presents once the UI thread missed ~2 frames
    constexpr auto c_renderThreadPeriod = std::chrono::milliseconds(16);
    constexpr uint32_t c_curveSamples = 64;
    constexpr float c_spinnerDegreesPerSecond = 360.0f;
    constexpr float c_spinnerSweepDegrees = 270.0f;
}

// Reads timing state that CTimeManager/CStoryboard keep private (friend of both).
struct ImmediateAnimationAccess
{
    static void Index(_In_ CTimeManager* timeManager, _Inout_ std::unordered_map<const CDependencyObject*, std::vector<AnimationBinding>>& out)
    {
        out.clear();
        for (const auto& entry : timeManager->m_hashTable)
        {
            CAnimation* animation = entry.second.get();
            if (animation == nullptr || animation->GetTypeIndex() != KnownTypeIndex::DoubleAnimation
                || animation->m_clockState != DirectUI::ClockState::Active)
            {
                continue;
            }
            CDependencyObject* target = entry.first.m_weakRef.lock_noref();
            if (target != nullptr)
            {
                out[target].push_back({ entry.first.m_propertyIndex, static_cast<CDoubleAnimation*>(animation) });
            }
        }
    }

    static bool Spec(_In_ CTimeline* timeline, _Out_ II::TimingSpec& spec)
    {
        spec = {};
        spec.beginTime = timeline->m_pBeginTime != nullptr ? timeline->m_pBeginTime->m_rTimeSpan : 0;
        spec.speedRatio = timeline->m_rSpeedRatio;
        DirectUI::DurationType type = DirectUI::DurationType::Automatic;
        float value = 0;
        timeline->GetNaturalDuration(&type, &value);
        spec.naturalForever = type == DirectUI::DurationType::Forever;
        spec.naturalDuration = value;
        timeline->GetDurationForProgress(&type, &value);
        spec.progressForever = type == DirectUI::DurationType::Forever;
        spec.progressDuration = value;
        if (timeline->m_repeatBehavior)
        {
            const auto& repeat = timeline->m_repeatBehavior->Value();
            switch (repeat.GetRepeatBehaviorType())
            {
                case DirectUI::RepeatBehaviorType::Forever:
                    spec.repeat = II::RepeatKind::Forever;
                    break;
                case DirectUI::RepeatBehaviorType::Duration:
                    spec.repeat = II::RepeatKind::Duration;
                    spec.repeatDuration = repeat.GetDurationInSec();
                    break;
                default:
                    spec.repeat = II::RepeatKind::Count;
                    spec.repeatCount = repeat.GetCount();
                    break;
            }
        }
        spec.autoReverse = timeline->m_fAutoReverse;
        spec.holdEnd = timeline->m_fillBehavior == DirectUI::FillBehavior::HoldEnd;
        return spec.speedRatio > 0 && std::isfinite(spec.naturalDuration) && std::isfinite(spec.progressDuration);
    }

    // Snapshots a running From/To/By DoubleAnimation in a top-level Storyboard. Key frame animations are not snapshotted.
    static bool Track(_In_ CDoubleAnimation* animation, _Out_ II::AnimationTrack& track)
    {
        track = {};
        if (animation->m_fUsesKeyFrames)
        {
            return false;
        }
        std::vector<CTimeline*> chain;
        for (CTimeline* timeline = animation; timeline != nullptr; timeline = timeline->GetTimingParent())
        {
            chain.push_back(timeline);
            if (timeline->IsTopLevelTimeline() || chain.size() > 16)
            {
                break;
            }
        }
        CTimeline* top = chain.back();
        if (!top->IsTopLevelTimeline() || top->GetTypeIndex() != KnownTypeIndex::Storyboard)
        {
            return false;
        }
        auto storyboard = static_cast<CStoryboard*>(top);
        if (storyboard->IsPaused() || storyboard->m_fIsBeginning || storyboard->m_fIsSeeking
            || storyboard->m_lastParentTime == XDOUBLE_MAX || storyboard->m_rTimeDelta == XDOUBLE_MAX)
        {
            return false;
        }
        for (auto it = chain.rbegin(); it != chain.rend(); ++it)
        {
            II::TimingSpec spec;
            if (!Spec(*it, spec))
            {
                return false;
            }
            track.chain.push_back(spec);
        }
        track.rootParentTime0 = storyboard->m_lastParentTime + storyboard->m_rTimeDelta;

        track.curve.samples.resize(c_curveSamples + 1);
        for (uint32_t i = 0; i <= c_curveSamples; ++i)
        {
            const float eased = CEasingFunctionBase::EaseValue(animation->m_pEasingFunction, static_cast<float>(i) / c_curveSamples);
            track.curve.samples[i] = animation->m_vFrom * (1.0f - eased) + eased * animation->m_vTo;
        }
        track.curve.baseValue = animation->m_vBaseValue;

        // The snapshot must reproduce the value the UI thread just computed; otherwise the clock model does not match.
        const float now = II::EvaluateTrack(track, 0);
        const float tolerance = std::max(0.5f, std::fabs(animation->m_vTo - animation->m_vFrom) * 0.05f);
        return std::isfinite(now) && std::fabs(now - animation->m_vCurrentValue) <= tolerance;
    }
};

namespace
{
    const AnimationBinding* FindBinding(
        const std::unordered_map<const CDependencyObject*, std::vector<AnimationBinding>>& animations,
        _In_ const CDependencyObject* target,
        KnownPropertyIndex property)
    {
        const auto found = animations.find(target);
        if (found == animations.end())
        {
            return nullptr;
        }
        for (const AnimationBinding& binding : found->second)
        {
            if (binding.property == property)
            {
                return &binding;
            }
        }
        return nullptr;
    }

    struct ModelBuilder
    {
        const std::unordered_map<const CDependencyObject*, std::vector<AnimationBinding>>& animations;
        IndependentLayer& layer;
        bool animated = false;

        void Param(II::TransformOp& op, size_t index, float value, _In_ const CDependencyObject* target, KnownPropertyIndex property)
        {
            op.params[index] = value;
            if (const AnimationBinding* binding = FindBinding(animations, target, property))
            {
                II::AnimationTrack track;
                if (ImmediateAnimationAccess::Track(binding->animation, track))
                {
                    op.track[index] = static_cast<int8_t>(layer.tracks.size());
                    layer.tracks.push_back(std::move(track));
                    animated = true;
                }
            }
        }

        bool Add(_In_ CTransform* transform, int depth)
        {
            if (depth > 4 || layer.model.ops.size() > 16)
            {
                return false;
            }
            II::TransformOp op;
            switch (transform->GetTypeIndex())
            {
                case KnownTypeIndex::TranslateTransform:
                {
                    auto t = static_cast<CTranslateTransform*>(transform);
                    op.type = II::TransformOpType::Translate;
                    Param(op, 0, t->m_eX, t, KnownPropertyIndex::TranslateTransform_X);
                    Param(op, 1, t->m_eY, t, KnownPropertyIndex::TranslateTransform_Y);
                    break;
                }
                case KnownTypeIndex::ScaleTransform:
                {
                    auto t = static_cast<CScaleTransform*>(transform);
                    op.type = II::TransformOpType::Scale;
                    Param(op, 0, t->m_eScaleX, t, KnownPropertyIndex::ScaleTransform_ScaleX);
                    Param(op, 1, t->m_eScaleY, t, KnownPropertyIndex::ScaleTransform_ScaleY);
                    Param(op, 2, t->m_ptCenter.x, t, KnownPropertyIndex::ScaleTransform_CenterX);
                    Param(op, 3, t->m_ptCenter.y, t, KnownPropertyIndex::ScaleTransform_CenterY);
                    break;
                }
                case KnownTypeIndex::RotateTransform:
                {
                    auto t = static_cast<CRotateTransform*>(transform);
                    op.type = II::TransformOpType::Rotate;
                    Param(op, 0, t->m_eAngle, t, KnownPropertyIndex::RotateTransform_Angle);
                    Param(op, 1, t->m_ptCenter.x, t, KnownPropertyIndex::RotateTransform_CenterX);
                    Param(op, 2, t->m_ptCenter.y, t, KnownPropertyIndex::RotateTransform_CenterY);
                    break;
                }
                case KnownTypeIndex::SkewTransform:
                {
                    auto t = static_cast<CSkewTransform*>(transform);
                    op.type = II::TransformOpType::Skew;
                    Param(op, 0, t->m_eAngleX, t, KnownPropertyIndex::SkewTransform_AngleX);
                    Param(op, 1, t->m_eAngleY, t, KnownPropertyIndex::SkewTransform_AngleY);
                    Param(op, 2, t->m_ptCenter.x, t, KnownPropertyIndex::SkewTransform_CenterX);
                    Param(op, 3, t->m_ptCenter.y, t, KnownPropertyIndex::SkewTransform_CenterY);
                    break;
                }
                case KnownTypeIndex::CompositeTransform:
                {
                    auto t = static_cast<CCompositeTransform*>(transform);
                    op.type = II::TransformOpType::Composite;
                    Param(op, 0, t->m_ptCenter.x, t, KnownPropertyIndex::CompositeTransform_CenterX);
                    Param(op, 1, t->m_ptCenter.y, t, KnownPropertyIndex::CompositeTransform_CenterY);
                    Param(op, 2, t->m_eScaleX, t, KnownPropertyIndex::CompositeTransform_ScaleX);
                    Param(op, 3, t->m_eScaleY, t, KnownPropertyIndex::CompositeTransform_ScaleY);
                    Param(op, 4, t->m_eSkewX, t, KnownPropertyIndex::CompositeTransform_SkewX);
                    Param(op, 5, t->m_eSkewY, t, KnownPropertyIndex::CompositeTransform_SkewY);
                    Param(op, 6, t->m_eRotation, t, KnownPropertyIndex::CompositeTransform_Rotation);
                    Param(op, 7, t->m_eTranslateX, t, KnownPropertyIndex::CompositeTransform_TranslateX);
                    Param(op, 8, t->m_eTranslateY, t, KnownPropertyIndex::CompositeTransform_TranslateY);
                    break;
                }
                case KnownTypeIndex::TransformGroup:
                {
                    auto group = static_cast<CTransformGroup*>(transform);
                    if (group->m_pChild != nullptr)
                    {
                        const XUINT32 count = group->m_pChild->GetCount();
                        for (XUINT32 i = 0; i < count; ++i)
                        {
                            CDependencyObject* child = static_cast<CDependencyObject*>(group->m_pChild->GetItemWithAddRef(i));
                            auto release = wil::scope_exit([&] { ReleaseInterface(child); });
                            auto childTransform = do_pointer_cast<CTransform>(child);
                            if (childTransform == nullptr || !Add(childTransform, depth + 1))
                            {
                                return false;
                            }
                        }
                    }
                    return true;
                }
                case KnownTypeIndex::MatrixTransform:
                {
                    CMILMatrix m(true);
                    transform->GetTransform(&m);
                    op.type = II::TransformOpType::Matrix;
                    op.params[0] = m.GetM11();
                    op.params[1] = m.GetM12();
                    op.params[2] = m.GetM21();
                    op.params[3] = m.GetM22();
                    op.params[4] = m.GetDx();
                    op.params[5] = m.GetDy();
                    break;
                }
                default:
                    return false;
            }
            layer.model.ops.push_back(op);
            return true;
        }
    };

    // Indeterminate MUXC ProgressRing whose Lottie content is not painted: drawn as a spinning arc.
    bool IsActiveProgressRing(_In_ CUIElement* element)
    {
        if (element->GetTypeIndex() != KnownTypeIndex::Control || !element->HasManagedPeer())
        {
            return false;
        }
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_progressRing, L"Microsoft.UI.Xaml.Controls.ProgressRing");
        if (FAILED(FxCallbacks::FrameworkCallbacks_CheckPeerType(element, c_progressRing, FALSE)))
        {
            return false;
        }
        CUIElement** children = nullptr;
        XUINT32 count = 0;
        element->GetChildrenInRenderOrder(&children, &count);
        return count > 0 && children[0] != nullptr && children[0]->IsVisible() && children[0]->GetOpacityLocal() > 0;
    }

    double SecondsNow() noexcept
    {
        return QpcMicroseconds() / 1e6;
    }
}

void ImmediateRenderer::BuildAnimationIndex(_In_ CXamlIslandRoot* island)
{
    m_independent->animations.clear();
    CCoreServices* core = island->GetContext();
    CTimeManager* timeManager = core != nullptr ? core->GetTimeManager() : nullptr;
    if (timeManager != nullptr)
    {
        ImmediateAnimationAccess::Index(timeManager, m_independent->animations);
    }
}

void ImmediateRenderer::ProbeIndependent(_In_ CUIElement* element, _Out_ IndependentProbe& probe)
{
    probe = {};
    IndependentLayer& layer = probe.layer;
    const auto& animations = m_independent->animations;
    if (!animations.empty())
    {
        layer.opacity0 = element->GetOpacityLocal();
        if (const AnimationBinding* binding = FindBinding(animations, element, KnownPropertyIndex::UIElement_Opacity))
        {
            II::AnimationTrack track;
            if (ImmediateAnimationAccess::Track(binding->animation, track))
            {
                layer.opacityTrack = static_cast<int>(layer.tracks.size());
                layer.tracks.push_back(std::move(track));
            }
        }

        // Only facades are applied before the RenderTransform in the local transform; skip those elements.
        CTransform* renderTransform = element->GetRenderTransformLocal();
        if (renderTransform != nullptr && !element->AreFacadesInUse())
        {
            const size_t tracksBefore = layer.tracks.size();
            ModelBuilder builder{ animations, layer };
            if (builder.Add(renderTransform, 0) && builder.animated)
            {
                layer.hasTransform = true;
                std::vector<float> values(layer.tracks.size());
                for (const II::TransformOp& op : layer.model.ops)
                {
                    for (size_t i = 0; i < II::TransformOp::c_maxParams; ++i)
                    {
                        if (op.track[i] >= 0)
                        {
                            values[op.track[i]] = op.params[i];
                        }
                    }
                }
                layer.render0 = II::EvaluateTransform(layer.model, values.data(), values.size());
            }
            else
            {
                layer.tracks.resize(tracksBefore);
                layer.model.ops.clear();
            }
        }
    }

    if (layer.opacityTrack < 0 && !layer.hasTransform && IsActiveProgressRing(element))
    {
        layer.spinner = true;
        layer.hasTransform = true;
        const float width = element->GetActualWidth();
        const float height = element->GetActualHeight();
        const float angle0 = static_cast<float>(std::fmod(SecondsNow() * c_spinnerDegreesPerSecond, 360.0));
        II::AnimationTrack spin;
        spin.kind = II::AnimationTrack::Kind::Linear;
        spin.value0 = angle0;
        spin.rate = c_spinnerDegreesPerSecond;
        layer.tracks.push_back(std::move(spin));
        II::TransformOp op;
        op.type = II::TransformOpType::Rotate;
        op.params[0] = angle0;
        op.params[1] = width / 2;
        op.params[2] = height / 2;
        op.track[0] = 0;
        layer.model.ops.push_back(op);
        layer.render0 = II::RotationMatrix(angle0, width / 2, height / 2);
    }
    probe.active = layer.opacityTrack >= 0 || layer.hasTransform;
}

void ImmediateRenderer::FinishIndependentLayer(
    _In_ CUIElement* element,
    size_t layerIndex,
    IndependentProbe& probe,
    const WalkContext& ctx,
    _Inout_ DisplayList& list)
{
    IndependentLayer& layer = probe.layer;
    layer.world0 = ToMatrix(ctx.world);
    layer.outerClip = probe.outerClip;
    const float width = element->GetActualWidth();
    const float height = element->GetActualHeight();
    if (!layer.spinner)
    {
        const XPOINTF origin = element->GetRenderTransformOrigin();
        layer.origin = { origin.x * width, origin.y * height };
        return;
    }

    // Spinner: draw the arc (at the snapshot angle) only when the subtree painted nothing (Lottie content).
    const bool drew = std::any_of(list.commands.begin() + layerIndex + 1, list.commands.end(),
        [](const Command& c) { return IsDrawing(c.type); });
    if (drew || width <= 0 || height <= 0)
    {
        probe.active = false;
        list.commands[layerIndex].pinned = false;
        return;
    }
    const float size = std::min(width, height);
    const float thickness = std::max(1.0f, size / 10.0f);
    ColorF color{ 0.0f, 0.47f, 0.84f, 1.0f };
    CValue value;
    if (SUCCEEDED(element->GetValueByIndex(KnownPropertyIndex::Control_Foreground, &value)))
    {
        ColorF resolved;
        if (ResolveBrush(do_pointer_cast<CBrush>(value.AsObject()), 1.0f, &resolved) == BrushResult::Color)
        {
            color = resolved;
        }
    }
    PathGeometryData data;
    II::BuildArc(width / 2, height / 2, (size - thickness) / 2, layer.model.ops[0].params[0], c_spinnerSweepDegrees, data.verbs, data.points);
    Emitter emitter{ list, element, ctx };
    emitter.Path(std::move(data), thickness, color, ctx.world, RectF{ -thickness, -thickness, width + thickness, height + thickness });
    // The arc is drawn at the snapshot angle; the render thread rotates it from there (render0 = identity offset).
    layer.render0 = II::RotationMatrix(layer.model.ops[0].params[0], width / 2, height / 2);
    layer.origin = {};
}

void ImmediateRenderer::AppendIndependentDamage(_Inout_ IslandState& state, _Inout_ std::vector<RectF>& damage, bool hasBlits, _Inout_ bool& fullDamage)
{
    IndependentIsland& independent = state.independent;
    if (!independent.extraDamage.empty())
    {
        // Pixels the render thread moved are not in the element diff. Blits would carry them to new places: repaint all.
        if (hasBlits)
        {
            fullDamage = true;
        }
        else
        {
            damage.insert(damage.end(), independent.extraDamage.begin(), independent.extraDamage.end());
        }
        independent.extraDamage.clear();
    }
    independent.lastBounds.clear();
    independent.paintedFinal = false;
}

void ImmediateRenderer::OnIslandPresented(_Inout_ IslandState& state, uint32_t frameNumber)
{
    IndependentState& s = *m_independent;
    if (s.recoveryPending)
    {
        s.recoveryPending = false;
        const int64_t elapsed = QpcMicroseconds() - s.lossQpc;
        TraceDeviceRecovered(m_mode, static_cast<uint32_t>(std::max<int64_t>(0, elapsed) / 1000), frameNumber, s.lastLossSimulated, s.lossCount);
    }
    if (!state.previousWalk.layers.empty())
    {
        StartRenderThreadIfNeeded();
    }
}

bool ImmediateRenderer::NeedsFrame() const noexcept
{
    return m_independent->recoveryPending;
}

void ImmediateRenderer::StartRenderThreadIfNeeded()
{
    IndependentState& s = *m_independent;
    s.running = true;
    if (s.thread == nullptr)
    {
        s.thread = CreateThread(nullptr, 0, [](void* context) -> DWORD
        {
            static_cast<ImmediateRenderer*>(context)->RenderThreadMain();
            return 0;
        }, this, 0, nullptr);
        if (s.thread == nullptr)
        {
            s.running = false;
            return;
        }
    }
    s.wake.notify_one();
}

void ImmediateRenderer::StopRenderThread() noexcept
{
    IndependentState& s = *m_independent;
    {
        std::lock_guard<std::recursive_mutex> lock(s.mutex);
        s.stop = true;
    }
    s.wake.notify_all();
    if (s.thread != nullptr)
    {
        WaitForSingleObject(s.thread, INFINITE);
        CloseHandle(s.thread);
        s.thread = nullptr;
    }
}

void ImmediateRenderer::RenderThreadMain()
{
    IGNOREHR(SetThreadDescription(GetCurrentThread(), L"WinUI immediate render thread"));
    IndependentState& s = *m_independent;
    std::unique_lock<std::recursive_mutex> lock(s.mutex);
    while (!s.stop)
    {
        if (!s.running)
        {
            s.wake.wait(lock, [&] { return s.stop || s.running; });
            continue;
        }
        s.wake.wait_for(lock, c_renderThreadPeriod, [&] { return s.stop; });
        if (s.stop)
        {
            break;
        }
        bool any = false;
        const int64_t now = QpcMicroseconds();
        for (auto& entry : m_islands)
        {
            IslandState& state = *entry.second;
            if (state.previousWalk.layers.empty() || !state.painter || FAILED(s.renderThreadLoss))
            {
                continue;
            }
            any = true;
            if (now - state.independent.lastUiQpc >= c_uiIdleMicroseconds && !state.independent.paintedFinal)
            {
                RenderIndependentFrame(entry.first, state, now);
            }
        }
        if (!any)
        {
            s.running = false;
        }
    }
}

void ImmediateRenderer::RenderIndependentFrame(_In_ CXamlIslandRoot* island, _Inout_ IslandState& state, int64_t now)
{
    const DisplayList& list = state.previousWalk;
    IndependentIsland& independent = state.independent;
    const float scale = state.scale > 0 ? state.scale : 1.0f;
    const double elapsed = std::max<int64_t>(0, now - list.snapshotQpc) / 1e6;

    std::vector<Command> commands = list.commands;
    if (independent.lastBounds.size() != list.layers.size())
    {
        independent.lastBounds.resize(list.layers.size());
        for (size_t i = 0; i < list.layers.size(); ++i)
        {
            independent.lastBounds[i] = list.commands[list.layers[i].pushIndex].deviceBounds;
        }
    }

    std::vector<RectF> damage;
    bool running = false;
    std::vector<float> values;
    for (size_t li = 0; li < list.layers.size(); ++li)
    {
        const IndependentLayer& layer = list.layers[li];
        values.resize(layer.tracks.size());
        for (size_t t = 0; t < layer.tracks.size(); ++t)
        {
            bool trackRunning = false;
            values[t] = II::EvaluateTrack(layer.tracks[t], elapsed, &trackRunning);
            running = running || trackRunning;
        }
        Command& push = commands[layer.pushIndex];
        const uint32_t pop = push.index;
        if (push.type != CommandType::PushLayer || pop >= commands.size())
        {
            continue;
        }
        if (layer.opacityTrack >= 0)
        {
            push.opacity = std::min(1.0f, std::max(0.0f, values[layer.opacityTrack]));
        }
        RectF bounds = push.deviceBounds;
        if (layer.hasTransform)
        {
            const II::Matrix3x2F render1 = II::EvaluateTransform(layer.model, values.data(), values.size());
            const II::Matrix3x2F delta = II::ComputeLayerDelta(layer.world0, layer.origin, layer.render0, render1);
            bounds = {};
            for (uint32_t i = layer.pushIndex + 1; i < pop; ++i)
            {
                Command& c = commands[i];
                if (c.type == CommandType::Nop)
                {
                    continue;
                }
                c.transform = II::Multiply(c.transform, delta);
                if (!RectsEqual(c.clip, layer.outerClip))
                {
                    c.clip = IntersectRect(II::TransformBounds(delta, c.clip), layer.outerClip);
                }
                c.deviceBounds = IntersectRect(II::TransformDeviceBounds(delta, c.deviceBounds, scale), ToDevice(c.clip, scale));
                bounds = UnionRect(bounds, c.deviceBounds);
            }
            push.deviceBounds = bounds;
            commands[pop].deviceBounds = bounds;
        }
        damage.push_back(independent.lastBounds[li]);
        damage.push_back(bounds);
        independent.lastBounds[li] = bounds;
    }

    const RectF surface{ 0, 0, static_cast<float>(state.width), static_cast<float>(state.height) };
    for (RectF& r : damage)
    {
        r = IntersectRect(r, surface);
    }
    damage.erase(std::remove_if(damage.begin(), damage.end(), [](const RectF& r) { return IsEmptyRect(r); }), damage.end());
    if (damage.empty())
    {
        independent.paintedFinal = !running;
        return;
    }
    CoalesceDamage(damage, c_maxDamageRects);

    const uint32_t frameId = 0x80000000u | (++m_independent->frameCounter & 0x7fffffffu);
    const RectF islandClip{ 0, 0, state.width / scale, state.height / scale };
    uint32_t rendered = 0;
    TraceBackendFrameBegin(frameId);
    const int64_t paintStart = QpcMicroseconds();
    const HRESULT hr = PaintTracedFrame(*state.painter, frameId, reinterpret_cast<uint64_t>(island), damage.data(),
        static_cast<uint32_t>(damage.size()), [&](IImmediatePainter& painter)
        {
            ReplayCommands(painter, commands, list, islandClip, false, damage, false, rendered);
        });
    const int64_t paintEnd = QpcMicroseconds();
    TraceBackendFrameEnd(frameId, SUCCEEDED(hr));
    uint64_t pixels = 0;
    for (const RectF& d : damage)
    {
        pixels += static_cast<uint64_t>((d.right - d.left) * (d.bottom - d.top));
    }
    TraceIndependentFrame(frameId, reinterpret_cast<uint64_t>(island), static_cast<uint32_t>(list.layers.size()),
        static_cast<uint32_t>((now - independent.lastUiQpc) / 1000), pixels, static_cast<uint64_t>(paintEnd - paintStart), hr);

    if (FAILED(hr))
    {
        // Never release resources off the UI thread (Composition visuals); the next UI frame recovers.
        if (IsDeviceLost(hr))
        {
            m_independent->renderThreadLoss = hr;
        }
        state.fullDamage = true;
        independent.paintedFinal = true;
        return;
    }
    independent.extraDamage.insert(independent.extraDamage.end(), damage.begin(), damage.end());
    CoalesceDamage(independent.extraDamage, c_maxDamageRects);
    ++independent.frames;
    independent.paintedFinal = !running;

    if (!GetCapturePath().empty())
    {
        LOG_IF_FAILED(CapturePresentedBuffer(state.swapChain.Get(), (GetCapturePath() + L".rt.bmp").c_str()));
    }
}

HRESULT ImmediateRenderer::ApplyDeviceLostSimulation(HRESULT hr, uint32_t frameNumber)
{
    UNREFERENCED_PARAMETER(frameNumber);
    IndependentState& s = *m_independent;
    if (!s.simulationParsed)
    {
        s.simulationParsed = true;
        wchar_t buffer[512];
        const DWORD length = GetEnvironmentVariableW(L"WINUI_IMMEDIATE_SIMULATE_DEVICE_LOST", buffer, ARRAYSIZE(buffer));
        if (length > 0 && length < ARRAYSIZE(buffer))
        {
            s.simulation = II::DeviceLostSimulation::Parse(buffer);
        }
    }
    if (FAILED(hr) || !s.simulation.IsEnabled())
    {
        return hr;
    }
    ++s.presentedFrames;
    if (s.simulation.ShouldLose(s.presentedFrames, s.lossCount))
    {
        s.simulatingLoss = true;
        return DXGI_ERROR_DEVICE_REMOVED;
    }
    return hr;
}

void ImmediateRenderer::OnDeviceLost(HRESULT hr, uint32_t frameNumber)
{
    IndependentState& s = *m_independent;
    const bool simulated = s.simulatingLoss;
    s.simulatingLoss = false;
    // D2DPainter traces its own real device losses; GDI+ and simulated losses are traced here.
    if (simulated || m_mode == Mode::ImmediateGdiPlus)
    {
        HRESULT removedReason = S_OK;
        if (!simulated && m_device)
        {
            removedReason = m_device->GetDeviceRemovedReason();
        }
        TraceDeviceLost(m_mode, hr, removedReason, frameNumber);
    }
    ++s.lossCount;
    if (!s.recoveryPending)
    {
        s.lossQpc = QpcMicroseconds();
    }
    s.recoveryPending = true;
    s.lastLossSimulated = simulated;
    s.renderThreadLoss = S_OK;
}

void ImmediateRenderer::CheckTargetChanges(_In_ CXamlIslandRoot* island, uint32_t frameNumber, uint32_t width, uint32_t height, float scale)
{
    IndependentState& s = *m_independent;
    const uint64_t rootId = reinterpret_cast<uint64_t>(island);

    uint32_t themeKey = 0;
    if (CCoreServices* core = island->GetContext())
    {
        themeKey = static_cast<uint32_t>(core->GetRequestedThemeForSubTree());
        if (FrameworkTheming* theming = core->GetFrameworkTheming())
        {
            themeKey |= (static_cast<uint32_t>(theming->GetTheme()) << 8) | (theming->HasHighContrastTheme() ? 0x10000u : 0u);
        }
    }
    if (s.themeKey != UINT32_MAX && s.themeKey != themeKey)
    {
        // Theme resources change brushes everywhere: repaint and re-walk every island without subtree reuse.
        for (auto& entry : m_islands)
        {
            entry.second->fullDamage = true;
            entry.second->previousWalk = DisplayList();
            TraceTargetChanged(reinterpret_cast<uint64_t>(entry.first), entry.second->width, entry.second->height,
                entry.second->scale, 3, frameNumber);
        }
    }
    s.themeKey = themeKey;

    const auto found = m_islands.find(island);
    if (found == m_islands.end() || !found->second->painter)
    {
        return;
    }
    IslandState& state = *found->second;
    if (state.scale != scale && II::ScaleChangeInvalidatesCaches(state.scale, scale))
    {
        // Glyph, path and bitmap realizations depend on the scale: rebuild the painter, swap chain and visual.
        TraceTargetChanged(rootId, width, height, scale, 2, frameNumber);
        IGNOREHR(state.painter->SetPresentationSwapChain(nullptr));
        m_islands.erase(found);
    }
    else if (state.width != width || state.height != height || state.scale != scale)
    {
        TraceTargetChanged(rootId, width, height, scale, 1, frameNumber);
    }
}
