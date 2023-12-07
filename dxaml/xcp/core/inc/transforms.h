// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <gsl/span>

//------------------------------------------------------------------------
//
//  Interface:  ITransformer
//
//  Synopsis:
//      Abstract transform interface.
//
//------------------------------------------------------------------------
struct ITransformer : public IObject
{
protected:
    virtual ~ITransformer() {}

// Nested types
public:
    enum TransformerType
    {
        TransformerType_Unknown = 0,
        TransformerType_Matrix,
        TransformerType_Perspective,
        TransformerType_Effect,
        TransformerType_Aggregate,
        TransformerType_UpDown
    };


public:
    // Goes from local space to world space
    virtual _Check_return_ HRESULT Transform(
        _In_reads_(uiCount) const XPOINTF *pSrcPoints, // Array of source points to transform
        _Out_writes_(uiCount) XPOINTF *pDestPoints,     // Destination for transformed points.
        XUINT32 uiCount                                 // Number of points to transform
        ) = 0;

    // Goes from world space to local space
    virtual _Check_return_ HRESULT ReverseTransform(
        _In_reads_(uiCount) const XPOINTF *pSrcPoints, // Array of source points to transform
        _Out_writes_(uiCount) XPOINTF *pDestPoints,     // Destination for transformed points.
        XUINT32 uiCount                                 // Number of points to transform
        ) = 0;

    virtual TransformerType GetType() = 0;

    virtual bool IsPure2D() = 0;
    virtual CMILMatrix Get2DMatrix() = 0;
    virtual CMILMatrix Get2DMatrixIgnore3D() = 0; // Ignore any 3D transformer

};

//------------------------------------------------------------------------
//
//  Class:  CTransformer
//
//  Synopsis:
//      Base transform interface, provides defaults and refcounting.
//
//------------------------------------------------------------------------
class CTransformer : public CXcpObjectBase<ITransformer>
{
protected:
    CTransformer() {}

    ~CTransformer() override {}

public:
    // Goes from local space to world space
    _Check_return_ HRESULT Transform(
        _In_reads_(uiCount) const XPOINTF *pSrcPoints,
        _Out_writes_(uiCount) XPOINTF *pDestPoints,
        XUINT32 uiCount
        ) override
    {
        UNREFERENCED_PARAMETER(pSrcPoints);
        UNREFERENCED_PARAMETER(pDestPoints);
        UNREFERENCED_PARAMETER(uiCount);
        RRETURN(E_NOTIMPL);
    }

    // Goes from world space to local space
    _Check_return_ HRESULT ReverseTransform(
        _In_reads_(uiCount) const XPOINTF *pSrcPoints,
        _Out_writes_(uiCount) XPOINTF *pDestPoints,
        XUINT32 uiCount
        ) override
    {
        UNREFERENCED_PARAMETER(pSrcPoints);
        UNREFERENCED_PARAMETER(pDestPoints);
        UNREFERENCED_PARAMETER(uiCount);
        RRETURN(E_NOTIMPL);
    }

    TransformerType GetType() override { return TransformerType_Unknown; } ;

    bool IsPure2D() override { return false; }

    CMILMatrix Get2DMatrix() override { return CMILMatrix(TRUE); };

    CMILMatrix Get2DMatrixIgnore3D() override { return CMILMatrix(TRUE); };

    static _Check_return_ HRESULT TransformBounds(
        _In_ ITransformer* pTransformer,
        _In_ const XRECTF *pSource,
        _Out_ XRECTF *pTarget,
        bool fReverse = false
        );
};

//------------------------------------------------------------------------
//
//  Class:  CMatrixTransformer
//
//  Synopsis:
//      2D matrix transform interface.
//
//------------------------------------------------------------------------
class CMatrixTransformer : public CTransformer
{
public:
    CMatrixTransformer(_In_ const CMILMatrix& matTransform)
    {
        m_matTransform = matTransform;
    }

protected:
    ~CMatrixTransformer() override {}

public:
    // Goes from local space to world space
    _Check_return_ HRESULT Transform(
        _In_reads_(uiCount) const XPOINTF *pSrcPoints,
        _Out_writes_(uiCount) XPOINTF *pDestPoints,
        XUINT32 uiCount
        ) override
    {
        m_matTransform.Transform(pSrcPoints, pDestPoints, uiCount);
        RRETURN(S_OK);
    }

