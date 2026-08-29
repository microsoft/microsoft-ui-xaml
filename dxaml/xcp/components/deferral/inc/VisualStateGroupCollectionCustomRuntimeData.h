// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <CustomWriterRuntimeData.h>
#include <StreamOffsetToken.h>

#include <xstring_ptr.h>
#include <vector>
#include <cstdint>

class VisualStateGroupCollectionCustomWriter;
class VisualTransitionTableOptimizedLookup;
enum class CustomWriterRuntimeDataTypeIndex : std::uint16_t;

#include <VisualStateGroupCollectionCustomRuntimeDataSerializer.h>

class VisualStateGroupEssence
{
    friend class VisualStateGroupCollectionCustomWriter;
    friend HRESULT CustomRuntimeDataSerializationHelpers::Serialize<VisualStateGroupEssence>(
        const VisualStateGroupEssence&,
        XamlBinaryFormatSubWriter2*,
        const std::vector<unsigned int>&);
    friend VisualStateGroupEssence CustomRuntimeDataSerializationHelpers::Deserialize<VisualStateGroupEssence>(
        XamlBinaryFormatSubReader2*);

public:
    VisualStateGroupEssence() = default;

    VisualStateGroupEssence(const VisualStateGroupEssence& other) = delete;
    VisualStateGroupEssence& operator=(const VisualStateGroupEssence& other) = delete;

    VisualStateGroupEssence(VisualStateGroupEssence&& other) = default;
    VisualStateGroupEssence& operator=(VisualStateGroupEssence&& other) = default;

    const xstring_ptr& GetName() const noexcept { return m_name; }
    bool HasDynamicTimelines() const noexcept { return m_hasDynamicTimelines; }

private:
    xstring_ptr m_name;
    bool m_hasDynamicTimelines = false;
    StreamOffsetToken m_deferredSelf;
};

class VisualStateEssence
{
    friend class VisualStateGroupCollectionCustomWriter;
    friend HRESULT CustomRuntimeDataSerializationHelpers::Serialize<VisualStateEssence>(
        const VisualStateEssence&,
        XamlBinaryFormatSubWriter2*,
        const std::vector<unsigned int>&);
    friend VisualStateEssence CustomRuntimeDataSerializationHelpers::Deserialize<VisualStateEssence>(
        XamlBinaryFormatSubReader2*,
        CustomWriterRuntimeDataTypeIndex);

public:
    VisualStateEssence() = default;

    VisualStateEssence(const VisualStateEssence& other) = delete;
    VisualStateEssence& operator=(const VisualStateEssence& other) = delete;

    VisualStateEssence(VisualStateEssence&& other) = default;

    VisualStateEssence& operator=(VisualStateEssence&& other) = default;


    const xstring_ptr& GetName() const noexcept { return m_name; }
    StreamOffsetToken GetStoryboardToken() const noexcept { return m_deferredStoryboardToken; }
    const std::vector<StreamOffsetToken>& GetPropertySetterTokens() const WI_NOEXCEPT { return m_deferredPropertySetterTokens; }
    bool HasStoryboard() const noexcept { return m_hasStoryboard; }
    bool HasPropertySetters() const noexcept { return !m_deferredPropertySetterTokens.empty(); }
    const std::vector<std::vector<int>>& GetStateTriggerValues() const noexcept { return m_stateTriggerValues; }
    bool HasStateTriggers() const noexcept { return (m_stateTriggerValues.size() > 0); }

    const std::vector<StreamOffsetToken>& GetExtensibleStateTriggerTokens() const noexcept { return m_extensibleStateTriggerTokens; }
    bool HasExtensibleStateTriggers() const noexcept { return (m_extensibleStateTriggerTokens.size() > 0); }

    const std::vector<StreamOffsetToken>& GetStateTriggerCollectionTokens() const noexcept { return m_stateTriggerCollectionTokens; }
    bool HasStateTriggerCollectionTokens() const noexcept { return (m_stateTriggerCollectionTokens.size() > 0); }

    const std::vector<StreamOffsetToken>& GetStaticResourceTriggerTokens() const noexcept { return m_staticResourceTriggerTokens; }
    bool HasStaticResourceTriggerTokens() const noexcept { return (m_stateTriggerCollectionTokens.size() > 0); }

private:
    xstring_ptr m_name;
    StreamOffsetToken m_deferredStoryboardToken;
    std::vector<StreamOffsetToken> m_deferredPropertySetterTokens;
    bool m_hasStoryboard = false;
    std::vector<std::vector<int>> m_stateTriggerValues;
    std::vector<StreamOffsetToken> m_extensibleStateTriggerTokens;
    std::vector<StreamOffsetToken> m_stateTriggerCollectionTokens;
    std::vector<StreamOffsetToken> m_staticResourceTriggerTokens;
};

namespace Windows {
    namespace UI {
        namespace Xaml {
            namespace Tests {
                namespace Framework {
                    class VisualTransitionTableOptimizedLookupUnitTests;
                }
            }
        }
    }
}

class VisualTransitionEssence
{
    friend class VisualStateGroupCollectionCustomWriter;
    friend class ::Windows::UI::Xaml::Tests::Framework::VisualTransitionTableOptimizedLookupUnitTests;
    friend HRESULT CustomRuntimeDataSerializationHelpers::Serialize<VisualTransitionEssence>(
        const VisualTransitionEssence&,
        XamlBinaryFormatSubWriter2*,
        const std::vector<unsigned int>&);
    friend VisualTransitionEssence CustomRuntimeDataSerializationHelpers::Deserialize<VisualTransitionEssence>(
        XamlBinaryFormatSubReader2*);

public:
    VisualTransitionEssence() = default;

    VisualTransitionEssence(const VisualTransitionEssence& other) = delete;
    VisualTransitionEssence& operator=(const VisualTransitionEssence& other) = delete;

