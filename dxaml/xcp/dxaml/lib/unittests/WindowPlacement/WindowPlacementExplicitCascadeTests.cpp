// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <Windows.h>
#include <WexTestClass.h>
#include <vector>
#include "WindowPlacementApplication.h"
#include "WindowPlacementAdapter.h"
#include "WindowPlacementPhysicalCoordinates.h"

using namespace DirectUI::WindowPlacementPersistence;

namespace
{
    class TestWindow
    {
    public:
        TestWindow()
        {
            WNDCLASSW wc{};
            wc.lpfnWndProc = ::DefWindowProcW;
            wc.hInstance = ::GetModuleHandleW(nullptr);
            wc.lpszClassName = L"WindowPlacementExplicitCascadeTests";
            ::RegisterClassW(&wc);
            Handle = ::CreateWindowExW(0, wc.lpszClassName, L"Explicit cascade",
                WS_OVERLAPPEDWINDOW, 120, 140, 640, 480,
                nullptr, nullptr, wc.hInstance, nullptr);
            VERIFY_IS_NOT_NULL(Handle);
        }
        ~TestWindow() { if (::IsWindow(Handle)) ::DestroyWindow(Handle); }

        HWND Handle{};
    };

    // Records what the application layer asks for and replies with a scripted peer.
    struct Anchor
    {
        inline static int Calls{};
        inline static std::u16string RequestedDeviceName{};
        inline static bool Found{true};
        inline static Snapshot Peer{};

        static void Reset(const Snapshot& peer)
        {
            Calls = 0;
            RequestedDeviceName.clear();
            Found = true;
            Peer = peer;
        }

        static bool TryGet(void* context, const std::u16string& targetDeviceName, Snapshot& peer)
        {
            ++Calls;
            RequestedDeviceName = targetDeviceName;
            if (context) *static_cast<int*>(context) += 1;
            if (!Found) return false;
            peer = Peer;
            return true;
        }

        static PeerAnchorSource Source(void* context = nullptr)
        {
            PeerAnchorSource source{};
            source.TryGetAnchor = TryGet;
            source.Context = context;
            return source;
        }
    };

    Snapshot Source(HWND hwnd)
    {
        WindowPlacementCapture capture;
        VERIFY_IS_TRUE(capture.TryCapture(hwnd, PresenterKind::Overlapped));
        auto source = *capture.TryGetPlacement();
        source.NormalRect = {source.WorkArea.X + 40, source.WorkArea.Y + 40, 440, 340};
        return source;
    }

    Snapshot Peer(const char16_t* deviceName)
    {
        Snapshot peer{};
        peer.WorkArea = {0, 0, 1920, 1040};
        peer.NormalRect = {40, 40, 440, 340};
        peer.Dpi = 96;
        peer.DisplayDeviceName = deviceName;
        return peer;
    }

    // Scripts a Z-ordered candidate list for the peer walk, topmost first.
    struct CandidateList
    {
        std::vector<Snapshot> Candidates;
        size_t Served{};

        static bool TryGetNext(void* context, Snapshot& candidate)
        {
            auto* self = static_cast<CandidateList*>(context);
            if (self->Served >= self->Candidates.size()) return false;
            candidate = self->Candidates[self->Served++];
            return true;
        }

        PlacementPeerCandidateSource Source()
        {
            PlacementPeerCandidateSource source{};
            source.TryGetNext = TryGetNext;
            source.Context = this;
            return source;
        }
    };

    bool Compute(
        HWND hwnd,
        const Snapshot& source,
        bool cascadePermitted,
        PlacementSource sourceKind,
        const PeerAnchorSource& anchor,
        NativeRequest& request)
    {
        NativeApplyOptions options{};
        options.KeepHidden = true;
        return TryComputeNativeRequest(
            hwnd, source, PlacementReason::Default, options,
            sourceKind, cascadePermitted, anchor, request);
    }
}

class WindowPlacementExplicitCascadeTests
{
public:
    TEST_CLASS(WindowPlacementExplicitCascadeTests);

