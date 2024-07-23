// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "InputServices.h"
#include "ManipulationTransform.h"
#include "ScrollViewer.h"

using namespace Microsoft::WRL::Wrappers;

DManipDataBase::DManipDataBase()
    : m_manipulationContentType(XcpDMContentTypePrimary)
    , m_contentOffsetX(0.0f)
    , m_contentOffsetY(0.0f)
{
}

_Check_return_ HRESULT DManipDataBase::SetManipulationContent(_In_opt_ IObject* pManipulationContent, XDMContentType contentType)
{
    if (m_spManipulationContent != pManipulationContent)
    {
        if (m_spManipulationContent != nullptr)
        {
            IFC_RETURN(m_spDMService->ReleaseSharedContentTransform(m_spManipulationContent, m_manipulationContentType));
        }
        m_spManipulationContent = pManipulationContent;
        m_manipulationContentType = contentType;
        IFC_RETURN(SetSharedContentTransforms(nullptr, nullptr));
    }

    return S_OK;
}

XDMContentType DManipDataBase::GetManipulationContentType() const
{
    // The caller better not be asking for content type if there's no content set
    ASSERT(m_spManipulationContent != nullptr);
    return m_manipulationContentType;
}

_Check_return_ HRESULT DManipDataBase::SetClipContent(_In_opt_ IObject* clipContent)
{
    if (m_spClipContent != clipContent)
    {
        if (m_spClipContent != nullptr)
        {
            IFC_RETURN(m_spDMService->ReleaseSharedContentTransform(m_spClipContent, XcpDMContentTypeDescendant));
        }
        m_spClipContent = clipContent;
        m_spSharedClipTransform.reset();
    }

    return S_OK;
}

// Update the stored DCompV1 primary shared content transforms (which are the mechanism for shairing data with DManip,
// even for DManipDataWinRT) if they have changed. Also notify ScrollViewer which might need to update the exposed ManipulationTransform PropertySet
_Check_return_ HRESULT DManipDataBase::SetSharedContentTransformsHelper(
    _In_opt_ IUnknown* sharedPrimaryContentTransform,
    _In_opt_ IUnknown* sharedSecondaryContentTransform,
    _Out_ bool* pPrimaryContentTransformChanged,
    _Out_ bool* pSecondaryContentTransformChanged)
{
    *pPrimaryContentTransformChanged = m_spSharedPrimaryContentTransform != sharedPrimaryContentTransform;
    *pSecondaryContentTransformChanged = m_spSharedSecondaryContentTransform != sharedSecondaryContentTransform;

    if (*pPrimaryContentTransformChanged || *pSecondaryContentTransformChanged)
    {
        m_spSharedPrimaryContentTransform = sharedPrimaryContentTransform;
        m_spSharedSecondaryContentTransform = sharedSecondaryContentTransform;

        if (m_manipulationContentType == XcpDMContentTypePrimary && m_manipulatedElement != nullptr)
        {
            if (m_spSharedPrimaryContentTransform)
            {
                CInputServices *inputServices = m_manipulatedElement->GetContext()->GetInputServices();
                ASSERT(inputServices);
                CUIElement *dmContainerElement = inputServices->GetPrimaryContentDMContainer(m_manipulatedElement);
                ASSERT(dmContainerElement && dmContainerElement->OfTypeByIndex<KnownTypeIndex::ScrollViewer>());
                auto scrollViewer = static_cast<CScrollViewer*>(dmContainerElement);
                m_sharedContentTransformAwareScrollViewer = xref::get_weakref(scrollViewer);
                IFC_RETURN(scrollViewer->OnSharedContentTransformChanged(m_manipulatedElement));
            }
            else if (CScrollViewer *scrollViewer = m_sharedContentTransformAwareScrollViewer.lock_noref())
            {
                // Use cached pointer as GetPrimaryContentDMContainer may return 0 if this is a delayed comp node removal.
                m_sharedContentTransformAwareScrollViewer.reset();
                IFC_RETURN(scrollViewer->OnSharedContentTransformChanged(m_manipulatedElement));
            }
        }
    }
    return S_OK;
}

// Ensure we remove the DManip visual/shared transform and release DManip related shared pointers
_Check_return_ HRESULT DManipDataBase::ReleaseManipulationData()
{
    // Only call this method if HasManipulationData() is TRUE
    ASSERT(m_spDMService != nullptr);

    IFC_RETURN(SetManipulationContent(nullptr, XcpDMContentTypePrimary));
    IFC_RETURN(SetClipContent(nullptr));
    m_spDMService.reset();

    return S_OK;
}

_Check_return_ HRESULT DManipDataBase::SetContentOffsetsHelper(_In_ float contentOffsetX, _In_ float contentOffsetY, _Out_ bool* pContentOffsetChanged)
{
    *pContentOffsetChanged = m_contentOffsetX != contentOffsetX || m_contentOffsetY != contentOffsetY;

    if (*pContentOffsetChanged)
    {
        m_contentOffsetX = contentOffsetX;
        m_contentOffsetY = contentOffsetY;

        if (CScrollViewer *scrollViewer = m_sharedContentTransformAwareScrollViewer.lock_noref())
        {
            IFC_RETURN(scrollViewer->OnContentOffsetChanged(m_manipulatedElement));
        }
    }
    return S_OK;
}

DManipDataWinRT::~DManipDataWinRT()
{
    if (HasManipulationData())
    {
        VERIFYHR(ReleaseManipulationData());
    }
}

bool DManipDataWinRT::HasSharedManipulationTransform(_In_ bool targetsClip)
{
    return targetsClip ? GetSharedClipTransform() != nullptr : GetOverallContentPropertySet() != nullptr;
}

