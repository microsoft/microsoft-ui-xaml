// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ContentRoot.h"

class CCoreServices;
class CUIElement;

namespace AccessKeys
{
    class AccessKeyExport;
}

class ContentRootCoordinator
{
public:
    friend class CContentRoot;

    ContentRootCoordinator(_In_ CCoreServices& coreServices);
    ~ContentRootCoordinator();

    xref_ptr<CContentRoot> CreateContentRoot(_In_ CContentRoot::Type type, _In_ XUINT32 backgroundColor, _In_opt_ CUIElement* rootElement);

    const std::vector<xref_ptr<CContentRoot>>& GetContentRoots() { return m_contentRoots; }

    CContentRoot* Unsafe_IslandsIncompatible_CoreWindowContentRoot() const { return m_unsafe_IslandsIncompatible_CoreWindowContentRoot; }

private:
    void RemoveContentRoot(_In_ CContentRoot* contentRoot);

    std::vector<xref_ptr<CContentRoot>> m_contentRoots;
    CCoreServices& m_coreServices;

    // Idealy m_unsafe_IslandsIncompatible_CoreWindowContentRoot must be null in Win32/Desktop/islands, but it is not. This has been tracked by Task# 30029924
    CContentRoot* m_unsafe_IslandsIncompatible_CoreWindowContentRoot = nullptr;
};