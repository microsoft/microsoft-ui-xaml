// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <map>
#include <wil\resource.h>
#include <TestEvent.h>
#include "XamlOM.WinUI.h"
#include <algorithm>
#include <unordered_set>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

    struct VisualElementStateDetails
    {
        std::wstring            Message;
        VisualElementState      State = VisualElementState::ErrorResolved;

        bool operator==(const VisualElementStateDetails& rhs) const
        {
            return (rhs.State == State &&
                rhs.Message == Message);
        }

        bool operator!=(const VisualElementStateDetails& rhs) const
        {
            return !(*this == rhs);
        }
    };

        class VisualTreeServiceCallback : public Microsoft::WRL::RuntimeClass<
            Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
            IVisualTreeServiceCallback2,
            Microsoft::WRL::FtmBase>
        {

        public:
            ~VisualTreeServiceCallback()
            {
            };

            HRESULT RuntimeClassInitialize()
            {
                m_hVisualTreeChanged.reset(CreateEvent(
                    nullptr /* lpEventAttributes */,
                    FALSE /* bManualReset */,
                    FALSE /* bInitialState */,
                    nullptr /* lpName */));

                return S_OK;
            }

            IFACEMETHODIMP QueryInterface(
                _In_ REFIID iid,
                _Outptr_ void** object) override
            {
                if (IsEqualGUID(iid, __uuidof(IVisualTreeServiceCallback)))
                {
                    *object = static_cast<IVisualTreeServiceCallback*>(this);
                }
                else if (IsEqualGUID(iid, __uuidof(IVisualTreeServiceCallback2)))
                {
                    *object = static_cast<IVisualTreeServiceCallback2*>(this);
                }
                else if (IsEqualGUID(iid, __uuidof(IUnknown)))
                {
                    *object = static_cast<IUnknown*>(static_cast<IVisualTreeServiceCallback2*>(this));
                }
                else
                {
                    return E_NOINTERFACE;
                }

               AddRef();
               return S_OK;
            }

            IFACEMETHODIMP OnVisualTreeChange(
                    _In_ ParentChildRelation relation,
                    _In_ VisualElement element,
                    _In_ VisualMutationType mutationType)
            {
                WEX::Common::Throw::If(element.Handle == 0, E_FAIL, L"Handle in mutation should not be null");
                if (mutationType == VisualMutationType::Add)
                {
                    if (relation.Parent == 0)
                    {
                        m_rootHandles.emplace(element.Handle);
                    }

                    // This is basically an attach, the memory inside of
                    // element will be managed by the wil::unique_visualelement now
                    wil::unique_visualelement elem;
                    elem.reset(element);
                    m_treeCache.emplace(element.Handle, std::move(elem));
                    m_parentRelations.emplace(element.Handle, relation);
                }
                else
                {
                    m_rootHandles.erase(element.Handle);
                    m_treeCache.erase(element.Handle);
                    m_parentRelations.erase(element.Handle);
                }

                SetEvent(m_hVisualTreeChanged.get());
                return S_OK;
            }

            IFACEMETHODIMP OnElementStateChanged(
                InstanceHandle element,
                VisualElementState errorType,
                _In_z_ LPCWSTR errorContext)
            {
                if (errorType == VisualElementState::ErrorResolved)
                {
                    LOG_OUTPUT(L"Error resolved - Handle: 0x%X", element);
                    m_currentErrors.erase(element);
                }
                else
                {
                    VisualElementStateDetails details{ errorContext, errorType };
                    auto lastError = GetLastError(element);
                    if (lastError != details)
                    {
                        LOG_OUTPUT(L"Error: %s for property %s on 0x%I64X", GetErrorMessage(errorType), errorContext, element);
                        m_currentErrors.emplace(element, details);
                    }
                }
                return S_OK;
            }

            LPCWSTR GetErrorMessage(VisualElementState state)
            {
                switch (state)
                {
                case VisualElementState::ErrorInvalidResource:
                    return L"Invalid resource reference";
                case VisualElementState::ErrorResourceNotFound:
                    return L"Resource not found";
                default:
                    return nullptr;
                }
            }

            bool HasError()
            {
                return !m_currentErrors.empty();
            }
            std::vector<VisualElementStateDetails> GetErrorsForElement(InstanceHandle element)
            {
                auto errorsForElement = m_currentErrors.equal_range(element);
                std::vector<VisualElementStateDetails> errors;
                errors.reserve(m_currentErrors.count(element));
                for (auto iter = errorsForElement.first; iter != errorsForElement.second; ++iter)
                {
                    errors.push_back(iter->second);
                }

                return errors;
            }

            VisualElementStateDetails GetLastError(InstanceHandle element)
            {
                auto range = m_currentErrors.equal_range(element);
                if (range.first == range.second) return {};
                --range.second;
                return range.second->second;
            }

            size_t GetErrorCount(InstanceHandle element = 0u)
            {
                if (element == 0u)
                {
                    return m_currentErrors.size();
                }
                else
                {
                    return m_currentErrors.count(element);
                }
            }

            wil::unique_handle& GetEvent()
            {
                return m_hVisualTreeChanged;
            }

            std::multimap<InstanceHandle, VisualElement> GetTree()
            {
                std::multimap<InstanceHandle, VisualElement> tree;
                for (auto& pair : m_treeCache)
                {
                    InstanceHandle parentHandle = 0;
                    auto rootIter = m_rootHandles.find(pair.first);
                    if (rootIter == m_rootHandles.end())
                    {
                        parentHandle = GetParent(pair.first).Handle;
                    }

                    tree.emplace(parentHandle, pair.second);
                }
                return tree;
            }

            std::vector<InstanceHandle> GetRoots()
            {
                return std::vector<InstanceHandle>(m_rootHandles.begin(), m_rootHandles.end());
            }

            std::vector<InstanceHandle> GetChildren(InstanceHandle parent)
            {
                std::vector<InstanceHandle> children;
                children.reserve(m_treeCache.size()); // Cant be more than the size of the tree

                for (auto& relation : m_parentRelations)
                {
                    if (relation.second.Parent == parent)
                    {
                        if (relation.second.ChildIndex >= children.size())
                        {
                            children.push_back(relation.first);
                        }
                        else
                        {
                            children.insert(children.begin() + relation.second.ChildIndex, relation.first);
                        }
                    }
                }
                children.shrink_to_fit();
                return children;
            }

            VisualElement GetParent(InstanceHandle child)
            {
                auto relation = m_parentRelations.find(child);
                WEX::Common::Throw::If(relation == m_parentRelations.end(), E_NOTFOUND, L"No parent child relation found");
                auto parent = m_treeCache.find(relation->second.Parent);
                if (parent == m_treeCache.end())
                {
                    VisualElement element = GetElementByHandle(child);
                    LOG_OUTPUT(L"   Parent was not found in visual tree for child: %s:%s", element.Type, element.Name);
                    WEX::Common::Throw::If(parent == m_treeCache.end(), E_NOTFOUND, L"Parent was not found in visual tree");
                    return {};
                }
                else
                {
                    return parent->second;
                }
            }

            // numInstancesToSkip allows us to get at elements inside templates
            VisualElement GetElementByName(_In_ LPCWSTR name, int numInstancesToSkip = 0)
            {
                int instancesFound = 0;
                for (const auto& pair : m_treeCache)
                {
                    auto& element = pair.second;
                    if (wcscmp(element.Name, name) == 0 &&
                        instancesFound++ == numInstancesToSkip)
                    {
                        return element;
                    }
                }
                WEX::Common::Throw::Exception(E_NOTFOUND, WEX::Common::String().Format(L"Element not found: %s", name));
                return{};
            }

            VisualElement GetElementInPageByType(_In_ LPCWSTR name, _In_ int numInstancesToSkip = 0)
            {
                // Since we only care about elements that have a page, we will filter out by those
                // that don't have a name
                int instancesFound = 0;
                for (auto& pair : m_treeCache)
                {
                    auto& element = pair.second;
                    if (wcscmp(element.Type, name) == 0 &&
                        wcscmp(element.SrcInfo.FileName, L"") != 0 &&
                        instancesFound++ == numInstancesToSkip)
                    {
                        return element;
                    }
                }

                WEX::Common::Throw::Exception(E_NOTFOUND, L"The requested element was not found");
                return{};
            }

            VisualElement GetElementByHandle(_In_ InstanceHandle element)
            {
                for (auto& pair : m_treeCache)
                {
                    if (pair.first == element)
                    {
                        return pair.second;
                    }
                }
                WEX::Common::Throw::Exception(E_NOTFOUND, L"The requested element was not found");
                return{};
            }

            void ValidateTreeState()
            {
                // For non-root objects, validate that we got tree mutations for their parent. If this fails, it's possible a bug
                // was introduced in how we hand back the tree. Or, the test has a UIElement contained inside a ResourceDictionary.
                // The latter isn't actually a bug, see what ResolveResourceTests::VerifyResolveCustomPropertyWithCustomType did to workaround it.
                for (auto& pair : m_treeCache)
                {
                    auto rootIter = m_rootHandles.find(pair.first);
                    if (rootIter == m_rootHandles.end())
                    {
                        GetParent(pair.first);
                    }
                }

                LOG_OUTPUT(L"Tree properly connected");
            }

            size_t GetNumberOfRoots()
            {
                return m_rootHandles.size();
            }

            void ClearCache()
            {
                m_treeCache.clear();
            }

            bool IsCacheEmpty()
            {
                return m_treeCache.empty();
            }

            size_t GetCacheSize()
            {
                return m_treeCache.size();
            }

            std::pair<InstanceHandle, bool> VerifyTestCleanup()
            {
                // The only item in the tree should be the root window since that hasn't been closed yet.
                // If any root tries to enter the tree, we fail and subscribe to the loaded event for the diagnostics
                // root.
                bool cleanedUp = true;
                InstanceHandle rootHandle = 0;
                for (auto& element : m_treeCache)
                {
                    if (wcscmp(element.second.Type, L"Microsoft.UI.Xaml.Window") != 0 &&
                        wcscmp(element.second.Type, L"Microsoft.UI.Xaml.Hosting.DesktopWindowXamlSource") != 0)
                    {
                        cleanedUp = false;
                        LOG_OUTPUT(L"Element %s of type %s left in tree", element.second.Name, element.second.Type);
                    }
                    else
                    {
                        rootHandle = element.first;
                    }
                }

                return std::make_pair(rootHandle, m_treeCache.size() <= 1 && cleanedUp);
            }

        private:
            wil::unique_handle m_hVisualTreeChanged;
            std::unordered_set<InstanceHandle> m_rootHandles;
            std::map<InstanceHandle, ParentChildRelation> m_parentRelations;
            std::map<InstanceHandle, wil::unique_visualelement> m_treeCache;
            std::multimap<InstanceHandle, VisualElementStateDetails> m_currentErrors;
        };
}}}}}
