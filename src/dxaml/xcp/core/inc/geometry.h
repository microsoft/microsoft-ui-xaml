// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CDependencyObject.h"
#include "RenderTypes.h"
#include "EnumDefs.g.h"
#include "DependencyObjectTraits.h"
#include "DependencyObjectTraits.g.h"
#include "DOCollection.h"
#include "D2D1.h"
#include "ComTemplates.h"
#include <windows.graphics.interop.h>
#include <microsoft.ui.composition.h>
#include <microsoft.ui.composition.experimental.h>

class CPlainPen;
struct NWRenderParams;
struct D2DRenderParams;
struct D2DPrecomputeParams;
class CWideningSink;
struct AcceleratedBrushParams;
class CCoreServices;
enum PathPointType;
struct IPALAcceleratedGeometry;
struct IPALAcceleratedGraphicsFactory;
struct IPALGeometrySink;
struct IPALAcceleratedPathGeometry;
class CBrush;
class HitTestPolygon;
class CTransform;
class VisualContentRenderer;
class CD2DFactory;
class CD3D11Device;
class WindowsGraphicsDeviceManager;
class CGeometrySource;

// Temporary structure used to build a path geometry

#define BLOCK_LIMIT     113

class CPathBlock
{
public:
    CPathBlock();

    CPathBlock *m_pNext;                //    4 bytes
    XPOINTF     m_aPoint[BLOCK_LIMIT];  // +892 bytes
    XUINT8      m_aType[BLOCK_LIMIT];   // +113 bytes
    XUINT8      m_cPoint;               // +  1 byte
};                                      // 1020 bytes total

// Flags that can be passed to AddLine/AddBezier/AddQuadratic/AddArc

enum XcpPointType
{
    XcpPointType_Absolute,
    XcpPointType_Relative
};

#define VALID_X     0x0002
#define VALID_Y     0x0004
#define VALID_XY    0x0006

class CPathGeometry;
class CPathFigureCollection;
class CPathFigure;

//------------------------------------------------------------------------
//
//  Class:  CGeometryBuilder
//
//  Synopsis:
//      Since we won't know beforehand how many points we'll be in a parsed
//  segment we'll use this helper class to avoid costly reallocates and copies.
//  When the geometry is completed we'll allocate the final buffers to be the
//  exact size required and copy them from these temporary structures.
//
//------------------------------------------------------------------------

class CGeometryBuilder
{
public:
    CGeometryBuilder(_In_ CCoreServices *pCoreServices);
   ~CGeometryBuilder();

    _Check_return_ HRESULT OpenGeometry();
    _Check_return_ HRESULT OpenFigure(_In_ XINT32 flags, _In_ XPOINTF *ppt);
    _Check_return_ HRESULT SetFigureFilled(_In_ bool bFill);
    _Check_return_ HRESULT CloseFigure();
    _Check_return_ HRESULT ComputeReflection(_In_ XINT32 flags, _In_ PathPointType type, _Out_ XPOINTF *ppt);
    _Check_return_ HRESULT AddLine(_In_ XINT32 flags, _In_ XPOINTF *ppt);
    _Check_return_ HRESULT AddBezier(_In_ XINT32 flags, _In_reads_(3) XPOINTF *ppt);
    _Check_return_ HRESULT AddQuadratic(_In_ XINT32 flags, _In_reads_(2) XPOINTF *ppt);
    _Check_return_ HRESULT AddArc(
                            _In_ XINT32 flags,
                            _In_reads_(2) XPOINTF *ppt,
                            _In_ XFLOAT eAngle,
                            _In_ XINT32 bLarge,
                            _In_ XINT32 bClockwise,
                            _In_opt_ const CMILMatrix *pMatrix = NULL );
    _Check_return_ HRESULT AddSegments(_In_ PathPointType type, _In_ XUINT32 cPoint, _In_reads_(cPoint) const XPOINTF *pPoint);
    _Check_return_ HRESULT ReplaceLastPoint(_In_ XPOINTF *ppt);

    static void CalculateRoundedCornersRectangle(
        _Out_writes_(8) XPOINTF *pCorners,
        _In_ const XRECTF& rc,
        _In_ const XCORNERRADIUS& rCornerRadius,
        _In_opt_ const XTHICKNESS *pBorders,
        bool fOuter
        );

