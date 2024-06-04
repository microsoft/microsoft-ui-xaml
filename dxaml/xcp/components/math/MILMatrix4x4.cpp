// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Matrix.h"
#include "MinMath.h"

void CMILMatrix4x4::Prepend(const CMILMatrix4x4& that)
{
    CMILMatrix4x4 temp(*this);

    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            m[i][j] = 0.0f;

            for (int k = 0; k < 4; k++)
            {
                m[i][j] += that.m[i][k] * temp.m[k][j];
            }
        }
    }
}

void CMILMatrix4x4::Append(const CMILMatrix4x4& that)
{
    CMILMatrix4x4 temp(*this);

    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            m[i][j] = 0.0f;

            for (int k = 0; k < 4; k++)
            {
                m[i][j] += temp.m[i][k] * that.m[k][j];
            }
        }
    }
}

void CMILMatrix4x4::SetToIdentity()
{
    _11 = 1.0f;
    _12 = 0.0f;
    _13 = 0.0f;
    _14 = 0.0f;

    _21 = 0.0f;
    _22 = 1.0f;
    _23 = 0.0f;
    _24 = 0.0f;

    _31 = 0.0f;
    _32 = 0.0f;
    _33 = 1.0f;
    _34 = 0.0f;

    _41 = 0.0f;
    _42 = 0.0f;
    _43 = 0.0f;
    _44 = 1.0f;
}

void CMILMatrix4x4::SetToRotationX(float angleInDegrees)
{
    float cos = MathCosDegrees(angleInDegrees);
    float sin = MathSinDegrees(angleInDegrees);

    _11 = 1.0f;
    _12 = 0.0f;
    _13 = 0.0f;
    _14 = 0.0f;

    _21 = 0.0f;
    _22 = cos;
    _23 = -sin;
    _24 = 0.0f;

    _31 = 0.0f;
    _32 = sin;
    _33 = cos;
    _34 = 0.0f;

    _41 = 0.0f;
    _42 = 0.0f;
    _43 = 0.0f;
    _44 = 1.0f;
}

void CMILMatrix4x4::SetToRotationY(float angleInDegrees)
{
    float cos = MathCosDegrees(angleInDegrees);
    float sin = MathSinDegrees(angleInDegrees);

    _11 = cos;
    _12 = 0.0f;
    _13 = sin;
    _14 = 0.0f;

    _21 = 0.0f;
    _22 = 1.0f;
    _23 = 0.0f;
    _24 = 0.0f;

    _31 = -sin;
    _32 = 0.0f;
    _33 = cos;
    _34 = 0.0f;

    _41 = 0.0f;
    _42 = 0.0f;
    _43 = 0.0f;
    _44 = 1.0f;
}

void CMILMatrix4x4::SetToRotationZ(float angleInDegrees)
{
    float cos = MathCosDegrees(angleInDegrees);
    float sin = MathSinDegrees(angleInDegrees);

    _11 = cos;
    _12 = -sin;
    _13 = 0.0f;
    _14 = 0.0f;

    _21 = sin;
    _22 = cos;
    _23 = 0.0f;
    _24 = 0.0f;

    _31 = 0.0f;
    _32 = 0.0f;
    _33 = 1.0f;
    _34 = 0.0f;

    _41 = 0.0f;
    _42 = 0.0f;
    _43 = 0.0f;
    _44 = 1.0f;
}

void CMILMatrix4x4::SetToScale(float scaleX, float scaleY, float scaleZ)
{
    _11 = scaleX;
    _12 = 0.0f;
    _13 = 0.0f;
    _14 = 0.0f;

    _21 = 0.0f;
    _22 = scaleY;
    _23 = 0.0f;
    _24 = 0.0f;

    _31 = 0.0f;
    _32 = 0.0f;
    _33 = scaleZ;
    _34 = 0.0f;

    _41 = 0.0f;
    _42 = 0.0f;
    _43 = 0.0f;
    _44 = 1.0f;
}

