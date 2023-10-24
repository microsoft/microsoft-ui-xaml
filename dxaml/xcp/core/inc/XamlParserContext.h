// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "XamlContext.h"

class XamlType;
class XamlProperty;

// Given that the XAML parser returns a stream of XamlNodes intead of a
// parse tree, we need to store the context of parents and ancestors
// necsesary to resolve namespaces, type info, etc.  The context works by
// adding and remove frames as we begin and finish parsing the elements
// they correspond to.
class XamlParserFrame
{
public:
    enum ElementKind
    {
        ekNone,
        ekEmptyElement,
        ekElement,
        ekPropertyElement
    };

private:
    std::shared_ptr<XamlType> m_Type;
    std::shared_ptr<XamlType> m_PreviousChildType;
    std::shared_ptr<XamlProperty> m_Member;
    tNamespaceMap m_Namespaces;
    ElementKind m_ElementKind;
    bool m_bInPropertyElement : 1;
    bool m_bSeenRealProperties : 1;
    bool m_bInItemsProperty : 1;
    bool m_bInInitProperty : 1;
    bool m_bInUnknownContent : 1;
    bool m_bInCollectionFromMember : 1;
    bool m_bInImplicitContent : 1;
    bool m_bParentIsPropertyElement : 1;
    bool m_inConditionalProperty : 1;
    bool m_inConditionalElement : 1;

public:

    XamlParserFrame()
        : m_ElementKind(XamlParserFrame::ekNone)
        , m_bSeenRealProperties(false)
        , m_bInItemsProperty(false)
        , m_bInInitProperty(false)
        , m_bInUnknownContent(false)
        , m_bInPropertyElement(false)
        , m_bInCollectionFromMember(false)
        , m_bInImplicitContent(false)
        , m_bParentIsPropertyElement(false)
        , m_inConditionalProperty(false)
        , m_inConditionalElement(false)
    {
    }

    void SetNamespaces(const tNamespaceMap& inNamespaces)
    {
        m_Namespaces = inNamespaces;
    }

    ElementKind get_ElementKind() const
    {
        // This is because properties don't push the stack.
        return m_bInPropertyElement
                    ? ekPropertyElement
                    : m_ElementKind;
    }

    void set_ElementKind(const ElementKind& inCurrentKind)
    {
        ASSERT(inCurrentKind != ekPropertyElement);

        m_bInPropertyElement = false;
        m_ElementKind = inCurrentKind;
    }

    const std::shared_ptr<XamlType>& get_Type() const
    {
        ASSERT(m_ElementKind != ekNone);
        return m_Type;
    }

    void set_Type(const std::shared_ptr<XamlType>& inType)
    {
        m_Type = inType;
    }

    bool get_SeenRealProperties() const
    {
        ASSERT(m_ElementKind != ekNone);
        return m_bSeenRealProperties;
    }

    void set_SeenRealProperties(const bool& bSeenRealProperties)
    {
        m_bSeenRealProperties = bSeenRealProperties;
    }

    const std::shared_ptr<XamlType>& get_PreviousChildType() const
    {
        ASSERT(m_ElementKind != ekNone);
        return m_PreviousChildType;
    }

    void set_PreviousChildType(const std::shared_ptr<XamlType>& inPreviousChildType)
    {
        m_PreviousChildType = inPreviousChildType;
    }

    bool get_InItemsProperty() const
    {
        ASSERT(m_ElementKind != ekNone);
        return m_bInItemsProperty;
    }

    void set_InItemsProperty(bool bInItemsProperty)
    {
        m_bInItemsProperty = bInItemsProperty;
    }

    const std::shared_ptr<XamlProperty>& get_Member() const
    {
        ASSERT(m_ElementKind != ekNone);
        return m_Member;
    }

    void set_Member(const std::shared_ptr<XamlProperty>& inMember)
    {
        m_Member = inMember;
    }

    bool get_InInitProperty() const
    {
        ASSERT(m_ElementKind != ekNone);
        return m_bInInitProperty;
    }

    void set_InInitProperty(const bool& bInInitProperty)
    {
        m_bInInitProperty = bInInitProperty;
    }

