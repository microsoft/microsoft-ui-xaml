// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Description:
//      Describes the basic structures for the compositor rendering tree
//
//  Synopsis:
//
//      This class provides the basic infrastructure to allow the UI thread
//      to construct and draw a simplified "composition tree" based on the UI element tree,
//      but which has the restricted ability to compose surfaces, and independently
//      animate them. A node in the composition tree can only have modifying properties
//      which are simple to accelerate in hardware, eg transform, opacity etc. A
//      node will also have a list of child nodes, and optionally a surface to draw.
//
//      This class provides the ability for the UI thread to construct such a
//      tree, take ownership and manage the lifetime of all necessary resources,
//      and the ability to render the tree given a graphics device.
//
//  Current non goals:
//      - Thread safety - currently there's no thread safety on this class.
//          Eventually this will have to be managed somehow because the independent
//          animations running on the render thread will be targeting members of this
//          class, and the UI thread will be building/modifying properties too.

#pragma once

//------------------------------------------------------------------------
//
//  Represents a resolved clip in the HWWalk.
//
//  Supports either a plain rect clip or a polygon clip
//  Can optimally intersect with other HWClips or XRECTF clips
//
//  All points stored and manipulated here are in CCW winding order. All
//  input points must also be CCW winding.
//
//------------------------------------------------------------------------
class HWClip
{
    friend class TransformAndClipFrame; // for inlining a clip into a TransformAndClipFrame

private:
    // Block shallow copy
    // Needed to inline TransformAndClipFrame into a vector.
    HWClip(_In_ const HWClip &other);

    HWClip& operator=(_In_ const HWClip &other)
    {
        XCP_FAULT_ON_FAILURE(FALSE);
        return *this;
    }


    void TransformPointsHelper(_In_ const CMILMatrix *pTransform);

public:
    HWClip();
    ~HWClip();

    bool Equals(_In_ const HWClip &other) const;

    void Set(_In_ const XRECTF *pClip);
    _Check_return_ HRESULT Set(_In_ const HWClip *pClip);
    void Reset();

    _Check_return_ HRESULT Transform(_In_ const CMILMatrix *pTransform);

    _Check_return_ HRESULT Transform(_In_ std::shared_ptr<const CMILMatrix4x4> pTransform);

    template <typename PointType>
    _Check_return_ HRESULT Clip(
        XUINT32 cUnclippedPoints,
        _In_reads_(cUnclippedPoints) PointType *pUnclippedPoints,
        _Out_ XUINT32 *pClippedPointsCount,
        _Outptr_result_buffer_(*pClippedPointsCount) PointType **ppClippedPoints
        ) const;

    //
    // Intersects with another HWClip.
    // - If both are rectangular and non-rotated, this can be a simple intersection.
    // - If both are rectangular and rotated at the same angle, this can be a simple intersection
    //      (this optimization not currently implemented - polygon clip is produced)
    // - In all other cases, this will produce a polygon clip
    //
    // Clips should be in the same space
    //
    _Check_return_ HRESULT Intersect(_In_opt_ const HWClip *pOther);
    _Check_return_ HRESULT Intersect(_In_opt_ const XRECTF *pClip);

    bool DoesIntersect(_In_ const XRECTF& clip) const;

    void GetBounds(_Out_ XRECTF *pBounds) const;

    bool IsRectilinear() const
    {
        // 0 points implies NULL array
        ASSERT(m_cPoints != 0 || m_pPoints == NULL);
        return (m_cPoints == 0);
    }

    bool IsClipAxisAlignedWith(_In_ const CMILMatrix* pWorldTransform) const;

    bool Contains(_In_ const XPOINTF *pPoint) const;
    bool DoesEntirelyContain(_In_ const HWClip *pOther) const;
    bool DoesEntirelyContain(_In_ const XRECTF *pPolygon) const;

    template <typename PointType>
    bool DoesEntirelyContainPoints(
        XUINT32 pointCount,
        _In_reads_(pointCount) const PointType *pPoints
        ) const;

    bool IsEmpty() const;
    bool IsInfinite() const;