void CMILMatrix4x4::SetToScaleAboutCenter(const wfn::Vector3& scale, const wfn::Vector3& centerPoint)
{
    // [ 1  0  0  0 ][ Sx  0  0  0 ][ 1 0 0 0 ]   [   Sx    0    0  0 ][ 1 0 0 0 ]   [    Sx      0      0   0 ]
    // [ 0  1  0  0 ][  0 Sy  0  0 ][ 0 1 0 0 ] = [    0   Sy    0  0 ][ 0 1 0 0 ] = [     0     Sy      0   0 ]
    // [ 0  0  1  0 ][  0  0 Sz  0 ][ 0 0 1 0 ]   [    0    0   Sz  0 ][ 0 0 1 0 ]   [     0      0     Sz   0 ]
    // [-x -y -z  1 ][  0  0  0  1 ][ x y z 1 ]   [ -xSx -ySy -zSz  1 ][ x y z 1 ]   [ -xSx+x -ySy+y -zSz+z  1 ]

    _11 = scale.X;
    _12 = 0.0f;
    _13 = 0.0f;
    _14 = 0.0f;

    _21 = 0.0f;
    _22 = scale.Y;
    _23 = 0.0f;
    _24 = 0.0f;

    _31 = 0.0f;
    _32 = 0.0f;
    _33 = scale.Z;
    _34 = 0.0f;

    _41 = -centerPoint.X * scale.X + centerPoint.X;
    _42 = -centerPoint.Y * scale.Y + centerPoint.Y;
    _43 = -centerPoint.Z * scale.Z + centerPoint.Z;
    _44 = 1.0f;
}

void CMILMatrix4x4::SetToTranslation(float translateX, float translateY, float translateZ)
{
    _11 = 1.0f;
    _12 = 0.0f;
    _13 = 0.0f;
    _14 = 0.0f;

    _21 = 0.0f;
    _22 = 1.0f;
    _23 = 0.0f;
    _24 = 0.0f;

    _31 = 0.0f;
    _32 = 0.0f;
    _33 = 1.0f;
    _34 = 0.0f;

    _41 = translateX;
    _42 = translateY;
    _43 = translateZ;
    _44 = 1.0f;
}

void CMILMatrix4x4::SetToPerspectiveTransform3D(float centerX, float centerY, float depth)
{
    //
    // Projection = Translate(-dX, -dY) x Perspective(Depth) x Translate(dX, dY)
    //
    //      This multiplication translates the origin to the center of the element modified by offsets, applies a perspective,
    //      and translates back.
    //
    //   ** Note: This does not apply a flattening matrix.
    //
    // Translate(dX, dY):
    //      [  1  0  0  0 ]
    //      [  0  1  0  0 ]
    //      [  0  0  1  0 ]
    //      [ dX dY  0  1 ]
    //
    // Perspective(D):
    //      [ 1   0   0   0  ]
    //      [ 0   1   0   0  ]
    //      [ 0   0   1 -1/D ]
    //      [ 0   0   0   1  ]
    //
    // The resulting matrix is:
    //      [    1     0     0     0  ]
    //      [    0     1     0     0  ]
    //      [ -dX/D -dY/D    1   -1/D ]
    //      [    0     0     0     1  ]
    //

    float inverseDepth = 1 / depth;

    _11 = 1.0f;
    _12 = 0.0f;
    _13 = 0.0f;
    _14 = 0.0f;

    _21 = 0.0f;
    _22 = 1.0f;
    _23 = 0.0f;
    _24 = 0.0f;

    _31 = -centerX / depth;
    _32 = -centerY / depth;
    _33 = 1.0f;
    _34 = -inverseDepth;

    _41 = 0;
    _42 = 0;
    _43 = 0;
    _44 = 1.0f;
}

template <typename PointType> void CMILMatrix4x4::Transform_PreserveW_TemplatedHelper(
    const gsl::span<const PointType>& source,
    const gsl::span<PointType>& destination) const
{
    ASSERT(source.size() == destination.size());

    for (int i = 0; i < source.size(); i++)
    {
        float x = source[i].x;
        float y = source[i].y;
        float z = source[i].z;
        float w = source[i].w;

        destination[i].x = (_11 * x) + (_21 * y) + (_31 * z) + (_41 * w);
        destination[i].y = (_12 * x) + (_22 * y) + (_32 * z) + (_42 * w);
        destination[i].z = (_13 * x) + (_23 * y) + (_33 * z) + (_43 * w);
        destination[i].w = (_14 * x) + (_24 * y) + (_34 * z) + (_44 * w);
    }
}

void CMILMatrix4x4::Transform_PreserveW(
    const gsl::span<const XPOINTF4>& source,
    const gsl::span<XPOINTF4>& destination) const
{
    Transform_PreserveW_TemplatedHelper(source, destination);
}

void CMILMatrix4x4::Transform_PreserveW(const gsl::span<SDPoint4UV>& points) const
{
    const gsl::span<const SDPoint4UV> constPoints(points);
    Transform_PreserveW_TemplatedHelper(constPoints, points);
}