    bool get_InUnknownContent() const
    {
        ASSERT(m_ElementKind != ekNone);
        return m_bInUnknownContent;
    }

    void set_InUnknownContent(_In_ const bool bInUnknownContent)
    {
        m_bInUnknownContent = bInUnknownContent;
    }

    bool get_InPropertyElement() const
    {
        ASSERT(m_ElementKind != ekNone);
        return m_bInPropertyElement;
    }

    void set_InPropertyElement(const bool& bInPropertyElement)
    {
        m_bInPropertyElement = bInPropertyElement;
    }

    bool get_InCollectionFromMember() const
    {
        ASSERT(m_ElementKind != ekNone);
        return m_bInCollectionFromMember;
    }

    void set_InCollectionFromMember(const bool& bInCollectionFromMember)
    {
        m_bInCollectionFromMember = bInCollectionFromMember;
    }

    bool get_InImplicitContent() const
    {
        ASSERT(m_ElementKind != ekNone);
        return m_bInImplicitContent;
    }

    void set_InImplicitContent(const bool& bInImplicitContent)
    {
        m_bInImplicitContent = bInImplicitContent;
    }

    bool get_ParentIsPropertyElement() const
    {
        ASSERT(m_ElementKind != ekNone);
        return m_bParentIsPropertyElement;
    }

    void set_ParentIsPropertyElement(const bool& bParentIsPropertyElement)
    {
        m_bParentIsPropertyElement = bParentIsPropertyElement;
    }

    bool get_InConditionalProperty() const
    {
        return m_inConditionalProperty;
    }

    void set_InConditionalProperty(const bool& inConditionalProperty)
    {
        m_inConditionalProperty = inConditionalProperty;
    }

    bool get_InConditionalElement() const
    {
        return m_inConditionalElement;
    }

    void set_InConditionalElement(const bool& inConditionalElement)
    {
        m_inConditionalElement = inConditionalElement;
    }

    std::shared_ptr<XamlNamespace> GetNamespaceByPrefix(_In_ const xstring_ptr& inPrefix) const
    {
        auto itNamespace = m_Namespaces->find(inPrefix);
        if (itNamespace == m_Namespaces->end())
        {
            return std::shared_ptr<XamlNamespace>();
        }
        else
        {
            return itNamespace->second;
        }
    }
};