    bool IsRectangular() const { return m_fIsRectangular; }
    void GetRectangularClip(_Out_ XRECTF *pRect) const;

    _Check_return_ HRESULT GetRectilinearClip(_Out_ XRECTF *pRect, _In_ const CMILMatrix* pTransform) const;

    void GetPolygonClip(_Outptr_result_buffer_(*pPointsCount) XPOINTF **ppPoints, _Out_ XUINT32 *pPointsCount) const;

    _Check_return_ HRESULT ApplyTransformAndClip(
        _In_opt_ const std::shared_ptr<const CMILMatrix4x4> p3DProjection,
        _In_ const CMILMatrix *p2DTransform,
        _In_ const HWClip *p2DClip,
        _Out_opt_ bool *pWasClipped
        );

    static _Check_return_ HRESULT ApplyTransformAndClip_DropZPreserveW(
        XUINT32 sourcePointCount,
        _Inout_updates_(sourcePointCount) XPOINTF4 *pSourcePoints,
        _In_opt_ const std::shared_ptr<const CMILMatrix4x4> p3DProjection,
        _In_ const CMILMatrix *p2DTransform,
        _In_ const HWClip *p2DClip,
        _Out_ XUINT32 *pDestPointCount,
        _Outptr_result_buffer_(*pDestPointCount) XPOINTF4 **ppDestPoints,
        _Out_opt_ bool *pWasClipped
        );

private:

    // m_clip will exist if rotated but will be invalid. (including multiples of 90 degrees)
    XRECTF m_clip;

    XPOINTF *m_pPoints;
    XUINT32 m_cPoints;
    CMILMatrix m_transform;
    bool m_fIsEmpty    : 1;
    bool m_fIsInfinite : 1;

    // true if the clip is rectangular but does not imply that the clip is rectilinear
    // ie a set of points that are rectangular but rotated or skewed makes this true while not being rectilinear
    bool m_fIsRectangular : 1;
};

//------------------------------------------------------------------------
//
//  Synopsis:
//      The combination of a 3D homogeneous transform and a HWClip under
//      a projection.
//
//------------------------------------------------------------------------
class TransformAndClipFrame final
{
    friend class TransformAndClipStack;

public:
    // for putting in xvector
    TransformAndClipFrame();
    virtual ~TransformAndClipFrame();

private:
    void PrependTransform(
        _In_ const CMILMatrix& transform
        );

    _Check_return_ HRESULT IntersectLocalSpaceClip(
        _Inout_ HWClip *pClip
        );

    void SetProjection(const CMILMatrix4x4* const pProjection);

    void Reset();

    _Check_return_ HRESULT Set(
        _In_ const TransformAndClipFrame *pOther
        );

    void SetTransformAndProjection(
        _In_ const TransformAndClipFrame *pOther
        );

    _Check_return_ HRESULT Prepend(
        _In_ const TransformAndClipFrame *pOther
        );

    bool Equals(
        _In_ const TransformAndClipFrame& other
        ) const;

    _Check_return_ HRESULT TransformToLocalSpace(
        _Inout_ HWClip *pWorldClip
        ) const;

    void GetProjectionWithoutZ(
        _Out_ CMILMatrix4x4 *pCombinedTransform
        ) const;

    void PrependNetTransformTo(
        _Inout_ CMILMatrix4x4 *pCombinedTransform
        ) const;

    void Get2DTransform(
        _Out_ CMILMatrix *p2DTransform
        ) const;

    void Set2DTransform(_In_ const CMILMatrix& transform);

    bool HasInfiniteClip() const;

    bool HasZeroSizedClip() const;

    bool HasProjection() const;

private:
    //
    // The 2D transforms are kept separate from the 3D projection because they'll be needed to
    // determine whether the clip can be optimized.
    //
    CMILMatrix m_2DTransform;

    //
    // This is the projection for all frames above this one (farther from the root than this).
    // It doesn't get applied to the clip in this frame, but does get applied to the clip in
    // all frames above this.
    //
    // Since the projection is prepended last into the frame, the frame essentially becomes
    // read-only once it's set, and no longer accepts transforms or clips.
    //
    std::shared_ptr<const CMILMatrix4x4> m_p3DTransform;

