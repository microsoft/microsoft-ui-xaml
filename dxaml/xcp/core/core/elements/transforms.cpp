// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "MinMath.h"

template void CXcpList<ITransformer>::Clean( XUINT8 bDoDelete );

namespace CoreImports
{
    //------------------------------------------------------------------------
    //
    //  Synopsis:
    //      Transform a point
    //
    //------------------------------------------------------------------------
    _Check_return_
    HRESULT
    InternalTransform_Transform(
        _In_ void *pvInternalTransform,
        _In_ XPOINTF ptHit,
        _Inout_ XPOINTF *pResult)
    {
        CInternalTransform *pInternalTransform = NULL;
        IFC_RETURN(DoPointerCast(pInternalTransform, static_cast<CDependencyObject *>( pvInternalTransform )));

        IFCPTR_RETURN(pResult);

        IFC_RETURN(CInternalTransform::Transform(pInternalTransform, ptHit, pResult));

        return S_OK;
    }

    //------------------------------------------------------------------------
    //
    //  Synopsis:
    //      Transform a rect
    //
    //------------------------------------------------------------------------
    _Check_return_
    HRESULT
    InternalTransform_TransformBounds(
        _In_ void *pvInternalTransform,
        _In_ XRECTF rectHit,
        _Inout_ XRECTF *pResult)
    {
        HRESULT hr = S_OK;

        CInternalTransform *pInternalTransform = NULL;
        ITransformer *pTransformer = NULL;
        IFC(DoPointerCast(pInternalTransform, static_cast<CDependencyObject *>( pvInternalTransform )));

        pTransformer = pInternalTransform->GetTransformer();

        IFCPTR(pResult);

        IFC(CTransformer::TransformBounds(pTransformer, &rectHit, pResult, pInternalTransform->GetIsReverse()));

    Cleanup:
        ReleaseInterface(pTransformer);
        RRETURN(hr);
    }

    //------------------------------------------------------------------------
    //
    //  Synopsis:
    //      Get the inverse
    //
    //------------------------------------------------------------------------
    _Check_return_
    HRESULT
    InternalTransform_Inverse(
        _In_ CInternalTransform *pInternalTransform,
        _Out_ CGeneralTransform **pResult)
    {
        IFCPTR_RETURN(pResult);
        IFC_RETURN(pInternalTransform->Inverse(pResult));

        return S_OK;
    }

    //------------------------------------------------------------------------
    //
    //  Synopsis:
    //      Transform a point
    //
    //------------------------------------------------------------------------
    _Check_return_
    HRESULT
    Transform_Transform(
        _In_ void *pvTransform,
        _In_ XPOINTF ptHit,
        _Inout_ XPOINTF *pResult)
    {
        CTransform *pTransform = NULL;
        IFC_RETURN(DoPointerCast(pTransform, static_cast<CDependencyObject*>(pvTransform)));

        IFCPTR_RETURN(pResult);

        IFC_RETURN(pTransform->TransformPoints(&ptHit, pResult));

        return S_OK;
    }

    //------------------------------------------------------------------------
    //
    //  Synopsis:
    //      Transform a rect
    //
    //------------------------------------------------------------------------
    _Check_return_
    HRESULT
    Transform_TransformBounds(
        _In_ void *pvTransform,
        _In_ XRECTF rectHit,
        _Inout_ XRECTF *pResult)
    {
        CTransform *pTransform = NULL;
        CMILMatrix mat(FALSE);

        IFC_RETURN(DoPointerCast(pTransform, static_cast<CDependencyObject *>( pvTransform )));

        pTransform->GetTransform(&mat);

        IFCPTR_RETURN(pResult);

        mat.TransformBounds(&rectHit, pResult);

        return S_OK;
    }

