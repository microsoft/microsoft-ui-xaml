// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class XamlType;
class XamlProperty;

class XamlScannerFrame 
{
public:
    XamlScannerFrame(const std::shared_ptr<XamlType>& inXamlType)
        : m_spXamlType(inXamlType)
        , m_bPreserveSpace(FALSE)
        , m_bInContent(FALSE)
    {
    }

    XamlScannerFrame()
        : m_bPreserveSpace(FALSE)
        , m_bInContent(FALSE)
    {
    }

    const std::shared_ptr<XamlType>& get_XamlType() const { return m_spXamlType; }
    void set_XamlType(const std::shared_ptr<XamlType>& inXamlType) { ASSERT(!!inXamlType); m_spXamlType = inXamlType; }

    const std::shared_ptr<XamlProperty>& get_XamlProperty() const { return m_spXamlProperty; }
    void set_XamlProperty(const std::shared_ptr<XamlProperty>& inXamlProperty) { m_spXamlProperty = inXamlProperty; }

    const bool& get_XmlSpacePreserve() const { return m_bPreserveSpace; }
    void set_XmlSpacePreserve(bool bPreserveSpace) { m_bPreserveSpace = bPreserveSpace; }

    const bool& get_InContent() const { return m_bInContent; }
    void set_InContent(bool bInContent) { m_bInContent = bInContent; }

private:
    std::shared_ptr<XamlType> m_spXamlType;
    std::shared_ptr<XamlProperty> m_spXamlProperty;
    bool m_bPreserveSpace;
    bool m_bInContent;
};

class XamlScannerStack 
{
public:
    XamlScannerStack()
    {
    }

    HRESULT Push(const std::shared_ptr<XamlType>& inType);
    HRESULT Pop();
    XUINT32 Depth() const { return m_Stack.size(); }

    const std::shared_ptr<XamlType>& get_CurrentType() const
    { 
        ASSERT(m_itTopOfStack != m_Stack.end()); 
        return m_itTopOfStack->get_XamlType(); 
    }

    const std::shared_ptr<XamlProperty>& get_CurrentProperty() const
    { 
        ASSERT(m_itTopOfStack != m_Stack.end()); 
        return m_itTopOfStack->get_XamlProperty(); 
    }

    void set_CurrentProperty(const std::shared_ptr<XamlProperty>& inProperty) 
    { 
        ASSERT(m_itTopOfStack != m_Stack.end()); 
        m_itTopOfStack->set_XamlProperty(inProperty); 
    }

    const bool& get_CurrentXmlSpacePreserve() const
    {
        ASSERT(m_itTopOfStack != m_Stack.end()); 
        return m_itTopOfStack->get_XmlSpacePreserve(); 
    }

    void set_CurrentXmlSpacePreserve(const bool& bSpacePreserve)
    {
        ASSERT(m_itTopOfStack != m_Stack.end()); 
        m_itTopOfStack->set_XmlSpacePreserve(bSpacePreserve); 
    }

    bool get_CurrentlyInContent() const
    {
        if (m_Stack.size() == 0)
        {
            return false;
        }

        ASSERT(m_itTopOfStack != m_Stack.end()); 
        return m_itTopOfStack->get_InContent(); 
    }

    void set_CurrentlyInContent(bool bInContent)
    {
        ASSERT(m_itTopOfStack != m_Stack.end()); 
        m_itTopOfStack->set_InContent(bInContent); 
    }

private:
    xstack<XamlScannerFrame> m_Stack;
    xstack<XamlScannerFrame>::iterator m_itTopOfStack;
};