bool CMILMatrix4x4::TransformWorldToLocalWithInverse(const gsl::span<const XPOINTF>& worldSpacePoints, const gsl::span<XPOINTF>& localSpacePoints) const
{
    ASSERT(worldSpacePoints.size() == localSpacePoints.size());
    ASSERT(!IsIdentity());  // It's the caller's job to check for identity and not call this method.

    bool areAllPointsTransformed = true;

    //
    // Given a 2D world space point (X, Y), find a point in local space that will render as (X, Y) on screen.
    //
    // That local space point will be transformed through the world transform and become the world space point (Xw, Yw, z, w)
    // for some z and w. We take that world space point and transform it back through the inverse world space transform to get
    // the local space point that we're after.
    //
    // Let the inverse matrix be:
    //      [ m_11  m_12  m_13  m_14 ]
    //      [ m_21  m_22  m_23  m_24 ]
    //      [ m_31  m_32  m_33  m_34 ]
    //      [ m_41  m_42  m_43  m_44 ]
    //
    // Then the local space point is the world space point (Xw, Yw, z, w) multiplied by the inverse, which is:
    //      (w*C_1 + z*m_31,    w*C_2 + z*m_32,    w*C_3 + z*m_33,    w*C_4 + z*m_34)
    //
    //   ** Using this short-hand for simplification:
    //          C_n = X*m_1n + Y*m_2n + m_4n
    //
    // We can find w and z because the Z coordinate of this local space point will be 0, and the W coordinate of this local
    // space point will be 1. That gives:
    //      Z = w*C_3 + z*m_33 = 0
    //      W = w*C_4 + z*m_34 = 1
    //
    // This has the solution:
    //      z =   C_3 / (m_34*C_3 - m_33*C_4)
    //      w = -m_33 / (m_34*C_3 - m_33*C_4)
    //
    //   ** Note that we require the denominator (m_34*C_3 - m_33*C_4) to not be 0.
    //
    // We then substitute this back to get the X and Y coordinates of the local space point:
    //      X = (m_31*C_3 - m_33*C_1) / (m_34*C_3 - m_33*C_4)
    //      Y = (m_32*C_3 - m_33*C_2) / (m_34*C_3 - m_33*C_4)
    //
    // This local space point (X, Y) can be used to proceed with hit testing.
    //
    //   ** Note that we could have picked (Xw', Yw', z'w', w') for the world space point, which solves to different values
    //      for z' and w'. Once we substitute z' and w' back to get the X and Y coordinates for the local space point, we'll
    //      get the same answer for X and Y.
    //
    // Note that this entire procedure assumes that the hit testing ray is perpendicular to the screen. This is what allows
    // us to use (Xw, Yw, z, w) as the world space point representing the intersection between the hit testing ray and the
    // local plane of the element.
    //
    // See the OneNote page for details:
    //      https://microsoft.sharepoint.com/teams/osg_core_dep/xaml/_layouts/OneNote.aspx?id=%2Fteams%2Fosg_core_dep%2Fxaml%2FShared%20Documents%2FXAML%20Dev%20Handbook&wd=target%28Knowledge%20Base%2FXaml%20Foundation%20Team.one%7C3D686604-DDD1-4D56-85B5-013C2A179875%2F3D%20Hit%20Testing%20Math%7CB3C491CC-A7F5-4B93-9763-4BC6583D588E%2F%29
    //      onenote:https://microsoft.sharepoint.com/teams/osg_core_dep/xaml/Shared%20Documents/XAML%20Dev%20Handbook/Knowledge%20Base/Xaml%20Foundation%20Team.one#3D%20Hit%20Testing%20Math&section-id={3D686604-DDD1-4D56-85B5-013C2A179875}&page-id={B3C491CC-A7F5-4B93-9763-4BC6583D588E}&end
    //

    // Find the inverse matrix. That matrix can be reused for all input points.
    wfn_::float4x4 matrix;
    static_assert(sizeof(CMILMatrix4x4) == sizeof(wfn_::float4x4), "Ensuring CMILMatrix4x4 size matches wfn_::float4x4 size");
    memcpy(&matrix, this, sizeof(wfn_::float4x4));
    wfn_::float4x4 inverse;
    bool invertible = invert(matrix, &inverse);

    // The value of C_n depends on the (X, Y) coordinate of the input point, so they can't be precomputed and shared between
    // all input points. Iterate over each world space input and figure out its local space equivalent.
    if (invertible)
    {
        for (int i = 0; i < worldSpacePoints.size(); i++)
        {
            bool isPointTransformed = TransformWorldToLocalHelper(inverse, worldSpacePoints[i], localSpacePoints[i]);
            if (!isPointTransformed)
            {
                areAllPointsTransformed = false;
            }
        }
    }
    else
    {
        areAllPointsTransformed = false;
    }

    return areAllPointsTransformed;
}