    // The anchor is only requested once the target monitor is known, so the callback
    // must receive that monitor and the result must keep the explicit size.
    TEST_METHOD(ExplicitCascadeUsesPeerPositionOnTargetMonitorAndKeepsExplicitSize)
    {
        PhysicalCoordinateScope coordinates;
        TestWindow window;
        const auto source = Source(window.Handle);

        NativeRequest uncascaded{};
        Anchor::Reset(source);
        VERIFY_IS_TRUE(Compute(
            window.Handle, source, false, PlacementSource::Explicit, {}, uncascaded));
        VERIFY_ARE_EQUAL(0, Anchor::Calls);

        auto peer = source;
        peer.NormalRect.X += 120;
        peer.NormalRect.Y += 90;
        peer.NormalRect.Width += 60;
        peer.NormalRect.Height += 60;
        Anchor::Reset(peer);

        int contextHits = 0;
        NativeRequest cascaded{};
        VERIFY_IS_TRUE(Compute(
            window.Handle, source, true, PlacementSource::Explicit,
            Anchor::Source(&contextHits), cascaded));

        VERIFY_ARE_EQUAL(1, Anchor::Calls);
        VERIFY_ARE_EQUAL(1, contextHits);
        VERIFY_IS_FALSE(Anchor::RequestedDeviceName.empty());
        VERIFY_IS_TRUE(Anchor::RequestedDeviceName == cascaded.DeviceName);

        // Position moves off the peer; size, monitor, and state do not change.
        VERIFY_ARE_NOT_EQUAL(uncascaded.NormalRect.X, cascaded.NormalRect.X);
        VERIFY_IS_TRUE(cascaded.NormalRect.X > peer.NormalRect.X);
        VERIFY_IS_TRUE(cascaded.NormalRect.Y > peer.NormalRect.Y);
        VERIFY_ARE_EQUAL(uncascaded.NormalRect.Width, cascaded.NormalRect.Width);
        VERIFY_ARE_EQUAL(uncascaded.NormalRect.Height, cascaded.NormalRect.Height);
        VERIFY_IS_TRUE(uncascaded.DeviceName == cascaded.DeviceName);
        VERIFY_ARE_EQUAL(
            static_cast<int>(uncascaded.ShowCommand), static_cast<int>(cascaded.ShowCommand));
        VERIFY_ARE_EQUAL(uncascaded.ArrangeRect.has_value(), cascaded.ArrangeRect.has_value());
    }

    // No peer is the common case. The explicit result must survive untouched.
    TEST_METHOD(MissingOrInvalidAnchorLeavesExplicitResultUncascaded)
    {
        PhysicalCoordinateScope coordinates;
        TestWindow window;
        const auto source = Source(window.Handle);

        NativeRequest expected{};
        VERIFY_IS_TRUE(Compute(
            window.Handle, source, true, PlacementSource::Explicit, {}, expected));

        auto peer = source;
        peer.NormalRect.X += 200;

        // Lookup failure.
        Anchor::Reset(peer);
        Anchor::Found = false;
        NativeRequest notFound{};
        VERIFY_IS_TRUE(Compute(
            window.Handle, source, true, PlacementSource::Explicit, Anchor::Source(), notFound));
        VERIFY_ARE_EQUAL(1, Anchor::Calls);
        VERIFY_ARE_EQUAL(expected.NormalRect.X, notFound.NormalRect.X);
        VERIFY_ARE_EQUAL(expected.NormalRect.Y, notFound.NormalRect.Y);

        // Peer snapshot that does not pass validation.
        auto invalid = peer;
        invalid.NormalRect.Width = 0;
        VERIFY_IS_FALSE(IsValid(invalid));
        Anchor::Reset(invalid);
        NativeRequest ignored{};
        VERIFY_IS_TRUE(Compute(
            window.Handle, source, true, PlacementSource::Explicit, Anchor::Source(), ignored));
        VERIFY_ARE_EQUAL(1, Anchor::Calls);
        VERIFY_ARE_EQUAL(expected.NormalRect.X, ignored.NormalRect.X);
        VERIFY_ARE_EQUAL(expected.NormalRect.Y, ignored.NormalRect.Y);
    }

    // Saved and peer sources take the ordinary cascade path, which never needs an anchor.
    TEST_METHOD(NonExplicitSourcesNeverRequestAnAnchor)
    {
        PhysicalCoordinateScope coordinates;
        TestWindow window;
        const auto source = Source(window.Handle);

        for (auto kind : {PlacementSource::Saved, PlacementSource::Peer})
        {
            Anchor::Reset(source);
            NativeRequest request{};
            VERIFY_IS_TRUE(Compute(
                window.Handle, source, true, kind, Anchor::Source(), request));
            VERIFY_ARE_EQUAL(0, Anchor::Calls);
        }
    }

