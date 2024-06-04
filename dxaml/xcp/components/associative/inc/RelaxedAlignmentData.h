// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

template <typename ExternalType, size_t Align>
class RelaxedAlignmentData
{
    static_assert(Align == 1 || Align == 2 || Align == 4 || Align == 8, "Alignment value not supported.");
    static_assert(sizeof(ExternalType) >= Align, "Size of stored type needs to be at least as large as alignment.");

    template <size_t Unit> struct UnitSelector {};

    template <>
    struct UnitSelector<1>
    {
        using Type = uint8_t;

        template<size_t index>
        static uint8_t GetByte(
            const Type& src)
        {
            static_assert(index < sizeof(Type), "Index out of bounds.");
            return src;
        }
    };

    template <>
    struct UnitSelector<2>
    {
        using Type = uint16_t;
    };

    template <>
    struct UnitSelector<4>
    {
        using Type = uint32_t;
    };

    template <>
    struct UnitSelector<8>
    {
        using Type = uint64_t;
    };

    template <size_t Align, size_t Repeat> struct RepeatSelector {};

    template <size_t Align>
    struct RepeatSelector<Align, 1>
    {
        using UnitType = typename UnitSelector<Align>::Type;
        using StorageType = UnitType;

        static void Set(
            const ExternalType& src,
            StorageType& dest)
        {
            dest = (StorageType)src;
        }

        static ExternalType Get(
            const StorageType& src)
        {
            return (ExternalType)src;
        }
    };

    template <size_t Align>
    struct RepeatSelector<Align, 2>
    {
        using UnitType = typename UnitSelector<Align>::Type;
        using StorageType = UnitType[2];

        static void Set(
            const ExternalType& src,
            StorageType& dest)
        {
            const UnitType* srcPtr = reinterpret_cast<const UnitType*>(&src);
            dest[0] = srcPtr[0];
            dest[1] = srcPtr[1];
        }

        static ExternalType Get(
            const StorageType& src)
        {
            ExternalType dest;
            UnitType* destPtr = reinterpret_cast<UnitType*>(&dest);
            destPtr[0] = src[0];
            destPtr[1] = src[1];
            return dest;
        }
    };

    template <size_t Align>
    struct RepeatSelector<Align, 4>
    {
        using UnitType = typename UnitSelector<Align>::Type;
        using StorageType = UnitType[4];

        static void Set(
            const ExternalType& src,
            StorageType& dest)
        {
            const UnitType* srcPtr = reinterpret_cast<const UnitType*>(&src);
            dest[0] = srcPtr[0];
            dest[1] = srcPtr[1];
            dest[2] = srcPtr[2];
            dest[3] = srcPtr[3];
        }

        static ExternalType Get(
            const StorageType& src)
        {
            ExternalType dest;
            UnitType* destPtr = reinterpret_cast<UnitType*>(&dest);
            destPtr[0] = src[0];
            destPtr[1] = src[1];
            destPtr[2] = src[2];
            destPtr[3] = src[3];
            return dest;
        }
    };

    template <size_t Align>
    struct RepeatSelector<Align, 8>
    {
        using UnitType = typename UnitSelector<Align>::Type;
        using StorageType = UnitType[8];

        static void Set(
            const ExternalType& src,
            StorageType& dest)
        {
            const UnitType* srcPtr = reinterpret_cast<const UnitType*>(&src);
            dest[0] = srcPtr[0];
            dest[1] = srcPtr[1];
            dest[2] = srcPtr[2];
            dest[3] = srcPtr[3];
            dest[4] = srcPtr[4];
            dest[5] = srcPtr[5];
            dest[6] = srcPtr[6];
            dest[7] = srcPtr[7];
        }

        static ExternalType Get(
            const StorageType& src)
        {
            ExternalType dest;
            UnitType* destPtr = reinterpret_cast<UnitType*>(&dest);
            destPtr[0] = src[0];
            destPtr[1] = src[1];
            destPtr[2] = src[2];
            destPtr[3] = src[3];
            destPtr[4] = src[4];
            destPtr[5] = src[5];
            destPtr[6] = src[6];
            destPtr[7] = src[7];
            return dest;
        }
    };

    using Selector = RepeatSelector<Align, sizeof(ExternalType) / Align>;
    using StorageType = typename Selector::StorageType;

    StorageType m_value = {};

public:
    operator ExternalType() const
    {
        return Selector::Get(m_value);
    }

    RelaxedAlignmentData& operator=(
        const ExternalType& value)
    {
        Selector::Set(value, m_value);
        return *this;
    }

    template <uint16_t index>
    uint8_t GetByte() const
    {
        static_assert(index < sizeof(StorageType), "Index out of bounds.");
        return reinterpret_cast<const uint8_t*>(&m_value)[index];
    }
};