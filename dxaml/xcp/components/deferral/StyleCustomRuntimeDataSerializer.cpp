// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "StyleCustomRuntimeDataSerializer.h"
#include "StyleCustomRuntimeData.h"
#include "CustomRuntimeDataSerializer.h"
#include "CustomWriterRuntimeDataTypeIndex.h"
#include <xamlbinaryformatsubwriter2.h>
#include <xamlbinaryformatsubreader2.h>
#include <CValue.h>
#include <XamlProperty.h>
#include <XbfMetadataApi.h>

namespace CustomRuntimeDataSerializationHelpers
{
    template <>
    _Check_return_ HRESULT Serialize(_In_ const StyleSetterEssence& target, _In_ XamlBinaryFormatSubWriter2* writer, _In_ const std::vector<unsigned int>& streamOffsetTokenTable)
    {
        IFC_RETURN(Serialize(static_cast<unsigned int>(target.m_flags.encode()), writer, streamOffsetTokenTable));

        if (target.m_flags.isPropertyResolved)
        {
            ASSERT(target.m_propertyIndex != KnownPropertyIndex::UnknownType_UnknownProperty);
            ASSERT(target.m_xamlProperty != nullptr);
            IFC_RETURN(writer->PersistProperty(target.m_xamlProperty));
        }
        else
        {
            ASSERT(!target.m_propertyString.IsNullOrEmpty());
            IFC_RETURN(Serialize(target.m_propertyString, writer, streamOffsetTokenTable));
            IFC_RETURN(writer->PersistType(target.m_propertyOwnerType));
        }

        if (target.m_flags.hasContainerValue)
        {
            ASSERT(!target.m_flags.hasTokenForSelf);
            ASSERT(!target.m_flags.hasObjectValue);
            ASSERT(!target.m_flags.hasStaticResourceValue);
            ASSERT(!target.m_flags.hasThemeResourceValue);
            IFC_RETURN(writer->PersistConstant(target.m_valueContainer));
        }
        else if (target.m_flags.hasStaticResourceValue)
        {
            ASSERT(!target.m_flags.hasTokenForSelf);
            ASSERT(!target.m_flags.hasContainerValue);
            ASSERT(!target.m_flags.hasObjectValue);
            ASSERT(!target.m_flags.hasThemeResourceValue);
            IFC_RETURN(Serialize(target.m_valueToken, writer, streamOffsetTokenTable));
        }
        else if (target.m_flags.hasThemeResourceValue)
        {
            ASSERT(!target.m_flags.hasTokenForSelf);
            ASSERT(!target.m_flags.hasContainerValue);
            ASSERT(!target.m_flags.hasObjectValue);
            ASSERT(!target.m_flags.hasStaticResourceValue);
            IFC_RETURN(Serialize(target.m_valueToken, writer, streamOffsetTokenTable));
        }
        else if (target.m_flags.hasObjectValue)
        {
            ASSERT(!target.m_flags.hasTokenForSelf);
            ASSERT(!target.m_flags.hasContainerValue);
            ASSERT(!target.m_flags.hasStaticResourceValue);
            ASSERT(!target.m_flags.hasThemeResourceValue);
            IFC_RETURN(Serialize(target.m_valueToken, writer, streamOffsetTokenTable));
        }
        else if (target.m_flags.hasTokenForSelf)
        {
            ASSERT(!target.m_flags.hasObjectValue);
            ASSERT(!target.m_flags.hasContainerValue);
            ASSERT(!target.m_flags.hasStaticResourceValue);
            ASSERT(!target.m_flags.hasThemeResourceValue);
            IFC_RETURN(Serialize(target.m_valueToken, writer, streamOffsetTokenTable));
        }
        return S_OK;
    }

