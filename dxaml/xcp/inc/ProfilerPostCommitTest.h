// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
#pragma once
#if defined(WINUI_PROFILER_FRAME_TEST_HOOKS)
namespace WinUIProfiler::FrameTest
{
    constexpr HRESULT PostCommitFailure = static_cast<HRESULT>(0x80040505L);
    class PostCommitState
    {
    public:
        HRESULT Arm() noexcept
        {
            if (m_state != 0) return E_ILLEGAL_METHOD_CALL;
            m_state = 1;
            return S_OK;
        }
        HRESULT AfterCleanup(HRESULT result) noexcept
        {
            // Never replace a real cleanup failure or consume the arm on that path.
            if (FAILED(result) || m_state != 1) return result;
            m_state = 2;
            return PostCommitFailure;
        }
        unsigned State() const noexcept { return m_state; }
    private:
        unsigned m_state = 0; // 0: never armed, 1: armed, 2: consumed; no rearm.
    };
}
#endif
