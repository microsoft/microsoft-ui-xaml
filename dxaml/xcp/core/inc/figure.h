// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CPathSegmentCollection;

//------------------------------------------------------------------------
//
//  Class:  CPathFigure
//
//  Synopsis:
//      Object created for <PathFigure> tag
//
//------------------------------------------------------------------------

class CPathFigure final : public CDependencyObject
{
private:
    CPathFigure(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {}

    ~CPathFigure() override;

public:
    // Creation method
    DECLARE_CREATE(CPathFigure);

    // CDependencyObject overrides

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CPathFigure>::Index;
    }

    // CPathFigure fields

    CPathSegmentCollection* m_pSegments = nullptr;
    XPOINTF m_ptStart = {};
    bool    m_bClosed = false;
    bool    m_bFilled = true;

    //-----------------------------------------------------------------------------
    // D2D Methods/Fields
    //-----------------------------------------------------------------------------
public:
    _Check_return_ HRESULT AddAcceleratedFigure(
        _In_ IPALGeometrySink *pPALGeometrySink
    );
};

//------------------------------------------------------------------------
//
//  Class:  CPathFigureCollection
//
//  Synopsis:
//      A collection of path figures
//
//------------------------------------------------------------------------

class CPathFigureCollection final : public CDOCollection
{
private:
    CPathFigureCollection(_In_ CCoreServices *pCore)
        : CDOCollection(pCore)
    {}

public:
    // Creation method
    DECLARE_CREATE(CPathFigureCollection);

    // CDependencyObject overrides

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CPathFigureCollection>::Index;
    }

    // CCollection Methods
    _Check_return_ bool NeedsOwnerInfo() override { return true; }

    //-----------------------------------------------------------------------------
    // D2D Methods/Fields
    //-----------------------------------------------------------------------------
public:
    _Check_return_ HRESULT AddAcceleratedFigures(
        _In_ IPALGeometrySink *pPALGeometrySink
    );
};