    // The behavior gate lives in TryApplyPlacement. Only Enabled reaches the anchor.
    TEST_METHOD(OnlyEnabledCascadeBehaviorReachesTheAnchorForExplicitPlacement)
    {
        PhysicalCoordinateScope coordinates;
        TestWindow window;
        const auto source = Source(window.Handle);
        WindowPlacementCapture cache;

        PlacementPass pass{};
        pass.PlacementId = u"explicit-cascade";
        pass.AutomaticPersistenceOptIn = true;
        pass.KeepHidden = true;
        pass.Request.Placement = source;

        auto run = [&](CascadeBehavior behavior, PlacementReason reason)
        {
            pass.Request.CascadeBehavior = static_cast<int>(behavior);
            pass.Request.Reason = static_cast<int>(reason);
            Anchor::Reset(source);
            TryApplyPlacement(
                window.Handle, pass, cache, ReadPersistPlacement, Anchor::Source());
            return Anchor::Calls;
        };

        VERIFY_ARE_EQUAL(0, run(CascadeBehavior::Automatic, PlacementReason::Default));
        VERIFY_ARE_EQUAL(0, run(CascadeBehavior::Disabled, PlacementReason::Default));
        VERIFY_ARE_EQUAL(0, run(CascadeBehavior::Enabled, PlacementReason::ApplicationRestart));
        VERIFY_ARE_EQUAL(1, run(CascadeBehavior::Enabled, PlacementReason::Default));

        // The application never writes back to the caller's placement object.
        VERIFY_ARE_EQUAL(source.NormalRect.X, pass.Request.Placement->NormalRect.X);
        VERIFY_ARE_EQUAL(source.NormalRect.Y, pass.Request.Placement->NormalRect.Y);
        VERIFY_ARE_EQUAL(source.NormalRect.Width, pass.Request.Placement->NormalRect.Width);
        VERIFY_ARE_EQUAL(source.NormalRect.Height, pass.Request.Placement->NormalRect.Height);
    }

    // The monitor filter used by the peer walk. These run on a single display because
    // the rule is a snapshot comparison, not a topology query.
    TEST_METHOD(PeerAcceptanceFiltersByMonitorAndValidity)
    {
        const std::u16string target = u"\\\\.\\DISPLAY1";
        const std::u16string other = u"\\\\.\\DISPLAY2";

        VERIFY_IS_TRUE(IsAcceptablePlacementPeer(Peer(u"\\\\.\\DISPLAY1"), target));

        // A peer on another monitor is never an anchor for this target.
        VERIFY_IS_FALSE(IsAcceptablePlacementPeer(Peer(u"\\\\.\\DISPLAY1"), other));

        // Device names arrive from different Win32 calls, so case must not matter.
        VERIFY_IS_TRUE(IsAcceptablePlacementPeer(Peer(u"\\\\.\\display1"), target));

        // An empty requirement is the saved/peer path, which accepts any monitor.
        VERIFY_IS_TRUE(IsAcceptablePlacementPeer(Peer(u"\\\\.\\DISPLAY2"), {}));

        // A peer with no captured monitor cannot satisfy a specific requirement.
        VERIFY_IS_FALSE(IsAcceptablePlacementPeer(Peer(u""), target));

        auto invalid = Peer(u"\\\\.\\DISPLAY1");
        invalid.NormalRect.Width = 0;
        VERIFY_IS_FALSE(IsAcceptablePlacementPeer(invalid, target));
        VERIFY_IS_FALSE(IsAcceptablePlacementPeer(invalid, {}));
    }

    // The walk must not stop at the topmost marked window. A candidate on another
    // monitor is skipped, and a lower candidate on the target monitor still wins.
    TEST_METHOD(PeerWalkSkipsOffMonitorCandidatesAndKeepsWalking)
    {
        const std::u16string target = u"\\\\.\\DISPLAY2";

        auto invalid = Peer(u"\\\\.\\DISPLAY2");
        invalid.NormalRect.Width = 0;

        CandidateList candidates{{
            Peer(u"\\\\.\\DISPLAY1"),     // topmost, wrong monitor
            invalid,                       // right monitor, unusable snapshot
            Peer(u"\\\\.\\display2"),      // right monitor, different case
            Peer(u"\\\\.\\DISPLAY1")}};    // never reached

        Snapshot peer{};
        VERIFY_IS_TRUE(TryFindAcceptablePlacementPeer(candidates.Source(), target, peer));
        VERIFY_ARE_EQUAL(std::u16string(u"\\\\.\\display2"), peer.DisplayDeviceName);

        // The walk stops as soon as a candidate is accepted.
        VERIFY_ARE_EQUAL(3u, candidates.Served);

        // With no acceptable candidate the walk runs to exhaustion and reports failure.
        CandidateList none{{Peer(u"\\\\.\\DISPLAY1"), invalid}};
        Snapshot unused{};
        VERIFY_IS_FALSE(TryFindAcceptablePlacementPeer(none.Source(), target, unused));
        VERIFY_ARE_EQUAL(2u, none.Served);

        // An empty source is not a crash, just no peer.
        Snapshot ignored{};
        VERIFY_IS_FALSE(TryFindAcceptablePlacementPeer({}, target, ignored));
    }
};
