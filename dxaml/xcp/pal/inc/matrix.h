// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <windows.foundation.numerics.h>
#include <vector>
#include <gsl/span>
#include <WindowsNumerics.h>
#include "paltypes.h"
#include <NamespaceAliases.h>

class CValue;

//--------------------------------------------------------------------------
// Represents a 2D affine transformation matrix
//--------------------------------------------------------------------------

class CMILMatrix
{
public:
    CMILMatrix()
    {
    }

    CMILMatrix(bool fInitialize)
    {
        if (fInitialize)
        {
            SetToIdentity();
        }
    }

    CMILMatrix(_In_ const XMATRIX &matrix)
        : _11(matrix._11)
        , _12(matrix._12)
        , _21(matrix._21)
        , _22(matrix._22)
        , _31(matrix._31)
        , _32(matrix._32)
    {
    }


    void SetToEmpty();
    void SetToIdentity();
    void SetToRotation(XFLOAT rAngle);
    void SetToRotationAboutCenter(XFLOAT angle, XFLOAT centerX, XFLOAT centerY);
    void SetToScaleAboutCenter(XFLOAT scaleX, XFLOAT scaleY, XFLOAT centerX, XFLOAT centerY);

    bool IsEmpty() const
    {
        return _11 == 0.0f
            && _12 == 0.0f
            && _21 == 0.0f
            && _22 == 0.0f
            && _31 == 0.0f
            && _32 == 0.0f;
    }

    bool IsIdentity() const
    {
        return _11 == 1.0f
            && _12 == 0.0f
            && _21 == 0.0f
            && _22 == 1.0f
            && _31 == 0.0f
            && _32 == 0.0f;
    }

    bool IsTranslationOnly() const
    {
        return _11 == 1.0f
            && _22 == 1.0f
            && _12 == 0.0f
            && _21 == 0.0f;
    }

    bool IsScaleOnly() const
    {
        return _12 == 0.0f
            && _21 == 0.0f
            && _31 == 0.0f
            && _32 == 0.0f;
    }

    bool IsScaleOrTranslationOnly() const
    {
        return _12 == 0.0f
            && _21 == 0.0f;
    }

    bool IsAxisAlignedScaleOrTranslationReal() const;

    void Transform(_In_reads_(cPoint) const XPOINTF *pSource,
                   _Out_writes_(cPoint) XPOINTF *pTarget,
                   _In_ XUINT32 cPoint) const;

    void Transform(const gsl::span<XPOINTF>& points) const;
    void Transform(const XPOINTF& source, XPOINTF& destination) const;

    void Transform3DPoints_PreserveW(const gsl::span<XPOINTF4>& points) const;

    void Transform(_In_reads_(cPoint) const XPOINTD *pSource,
                   _Out_writes_(cPoint) XPOINTD *pTarget,
                   _In_ XUINT32 cPoint) const;

    void TransformBounds(
                   _In_ const XRECTF *pSource,
                   _Out_ XRECTF *pTarget) const;

    void TransformBounds(
                   _In_ const XRECTF_RB *pSource,
                   _Out_ XRECTF_RB *pTarget) const;

    void TransformAsVectors(
        _In_reads_(count) const XPOINTF *srcVectors,
        _Out_writes_(count) XPOINTF *destVectors,
        _In_ XUINT32 count
        ) const;

    void Append(_In_ const CMILMatrix& m);

    inline void AppendTranslation(
        const XFLOAT& translateX,
        const XFLOAT& translateY
        )
    {
        _31 += translateX;
        _32 += translateY;
    }

    void Prepend(_In_ const CMILMatrix& m);

    void Scale(_In_ XFLOAT rScaleX, _In_ XFLOAT rScaleY);

    bool Invert();

    void InferAffineMatrix(
        _In_reads_(3) const XPOINTF *pTargetPoints,
        _In_reads_(1) const XRECTF &rcSource
        );

    XFLOAT GetMaxFactor() const;

    float GetM11() const { return _11; }
    float GetM12() const { return _12; }
    float GetM21() const { return _21; }
    float GetM22() const { return _22; }
    float GetDx() const { return _31; }
    float GetDy() const { return _32; }

    void SetM11(float r) { _11 = r; }
    void SetM12(float r) { _12 = r; }
    void SetM21(float r) { _21 = r; }
    void SetM22(float r) { _22 = r; }
    void SetDx(float dx) { _31 = dx; }
    void SetDy(float dy) { _32 = dy; }

