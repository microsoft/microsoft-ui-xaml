// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <CustomWriterRuntimeData.h>
#include <DeferredElementCustomRuntimeDataSerializer.h>

class DeferredElementCustomRuntimeData
    : public CustomWriterRuntimeData
{
    friend class DeferredElementCustomWriter;
    friend HRESULT CustomRuntimeDataSerializationHelpers::Serialize<DeferredElementCustomRuntimeData>(
        const DeferredElementCustomRuntimeData&,
        XamlBinaryFormatSubWriter2*,
        const std::vector<unsigned int>&);
    friend DeferredElementCustomRuntimeData CustomRuntimeDataSerializationHelpers::Deserialize<DeferredElementCustomRuntimeData>(
        XamlBinaryFormatSubReader2*,
        CustomWriterRuntimeDataTypeIndex);

public:
    DeferredElementCustomRuntimeData() = default;

    // Explicitly disabling copying and assignment.
    DeferredElementCustomRuntimeData(const DeferredElementCustomRuntimeData& other) = delete;
    DeferredElementCustomRuntimeData& operator=(const DeferredElementCustomRuntimeData& other) = delete;

    DeferredElementCustomRuntimeData(DeferredElementCustomRuntimeData&& other) = default;
    DeferredElementCustomRuntimeData& operator=(DeferredElementCustomRuntimeData&& other) = default;

    const xstring_ptr& GetName() const { return m_strName; }
    const std::vector<std::pair<std::shared_ptr<XamlProperty>, CValue>>& GetNonDeferredProperties() const { return m_nonDeferredProperties; }
    bool Realize() const { return m_realize; }

    // CustomWriterRuntimeData Serialization/Deserialization
    _Check_return_ HRESULT ToString(
        _In_ bool verboseData,
        _Out_ xstring_ptr& strValue) const override;
protected:
    _Check_return_ HRESULT SerializeImpl(
        _In_ XamlBinaryFormatSubWriter2*,
        _In_ const std::vector<unsigned int>&) override;
    CustomWriterRuntimeDataTypeIndex GetTypeIndexForSerialization(_In_ const TargetOSVersion& targetOS) const override;

private:
    xstring_ptr m_strName;
    std::vector<std::pair<std::shared_ptr<XamlProperty>, CValue>> m_nonDeferredProperties;
    bool m_realize = false;
};


