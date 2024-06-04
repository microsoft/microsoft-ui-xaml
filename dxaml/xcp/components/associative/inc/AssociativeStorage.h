// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "RelaxedAlignmentData.h"
#include "StaticAssertFalse.h"
#include <type_traits>
#include <stdint.h>
#include <array>
#include <utility>
#include <memory>

namespace AssociativeStorage
{
    //
    // Associative storage provides a method for adding rarely set fields without
    // incurring heap cost on all instances.  The decision to use it needs to be
    // made on case-by-case basis, as it depends on how many fields can be
    // moved to storage and how many instances set it.  The number of fields is
    // important since there is cost for associative storage (pointer and a bit-field).
    // It should be offset by moving data out of instance.
    // Occupancy rate plays a role in similar way – if too many
    // instances set the value, the benefit of moving it to associative storage
    // diminishes and at a certain point heap usage will increase (e.g. paying for
    // overhead of associative storage in addition to data).  The runtime cost
    // for accessing data is relatively small.  Checking if data is set is a bit
    // comparison on instance.  Getting data is constant time address translation
    // and lookup.  Adding new field at runtime costs an allocation and moving
    // existing data over to a new location, so care must be taken to avoid multiple
    // reallocations by analyzing scenarios in which it occurs and using
    // LocalStorage::PreAllocate method.
    //
    // To add associative storage to a class:
    //
    // * Declare enum for each field stored.  There are a few requirements here.
    //   Fields need to be in decreasing alignment order to avoid misaligned access
    //   (this will be validated during compile-time).
    //   Add Sentinel enum after the last field enum value.
    //   Declare it in AssociativeStorage namespace (e.g. CDOAssociative.h).
    //
    // * For each field, specialize FieldInfo<EnumType, EnumType::field> struct,
    //   where StorageType is an alias for type of field.
    //   This also goes in AssociativeStorage namespace (e.g. CDOAssociative.h).
    //
    // * Specialize FieldRuntimeInfoArray<EnumType> in AssociativeStorage namespace.
    //   It declares array of structs containing pointers to constructors and destructors
    //   of each field (e.g. CDOAssociativeImpl.h).
    //   IMPORTANT: it needs to be in the same order as order of fields in enum!
    //
    // * NOTE: types stored in fields need to have a default and move constructors
    //   and a destructor (compiler generated is ok as well).
    //   Currently, for new objects only no arguments constructor is called.
    //
    // * For each field declare and define manipulation methods by adding
    //   ASSOCIATIVE_STORAGE_ACCESSORS_DECL macro in header file (e.g.CDependencyObject.h)
    //   and ASSOCIATIVE_STORAGE_ACCESSORS_IMPL macro in implementation file
    //   (e.g.DependencyObject.cpp).
    //
    // * Instantiate s_lookupTable in one translation unit (e.g.DependencyObject.cpp).
    //
    // * Declare private field (m_associativeStorage) for associative storage (e.g. CDependencyObject.h).
    //
    // * Change all references from using instance field to EnsureZZZStorage/GetZZZStorage.
    //   IMPORTANT: do not rely on address of field not changing – it will change if
    //   other fields are added or removed.
    //
    // * Use the following access patterns:
    //
    //   To set value of field :
    //      FieldType& field = EnsureFieldStorage();
    //      // use field...
    //
    //   To get value of field :
    //      const FieldType* field = GetFieldStorage();
    //
    //      if (field)
    //      {
    //          // use field...
    //      }
    //

    template <typename TFieldEnum, TFieldEnum field>    struct FieldInfo {};
    template <typename TFieldEnum>                      struct FieldRuntimeInfoArray {};

    // Pointers to methods used for initialization, destruction and moving objects in fields.

    struct FieldRuntimeInfo
    {
        using CtorFn        = void(*)(uint8_t*);
        using MoveCtorFn    = void(*)(uint8_t*, uint8_t*);
        using DtorFn        = void(*)(uint8_t*);

