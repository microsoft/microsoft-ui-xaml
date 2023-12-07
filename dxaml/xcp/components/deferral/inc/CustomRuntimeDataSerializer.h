// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cvalue.h>
#include <StreamOffsetToken.h>
#include <XamlProperty.h>
#include <type_traits>
#include <cstdint>
#include <vector_map.h>
#include <vector_set.h>
#include <unordered_map>
#include "StaticAssertFalse.h"

class XamlBinaryFormatSubWriter2;
class XamlBinaryFormatSubReader2;
enum class CustomWriterRuntimeDataTypeIndex : std::uint16_t;

namespace Parser
{
struct XamlPredicateAndArgs;
}

namespace CustomRuntimeDataSerializationHelpers {
    template <typename T>
    _Check_return_ HRESULT Serialize(
        _In_ const T& target,
        _In_ XamlBinaryFormatSubWriter2* writer,
        _In_ const std::vector<unsigned int>& streamOffsetTokenTable);
    template <typename T> T Deserialize(_In_ XamlBinaryFormatSubReader2* reader);
    template <typename T> T Deserialize(_In_ XamlBinaryFormatSubReader2* reader, _In_ CustomWriterRuntimeDataTypeIndex typeIndex);

    namespace detail {
#pragma region serialization traits
        // Prep for tag-based dispatch
        struct serialization_tag_value {};
        struct serialization_tag_container {};
        struct serialization_tag_sequential_container : public serialization_tag_container {};
        struct serialization_tag_unordered_container : public serialization_tag_container {};
        struct serialization_tag_presorted_container : public serialization_tag_container {};

        // By default, treat everything as a value
        template <typename T>
        struct serialization_traits {
            using category = serialization_tag_value;
        };

        template <typename T, typename... Args>
        struct serialization_traits < ::std::vector<T, Args...> > {
            using category = serialization_tag_sequential_container;
        };

        template <typename K, typename V, typename... Args>
        struct serialization_traits < ::std::unordered_map<K, V, Args...> > {
            using category = serialization_tag_unordered_container;
        };

        template <typename K, typename V, typename... Args>
        struct serialization_traits < containers::vector_map<K, V, Args...> > {
            using category = serialization_tag_presorted_container;
        };

        template <typename T, typename... Args>
        struct serialization_traits < containers::vector_set<T, Args...> > {
            using category = serialization_tag_presorted_container;
        };
#pragma endregion

#pragma region container dispatch
        template<typename Container, typename Value>
        void add_to_container(Container& c, Value&& v, serialization_tag_sequential_container)
        {
            c.emplace_back(std::forward<Value>(v));
        }

        template<typename Container, typename Value>
        void add_to_container(Container& c, Value&& v, serialization_tag_unordered_container)
        {
            c.emplace(std::forward<Value>(v));
        }

        template<typename Container, typename Value>
        void add_to_container(Container& c, Value&& v, serialization_tag_presorted_container)
        {
            c.emplace_hint(c.end(), std::forward<Value>(v));
        }
#pragma endregion

#pragma region helper dispatchers

        // Write a value
        template <typename T>
        _Check_return_ HRESULT SerializeImpl(
            _In_ const T& target,
            _In_ XamlBinaryFormatSubWriter2* writer,
            _In_ const std::vector<unsigned int>& streamOffsetTokenTable,
            _In_ serialization_tag_value)
        {
            IFC_RETURN(Serializer<T>::Write(target, writer, streamOffsetTokenTable));
            return S_OK;
        }

        // Write a container
        template<typename T>
        _Check_return_ HRESULT SerializeImpl(
            _In_ const T& target,
            _In_ XamlBinaryFormatSubWriter2* writer,
            _In_ const std::vector<unsigned int>& streamOffsetTokenTable,
            _In_ serialization_tag_container)
        {
            IFC_RETURN(Serialize(static_cast<unsigned int>(target.size()), writer, streamOffsetTokenTable));
            for (const auto& item : target) {
                IFC_RETURN(Serialize(item, writer, streamOffsetTokenTable));
            }
            return S_OK;
        }

