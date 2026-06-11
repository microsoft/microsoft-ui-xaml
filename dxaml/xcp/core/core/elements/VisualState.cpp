// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <VisualState.h>
#include <animation.h>
#include <duration.h>
#include <Storyboard.h>
#include <cvalue.h>
#include <ObjectWriterCallbacksTemplateParentSetter.h>
#include <AutoReentrantReferenceLock.h>
#include <namescope\inc\NameScopeRoot.h>
#include <VisualStateCollection.h>
#include "XamlQualifiedObject.h"
#include "ObjectWriterSettings.h"
#include "theming\inc\Theme.h"

using namespace DirectUI;

_Check_return_ HRESULT
CVisualState::GetStoryboard(
    _Outptr_result_maybenull_ CStoryboard **ppStoryboard)
{
    IFC_RETURN(TryDelayCreateLegacyStoryboard());

    if(m_pStoryboard)
    {
        *ppStoryboard = m_pStoryboard;
        (*ppStoryboard)->AddRef();
    }
    else
    {
        *ppStoryboard = nullptr;
    }

    return S_OK;
}

_Check_return_ HRESULT
CVisualState::GetValue(
    _In_ const CDependencyProperty *pdp,
    _Out_ CValue *pValue)
{
    // Ensure target object has been resolved (since we lazy resolve it if this TargetPropertyPath
    // was created from a string)
    if (pdp->GetIndex() == KnownPropertyIndex::VisualState_Setters)
    {
        IFC_RETURN(TryDelayCreateLegacyPropertySetters());
    }

    IFC_RETURN(CDependencyObject::GetValue(pdp, pValue));

    return S_OK;
}

_Check_return_ HRESULT
CVisualState::Storyboard(
    _In_ CDependencyObject* pObject,
    _In_ XUINT32 cArgs,
    _In_reads_(cArgs) CValue* pArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue* pResult)
{
    CVisualState* pThis = do_pointer_cast<CVisualState>(pObject);

    if (cArgs == 0)
    {
        IFC_RETURN(pThis->GetStoryboard(&pThis->m_pStoryboard));
        pResult->SetObjectNoRef(pThis->m_pStoryboard);
    }
    else
    {
        CStoryboard* pNewStoryboard = nullptr;
        CStoryboard* pOldStoryboard = nullptr;

        IFC_RETURN((pArgs->GetType() == valueObject || pArgs->GetType() == valueNull) ? S_OK : E_INVALIDARG);

        if (pArgs->AsObject())
        {
            pNewStoryboard = do_pointer_cast<CStoryboard>(pArgs->AsObject());
            IFC_RETURN(pNewStoryboard ? S_OK : E_INVALIDARG);
        }

        if (pThis->m_pStoryboard)
        {
            pThis->UnparentAndLeaveDO(pThis->m_pStoryboard);
        }
        if (pNewStoryboard)
        {
            pThis->ParentAndEnterDO(pNewStoryboard);
        }

        pOldStoryboard = pThis->m_pStoryboard;
        {
            // m_pStoryboard is walked for GC, so take GC lock before it is changed
            AutoReentrantReferenceLock lock(DXamlServices::GetPeerTableHost());
            pThis->m_pStoryboard = pNewStoryboard;
        }
        ReleaseInterface(pOldStoryboard);
        AddRefInterface(pThis->m_pStoryboard);
    }

    return S_OK;
}

_Check_return_ HRESULT CVisualState::TryDelayCreateLegacyStoryboard()
{
    xref_ptr<CDependencyObject> result;

    if (!m_pStoryboard && m_pDeferredStoryboard)
    {
        IFC_RETURN(CreateDeferredItemFromTemplateContent(m_pDeferredStoryboard, result.ReleaseAndGetAddressOf()));

        if (result && result->OfTypeByIndex<KnownTypeIndex::Storyboard>())
        {
            m_pStoryboard = static_cast<CStoryboard*>(result.detach());
            ParentAndEnterDO(m_pStoryboard);
        }
    }
    return S_OK;
}

_Check_return_ HRESULT
CVisualState::TryDelayCreateLegacyPropertySetters()
{
    if (!m_setters && m_pDeferredSetters)
    {
        xref_ptr<CDependencyObject> result;
        IFC_RETURN(CreateDeferredItemFromTemplateContent(m_pDeferredSetters, result.ReleaseAndGetAddressOf()));

        if (result && result->OfTypeByIndex<KnownTypeIndex::SetterBaseCollection>())
        {
            m_setters = static_cast<CSetterBaseCollection*>(result.detach());
            ParentAndEnterDO(m_setters);
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CVisualState::CreateDeferredItemFromTemplateContent(
    _In_ CTemplateContent* pTemplateContent,
    _Outptr_ CDependencyObject** result)
{
    auto templateParent = m_templatedParent.lock_noref();

    // Get the current event root for this template
    xref_ptr<CDependencyObject> pEventRoot = pTemplateContent->GetSavedEventRoot();
    std::shared_ptr<XamlQualifiedObject> qoEventRoot;
    if (pEventRoot)
    {
        qoEventRoot = std::make_shared<XamlQualifiedObject>();
        qoEventRoot->SetDependencyObjectNoAddRef(pEventRoot.detach());
    }

    // Set up the ObjectWriterSettings
    ObjectWriterSettings objectWriterSettings;
    objectWriterSettings.set_EventRoot(qoEventRoot);
    objectWriterSettings.set_ObjectWriterCallbacks(
        std::make_shared<ObjectWriterCallbacksTemplateParentSetter>(xref_ptr<CDependencyObject>(templateParent),
        Jupiter::NameScoping::NameScopeType::TemplateNameScope));
    // FUTURE: This is broken right now for two reasons:
    // - We shouldn't really be doing this in CVisualState at all- it would be much better to either use CFrameworkTemplate's
    // machinery directly instead of needlessly duplicating in here or even better using CustomWriterRuntimeObjectCreator to
    // expand XAML, as it's meticulously crafted to minimize any divergence in the object creation pattern that could occur
    // between first parse and deferred parsing, including getting things like namescoping right.
    // - NameScoping is busted here- we never set a INameScope provider.

    // Now that the TemplateContent is set up, create the object it represents
    IFC_RETURN(pTemplateContent->Load(objectWriterSettings, false /* bTryOptimizeContent */, result));

    return S_OK;
}

void CVisualState::SetTemplatedParentImpl(_In_ CDependencyObject* parent)
{
    m_templatedParent = xref::get_weakref(parent);
}

// Reference tracker walk for GC
bool
CVisualState::ReferenceTrackerWalkCore(
    _In_ DirectUI::EReferenceTrackerWalkType walkType,
    _In_ bool isRoot,
    _In_ bool shouldWalkPeer)
{
    bool walked = CDependencyObject::ReferenceTrackerWalkCore(walkType, isRoot, shouldWalkPeer);

    if (walked)
    {
        if (m_pStoryboard)
        {
            m_pStoryboard->ReferenceTrackerWalkCore(walkType,
                false,  //isRoot
                true);  //shouldWalkPeer);
        }
    }

    return walked;
}