    // Goes from world space to local space
    _Check_return_ HRESULT ReverseTransform(
        _In_reads_(uiCount) const XPOINTF *pSrcPoints,
        _Out_writes_(uiCount) XPOINTF *pDestPoints,
        XUINT32 uiCount
        ) override
    {
        CMILMatrix matInverseTransform = m_matTransform;

        if (matInverseTransform.Invert())
        {
            matInverseTransform.Transform(pSrcPoints, pDestPoints, uiCount);
        }
        else
        {
            // no-op for non-invertible matrices
            memcpy(pDestPoints, pSrcPoints, uiCount * sizeof(XPOINTF));
        }

        RRETURN(S_OK);
    }

    TransformerType GetType() override { return TransformerType_Matrix; }

    bool IsPure2D() override { return true; }

    CMILMatrix Get2DMatrix() override { return m_matTransform; }

    CMILMatrix Get2DMatrixIgnore3D() override { return m_matTransform; };

    void Append(_In_ const CMatrixTransformer* pOther)
    {
        if (pOther)
        {
            m_matTransform.Append(pOther->m_matTransform);
        }
    }

    void Prepend(_In_ const CMatrixTransformer* pOther)
    {
        if (pOther)
        {
            m_matTransform.Prepend(pOther->m_matTransform);
        }
    }

private:
    CMILMatrix m_matTransform;
};

//------------------------------------------------------------------------
//
//  Class:  CPerspectiveTransformer
//
//  Synopsis:
//      Perspective plane transform interface.
//
//------------------------------------------------------------------------
class CPerspectiveTransformer : public CTransformer
{
public:
    CPerspectiveTransformer(
        _In_ const CMILMatrix4x4& matProjection,
        _In_ const XVERTEX25D& vTopLeft,
        _In_ const XVERTEX25D& vOneDx,
        _In_ const XVERTEX25D& vOneDy,
        bool fIsSingular
        )
        : m_matProjection(matProjection)
        , m_vTopLeft(vTopLeft)
        , m_vOneDx(vOneDx)
        , m_vOneDy(vOneDy)
        , m_fIsSingular(fIsSingular)
    {
    }

protected:
    ~CPerspectiveTransformer() override {}

public:
    // Goes from local space to world space, returns whether all points have positive w values.
    // A negative w value will cause the projected point to be reflected across the origin when
    // it's divided out, and will likely messes up the winding order of the transformed points.
    // Use this method when the shape of the final polygon matters.
    _Check_return_ HRESULT TransformCheckW(
        _In_reads_(uiCount) const XPOINTF *pSrcPoints,
        _Out_writes_(uiCount) XPOINTF *pDestPoints,
        XUINT32 uiCount,
        _Out_opt_ bool *pAreAllWValuesPositive
        )
    {
        const gsl::span<const XPOINTF> source(pSrcPoints, uiCount);
        const gsl::span<XPOINTF> destination(pDestPoints, uiCount);
        bool areAllWValuesPositive = m_matProjection.Transform2DPoints_DivideW(source, destination);
        if (pAreAllWValuesPositive != nullptr)
        {
            *pAreAllWValuesPositive = areAllWValuesPositive;
        }
        RRETURN(S_OK);
    }

    // Goes from local space to world space, ignoring whether all w values are positive. Use this
    // method when the points don't represent a polygon.
    _Check_return_ HRESULT Transform(
        _In_reads_(uiCount) const XPOINTF *pSrcPoints,
        _Out_writes_(uiCount) XPOINTF *pDestPoints,
        XUINT32 uiCount
        ) override
    {
        const gsl::span<const XPOINTF> source(pSrcPoints, uiCount);
        const gsl::span<XPOINTF> destination(pDestPoints, uiCount);
        m_matProjection.Transform2DPoints_DivideW(source, destination);
        RRETURN(S_OK);
    }

    // Goes from local space to world space
    _Check_return_ HRESULT Transform(
        _In_reads_(uiCount) const XPOINTF4 *pSrcPoints,
        _Out_writes_(uiCount) XPOINTF4 *pDestPoints,
        XUINT32 uiCount
        )
    {
        const gsl::span<const XPOINTF4> source(pSrcPoints, uiCount);
        const gsl::span<XPOINTF4> destination(pDestPoints, uiCount);
        m_matProjection.Transform_PreserveW(source, destination);
        RRETURN(S_OK);
    }

