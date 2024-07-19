// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Define as 1 (i.e. XCP_TRACE_OUTPUT_MSG) to get DirectManipulation debug outputs, and 0 otherwise
#define DMCNTNT_DBG 0

class CSecondaryContentRelationship;

class CDMContent : public CXcpObjectBase<IObject>
{
public:
    // ------------------------------------------------------------------------
    // CDMContent Public Methods
    // ------------------------------------------------------------------------
    static _Check_return_ HRESULT Create(
        _In_ CUIElement* pContentElement,
        _In_ XDMContentType contentType,
        _Outptr_ CDMContent** ppContent);

    CUIElement* GetContentElementNoRef() const
    {
        return m_pContentElement;
    }

    XDMContentType GetContentType() const
    {
        return m_contentType;
    }

    IObject* GetCompositorSecondaryContentNoRef() const
    {
        return m_pCompositorSecondaryContent;
    }

    void SetCompositorSecondaryContent(_In_opt_ IObject* pCompositorSecondaryContent)
    {
#ifdef DM_DEBUG
        if (DMCNTNT_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT))
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMCNTNT_DBG) /*traceType*/, L"DMC[0x%p]:   SetCompositorSecondaryContent - old=0x%p, new=0x%p.",
                this, m_pCompositorSecondaryContent, pCompositorSecondaryContent));
        }
#endif // DM_DEBUG
        ReplaceInterface(m_pCompositorSecondaryContent, pCompositorSecondaryContent);
    }

    void GetInitialTransformationValues(
        _Out_ XFLOAT& initialTranslationX,
        _Out_ XFLOAT& initialTranslationY,
        _Out_ XFLOAT& initialUncompressedZoomFactor,
        _Out_ XFLOAT& initialZoomFactorX,
        _Out_ XFLOAT& initialZoomFactorY)
    {
        initialTranslationX = m_initialTranslationX;
        initialTranslationY = m_initialTranslationY;
        initialUncompressedZoomFactor = m_initialUncompressedZoomFactor;
        initialZoomFactorX = m_initialZoomFactorX;
        initialZoomFactorY = m_initialZoomFactorY;
    }

    void SetInitialTransformationValues(
        _In_ XFLOAT initialTranslationX,
        _In_ XFLOAT initialTranslationY,
        _In_ XFLOAT initialUncompressedZoomFactor,
        _In_ XFLOAT initialZoomFactorX,
        _In_ XFLOAT initialZoomFactorY)
    {
#ifdef DM_DEBUG
        if (DMCNTNT_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT))
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMCNTNT_DBG) /*traceType*/,
                L"DMC[0x%p]:   SetInitialTransformationValues - initialTranslationX=%4.6lf, initialTranslationY=%4.6lf, initialUncompressedZoomFactor=%4.8lf, initialZoomFactorX=%4.8lf, initialZoomFactorY=%4.8lf.",
                this, initialTranslationX, initialTranslationY, initialUncompressedZoomFactor, initialZoomFactorX, initialZoomFactorY));
        }
#endif // DM_DEBUG
        ASSERT(initialUncompressedZoomFactor != 0.0f);
        ASSERT(initialZoomFactorX != 0.0f);
        ASSERT(initialZoomFactorY != 0.0f);
        m_initialTranslationX = initialTranslationX;
        m_initialTranslationY = initialTranslationY;
        m_initialUncompressedZoomFactor = initialUncompressedZoomFactor;
        m_initialZoomFactorX = initialZoomFactorX;
        m_initialZoomFactorY = initialZoomFactorY;
    }

    void GetCurrentTransformationValues(
        _Out_ XFLOAT& translationX,
        _Out_ XFLOAT& translationY,
        _Out_ XFLOAT& uncompressedZoomFactor,
        _Out_ XFLOAT& zoomFactorX,
        _Out_ XFLOAT& zoomFactorY)
    {
        translationX = m_currentTranslationX;
        translationY = m_currentTranslationY;
        uncompressedZoomFactor = m_currentUncompressedZoomFactor;
        zoomFactorX = m_currentZoomFactorX;
        zoomFactorY = m_currentZoomFactorY;
    }

    void SetCurrentTransformationValues(
        _In_ XFLOAT translationX,
        _In_ XFLOAT translationY,
        _In_ XFLOAT uncompressedZoomFactor,
        _In_ XFLOAT zoomFactorX,
        _In_ XFLOAT zoomFactorY)
    {
#ifdef DM_DEBUG
        if (DMCNTNT_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT))
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMCNTNT_DBG) /*traceType*/,
                L"DMC[0x%p]:   SetCurrentTransformationValues - translationX=%4.6lf, translationY=%4.6lf, uncompressedZoomFactor=%4.8lf, zoomFactorX=%4.8lf, zoomFactorY=%4.8lf.",
                this, translationX, translationY, uncompressedZoomFactor, zoomFactorX, zoomFactorY));
        }