    float GetDeterminant() const
    {
        return _11*_22 - _12*_21;
    }

    void GetScaleDimensions(
        _Out_writes_(1) XFLOAT *prScaleX,
        _Out_writes_(1) XFLOAT *prScaleY
    ) const;

    bool operator==(const CMILMatrix& other) const;

    bool operator!=(const CMILMatrix& other) const;

    XMATRIX GetXMATRIX() const
    {
        XMATRIX matrix;
        matrix._11 = _11;
        matrix._12 = _12;
        matrix._21 = _21;
        matrix._22 = _22;
        matrix._31 = _31;
        matrix._32 = _32;

        return matrix;
    }

    void ToMatrix3x2(
        _Out_ wfn::Matrix3x2* outputMatrix
    ) const
    {
        outputMatrix->M11 = _11;
        outputMatrix->M12 = _12;
        outputMatrix->M21 = _21;
        outputMatrix->M22 = _22;
        outputMatrix->M31 = _31;
        outputMatrix->M32 = _32;
    }

    void ToMatrix4x4(
        _Out_ wfn::Matrix4x4* outputMatrix
        ) const
    {
        outputMatrix->M11 = _11;
        outputMatrix->M12 = _12;
        outputMatrix->M13 = 0.0f;
        outputMatrix->M14 = 0.0f;
        outputMatrix->M21 = _21;
        outputMatrix->M22 = _22;
        outputMatrix->M23 = 0.0f;
        outputMatrix->M24 = 0.0f;
        outputMatrix->M31 = 0.0f;
        outputMatrix->M32 = 0.0f;
        outputMatrix->M33 = 1.0f;
        outputMatrix->M34 = 0.0f;
        outputMatrix->M41 = _31;
        outputMatrix->M42 = _32;
        outputMatrix->M43 = 0.0f;
        outputMatrix->M44 = 1.0f;
    }

public:
    float _11;
    float _12;
    float _21;
    float _22;
    float _31;
    float _32;
};

//--------------------------------------------------------------------------
// Represents a 4x4 matrix
//--------------------------------------------------------------------------
class CMILMatrix4x4
{
public:
    CMILMatrix4x4()
    {
    }

    explicit CMILMatrix4x4(bool fInitialize)
    {
        if (fInitialize)
        {
            SetToIdentity();
        }
    }

    explicit CMILMatrix4x4(_In_ const CMILMatrix *pOther);
    explicit CMILMatrix4x4(_In_ const wfn::Matrix4x4& matrix4x4);

    CMILMatrix4x4(_In_ const CMILMatrix4x4 *pOther) = delete;   // Blocked. Use the implicit copy ctor instead.

    static CMILMatrix4x4 FromFloatArray(_In_ CValue& value);

    void SetToIdentity();

    bool IsIdentity() const
    {
        return (   _11 == 1.0f && _12 == 0.0f && _13 == 0.0f && _14 == 0.0f
                && _21 == 0.0f && _22 == 1.0f && _23 == 0.0f && _24 == 0.0f
                && _31 == 0.0f && _32 == 0.0f && _33 == 1.0f && _34 == 0.0f
                && _41 == 0.0f && _42 == 0.0f && _43 == 0.0f && _44 == 1.0f);
    }

    void SetToRotationX(float angleInDegrees);
    void SetToRotationY(float angleInDegrees);
    void SetToRotationZ(float angleInDegrees);

    void SetToScale(float scaleX, float scaleY, float scaleZ);
    void SetToScaleAboutCenter(const wfn::Vector3& scale, const wfn::Vector3& centerPoint);

    void SetToTranslation(float translateX, float translateY, float translateZ);

    void SetToPerspectiveTransform3D(float centerX, float centerY, float depth);

    void SetToPerspective(
            XFLOAT rNear,
            XFLOAT rFar,
            XFLOAT rFieldOfView,
            XFLOAT rAspectRatio,
            XUINT32 bRightHanded = TRUE
            );

    void SetTo2DTransform( _In_ const CMILMatrix *pmat2D );

    CMILMatrix Get2DRepresentation() const;
    CMILMatrix Extract2DComponents() const;

    bool Is2D() const;

    bool IsScaleOrTranslationOnly() const;

    void Get2DScaleDimensions(
        _In_ const XSIZEF &elementSize,
        XFLOAT minScaleX,
        XFLOAT minScaleY,
        _Out_ XFLOAT *pScaleX,
        _Out_ XFLOAT *pScaleY
        ) const;

