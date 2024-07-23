// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ParserTypedefs.h"
#include "CompressedObjectWriterStack.h"

// NOTE: This needs to be included to allow the expansion of the
// DataStructureFunctionProvider<> use *_directive inline methods.
#include "XamlProperty.h"

#include "INamescope.h"
#include <weakref_ptr.h>
#include <bit_vector.h>

struct XamlQualifiedObject;
struct FrameOverflowStorage;
struct FrameLegacyStorage;
class XamlNamespace;
class XamlType;
class XamlProperty;
class INameScope;

class ObjectWriterFrame final
{
    typedef xpairlist<std::shared_ptr<DirectiveProperty>, std::shared_ptr<XamlQualifiedObject> > MapDirectiveToQO;

    struct FrameOverflowStorage
    {
        xref::weakref_ptr<CDependencyObject> m_weakRefInstance;
        tNamespaceMap m_Namespaces;
        SP_MapPropertyToQO m_spReplacementPropertyValues;
        std::shared_ptr<CompressedObjectWriterStack> m_spCompressedStack;
        // for each conditional scope node encountered in this frame, stores whether or not skipping should be triggered
        containers::bit_vector m_conditionalScopesToSkip;

        FrameOverflowStorage() = default;

        FrameOverflowStorage(const FrameOverflowStorage&) = default;
        FrameOverflowStorage& operator=(const FrameOverflowStorage&) = default;

        FrameOverflowStorage(FrameOverflowStorage&& other) noexcept
            : m_weakRefInstance(std::move(other.m_weakRefInstance))
            , m_Namespaces(std::move(other.m_Namespaces))
            , m_spReplacementPropertyValues(std::move(other.m_spReplacementPropertyValues))
            , m_spCompressedStack(std::move(other.m_spCompressedStack))
            , m_conditionalScopesToSkip(std::move(other.m_conditionalScopesToSkip))
        {}

        FrameOverflowStorage& operator=(FrameOverflowStorage&& other)
        {
            if (this != &other)
            {
                m_weakRefInstance = std::move(other.m_weakRefInstance);
                m_Namespaces = std::move(other.m_Namespaces);
                m_spReplacementPropertyValues = std::move(other.m_spReplacementPropertyValues);
                m_spCompressedStack = std::move(other.m_spCompressedStack);
                m_conditionalScopesToSkip = std::move(other.m_conditionalScopesToSkip);
            }

            return *this;
        }
    };

    struct FrameLegacyStorage
    {
        std::shared_ptr<XamlQualifiedObject> m_qoCollection;
        std::shared_ptr<XamlQualifiedObject> m_qoValue;
        std::shared_ptr<std::vector<std::shared_ptr<XamlProperty>>> m_spAssignedProperties;
        std::shared_ptr<MapDirectiveToQO> m_spDirectiveValuesMap;
        bool m_fIsObjectFromMember = false;

        FrameLegacyStorage() = default;

        FrameLegacyStorage(const FrameLegacyStorage&) = default;
        FrameLegacyStorage& operator=(const FrameLegacyStorage&) = default;

        FrameLegacyStorage(FrameLegacyStorage&& other) noexcept
            : m_qoCollection(std::move(other.m_qoCollection))
            , m_qoValue(std::move(other.m_qoValue))
            , m_spAssignedProperties(std::move(other.m_spAssignedProperties))
            , m_spDirectiveValuesMap(std::move(other.m_spDirectiveValuesMap))
            , m_fIsObjectFromMember(std::move(other.m_fIsObjectFromMember))
        {}

        FrameLegacyStorage& operator=(FrameLegacyStorage&& other)
        {
            if (this != &other)
            {
                m_qoCollection = std::move(other.m_qoCollection);
                m_qoValue = std::move(other.m_qoValue);
                m_spAssignedProperties = std::move(other.m_spAssignedProperties);
                m_spDirectiveValuesMap = std::move(other.m_spDirectiveValuesMap);
                m_fIsObjectFromMember = std::move(other.m_fIsObjectFromMember);
            }

            return *this;
        }
    };

public:

    ObjectWriterFrame();