        size_t      m_size;
        CtorFn      m_ctorFn;
        MoveCtorFn  m_moveCtorFn;
        DtorFn      m_dtorFn;
    };

    // Convenience alias for declaring array of runtime infos.

    template <typename TFieldEnum>
    using FieldRuntimeInfoArrayType = std::array<FieldRuntimeInfo, static_cast<size_t>(TFieldEnum::Sentinel)>;

    namespace Detail
    {
        // Fast way to calculate log2 for powers of 2.  Specialized for arg widths to avoid unnecessary comparisons.

        template <typename T>
        inline uint16_t log2(T value)
        {
            static_assert_false("Function not specialized for this type.");
        }

        template <>
        inline uint16_t log2<uint8_t>(uint8_t value)
        {
            uint16_t result = 0;

            if (value & 0xF0)
            {
                value >>= 4;
                result |= 4;
            }

            if (value & 0xC)
            {
                value >>= 2;
                result |= 2;
            }

            if (value & 0x2)
            {
                value >>= 1;
                result |= 1;
            }

            return result;
        }

        template <>
        inline uint16_t log2<uint16_t>(uint16_t value)
        {
            uint16_t result = 0;

            if (value & 0xFF00)
            {
                value >>= 8;
                result |= 8;
            }

            if (value & 0xF0)
            {
                value >>= 4;
                result |= 4;
            }

            if (value & 0xC)
            {
                value >>= 2;
                result |= 2;
            }

            if (value & 0x2)
            {
                value >>= 1;
                result |= 1;
            }

            return result;
        }

        template <>
        inline uint16_t log2<uint32_t>(uint32_t value)
        {
            uint16_t result = 0;

            if (value & 0xFFFF0000)
            {
                value >>= 16;
                result |= 16;
            }

            if (value & 0xFF00)
            {
                value >>= 8;
                result |= 8;
            }

            if (value & 0xF0)
            {
                value >>= 4;
                result |= 4;
            }

            if (value & 0xC)
            {
                value >>= 2;
                result |= 2;
            }

            if (value & 0x2)
            {
                value >>= 1;
                result |= 1;
            }

            return result;
        }

        // Mostly compile-time integer log2.  For runtime use ones above - they will be faster.
        // Note: _log2(0) = 0, as opposed to -inf.

        constexpr size_t _log2(size_t value)
        {
            return ((value < 2) ? 0 : 1 + _log2(value / 2));
        }

        using IndexType     = uint16_t;

        // Enum addressed bit-field abstraction.

        template <typename TFieldEnum>
        class Bitfield
        {
        public:
            using NumericEnumType   = typename std::underlying_type<TFieldEnum>::type;
            using FieldEnum         = TFieldEnum;

            void Add(
                FieldEnum enumValue)
            {
                m_value = static_cast<NumericEnumType>(m_value) | GetBitImage(enumValue);
            }

            void Remove(
                FieldEnum enumValue)
            {
                m_value = static_cast<NumericEnumType>(m_value) & ~GetBitImage(enumValue);
            }

            bool Has(
                FieldEnum enumValue) const
            {
                return Has(m_value, enumValue);
            }

            template <IndexType index>
            uint8_t GetByte() const
            {
                return m_value.template GetByte<index>();
            }

            operator NumericEnumType() const
            {
                return m_value;
            }

            const Bitfield& operator|=(
                const Bitfield& other)
            {
                if (this != &other)
                {
                    m_value = static_cast<NumericEnumType>(m_value) | static_cast<NumericEnumType>(other);
                }

                return *this;
            }

            static bool Has(
                NumericEnumType value,
                FieldEnum enumValue)
            {
                return (value & GetBitImage(enumValue)) != 0;
            }

            class const_iterator
            {
            public:
                const_iterator(NumericEnumType value)
                    : m_state(value)
                {}

                const_iterator(const const_iterator& other)
                    : m_state(other.m_state)
                {}

                const_iterator& operator=(const const_iterator&) = default;

                bool operator==(const const_iterator& other)
                {
                    return m_state == other.m_state;
                }