    static _Check_return_ HRESULT DrawRoundedCornersRectangle(
        _Outptr_ IPALAcceleratedGeometry **ppPALGeometry,
        _In_ IPALAcceleratedGraphicsFactory *pD2DFactory,
        _In_ const XRECTF& rc,
        _In_ const XCORNERRADIUS& rCornerRadius,
        _In_opt_ const XTHICKNESS *pBorders = NULL,
        bool fOuter = false
        );

    static _Check_return_ HRESULT DrawRoundedCornersRectangle(
        _In_ IPALGeometrySink* pSink,
        _In_ const XRECTF& rc,
        _In_ const XCORNERRADIUS& rCornerRadius,
        _In_opt_ const XTHICKNESS *pBorders = NULL,
        bool fOuter = false
        );

    static _Check_return_ HRESULT DrawMultipleLines(
        _Outptr_ IPALAcceleratedGeometry **ppPALGeometry,
        _In_ IPALAcceleratedGraphicsFactory *pD2DFactory,
        XUINT32 cPoints,
        _In_reads_(cPoints) const XPOINTF *pPoints
        );

    static _Check_return_ HRESULT AddPoints(
        _Inout_ IPALAcceleratedPathGeometry *pPALGeometry,
        _In_reads_(cPoints) XPOINTF* pPoints,
        XUINT32 cPoints,
        _In_reads_(cPoints) XPATHTYPE* pTypes,
        XcpFillMode fillMode
        );

protected:
// Verify if a flag is set or not
    XCP_FORCEINLINE XINT32 VerifyFlag(_In_ XINT32 x, _In_ XINT32 flag)
    {
        return ((x & flag) == flag);
    }

    XCP_FORCEINLINE void MakeAbsolute(_In_ XINT32 flags, _In_range_(1,3) XUINT32 cpt, _In_reads_(cpt) XPOINTF *ppt)
    {
        if (VerifyFlag(flags, XcpPointType_Relative))
        {
            while (cpt)
            {
                cpt--;
                m_aAbsolute[cpt].x = ppt[cpt].x + m_pptCurr->x;
                m_aAbsolute[cpt].y = ppt[cpt].y + m_pptCurr->y;
            }

            m_pAbsolute = m_aAbsolute;
        }
        else
            m_pAbsolute = ppt;
    }

    _Check_return_ HRESULT StorePoints(_In_ PathPointType type, _In_ XUINT32 cPoint);

private:
    CPathBlock *m_pHead;        // First block of points
    CPathBlock *m_pCurr;        // Current block of points
    XUINT32     m_cPoint;       // Total count of points
    XPOINTF     m_ptStart;      // Starting point of current figure
    XPOINTF     m_aAbsolute[3]; // Buffer for absolute point conversion
    XPOINTF    *m_pAbsolute;    // Pointer to absolute points
    XPOINTF    *m_pptCurr;      // Pointer to point that is the current position
    XPOINTF    *m_pptReflect;   // Pointer to point used to compute reflection
    XPOINTF     m_ptLastMove;   // The last move point
    XUINT32     m_typePrev;     // Type of previously added segment
    XINT32      m_bValid;       // Has the initial 'moveto' command been seen?
    XINT32      m_bOpen;        // Are we in an open figure?
    XINT32      m_bPendingMove; // Do we have a deferred move point?
    XUINT8      m_fTypeFlags;   // Flags to annotate the rgType of each point. Currently only PathPointTypeEmptyFill

//-----------------------------------------------------------------------------
// D2D Methods/Fields
//-----------------------------------------------------------------------------
public:
    _Check_return_ HRESULT OpenPathGeometryBuilder();

    _Check_return_ HRESULT ClosePathGeometryBuilder(
        _In_ CPathGeometry *pPathGeometry
        );

private:
    bool IsBuildingFigure() { return (m_pPathFigures != NULL); }

    CCoreServices *m_pCoreNoRef;
    CPathFigureCollection *m_pPathFigures;
    CPathFigure *m_pCurrentPathFigure;
    XPOINTF m_ptCurr{};
    XPOINTF m_ptReflect{};

};

//------------------------------------------------------------------------
//
//  Class:  CGeometry
//
//  Synopsis:
//      The base class of all geometries.  This class can not be directly
//  created.
//
//------------------------------------------------------------------------