    XCP_FORCEINLINE void CopySource(_In_ const XPOINTF& source, _Out_ XPOINTF *pDestination)
    {
        *pDestination = source;
    }

    XCP_FORCEINLINE void CopySource(_In_ const XPOINTF4& source, _Out_ XPOINTF *pDestination)
    {
        pDestination->x = source.x;
        pDestination->y = source.y;
    }

    XCP_FORCEINLINE void SaveDestination(_In_ const XVERTEX25D& source, _Out_ XPOINTF *pDestination)
    {
        pDestination->x = source.u();
        pDestination->y = source.v();
    }

    XCP_FORCEINLINE void SaveDestination(_In_ const XVERTEX25D& source, _Out_ XPOINTF4 *pDestination)
    {
        pDestination->x = source.u();
        pDestination->y = source.v();
        pDestination->z = 0.0f;
        pDestination->w = 1.0f;
    }

    // Goes from world space to local space, returns whether all points have positive w values.
    // The w values in the transformer come from projecting the original quad and should all be
    // positive (see CProjection::GetTransformerCommon). If we encounter a negative w value here,
    // it means:
    // - The original quad had a negative w value after it was projected. In that case we would
    //   have near-plane clipped the point out and it wouldn't have rendered (see
    //   HWClip::ApplyTransformAndClip_DropZPreserveW). Since it didn't render, we should skip
    //   hit testing it.
    // - A point was outside the original quad, and was past the horizon of the projection. When
    //   we extrapolated its u/v coordinates, the 1/w value went past 0 and became negative. This
    //   point is not valid inside the projection.
    // In both cases we can stop hit testing the element.
    // XPOINTF used by 2D, XPOINTF4 used by projections
    template <typename PointType>
    void ReverseTransformPointsCheckW(
        _In_reads_(uiCount) const PointType *pSrcPoints,
        _Out_writes_(uiCount) PointType *pDestPoints,
        XUINT32 uiCount,
        _Out_opt_ bool *pAreAllWValuesPositive
        )
    {
        bool areAllWValuesPositive = true;

        if (!m_fIsSingular)
        {
            XVERTEX25D vHit;
            XPOINTF pt;

            for (XUINT32 i = 0; i < uiCount; i++)
            {
                // Adjust hit point by our starting offset
                CopySource(pSrcPoints[i], &pt);
                pt.x -= m_vTopLeft.x;
                pt.y -= m_vTopLeft.y;

                // Compute where the hit point would project onto our texture
                vHit = m_vTopLeft + (m_vOneDx * pt.x) + (m_vOneDy * pt.y);
                if (vHit.oneOverW < 0)
                {
                    areAllWValuesPositive = FALSE;
                }

                // Compute UV coordinates - these will become X,Y for the child
                // subtree
                SaveDestination(vHit, &pDestPoints[i]);
            }
        }

        if (pAreAllWValuesPositive != NULL)
        {
            *pAreAllWValuesPositive = areAllWValuesPositive;
        }
    }

    // Goes from world space to local space, ignoring whether all w values are positive.
    // XPOINTF used by 2D, XPOINTF4 used by projections
    template <typename PointType>
    void ReverseTransformPoints(
        _In_reads_(uiCount) const PointType *pSrcPoints,
        _Out_writes_(uiCount) PointType *pDestPoints,
        XUINT32 uiCount
        )
    {
        ReverseTransformPointsCheckW(pSrcPoints, pDestPoints, uiCount, NULL);
    }

    // Goes from world space to local space, ignoring whether all w values are positive.
    _Check_return_ HRESULT ReverseTransform(
        _In_reads_(uiCount) const XPOINTF *pSrcPoints,
        _Out_writes_(uiCount) XPOINTF *pDestPoints,
        XUINT32 uiCount
        ) override
    {
        ReverseTransformPointsCheckW(pSrcPoints, pDestPoints, uiCount, NULL);

        RRETURN(S_OK);
    }

    TransformerType GetType() override { return TransformerType_Perspective; }

    bool IsInvertible() const { return !m_fIsSingular; }

private:
    CMILMatrix4x4 m_matProjection;
    XVERTEX25D m_vTopLeft;
    XVERTEX25D m_vOneDx;
    XVERTEX25D m_vOneDy;
    bool      m_fIsSingular;
};

