// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "CustomRuntimeDataSerializer.h"
#include <XamlBinaryFormatSubWriter2.h>
#include <XamlBinaryFormatSubReader2.h>
#include "CustomWriterRuntimeDataTypeIndex.h"
#include <XamlPredicateHelpers.h>

namespace CustomRuntimeDataSerializationHelpers {

    _Check_return_ HRESULT Serializer<unsigned int>::Write(
        _In_ unsigned int target,
        _In_ XamlBinaryFormatSubWriter2* writer,
        _In_ const std::vector<unsigned int>&)
    {
        IFC_RETURN(writer->PersistConstant(target));
        return S_OK;
    }

    unsigned int Serializer<unsigned int>::Read(_In_ XamlBinaryFormatSubReader2* reader)
    {
        return reader->ReadUInt();
    }

    _Check_return_ HRESULT Serializer<xstring_ptr>::Write(
        _In_ const xstring_ptr& target,
        _In_ XamlBinaryFormatSubWriter2* writer,
        _In_ const std::vector<unsigned int>&)
    {
        IFC_RETURN(writer->PersistSharedString(target.IsNull() ? xstring_ptr::EmptyString() : target));
        return S_OK;
    }

    xstring_ptr Serializer<xstring_ptr>::Read(_In_ XamlBinaryFormatSubReader2* reader)
    {
        return reader->ReadSharedString();
    }

    _Check_return_ HRESULT Serializer<StreamOffsetToken>::Write(
        _In_ const StreamOffsetToken& target,
        _In_ XamlBinaryFormatSubWriter2* writer,
        _In_ const std::vector<unsigned int>& streamOffsetTokenTable)
    {
        ASSERT(target.GetIndex() < streamOffsetTokenTable.size());
        IFC_RETURN(writer->PersistConstant(streamOffsetTokenTable[target.GetIndex()]));
        return S_OK;
    }

    StreamOffsetToken Serializer<StreamOffsetToken>::Read(_In_ XamlBinaryFormatSubReader2* reader)
    {
        return StreamOffsetToken(reader->ReadUInt());
    }

    _Check_return_ HRESULT Serializer<bool>::Write(
        _In_ bool target,
        _In_ XamlBinaryFormatSubWriter2* writer,
        _In_ const std::vector<unsigned int>&)
    {
        CValue targetCValue;
        targetCValue.SetBool(target ? TRUE : FALSE);
        IFC_RETURN(writer->PersistConstant(targetCValue));
        return S_OK;
    }

    bool Serializer<bool>::Read(_In_ XamlBinaryFormatSubReader2* reader)
    {
        CValue targetCValue = reader->ReadCValue();
        return targetCValue.AsBool();
    }

    _Check_return_ HRESULT Serializer<CustomWriterRuntimeDataTypeIndex>::Write(
        _In_ CustomWriterRuntimeDataTypeIndex target,
        _In_ XamlBinaryFormatSubWriter2* writer,
        _In_ const std::vector<unsigned int>&)
    {
        unsigned int targetTypeTokenIndex = static_cast<unsigned int>(target);
        IFC_RETURN(writer->PersistConstant(targetTypeTokenIndex));
        return S_OK;
    }

    CustomWriterRuntimeDataTypeIndex Serializer<CustomWriterRuntimeDataTypeIndex>::Read(_In_ XamlBinaryFormatSubReader2* reader)
    {
        return static_cast<CustomWriterRuntimeDataTypeIndex>(reader->ReadUInt());
    }

    _Check_return_ HRESULT Serializer<std::shared_ptr<XamlProperty>>::Write(
        _In_ const std::shared_ptr<XamlProperty>& target,
        _In_ XamlBinaryFormatSubWriter2* writer,
        _In_ const std::vector<unsigned int>&)
    {
        IFC_RETURN(writer->PersistProperty(target));
        return S_OK;
    }

    std::shared_ptr<XamlProperty> Serializer<std::shared_ptr<XamlProperty>>::Read(_In_ XamlBinaryFormatSubReader2* reader)
    {
        return reader->ReadXamlProperty();
    }

    _Check_return_ HRESULT Serializer<CValue>::Write(
        _In_ const CValue& target,
        _In_ XamlBinaryFormatSubWriter2* writer,
        _In_ const std::vector<unsigned int>&)
    {
        // Calling XamlBinaryFormatSubWriter2::PersistConstant knows how to
        // deal with CValues and applies appropriate type checks to verify
        // that we are not serializing a type we do not understand.
        IFC_RETURN(writer->PersistConstant(target));
        return S_OK;
    }

    CValue Serializer<CValue>::Read(_In_ XamlBinaryFormatSubReader2* reader)
    {
        return reader->ReadCValue();
    }

    _Check_return_ HRESULT Serializer<std::shared_ptr<Parser::XamlPredicateAndArgs>>::Write(
        _In_ const std::shared_ptr<Parser::XamlPredicateAndArgs>& target,
        _In_ XamlBinaryFormatSubWriter2* writer,
        _In_ const std::vector<unsigned int>& streamOffsetTokenTable)
    {
        IFC_RETURN(writer->PersistType(target->PredicateType));
        IFC_RETURN(Serialize(target->Arguments, writer, streamOffsetTokenTable));
        return S_OK;
    }

    std::shared_ptr<Parser::XamlPredicateAndArgs> Serializer<std::shared_ptr<Parser::XamlPredicateAndArgs>>::Read(_In_ XamlBinaryFormatSubReader2* reader)
    {
        auto predicateType = reader->ReadXamlType();
        auto arguments = Serializer<xstring_ptr>::Read(reader);

        return std::make_shared<Parser::XamlPredicateAndArgs>(predicateType, arguments);
    }
}