class CGeometry : public CDependencyObject
{
protected:
    CGeometry(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
        , m_fSmoothJoin(false)
        , m_fNeedsStretching(true)
    {}

    ~CGeometry() override;

public:
    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate
        );

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CGeometry>::Index;
    }

    _Check_return_ HRESULT static  GetBounds(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _In_reads_(cArgs) CValue *pArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue *pResult
        );

    _Check_return_ HRESULT ComputeStretchMatrix(
        DirectUI::Stretch stretch,
        _In_ const XRECTF *pRenderBounds,
        _In_ const XRECTF *pNaturalBounds,
        _Inout_ CMILMatrix *pTransform
        );


    // Not every geometry is cloneable for hardware-acceleration on the render thread.
    virtual bool CanBeAccelerated() { return false; }

    virtual WUComp::ICompositionGeometry* GetCompositionGeometry(_In_ VisualContentRenderer* renderer) = 0;

    void SetWUCGeometryDirty(bool isDirty)
    {
        m_isWUCGeometryDirty = isDirty;
    }

    bool IsGeometryDirty() const
    {
        return m_isWUCGeometryDirty;
    }

    _Check_return_ HRESULT LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params) override;
    void ReleaseDCompResources() override;

    //-----------------------------------------------------------------------------
    //
    //  Bounds and Hit Testing
    //
    //-----------------------------------------------------------------------------
public:
    virtual _Check_return_ HRESULT GetBounds(
        _Out_ XRECTF_RB* pBounds
        );

    virtual _Check_return_ HRESULT GetWidenedBounds(
        _In_ const CPlainPen& pen,
        _Out_ XRECTF_RB* pBounds
        );

    _Check_return_ HRESULT FillContainsPoint(
        _In_ const XPOINTF& target,
        _In_opt_ const CMILMatrix* pTransform,
        _Out_ bool* pContainsPoint
        );

    virtual _Check_return_ HRESULT HitTestFill(
        _In_ const XPOINTF& target,
        _In_opt_ const CMILMatrix* pTransform,
        _Out_ bool* pHit
        );

    virtual _Check_return_ HRESULT HitTestFill(
        _In_ const HitTestPolygon& target,
        _In_opt_ const CMILMatrix* pTransform,
        _Out_ bool* pHit
        );

    virtual _Check_return_ HRESULT ClipToFill(
        _Inout_ XPOINTF& target,
        _In_opt_ const CMILMatrix* pTransform,
        _Out_ bool* pHit
        );

    virtual _Check_return_ HRESULT ClipToFill(
        _Inout_ HitTestPolygon& target,
        _In_opt_ const CMILMatrix* pTransform,
        _Out_ bool* pHit
        );

    _Check_return_ HRESULT StrokeContainsPoint(
        _In_ const XPOINTF& target,
        _In_ const CPlainPen& pen,
        _In_opt_ const CMILMatrix* pTransform,
        _Out_ bool* pContainsPoint
        );

    virtual _Check_return_ HRESULT HitTestStroke(
        _In_ const XPOINTF& target,
        _In_ const CPlainPen& pen,
        _In_opt_ const CMILMatrix* pTransform,
        _Out_ bool* pHit
        );

    virtual _Check_return_ HRESULT HitTestStroke(
        _In_ const HitTestPolygon& target,
        _In_ const CPlainPen& pen,
        _In_opt_ const CMILMatrix* pTransform,
        _Out_ bool* pHit
        );

    virtual _Check_return_ HRESULT VisitSink(
        _In_ IPALGeometrySink* pSink
        );

protected:
    virtual _Check_return_ HRESULT VisitSinkInternal(
        _In_ IPALGeometrySink* pSink
        ) = 0;

    _Check_return_ HRESULT WidenToSink(
        const CPlainPen& pen,
        _In_opt_ const CMILMatrix* pTransform,
        _In_ CWideningSink* pSink
        );

// CGeometry fields
public:
    // Geometry.Transform DP
    CTransform *m_pTransform = nullptr;

    // Used by derived classes.
    XcpFillMode m_fillMode = XcpFillModeAlternate;
    bool m_fSmoothJoin : 1;
    bool m_fNeedsStretching : 1;