template<>
void CXcpList<ITransformer>::Clean(XUINT8 bDoDelete);

//------------------------------------------------------------------------
//
//  Class:  CAggregateTransformer
//
//  Synopsis:
//      Aggregate ITransformer.
//
//------------------------------------------------------------------------
class CAggregateTransformer final : public CTransformer
{
public:
    CAggregateTransformer()
    {
    }

    ~CAggregateTransformer() override;

    // Goes from local space to world space
    _Check_return_ HRESULT Transform(
        _In_reads_(uiCount) const XPOINTF *pSrcPoints,
        _Out_writes_(uiCount) XPOINTF *pDestPoints,
        XUINT32 uiCount
        ) override;

    // Goes from world space to local space
    _Check_return_ HRESULT ReverseTransform(
        _In_reads_(uiCount) const XPOINTF *pSrcPoints,
        _Out_writes_(uiCount) XPOINTF *pDestPoints,
        XUINT32 uiCount
        ) override;

    TransformerType GetType() override { return TransformerType_Aggregate; } ;

    bool IsPure2D() override
    {
        CXcpList<ITransformer>::XCPListNode *pCurrent = m_oList.GetHead();
        while (pCurrent != NULL)
        {
            if (!pCurrent->m_pData->IsPure2D())
            {
                return false;
            }
            pCurrent = pCurrent->m_pNext;
        }
        return true;
    }

    CMILMatrix Get2DMatrix() override
    {
        if(!IsPure2D())
        {
            return CMILMatrix(TRUE);
        }
        return m_oList.GetHead() ?
            m_oList.GetHead()->m_pData->Get2DMatrix()
            : CTransformer::Get2DMatrix() ;
    }

    CMILMatrix Get2DMatrixIgnore3D() override
    {
        CMILMatrix matrix(TRUE);
        for ( CXcpList<ITransformer>::XCPListNode *p = m_oList.GetHead();
              p != nullptr;
              p = p->m_pNext )
        {
            matrix.Append(p->m_pData->Get2DMatrixIgnore3D());
        }
        return matrix;
    }

    _Check_return_ HRESULT Add(_In_ ITransformer* pChild);

private:
    CXcpList<ITransformer> m_oList;
};

//------------------------------------------------------------------------
//
//  Class:  CUpDownTransformer
//
//  Synopsis:
//      ITransformer with forward- and backward-facing transformers.
//
//------------------------------------------------------------------------
class CUpDownTransformer final : public CTransformer
{
public:
    CUpDownTransformer(
        _In_ ITransformer *pUp,
        _In_ ITransformer *pDown
        )
    {
        SetInterface(m_pUp, pUp);
        SetInterface(m_pDown, pDown);
    }

protected:
    ~CUpDownTransformer() override
    {
        ReleaseInterface(m_pUp);
        ReleaseInterface(m_pDown);
    }

public:
    // Goes from local space to world space
    _Check_return_ HRESULT Transform(
        _In_reads_(uiCount) const XPOINTF *pSrcPoints,
        _Out_writes_(uiCount) XPOINTF *pDestPoints,
        XUINT32 uiCount
        ) override
    {
        memcpy(pDestPoints, pSrcPoints, uiCount * sizeof(XPOINTF));

        if (m_pUp)
        {
            IFC_RETURN(m_pUp->Transform(pDestPoints, pDestPoints, uiCount));
        }
        if (m_pDown)
        {
            IFC_RETURN(m_pDown->ReverseTransform(pDestPoints, pDestPoints, uiCount));
        }

        return S_OK;
    }

    // Goes from world space to local space
    _Check_return_ HRESULT ReverseTransform(
        _In_reads_(uiCount) const XPOINTF *pSrcPoints,
        _Out_writes_(uiCount) XPOINTF *pDestPoints,
        XUINT32 uiCount
        ) override
    {
        memcpy(pDestPoints, pSrcPoints, uiCount * sizeof(XPOINTF));

        if (m_pDown)
        {
            IFC_RETURN(m_pDown->Transform(pDestPoints, pDestPoints, uiCount));
        }
        if (m_pUp)
        {
            IFC_RETURN(m_pUp->ReverseTransform(pDestPoints, pDestPoints, uiCount));
        }

        return S_OK;
    }

