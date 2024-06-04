// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <cstdint>

enum class QualifierFlags : std::uint8_t
{
    None            =   0x0,
    Width           =   0x1,
    Height          =   0x2,
    Extensible      =   0x4,
    Identifier      =   0x8
};

inline QualifierFlags operator|(QualifierFlags q1, QualifierFlags q2)
{
    return static_cast<QualifierFlags>(static_cast<std::uint8_t>(q1) | static_cast<std::uint8_t>(q2));
}

inline QualifierFlags operator&(QualifierFlags q1, QualifierFlags q2)
{
    return static_cast<QualifierFlags>(static_cast<std::uint8_t>(q1) & static_cast<std::uint8_t>(q2));
}

inline QualifierFlags operator~(QualifierFlags q1)
{
    return static_cast<QualifierFlags>(~static_cast<std::uint8_t>(q1));
}

inline bool operator!(QualifierFlags q1)
{
    return q1 == QualifierFlags::None;
}

inline bool operator==(const int q1, QualifierFlags q2)
{
    return static_cast<QualifierFlags>(q1) == q2;
}

inline bool operator!=(const int q1, QualifierFlags q2)
{
    return static_cast<QualifierFlags>(q1) == q2;
}