    //
    // Stored in the coordinate space of the rootmost element in the frame. The projection in
    // this frame does not apply to the clip.
    //
    HWClip m_clip;
};


//------------------------------------------------------------------------
//
//  Synopsis:
//      A stack of transform and clip frames. Used to track the net
//      transform and clip of a primitive rendered in PC.
//
//      Each projected element will push a frame on the stack.
//
//      For a tree that looks like:
//        <root>
//          <a>
//            <b>
//              <c projection>
//                <d>
//                  <e projection/>
//                </d>
//              </c>
//            </b>
//          </a>
//        </root>
//
//      The corresponding stack will be:
//        {
//          transform: identity
//               clip: e (in e's coordinate space)
//        }
//        {
//          transform: eProjection-e-d
//               clip: c-d (in c's coordinate space)
//        }
//        {
//          transform: cProjection-c-b-a-root
//               clip: root-a-b (in root's coordinate space)
//        }
//
//      Note that the transform for each frame in the stack has at most
//      one projection, and that the projection is the last transform
//      to be prepended into the frame.
//
//      The individual frames are needed because near-plane clipping
//      needs to be done on a per-projection basis. If the transforms are
//      combined, then there's no way to clip once per projection
//      involved.
//
//      The individual frames will also be needed if we want to push a 2D
//      clip into a projection in order to take the fast clipping path.
//      The full projection transform (which preserves the z coordinate)
//      must be available in order for it to be invertible, but the full
//      projections can't be multiplied together without dropping the z
//      coordinate first.
//
//------------------------------------------------------------------------
class TransformAndClipStack final
{
public:
    TransformAndClipStack();
    TransformAndClipStack(_In_ const XRECTF& backBufferSize);
    virtual ~TransformAndClipStack();

    static _Check_return_ HRESULT Create(
        _In_opt_ const TransformAndClipStack *pOriginalTransformAndClipStack,
        _Outptr_ TransformAndClipStack **ppTransformAndClipStackCopy
        );

    void PrependTransform(
        _In_ const CMILMatrix& transform
        );

    _Check_return_ HRESULT IntersectLocalSpaceClip(
        _Inout_ HWClip *pClip
        );

    _Check_return_ HRESULT PrependProjection(const CMILMatrix4x4 *pProjection);

    void Reset();

    _Check_return_ HRESULT Set(
        _In_opt_ const TransformAndClipStack *pOther
        );

    _Check_return_ HRESULT PushTransformsAndClips(
        _In_ const TransformAndClipStack *pOther
        );

    bool Equals(
        _In_opt_ const TransformAndClipStack& other
        ) const;

    void GetCombinedTransform(
        _Out_ CMILMatrix4x4 *pCombinedTransform
        ) const;

    void Get2DTransformInLeafmostProjection(
        _Out_ CMILMatrix *p2DTransform
        ) const;

    void Set2DTransformInLeafmostProjection(
        _In_ const CMILMatrix& transform
        );

    _Check_return_ HRESULT TransformToLocalSpace(_Inout_ HWClip *pWorldClip) const;

    bool HasZeroSizedClip() const;

    bool HasProjection() const;

private:
    TransformAndClipStack& operator=(_In_ const TransformAndClipStack &other);

    TransformAndClipFrame* GetFrame(XUINT32 number);
    TransformAndClipFrame* GetTopFrame();
    const TransformAndClipFrame* GetTopFrameConst() const;
    _Check_return_ HRESULT PushFrame(_Outptr_result_maybenull_ TransformAndClipFrame **ppNewFrame);

private:
    // Keep one frame inlined to prevent memory allocation in the general case
    TransformAndClipFrame m_bottomFrame;
    // Using xvector rather than xstack because:
    //  - xstack::top() isn't a const method.
    //  - indexer isn't available to xstack due to private inheritance
    xvector<TransformAndClipFrame> m_additionalFrames;
};

#include "TransformToRoot.h"
