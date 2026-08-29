// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <ObjectWriterCallbacksTemplateParentSetter.h>

#include <namescope\inc\NameScopeRoot.h>

HRESULT ObjectWriterCallbacksTemplateParentSetter::OnObjectCreated(
    _In_ const std::shared_ptr<XamlQualifiedObject>& qoRoot, _In_ const std::shared_ptr<XamlQualifiedObject>& qoNewInstance)
{
    ASSERT(!!qoNewInstance);
    auto pObject = qoNewInstance->GetDependencyObject();
    auto lockedParent = m_templatedParent.lock();
    if (pObject && lockedParent)
    {
        IFC_RETURN(pObject->SetTemplatedParent(lockedParent.get()));
        // ItemPanelTemplate doesn't have a template namescope. In this case we allow its
        // children to behave normally, EXCEPT in preThreshold apps where namescopes were actually broken
        // and the children of a ItemPanelTemplate didn't register anywhere.
        
        if (m_nameScopeType == Jupiter::NameScoping::NameScopeType::TemplateNameScope)
        {
            pObject->SetIsTemplateNamescopeMember(TRUE);
        }
    }
    return S_OK;
}