        // Read a value
        template<typename T, typename... Args>
        T DeserializeImpl(
            _In_ XamlBinaryFormatSubReader2* reader,
            _In_ serialization_tag_value,
            Args&&... args)
        {
            return Serializer<T>::Read(reader, ::std::forward<Args>(args)...);
        }

        // Read a container
        template<typename T, typename... Args>
        T DeserializeImpl(
            _In_ XamlBinaryFormatSubReader2* reader,
            _In_ serialization_tag_container,
            Args&&... args)
        {
            auto size = Deserialize<unsigned int>(reader);
            T result;
            // This call to reserve() assumes we have reservable containers
            // If we add another container type that doesn't do this, add to
            // the existing traits pattern and offload this call similar to the emplace call below
            result.reserve(size);
            for (unsigned int i = 0; i < size; ++i) {
                add_to_container(result, Deserialize<typename T::value_type>(reader, ::std::forward<Args>(args)...), typename serialization_traits<T>::category());
            }
            return result;
        }

#pragma endregion
    }

    template <typename T>
    _Check_return_ HRESULT Serialize(
        _In_ const T& target,
        _In_ XamlBinaryFormatSubWriter2* writer,
        _In_ const std::vector<unsigned int>& streamOffsetTokenTable)
    {
        IFC_RETURN(detail::SerializeImpl(target, writer, streamOffsetTokenTable, typename detail::serialization_traits<T>::category()));
        return S_OK;
    }

    template <typename T>
    T Deserialize(_In_ XamlBinaryFormatSubReader2* reader)
    {
        return detail::DeserializeImpl<T>(reader, typename detail::serialization_traits<T>::category());
    }

    template <typename T>
    T Deserialize(_In_ XamlBinaryFormatSubReader2* reader, _In_ CustomWriterRuntimeDataTypeIndex typeIndex)
    {
        return detail::DeserializeImpl<T>(reader, typename detail::serialization_traits<T>::category(), typeIndex);
    }

    // In order to differentiate Deserialize methods, we rely on template specialization
    // because overloads can't differ only by return type. This is trickier, because
    // function templates can't be partially specialized, leading to overload resolution
    // failures when the template type parameter is itself a template (i.e. pair<T1, T2>).
    // To combat this, the various value specializations occur at the class level in
    // a Serializer struct

    // Stub out unspecialized versions as fallbacks because the resultant compile error is easier and
    // quicker to diagnose than the linker error we'd get if they were simply missing
    template<typename T>
    struct Serializer
    {
        static _Check_return_ HRESULT Write(_In_ const T&, _In_ XamlBinaryFormatSubWriter2*, _In_ const std::vector<unsigned int>&)
        {
            static_assert_false("Unspecialized Serializer::Write called");
        }

        static T Read(_In_ XamlBinaryFormatSubReader2*)
        {
            static_assert_false("Unspecialized Serializer::Read(XamlBinaryFormatSubReader2*) called");
        }

        static T Read(_In_ XamlBinaryFormatSubReader2*, _In_ CustomWriterRuntimeDataTypeIndex)
        {
            static_assert_false("Unspecialized Serializer::Read(XamlBinaryFormatSubReader2*, CustomWriterRuntimeDataTypeIndex) called");
        }
    };

    // Serializers/deserializers for standard value types
    template<>
    struct Serializer < unsigned int >
    {
        static _Check_return_ HRESULT Write(
            _In_ unsigned int target,
            _In_ XamlBinaryFormatSubWriter2* writer,
            _In_ const std::vector<unsigned int>& streamOffsetTokenTable);
        static unsigned int Read(_In_ XamlBinaryFormatSubReader2* reader);
    };

    template<>
    struct Serializer < int >
    {
        static _Check_return_ HRESULT Write(
            _In_ int target,
            _In_ XamlBinaryFormatSubWriter2* writer,
            _In_ const std::vector<unsigned int>& streamOffsetTokenTable)
        {
            IFC_RETURN(Serialize(static_cast<unsigned int>(target), writer, streamOffsetTokenTable));
            return S_OK;
        }
        static int Read(_In_ XamlBinaryFormatSubReader2* reader)
        {
            return static_cast<int>(Deserialize<unsigned int>(reader));
        }
    };