    TransformerType GetType() override { return TransformerType_UpDown; }

    bool IsPure2D() override
    {
        return (m_pUp ? m_pUp->IsPure2D() : TRUE) && (m_pDown ? m_pDown->IsPure2D() : TRUE);
    }

    CMILMatrix Get2DMatrix() override
    {
        CMILMatrix result(TRUE);

        if (IsPure2D())
        {
            if (m_pUp)
            {
                result.Append(m_pUp->Get2DMatrix());
            }
            if (m_pDown)
            {
                CMILMatrix matDown = m_pDown->Get2DMatrix();
                matDown.Invert();
                result.Append(matDown);
            }
        }
        return result;
    };

    CMILMatrix Get2DMatrixIgnore3D() override
    {
        CMILMatrix result(TRUE);

        if (m_pUp)
        {
            result.Append(m_pUp->Get2DMatrixIgnore3D());
        }
        if (m_pDown)
        {
            CMILMatrix matDown = m_pDown->Get2DMatrixIgnore3D();
            matDown.Invert();
            result.Append(matDown);
        }

        return result;
    };

private:
    ITransformer *m_pUp;
    ITransformer *m_pDown;
};

#include "GeneralTransform.h"

//------------------------------------------------------------------------
//
//  Class:  CInternalTransform
//
//  Synopsis:
//      For exposing transforms to user code through TransformToVisual
//
//------------------------------------------------------------------------

class CInternalTransform final : public CGeneralTransform
{
private:
    CInternalTransform(_In_ CCoreServices *pCore)
        : CGeneralTransform(pCore)
    {}

protected:
    // CNoParentShareableDependencyObject overrides
    CInternalTransform(_In_ const CInternalTransform& original, _Out_ HRESULT& hr)
        : CGeneralTransform(original, hr)
    {
        m_pTransformer = original.GetTransformer();
        m_fUseReverse = original.GetIsReverse();
    }

    ~CInternalTransform() override
    {
        ReleaseInterface(m_pTransformer);
    }

public:
    DECLARE_CREATE(CInternalTransform);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CInternalTransform>::Index;
    }

    // CNoParentShareableDependencyObject overrides
    DECLARE_SHAREABLEDEPENDENCYOBJECT_CLONE(CInternalTransform);

    // CGeneralTransform overrides
    _Check_return_ HRESULT TransformPoints(
        _In_reads_(cPoints) XPOINTF *pptOriginal,
        _Inout_updates_(cPoints) XPOINTF *pptTransformed,
        XUINT32 cPoints = 1
        ) override
    {
        if (m_pTransformer)
        {
            if (m_fUseReverse)
            {
                IFC_RETURN(m_pTransformer->ReverseTransform(
                    pptOriginal,
                    pptTransformed,
                    cPoints));
            }
            else
            {
                IFC_RETURN(m_pTransformer->Transform(
                    pptOriginal,
                    pptTransformed,
                    cPoints));
            }
        }
        else
        {
            // No transformer means no-op transform
            memcpy(pptTransformed, pptOriginal, cPoints * sizeof(XPOINTF));
        }

        return S_OK;
    }

    // CInternalTransform methods
    _Check_return_ HRESULT static Transform(
        _In_ void *pvInternalTransform,
        _In_ XPOINTF ptHit,
        _Inout_ XPOINTF *pResult);

    _Check_return_ HRESULT Inverse(_Outptr_ CGeneralTransform **ppResult);

    void SetTransformer(_In_ ITransformer* pTransformer, bool fReverse = false)
    {
        ReplaceInterface(m_pTransformer, pTransformer);
        m_fUseReverse = fReverse;
    }

    ITransformer* GetTransformer() const
    {
        AddRefInterface(m_pTransformer);
        return m_pTransformer;
    }

    bool GetIsReverse() const
    {
        return m_fUseReverse;
    }

private:
    ITransformer* m_pTransformer    = nullptr;
    bool m_fUseReverse = false;
};

#include "Transform.h"
#include "MatrixTransform.h"
#include "TransformCollection.h"
#include "TransformGroup.h"
#include "RotateTransform.h"
#include "ScaleTransform.h"
#include "SkewTransform.h"
#include "TranslateTransform.h"
#include "CompositeTransform.h"