    template<>
    StyleSetterEssence Deserialize(_In_ XamlBinaryFormatSubReader2* reader)
    {
        StyleSetterEssence essence;

        essence.m_flags.decode(static_cast<unsigned char>(Deserialize<unsigned int>(reader)));

        if (essence.m_flags.isPropertyResolved)
        {
            auto xamlProperty = reader->ReadXamlProperty();
            essence.m_propertyIndex = xamlProperty->get_PropertyToken().GetHandle();
        }
        else
        {
            essence.m_propertyString = Deserialize<xstring_ptr>(reader);
            essence.m_propertyOwnerType = reader->ReadXamlType();
        }

        if (essence.m_flags.hasStringValue)
        {
            ASSERT(!essence.m_flags.hasTokenForSelf);
            ASSERT(!essence.m_flags.hasContainerValue);
            ASSERT(!essence.m_flags.hasObjectValue);
            ASSERT(!essence.m_flags.hasStaticResourceValue);
            ASSERT(!essence.m_flags.hasThemeResourceValue);
            essence.SetContainerValue(Deserialize<xstring_ptr>(reader));
        }
        else if (essence.m_flags.hasContainerValue)
        {
            ASSERT(!essence.m_flags.hasTokenForSelf);
            ASSERT(!essence.m_flags.hasStringValue);
            ASSERT(!essence.m_flags.hasObjectValue);
            ASSERT(!essence.m_flags.hasStaticResourceValue);
            ASSERT(!essence.m_flags.hasThemeResourceValue);
            essence.m_valueContainer = reader->ReadCValue();
        }
        else if (essence.m_flags.hasStaticResourceValue)
        {
            ASSERT(!essence.m_flags.hasTokenForSelf);
            ASSERT(!essence.m_flags.hasStringValue);
            ASSERT(!essence.m_flags.hasContainerValue);
            ASSERT(!essence.m_flags.hasObjectValue);
            ASSERT(!essence.m_flags.hasThemeResourceValue);
            essence.m_valueToken = Deserialize<StreamOffsetToken>(reader);
        }
        else if (essence.m_flags.hasThemeResourceValue)
        {
            ASSERT(!essence.m_flags.hasTokenForSelf);
            ASSERT(!essence.m_flags.hasStringValue);
            ASSERT(!essence.m_flags.hasContainerValue);
            ASSERT(!essence.m_flags.hasObjectValue);
            ASSERT(!essence.m_flags.hasStaticResourceValue);
            essence.m_valueToken = Deserialize<StreamOffsetToken>(reader);
        }
        else if (essence.m_flags.hasObjectValue)
        {
            ASSERT(!essence.m_flags.hasTokenForSelf);
            ASSERT(!essence.m_flags.hasStringValue);
            ASSERT(!essence.m_flags.hasContainerValue);
            ASSERT(!essence.m_flags.hasStaticResourceValue);
            ASSERT(!essence.m_flags.hasThemeResourceValue);
            essence.m_valueToken = Deserialize<StreamOffsetToken>(reader);
        }
        else if (essence.m_flags.hasTokenForSelf)
        {
            ASSERT(!essence.m_flags.hasObjectValue);
            ASSERT(!essence.m_flags.hasStringValue);
            ASSERT(!essence.m_flags.hasContainerValue);
            ASSERT(!essence.m_flags.hasStaticResourceValue);
            ASSERT(!essence.m_flags.hasThemeResourceValue);
            essence.m_valueToken = Deserialize<StreamOffsetToken>(reader);
        }

        return essence;
    }

    template <>
    _Check_return_ HRESULT Serialize(_In_ const StyleCustomRuntimeData& target, _In_ XamlBinaryFormatSubWriter2* writer, _In_ const std::vector<unsigned int>& streamOffsetTokenTable)
    {
        IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(target.m_setters, writer, streamOffsetTokenTable));
        return S_OK;
    }

    template<>
    StyleCustomRuntimeData Deserialize(_In_ XamlBinaryFormatSubReader2* reader, _In_ CustomWriterRuntimeDataTypeIndex typeIndex)
    {
        UNREFERENCED_PARAMETER(typeIndex);

        StyleCustomRuntimeData data;

        data.m_setters = Deserialize<std::vector<StyleSetterEssence>>(reader);

        return data;
    }
}