                bool operator!=(const const_iterator& other)
                {
                    return m_state == other.m_state;
                }

                operator bool() const
                {
                    return m_state != 0;
                }

                NumericEnumType operator*() const
                {
                    NumericEnumType nextState = m_state & (m_state - 1);
                    return m_state ^ nextState;
                }

                const_iterator& operator++()
                {
                    m_state &= m_state - 1;
                    return *this;
                }

                const_iterator operator++(int)
                {
                    const_iterator tmp(*this);
                    operator++();
                    return tmp;
                }

            private:
                NumericEnumType m_state;
            };

            // Returns iterator for iterating over bits which are set.

            const_iterator begin() const
            {
                return const_iterator(m_value);
            }

            // Returns iterator for iterating over bits which are set.

            const_iterator cbegin() const
            {
                return const_iterator(m_value);
            }

            const_iterator end() const
            {
                return const_iterator(0);
            }

            const_iterator cend() const
            {
                return const_iterator(0);
            }

        private:
            static constexpr NumericEnumType GetBitImage(
                FieldEnum enumValue)
            {
                return (1 << static_cast<NumericEnumType>(enumValue));
            }

            RelaxedAlignmentData<NumericEnumType, 1> m_value;
        };

        template <typename TFieldEnum>
        inline Bitfield<TFieldEnum> operator| (
            const Bitfield<TFieldEnum>& lhs,
            const Bitfield<TFieldEnum>& rhs)
        {
            if (&lhs != &rhs)
            {
                Bitfield<TFieldEnum> result(lhs);
                result |= rhs;
                return result;
            }
            else
            {
                return Bitfield<TFieldEnum>();
            }
        }

        // Helper for compile-time field checks.

        template <typename TFieldEnum>
        struct FieldInfoHelper
        {
            static constexpr bool check_alignment()
            {
                return _check_alignment<static_cast<size_t>(TFieldEnum::Sentinel) - 1>();
            }

            static constexpr size_t get_size()
            {
                return _get_size<static_cast<size_t>(TFieldEnum::Sentinel) - 1>();
            }

        private:
            template <size_t index>
            static constexpr size_t get_alignemnt()
            {
                return alignof(typename FieldInfo<TFieldEnum, static_cast<TFieldEnum>(index)>::StorageType);
            }

            template <size_t index>
            static constexpr bool _check_alignment()
            {
                return (get_alignemnt<index>() <= get_alignemnt<index - 1>()) &&
                        _check_alignment<index - 1>();
            }

            template <>
            static constexpr bool _check_alignment<0>()
            {
                return true;
            }

            template <size_t index>
            static constexpr size_t get_size()
            {
                return sizeof(typename FieldInfo<TFieldEnum, static_cast<TFieldEnum>(index)>::StorageType);
            }

            template <size_t index>
            static constexpr size_t _get_size()
            {
                return get_size<index>() + _get_size<index - 1>();
            }

            template <>
            static constexpr size_t _get_size<0>()
            {
                return get_size<0>();
            }
        };

        // Helper class for providing fast lookups of offsets and sizes for fields in different layouts.

        template <typename TBitfield>
        class OffsetLookupTable
        {
        public:
            using Bitfield          = TBitfield;
            using FieldEnum         = typename TBitfield::FieldEnum;
            using NumericEnumType   = typename TBitfield::NumericEnumType;

        private:
            using FieldRuntimeInfoArray = FieldRuntimeInfoArray<FieldEnum>;

            template <size_t size>  struct OffsetTypeSelector       { static_assert(size == 0, "Specialize OffsetTypeSelector for required size."); };
            template <>             struct OffsetTypeSelector<1>    { using OffsetType = uint8_t; };
            template <>             struct OffsetTypeSelector<2>    { using OffsetType = uint16_t; };
            template <>             struct OffsetTypeSelector<3>    { using OffsetType = uint32_t; };
            template <>             struct OffsetTypeSelector<4>    { using OffsetType = uint32_t; };

