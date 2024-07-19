// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CParametricCurveSegment : public CDependencyObject
{
private:
    CParametricCurveSegment(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {}

    ~CParametricCurveSegment() override;

public:
    // Creation methods
    DECLARE_CREATE(CParametricCurveSegment);

    _Check_return_ HRESULT GetParametricCurveSegmentDefinition(
        _Inout_ CParametricCurveSegmentDefinition *pDefinition);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CParametricCurveSegment>::Index;
    }

public:
    XFLOAT m_beginOffset            = 0.0f;
    XFLOAT m_constantCoefficient    = 0.0f;
    XFLOAT m_linearCoefficient      = 0.0f;
    XFLOAT m_quadraticCoefficient   = 0.0f;
    XFLOAT m_cubicCoefficient       = 0.0f;
};

//------------------------------------------------------------------------
//
//  Class:  CParametricCurveSegmentCollection
//
//  Synopsis:
//      Collection that holds segments of a parametric curve
//
//------------------------------------------------------------------------

class CParametricCurveSegmentCollection : public CDOCollection
{
private:
    CParametricCurveSegmentCollection(_In_ CCoreServices *pCore)
        : CDOCollection(pCore)
        , m_bIsSorted(FALSE)
    {}

public:
    DECLARE_CREATE(CParametricCurveSegmentCollection);

    void SortCollection();
    
        // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CParametricCurveSegmentCollection>::Index;
    }

private:
    XUINT32 m_bIsSorted : 1;
};

//------------------------------------------------------------------------
//
//  Class:  CParametricCurve
//
//  Synopsis:
//      Object created for <ParametricCurve> tag
//
//------------------------------------------------------------------------

class CParametricCurve : public CDependencyObject
{
private:
    CParametricCurve(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {}

    ~CParametricCurve() override;
    
public:
    static _Check_return_ HRESULT Create(
        _Outptr_result_maybenull_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate
        );
        
    _Check_return_ HRESULT SetPrimaryContentProperty(
        _In_ const xstring_ptr_view& strPrimaryContentProperty);

    _Check_return_ HRESULT SetSecondaryContentProperty(
        _In_ DirectUI::DirectManipulationProperty secondaryContentProperty,
        _In_ const xstring_ptr_view& strSecondaryContentProperty);

    _Check_return_ HRESULT ResolveProperties(
        _In_ CDependencyObject *pPrimaryContent,
        _In_ CDependencyObject *pDependencyPropertyHolder);

    _Check_return_ HRESULT UpdateDependencyProperty(
        _In_ CDependencyObject *pPrimaryContent,
        _In_ CDependencyObject *pDependencyPropertyHolder);

    _Check_return_ HRESULT EvaluateCurve(
        _In_ XFLOAT input,
        _Out_ XFLOAT *pOutput);

    _Check_return_ HRESULT GetParametricCurveDefinition(
        _In_ CUIElement *pPrimaryContent,
        _Inout_ CParametricCurveDefinition *pDefinition);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CParametricCurve>::Index;
    }

private:
    xstring_ptr m_strPrimaryContentPropertyName;
    const CDependencyProperty* m_pPrimaryContentProperty        = nullptr;
    XDMProperty m_secondaryContentProperty                      = XcpDMPropertyNone;
    xstring_ptr m_strAssociatedDependencyPropertyName;
    const CDependencyProperty* m_pAssociatedDependencyProperty  = nullptr;

public:
    CParametricCurveSegmentCollection *m_pCurveSegments         = nullptr;
};

//------------------------------------------------------------------------
//
//  Class:  CParametricCurveCollection
//
//  Synopsis:
//      Collection that holds parametric curves
//
//------------------------------------------------------------------------

class CParametricCurveCollection : public CDOCollection
{
private:
    CParametricCurveCollection(_In_ CCoreServices *pCore)
        : CDOCollection(pCore)
    {}
    
public:
    DECLARE_CREATE(CParametricCurveCollection);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CParametricCurveCollection>::Index;
    }
};

//------------------------------------------------------------------------
//
//  Class:  CSecondaryContentRelationship
//
//  Synopsis:
//      Object created for <SecondaryContentRelationship> tag
//
//------------------------------------------------------------------------

class CSecondaryContentRelationship : public CDependencyObject
{
private:
    CSecondaryContentRelationship(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {}

    ~CSecondaryContentRelationship() override;
    
public:
    static _Check_return_ HRESULT Create(
        _Outptr_result_maybenull_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate
        );
        
    _Check_return_ HRESULT SetPrimaryContent(
        _In_ CUIElement* pPrimaryContent);

    _Check_return_ HRESULT SetSecondaryContent(
        _In_ CUIElement* pSecondaryContent,
        _In_ CDependencyObject* pDependencyPropertyHolder);

    _Check_return_ HRESULT SetAuxiliaryDependencyPropertyHolder(_In_ CDependencyObject* pAuxiliaryDependencyPropertyHolder);

    _Check_return_ HRESULT PrepareForCurveUpdate();

    _Check_return_ HRESULT Apply();

    _Check_return_ HRESULT Remove();

    _Check_return_ HRESULT UpdateDependencyProperties(_In_ bool bManipulationCompleting = false);

    _Check_return_ HRESULT GetParametricCurveDefinitions(
        _Out_ XUINT32 *pcDefinitions,
        _Outptr_result_buffer_maybenull_(*pcDefinitions) CParametricCurveDefinition **ppDefinitions);
    
    xref_ptr<CUIElement> GetPrimaryContent() const;
    
    xref_ptr<CUIElement> GetSecondaryContent() const;
    
    xref_ptr<CDependencyObject> GetDependencyPropertyHolder() const;

    xref_ptr<CDependencyObject> GetAuxiliaryDependencyPropertyHolder() const;

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CSecondaryContentRelationship>::Index;
    }

private:
    // Adds/Releases a tracked reference on the primary content.
    _Check_return_ HRESULT EnsureRefOnPrimaryContent();
    _Check_return_ HRESULT ReleaseRefOnPrimaryContent();

    xref::weakref_ptr<CUIElement> m_primaryContentWeakRef;
    xref::weakref_ptr<CUIElement> m_secondaryContentWeakRef;
    xref::weakref_ptr<CDependencyObject> m_dependencyPropertyHolderWeakRef;
    xref::weakref_ptr<CDependencyObject> m_auxiliaryDependencyPropertyHolderWeakRef;

public:
    CParametricCurveCollection* m_pCurves       = nullptr;
    bool m_shouldTargetClip                     = false;
    bool m_isDescendant                         = true;
    bool m_holdingTrackedRefOnPrimaryContent    = false;
};