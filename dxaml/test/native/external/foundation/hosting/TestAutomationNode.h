// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// This is a helper type to allow a test to construct a tree of automation nodes.
struct TestAutomationNode : 
    public Microsoft::WRL::RuntimeClass<
            Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::WinRtClassicComMix>,
            IRawElementProviderSimple,
            IRawElementProviderFragment,
            IRawElementProviderFragmentRoot>
{
    // IRawElementProviderSimple methods
    HRESULT STDMETHODCALLTYPE get_ProviderOptions(_Out_ ProviderOptions * pRetVal) override
    {
        LOG_UIA(L"[get_ProviderOptions]");
        *pRetVal = ProviderOptions_ServerSideProvider | ProviderOptions_UseComThreading;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetPatternProvider(_In_ PATTERNID patternId, _Out_ IUnknown ** pRetVal) override
        {
        LOG_UIA(L"[GetPatternProvider]");
        *pRetVal = nullptr;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetPropertyValue(_In_ PROPERTYID propertyId, _Out_ VARIANT * pRetVal) override
        {
        LOG_UIA(L"[GetPropertyValue]");
        if (propertyId == UIA_NamePropertyId)
        {
            pRetVal->vt = VT_BSTR;
            pRetVal->bstrVal = SysAllocString(m_name->Data());
        }
        else if (propertyId == UIA_FrameworkIdPropertyId)
        {
            pRetVal->vt = VT_BSTR;
            pRetVal->bstrVal = SysAllocString(L"TestTree");
        }
        else if (propertyId == UIA_ClassNamePropertyId)
        {
            pRetVal->vt = VT_BSTR;
            pRetVal->bstrVal = SysAllocString(L"TestClass");   
        }
        else
        {
            pRetVal->vt = VT_EMPTY;
        }
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE get_HostRawElementProvider(_Out_ IRawElementProviderSimple ** pRetVal) override
    {
        LOG_UIA(L"[get_HostRawElementProvider]");
        if (m_isRoot)
        {
            CopyObjectTo(m_island->GetAutomationHostProvider(), pRetVal);
            return S_OK;
        }
        *pRetVal = nullptr;
        return S_OK;
    }

    // IRawElementProviderFragment implementation.

    HRESULT __stdcall Navigate(
        _In_ NavigateDirection direction,
        _COM_Outptr_opt_result_maybenull_ ::IRawElementProviderFragment** fragment) override
        {
            LOG_UIA(L"[Navigate]");

            if (m_isRoot)
            {
                if (direction == NavigateDirection_Parent)
                {
                    CopyObjectTo(m_island->ParentAutomationProvider, fragment);
                    return S_OK;
                }
                if (direction == NavigateDirection_NextSibling)
                {
                    CopyObjectTo(m_island->NextSiblingAutomationProvider, fragment);
                    return S_OK;
                }
                if (direction == NavigateDirection_PreviousSibling)
                {
                    CopyObjectTo(m_island->PreviousSiblingAutomationProvider, fragment);
                    return S_OK;
                }
            }


            if (direction == NavigateDirection_Parent)
            {
                if (m_isRoot)
                {
                    *fragment = nullptr;
                    return S_OK;
                }

                if (m_parent)
                {
                    LOG_UIA(L"[Navigate Parent] %s : Return %s", m_name->Data(), m_parent->m_name->Data());
                    m_parent.CopyTo(fragment);
                    return S_OK;
                }
                else if (m_island)
                {
                    auto p = m_island->ParentAutomationProvider;
                    if (p)
                    {
                        LOG_UIA(L"[Navigate Parent] %s : Return m_island->ParentAutomationProvider", m_name->Data());
                        CopyObjectTo(p, fragment);
                        return S_OK;
                    }
                }
            }
            else if (direction == NavigateDirection_FirstChild)
            {
                if (m_children.size() > 0)
                {
                    //LOG_UIA(L"[Navigate FirstChild] %s : Return %p", m_name->Data(), m_children[0]->m_name->Data());
                    m_children[0].CopyTo(fragment);
                    return S_OK;
                }
            }
            else if (direction == NavigateDirection_LastChild)
            {
                if (m_children.size() > 0)
                {
                    auto child = m_children[m_children.size() - 1];
                    //LOG_UIA(L"[Navigate LastChild] %s : Return %s", m_name->Data(), child->m_name->Data());
                    child.CopyTo(fragment);
                    return S_OK;
                }
            }
            else if (direction == NavigateDirection_NextSibling)
            {
                if (m_parent != nullptr)
                {
                    auto it = std::find(m_parent->m_children.begin(), m_parent->m_children.end(), this);
                    if (it != m_parent->m_children.end() && it + 1 != m_parent->m_children.end())
                    {
                        //LOG_UIA(L"[Navigate NextSibling] %s", m_name->Data());
                        (it + 1)->CopyTo(fragment);
                        return S_OK;
                    }
                }
            }
            else if (direction == NavigateDirection_PreviousSibling)
            {
                if (m_parent != nullptr)
                {
                    auto it = std::find(m_parent->m_children.begin(), m_parent->m_children.end(), this);
                    if (it != m_parent->m_children.end() && it != m_parent->m_children.begin())
                    {
                        //LOG_UIA(L"[Navigate PreviousSibling] %s", m_name->Data());
                        (it - 1)->CopyTo(fragment);
                        return S_OK;
                    }
                }
            }
            LOG_UIA(L"[Navigate %d] %s : Return null", direction, m_name->Data());
            *fragment = nullptr;
            return S_OK;
        }

    HRESULT __stdcall GetRuntimeId(
        _Outptr_opt_result_maybenull_ SAFEARRAY** runtimeId) override
        {
            LOG_UIA(L"[GetRuntimeId]");
            int rId[] = { UiaAppendRuntimeId, m_id };
            SAFEARRAY *psa = SafeArrayCreateVector(VT_I4, 0, 2);
            
            for (LONG i = 0; i < 2; i++)
            {
                SafeArrayPutElement(psa, &i, (void*)&(rId[i]));
            }
            
            *runtimeId = psa;
            return S_OK;
        }

    HRESULT __stdcall get_BoundingRectangle(
        _Out_ UiaRect* boundingRectangle) override
        {
            LOG_UIA(L"[get_BoundingRectangle]");
            *boundingRectangle = GetBoundingRectangleInScreenCoodinates();
            return S_OK;
        }

    HRESULT __stdcall GetEmbeddedFragmentRoots(
        _Outptr_opt_result_maybenull_ SAFEARRAY** embeddedFragmentRoots) override
        {
            LOG_UIA(L"[GetEmbeddedFragmentRoots]");
            *embeddedFragmentRoots = nullptr;
            return S_OK;
        }

    HRESULT __stdcall SetFocus() override
        {
            LOG_UIA(L"[SetFocus]");
            return S_OK;
        }

    HRESULT __stdcall get_FragmentRoot(
        _COM_Outptr_opt_result_maybenull_ ::IRawElementProviderFragmentRoot** root) override
        {
            if (m_root)
            {
                LOG_UIA(L"[get_FragmentRoot] %s : Return %s", m_name->Data(), m_root->m_name->Data());
                m_root.CopyTo(root);
                return S_OK;
            }
            else if (m_island)
            {
                if (auto p = m_island->FragmentRootAutomationProvider)
                {
                    LOG_UIA(L"[get_FragmentRoot] %s : Return m_island->FragmentRootAutomationProvider", m_name->Data());
                    CopyObjectTo(p, root);
                    return S_OK;
                }
            }
            *root = nullptr;
            LOG_UIA(L"[get_FragmentRoot] %s : Return null", m_name->Data());
            return S_OK;
        }

    // IRawElementProviderFragmentRoot implementation.
    HRESULT __stdcall ElementProviderFromPoint(
        _In_ double x,
        _In_ double y,
        _COM_Outptr_opt_result_maybenull_ ::IRawElementProviderFragment** fragment) override
        {         
            LOG_UIA(L"[ElementProviderFromPoint]");

            auto rect = GetBoundingRectangleInScreenCoodinates();

            if (rect.left <= x && x <= rect.left + rect.width
                && rect.top <= y && y <= rect.top + rect.height)
            {
                // Iterate through children to find the one that contains the point
                for (auto& child : m_children)
                {
                    wrl::ComPtr<::IRawElementProviderFragmentRoot> m_root;
                    child.As(&m_root);
                    if (m_root)
                    {
                        m_root->ElementProviderFromPoint(x, y, fragment);
                        if (*fragment)
                        {
                            return S_OK;
                        }
                    }
                }
                *fragment = this;
                this->AddRef();
            }
            else
            {
                *fragment = nullptr;
            }
            return S_OK;
        }

    HRESULT __stdcall GetFocus(
        _COM_Outptr_opt_result_maybenull_ ::IRawElementProviderFragment** fragmentInFocus) override
        {
            LOG_UIA(L"[GetFocus]");
            *fragmentInFocus = nullptr;
            return S_OK;
        }

    // Helper methods

    void SetBoundingRectangle(Windows::Foundation::Rect rect)
    {
        m_localBoundingRect = rect;
    }

    UiaRect GetBoundingRectangleInScreenCoodinates()
    {
        auto converter = m_island->CoordinateConverter;
        auto rect = converter->ConvertLocalToScreen(m_localBoundingRect);
        const UiaRect result = { rect.X, rect.Y, rect.Width, rect.Height };
        return result;
    }

    bool m_isRoot {false};
    int m_id {};
    wrl::ComPtr<TestAutomationNode> m_root;
    wrl::ComPtr<TestAutomationNode> m_parent;
    std::vector<wrl::ComPtr<IRawElementProviderFragment>> m_children;
    Platform::String^ m_name;
    Microsoft::UI::Content::ContentIsland^ m_island;
    Windows::Foundation::Rect m_localBoundingRect {};
};

bool operator==(Microsoft::WRL::ComPtr<IRawElementProviderFragment>& a, IRawElementProviderFragment* b)
{    
    return a.Get() == b;
}