        public:
            // Select offset type at compile-time based on maximum possible size of associative storage.
            // This has impact on size of lookup tables.
            using OffsetType        = typename OffsetTypeSelector<_log2(FieldInfoHelper<FieldEnum>::get_size()) / 8 + 1>::OffsetType;

            OffsetType GetOffset(
                const Bitfield& occupancy,
                NumericEnumType field) const
            {
                return m_table.GetOffset(
                    occupancy,
                    field);
            }

            OffsetType GetSize(
                const Bitfield& occupancy) const
            {
                return m_table.GetSize(
                    occupancy);
            }

        private:
            static_assert(sizeof(Bitfield) <= 4, "Only up to 4-byte bitfields supported");

            static constexpr size_t c_fullTableSize     = 256;
            static constexpr size_t c_partialTableSize  = (static_cast<size_t>(FieldEnum::Sentinel) < 8) ? 1 << (static_cast<size_t>(FieldEnum::Sentinel) % 8) : 256;

            using FullTableType     = std::array<OffsetType, c_fullTableSize>;
            using PartialTableType  = std::array<OffsetType, c_partialTableSize>;

            template <typename TTable>
            static void LookupTableInitHelper(
                TTable& table,
                size_t lsbIndex)
            {
                // Don't try to initialize if enum is larger than necessary, e.g. does not hold any values in this byte.

                if (lsbIndex >= FieldRuntimeInfoArray::fields.size())
                {
                    return;
                }

                uint32_t currentBitMask  = -1 << 1;
                uint32_t currentBitIndex = 0;
                size_t   currentSize     = 0;
                size_t   accumulatedSize = 0;

                table[0] = 0;

                for (uint32_t occupancyValue = 1; occupancyValue < table.size(); ++occupancyValue)
                {
                    // Check if occupancy combination value's highest order bit advanced.

                    if ((occupancyValue & currentBitMask) != 0)
                    {
                        // If so, advance currentBitMask and currentBitIndex.
                        currentBitMask <<= 1;
                        ++currentBitIndex;
                    }

                    // Size for current field.
                    currentSize = FieldRuntimeInfoArray::fields[currentBitIndex + lsbIndex].m_size;

                    // Look up sum of fields for this combination without highest bit on.
                    accumulatedSize = table[occupancyValue & (~currentBitMask >> 1)];

                    // Sanity check - right type should be chosen during compile time.  Make sure we are not going to overflow.
                    ASSERT(accumulatedSize + currentSize < static_cast<OffsetType>(-1));

                    // Update table with size value.
                    table[occupancyValue] = static_cast<OffsetType>(accumulatedSize + currentSize);
                }
            }

            // If we treat occupancy value as index into an array storing sizes of different layouts,
            // size and offset lookups become constant-time operation (as opposed linear to number of bits).
            // It would be practical for small number of bits (e.g. for 8-bits we need to store 256 elements),
            // but for larger types the space cost would be prohibitive.
            // Fortunately, since individual sizes are not interdependent, we can use occupancy value in 8-bit
            // chunks and just add lookups from tables corresponding to each chunk.
            // This way instead of storing 2^n entries, we can store just n/8*256.
            // Size lookup for occupancy value is just simple lookup indexed by occupancy value.
            // Offset lookup for occupancy value is a lookup of size for occupancy value without highest bit set (equivalent to size of everything before).

            template <size_t ByteCount> struct LookupTable {};

            // Lookup table for 8-bit values.

            template <>
            struct LookupTable<1>
            {
                LookupTable()
                {
                    LookupTableInitHelper(m_sizes0, 0);
                }

                OffsetType GetOffset(
                    const Bitfield& occupancy,
                    NumericEnumType field) const
                {
                    return m_sizes0[occupancy.template GetByte<0>() & ~(-1 << field)];
                }

                OffsetType GetSize(
                    const Bitfield& occupancy) const
                {
                    return m_sizes0[occupancy.template GetByte<0>()];
                }

                PartialTableType m_sizes0;
            };

            // Lookup table for 16-bit values.

