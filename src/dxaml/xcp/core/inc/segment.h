// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Synopsis:
//      The base class of all path segments.  This class can not be directly
//  created.

class CPathSegment : public CDependencyObject
{
protected:
    CPathSegment(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {}

public:
// Creation method

    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate);

//-----------------------------------------------------------------------------
// D2D Methods/Fields
//-----------------------------------------------------------------------------
public:
    virtual _Check_return_ HRESULT AddAcceleratedSegment(
        _In_ IPALGeometrySink *pPALGeometrySink
        );
};

//------------------------------------------------------------------------
//
//  Class:  CPathSegmentCollection
//
//  Synopsis:
//      A collection of path segments
//
//------------------------------------------------------------------------

class CPathSegmentCollection final : public CDOCollection
{
private:
    CPathSegmentCollection(_In_ CCoreServices *pCore)
        : CDOCollection(pCore)
    {}

public:
// Creation method

    DECLARE_CREATE(CPathSegmentCollection);

// CDependencyObject overrides

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CPathSegmentCollection>::Index;
    }

//-----------------------------------------------------------------------------
// D2D Methods/Fields
//-----------------------------------------------------------------------------
public:
    _Check_return_ HRESULT AddAcceleratedSegments(
        _In_ IPALGeometrySink *pPALGeometrySink
        );
};

//------------------------------------------------------------------------
//
//  Class:  CLineSegment
//
//  Synopsis:
//      Created by the XML parser to hold a LineSegment object.
//
//------------------------------------------------------------------------

class CLineSegment final : public CPathSegment
{
private:
    CLineSegment(_In_ CCoreServices *pCore)
        : CPathSegment(pCore)
    {}

public:
// Creation method

    DECLARE_CREATE(CLineSegment);

// CDependencyObject overrides

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CLineSegment>::Index;
    }

// CLineSegment fields

    XPOINTF m_pt    = {};

//-----------------------------------------------------------------------------
// D2D Methods/Fields
//-----------------------------------------------------------------------------
public:
    _Check_return_ HRESULT AddAcceleratedSegment(
        _In_ IPALGeometrySink *pPALGeometrySink
        ) override;
};

//------------------------------------------------------------------------
//
//  Class:  CBezierSegment
//
//  Synopsis:
//      Created by the XML parser to hold a BezierSegment object.
//
//------------------------------------------------------------------------

class CBezierSegment final : public CPathSegment
{
private:
    CBezierSegment(_In_ CCoreServices *pCore)
        : CPathSegment(pCore)
    {
        memset(m_apt, 0, 3 * sizeof(XPOINTF));
    }

public:
// Creation method

    DECLARE_CREATE(CBezierSegment);

// CDependencyObject overrides

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CBezierSegment>::Index;
    }

// CBezierSegment fields

public:
    union
    {
        struct
        {
            XPOINTF m_ptOne;
            XPOINTF m_ptTwo;
            XPOINTF m_ptThree;
        };
        XPOINTF m_apt[3];
    };

//-----------------------------------------------------------------------------
// D2D Methods/Fields
//-----------------------------------------------------------------------------
public:
    _Check_return_ HRESULT AddAcceleratedSegment(
        _In_ IPALGeometrySink *pPALGeometrySink
        ) override;
};

//------------------------------------------------------------------------
//
//  Class:  CQuadraticSegment
//
//  Synopsis:
//      Created by the XML parser to hold a QuadraticBezierSegment object.
//
//------------------------------------------------------------------------

class CQuadraticSegment final : public CPathSegment
{
private:
    CQuadraticSegment(_In_ CCoreServices *pCore)
        : CPathSegment(pCore)
    {
        memset(m_apt, 0, 2 * sizeof(XPOINTF));
    }

public:
// Creation method

    DECLARE_CREATE(CQuadraticSegment);

// CDependencyObject overrides

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CQuadraticSegment>::Index;
    }

// CQuadraticSegment fields

    union
    {
        struct
        {
            XPOINTF m_ptOne;
            XPOINTF m_ptTwo;
        };
        XPOINTF m_apt[2];
    };

//-----------------------------------------------------------------------------
// D2D Methods/Fields
//-----------------------------------------------------------------------------
public:
    _Check_return_ HRESULT AddAcceleratedSegment(
        _In_ IPALGeometrySink *pPALGeometrySink
        ) override;
};