bool CMILMatrix4x4::TransformWorldToLocalWithInverse(const XPOINTF& worldSpacePoint, XPOINTF& localSpacePoint) const
{
    wfn_::float4x4 matrix;
    memcpy(&matrix, this, sizeof(wfn_::float4x4));
    wfn_::float4x4 inverse;
    bool invertible = invert(matrix, &inverse);

    if (invertible)
    {
        bool isPointTransformed = TransformWorldToLocalHelper(inverse, worldSpacePoint, localSpacePoint);
        return isPointTransformed;
    }
    else
    {
        return false;
    }
}

bool CMILMatrix4x4::TransformWorldToLocalHelper(const wfn_::float4x4& inverse, const XPOINTF& worldSpacePoint, XPOINTF& localSpacePoint)
{
    float x = worldSpacePoint.x;
    float y = worldSpacePoint.y;

    float c1 = x * inverse.m11 + y * inverse.m21 + inverse.m41;
    float c2 = x * inverse.m12 + y * inverse.m22 + inverse.m42;
    float c3 = x * inverse.m13 + y * inverse.m23 + inverse.m43;
    float c4 = x * inverse.m14 + y * inverse.m24 + inverse.m44;

    float denominator = inverse.m34 * c3 - inverse.m33 * c4;

    if (denominator != 0)
    {
        float localXNumerator = inverse.m31 * c3 - inverse.m33 * c1;
        float localYNumerator = inverse.m32 * c3 - inverse.m33 * c2;

        localSpacePoint.x = localXNumerator / denominator;
        localSpacePoint.y = localYNumerator / denominator;

        return true;
    }

    // Note: We don't check for negative W values here.

    return false;
}