            template <>
            struct LookupTable<2>
            {
                LookupTable()
                {
                    LookupTableInitHelper(m_sizes0, 0);
                    LookupTableInitHelper(m_sizes1, 8);
                }

                OffsetType GetOffset(
                    const Bitfield& occupancy,
                    NumericEnumType field) const
                {
                    if (field <= 8)
                    {
                        return m_sizes0[occupancy.template GetByte<0>() & ~(-1 << field)];
                    }
                    else
                    {
                        field -= 8;
                        return m_sizes0[occupancy.template GetByte<0>()] +
                               m_sizes1[occupancy.template GetByte<1>() & ~(-1 << field)];
                    }
                }

                OffsetType GetSize(
                    const Bitfield& occupancy) const
                {
                    return m_sizes0[occupancy.template GetByte<0>()] +
                           m_sizes1[occupancy.template GetByte<1>()];
                }

                FullTableType       m_sizes0;
                PartialTableType    m_sizes1;
            };

            // Lookup table for 32-bit values.

            template <>
            struct LookupTable<4>
            {
                LookupTable()
                {
                    LookupTableInitHelper(m_sizes0, 0);
                    LookupTableInitHelper(m_sizes1, 8);
                    LookupTableInitHelper(m_sizes2, 16);
                    LookupTableInitHelper(m_sizes3, 24);
                }

                OffsetType GetOffset(
                    const Bitfield& occupancy,
                    NumericEnumType field) const
                {
                    if (field <= 8)
                    {
                        return m_sizes0[occupancy.template GetByte<0>() & ~(-1 << field)];
                    }
                    else if (field <= 16)
                    {
                        field -= 8;
                        return m_sizes0[occupancy.template GetByte<0>()] +
                               m_sizes1[occupancy.template GetByte<1>() & ~(-1 << field)];
                    }
                    else if (field <= 24)
                    {
                        field -= 16;
                        return m_sizes0[occupancy.template GetByte<0>()] +
                               m_sizes1[occupancy.template GetByte<1>()] +
                               m_sizes2[occupancy.template GetByte<2>() & ~(-1 << field)];
                    }
                    else
                    {
                        field -= 24;
                        return m_sizes0[occupancy.template GetByte<0>()] +
                               m_sizes1[occupancy.template GetByte<1>()] +
                               m_sizes2[occupancy.template GetByte<2>()] +
                               m_sizes3[occupancy.template GetByte<3>() & ~(-1 << field)];
                    }
                }

                OffsetType GetSize(
                    const Bitfield& occupancy) const
                {
                    return m_sizes0[occupancy.template GetByte<0>()] +
                           m_sizes1[occupancy.template GetByte<1>()] +
                           m_sizes2[occupancy.template GetByte<2>()] +
                           m_sizes3[occupancy.template GetByte<3>()];
                }

                FullTableType       m_sizes0;
                FullTableType       m_sizes1;
                FullTableType       m_sizes2;
                PartialTableType    m_sizes3;
            };

            LookupTable<sizeof(Bitfield)> m_table;
        };

        // Helper wrapper which temporarily manages buffer memory and calls destructors of objects stored in buffer.

        template <typename TBitfield>
        struct BufferDtorGuard
        {
            using Bitfield              = TBitfield;
            using FieldEnum             = typename TBitfield::FieldEnum;
            using NumericEnumType       = typename TBitfield::NumericEnumType;
            using FieldRuntimeInfoArray = FieldRuntimeInfoArray<FieldEnum>;

            BufferDtorGuard() = delete;
            BufferDtorGuard(const BufferDtorGuard&) = delete;
            BufferDtorGuard(BufferDtorGuard&&) = delete;

            BufferDtorGuard& operator=(const BufferDtorGuard&) = delete;
            BufferDtorGuard& operator=(BufferDtorGuard&&) = delete;

            BufferDtorGuard(
                uint8_t* buffer,
                Bitfield occupancy)
                : m_buffer(buffer)
                , m_occupancy(occupancy)
            {}

