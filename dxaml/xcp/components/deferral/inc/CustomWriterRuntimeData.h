// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <StreamOffsetToken.h>

class XamlBinaryFormatSubWriter2;
class XamlBinaryFormatSubReader2;
class ObjectWriterStack;
class ObjectWriterNodeList;
class SubObjectWriterResult;
class xstring_ptr;
enum class CustomWriterRuntimeDataTypeIndex : std::uint16_t;
struct TargetOSVersion;

namespace Parser
{
struct XamlPredicateAndArgs;
}

// This object is handed back to the CustomWriterManager through the ICustomWriterCallbacks
// interface. It contains all the data the CustomWriter extracted from the
// XAML node stream and will be handed to the constructed type for storage at use
// at runtime.
class CustomWriterRuntimeData
{
public:
    CustomWriterRuntimeData()
        : m_shouldEncodeAsCustomData(true)
    {
    }

    virtual ~CustomWriterRuntimeData() {};

    _Check_return_
    HRESULT Serialize(_In_ XamlBinaryFormatSubWriter2* writer, _In_ const std::vector<unsigned int>& streamOffsetTable);

    _Check_return_
    static HRESULT Deserialize(_In_ XamlBinaryFormatSubReader2* reader, _In_ std::unique_ptr<CustomWriterRuntimeData>& runtimeData);

    _Check_return_
    virtual HRESULT ToString(_In_ bool verboseData, _Out_ xstring_ptr& strValue) const = 0;

    virtual bool ShouldShare() const { return false; }

    // PrepareStream allows a CustomWriterRuntimeData instance to modify the nodestream its attach to before having it persisted
    // to disk. This allows for optimizations to occur that make use of details known only after we've built up the
    // CustomRuntimeData. One might argue that a method like this doesn't belong on a class that is intended to only 
    // serialize/deserialize data and that it would be better placed on the corresponding writer. I wouldn't disagree with them.
    _Check_return_
    HRESULT virtual PrepareStream(_In_ std::shared_ptr<SubObjectWriterResult>& customWriterStream);

    void SetShouldEncodeAsCustomData(_In_ bool shouldEncode)
    {
        m_shouldEncodeAsCustomData = shouldEncode;
    }

    // Indicates if the data stream should use a custom data blob. For degenerate cases (e.g. resource
    // dictionaries with small cardinalities) the overhead of CustomRuntimeData is greater than the savings.
    // In other scnearios we might be using the CustomWriter machinery for purposes that involve manipulation
    // of the node stream outside of creating CustomRuntimeData. This method will cause the encoder to avoid
    // emitting the custom runtime data ObjectWriterNode.
    bool ShouldEncodeAsCustomData() const
    {
        return m_shouldEncodeAsCustomData;
    }

    // Encode-time methods for helping handling of conditional scopes
    void PushConditionalScope(std::shared_ptr<Parser::XamlPredicateAndArgs> scope)
    {
        m_conditionalScopes.push_back(scope);
    }
    void PopConditionalScope()
    {
        m_conditionalScopes.pop_back();
    }
    bool IsInConditionalScope() const { return !m_conditionalScopes.empty(); }

    void RecordConditionallyDeclaredObject(StreamOffsetToken token);

protected:
    _Check_return_
    virtual HRESULT SerializeImpl(_In_ XamlBinaryFormatSubWriter2* writer, _In_ const std::vector<unsigned int>& streamOffsetTable) = 0;

    virtual CustomWriterRuntimeDataTypeIndex GetTypeIndexForSerialization(_In_ const TargetOSVersion& targetOS) const = 0;

    // Does the input StreamOffsetToken correspond to a conditionally declared object that should be ignored,
    // i.e. does it have an associated XamlPredicateAndArgs that evaluates to false.
    bool IsTokenForIgnoredConditionalObject(StreamOffsetToken token) const;

    // This is a map of StreamOffsetTokens (representing objects that were conditionally declared) to 
    // XamlPredicateAndArgs. The value is a vector of XamlPredicateAndArgs since the object may be part
    // of nested conditional scopes.
    containers::vector_map<StreamOffsetToken, std::vector<std::shared_ptr<Parser::XamlPredicateAndArgs>>> m_conditionallyDeclaredObjects;

    bool m_shouldEncodeAsCustomData;

private:
    std::vector<std::shared_ptr<Parser::XamlPredicateAndArgs>> m_conditionalScopes;
};