_Check_return_ HRESULT DManipDataWinRT::SetContentOffsets(_In_ float contentOffsetX, _In_ float contentOffsetY)
{
    bool contentOffsetChanged = false;

    IFC_RETURN(DManipDataBase::SetContentOffsetsHelper(contentOffsetX, contentOffsetY, &contentOffsetChanged));

    if (contentOffsetChanged)
    {
        m_spOverallContentPropertySet.reset();
    }
    return S_OK;
}

// The overall PropertySet is the target of an expression animation with multiple pieces, depending on the situation.
_Check_return_ HRESULT DManipDataWinRT::EnsureOverallContentPropertySet(_In_ WUComp::ICompositor* pCompositor)
{
    if (m_spSharedPrimaryContentTransformCO && !m_spOverallContentPropertySet)
    {
        ASSERT(pCompositor);
        IFC_RETURN(pCompositor->CreatePropertySet(m_spOverallContentPropertySet.ReleaseAndGetAddressOf()));
        IFC_RETURN(m_spOverallContentPropertySet->InsertMatrix4x4(HStringReference(L"Matrix").Get(), { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 }));

        // Update Xaml-computed offset (constant in expression)
        wfn::Matrix4x4 prependTransform = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, m_contentOffsetX, m_contentOffsetY, 0, 1 };

        xref_ptr<WUComp::ICompositionObject> spOverallContentPropertySetCO;
        IFC_RETURN(m_spOverallContentPropertySet->QueryInterface(IID_PPV_ARGS(spOverallContentPropertySetCO.ReleaseAndGetAddressOf())));

        // Update DManip's animated shared transform (reference in expression)
        // The expression can take two possible forms:
        if (m_spSharedSecondaryContentTransformCO)
        {
            // In this case we have both a primary and secondary transform and require a more complex expression, which looks like:
            // "targetPS.Matrix = ContentOffsetTransform * DManipSecondaryTransform.Matrix * DManipPrimaryTransform.Matrix"
            IFC_RETURN(::ConnectComplexAnimationWithPrependTransform(spOverallContentPropertySetCO.get(), m_spSharedPrimaryContentTransformCO.get(), m_spSharedSecondaryContentTransformCO.get(), prependTransform, L"Matrix"));
        }
        else
        {
            // In this case we only have a primary transform and can use the simpler expression, which looks like:
            // "targetPS.Matrix = ContentOffsetTransform * DManipTransform.Matrix"
            IFC_RETURN(::ConnectAnimationWithPrependTransform(spOverallContentPropertySetCO.get(), m_spSharedPrimaryContentTransformCO.get(), prependTransform, L"Matrix"));
        }
    }

    return S_OK;
}

// Call base helper and also update stored DCompv2 representation of primary content transform
_Check_return_ HRESULT DManipDataWinRT::SetSharedContentTransforms(
    _In_opt_ IUnknown* sharedPrimaryContentTransform,
    _In_opt_ IUnknown* sharedSecondaryContentTransform,
    _In_opt_ WUComp::ICompositor* pCompositor)
{
    bool primaryContentTransformChanged = false;
    bool secondaryContentTransformChanged = false;

    IFC_RETURN(DManipDataBase::SetSharedContentTransformsHelper(
        sharedPrimaryContentTransform,
        sharedSecondaryContentTransform,
        &primaryContentTransformChanged,
        &secondaryContentTransformChanged));

    // Create and store CompositionObject wrapping shared primary content.
    if (primaryContentTransformChanged || secondaryContentTransformChanged)
    {
        Microsoft::WRL::ComPtr<WUComp::IInteropCompositorPartner> interopCompositorPartner;
        if (pCompositor != nullptr)
        {
            IFC_RETURN(pCompositor->QueryInterface(IID_PPV_ARGS(&interopCompositorPartner)));
        }

        if (sharedPrimaryContentTransform != nullptr)
        {
            IFC_RETURN(m_spSharedPrimaryContentTransform->QueryInterface(IID_PPV_ARGS(m_spSharedPrimaryContentTransformCO.ReleaseAndGetAddressOf())));
        }
        else
        {
            m_spSharedPrimaryContentTransformCO.reset();
        }

        if (sharedSecondaryContentTransform != nullptr)
        {
            IFC_RETURN(m_spSharedSecondaryContentTransform->QueryInterface(IID_PPV_ARGS(m_spSharedSecondaryContentTransformCO.ReleaseAndGetAddressOf())));
        }
        else
        {
            m_spSharedSecondaryContentTransformCO.reset();
        }

        m_spOverallContentPropertySet.reset();
    }

    return S_OK;
}

// Call base helper and also update stored DCompv2 representation of primary clip transform
_Check_return_ HRESULT DManipDataWinRT::SetSharedClipTransform(
    _In_opt_ IUnknown* sharedClipTransform,
    _In_opt_ WUComp::ICompositor* pCompositor)
{
    if (m_spSharedClipTransform != sharedClipTransform)
    {
        m_spSharedClipTransform = sharedClipTransform;

        if (sharedClipTransform != nullptr)
        {
            IFC_RETURN(m_spSharedClipTransform->QueryInterface(IID_PPV_ARGS(m_spClipPrimaryTransformCO.ReleaseAndGetAddressOf())));
        }
        else
        {
            m_spClipPrimaryTransformCO.reset();
        }
    }

    return S_OK;
}

_Check_return_ HRESULT DManipDataWinRT::SetClipContent(_In_opt_ IObject* clipContent)
{
    if (m_spClipContent != clipContent)
    {
        if (m_spClipPrimaryTransformCO.get() != nullptr) { m_spClipPrimaryTransformCO.reset(); }
        IFC_RETURN(DManipDataBase::SetClipContent(clipContent));
    }

    return S_OK;
}