            ~BufferDtorGuard()
            {
                if (m_buffer)
                {
                    std::unique_ptr<uint8_t[]> guard(m_buffer);
                    size_t offset = 0;

                    for (auto iter = m_occupancy.begin(); iter; ++iter)
                    {
                        const auto& field = FieldRuntimeInfoArray::fields[log2(*iter)];
                        field.m_dtorFn(&m_buffer[offset]);
                        offset += field.m_size;
                    }
                }
            }

            uint8_t* m_buffer;
            Bitfield m_occupancy;
        };

        // VariableStorageBlock manages memory and performs type-related operations.

        template <typename TBitfield>
        class VariableStorageBlock
        {
        public:
            using Bitfield              = TBitfield;
            using FieldEnum             = typename TBitfield::FieldEnum;
            using NumericEnumType       = typename TBitfield::NumericEnumType;
            using OffsetLookupTable     = OffsetLookupTable<TBitfield>;

        private:
            using FieldRuntimeInfoArray = FieldRuntimeInfoArray<FieldEnum>;
            using BufferDtorGuard       = BufferDtorGuard<Bitfield>;

        public:
            VariableStorageBlock() = default;

            void Destroy(
                const Bitfield& occupancy)
            {
                BufferDtorGuard guard(m_dataPtr, occupancy);
                m_dataPtr = nullptr;
            }

            uint8_t* GetFieldPtr(
                FieldEnum field,
                const Bitfield& occupancy)
            {
                return m_dataPtr + s_lookupTable.GetOffset(occupancy, static_cast<NumericEnumType>(field));
            }

            const uint8_t* GetFieldPtr(
                FieldEnum field,
                const Bitfield& occupancy) const
            {
                return const_cast<VariableStorageBlock*>(this)->GetFieldPtr(
                    field,
                    occupancy);
            }

            // Resize reallocates and calls constructors for new occupancy layout.  It does not move previously held content.

            void Resize(
                const Bitfield& oldOccupancy,
                const Bitfield& newOccupancy)
            {
                ASSERT(oldOccupancy != newOccupancy);

                BufferDtorGuard newGuard(Allocate(newOccupancy), Bitfield());

                // oldGuard will call dtors and releases memory
                BufferDtorGuard oldGuard(m_dataPtr, oldOccupancy);
                m_dataPtr = nullptr;

                // Call constructors first

                size_t offset = 0;

                for (auto iter = newOccupancy.begin(); iter; ++iter)
                {
                    NumericEnumType curr = *iter;
                    const auto& field = FieldRuntimeInfoArray::fields[log2(curr)];
                    field.m_ctorFn(&newGuard.m_buffer[offset]);
                    newGuard.m_occupancy.Add(static_cast<FieldEnum>(curr));
                    offset += field.m_size;
                }

                m_dataPtr = newGuard.m_buffer;
                newGuard.m_buffer = nullptr;
            }

            Bitfield Add(
                FieldEnum field,
                const Bitfield& oldOccupancy)
            {
                ASSERT(!oldOccupancy.Has(field));

                Bitfield newOccupancy = oldOccupancy;
                newOccupancy.Add(field);

                // Allocate memory for the new block

                BufferDtorGuard guard(Allocate(newOccupancy), Bitfield());

                // Call constructor first

                size_t constructedFieldOffset = s_lookupTable.GetOffset(newOccupancy, static_cast<IndexType>(field));
                FieldRuntimeInfoArray::fields[static_cast<IndexType>(field)].m_ctorFn(&guard.m_buffer[constructedFieldOffset]);
                guard.m_occupancy.Add(field);

                // Move existing fields over.  Move ctor cannot throw.

                uint8_t* oldData = m_dataPtr;

                if (oldOccupancy != 0)
                {
                    MoveFields(
                        guard.m_buffer,
                        oldData,
                        newOccupancy,
                        static_cast<IndexType>(field),
                        true);
                }

                // We've successfully constructed a new field and moved existing ones.
                // Swap guard's buffer pointer and occupancy with old one, so it can call destructors when we exit this scope.

                m_dataPtr = guard.m_buffer;
                guard.m_buffer = oldData;
                guard.m_occupancy = oldOccupancy;

                return newOccupancy;
            }

