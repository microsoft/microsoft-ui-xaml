// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

struct IPALDirectManipulationService;

// Helper class to manage DManip-related data stored on HWCompTreeNode.
// DManipDataBase manages the shared DCompV1 primary/secondary content transforms as well as
// the shared clip transform. This is how DManip informs us of expected rendering changes to manipulatable content
// as a result of DManip-handled input.
class DManipDataBase
{
public:
    DManipDataBase();
    virtual ~DManipDataBase() {};

    void SetDMService(_In_ IPALDirectManipulationService* pDMService) { m_spDMService = pDMService; }
    IPALDirectManipulationService* GetDMService() const { return m_spDMService; }

    void SetManipulatedElement(_In_opt_ CUIElement* manipulatedElement) { m_manipulatedElement = manipulatedElement; }

    _Check_return_ HRESULT SetManipulationContent(_In_opt_ IObject* pManipulationContent, XDMContentType contentType);
    IObject* GetManipulationContent() const { return m_spManipulationContent; }
    void ResetManipulationContent() { m_spManipulationContent.reset(); }

    XDMContentType GetManipulationContentType() const;

    virtual _Check_return_ HRESULT SetClipContent(_In_opt_ IObject* pClipContent);
    IObject* GetClipContent() const { return m_spClipContent; }
    void ResetClipContent() { m_spClipContent.reset(); }

    IUnknown* GetSharedPrimaryContentTransform() const { return m_spSharedPrimaryContentTransform; }
    IUnknown* GetSharedClipTransform() const { return m_spSharedClipTransform; }

    BOOL HasManipulationData() const { return m_spDMService != nullptr; }
    _Check_return_ HRESULT ReleaseManipulationData();

    float GetDirectManipulationContentOffsetX() const { return m_contentOffsetX; }
    float GetDirectManipulationContentOffsetY() const { return m_contentOffsetY; }

    virtual bool HasSharedManipulationTransform(_In_ bool targetsClip) = 0;
    virtual _Check_return_ HRESULT SetContentOffsets(_In_ float contentOffsetX, _In_ float contentOffsetY) = 0;
    virtual _Check_return_ HRESULT SetSharedContentTransforms(
        _In_opt_ IUnknown* sharedPrimaryContentTransform,
        _In_opt_ IUnknown* sharedSecondaryContentTransform,
        _In_opt_ WUComp::ICompositor* pCompositor = nullptr) = 0;
    virtual _Check_return_ HRESULT SetSharedClipTransform(
        _In_ IUnknown* sharedClipTransform,
        _In_opt_ WUComp::ICompositor* compositor = nullptr) = 0;

protected:
    _Check_return_ HRESULT SetSharedContentTransformsHelper(
        _In_opt_ IUnknown* sharedPrimaryContentTransform,
        _In_opt_ IUnknown* sharedSecondaryContentTransform,
        _Out_ bool* pPrimaryContentTransformChanged,
        _Out_ bool* pSecondaryContentTransformChanged);

    _Check_return_ HRESULT SetContentOffsetsHelper(_In_ float contentOffsetX, _In_ float contentOffsetY, _Out_ bool* pContentOffsetChanged);

protected:
    xref_ptr<IPALDirectManipulationService> m_spDMService;
    xref_ptr<IObject> m_spManipulationContent;
    XDMContentType m_manipulationContentType;
    xref_ptr<IObject> m_spClipContent;

    // Xaml gets DManip transforms from the DirectManipulationCompositor and sets them in the visual tree. Xaml doesn't
    // ever look at or modify these transforms, so they're typed to an IUnknown. Internally these are legacy
    // IDComposition transforms, which are private to IXP.
    xref_ptr<IUnknown> m_spSharedPrimaryContentTransform;
    xref_ptr<IUnknown> m_spSharedSecondaryContentTransform;
    xref_ptr<IUnknown> m_spSharedClipTransform;

    CUIElement* m_manipulatedElement = nullptr;
    xref::weakref_ptr<CScrollViewer> m_sharedContentTransformAwareScrollViewer;
    float m_contentOffsetX;
    float m_contentOffsetY;
};

// Manages the overall DManip contribution in WinRT Composition (DCompV2) format
// This is based on expression animation referencing wrapped DCompV1 shared transforms
class DManipDataWinRT : public DManipDataBase
{
public:
    bool HasSharedManipulationTransform(_In_ bool targetsClip) override;
    _Check_return_ HRESULT SetContentOffsets(_In_ float contentOffsetX, _In_ float contentOffsetY) override;
    _Check_return_ HRESULT SetSharedContentTransforms(
        _In_opt_ IUnknown* sharedPrimaryContentTransform,
        _In_opt_ IUnknown* sharedSecondaryContentTransform,
        _In_opt_ WUComp::ICompositor* pCompositor = nullptr) override;

    // For content DManip trasform, we maintain our own PS. It stores the DManip primary transform contribution and Xaml's content offset.
    _Check_return_ HRESULT EnsureOverallContentPropertySet(_In_ WUComp::ICompositor* pCompositor);
    WUComp::ICompositionPropertySet* GetOverallContentPropertySet() const { return m_spOverallContentPropertySet; }

    // We use the same pattern to update the DManip-driven clip transform as we did for the DManip-driven content transform.
    // However, the clip case is simpler in that we only need the primary shared transform (unlike the content case where it had to be comnined with ContentOffset).
    // Therefore, for clip tranform we omit managing an intermedate expression (equivalent of m_spOverallContentPropertySet / EnsureOverallContentPropertySet()).
    _Check_return_ HRESULT SetSharedClipTransform(
        _In_ IUnknown* sharedClipTransform,
        _In_opt_ WUComp::ICompositor* pCompositor = nullptr) override;
    WUComp::ICompositionObject* GetClipPrimaryTransformCO() const { return m_spClipPrimaryTransformCO; }

    _Check_return_ HRESULT SetClipContent(_In_opt_ IObject* pClipContent) override;

    ~DManipDataWinRT() override;

private:
    // ::Windows::UI::Composition wrappers (DCompV2) enable us to use DManip's shared transforms in expressions
    //
    // Note: the objects obtained from DManip via CreateManipulationTransform are of type CompositionObejct, and conecptually represent a PropertySet,
    //       but appears to be opaque in that QI for ICompositionPropertySet will fail. They are meant to be used as a reference parameter,
    //       which works since CA::SetReferenceParameter() takes a CO.
    xref_ptr<WUComp::ICompositionObject> m_spSharedPrimaryContentTransformCO;    // Primary Content Transform (CO)- Wraps an opaque PropertySet with several properties (Translation/Pan/Scale/CenterPoint/Matrix)
    xref_ptr<WUComp::ICompositionObject> m_spSharedSecondaryContentTransformCO;  // Secondary Content Transform (CO), optional, when present, is combined with Primary Content Transform
    xref_ptr<WUComp::ICompositionPropertySet> m_spOverallContentPropertySet;     // Overall Content Transform (PS) - PropertySet used in expression that combines the Dmanip primary transform and Xaml offset
    xref_ptr<WUComp::ICompositionObject> m_spClipPrimaryTransformCO;             // Clip Primary Transform (CO) - Wraps an opaque PropertySet with several properties (Translation/Pan/Scale/CenterPoint/Matrix)
};