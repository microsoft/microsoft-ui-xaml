// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ContentRenderer.h"

ElementRenderingContext::ElementRenderingContext(
    _In_ IContentRenderer* pContentRenderer,
    _In_ CUIElement* pUIElement,
    _In_ const HWRenderParams* pRenderParams,
    _In_opt_ HWElementRenderParams* pElementRenderParams,
    _In_opt_ const CTransformToRoot* pTransformToRoot)
{
    m_pContentRenderer = pContentRenderer;
    m_pSavedUIElement = pContentRenderer->GetUIElement();
    m_pSavedRenderParams = pContentRenderer->GetRenderParams();
    m_pSavedElementRenderParams = pContentRenderer->GetElementRenderParams();
    m_pSavedTransformToRoot = pContentRenderer->GetTransformToRoot();

    pContentRenderer->SetUIElement(pUIElement);
    pContentRenderer->SetRenderParams(pRenderParams);
    pContentRenderer->SetElementRenderParams(pElementRenderParams);
    pContentRenderer->SetTransformToRoot(pTransformToRoot);
}

ElementRenderingContext::~ElementRenderingContext()
{
    m_pContentRenderer->SetUIElement(m_pSavedUIElement);
    m_pContentRenderer->SetRenderParams(m_pSavedRenderParams);
    m_pContentRenderer->SetElementRenderParams(m_pSavedElementRenderParams);
    m_pContentRenderer->SetTransformToRoot(m_pSavedTransformToRoot);
}

HWRenderParamsOverride::HWRenderParamsOverride(
    _In_ IContentRenderer* pContentRenderer,
    _In_ const HWRenderParams* pRenderParams)
{
    m_pContentRenderer = pContentRenderer;
    m_pSavedRenderParams = pContentRenderer->GetRenderParams();
    pContentRenderer->SetRenderParams(pRenderParams);
}

HWRenderParamsOverride::~HWRenderParamsOverride()
{
    m_pContentRenderer->SetRenderParams(m_pSavedRenderParams);
}

TransformToRoot2DOverride::TransformToRoot2DOverride(
    _In_ IContentRenderer* pContentRenderer,
    _In_ const CTransformToRoot* pTransformToRoot
    )
{
    m_pContentRenderer = pContentRenderer;
    m_pSavedTransformToRoot = pContentRenderer->GetTransformToRoot();
    pContentRenderer->SetTransformToRoot(pTransformToRoot);
}

TransformToRoot2DOverride::~TransformToRoot2DOverride()
{
    m_pContentRenderer->SetTransformToRoot(m_pSavedTransformToRoot);
}

CaptureRenderData::CaptureRenderData(
    _In_ IContentRenderer* pContentRenderer,
    _In_opt_ PCRenderDataList* pRenderDataList,
    _In_ bool fAppendOnly
    )
{
    m_pContentRenderer = pContentRenderer;
    pContentRenderer->PushRenderDataList(pRenderDataList, fAppendOnly);
}

CaptureRenderData::~CaptureRenderData()
{
    m_pContentRenderer->PopRenderDataList();
}

RealizationUpdateContext::RealizationUpdateContext(
    _In_ IContentRenderer* pContentRenderer
    )
{
    m_pContentRenderer = pContentRenderer;
    m_savedUpdatedShapeRealization = m_pContentRenderer->GetUpdatedShapeRealization();
    m_pContentRenderer->SetUpdatedShapeRealization(false);
}

RealizationUpdateContext::~RealizationUpdateContext()
{
    m_pContentRenderer->SetUpdatedShapeRealization(m_savedUpdatedShapeRealization);
}

