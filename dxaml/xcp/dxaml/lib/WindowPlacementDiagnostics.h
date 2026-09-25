// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <windows.h>
#include <cstdint>

namespace DirectUI::WindowPlacementPersistence
{
    enum class PlacementOperation : uint8_t { Load, Save, Apply };
    enum class PlacementFailureCategory : uint8_t { None, Capture, Identity, Read, Decode, Apply, Write };

    struct PlacementFailure
    {
        PlacementFailureCategory Category{PlacementFailureCategory::None};
        // S_OK means there is no native error. Category still identifies the failure.
        HRESULT Error{S_OK};
    };

    // Intentionally accepts only fixed categories and a numeric error, never placement data.
    void ReportPlacementFailure(
        PlacementOperation operation, PlacementFailureCategory category, HRESULT error) noexcept;

    class PlacementFailureScope final
    {
    public:
        explicit PlacementFailureScope(PlacementOperation operation) noexcept : m_operation(operation) {}
        ~PlacementFailureScope() noexcept
        {
            if (m_failure.Category != PlacementFailureCategory::None)
            {
                ReportPlacementFailure(m_operation, m_failure.Category, m_failure.Error);
            }
        }
        void Record(PlacementFailureCategory category, HRESULT error = S_OK) noexcept
        {
            // A fallback failure must not obscure the original load failure.
            if (m_failure.Category == PlacementFailureCategory::None) m_failure = {category, error};
        }
        PlacementFailureScope(const PlacementFailureScope&) = delete;
        PlacementFailureScope& operator=(const PlacementFailureScope&) = delete;

    private:
        PlacementOperation m_operation;
        PlacementFailure m_failure;
    };

#if WINDOWPLACEMENT_TESTS
    using PlacementFailureObserver = void (*)(PlacementOperation, PlacementFailureCategory, HRESULT);
    PlacementFailureObserver SetPlacementFailureObserverForTesting(PlacementFailureObserver observer) noexcept;
#endif
}
