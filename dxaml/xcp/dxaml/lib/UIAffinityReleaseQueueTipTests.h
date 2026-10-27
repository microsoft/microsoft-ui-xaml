// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#define wil_details_GetKernelBaseModuleHandle() GetModuleHandleW(L"kernelbase.dll")
#include <tip/tip.h>
#undef wil_details_GetKernelBaseModuleHandle
#include "WindowsAppSdk-ProductInfo.h"

TIP_declare_test(UIAffinityReleaseQueueCleanupTest, 64884230)
{
    TIP_set_data_version(1);
    TIP_set_metrics_enabled();

    enum class reason
    {
        batch_released = 1,
        cleanup_failed,
        incomplete_batch,
        cancelled
    };

    uint32_t componentMajor{ WINUI_RELEASE_MAJOR };
    uint32_t productMajor{ WINDOWSAPPSDK_RELEASE_MAJOR };
    uint32_t productMinor{ WINDOWSAPPSDK_RELEASE_MINOR };
    uint32_t componentBuild{ WINUI_BUILD_VERSION };
    uint8_t delayBucket{};
    uint8_t releaseCountBucket{};
    HRESULT failureHResult{ S_OK };

    TIP_require_flag_clear(reason::cleanup_failed);

    void evaluate()
    {
        TIP_fail_if_complete_not_called();
        TIP_fail_if(!state().is_flag_set(reason::batch_released), reason::incomplete_batch);
        TIP_succeed(reason::batch_released);
    }

    template<class Archive>
    void serialize(Archive& archive)
    {
        archive(TIP_value(failureHResult));
    }

    template<class Archive>
    unsigned char serialize_metrics(Archive& archive)
    {
        archive(TIP_value(componentMajor), TIP_value(productMajor), TIP_value(productMinor),
            TIP_value(componentBuild), TIP_value(delayBucket), TIP_value(releaseCountBucket));
        return delayBucket;
    }
};

namespace DirectUI
{
    struct ReleaseQueueCleanupTip
    {
        UIAffinityReleaseQueueCleanupTest test;

        ReleaseQueueCleanupTip(uint64_t queuedAt, size_t count)
            : m_queuedAt(queuedAt), m_count(count), m_remaining(count)
        {
            test.start();
        }

        ReleaseQueueCleanupTip(const ReleaseQueueCleanupTip&) = delete;
        ReleaseQueueCleanupTip& operator=(const ReleaseQueueCleanupTip&) = delete;

        ~ReleaseQueueCleanupTip()
        {
            test.complete_and_ignore(TIP_reason(UIAffinityReleaseQueueCleanupTest::reason::cancelled));
        }

        bool RecordRelease()
        {
            if (m_remaining != 0 && --m_remaining == 0)
            {
                RecordMetrics();
                test.set_flag(TIP_reason(UIAffinityReleaseQueueCleanupTest::reason::batch_released));
                test.complete();
                return true;
            }
            return false;
        }

        void Fail(HRESULT hr)
        {
            RecordMetrics();
            test.data()->failureHResult = hr;
            test.set_flag(TIP_reason(UIAffinityReleaseQueueCleanupTest::reason::cleanup_failed));
            test.complete();
        }

        static uint8_t DelayBucket(uint64_t milliseconds)
        {
            return milliseconds < 16 ? 0 : milliseconds < 100 ? 1 : milliseconds < 1000 ? 2 : 3;
        }

        static uint8_t CountBucket(size_t count)
        {
            return count <= 1 ? 0 : count <= 16 ? 1 : count <= 256 ? 2 : 3;
        }

    private:
        void RecordMetrics()
        {
            auto data = test.data();
            data->delayBucket = DelayBucket(GetTickCount64() - m_queuedAt);
            data->releaseCountBucket = CountBucket(m_count - m_remaining);
        }

        uint64_t m_queuedAt;
        size_t m_count;
        size_t m_remaining;
    };
}