    void Transform_PreserveW(const gsl::span<const XPOINTF4>& source, const gsl::span<XPOINTF4>& destination) const;

    void Transform_PreserveW(const gsl::span<SDPoint4UV>& points) const;

    bool Transform2DPoints_DivideW(const gsl::span<const XPOINTF>& source, const gsl::span<XPOINTF>& destination) const;

    void TransformBoundsIgnoreZ(
        _In_ const XRECTF *pSource,
        _Out_ XRECTF *pTarget
        ) const;

    void Append(const CMILMatrix4x4& matrixBehind);

    void Append(_In_ const CMILMatrix& m);

    void Prepend(const CMILMatrix4x4& matrixInFront);

    void Prepend_DropZ(_In_ const CMILMatrix& m);

    float GetM11() const { return _11; }
    float GetM12() const { return _12; }
    float GetM13() const { return _13; }
    float GetM14() const { return _14; }
    float GetM21() const { return _21; }
    float GetM22() const { return _22; }
    float GetM23() const { return _23; }
    float GetM24() const { return _24; }
    float GetM31() const { return _31; }
    float GetM32() const { return _32; }
    float GetM33() const { return _33; }
    float GetM34() const { return _34; }
    float GetM41() const { return _41; }
    float GetM42() const { return _42; }
    float GetM43() const { return _43; }
    float GetM44() const { return _44; }

    void SetM11(float r) { _11 = r; }
    void SetM12(float r) { _12 = r; }
    void SetM13(float r) { _13 = r; }
    void SetM14(float r) { _14 = r; }
    void SetM21(float r) { _21 = r; }
    void SetM22(float r) { _22 = r; }
    void SetM23(float r) { _23 = r; }
    void SetM24(float r) { _24 = r; }
    void SetM31(float r) { _31 = r; }
    void SetM32(float r) { _32 = r; }
    void SetM33(float r) { _33 = r; }
    void SetM34(float r) { _34 = r; }
    void SetM41(float r) { _41 = r; }
    void SetM42(float r) { _42 = r; }
    void SetM43(float r) { _43 = r; }
    void SetM44(float r) { _44 = r; }

    bool operator==(const CMILMatrix4x4& other) const;

    bool operator!=(const CMILMatrix4x4& other) const;

    void ToMatrix4x4(
        _Out_ wfn::Matrix4x4* outputMatrix
        ) const
    {
        ASSERT(sizeof(CMILMatrix4x4) == sizeof(wfn::Matrix4x4));
        memcpy(outputMatrix, this, sizeof(wfn::Matrix4x4));
    }

    // Given a span of points in world space, figure out the corresponding local space points using the world space transform
    // represented by this 4x4 matrix. If a point cannot be transformed into local space, it's not part of the output.
    // Note: We have two ways to inverse transform a point through a 3D matrix. One is used for projections, the other for Transform3D.
    // TODO: HitTest: Go with the faster one. Although using an inverse can be extended to do ray hit testing.
    // TODO: HitTest: WithInverse requires an invertible 4x4 matrix, which means no flattening.
    // TODO: HitTest: Take one span and transform it in-place?
    bool TransformWorldToLocalWithInverse(const gsl::span<const XPOINTF>& worldSpacePoints, const gsl::span<XPOINTF>& localSpacePoints) const;
    bool TransformWorldToLocalWithInverse(const XPOINTF& worldSpacePoint, XPOINTF& localSpacePoint) const;
    bool TransformWorldToLocalWithInterpolation(const gsl::span<const XPOINTF>& worldSpacePoints, const gsl::span<XPOINTF>& localSpacePoints) const;

private:
    static bool TransformWorldToLocalHelper(const wfn_::float4x4& inverse, const XPOINTF& worldSpacePoint, XPOINTF& localSpacePoint);

    // Templated gsl::span methods are hard to use, because the call site needs to match the span exactly. There's no
    // conversion from raw arrays, or from span<point> to span<const point>. So declare the methods with concrete types
    // and have them call the templated implementation under the covers.
    template <typename PointType> void Transform_PreserveW_TemplatedHelper(
        const gsl::span<const PointType>& source,
        const gsl::span<PointType>& destination) const;

public:
    union
    {
        struct
        {
            float   _11, _12, _13, _14;
            float   _21, _22, _23, _24;
            float   _31, _32, _33, _34;
            float   _41, _42, _43, _44;
        };

        float m[4][4];
    };
};

bool Matrix4x4Is2D(const  wfn::Matrix4x4& matrix);