    ObjectWriterFrame(const ObjectWriterFrame& other)
        : m_spType(other.m_spType)
        , m_spMember(other.m_spMember)
        , m_qoInstance(other.m_qoInstance)
    {
        if (other.m_spFrameOverflowStorage)
        {
            m_spFrameOverflowStorage.reset(new FrameOverflowStorage(*other.m_spFrameOverflowStorage));
        }

        if (other.m_spFrameLegacyStorage)
        {
            m_spFrameLegacyStorage.reset(new FrameLegacyStorage(*other.m_spFrameLegacyStorage));
        }
    }

    ObjectWriterFrame& operator=(const ObjectWriterFrame& other)
    {
        if (this != &other)
        {
            m_spType = other.m_spType;
            m_spMember = other.m_spMember;
            m_qoInstance = other.m_qoInstance;

            if (other.m_spFrameOverflowStorage)
            {
                m_spFrameOverflowStorage.reset(new FrameOverflowStorage(*other.m_spFrameOverflowStorage));
            }

            if (other.m_spFrameLegacyStorage)
            {
                m_spFrameLegacyStorage.reset(new FrameLegacyStorage(*other.m_spFrameLegacyStorage));
            }
        }
        return *this;
    }

    ObjectWriterFrame(ObjectWriterFrame&& other) noexcept
        : m_spType(std::move(other.m_spType))
        , m_spMember(std::move(other.m_spMember))
        , m_qoInstance(std::move(other.m_qoInstance))
        , m_spFrameOverflowStorage(std::move(other.m_spFrameOverflowStorage))
        , m_spFrameLegacyStorage(std::move(other.m_spFrameLegacyStorage))
    {
    }

    ObjectWriterFrame& operator=(ObjectWriterFrame&& other) noexcept
    {
        if (this != &other) {
            m_spType = std::move(other.m_spType);
            m_spMember = std::move(other.m_spMember);
            m_qoInstance = std::move(other.m_qoInstance);
            m_spFrameOverflowStorage = std::move(other.m_spFrameOverflowStorage);
            m_spFrameLegacyStorage = std::move(other.m_spFrameLegacyStorage);
        }
        return *this;
    }

    _Check_return_ HRESULT AddNamespacePrefix(
        _In_ const xstring_ptr& inPrefix,
        _In_ const std::shared_ptr<XamlNamespace>& spXamlNamespace);

    std::shared_ptr<XamlNamespace> FindNamespaceByPrefix(
        _In_ const xstring_ptr& inPrefix) const;

    std::shared_ptr<MapDirectiveToQO> get_Directives();

    bool exists_Directives() const
    {
        return m_spFrameLegacyStorage && !!m_spFrameLegacyStorage->m_spDirectiveValuesMap;
    }

    bool contains_Directive(
        _In_ const std::shared_ptr<DirectiveProperty>& inProperty) const
    {
        return m_spFrameLegacyStorage && m_spFrameLegacyStorage->m_spDirectiveValuesMap && m_spFrameLegacyStorage->m_spDirectiveValuesMap->ContainsKey(inProperty);
    }

    _Check_return_ HRESULT tryget_Directive(
        _In_ const std::shared_ptr<DirectiveProperty>& inProperty,
        _Out_ std::shared_ptr<XamlQualifiedObject>& outInstance)
    {
        if (m_spFrameLegacyStorage && m_spFrameLegacyStorage->m_spDirectiveValuesMap)
        {
            return m_spFrameLegacyStorage->m_spDirectiveValuesMap->Get(inProperty, outInstance);
        }
        return S_FALSE;
    }

    _Check_return_ HRESULT add_Directive(
        _In_ const std::shared_ptr<DirectiveProperty>& inProperty,
        _In_ const std::shared_ptr<XamlQualifiedObject>& inInstance)
    {
        EnsureFrameLegacyStorage();
        EnsureDirectives();
        ASSERT(!!m_spFrameLegacyStorage->m_spDirectiveValuesMap);

        IFC_RETURN(m_spFrameLegacyStorage->m_spDirectiveValuesMap->push_back(MapDirectiveToQO::TPair(inProperty, inInstance)));
        return S_OK;
    }

    bool exists_CompressedStack() const
    {
        return m_spFrameOverflowStorage && m_spFrameOverflowStorage->m_spCompressedStack;
    }