private:
    bool m_isWUCGeometryDirty : 1;

protected:
    wrl::ComPtr<WUComp::ICompositionGeometry> m_wucGeometry;

private:
    void GetStrokeAdjustedElementBounds(
        _In_opt_ XRECTF *pElementBounds,
        _In_ const XRECTF& naturalBounds,
        XFLOAT penThickness,
        _Out_ XRECTF *pStrokeAdjustedElementBounds
        );

//-----------------------------------------------------------------------------
// D2D Methods/Fields
//-----------------------------------------------------------------------------
public:
    static _Check_return_ HRESULT DrawAccelerated(
        _In_ IPALAcceleratedGeometry *pPALAcceleratedGeometry,
        _In_ const D2DRenderParams& d2dRP,
        _In_ const CMILMatrix *pWorldTransform,
        XFLOAT strokeThickness,
        _In_opt_ IPALStrokeStyle *pStrokeStyle,
        XFLOAT opacity,
        _In_ IPALAcceleratedBrush *pPALBrush,
        _In_ AcceleratedBrushParams *pPALBrushParams
        );

//-----------------------------------------------------------------------------
// Printing Methods
//-----------------------------------------------------------------------------
public:
    _Check_return_ HRESULT GetPrintGeometry(
        _In_ const D2DPrecomputeParams& cp,
        _In_ const D2DRenderParams &printParams,
        _Outptr_ IPALAcceleratedGeometry** ppGeometry
        );

    _Check_return_ HRESULT GetPrintGeometry(
        _In_ const D2DPrecomputeParams& cp,
        _In_ const D2DRenderParams &printParams,
        _In_opt_ XRECTF *pElementBounds,
        DirectUI::Stretch stretch,
        XFLOAT rPenThickness,
        _Outptr_ IPALAcceleratedGeometry** ppGeometry
        );

protected:
    virtual _Check_return_ HRESULT GetPrintGeometryVirtual(
        _In_ const D2DPrecomputeParams& cp,
        _In_ const D2DRenderParams &printParams,
        IPALAcceleratedGeometry** ppGeometry
        );
};

//------------------------------------------------------------------------
//
//  Class:  CGeometryCollection
//
//  Synopsis:
//      A collection of geometries.
//
//------------------------------------------------------------------------
class CGeometryCollection final : public CDOCollection
{
private:
    CGeometryCollection(_In_ CCoreServices *pCore)
        : CDOCollection(pCore)
    {}

public:
    DECLARE_CREATE(CGeometryCollection);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CGeometryCollection>::Index;
    }

//-----------------------------------------------------------------------------
// D2D Methods/Fields
//-----------------------------------------------------------------------------
public:
    _Check_return_ HRESULT GetPrintGeometry(
        _In_ const D2DPrecomputeParams &cp,
        _In_ const D2DRenderParams &printParams,
        XcpFillMode fillMode,
        _Outptr_ IPALAcceleratedGeometry **ppPALGeometry
        );

    //-----------------------------------------------------------------------------
    //
    //  Bounds and Hit Testing
    //
    //-----------------------------------------------------------------------------
public:
    _Check_return_ HRESULT VisitSink(
        _In_ IPALGeometrySink* pSink
        );
};

//------------------------------------------------------------------------
//
//  Class:  CGeometryGroup
//
//  Synopsis:
//      Object created for <GeometryGroup> tag. A group of geometries that act
//  as one geometry.
//
//------------------------------------------------------------------------

class CGeometryGroup final : public CGeometry
{
private:
    CGeometryGroup(_In_ CCoreServices *pCore)
        : CGeometry(pCore)
        , m_pChild(NULL)
    {
    }

    ~CGeometryGroup() override;

    WUComp::ICompositionGeometry* GetCompositionGeometry(_In_ VisualContentRenderer* renderer) override;

    //-----------------------------------------------------------------------------
    //
    //  Bounds and Hit Testing
    //
    //-----------------------------------------------------------------------------
protected:
    _Check_return_ HRESULT VisitSinkInternal(
        _In_ IPALGeometrySink* pSink
        ) override;

public:
    DECLARE_CREATE(CGeometryGroup);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CGeometryGroup>::Index;
    }

    // CGeometryGroup fields
    CGeometryCollection *m_pChild;

