// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <CustomWriterRuntimeData.h>
#include <StreamOffsetToken.h>
#include <XamlType.h>
#include <XamlProperty.h>
#include <xstring_ptr.h>
#include <cvalue.h>
#include <vector>

class StyleCustomWriter;
class ObjectWriterNode;

#include <StyleCustomRuntimeDataSerializer.h>

class StyleSetterEssence
{
    friend HRESULT CustomRuntimeDataSerializationHelpers::Serialize<StyleSetterEssence>(
        const StyleSetterEssence&,
        XamlBinaryFormatSubWriter2*,
        const std::vector<unsigned int>&);
    friend StyleSetterEssence CustomRuntimeDataSerializationHelpers::Deserialize<StyleSetterEssence>(
        XamlBinaryFormatSubReader2*);

public:
    StyleSetterEssence()
        : m_propertyIndex(KnownPropertyIndex::UnknownType_UnknownProperty)
    {}

    // Explicitly disabling copying and assignment- don't do that, it's expensive
    // and not part of how CustonRuntimeData should be used.
    StyleSetterEssence(const StyleSetterEssence& other) = delete;
    StyleSetterEssence& operator=(const StyleSetterEssence& other) = delete;

    StyleSetterEssence(StyleSetterEssence&& other) = default;
    StyleSetterEssence& operator=(StyleSetterEssence&& other) = default;

    void SetResolvedPropertyData(const std::shared_ptr<XamlProperty>& xamlProperty)
    {
        ASSERT(!m_flags.isPropertyResolved);
        ASSERT(m_propertyIndex == KnownPropertyIndex::UnknownType_UnknownProperty);
        ASSERT(m_xamlProperty == nullptr);
        ASSERT(m_propertyString.IsNullOrEmpty());
        ASSERT(xamlProperty != nullptr);
        m_xamlProperty = xamlProperty;
        m_propertyIndex = xamlProperty->get_PropertyToken().GetHandle();
        ASSERT(m_propertyIndex != KnownPropertyIndex::UnknownType_UnknownProperty);
        m_flags.isPropertyResolved = true;
    }

    void SetUnresolvedPropertyData(const std::shared_ptr<XamlType>& xamlType, const xstring_ptr& value)
    {
        ASSERT(!m_flags.isPropertyResolved);
        ASSERT(m_propertyIndex == KnownPropertyIndex::UnknownType_UnknownProperty);
        ASSERT(m_propertyString.IsNullOrEmpty());
        ASSERT(xamlType != nullptr);
        m_propertyString = value;
        m_propertyOwnerType = std::move(xamlType);
    }

    void SetContainerValue(const xstring_ptr& stringValue)
    {
        CValue value;
        value.SetString(stringValue);
        SetContainerValue(std::move(value));
    }

    void SetContainerValue(CValue&& value)
    {
        ASSERT(!m_flags.hasTokenForSelf);
        ASSERT(!m_flags.hasObjectValue);
        ASSERT(!m_flags.hasStaticResourceValue);
        ASSERT(!m_flags.hasThemeResourceValue);
        m_valueContainer = std::move(value);
        m_flags.hasContainerValue = true;
    }

    void SetStaticResourceValue(const StreamOffsetToken value)
    {
        ASSERT(!m_flags.hasTokenForSelf);
        ASSERT(!m_flags.hasContainerValue);
        ASSERT(!m_flags.hasObjectValue);
        ASSERT(!m_flags.hasThemeResourceValue);
        m_valueToken = value;
        m_flags.hasStaticResourceValue = true;
    }

    void SetThemeResourceValue(const StreamOffsetToken value)
    {
        ASSERT(!m_flags.hasTokenForSelf);
        ASSERT(!m_flags.hasContainerValue);
        ASSERT(!m_flags.hasObjectValue);
        ASSERT(!m_flags.hasStaticResourceValue);
        m_valueToken = value;
        m_flags.hasThemeResourceValue = true;
    }

    void SetObjectValue(const StreamOffsetToken value)
    {
        ASSERT(!m_flags.hasTokenForSelf);
        ASSERT(!m_flags.hasContainerValue);
        ASSERT(!m_flags.hasStaticResourceValue);
        ASSERT(!m_flags.hasThemeResourceValue);
        m_valueToken = value;
        m_flags.hasObjectValue = true;
    }

    void SetSetterObject(const StreamOffsetToken value, const bool isMutable)
    {
        m_valueToken = value;
        m_flags.decode(m_flags.encode() & 0x10); // clear all flags except isPropertyResolved
        m_flags.hasTokenForSelf = true;
        m_flags.isMutable = isMutable;
    }

    bool IsPropertyResolved() const { return m_flags.isPropertyResolved; }
    bool HasContainerValue() const { return m_flags.hasContainerValue; }
    bool HasObjectValue() const { return m_flags.hasObjectValue; }
    bool HasStaticResourceValue() const { return m_flags.hasStaticResourceValue; }
    bool HasThemeResourceValue() const { return m_flags.hasThemeResourceValue; }
    bool HasTokenForSelf() const { return m_flags.hasTokenForSelf; }
    bool IsMutable() const { return m_flags.isMutable; }

    std::shared_ptr<XamlProperty> GetXamlProperty() const { return m_xamlProperty; }
    KnownPropertyIndex GetPropertyIndex() const { return m_propertyIndex; }
    const xstring_ptr& GetPropertyString() const { return m_propertyString; }
    const std::shared_ptr<XamlType>& GetPropertyOwnerType() const { return m_propertyOwnerType; }
    CValue& GetValueContainer() { return m_valueContainer; }
    StreamOffsetToken GetValueToken() const { return m_valueToken; }

private:
    struct valueFlags
    {
        bool isPropertyResolved : 1;
        bool hasObjectValue : 1;
        bool hasStringValue : 1;
        bool hasStaticResourceValue : 1;
        bool hasThemeResourceValue : 1;
        bool hasContainerValue : 1;
        bool hasTokenForSelf : 1;
        bool isMutable : 1;

