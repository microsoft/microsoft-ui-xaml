// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RenderingBackendTests.h"
#include <RenderingBackend.h>

namespace RenderingBackendTests
{
    using namespace RenderingBackend;

    void SelectionTests::ParsesEveryModeName()
    {
        Mode mode{};
        VERIFY_IS_TRUE(TryParseMode(L"composition", mode));
        VERIFY_IS_TRUE(mode == Mode::Composition);
        VERIFY_IS_TRUE(TryParseMode(L"gdiplus", mode));
        VERIFY_IS_TRUE(mode == Mode::ImmediateGdiPlus);
        VERIFY_IS_TRUE(TryParseMode(L"d2d-hw", mode));
        VERIFY_IS_TRUE(mode == Mode::ImmediateD2DHardware);
        VERIFY_IS_TRUE(TryParseMode(L"d2d-sw", mode));
        VERIFY_IS_TRUE(mode == Mode::ImmediateD2DSoftware);
    }

    void SelectionTests::ParsingIsCaseInsensitive()
    {
        Mode mode{};
        VERIFY_IS_TRUE(TryParseMode(L"GdiPlus", mode));
        VERIFY_IS_TRUE(mode == Mode::ImmediateGdiPlus);
        VERIFY_IS_TRUE(TryParseMode(L"D2D-HW", mode));
        VERIFY_IS_TRUE(mode == Mode::ImmediateD2DHardware);
    }

    void SelectionTests::RejectsUnknownOrEmptyText()
    {
        Mode mode = Mode::ImmediateD2DSoftware;
        VERIFY_IS_FALSE(TryParseMode(nullptr, mode));
        VERIFY_IS_FALSE(TryParseMode(L"", mode));
        VERIFY_IS_FALSE(TryParseMode(L"gdi", mode));
        VERIFY_IS_FALSE(TryParseMode(L"d2d", mode));
        VERIFY_IS_FALSE(TryParseMode(L" d2d-hw", mode));
        VERIFY_IS_TRUE(mode == Mode::ImmediateD2DSoftware);
    }

    void SelectionTests::ModeNamesRoundTrip()
    {
        for (uint32_t index = 0; index < ModeCount; ++index)
        {
            const auto expected = static_cast<Mode>(index);
            Mode parsed{};
            VERIFY_IS_TRUE(TryParseMode(ModeName(expected), parsed));
            VERIFY_IS_TRUE(parsed == expected);
        }
        VERIFY_IS_NOT_NULL(FallbackReasonName(FallbackReason::None));
        VERIFY_IS_NOT_NULL(FallbackReasonName(FallbackReason::NotImplemented));
        VERIFY_IS_NOT_NULL(FallbackReasonName(static_cast<FallbackReason>(UINT32_MAX)));
    }

    void SelectionTests::UnsetRequestSelectsComposition()
    {
        const auto selection = Resolve(nullptr, ModeBit(Mode::Composition));
        VERIFY_IS_TRUE(selection.requested == Mode::Composition);
        VERIFY_IS_TRUE(selection.effective == Mode::Composition);
        VERIFY_IS_TRUE(selection.reason == FallbackReason::None);
    }

    void SelectionTests::InvalidRequestFallsBackToComposition()
    {
        const auto selection = Resolve(L"vulkan", AllModesMask);
        VERIFY_IS_TRUE(selection.requested == Mode::Composition);
        VERIFY_IS_TRUE(selection.effective == Mode::Composition);
        VERIFY_IS_TRUE(selection.reason == FallbackReason::InvalidRequest);
    }

    void SelectionTests::UnavailableModeFallsBackWithReason()
    {
        const auto selection = Resolve(L"d2d-sw", ModeBit(Mode::Composition));
        VERIFY_IS_TRUE(selection.requested == Mode::ImmediateD2DSoftware);
        VERIFY_IS_TRUE(selection.effective == Mode::Composition);
        VERIFY_IS_TRUE(selection.reason == FallbackReason::NotImplemented);
    }