    VisualTransitionEssence(VisualTransitionEssence&& other) = default;
    VisualTransitionEssence& operator=(VisualTransitionEssence&& other) = default;

    const xstring_ptr& GetToState() const noexcept { return m_toState; }
    const xstring_ptr& GetFromState() const noexcept { return m_fromState; }
    StreamOffsetToken GetVisualTransitionToken() const noexcept { return m_deferredTransitionToken; }

private:
    xstring_ptr m_toState;
    xstring_ptr m_fromState;
    StreamOffsetToken m_deferredTransitionToken;
};

class VisualStateGroupCollectionCustomRuntimeData
    : public CustomWriterRuntimeData
{
    friend class VisualStateGroupCollectionCustomWriter;
    friend HRESULT CustomRuntimeDataSerializationHelpers::Serialize<VisualStateGroupCollectionCustomRuntimeData>(
        const VisualStateGroupCollectionCustomRuntimeData&,
        XamlBinaryFormatSubWriter2*,
        const std::vector<unsigned int>&);
    friend VisualStateGroupCollectionCustomRuntimeData CustomRuntimeDataSerializationHelpers::Deserialize<VisualStateGroupCollectionCustomRuntimeData>(
        XamlBinaryFormatSubReader2*,
        CustomWriterRuntimeDataTypeIndex);

public:
    // Explicit ctor/dtor to support forward declaration of
    // unique_ptr's template type below.
    VisualStateGroupCollectionCustomRuntimeData();
    ~VisualStateGroupCollectionCustomRuntimeData() override;

    bool ShouldShare() const override { return true; }

    // Explicitly disabling copying and assignment- don't do that, it's expensive
    // and not part of how CustonRuntimeData should be used.
    VisualStateGroupCollectionCustomRuntimeData(const VisualStateGroupCollectionCustomRuntimeData& other) = delete;
    VisualStateGroupCollectionCustomRuntimeData& operator=(const VisualStateGroupCollectionCustomRuntimeData& other) = delete;

    VisualStateGroupCollectionCustomRuntimeData(VisualStateGroupCollectionCustomRuntimeData&& other) = default;
    VisualStateGroupCollectionCustomRuntimeData& operator=(VisualStateGroupCollectionCustomRuntimeData&& other) = default;

    unsigned int GetGroupIndex(_In_ unsigned int visualStateIndex) const;
    unsigned int GetGroupVisualStateIndex(_In_ unsigned int visualStateIndex) const;
    bool HasStoryboard(_In_ unsigned int visualStateIndex) const;
    bool HasPropertySetters(_In_ unsigned int visualStateIndex) const;
    StreamOffsetToken GetStoryboard(_In_ unsigned int visualStateIndex) const;
    const std::vector<StreamOffsetToken>& GetPropertySetterTokens(_In_ unsigned int visualStateIndex) const;

    bool HasStateTriggers(_In_ unsigned int visualStateIndex) const;
    const std::vector<std::vector<int>>& GetStateTriggers(_In_ unsigned int visualStateIndex) const;
    const std::vector<StreamOffsetToken>& GetExtensibleStateTriggerTokens(_In_ unsigned int visualStateIndex) const;
    const std::vector<StreamOffsetToken> GetStateTriggerCollectionTokens() const;
    const std::vector<StreamOffsetToken>& GetStaticResourceTriggerTokens(_In_ unsigned int visualStateIndex) const;

    _Success_(return) _Must_inspect_result_ bool TryGetVisualStateIndex(_In_z_ const wchar_t* name, _Out_ unsigned int * visualStateIndex) const;
    size_t GetVisualStateGroupCount() const;
    size_t GetVisualStateCount() const;
    bool HasVisualTransitions(_In_ unsigned int groupIndex) const;
    bool ShouldBailOut() const;
    bool ShouldBailOutForUserControls() const;

    StreamOffsetToken GetEntireGroupCollectionToken() const;
    const xstring_ptr& GetVisualStateName(_In_ unsigned int visualStateIndex) const;

    // The fromIndex can optionally be -1, which represents the null state.
    bool TryGetVisualTransition(_In_ int fromIndex, _In_ unsigned int toIndex, _Out_ StreamOffsetToken* token) const;

    const std::vector<xstring_ptr>& GetSeenNames() const;

    // Test hooks
    std::vector<std::wstring> GetVisualStateNamesForGroup(_In_ unsigned int groupIndex) const;
    std::vector<std::wstring> GetVisualStateGroupNames() const;

    // CustomWriterRuntimeData Serialization/Deserialization
    _Check_return_ HRESULT ToString(_In_ bool verboseData, _Out_ xstring_ptr& strValue) const override;
protected:
    _Check_return_ HRESULT SerializeImpl(_In_ XamlBinaryFormatSubWriter2*, _In_ const std::vector<unsigned int>&) override;
    CustomWriterRuntimeDataTypeIndex GetTypeIndexForSerialization(_In_ const TargetOSVersion& targetOS) const override;

private:
    std::vector<unsigned int> m_visualStateToGroupMap;

    std::vector<VisualStateEssence> m_visualStates;
    std::vector<VisualStateGroupEssence> m_visualStateGroups;
    std::vector<VisualTransitionEssence> m_visualTransitions;

    std::vector<xstring_ptr> m_seenNameDirectives;

    bool m_unexpectedTokensDetected;
    StreamOffsetToken m_entireCollectionToken;
    CustomWriterRuntimeDataTypeIndex m_version;

    // unique_ptr to both support boost::optional-esque delayed init
    // and to avoid making VTTOL part of the public includes for the Deferral
    // component.
    std::unique_ptr<VisualTransitionTableOptimizedLookup> m_visualTransitionLookup;
};