    const std::shared_ptr<CompressedObjectWriterStack>& get_CompressedStack() const
    {
        ASSERT (!!m_spFrameOverflowStorage);
        return m_spFrameOverflowStorage->m_spCompressedStack;
    }

    void ensure_CompressedStack()
    {
        EnsureOverflowStorage();
        if (!m_spFrameOverflowStorage->m_spCompressedStack)
        {
            m_spFrameOverflowStorage->m_spCompressedStack = std::make_shared<CompressedObjectWriterStack>();
        }
    }

    bool get_IsPropertyAssigned(
        _In_ const std::shared_ptr<XamlProperty>& inProperty) const;

    void NotifyPropertyAssigned(
        _In_ const std::shared_ptr<XamlProperty>& inProperty);

    const std::shared_ptr<XamlType>& get_Type() const
    {
        return m_spType;
    }

    bool exists_Type() const
    {
        return !!get_Type();
    }

    void set_Type(
        _In_ const std::shared_ptr<XamlType>& spType)
    {
        m_spType = spType;
    }

    const std::shared_ptr<XamlProperty>& get_Member() const
    {
        return m_spMember;
    }

    bool exists_Member() const
    {
        return !!get_Member();
    }

    void clear_Member()
    {
        m_spMember.reset();
    }

    void set_Member(
        _In_ const std::shared_ptr<XamlProperty>& spMember)
    {
        m_spMember = spMember;
    }

    const std::shared_ptr<XamlQualifiedObject>& get_Instance() const
    {
        return m_qoInstance;
    }

    bool exists_Instance() const
    {
        return !!get_Instance();
    }

    void clear_Instance()
    {
        m_qoInstance.reset();
    }

    void set_Instance(
        _In_ const std::shared_ptr<XamlQualifiedObject>& qoInstance)
    {
        // We're setting a strongref to an instance, so clear the existing
        // weakref instance
        clear_WeakRefInstance();
        if (   m_spFrameLegacyStorage
            && m_spFrameLegacyStorage->m_spAssignedProperties
            && m_qoInstance
            && qoInstance != m_qoInstance)
        {
            // MSFT:10437264/10520955: GenXbf is doing something screwy whereby it
            // pseudo-duplicates a CreateType*/SetName node pair, which can result
            // in the appearance of a duplicate property assignment. If we're
            // overwriting a non-null instance (which should only occur in that
            // particular incorrect scenario), then clear the list of assigned
            // properties since technically it applies to the previous instance, not
            // the new one.
            // We only do this for non-null old instances because in typical operation
            // the ObjectWriter will set all the members first *then* create/set the
            // instance object.
            m_spFrameLegacyStorage->m_spAssignedProperties->clear();
        }
        m_qoInstance = qoInstance;
    }

    bool exists_WeakRefInstance() const
    {
        return m_spFrameOverflowStorage && m_spFrameOverflowStorage->m_weakRefInstance;
    }

    const xref::weakref_ptr<CDependencyObject>& get_WeakRefInstance() const
    {
        ASSERT(!exists_Instance());
        ASSERT(!!m_spFrameOverflowStorage);
        return m_spFrameOverflowStorage->m_weakRefInstance;
    }

    void clear_WeakRefInstance()
    {
        if (m_spFrameOverflowStorage)
        {
            m_spFrameOverflowStorage->m_weakRefInstance.reset();
        }
    }

    void set_WeakRefInstance(
        _In_ xref::weakref_ptr<CDependencyObject> weakRefInstance)
    {
        EnsureOverflowStorage();
        // We're setting a weakref to an instance, so clear the existing
        // strongref instance
        // This setter is used as part of weakening the strongref to the existing
        // instance. Therefore, the strongref instance should point to the same thing
        // as the weakref instance.
        // FUTURE: ObjectWriterRuntime::RemoveObjectReferencesFromStack should be
        // refactored so that the weakening is performed entirely by the ObjectWriterFrame
        m_spFrameOverflowStorage->m_weakRefInstance = std::move(weakRefInstance);
        clear_Instance();
    }

    std::shared_ptr<XamlQualifiedObject> get_Collection() const;