//-----------------------------------------------------------------------------
// D2D Methods/Fields
//-----------------------------------------------------------------------------
protected:
    _Check_return_ HRESULT GetPrintGeometryVirtual(
        _In_ const D2DPrecomputeParams& cp,
        _In_ const D2DRenderParams &printParams,
        _Outptr_ IPALAcceleratedGeometry** ppGeometry
        ) override;
};

//------------------------------------------------------------------------
//
//  Class:  CPathGeometry
//
//  Synopsis:
//      Object created for <PathGeometry> tag or the Path.Data or Path.Clip
//  attributes.
//
//------------------------------------------------------------------------

class CPathGeometry : public CGeometry
{
private:
    CPathGeometry(_In_ CCoreServices *pCore)
        : CGeometry(pCore)
        , m_pFigures(NULL)
    {
    }

   ~CPathGeometry() override;

   wrl::ComPtr<CGeometrySource> m_geometrySource;

public:
    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate
        );

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CPathGeometry>::Index;
    }

    WUComp::ICompositionGeometry* GetCompositionGeometry(_In_ VisualContentRenderer* renderer) override;

    // CPathGeometry methods
    _Check_return_ HRESULT DrawLines(
        _In_reads_(uiPointCount) const XPOINTF *pPoints,
        XUINT32 uiPointCount,
        bool fIsClosed
        );

//-----------------------------------------------------------------------------
// Printing Methods
//-----------------------------------------------------------------------------
protected:
    _Check_return_ HRESULT GetPrintGeometryVirtual(
        _In_ const D2DPrecomputeParams& cp,
        _In_ const D2DRenderParams &printParams,
        IPALAcceleratedGeometry** ppGeometry
        ) override;

private:
    _Check_return_ HRESULT ParseGeometry(
        _In_ CGeometryBuilder *pBuilder,
        _In_ XUINT32 cString,
        _In_reads_(cString) const WCHAR *pString,
        _In_ XINT32 bAllowFill
        );

public:
    // CPathGeometry fields
    CPathFigureCollection *m_pFigures;

    //-----------------------------------------------------------------------------
    //
    //  Bounds and Hit Testing
    //
    //-----------------------------------------------------------------------------
protected:
    _Check_return_ HRESULT VisitSinkInternal(
        _In_ IPALGeometrySink* pSink
        ) override;
};

// This class "wraps" ID2D1Geometry as a IGeometrySource2D
class CGeometrySource final:
    public ctl::implements_inspectable<wgr::IGeometrySource2D>,
    public wgr::IGeometrySource2DInterop
{
public:
    CGeometrySource() = default;

    void UpdateD2DGeometry(_In_ ID2D1Geometry* geometry)
    {
        m_geometry = geometry;
    }

    IFACEMETHODIMP_(ULONG) AddRef() override
    {
        return ctl::implements_inspectable<wgr::IGeometrySource2D>::AddRef();
    }

    IFACEMETHODIMP_(ULONG) Release() override
    {
        return ctl::implements_inspectable<wgr::IGeometrySource2D>::Release();
    }

    // IUnknown
    IFACEMETHODIMP QueryInterface(
        _In_ REFIID iid,
        _Outptr_opt_ void ** ppvObject) override
    {
        if (iid == __uuidof(wgr::IGeometrySource2DInterop))
        {
            AddRef();
            *ppvObject = static_cast<wgr::IGeometrySource2DInterop*>(this);
            return S_OK;
        }

        return ctl::implements_inspectable<wgr::IGeometrySource2D>::QueryInterface(iid, ppvObject);
    }

    // wgr::IGeometrySource2DInterop
    IFACEMETHODIMP GetGeometry(
        _COM_Outptr_ ID2D1Geometry** value) override
    {
        SetInterface(*value, m_geometry.Get());
        return S_OK;
    }

    IFACEMETHODIMP TryGetGeometryUsingFactory(
        _In_ ID2D1Factory* /* factory */,
        _COM_Outptr_result_maybenull_ ID2D1Geometry** value) override
    {
        *value = nullptr;
        return E_NOTIMPL;
    }

protected:
    ~CGeometrySource() override = default;

private:
    wrl::ComPtr<ID2D1Geometry> m_geometry;
};