    //------------------------------------------------------------------------
    //
    //  Synopsis:
    //      Get the inverse
    //
    //------------------------------------------------------------------------
    _Check_return_
    HRESULT
    Transform_Inverse(
        _In_ CTransform *pTransform,
        _Out_ CGeneralTransform **pResult)
    {
        IFC_RETURN(pTransform->Inverse(pResult));

        return S_OK;
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Public API to transform a point
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInternalTransform::Transform(
    _In_ void *pvInternalTransform,
    _In_ XPOINTF ptHit,
    _Inout_ XPOINTF *pResult
    )
{
    // Validate parameters
    if ( pvInternalTransform == NULL
      || pResult == NULL )
    {
        IFC_RETURN(E_INVALIDARG);
    }
    else
    {
        CInternalTransform *pGT = static_cast<CInternalTransform*>(pvInternalTransform);
        IFCPTR_RETURN(pGT);

        // Do the actual work
        IFC_RETURN( pGT->TransformPoints( &ptHit, pResult, 1 ) );
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Public API to transform a point
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGeneralTransform::TransformXY(
    _In_ CDependencyObject *pObject,
    _In_ XUINT32 cArgs,
    _Inout_updates_(cArgs) CValue *ppArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue *pResult)
{
    HRESULT hr = S_OK;

    CGeneralTransform *pGT = NULL;
    CPoint *pPoint = NULL;
    XPOINTF pt;

    IFC(DoPointerCast(pGT, pObject));

    // Validate input
    if ( (cArgs != 2)
         || !pObject
         || !pObject->OfTypeByIndex<KnownTypeIndex::GeneralTransform>()
         )
    {
        IFC(E_INVALIDARG);
    }
    IFCPTR(pGT);

    switch ( ppArgs[0].GetType() )
    {
        case valueFloat:
            pt.x = ppArgs[0].AsFloat();
            break;
        case valueSigned:
            pt.x = (XFLOAT)ppArgs[0].AsSigned();
            break;
        default:
            IFC(E_INVALIDARG);
            break;
    }
    switch ( ppArgs[1].GetType() )
    {
        case valueFloat:
            pt.y = ppArgs[1].AsFloat();
            break;
        case valueSigned:
            pt.y = (XFLOAT)ppArgs[1].AsSigned();
            break;
        default:
            IFC(E_INVALIDARG);
            break;
    }

    // Do the actual work
    IFC(pGT->TransformPoints(&pt, &pt, 1));

    // Create object to represent new coordinates
    {
        CREATEPARAMETERS cp(pGT->GetContext());
        IFC(CPoint::Create((CDependencyObject**)(&pPoint), &cp));
    }

    pPoint->m_pt = pt;
    pResult->SetObjectNoRef(pPoint);
    pPoint = NULL;

Cleanup:
    ReleaseInterface(pPoint);
    RRETURN(hr);
}

// Transform a rect.  Returns a bounding box if the rect is transformed
_Check_return_ HRESULT
CGeneralTransform::TransformRect(_In_ const XRECTF& source, _Out_ XRECTF* pTransformedRect)
{
    const int pointCount = 4;
    XPOINTF points[pointCount] = {
        {source.X, source.Y},
        {source.X+source.Width, source.Y},
        {source.X+source.Width, source.Y+source.Height},
        {source.X, source.Y+source.Height}};
    XPOINTF resultPoints[pointCount];

    IFC_RETURN(TransformPoints(points, resultPoints, pointCount));

    BoundPoints(resultPoints, pointCount, pTransformedRect);

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Public API to get the inverse transform
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CInternalTransform::Inverse(_Outptr_ CGeneralTransform **ppResult)
{
    HRESULT hr = S_OK;

    CInternalTransform *pInverseGT = NULL;
    ITransformer* pTransformer = NULL;

    CREATEPARAMETERS cp(GetContext());

    // Create result
    IFC(CInternalTransform::Create((CDependencyObject**)(&pInverseGT), &cp));

    // Get the existing transformer
    pTransformer = GetTransformer();

    // Now set it to use the inverse path on the new object
    pInverseGT->SetTransformer(pTransformer, !GetIsReverse());

    // Set the result back
    *ppResult = pInverseGT;
    pInverseGT = NULL;

Cleanup:
    ReleaseInterface(pInverseGT);
    ReleaseInterface(pTransformer);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Type converter for creating transforms from matrices.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CTransform::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate
    )
{
    // We only expect a string or the default value
    ASSERT(   (pCreate->m_value.GetType() == valueString)
           || (pCreate->m_value.GetType() == valueAny));

    // Initialize via the type converter
    RRETURN(CMatrixTransform::Create(ppObject, pCreate));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a new CTransform representing the inverse of this transform
//      Returns E_FAIL if the transform is non-invertible
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CTransform::Inverse(_Outptr_ CGeneralTransform **ppInverse)
{
    HRESULT hr = S_OK;
    CMatrixTransform *pResult = NULL;

    CMILMatrix mat(FALSE);
    CREATEPARAMETERS cp(GetContext());

    GetTransform(&mat);
    if (!mat.Invert())
    {
        IFC(E_FAIL);
    }

    IFC(CMatrixTransform::Create(reinterpret_cast<CDependencyObject**>(&pResult), &cp));
    IFC(CMatrix::Create(reinterpret_cast<CDependencyObject**>(&(pResult->m_pMatrix)), &cp));
    pResult->m_pMatrix->m_matrix = mat;

    *ppInverse = pResult;
    pResult = NULL;

Cleanup:
    ReleaseInterface(pResult);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CTransformer::TransformBounds
//
//  Synopsis:
//      Static helper to transform the 4 points of an XRECTF that represent
//  bounds.
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CTransformer::TransformBounds(
    _In_ ITransformer* pTransformer,
    _In_ const XRECTF *pSource,
    _Out_ XRECTF *pTarget,
    bool bReverse )
{
    if (pTransformer && pSource && pTarget)
    {
        XFLOAT rXMin,rXMax,rYMin,rYMax;
        XPOINTF pt[4];
        pt[0].x = pSource->X;
        pt[0].y = pSource->Y;
        pt[1].x = pSource->X + pSource->Width;
        pt[1].y = pSource->Y;
        pt[2].x = pSource->X;
        pt[2].y = pSource->Y + pSource->Height;
        pt[3].x = pSource->X + pSource->Width;
        pt[3].y = pSource->Y + pSource->Height;

        if ( bReverse )
        {
            IFC_RETURN(pTransformer->ReverseTransform(pt, pt, 4));
        }
        else
        {
            IFC_RETURN(pTransformer->Transform(pt, pt, 4));
        }

        // Do a Min/Max pass on all the points
        rXMin = rXMax = pt[0].x;
        rYMin = rYMax = pt[0].y;
        for ( XINT32 i=1; i<4; i++)
        {
            rXMin = MIN( rXMin, pt[i].x );
            rXMax = MAX( rXMax, pt[i].x );
            rYMin = MIN( rYMin, pt[i].y );
            rYMax = MAX( rYMax, pt[i].y );
        }

        pTarget->X = rXMin;
        pTarget->Y = rYMin;
        pTarget->Width = rXMax - rXMin;
        pTarget->Height = rYMax - rYMin;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CXcpList<ITransformer>::Clean
//
//  Synopsis:
//      Specialized templated list clean to deal with refcounted ITransformer
//
//------------------------------------------------------------------------
template<>
void
CXcpList<ITransformer>::Clean( XUINT8 bDoDelete )
{
    XCPListNode *pTemp;

    while (m_pHead)
    {
        pTemp = m_pHead;
        m_pHead = m_pHead->m_pNext;

        if ( bDoDelete )
        {
            ITransformer *pCurrent = static_cast<ITransformer *>( pTemp->m_pData );
            ReleaseInterface(pCurrent);
        }

        pTemp->m_pData = NULL;
        pTemp->m_pNext = NULL;
        delete pTemp;
    }

    m_pHead = NULL;
}

//------------------------------------------------------------------------
//
//  Method:   ~CAggregateTransformer
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------
CAggregateTransformer::~CAggregateTransformer()
{
    m_oList.Clean(TRUE);
}

//------------------------------------------------------------------------
//
//  Method:   CAggregateTransformer::Transform
//
//  Synopsis:
//      Transform points from inner to outer space, used when transforming
//      local bounds to world bounds.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CAggregateTransformer::Transform(
    _In_reads_(count) const XPOINTF *srcPoints,
    _Out_writes_(count) XPOINTF *destPoints,
    _In_ XUINT32 count)
{
    CXcpList< ITransformer > m_oReverseList;

    memcpy( destPoints, srcPoints, count * sizeof(XPOINTF) );

    // The "list" is really a stack ...
    m_oList.GetReverse( &m_oReverseList );
    for ( CXcpList<ITransformer>::XCPListNode *p = m_oReverseList.GetHead();
          p != NULL;
          p = p->m_pNext )
    {
        IFC_RETURN(p->m_pData->Transform( destPoints, destPoints, count ));
    }

    m_oReverseList.Clean(FALSE);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CAggregateTransformer::ReverseTransform
//
//  Synopsis:
//      Transform points from outer to inner space, used when transforming
//      world bounds to local bounds.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CAggregateTransformer::ReverseTransform(
    _In_reads_(count) const XPOINTF *srcPoints,
    _Out_writes_(count) XPOINTF *destPoints,
    _In_ XUINT32 count)
{
    memcpy( destPoints, srcPoints, count * sizeof(XPOINTF) );

    for ( CXcpList<ITransformer>::XCPListNode *p = m_oList.GetHead();
          p != NULL;
          p = p->m_pNext )
    {
        IFC_RETURN(p->m_pData->ReverseTransform( destPoints, destPoints, count ));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CAggregateTransformer::Add
//
//  Synopsis:
//      Adds another child ITransformer
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CAggregateTransformer::Add( _In_ ITransformer* pChild )
{
    if ( pChild )
    {
        // If we already have a matrix in there
        if ( pChild->GetType() == ITransformer::TransformerType_Matrix
          && m_oList.GetHead()
          && m_oList.GetHead()->m_pData
          && m_oList.GetHead()->m_pData->GetType() == ITransformer::TransformerType_Matrix )
        {
            // Coalesce 2D transforms
            CMatrixTransformer *pHead  = static_cast<CMatrixTransformer*>( m_oList.GetHead()->m_pData );
            CMatrixTransformer *pOther = static_cast<CMatrixTransformer*>( pChild );
            pHead->Append(pOther);
        }
        else
        {
            AddRefInterface(pChild);
            IFC_RETURN(m_oList.Add(pChild));
        }
    }

    return S_OK;
}