    void SelectionTests::DeviceUnavailableModeFallsBackWithReason()
    {
        const uint32_t available = ModeBit(Mode::Composition) | ModeBit(Mode::ImmediateD2DSoftware);
        auto selection = Resolve(L"d2d-hw", available, ModeBit(Mode::ImmediateD2DHardware));
        VERIFY_IS_TRUE(selection.requested == Mode::ImmediateD2DHardware);
        VERIFY_IS_TRUE(selection.effective == Mode::Composition);
        VERIFY_IS_TRUE(selection.reason == FallbackReason::DeviceUnavailable);

        // The device-unavailable mask only affects modes that are not available.
        selection = Resolve(L"d2d-sw", available, ModeBit(Mode::ImmediateD2DSoftware));
        VERIFY_IS_TRUE(selection.effective == Mode::ImmediateD2DSoftware);
        VERIFY_IS_TRUE(selection.reason == FallbackReason::None);

        selection = Resolve(L"gdiplus", available, ModeBit(Mode::ImmediateD2DHardware));
        VERIFY_IS_TRUE(selection.effective == Mode::Composition);
        VERIFY_IS_TRUE(selection.reason == FallbackReason::NotImplemented);
    }

    namespace
    {
        uint32_t s_probeCalls = 0;

        FallbackReason FakeProbe(Mode mode) noexcept
        {
            ++s_probeCalls;
            switch (mode)
            {
                case Mode::ImmediateGdiPlus: return FallbackReason::None;
                case Mode::ImmediateD2DHardware: return FallbackReason::DeviceUnavailable;
                default: return FallbackReason::NotImplemented;
            }
        }
    }

    void SelectionTests::ProbeDeterminesAvailableModes()
    {
        SetImmediateModeProbe(&FakeProbe);
        auto restore = wil::scope_exit([] { SetImmediateModeProbe(nullptr); });

        s_probeCalls = 0;
        const uint32_t modes = GetAvailableModes();
        VERIFY_ARE_EQUAL(ModeBit(Mode::Composition) | ModeBit(Mode::ImmediateGdiPlus), modes);
        VERIFY_ARE_EQUAL(3u, s_probeCalls);

        // Results are cached: probing creates devices, so it runs at most once per mode.
        VERIFY_ARE_EQUAL(modes, GetAvailableModes());
        VERIFY_ARE_EQUAL(3u, s_probeCalls);
    }

    void SelectionTests::AvailableModeIsHonored()
    {
        const auto selection = Resolve(L"gdiplus", ModeBit(Mode::Composition) | ModeBit(Mode::ImmediateGdiPlus));
        VERIFY_IS_TRUE(selection.requested == Mode::ImmediateGdiPlus);
        VERIFY_IS_TRUE(selection.effective == Mode::ImmediateGdiPlus);
        VERIFY_IS_TRUE(selection.reason == FallbackReason::None);
    }

    void SelectionTests::CompositionIsAlwaysAvailable()
    {
        VERIFY_IS_TRUE((GetAvailableModes() & ModeBit(Mode::Composition)) != 0);
        const auto selection = Resolve(L"composition", 0);
        VERIFY_IS_TRUE(selection.effective == Mode::Composition);
        VERIFY_IS_TRUE(selection.reason == FallbackReason::None);
    }

    void SelectionTests::ReadsEnvironmentVariable()
    {
        wchar_t previous[64]{};
        const auto previousLength = GetEnvironmentVariableW(EnvironmentVariableName, previous, ARRAYSIZE(previous));
        auto restore = wil::scope_exit([&]
        {
            SetEnvironmentVariableW(EnvironmentVariableName, previousLength ? previous : nullptr);
        });

        VERIFY_WIN32_BOOL_SUCCEEDED(SetEnvironmentVariableW(EnvironmentVariableName, L"d2d-hw"));
        auto selection = ReadSelectionFromEnvironment(ModeBit(Mode::Composition));
        VERIFY_IS_TRUE(selection.requested == Mode::ImmediateD2DHardware);
        VERIFY_IS_TRUE(selection.effective == Mode::Composition);
        VERIFY_IS_TRUE(selection.reason == FallbackReason::NotImplemented);

        VERIFY_WIN32_BOOL_SUCCEEDED(SetEnvironmentVariableW(EnvironmentVariableName, nullptr));
        selection = ReadSelectionFromEnvironment(AllModesMask);
        VERIFY_IS_TRUE(selection.requested == Mode::Composition);
        VERIFY_IS_TRUE(selection.reason == FallbackReason::None);
    }

    void SelectionTests::ProviderIdentityIsStable()
    {
        // EventSource-style name hash of "Microsoft-WinUI-RenderingBackend"; the WPR profile and analyzer depend on it.
        constexpr GUID expected = { 0xd18ce99c, 0xdd66, 0x5390, { 0x18, 0xda, 0x06, 0x85, 0x46, 0x73, 0x98, 0x01 } };
        VERIFY_IS_TRUE(expected == GetProviderId());
    }
}