#endif // DM_DEBUG
        ASSERT(uncompressedZoomFactor != 0.0f);
        ASSERT(zoomFactorX != 0.0f);
        ASSERT(zoomFactorY != 0.0f);
        m_currentTranslationX = translationX;
        m_currentTranslationY = translationY;
        m_currentUncompressedZoomFactor = uncompressedZoomFactor;
        m_currentZoomFactorX = zoomFactorX;
        m_currentZoomFactorY = zoomFactorY;
    }

    CSecondaryContentRelationship * GetSecondaryContentRelationship()
    {
        return m_pSecondaryContentRelationship;
    }

    void SetSecondaryContentRelationship(
        _In_ CSecondaryContentRelationship *pSecondaryContentRelationship);

    // ------------------------------------------------------------------------
    // CDMContent Protected Constructor/Destructor
    // ------------------------------------------------------------------------
protected:
    CDMContent(
        _In_ CUIElement* pContentElement,
        _In_ XDMContentType contentType)
        : m_pContentElement(pContentElement)
        , m_contentType(contentType)
        , m_pCompositorSecondaryContent(NULL)
        , m_initialTranslationX(0.0f)
        , m_initialTranslationY(0.0f)
        , m_initialUncompressedZoomFactor(1.0f)
        , m_initialZoomFactorX(1.0f)
        , m_initialZoomFactorY(1.0f)
        , m_currentTranslationX(0.0f)
        , m_currentTranslationY(0.0f)
        , m_currentUncompressedZoomFactor(1.0f)
        , m_currentZoomFactorX(1.0f)
        , m_currentZoomFactorY(1.0f)
        , m_pSecondaryContentRelationship(NULL)
    {
        ASSERT(pContentElement);
        AddRefInterface(pContentElement);

#ifdef DM_DEBUG
        if (DMCNTNT_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT))
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMCNTNT_DBG) /*traceType*/,
                L"DMC[0x%p]:   CDMContent constructor with pContentElement=0x%p, contentType=%d.", this, pContentElement, contentType));
        }
#endif // DM_DEBUG
    }

    ~CDMContent() override;

    // ------------------------------------------------------------------------
    // CDMContent Private Fields
    // ------------------------------------------------------------------------
private:
    // UI element associated with this CDMContent instance
    CUIElement* m_pContentElement;

    // Content type: TopLeftHeader or TopHeader or LeftHeader
    XDMContentType m_contentType;

    // Object provided to the compositor for interaction with the
    // IPALDirectManipulationCompositorService interface. It wraps
    // a IDirectManipulationContent pointer for the secondary content.
    IObject* m_pCompositorSecondaryContent;

    // Transformation values at the beginning of the current manipulation.
    XFLOAT m_initialTranslationX;
    XFLOAT m_initialTranslationY;
    XFLOAT m_initialUncompressedZoomFactor;
    XFLOAT m_initialZoomFactorX;
    XFLOAT m_initialZoomFactorY;

    // Latest transformation values recorded at UI thread tick.
    // Used in CInputServices::GetDirectManipulationTransform.
    XFLOAT m_currentTranslationX;
    XFLOAT m_currentTranslationY;
    XFLOAT m_currentUncompressedZoomFactor;
    XFLOAT m_currentZoomFactorX;
    XFLOAT m_currentZoomFactorY;

    // Secondary content that describes Jupiter values to be set
    // when a manipulation completes.
    CSecondaryContentRelationship* m_pSecondaryContentRelationship;
};
