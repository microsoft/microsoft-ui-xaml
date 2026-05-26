// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <corep.h>
#include "InputServices.h"

#include "ContentRootCoordinator.h"

ContentRootCoordinator::ContentRootCoordinator(_In_ CCoreServices& coreServices)
    : m_coreServices(coreServices)
{
}

ContentRootCoordinator::~ContentRootCoordinator()
{
}

xref_ptr<CContentRoot> ContentRootCoordinator::CreateContentRoot(_In_ CContentRoot::Type type, _In_ XUINT32 backgroundColor, _In_opt_ CUIElement* rootElement)
{
    xref_ptr<CContentRoot> contentRoot;
    contentRoot.attach(new CContentRoot(type, backgroundColor, rootElement, m_coreServices));

    m_contentRoots.push_back(contentRoot);
    return contentRoot;
}

void ContentRootCoordinator::RemoveContentRoot(_In_ CContentRoot* contentRoot)
{
    m_contentRoots.erase(
        std::remove(m_contentRoots.begin(), m_contentRoots.end(), contentRoot), m_contentRoots.end());

    if (m_contentRoots.empty())
    {
        // Remove outstanding heap allocations
        m_contentRoots.shrink_to_fit();
    }
}