            Bitfield Remove(
                FieldEnum field,
                const Bitfield& oldOccupancy)
            {
                ASSERT(oldOccupancy.Has(field));

                Bitfield newOccupancy = oldOccupancy;
                newOccupancy.Remove(field);

                // Allocate memory for the new block

                BufferDtorGuard guard(Allocate(newOccupancy), Bitfield());

                // Move existing fields over.  Move ctor cannot throw.

                uint8_t* oldData = m_dataPtr;

                MoveFields(
                    guard.m_buffer,
                    oldData,
                    oldOccupancy,
                    static_cast<IndexType>(field),
                    false);

                // We've successfully moved existing fields.
                // Swap guard's buffer pointer and occupancy with old one, so it can call destructors when we exit this scope.

                m_dataPtr = guard.m_buffer;
                guard.m_buffer = oldData;
                guard.m_occupancy = oldOccupancy;

                return newOccupancy;
            }

        private:
            // Pointer to allocated block, stored in 16-bit chunks to prevent padding / changing container alignment requirements.
            RelaxedAlignmentData<uint8_t*, 2> m_dataPtr;

            static void MoveFields(
                uint8_t* newData,
                uint8_t* oldData,
                const Bitfield& occupancy,
                IndexType changedField,
                bool changeIsAdd)
            {
                size_t newOffset = 0;
                size_t oldOffset = 0;

                for (auto iter = occupancy.begin(); iter; ++iter)
                {
                    // Since newOccupancy differs from oldOccupancy by just one bit, we can calculate offsets
                    // for both the same way.  The diff is handled by the if statement.

                    IndexType fieldIndex = log2(*iter);

                    if (fieldIndex != changedField)
                    {
                        const auto& field = FieldRuntimeInfoArray::fields[fieldIndex];
                        field.m_moveCtorFn(&newData[newOffset], &oldData[oldOffset]);
                        newOffset += field.m_size;
                        oldOffset += field.m_size;
                    }
                    else
                    {
                        // Changed field has been handled, so just advance appropriate offset

                        if (changeIsAdd)
                        {
                            // add
                            newOffset += FieldRuntimeInfoArray::fields[changedField].m_size;
                        }
                        else
                        {
                            // remove
                            oldOffset += FieldRuntimeInfoArray::fields[changedField].m_size;
                        }
                    }
                }
            }

            static uint8_t* Allocate(
                const Bitfield& occupancy)
            {
                size_t size = s_lookupTable.GetSize(occupancy);

                if (size > 0)
                {
                    return new uint8_t[size];
                }
                else
                {
                    return nullptr;
                }
            }

            static OffsetLookupTable s_lookupTable;
        };
    }

    template <typename TFieldEnum>
    class LocalStorage
    {
        template <TFieldEnum field> using MyFieldInfo           = FieldInfo<TFieldEnum, field>;
        template <TFieldEnum field> using StorageTypeRef        = typename MyFieldInfo<field>::StorageType&;
        template <TFieldEnum field> using StorageTypePtr        = typename MyFieldInfo<field>::StorageType*;
        template <TFieldEnum field> using StorageTypeConstPtr   = const typename MyFieldInfo<field>::StorageType*;

        static_assert(Detail::FieldInfoHelper<TFieldEnum>::check_alignment(), "Types need to be stored in decreasing alignment order to guarantee proper alignment.");

    public:
        using Bitfield  = typename Detail::Bitfield<TFieldEnum>;
        using FieldEnum = TFieldEnum;

        ~LocalStorage()
        {
            m_dataBlock.Destroy(m_occupancy);
        }

        // Preallocates memory block with requested fields.
        // Note that existing data will be destroyed.

        void PreAllocate(
            const Bitfield& value)
        {
            if (value != m_occupancy)
            {
                m_dataBlock.Resize(
                    m_occupancy,
                    value);

                m_occupancy = value;
            }
        }

