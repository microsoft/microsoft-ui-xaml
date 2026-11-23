// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// ---- transitions ----
// F10: ConnectedAnimation in immediate-rendered windows. CConnectedAnimation (components\animation) snapshots the source
// with the immediate capture path and evaluates the host visual's motion itself (ConnectedAnimationImmediateState); the
// ConnectedAnimationRoot paints each host here, in its z-order: the source snapshot, then the destination walked live
// through the host transform, under the host's inset clip. The source/destination are hidden in place meanwhile.
// Included at the end of ImmediateRenderer.cpp (uses its file-local display-list types).

#include <ConnectedAnimation.h>

namespace
{
    template <typename Fn>
    bool ForEachConnectedAnimation(_In_ CUIElement* element, Fn&& fn)
    {
        CConnectedAnimationService* service = element->GetContext()->GetConnectedAnimationServiceNoRef();
        if (service == nullptr)
        {
            return false;
        }
        for (const auto& animation : service->GetAnimations())
        {
            if (animation != nullptr && fn(animation.get()))
            {
                return true;
            }
        }
        return false;
    }
}

bool ImmediateRenderer::IsConnectedHidden(_In_ CUIElement* element) noexcept
{
    return ForEachConnectedAnimation(element, [element](CConnectedAnimation* animation)
    {
        return animation->IsHiddenByImmediateEmulation(element);
    });
}

bool ImmediateRenderer::IsConnectedEmulated(_In_ CUIElement* element) noexcept
{
    return ForEachConnectedAnimation(element, [element](CConnectedAnimation* animation)
    {
        return animation->IsEmulatedImmediately(element);
    });
}

void ImmediateRenderer::EmitConnectedAnimations(
    _In_ CUIElement* root,
    const WalkContext& parent,
    _Inout_ DisplayList& list)
{
    const int64_t now = ImmediateAnimations::NowMicroseconds();
    ForEachConnectedAnimation(root, [&](CConnectedAnimation* animation)
    {
        const ConnectedAnimationImmediateState& state = animation->GetImmediateState();
        if (!state.active || animation->GetRootNoRef() != root)
        {
            return false;
        }
        ++list.uncacheable;

        float progress = 0;
        if (state.running)
        {
            bool finished = false;
            progress = ImmediateAnimations::GetProgress(state.startUs, state.durationUs, now, &finished);
            if (!finished)
            {
                root->GetContext()->RequestAdditionalFrame(RequestFrameReason::AnimationTick);
            }
            ReportUnsupported(root, L"ConnectedAnimation cross-fade:approximated");
            if (state.approximated)
            {
                ReportUnsupported(root, L"ConnectedAnimation easing:approximated");
            }
            if (state.customComponents)
            {
                ReportUnsupported(root, L"ConnectedAnimation component animation:approximated");
            }
        }
        const ImmediateAnimations::ConnectedFrame frame = ImmediateAnimations::EvaluateConnected(state.params, state.curve, progress);

        // Host visual: Scale (about its origin) then Offset, in the ConnectedAnimationRoot's space.
        CMILMatrix host(true);
        host.SetM11(frame.scaleX);
        host.SetM22(frame.scaleY);
        host.SetDx(frame.offsetX);
        host.SetDy(frame.offsetY);
        host.Append(parent.world);

        const RectF hostClipLocal = {
            frame.insets[0], frame.insets[1],
            state.hostWidth - frame.insets[2], state.hostHeight - frame.insets[3] };
        if (IsEmptyRect(hostClipLocal))
        {
            return false;
        }
        WalkContext ctx = parent;
        ctx.scroller = -1;
        ctx.scrollerContent = false;
        ctx.offsetX = ctx.offsetY = 0;
        ctx.clip = IntersectRect(parent.clip, TransformRectBounds(host, hostClipLocal));
        ctx.contentClip = ctx.clip;
        if (IsEmptyRect(ctx.clip))
        {
            return false;
        }

        CUIElement* destination = state.running ? animation->GetDestinationElementNoRef() : nullptr;
        const bool drawDestination = destination != nullptr && (state.hasDestination || state.coordinated);
        const float weight = state.running ? frame.crossFade : 0.0f;
        float sourceOpacity = 1;
        float destinationOpacity = 0;
        if (state.coordinated)
        {
            destinationOpacity = weight; // the host fades in
        }
        else if (drawDestination)
        {
            destinationOpacity = weight;
            sourceOpacity = ImmediateAnimations::ConnectedSourceOpacity(weight);
        }
        else
        {
            sourceOpacity = 1 - weight;
        }

        Emitter emitter{ list, root, ctx };
        if (state.shadow && frame.liftZ > 0)
        {
            bool dark = false;
            CUIElement* themed = destination != nullptr ? destination : nullptr;
            CValue value;
            if (themed != nullptr && themed->OfTypeByIndex<KnownTypeIndex::FrameworkElement>()
                && SUCCEEDED(themed->GetValueByIndex(KnownPropertyIndex::FrameworkElement_ActualTheme, &value)))
            {
                dark = static_cast<DirectUI::ElementTheme>(value.AsEnum()) == DirectUI::ElementTheme::Dark;
            }
            ImmediateRenderingPolicy::ShadowLayer layers[2];
            const uint32_t count = ImmediateRenderingPolicy::GetThemeShadowLayers(frame.liftZ, dark, layers);
            for (uint32_t i = 0; i < count; ++i)
            {
                emitter.Shadow(hostClipLocal, 0, 0, layers[i], host, parent.clip);
            }
        }
        if (state.snapshot != nullptr && !state.coordinated)
        {
            const RectF content = FromXRect(state.sourceContent);
            const RectF crop = FromXRect(state.sourceCrop);
            // Captured once per animation and never written again; the island's bitmap cache keeps it alive while cached,
            // so its address is a stable key.
            BitmapRef snapshot;
            snapshot.surface = state.snapshot.get();
            snapshot.key = reinterpret_cast<uint64_t>(state.snapshot.get());
            emitter.Bitmap(snapshot, content, crop, sourceOpacity, host, ctx.clip, 0, 0);
        }
        if (drawDestination && destinationOpacity > 0 && state.destRect.Width > 0 && state.destRect.Height > 0)
        {
            // destination parent -> root, then destRect onto destContent (host space), then the host.
            CMILMatrix world(true);
            if (CUIElement* destinationParent = destination->GetUIElementAdjustedParentInternal(false))
            {
                xref_ptr<ITransformer> transformer;
                if (SUCCEEDED(destinationParent->TransformToRoot(transformer.ReleaseAndGetAddressOf())))
                {
                    world = transformer->Get2DMatrixIgnore3D();
                }
            }
            CMILMatrix toHost(true);
            const float kx = state.destContent.Width / state.destRect.Width;
            const float ky = state.destContent.Height / state.destRect.Height;
            toHost.SetM11(kx);
            toHost.SetM22(ky);
            toHost.SetDx(state.destContent.X - state.destRect.X * kx);
            toHost.SetDy(state.destContent.Y - state.destRect.Y * ky);
            world.Append(toHost);
            world.Append(host);
            ctx.world = world;

            const CUIElement* previousElement = m_connectedOverlayElement;
            const float previousOpacity = m_connectedOverlayOpacity;
            m_connectedOverlayElement = destination;
            m_connectedOverlayOpacity = destinationOpacity;
            Walk(destination, ctx, list);
            m_connectedOverlayElement = previousElement;
            m_connectedOverlayOpacity = previousOpacity;
        }
        return false;
    });
}