    void set_Collection(
        _In_ const std::shared_ptr<XamlQualifiedObject>& qoCollection)
    {
        EnsureFrameLegacyStorage();
        m_spFrameLegacyStorage->m_qoCollection = qoCollection;
    }

    void clear_Collection()
    {
        if (m_spFrameLegacyStorage)
        {
            m_spFrameLegacyStorage->m_qoCollection.reset();
        }
    }

    bool exists_Collection() const
    {
        return m_spFrameLegacyStorage && !!m_spFrameLegacyStorage->m_qoCollection;
    }

    std::shared_ptr<XamlQualifiedObject> get_Value() const;

    bool exists_Value() const
    {
        return m_spFrameLegacyStorage && !!m_spFrameLegacyStorage->m_qoValue;
    }

    void set_Value(
        _In_ const std::shared_ptr<XamlQualifiedObject>& qoValue)
    {
        EnsureFrameLegacyStorage();
        m_spFrameLegacyStorage->m_qoValue = qoValue;
    }

    bool exists_ReplacementPropertyValues() const
    {
        return m_spFrameOverflowStorage && !!m_spFrameOverflowStorage->m_spReplacementPropertyValues;
    }

    const SP_MapPropertyToQO& get_ReplacementPropertyValues() const
    {
        ASSERT(!!m_spFrameOverflowStorage);
        return m_spFrameOverflowStorage->m_spReplacementPropertyValues;
    }

    void set_ReplacementPropertyValues(
        _In_ const SP_MapPropertyToQO& spReplacementPropertyValues)
    {
        EnsureOverflowStorage();
        m_spFrameOverflowStorage->m_spReplacementPropertyValues = spReplacementPropertyValues;
    }

    _Check_return_ HRESULT GetAndRemoveReplacementPropertyValue(
        _In_ const std::shared_ptr<XamlProperty>& spProperty,
        _Out_ std::shared_ptr<XamlQualifiedObject>& qoValue);

    bool get_IsObjectFromMember() const
    {
        return m_spFrameLegacyStorage && m_spFrameLegacyStorage->m_fIsObjectFromMember;
    }

    void  set_IsObjectFromMember(
        _In_ bool value
        )
    {
        EnsureFrameLegacyStorage();
        m_spFrameLegacyStorage->m_fIsObjectFromMember = value;
    }

    // Returns whether or not the current frame has at least one conditional
    // scope associated with it. 'ignoreInactiveScopes' parameter governs
    // whether or not inactive scopes (i.e. those with predicates which evaluate
    // to true) should be considered.
    bool get_HasConditionalScopeToSkip(bool ignoreInactiveScopes) const
    {
        if (m_spFrameOverflowStorage)
        {
            if (!ignoreInactiveScopes)
            {
                return !m_spFrameOverflowStorage->m_conditionalScopesToSkip.empty();
            }
            else
            {
                return !m_spFrameOverflowStorage->m_conditionalScopesToSkip.all_false();
            }
        }

        return false;
    }

    void PushConditionalScopeNode(bool shouldSkip)
    {
        EnsureOverflowStorage();
        m_spFrameOverflowStorage->m_conditionalScopesToSkip.push_back(shouldSkip);
    }

    void PopConditionalScopeNode()
    {
        EnsureOverflowStorage();

        if (!m_spFrameOverflowStorage->m_conditionalScopesToSkip.empty())
        {
            m_spFrameOverflowStorage->m_conditionalScopesToSkip.pop_back();
        }
        else
        {
            ASSERT(false);
        }
    }

    void EnsureOverflowStorage()
    {
        if (!m_spFrameOverflowStorage)
        {
            m_spFrameOverflowStorage.reset(new FrameOverflowStorage);
        }
    }

    void EnsureFrameLegacyStorage()
    {
        if (!m_spFrameLegacyStorage)
        {
            m_spFrameLegacyStorage.reset(new FrameLegacyStorage);
        }
    }

private:
    void EnsureDirectives();

private:
    std::shared_ptr<XamlType> m_spType;
    std::shared_ptr<XamlProperty> m_spMember;
    std::shared_ptr<XamlQualifiedObject> m_qoInstance;
    std::unique_ptr<FrameOverflowStorage> m_spFrameOverflowStorage;
    std::unique_ptr<FrameLegacyStorage> m_spFrameLegacyStorage;
};