        // Adds a reference to a new field or an existing one.
        // !!! Do not store this reference !!!  If new fields are added or removed it will point to deallocated memory block.

        template <FieldEnum field>
        StorageTypeRef<field> Ensure()
        {
            if (!m_occupancy.Has(field))
            {
                m_occupancy = m_dataBlock.Add(
                    field,
                    m_occupancy);
            }

            return *reinterpret_cast<StorageTypePtr<field>>(m_dataBlock.GetFieldPtr(
                field,
                m_occupancy));
        }

        // Gets pointer to existing field or returns null.
        // !!! Do not store this reference !!!  If new fields are added or removed it will point to deallocated memory block.

        template <FieldEnum field>
        StorageTypePtr<field> Get()
        {
            if (m_occupancy.Has(field))
            {
                return reinterpret_cast<StorageTypePtr<field>>(m_dataBlock.GetFieldPtr(
                    field,
                    m_occupancy));
            }
            else
            {
                return nullptr;
            }
        }

        // Gets pointer to existing field or returns null.
        // !!! Do not store this reference !!!  If new fields are added or removed it will point to deallocated memory block.

        template <FieldEnum field>
        StorageTypeConstPtr<field> Get() const
        {
            if (m_occupancy.Has(field))
            {
                return reinterpret_cast<StorageTypeConstPtr<field>>(m_dataBlock.GetFieldPtr(
                    field,
                    m_occupancy));
            }
            else
            {
                return nullptr;
            }
        }

        // Removes field if it exists.
        // For performance reasons it's better not to call this method but rather set field to default (empty) value and
        // perform destruction when entire object goes out of scope.  This method will cause reallocation, while destruction will not.

        template <FieldEnum field>
        void Remove()
        {
            if (m_occupancy.Has(field))
            {
                m_occupancy = m_dataBlock.Remove(
                    field,
                    m_occupancy);
            }
        }

    private:
        Detail::VariableStorageBlock<Bitfield> m_dataBlock;
        Bitfield m_occupancy;
    };

    namespace Detail
    {
        template <typename T>
        struct DefaultFieldOperations
        {
            static void Ctor(uint8_t* ptr)
            {
                new (ptr)T();
            }

            static void CtorMove(uint8_t* ptr, uint8_t* from)
            {
                new (ptr)T(std::move(*reinterpret_cast<T*>(from)));
            }

            static void Dtor(uint8_t* ptr)
            {
                reinterpret_cast<T*>(ptr)->~T();
            }
        };
    }

    template <typename TFieldEnum, TFieldEnum field>
    static constexpr FieldRuntimeInfo GenerateFieldRuntimeInfo()
    {
        using FieldInfo = FieldInfo<TFieldEnum, field>;

        return
        {
            sizeof(typename FieldInfo::StorageType),
            &Detail::DefaultFieldOperations<typename FieldInfo::StorageType>::Ctor,
            &Detail::DefaultFieldOperations<typename FieldInfo::StorageType>::CtorMove,
            &Detail::DefaultFieldOperations<typename FieldInfo::StorageType>::Dtor
        };
    }
}

#define ASSOCIATIVE_STORAGE_ACCESSORS_DECL(stored_type, field_enum_value) \
stored_type& Ensure##field_enum_value##Storage(); \
stored_type* Get##field_enum_value##Storage(); \
const stored_type* Get##field_enum_value##Storage() const;

#define ASSOCIATIVE_STORAGE_ACCESSORS_IMPL(declaring_class, stored_type, field_enum_type, field_enum_value) \
stored_type& declaring_class::Ensure##field_enum_value##Storage() \
{ return m_associativeStorage.Ensure<AssociativeStorage::field_enum_type::field_enum_value>(); } \
stored_type* declaring_class::Get##field_enum_value##Storage() \
{ return m_associativeStorage.Get<AssociativeStorage::field_enum_type::field_enum_value>(); } \
const stored_type* declaring_class::Get##field_enum_value##Storage() const \
{ return m_associativeStorage.Get<AssociativeStorage::field_enum_type::field_enum_value>(); }
