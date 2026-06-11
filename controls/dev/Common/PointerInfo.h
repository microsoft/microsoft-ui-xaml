// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Flags_Enum.h>

enum class PointerOverStatus
{
    None  = 0x00,
    Touch = 0x01,
    Pen   = 0x02,
    Mouse = 0x04
};

enum class PointerPressedStatus
{
    None             = 0x00,
    LeftMouseButton  = 0x01,
    RightMouseButton = 0x02,
    Pointer          = 0x04
};

template <>
struct is_flags_enum<PointerOverStatus>
{
    static constexpr bool value = true;
};

template <>
struct is_flags_enum<PointerPressedStatus>
{
    static constexpr bool value = true;
};

template<typename T>
class PointerInfo
{
public:
    /* Use this constructor if pointer capturing is added through
       CapturedPointer/SetCapturedPointer/m_capturedPointer members below.
    PointerInfo(const ITrackerHandleManager* owner) : m_owner(owner)
    {
    }
    */

    PointerInfo()
    {
    }

    ~PointerInfo()
    {
    }

    bool IsPointerOver() const
    {
        return m_pointerOverStatus != PointerOverStatus::None;
    }

    bool IsTouchPointerOver() const
    {
        return flags_enum::is_set(m_pointerOverStatus, PointerOverStatus::Touch);
    }

    bool IsPenPointerOver() const
    {
        return flags_enum::is_set(m_pointerOverStatus, PointerOverStatus::Pen);
    }

    bool IsMousePointerOver() const
    {
        return flags_enum::is_set(m_pointerOverStatus, PointerOverStatus::Mouse);
    }

    void SetIsTouchPointerOver()
    {
        m_pointerOverStatus = flags_enum::set(m_pointerOverStatus, PointerOverStatus::Touch);
    }

    void ResetIsTouchPointerOver()
    {
        m_pointerOverStatus = flags_enum::unset(m_pointerOverStatus, PointerOverStatus::Touch);
    }

    void SetIsPenPointerOver()
    {
        m_pointerOverStatus = flags_enum::set(m_pointerOverStatus, PointerOverStatus::Pen);
    }

    void ResetIsPenPointerOver()
    {
        m_pointerOverStatus = flags_enum::unset(m_pointerOverStatus, PointerOverStatus::Pen);
    }

    void SetIsMousePointerOver()
    {
        m_pointerOverStatus = flags_enum::set(m_pointerOverStatus, PointerOverStatus::Mouse);
    }

    void ResetIsMousePointerOver()
    {
        m_pointerOverStatus = flags_enum::unset(m_pointerOverStatus, PointerOverStatus::Mouse);
    }

    bool IsPressed() const
    {
        return m_pointerPressedStatus != PointerPressedStatus::None;
    }

    bool IsMouseButtonPressed(bool isForLeftMouseButton) const
    {
        return isForLeftMouseButton ? flags_enum::is_set(m_pointerPressedStatus, PointerPressedStatus::LeftMouseButton) : flags_enum::is_set(m_pointerPressedStatus, PointerPressedStatus::RightMouseButton);
    }

    void SetIsMouseButtonPressed(bool isForLeftMouseButton)
    {
        if (isForLeftMouseButton)
        {
            m_pointerPressedStatus = flags_enum::set(m_pointerPressedStatus, PointerPressedStatus::LeftMouseButton);
        }
        else
        {
            m_pointerPressedStatus = flags_enum::set(m_pointerPressedStatus, PointerPressedStatus::RightMouseButton);
        }
    }

    void ResetIsMouseButtonPressed(bool isForLeftMouseButton)
    {
        if (isForLeftMouseButton)
        {
            m_pointerPressedStatus = flags_enum::unset(m_pointerPressedStatus, PointerPressedStatus::LeftMouseButton);
        }
        else
        {
            m_pointerPressedStatus = flags_enum::unset(m_pointerPressedStatus, PointerPressedStatus::RightMouseButton);
        }
    }

    void SetPointerPressed()
    {
        m_pointerPressedStatus = flags_enum::set(m_pointerPressedStatus, PointerPressedStatus::Pointer);
    }

    void ResetPointerPressed()
    {
        m_pointerPressedStatus = flags_enum::unset(m_pointerPressedStatus, PointerPressedStatus::Pointer);
    }

    bool IsTrackingPointer() const
    {
        return m_trackedPointerId != 0;
    }

    bool IsPointerIdTracked(uint32_t pointerId) const
    {
        return m_trackedPointerId == pointerId;
    }

    void TrackPointerId(uint32_t pointerId)
    {
        MUX_ASSERT(m_trackedPointerId == 0);

        m_trackedPointerId = pointerId;
    }

    void ResetTrackedPointerId()
    {
        m_trackedPointerId = 0;
    }

    void ResetAll()
    {
        m_trackedPointerId = 0;
        m_pointerOverStatus = PointerOverStatus::None;
        m_pointerPressedStatus = PointerPressedStatus::None;
    }

    /* Uncomment when pointer capturing becomes necessary.
    winrt::Pointer CapturedPointer() const
    {
        return m_capturedPointer.get();
    }

    void SetCapturedPointer(const winrt::Pointer& pointer)
    {
        m_capturedPointer = pointer;
    }
    */

private:
    //const ITrackerHandleManager* m_owner;
    //tracker_ref<winrt::Pointer> m_capturedPointer{ m_owner };
    uint32_t m_trackedPointerId{ 0 };
    PointerOverStatus m_pointerOverStatus{ PointerOverStatus::None };
    PointerPressedStatus m_pointerPressedStatus{ PointerPressedStatus::None };
};