class XamlParserContext final
    : public XamlContext
{
private:
    std::vector<XamlParserFrame> m_parserStack;

    tNamespaceMap m_PrescopeNamespaces;

    // THe BOOL param is not really used. Set to FALSE for now.
    containers::vector_map<xstring_ptr, bool> m_IgnoredNamespaceUris;

public:
    XamlParserContext(const std::shared_ptr<XamlSchemaContext>& inSchemaContext);

    static HRESULT Create(
                _In_ const std::shared_ptr<XamlSchemaContext>& inSchemaContext,
                _In_ std::shared_ptr<XamlParserContext>& outParserContext);

    HRESULT AddNamespacePrefix(
                _In_ const xstring_ptr& inPrefix,
                _In_ const std::shared_ptr<XamlNamespace>& inXamlNamespace) override;

    std::shared_ptr<XamlNamespace> FindNamespaceByPrefix(
                _In_ const xstring_ptr& inPrefix) override;

    virtual void AddIgnoredUri(_In_ const xstring_ptr& inUriToIgnore);
    virtual bool IsNamespaceUriIgnored(_In_ const xstring_ptr& uri);

    void PushScope(bool fPushNamescopes = true);
    void PopScope()
    {
        m_parserStack.pop_back();
    }

    bool IsStackEmpty() const
    {
        return m_parserStack.empty();
    }

    XamlParserFrame::ElementKind get_CurrentKind() const
    {
        return m_parserStack.back().get_ElementKind();
    }

    void set_CurrentKind(const XamlParserFrame::ElementKind& inCurrentKind)
    {
        m_parserStack.back().set_ElementKind(inCurrentKind);
    }

    bool get_CurrentInPropertyElement() const
    {
        return (m_parserStack.empty())
                    ? false
                    : m_parserStack.back().get_InPropertyElement();
    }

    void set_CurrentInPropertyElement(const bool& bCurrentInPropertyElement)
    {
        m_parserStack.back().set_InPropertyElement(bCurrentInPropertyElement);
    }

    bool get_ParentIsPropertyElement() const
    {
        return (m_parserStack.empty())
                    ? false
                    : m_parserStack.back().get_ParentIsPropertyElement();
    }

    void set_ParentIsPropertyElement(const bool& bParentIsPropertyElement)
    {
        m_parserStack.back().set_ParentIsPropertyElement(bParentIsPropertyElement);
    }

    const std::shared_ptr<XamlType>& get_CurrentType() const
    {
        return m_parserStack.back().get_Type();
    }

    void set_CurrentType(const std::shared_ptr<XamlType>& inCurrentType)
    {
        m_parserStack.back().set_Type(inCurrentType);
    }

    bool get_CurrentSeenRealProperties() const
    {
        return m_parserStack.back().get_SeenRealProperties();
    }

    void set_CurrentSeenRealProperties(const bool& bSeenRealProperties)
    {
        m_parserStack.back().set_SeenRealProperties(bSeenRealProperties);
    }

    const std::shared_ptr<XamlType>& get_CurrentPreviousChildType() const
    {
        return m_parserStack.back().get_PreviousChildType();
    }

    void set_CurrentPreviousChildType(const std::shared_ptr<XamlType>& inCurrentPreviousChildType)
    {
        m_parserStack.back().set_PreviousChildType(inCurrentPreviousChildType);
    }

    bool get_CurrentInItemsProperty() const
    {
        return (m_parserStack.empty())
                    ? false
                    : m_parserStack.back().get_InItemsProperty();
    }

    void set_CurrentInItemsProperty(_In_ const bool& bCurrentInItemsProperty)
    {
        m_parserStack.back().set_InItemsProperty(bCurrentInItemsProperty);
    }

    const std::shared_ptr<XamlProperty>& get_CurrentMember() const
    {
        return m_parserStack.back().get_Member();
    }

    void set_CurrentMember(_In_ const std::shared_ptr<XamlProperty>& inCurrentMember)
    {
        m_parserStack.back().set_Member(inCurrentMember);
    }

    bool get_CurrentInInitProperty() const
    {
        return m_parserStack.back().get_InInitProperty();
    }

    void set_CurrentInInitProperty(_In_ const bool& bCurrentInInitProperty)
    {
        m_parserStack.back().set_InInitProperty(bCurrentInInitProperty);
    }

    bool get_CurrentInUnknownContent() const
    {
        return m_parserStack.back().get_InUnknownContent();
    }

    void set_CurrentInUnknownContent(_In_ bool bCurrentInUnknownContent)
    {
        m_parserStack.back().set_InUnknownContent(bCurrentInUnknownContent);
    }

    bool get_CurrentInCollectionFromMember() const
    {
        return (m_parserStack.empty())
                    ? false
                    : m_parserStack.back().get_InCollectionFromMember();
    }

    void set_CurrentInCollectionFromMember(_In_ const bool& bCurrentInCollectionFromMember)
    {
        m_parserStack.back().set_InCollectionFromMember(bCurrentInCollectionFromMember);
    }

    bool get_CurrentInImplicitContent() const
    {
        return (m_parserStack.empty())
                    ? false
                    : m_parserStack.back().get_InImplicitContent();
    }

    void set_CurrentInImplicitContent(_In_ const bool& bCurrentInImplicitContent)
    {
        m_parserStack.back().set_InImplicitContent(bCurrentInImplicitContent);
    }

    bool get_CurrentInConditionalProperty() const
    {
        return (m_parserStack.empty())
            ? false
            : m_parserStack.back().get_InConditionalProperty();
    }

    void set_CurrentInConditionalProperty(_In_ const bool& currentInConditionalProperty)
    {
        m_parserStack.back().set_InConditionalProperty(currentInConditionalProperty);
    }

    bool get_CurrentInConditionalElement() const
    {
        return (m_parserStack.empty())
            ? false
            : m_parserStack.back().get_InConditionalElement();
    }

    void set_CurrentInConditionalElement(_In_ const bool& currentInConditionalElement)
    {
        m_parserStack.back().set_InConditionalElement(currentInConditionalElement);
    }
};