//------------------------------------------------------------------------
//
//  Class:  CArcSegment
//
//  Synopsis:
//      Created by the XML parser to hold an ArcSegment object.
//
//------------------------------------------------------------------------

class CArcSegment final : public CPathSegment
{
private:
    CArcSegment(_In_ CCoreServices *pCore)
        : CPathSegment(pCore)
    {
        memset(m_apt, 0, 2 * sizeof(XPOINTF));
    }

public:
// Creation method

    DECLARE_CREATE(CArcSegment);

// CDependencyObject overrides

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CArcSegment>::Index;
    }

// CArcSegment fields

    union
    {
        struct
        {
            XPOINTF m_ptBase;
            XSIZEF m_size;
        };
        XPOINTF m_apt[2];
    };

    XPOINTF m_aptCopy[3];   // Need space for geometry builder to play.
    XFLOAT  m_eAngle        = 0.0f;
    bool    m_bClockwise    = false;
    bool    m_bLarge        = false;

//-----------------------------------------------------------------------------
// D2D Methods/Fields
//-----------------------------------------------------------------------------
public:
    _Check_return_ HRESULT AddAcceleratedSegment(
        _In_ IPALGeometrySink *pPALGeometrySink
        ) override;
};

//------------------------------------------------------------------------
//
//  Class:  CPolyLineSegment
//
//  Synopsis:
//      Created by the XML parser to hold a PolyLineSegment object.
//
//------------------------------------------------------------------------

class CPolyLineSegment final : public CPathSegment
{
private:
    CPolyLineSegment(_In_ CCoreServices *pCore)
        : CPathSegment(pCore)
    {}

   ~CPolyLineSegment() override;

public:
// Creation method

    DECLARE_CREATE(CPolyLineSegment);

// CDependencyObject overrides

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CPolyLineSegment>::Index;
    }

// CPolyLineSegment fields

    CPointCollection* m_pPoints = nullptr;

//-----------------------------------------------------------------------------
// D2D Methods/Fields
//-----------------------------------------------------------------------------
public:
    _Check_return_ HRESULT AddAcceleratedSegment(
        _In_ IPALGeometrySink *pPALGeometrySink
        ) override;
};

//------------------------------------------------------------------------
//
//  Class:  CPolyBezierSegment
//
//  Synopsis:
//      Created by the XML parser to hold a PolyBezierSegment object.
//
//------------------------------------------------------------------------

class CPolyBezierSegment final : public CPathSegment
{
private:
    CPolyBezierSegment(_In_ CCoreServices *pCore)
        : CPathSegment(pCore)
    {}

   ~CPolyBezierSegment() override;

public:
// Creation method

    DECLARE_CREATE(CPolyBezierSegment);

// CDependencyObject overrides

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CPolyBezierSegment>::Index;
    }

// CPolyBezierSegment fields

    CPointCollection* m_pPoints = nullptr;

//-----------------------------------------------------------------------------
// D2D Methods/Fields
//-----------------------------------------------------------------------------
public:
    _Check_return_ HRESULT AddAcceleratedSegment(
        _In_ IPALGeometrySink *pPALGeometrySink
        ) override;
};

//------------------------------------------------------------------------
//
//  Class:  CPolyQuadraticSegment
//
//  Synopsis:
//      Created by the XML parser to hold a PolyQuadraticBezierSegment object.
//
//------------------------------------------------------------------------

class CPolyQuadraticSegment final : public CPathSegment
{
private:
    CPolyQuadraticSegment(_In_ CCoreServices *pCore)
        : CPathSegment(pCore)
    {}

   ~CPolyQuadraticSegment() override;

public:
// Creation method

    DECLARE_CREATE(CPolyQuadraticSegment);

// CDependencyObject overrides

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CPolyQuadraticSegment>::Index;
    }

// CPolyQuadraticSegment fields

    CPointCollection *m_pPoints = nullptr;

//-----------------------------------------------------------------------------
// D2D Methods/Fields
//-----------------------------------------------------------------------------
public:
    _Check_return_ HRESULT AddAcceleratedSegment(
        _In_ IPALGeometrySink *pPALGeometrySink
        ) override;
};
