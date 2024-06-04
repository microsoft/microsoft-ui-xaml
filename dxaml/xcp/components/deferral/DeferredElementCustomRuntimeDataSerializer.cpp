// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "CustomRuntimeDataSerializer.h"
#include "CustomWriterRuntimeDataTypeIndex.h"
#include "DeferredElementCustomRuntimeDataSerializer.h"
#include <DeferredElementCustomRuntimeData.h>
#include <xamlbinaryformatsubreader2.h>
#include <xamlbinaryformatsubwriter2.h>
#include <XbfVersioning.h>

namespace CustomRuntimeDataSerializationHelpers
{
    template<>
    _Check_return_ HRESULT Serialize(_In_ const DeferredElementCustomRuntimeData& target, _In_ XamlBinaryFormatSubWriter2* writer, _In_ const std::vector<unsigned int>& streamOffsetTokenTable)
    {
        auto typeIndex = Parser::Versioning::GetDeferredElementSerializationVersion(writer->GetTargetOSVersion());

        // Serialize fields for v2 (v1 was never publicly shipped)
        IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(target.m_strName, writer, streamOffsetTokenTable));
        IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(target.m_nonDeferredProperties, writer, streamOffsetTokenTable));

        if (   typeIndex != CustomWriterRuntimeDataTypeIndex::DeferredElement_v1
            && typeIndex != CustomWriterRuntimeDataTypeIndex::DeferredElement_v2)
        {
            // Serialize fields for v3 or higher
            IFC_RETURN(CustomRuntimeDataSerializationHelpers::Serialize(target.m_realize, writer, streamOffsetTokenTable));
        }
        
        return S_OK;
    }

    template<>
    DeferredElementCustomRuntimeData Deserialize(_In_ XamlBinaryFormatSubReader2* reader, _In_ CustomWriterRuntimeDataTypeIndex typeIndex)
    {
        DeferredElementCustomRuntimeData data;

        data.m_strName = CustomRuntimeDataSerializationHelpers::Deserialize<xstring_ptr>(reader);

        if (typeIndex != CustomWriterRuntimeDataTypeIndex::DeferredElement_v1)
        {
            data.m_nonDeferredProperties = CustomRuntimeDataSerializationHelpers::Deserialize<std::vector<std::pair<std::shared_ptr<XamlProperty>, CValue>>>(reader);

            if (typeIndex != CustomWriterRuntimeDataTypeIndex::DeferredElement_v2)
            {
                data.m_realize = CustomRuntimeDataSerializationHelpers::Deserialize<bool>(reader);
            }
        }

        return data;
    }
}
