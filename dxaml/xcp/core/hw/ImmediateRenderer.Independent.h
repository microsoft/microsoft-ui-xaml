// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Immediate render thread (S15) and device-lost recovery (S21) state for ImmediateRenderer. Included by
// ImmediateRenderer.cpp before its display list types; the implementation is in ImmediateRenderer.Independent.inl.

#include <ImmediateIndependent.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <unordered_map>
#include <vector>

class CDependencyObject;
class CDoubleAnimation;

namespace ImmediateRenderingDetail
{
    // A recorded opacity layer (pinned PushLayer .. PopLayer) whose transform and/or opacity the render thread
    // re-evaluates from snapshotted animation curves while the UI thread is not producing frames.
    struct IndependentLayer
    {
        uint32_t pushIndex = 0;                       // PushLayer command; its index is the matching PopLayer
        bool spinner = false;                         // indeterminate ProgressRing arc (linear rotation)
        RenderingBackend::Matrix3x2F world0;          // element -> island at the snapshot (RenderTransform included)
        RenderingBackend::PointF origin{};            // RenderTransformOrigin in element DIPs
        RenderingBackend::RectF outerClip{};          // DIPs, island space: clip from the ancestors (does not move)
        bool hasTransform = false;
        ImmediateIndependent::TransformModel model;   // the RenderTransform (or the spinner rotation)
        RenderingBackend::Matrix3x2F render0;         // model evaluated at the snapshot
        std::vector<ImmediateIndependent::AnimationTrack> tracks;
        int opacityTrack = -1;                        // index into tracks
        float opacity0 = 1;
    };

    // Result of probing an element during the walk: when active, its opacity layer is pinned and recorded.
    struct IndependentProbe
    {
        bool active = false;
        RenderingBackend::RectF outerClip{};
        IndependentLayer layer;
    };

    // Per-island render-thread bookkeeping (guarded by IndependentState::mutex).
    struct IndependentIsland
    {
        std::vector<RenderingBackend::RectF> lastBounds;  // device px painted per layer by the last render-thread frame
        std::vector<RenderingBackend::RectF> extraDamage; // painted by the render thread since the last UI frame
        int64_t lastUiQpc = 0;                            // last UI-thread RenderIsland
        uint32_t frames = 0;
        bool paintedFinal = false;
    };

    struct AnimationBinding
    {
        KnownPropertyIndex property;
        CDoubleAnimation* animation;
    };

    struct IndependentState
    {
        std::recursive_mutex mutex;   // serializes painters, island state and display lists between the two threads
        std::condition_variable_any wake;
        HANDLE thread = nullptr;
        bool stop = false;
        bool running = false;         // a render-thread frame loop is wanted (some layer has a running track)
        uint32_t frameCounter = 0;
        HRESULT renderThreadLoss = S_OK; // device lost seen by the render thread; handled by the next UI frame

        // UI thread only: active double animations by resolved target (rebuilt per frame from the time manager).
        std::unordered_map<const CDependencyObject*, std::vector<AnimationBinding>> animations;

        // Device-lost simulation and recovery (UI thread).
        ImmediateIndependent::DeviceLostSimulation simulation;
        bool simulationParsed = false;
        uint32_t presentedFrames = 0;
        uint32_t lossCount = 0;
        bool recoveryPending = false;
        bool lastLossSimulated = false;
        int64_t lossQpc = 0;

        // Target changes (S20).
        uint32_t themeKey = UINT32_MAX;
        bool simulatingLoss = false;
    };
}