bool CMILMatrix4x4::TransformWorldToLocalWithInterpolation(const gsl::span<const XPOINTF>& worldSpacePoints, const gsl::span<XPOINTF>& localSpacePoints) const
{
    ASSERT(worldSpacePoints.size() == localSpacePoints.size());
    ASSERT(!IsIdentity());  // It's the caller's job to check for identity and not call this method.

    XVERTEX25D origin;
    bool areAllPointsTransformed = true;

    //
    // This method simplifies math from CPerspectiveTransformer, which handles transforms in both directions. Here,
    // we only care about going from world space down to local space.
    //
    // CPerspectiveTransformer works by mapping xy world bounds to uv texture bounds inside the perspective projection.
    // Here, texture space is the same as local space, if we assume that the texture size exactly matches the element's
    // local size. The transformer uses the fact that u/w, v/w, and 1/w can be interpolated linearly across world space
    // to calculate u and v from a given x and y.
    //
    // It starts by defining a set of axes for u and v using three points. Then it transforms all three points through
    // the 3D matrix, to find the transformed axes in world space. Then it calculates a linear combination of the
    // transformed axes corresponding to (1, 0) and (0, 1), and uses those for interpolating.
    //

    //
    // Create 3 points that define a set of local space axes. We use the x and y axes for simplicity. Also use z=0 and
    // w=1 for all points. xyzw will be transformed to world space, and uv will be kept in local space.
    //
    const unsigned int originIndex = 0;
    const unsigned int localXAxisIndex = 1;
    const unsigned int localYAxisIndex = 2;
    //
    //       | x | y | z | w | u | v
    //  -----+---+---+---+---+---+----
    //   [0] | 0 | 0 | 0 | 1 | 0 | 0
    //   [1] | 1 | 0 | 0 | 1 | 1 | 0
    //   [2] | 0 | 1 | 0 | 1 | 0 | 1
    //
    SDPoint4UV points[3];
    points[originIndex].x = points[originIndex].y = points[originIndex].u = points[originIndex].v = 0;
    points[localXAxisIndex].x = points[localXAxisIndex].u = 1;
    points[localXAxisIndex].y = points[localXAxisIndex].v = 0;
    points[localYAxisIndex].x = points[localYAxisIndex].u = 0;
    points[localYAxisIndex].y = points[localYAxisIndex].v = 1;
    points[originIndex].z = points[localXAxisIndex].z = points[localYAxisIndex].z = 0.0f;
    points[originIndex].w = points[localXAxisIndex].w = points[localYAxisIndex].w = 1.0f;

    // Transform the 3D points through the pipeline. Now the points are in world space.
    Transform_PreserveW(points);

    // Prepare the interpolators. Save the world space xy coordinates of the transformed points (which involves
    // dividing by w), as well as the uv texture coordinates that they correspond to. The XVERTEX25D constructor
    // automatically calculates u/w, v/w, and 1/w, which can be interpolated linearly in world space.
    origin = {
            points[originIndex].x / points[originIndex].w,
            points[originIndex].y / points[originIndex].w,
            points[originIndex].u,
            points[originIndex].v,
            points[originIndex].w};

    XVERTEX25D vX(
            points[localXAxisIndex].x / points[localXAxisIndex].w,
            points[localXAxisIndex].y / points[localXAxisIndex].w,
            points[localXAxisIndex].u,
            points[localXAxisIndex].v,
            points[localXAxisIndex].w);

    XVERTEX25D vY(
            points[localYAxisIndex].x / points[localYAxisIndex].w,
            points[localYAxisIndex].y / points[localYAxisIndex].w,
            points[localYAxisIndex].u,
            points[localYAxisIndex].v,
            points[localYAxisIndex].w);

    // Now find the transformed versions of the local space axes. Index [0] is the origin. One is [1]-[0],
    // and the other is [2]-[0]. These are the transformed axes in world space.
    XVERTEX25D transformedXAxis = vX - origin;
    XVERTEX25D transformedYAxis = vY - origin;

    // These transformed axes can be used to interpolate, but they're difficult to use. For example, transformedXAxis
    // stores the change in u/w, v/w, and 1/w for every change of (transformedXAxis.X, transformedXAxis.Y) in world
    // space. What we want is the change in u/w, v/w, and 1/w for every change of (1, 0) and (0, 1) in world space.
    // To do that, we compute linear combinations of transformedXAxis and transformedYAxis that result in (1, 0)
    // and (0, 1).

    //
    // Define a matrix where the rows are the transformed axes, (x1, y1) and (x2, y2):
    //      [x1 y1]
    //      [x2 y2]
    //
    // Find the inverse of this matrix, and let it be:
    //      [a b]
    //      [c d]
    //
    // Since inverses multiply to identity, we have:
    //      [a b][x1 y1]   [1 0]
    //      [c d][x2 y2] = [0 1]
    //
    // Which means:
    //      [a b][x1 y1] = [1 0], and [c d][x1 y1] = [0 1]
    //           [x2 y2]                   [x2 y2]
    //
    // So we end up with (1, 0) and (0, 1). This is exactly the linear combination that we're looking for. Then
    // we want to put the axes in a matrix, invert it, read the rows off, and use that to compute the linear combination.
    //

    CMILMatrix linearCombinationMatrix;
    linearCombinationMatrix._11 = transformedXAxis.x;
    linearCombinationMatrix._12 = transformedXAxis.y;
    linearCombinationMatrix._21 = transformedYAxis.x;
    linearCombinationMatrix._22 = transformedYAxis.y;
    linearCombinationMatrix._31 = 0.0f;
    linearCombinationMatrix._32 = 0.0f;

    // Note that if the transformed axes end up parallel, this matrix will not be invertible. That can happen if there's
    // a 90 degree rotation, for example.
    bool isInvertible = linearCombinationMatrix.Invert();

    if (isInvertible)
    {
        // Since the original matrix has _31 and _32 as 0, the inverse should also have them as 0.
        ASSERT(linearCombinationMatrix._31 == 0);
        ASSERT(linearCombinationMatrix._32 == 0);

        // Read the rows off and calculate the linear combination.
        XVERTEX25D oneDx = (transformedXAxis * linearCombinationMatrix._11) + (transformedYAxis * linearCombinationMatrix._12);
        XVERTEX25D oneDy = (transformedXAxis * linearCombinationMatrix._21) + (transformedYAxis * linearCombinationMatrix._22);

        // Transform the points
        for (int i = 0; i < worldSpacePoints.size(); i++)
        {
            // Adjust hit point by our starting offset
            XPOINTF pt = worldSpacePoints[i];
            pt.x -= origin.x;
            pt.y -= origin.y;

            // Compute where the hit point would project onto our texture
            XVERTEX25D vHit = origin + (oneDx * pt.x) + (oneDy * pt.y);
            if (vHit.oneOverW < 0)
            {
                areAllPointsTransformed = false;
            }

            // Compute UV coordinates - these are XY in local space
            localSpacePoints[i].x = vHit.u();
            localSpacePoints[i].y = vHit.v();
        }

        return areAllPointsTransformed;
    }
    else
    {
        return false;
    }
}