    template<>
    struct Serializer < xstring_ptr >
    {
        static _Check_return_ HRESULT Write(
            _In_ const xstring_ptr& target,
            _In_ XamlBinaryFormatSubWriter2* writer,
            _In_ const std::vector<unsigned int>& streamOffsetTokenTable);
        static xstring_ptr Read(_In_ XamlBinaryFormatSubReader2* reader);
    };

    template<>
    struct Serializer < StreamOffsetToken >
    {
        static _Check_return_ HRESULT Write(
            _In_ const StreamOffsetToken& target,
            _In_ XamlBinaryFormatSubWriter2* writer,
            _In_ const std::vector<unsigned int>& streamOffsetTokenTable);
        static StreamOffsetToken Read(_In_ XamlBinaryFormatSubReader2* reader);
    };

    template<>
    struct Serializer < bool >
    {
        static _Check_return_ HRESULT Write(
            _In_ bool target,
            _In_ XamlBinaryFormatSubWriter2* writer,
            _In_ const std::vector<unsigned int>& streamOffsetTokenTable);
        static bool Read(_In_ XamlBinaryFormatSubReader2* reader);
    };

    template<>
    struct Serializer < CustomWriterRuntimeDataTypeIndex >
    {
        static _Check_return_ HRESULT Write(
            _In_ CustomWriterRuntimeDataTypeIndex target,
            _In_ XamlBinaryFormatSubWriter2* writer,
            _In_ const std::vector<unsigned int>& streamOffsetTokenTable);
        static CustomWriterRuntimeDataTypeIndex Read(_In_ XamlBinaryFormatSubReader2* reader);
    };

    template<>
    struct Serializer < std::shared_ptr<XamlProperty> >
    {
        static _Check_return_ HRESULT Write(
            _In_ const std::shared_ptr<XamlProperty>& target,
            _In_ XamlBinaryFormatSubWriter2* writer,
            _In_ const std::vector<unsigned int>& streamOffsetTokenTable);
        static std::shared_ptr<XamlProperty> Read(_In_ XamlBinaryFormatSubReader2* reader);
    };

    template<>
    struct Serializer < CValue >
    {
        static _Check_return_ HRESULT Write(
            _In_ const CValue& target,
            _In_ XamlBinaryFormatSubWriter2* writer,
            _In_ const std::vector<unsigned int>& streamOffsetTokenTable);
        static CValue Read(_In_ XamlBinaryFormatSubReader2* reader);
    };

    template<>
    struct Serializer<std::shared_ptr<Parser::XamlPredicateAndArgs>>
    {
        static _Check_return_ HRESULT Write(
            _In_ const std::shared_ptr<Parser::XamlPredicateAndArgs>& target,
            _In_ XamlBinaryFormatSubWriter2* writer,
            _In_ const std::vector<unsigned int>& streamOffsetTokenTable);
        static std::shared_ptr<Parser::XamlPredicateAndArgs> Read(_In_ XamlBinaryFormatSubReader2* reader);
    };

    template<typename T1, typename T2>
    struct Serializer < ::std::pair<T1, T2> >
    {
        static _Check_return_ HRESULT Write(
            _In_ const ::std::pair<T1, T2>& target,
            _In_ XamlBinaryFormatSubWriter2* writer,
            _In_ const std::vector<unsigned int>& streamOffsetTokenTable)
        {
            IFC_RETURN(Serialize(target.first, writer, streamOffsetTokenTable));
            IFC_RETURN(Serialize(target.second, writer, streamOffsetTokenTable));
            return S_OK;
        }
        static ::std::pair<T1, T2> Read(_In_ XamlBinaryFormatSubReader2* reader)
        {
            auto first = Deserialize<typename ::std::remove_cv<T1>::type>(reader);
            auto second = Deserialize<typename ::std::remove_cv<T2>::type>(reader);
            return ::std::make_pair(::std::move(first), ::std::move(second));
        }
    };
}

