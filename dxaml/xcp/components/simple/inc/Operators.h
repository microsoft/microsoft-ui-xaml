// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <windows.foundation.numerics.h>
#include <windows.foundation.h>
#include <tuple>
#include <NamespaceAliases.h>

inline bool operator==(const wfn::Vector2& rhs, const wfn::Vector2& lhs)
{
    // TODO: take care of NaN
    return std::tie(rhs.X, rhs.Y) == std::tie(lhs.X, lhs.Y);
}

inline bool operator!=(const wfn::Vector2& rhs, const wfn::Vector2& lhs)
{
    return !(rhs == lhs);
}

inline bool operator==(const wfn::Vector3& rhs, const wfn::Vector3& lhs)
{
    // TODO: take care of NaN
    return std::tie(rhs.X, rhs.Y, rhs.Z) == std::tie(lhs.X, lhs.Y, lhs.Z);
}

inline bool operator!=(const wfn::Vector3& rhs, const wfn::Vector3& lhs)
{
    return !(rhs == lhs);
}

inline bool operator==(const wfn::Matrix3x2& rhs, const wfn::Matrix3x2& lhs)
{
    // TODO: take care of NaN
    return std::tie(rhs.M11, rhs.M12, rhs.M21, rhs.M22, rhs.M31, rhs.M32) == std::tie(lhs.M11, lhs.M12, lhs.M21, lhs.M22, lhs.M31, lhs.M32);
}

inline bool operator!=(const wfn::Matrix3x2& rhs, const wfn::Matrix3x2& lhs)
{
    return !(rhs == lhs);
}

inline bool operator==(const wfn::Matrix4x4& rhs, const wfn::Matrix4x4& lhs)
{
    // TODO: take care of NaN
    return std::tie(rhs.M11, rhs.M12, rhs.M13, rhs.M14, rhs.M21, rhs.M22, rhs.M23, rhs.M24, rhs.M31, rhs.M32, rhs.M33, rhs.M34, rhs.M41, rhs.M42, rhs.M43, rhs.M44) ==
           std::tie(lhs.M11, lhs.M12, lhs.M13, lhs.M14, lhs.M21, lhs.M22, lhs.M23, lhs.M24, lhs.M31, lhs.M32, lhs.M33, lhs.M34, lhs.M41, lhs.M42, lhs.M43, lhs.M44);
}

inline bool operator!=(const wfn::Matrix4x4& rhs, const wfn::Matrix4x4& lhs)
{
    return !(rhs == lhs);
}

inline bool operator==(const wf::TimeSpan& lhs, const wf::TimeSpan& rhs)
{
    return lhs.Duration == rhs.Duration;
}

inline bool operator!=(const wf::TimeSpan& rhs, const wf::TimeSpan& lhs)
{
    return rhs.Duration != lhs.Duration;
}

inline bool operator==(const wf::DateTime& lhs, const wf::DateTime& rhs)
{
    return lhs.UniversalTime == rhs.UniversalTime;
}

inline bool operator!=(const wf::DateTime& lhs, const wf::DateTime& rhs)
{
    return lhs.UniversalTime != rhs.UniversalTime;
}