        const std::uint8_t encode() const
        {
            std::uint8_t result = 0;
            result = result | (static_cast<unsigned char>(isMutable) << 7);
            result = result | (static_cast<unsigned char>(hasTokenForSelf) << 6);
            result = result | (static_cast<unsigned char>(hasContainerValue) << 5);
            result = result | (static_cast<unsigned char>(isPropertyResolved) << 4);
            result = result | (static_cast<unsigned char>(hasObjectValue) << 3);
            result = result | (static_cast<unsigned char>(hasStringValue) << 2);
            result = result | (static_cast<unsigned char>(hasStaticResourceValue) << 1);
            result = result | (static_cast<unsigned char>(hasThemeResourceValue));
            return result;
        }

        void decode(std::uint8_t value)
        {
            isMutable = (value & (1 << 7)) != 0;
            hasTokenForSelf = (value & (1 << 6)) != 0;
            hasContainerValue = (value & (1 << 5)) != 0;
            isPropertyResolved = (value & (1 << 4)) != 0;
            hasObjectValue = (value & (1 << 3)) != 0;
            hasStringValue = (value & (1 << 2)) != 0;
            hasStaticResourceValue = (value & (1 << 1)) != 0;
            hasThemeResourceValue = (value & 0x1) != 0;
        }
    } m_flags {};

    KnownPropertyIndex m_propertyIndex;
    std::shared_ptr<XamlProperty> m_xamlProperty;
    xstring_ptr m_propertyString;
    std::shared_ptr<XamlType> m_propertyOwnerType;
    CValue m_valueContainer;
    StreamOffsetToken m_valueToken;
};

class StyleCustomRuntimeData
    : public CustomWriterRuntimeData
{
    friend class StyleCustomWriter;

    friend HRESULT CustomRuntimeDataSerializationHelpers::Serialize<StyleCustomRuntimeData>(
        const StyleCustomRuntimeData&,
        XamlBinaryFormatSubWriter2*,
        const std::vector<unsigned int>&);
    friend StyleCustomRuntimeData CustomRuntimeDataSerializationHelpers::Deserialize<StyleCustomRuntimeData>(
        XamlBinaryFormatSubReader2*,
        CustomWriterRuntimeDataTypeIndex);

public:
    // Explicit ctor/dtor to support forward declaration of
    // unique_ptr's template type below.
    StyleCustomRuntimeData() = default;

    // Explicitly disabling copying and assignment- don't do that, it's expensive
    // and not part of how CustonRuntimeData should be used.
    StyleCustomRuntimeData(const StyleCustomRuntimeData& other) = delete;
    StyleCustomRuntimeData& operator=(const StyleCustomRuntimeData& other) = delete;

    StyleCustomRuntimeData(StyleCustomRuntimeData&& other) = default;
    StyleCustomRuntimeData& operator=(StyleCustomRuntimeData&& other) = default;

    // CustomWriterRuntimeData Serialization/Deserialization
    _Check_return_ HRESULT PrepareStream(_In_ std::shared_ptr<SubObjectWriterResult>& customWriterStream) override;
protected:
    _Check_return_ HRESULT SerializeImpl(_In_ XamlBinaryFormatSubWriter2*, _In_ const std::vector<unsigned int>&) override;
    CustomWriterRuntimeDataTypeIndex GetTypeIndexForSerialization(_In_ const TargetOSVersion& targetOS) const override;
public:
    _Check_return_ HRESULT ToString(_In_ bool verboseData, _Out_ xstring_ptr& strValue) const override;

    unsigned int GetSetterCount() const { return static_cast<unsigned int>(m_setters.size()); }
    bool IsSetterPropertyResolved(_In_ unsigned int index) const { return m_setters[index].IsPropertyResolved(); }
    bool SetterHasContainerValue(_In_ unsigned int index) const { return m_setters[index].HasContainerValue(); }
    bool SetterHasObjectValue(_In_ unsigned int index) const { return m_setters[index].HasObjectValue(); }
    bool SetterHasStaticResourceValue(_In_ unsigned int index) const { return m_setters[index].HasStaticResourceValue(); }
    bool SetterHasThemeResourceValue(_In_ unsigned int index) const { return m_setters[index].HasThemeResourceValue(); }
    bool SetterHasTokenForSelf(_In_ unsigned int index) const { return m_setters[index].HasTokenForSelf(); }
    bool SetterIsMutable(_In_ unsigned int index) const { return m_setters[index].IsMutable(); }

    KnownPropertyIndex GetSetterPropertyIndex(_In_ unsigned int index) const { return m_setters[index].GetPropertyIndex(); }
    const xstring_ptr& GetSetterPropertyString(_In_ unsigned int index) const { return m_setters[index].GetPropertyString(); }
    const std::shared_ptr<XamlType>& GetSetterPropertyOwnerType(_In_ unsigned int index) const { return m_setters[index].GetPropertyOwnerType(); }
    CValue& GetSetterValueContainer(_In_ unsigned int index) { return m_setters[index].GetValueContainer(); }
    StreamOffsetToken GetSetterValueToken(_In_ unsigned int index) const { return m_setters[index].GetValueToken(); }

private:
    std::vector<StyleSetterEssence> m_setters;
    std::vector<std::pair<bool,bool>> m_offsetTokenStates;

    void SkipMarkerNodes(_In_ std::vector<ObjectWriterNode>& nodeList, _In_ std::vector<ObjectWriterNode>::iterator& itr, std::pair<bool, bool> isSetterToken);
};


