// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class XamlType;
class XamlProperty;
struct XamlQualifiedObject;
class XamlSchemaContext;
class ObjectWriterContext;
class ObjectWriter;
class ICustomWriter;
struct ICustomWriterCallbacks;

// Enum indicating which element is currently being processed
enum class ActivatorTrigger
{
    ObjectStart,
    MemberEnd
};

// State kept by CustomWriterManager which is passed to CustomWriterManangerActivator instances.
// The state is kept by CWM in one place to avoid having each activator keep track of state.
class CustomWriterActivatorState
{
public:
    typedef std::pair<xstring_ptr, std::shared_ptr<XamlNamespace>> NamespacePair;

    CustomWriterActivatorState()
    {
        reset();
    }

    void reset()
    {
        m_namespaces.clear();
        m_objectType.reset();
        m_trigger = ActivatorTrigger::ObjectStart;
        ResetProperty();
    }

    void ResetProperty()
    {
        m_property.reset();
        m_value.reset();
        m_fromMember = false;
    }

    void SwapInNamespaces(
        _Inout_ std::vector<NamespacePair>& namespaces)
    {
        m_namespaces.clear();
        std::swap(m_namespaces, namespaces);
    }

    const std::vector<NamespacePair> GetNamespaces() const
    {
        return m_namespaces;
    }

    void SetObject(
        _In_ const std::shared_ptr<XamlType>& objectType,
        _In_ bool fromMember)
    {
        m_objectType = objectType;
        m_fromMember = fromMember;
    }

    std::shared_ptr<XamlType> GetObject(
        _Out_opt_ bool* pFromMember) const
    {
        if (pFromMember)
        {
            *pFromMember = m_fromMember;
        }

        return m_objectType;
    }

    void SetProperty(
        _In_ const std::shared_ptr<XamlProperty>& xamlProperty)
    {
        m_property = xamlProperty;
    }

    std::shared_ptr<XamlProperty> GetProperty() const
    {
        return m_property;
    }

    void SetValue(
        _In_ const std::shared_ptr<XamlQualifiedObject>& value)
    {
        m_value = value;
    }

    std::shared_ptr<XamlQualifiedObject> GetValue() const
    {
        return m_value;
    }

    void SetTrigger(
        _In_ ActivatorTrigger trigger)
    {
        m_trigger = trigger;
    }

    ActivatorTrigger GetTrigger() const
    {
        return m_trigger;
    }

private:
    std::vector<NamespacePair> m_namespaces;
    std::shared_ptr<XamlType> m_objectType;
    bool m_fromMember;
    std::shared_ptr<XamlProperty> m_property;
    std::shared_ptr<XamlQualifiedObject> m_value;
    ActivatorTrigger m_trigger;
};

// Interface for instantiation of CustomWriters.  It is responsible for determining is given current state
// CustomWriter should be instantiated and actual instantiation of writer.  It also provides a way to
// adjust the state of node stream collecting writer if custom writer requires it.
struct ICustomWriterActivator
{
    virtual ~ICustomWriterActivator() {}

    // Determines if based on current state activator is interested in collecting custom data.
    virtual _Check_return_ HRESULT ShouldActivateCustomWriter(
        _In_ const CustomWriterActivatorState& activatorState,
        _Out_ bool* pResult) = 0;

    // Instantiates and initializes node stream collecting writer.
    virtual _Check_return_ HRESULT CreateNodeStreamCollectingWriter(
        _In_ const CustomWriterActivatorState& activatorState,
        _Out_ std::shared_ptr<ObjectWriter>* pWriter) = 0;

    // Instantiates and initializes custom writer.
    virtual _Check_return_ HRESULT CreateCustomWriter(
        _In_ ICustomWriterCallbacks* pCallbacks,
        _In_ const CustomWriterActivatorState& activatorState,
        _Out_ std::unique_ptr<ICustomWriter>* pWriter) = 0;
};
