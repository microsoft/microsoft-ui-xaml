// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlLogging.h"
#include "MILMatrix4x4UnitTests.h"
#include "Matrix.h"

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Math {

void MILMatrix4x4UnitTests::VerifyPoint(const XPOINTF& expected, const XPOINTF& actual, int index)
{
    static float epsilon = 0.005f;

    if (actual.x > expected.x + epsilon
        || actual.x < expected.x - epsilon
        || actual.y > expected.y + epsilon
        || actual.y < expected.y - epsilon)
    {
        VERIFY_FAIL(WEX::Common::String().Format(L"Point at index [%i]: expected (%f, %f) but got (%f, %f)", index, expected.x, expected.y, actual.x, actual.y));
    }
    else
    {
        LOG_OUTPUT(L"  Passed - point at index [%i]: expected (%f, %f), got (%f, %f)", index, expected.x, expected.y, actual.x, actual.y);
    }
}

void MILMatrix4x4UnitTests::VerifyPoints(const CMILMatrix4x4& transform, const gsl::span<const XPOINTF>& input, const gsl::span<const XPOINTF>& expected)
{
    VerifyPoints(transform, input, expected, true);
}

void MILMatrix4x4UnitTests::VerifyPoints(const CMILMatrix4x4& transform, const gsl::span<const XPOINTF>& input, const gsl::span<const XPOINTF>& expected, const bool expectedWasTransformed)
{
    std::vector<XPOINTF> output;
    output.reserve(input.size());
    for (XPOINTF point : input)
    {
        output.push_back(point);
    }
    bool wasTransformed = transform.TransformWorldToLocalWithInverse(input, output);

    VERIFY_ARE_EQUAL(expectedWasTransformed, wasTransformed);
    if (expectedWasTransformed)
    {
        VERIFY_ARE_EQUAL(expected.size(), output.size());
        for (int index = 0; index < expected.size(); index++)
        {
            VerifyPoint(expected[index], output[index], index);
        }
    }
}

void MILMatrix4x4UnitTests::ValidateTransformWorldToLocal_Identity()
{
    // Do nothing. The reverse transform method asserts that the matrix isn't identity.
}

void MILMatrix4x4UnitTests::ValidateTransformWorldToLocal_TranslateZ()
{
    CMILMatrix4x4 matrix(true);
    const XPOINTF input[] = { {1234, 4321}, {0, 0}, {-10, 30} };
    const XPOINTF expected[] = { {1234, 4321}, {0, 0}, {-10, 30} };

    matrix.SetToTranslation(0, 0, 200);
    VerifyPoints(matrix, input, expected);

    matrix.SetToTranslation(0, 0, -200);
    VerifyPoints(matrix, input, expected);
}

void MILMatrix4x4UnitTests::ValidateTransformWorldToLocal_TranslateXZ()
{
    CMILMatrix4x4 matrix(true);
    const XPOINTF input[] = { {1234, 4321}, {0, 0}, {-10, 30} };
    const XPOINTF expected[] = { {1034, 4321}, {-200, 0}, {-210, 30} };

    matrix.SetToTranslation(200, 0, 300);
    VerifyPoints(matrix, input, expected);

    matrix.SetToTranslation(200, 0, -300);
    VerifyPoints(matrix, input, expected);
}

void MILMatrix4x4UnitTests::ValidateTransformWorldToLocal_RotationY()
{
    CMILMatrix4x4 matrix(true);
    const XPOINTF input[] = { {1234, 4321}, {0, 0}, {-10, 30} };
    // Rotate Y by 60 degrees = X coordinate is halved, so 100 in local coords becomes 50 in screen space.
    // To go from screen space to local space, double the X.
    const XPOINTF expected[] = { {2468, 4321}, {0, 0}, {-20, 30} };

    matrix.SetToRotationY(60);
    VerifyPoints(matrix, input, expected);

    matrix.SetToRotationY(-60);
    VerifyPoints(matrix, input, expected);

    // A rotation by 90 degrees will make all points miss the plane. The points should fail to transform back.
    matrix.SetToRotationY(90);
    VerifyPoints(matrix, input, expected, false);
}

void MILMatrix4x4UnitTests::ValidateTransformWorldToLocal_RotationX()
{
    CMILMatrix4x4 matrix(true);
    const XPOINTF input[] = { {1234, 4321}, {0, 0}, {-10, 30} };
    // Rotate X by 30 degrees = Y coordinate is scaled by root(3)/2, so 100 in local coords becomes 86.6 in screen space.
    // To go from screen space to local space, scale Y by 2/root(3).
    const XPOINTF expected[] = { {1234, static_cast<float>(4321 * 2 / sqrt(3))}, {0, 0}, {-10, static_cast<float>(30 * 2 / sqrt(3))} };

    matrix.SetToRotationX(30);
    VerifyPoints(matrix, input, expected);

    matrix.SetToRotationX(-30);
    VerifyPoints(matrix, input, expected);

    // A rotation by 90 degrees will make all points miss the plane. The points should fail to transform back.
    matrix.SetToRotationX(90);
    VerifyPoints(matrix, input, expected, false);
}

void MILMatrix4x4UnitTests::ValidateTransformWorldToLocal_ScaleXYZ()
{
    CMILMatrix4x4 matrix(true);
    const XPOINTF input[] = { {1234, 4321}, {0, 0}, {-10, 30} };
    const XPOINTF expected[] = { {617, 8642}, {0, 0}, {-5, 60} };

    matrix.SetToScale(2, 0.5, 1);
    VerifyPoints(matrix, input, expected);

    matrix.SetToScale(2, 0.5, -2);
    VerifyPoints(matrix, input, expected);

    // A scale to 0 is not invertible. The points should fail to transform back.
    matrix.SetToScale(2, 0.5, 0);
    VerifyPoints(matrix, input, expected, false);
}

void MILMatrix4x4UnitTests::ValidateTransformWorldToLocal_PerspectiveTranslateZ()
{
    // Depth = 100 => _34 is -1/100. So the rightmost *column* is (0  0  -1/100  1)^T
    // A point at Z =   50 will end up with W =   (50)(-1/100) + 1 = 0.5, so its X and Y will be doubled.
    // A point at Z =    0 will end up with W =    (0)(-1/100) + 1 = 1, so its X and Y will be untouched.
    // A point at Z = -100 will end up with W = (-100)(-1/100) + 1 = 2, so its X and Y will be halved.
    // A point at Z = -300 will end up with W = (-300)(-1/100) + 1 = 4, so its X and Y will be quartered.
    // A point at Z =  200 will end up with W =  (200)(-1/100) + 1 = -1, so its X and Y will be mirrored.
    // A point at Z =  100 will end up with W =  (100)(-1/100) + 1 = 0, so its X and Y will be scaled up by infinity.
    CMILMatrix4x4 perspective(true);
    perspective.SetToPerspectiveTransform3D(0, 0, 100);

    // Note: This is the transform from local space up to the world. The translate happens first, followed by the
    // perspective, so the perspective should be appended.
    CMILMatrix4x4 perspectiveTranslate(true);

    const XPOINTF input[] = { {1234, 4321}, {0, 0}, {-10, 30} };

    {
        LOG_OUTPUT(L"Z = 50"); //X and Y will be doubled on screen. They'll be halved going back to local space.
        perspectiveTranslate.SetToTranslation(0, 0, 50);
        perspectiveTranslate.Append(perspective);
        const XPOINTF expected[] = { {617, 2160.5}, {0, 0}, {-5, 15} };
        VerifyPoints(perspectiveTranslate, input, expected);
    }

    {
        LOG_OUTPUT(L"Z = 0"); // X and Y will be untouched on screen.
        perspectiveTranslate.SetToTranslation(0, 0, 0);
        perspectiveTranslate.Append(perspective);
        const XPOINTF expected[] = { {1234, 4321}, {0, 0}, {-10, 30} };
        VerifyPoints(perspectiveTranslate, input, expected);
    }

    {
        LOG_OUTPUT(L"Z = -100"); // X and Y will be halved on screen. They'll be doubled going back to local space.
        perspectiveTranslate.SetToTranslation(0, 0, -100);
        perspectiveTranslate.Append(perspective);
        const XPOINTF expected[] = { {2468, 8642}, {0, 0}, {-20, 60} };
        VerifyPoints(perspectiveTranslate, input, expected);
    }

    {
        LOG_OUTPUT(L"Z = -300"); // X and Y will be quartered on screen. They'll be quadrupled going back to local space.
        perspectiveTranslate.SetToTranslation(0, 0, -300);
        perspectiveTranslate.Append(perspective);
        const XPOINTF expected[] = { {4936, 17284}, {0, 0}, {-40, 120} };
        VerifyPoints(perspectiveTranslate, input, expected);
    }

    {
        // This is an edge case of content rendering behind the camera. The math still works, but we might want to special
        // case hit testing to make this miss.
        LOG_OUTPUT(L"Z = 200"); // X and Y will be mirrored on screen. They'll be mirrored going back into local space.
        perspectiveTranslate.SetToTranslation(0, 0, 200);
        perspectiveTranslate.Append(perspective);
        const XPOINTF expected[] = { {-1234, -4321}, {0, 0}, {10, -30} };
        VerifyPoints(perspectiveTranslate, input, expected);
    }

    {
        // This is an edge case of content rendering at the camera. The math still works, but we might want to special
        // case hit testing to make this miss.
        LOG_OUTPUT(L"Z = 100"); // X and Y will be scaled up to infinity. They'll be scaled down to 0 going back to local space.
        perspectiveTranslate.SetToTranslation(0, 0, 100);
        perspectiveTranslate.Append(perspective);
        const XPOINTF expected[] = { {0, 0}, {0, 0}, {0, 0} };
        VerifyPoints(perspectiveTranslate, input, expected);
    }
}

void MILMatrix4x4UnitTests::ValidateTransformWorldToLocal_PerspectiveRotationY()
{
    // Depth = 40 => _34 is -1/40. So the rightmost *column* is (0  0  -1/40  1)^T
    // A point at Z =   20 will end up with W =   (20)(-1/40) + 1 = 0.5, so its X and Y will be doubled.
    // A point at Z =    0 will end up with W =    (0)(-1/40) + 1 = 1, so its X and Y will be untouched.
    // A point at Z =  -40 will end up with W =  (-40)(-1/40) + 1 = 2, so its X and Y will be halved.
    // A point at Z = -120 will end up with W = (-120)(-1/40) + 1 = 4, so its X and Y will be quartered.
    // A point at Z =   80 will end up with W =   (80)(-1/40) + 1 = -1, so its X and Y will be mirrored.
    // A point at Z =   40 will end up with W =   (40)(-1/40) + 1 = 0, so its X and Y will be scaled up by infinity.
    CMILMatrix4x4 perspective(true);
    perspective.SetToPerspectiveTransform3D(0, 0, 40);

    // Note: This is the transform from local space up to the world. The translate happens first, followed by the
    // perspective, so the perspective should be appended.
    CMILMatrix4x4 perspectiveRotation(true);
    perspectiveRotation.SetToRotationY(30);
    perspectiveRotation.Append(perspective);

    //
    // A 30 degree rotation gives:
    //  - World space X = Local space X / 2 * sqrt(3)
    //  - World space Y = Local space Y
    //  - World space Z = Local space X / 2
    //
    // The depth is set to 40, so we pick some interesting world space Z values to test.
    //
    //    - Start with an arbitrary world space Z value.
    //
    //    - The world space Z value determines the world space W value.
    //      The rightmost column of the perspective matrix is (0  0  -1/40  1)^T, so the world space W value is
    //          W_world = -Z_world/40 + 1
    //
    //    - The 30 degree Y rotation means the world space Z value is determined from the local space X value.
    //      Specifically,
    //          Z_world = X_local / 2  <==>  X_local = 2*Z_world
    //
    //    - Given a local space X value and a 30 degree Y rotation, we can also determine the world space X value.
    //          X_world = X_local * r3 / 2, where r3 is the square root of 3.
    //
    //    - The local space Y value doesn't affect the transform, so we pick some arbitrary numbers. It is also
    //      unaffected by the Y rotation.
    //          Y_world = Y_local
    //
    //    - Finally, with the world space X, Y, Z, and W values, we can find the 2D point that gets rendered on screen.
    //      This is the point that is the input to hit testing.
    //          ( X_world/W_world, Y_world/W_world )
    //
    //    World  |  World  |  Local  |  World  | Local Space |       World Space         |    2D Point
    //   Space Z | Space W | Space X | Space X |    ( x, y ) |      ( x,  y,    z,  w )  |    ( x/w, y/w )
    //  ---------+---------+---------+---------+-------------+---------------------------+------------------
    //      20   |   0.5   |     40  |   20*r3 |  (  40, 10) | (  20*r3, 10,   20, 0.5 ) | (  40*r3,  20 )
    //       0   |     1   |      0  |    0    |  (   0, 20) | (   0   , 20,    0,  1  ) | (   0   ,  20 )
    //     -40   |     2   |    -80  |  -40*r3 |  ( -80, 30) | ( -40*r3, 30,  -40,  2  ) | ( -20*r3,  15 )
    //    -120   |     4   |   -240  | -120*r3 |  (-240, 40) | (-120*r3, 40, -120,  4  ) | ( -30*r3,  10 )
    //      80   |    -1   |    160  |   80*r3 |  ( 160, 50) | (  80*r3, 50,   80, -1  ) | ( -80*r3, -50 )
    //      40   |     0   |     80  |   40*r3 |  (  80, 60) | (  40*r3, 60,   40,  0  ) | (    inf, inf )
    //
    // We can't do infinity, but we can use the max value for a float. We're dealing with math on the numerical limits,
    // so we can't really expect the results to be precise.
    //
    // There's another infinity case, which is local space infinity. Here, the numerical limit for the 2D point gives
    // us a horizon. To find that vanishing point, we look at the relationship between the local space X value
    // and the 2D point (world space X / world space W). We have:
    //
    //      W_world = -Z_world/40 + 1
    //              = -X_local/80 + 1       (since Z_world = X_local / 2 from the 30 degree Y rotation)
    //              = (80 - X_local)/80
    //      1/W_world = 80/(80 - X_local)
    //
    //      X_world = X_local * r3 / 2 = X_local * r3 * 0.5
    //
    //      X_world / W_world = X_world * 1/W_world
    //        = X_local * r3 * 0.5 * 80/(80 - X_local)
    //        = X_local * r3 * 40 / (80 - X_local)
    //        = -40r3 * X_local/(X_local - 80)
    //
    // As X_local approaches positive infinity, X_local/(X_local - 80) approaches 1 from above (>1).
    // The whole expression -40r3 * X_local/(X_local - 80) approaches -40r3 from below (more negative).
    //
    // As X_local approaches negative infinity, X_local/(X_local - 80) approaches 1 from below (<1).
    // The whole expression -40r3 * X_local/(X_local - 80) approaches -40r3 from above (less negative).
    //
    // In either case, the whole expression approaches -40r3, which is the horizon.
    //
    // Intuitively this makes sense. We're doing a 30 degree rotation into the screen, where the X < 0 half goes into
    // the screen (producing larger W values that pull the 2D point closer to the origin). So the horizon that we see
    // should be from local X approaching -infinity from the right, and the corresponding world space point approaches
    // -40r3 from the right. Once we cross the horizon we wrap around to X = +infinity in local space, which lies behind
    // the camera. The fact that we wrapped around is a result of the math. In practice the stuff behind the camera will
    // be clipped out by the near plane.
    //

    const float root3 = static_cast<float>(sqrt(3));
    const XPOINTF input[] =
    {
        {  40 * root3,  20 },
        {           0,  20 },
        { -20 * root3,  15 },
        { -30 * root3,  10 },
        { -80 * root3, -50 },
    };

    const XPOINTF expected[] =
    {
        {   40, 10 },
        {    0, 20 },
        {  -80, 30 },
        { -240, 40 },
        {  160, 50 },
    };

    VerifyPoints(perspectiveRotation, input, expected);

    {
        LOG_OUTPUT(L"Test a (near) infinity world space point.");
        const XPOINTF maxFloat = { std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
        XPOINTF maxFloatOutput;
        bool wasTransformed = perspectiveRotation.TransformWorldToLocalWithInverse(maxFloat, maxFloatOutput);
        VERIFY_IS_TRUE(wasTransformed);

        // Local space X is reliable. As the local space X approaches 80, the world space W approaches 0 and the world space X
        // approaches infinity. We used max float instead of infinity, which should give us back a local space X near 80.
        VERIFY_IS_TRUE(79.5 < maxFloatOutput.x && maxFloatOutput.x < 80.5);

        // Local space Y is unreliable, so don't check it. It did not factor into the W parameter at all. We're doing a
        // multiplication between max float and the tiny W produced by the world space X coordinate.
    }

    {
        LOG_OUTPUT(L"Test near the horizon.");
        const XPOINTF horizon[] =
        {
            { -root3 * 40 + 0.0001f, 200 },  // Should give big negative number in local space
            { -root3 * 40 - 0.0001f, 100 },  // Should give big positive number in local space
        };
        XPOINTF horizonOutput[2];
        bool wasTransformed = perspectiveRotation.TransformWorldToLocalWithInverse(horizon, horizonOutput);

        // There should be 2 output points. The one from negative infinity can be seen. The other comes out of the perspective math.
        VERIFY_IS_TRUE(wasTransformed);

        // Test for the two local space points near infinity.
        VERIFY_IS_LESS_THAN(horizonOutput[0].x, -999999);
        VERIFY_IS_GREATER_THAN(horizonOutput[1].x, 999999);

        // Again, the local space Y is unreliable. As local X approaches either infinity, W approaches 0, which will pull the world
        // space Y towards 0. Whatever world space Y we give will be multiplied by this tiny W, whose magnitude depends on how close
        // world space X is to -40r3.
    }
}

} } } } } }
