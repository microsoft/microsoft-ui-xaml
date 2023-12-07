// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ExpressionHelper.h"

// Naming Convention: Property Set names: CAPS, Property names: Pascal

const wchar_t* const ExpressionHelper::sc_paramName_PropertySet             = L"PS";
const wchar_t* const ExpressionHelper::sc_paramName_Visual                  = L"V";
const wchar_t* const ExpressionHelper::sc_paramName_CenterX                 = L"CenterX";
const wchar_t* const ExpressionHelper::sc_paramName_CenterY                 = L"CenterY";
const wchar_t* const ExpressionHelper::sc_paramName_CenterZ                 = L"CenterZ";
const wchar_t* const ExpressionHelper::sc_paramName_ScaleX                  = L"ScaleX";
const wchar_t* const ExpressionHelper::sc_paramName_ScaleY                  = L"ScaleY";
const wchar_t* const ExpressionHelper::sc_paramName_ScaleZ                  = L"ScaleZ";
const wchar_t* const ExpressionHelper::sc_paramName_TranslateX              = L"X";
const wchar_t* const ExpressionHelper::sc_paramName_TranslateY              = L"Y";
const wchar_t* const ExpressionHelper::sc_paramName_TranslateZ              = L"Z";
const wchar_t* const ExpressionHelper::sc_paramName_RotateAngle             = L"RotateAngle";
const wchar_t* const ExpressionHelper::sc_paramName_SkewAngleX              = L"SkewAngleX";
const wchar_t* const ExpressionHelper::sc_paramName_SkewAngleY              = L"SkewAngleY";
const wchar_t* const ExpressionHelper::sc_paramName_M11                     = L"M11";
const wchar_t* const ExpressionHelper::sc_paramName_M12                     = L"M12";
const wchar_t* const ExpressionHelper::sc_paramName_M21                     = L"M21";
const wchar_t* const ExpressionHelper::sc_paramName_M22                     = L"M22";
const wchar_t* const ExpressionHelper::sc_paramName_TX                      = L"Tx";
const wchar_t* const ExpressionHelper::sc_paramName_TY                      = L"Ty";
const wchar_t* const ExpressionHelper::sc_paramName_Scale                   = L"Scale";
const wchar_t* const ExpressionHelper::sc_paramName_Skew                    = L"Skew";
const wchar_t* const ExpressionHelper::sc_paramName_Rotate                  = L"Rotate";
const wchar_t* const ExpressionHelper::sc_paramName_RotateX                 = L"RotateX";
const wchar_t* const ExpressionHelper::sc_paramName_RotateY                 = L"RotateY";
const wchar_t* const ExpressionHelper::sc_paramName_RotateZ                 = L"RotateZ";
const wchar_t* const ExpressionHelper::sc_paramName_Translate               = L"Translate";
const wchar_t* const ExpressionHelper::sc_paramName_OffsetX                 = L"X";   // Set on the visual's PropertySet. Never used in conjunction with TranslateX/Y.
const wchar_t* const ExpressionHelper::sc_paramName_OffsetY                 = L"Y";
const wchar_t* const ExpressionHelper::sc_paramName_Opacity                 = L"Opacity";
const wchar_t* const ExpressionHelper::sc_paramName_PrependOpacity          = L"POpacity";
const wchar_t* const ExpressionHelper::sc_paramName_TransitionTargetOpacity = L"TTOpacity";

// 2D and 3D Translate Transforms
const wchar_t* const ExpressionHelper::sc_Expression_Translate    = L"Matrix3x2.CreateFromTranslation(vector2(PS.X, PS.Y))";
const wchar_t* const ExpressionHelper::sc_Expression_Translate3D =  L"Matrix4x4.CreateFromTranslation(vector3(PS.X, PS.Y, PS.Z))";

// 2D and 3D Scale transforms optionally centered at (CenterX, CenterY, [CenterZ])
const wchar_t* const ExpressionHelper::sc_Expression_Scale                = L"Matrix3x2.CreateFromScale(vector2(PS.ScaleX, PS.ScaleY))";
const wchar_t* const ExpressionHelper::sc_Expression_Scale_CenterPoint    = L"Matrix3x2.CreateFromTranslation(-vector2(PS.CenterX, PS.CenterY)) * Matrix3x2.CreateFromScale(vector2(PS.ScaleX, PS.ScaleY)) * Matrix3x2.CreateFromTranslation(vector2(PS.CenterX, PS.CenterY))";
const wchar_t* const ExpressionHelper::sc_Expression_Scale3D              = L"Matrix4x4.CreateFromScale(vector3(PS.ScaleX, PS.ScaleY, PS.ScaleZ))";
const wchar_t* const ExpressionHelper::sc_Expression_Scale_CenterPoint3D  = L"Matrix4x4.CreateFromTranslation(-vector3(PS.CenterX, PS.CenterY, PS.CenterZ)) * Matrix4x4.CreateFromScale(vector3(PS.ScaleX, PS.ScaleY, PS.ScaleZ)) * Matrix4x4.CreateFromTranslation(vector3(PS.CenterX, PS.CenterY, PS.CenterZ))";

// 2D Clockwise Rotation around Z axis, optionally centered at (CenterX, CenterY), with angle given in degrees (converted to radians for DComp API)
// TODO_WinRT: Replace unoptimized Matrix3x2 rotation below using matrix constructor with Matrix3x2.CreateFromAngle() when it is available.
//             We needed to do this because currently, there is no built-in Matrix3x2 rotation. Comp is tracking this with TFS 6708016 (1604).
const wchar_t* const ExpressionHelper::sc_Expression_Rotate                = L"Matrix3x2(cos(PS.RotateAngle * pi / 180.0f), sin(PS.RotateAngle * pi / 180.0f), -sin(PS.RotateAngle * pi / 180.0f), cos(PS.RotateAngle * pi / 180.0f), 0, 0)";
const wchar_t* const ExpressionHelper::sc_Expression_Rotate_CenterPoint    = L"Matrix3x2.CreateFromTranslation(-vector2(PS.CenterX, PS.CenterY)) * Matrix3x2(cos(PS.RotateAngle * pi / 180.0f), sin(PS.RotateAngle * pi / 180.0f), -sin(PS.RotateAngle * pi / 180.0f), cos(PS.RotateAngle * pi / 180.0f), 0, 0) * Matrix3x2.CreateFromTranslation(vector2(PS.CenterX, PS.CenterY))";

// 3D Clockwise Rotations around X/Y/Z axes, optionally centered at (CenterX, CenterY, CenterZ), with angle given in degrees (converted to radians for DComp API)
const wchar_t* const ExpressionHelper::sc_Expression_Rotate3D_X             = L"Matrix4x4.CreateFromAxisAngle(vector3(-1, 0, 0), PS.RotateAngle * pi / 180.0f)";
const wchar_t* const ExpressionHelper::sc_Expression_Rotate3D_X_CenterPoint = L"Matrix4x4.CreateFromTranslation(-vector3(PS.CenterX, PS.CenterY, PS.CenterZ))* Matrix4x4.CreateFromAxisAngle(vector3(-1, 0, 0), PS.RotateAngle * pi / 180.0f) * Matrix4x4.CreateFromTranslation(vector3(PS.CenterX, PS.CenterY, PS.CenterZ))";
const wchar_t* const ExpressionHelper::sc_Expression_Rotate3D_Y             = L"Matrix4x4.CreateFromAxisAngle(vector3(0, -1, 0), PS.RotateAngle * pi / 180.0f)";
const wchar_t* const ExpressionHelper::sc_Expression_Rotate3D_Y_CenterPoint = L"Matrix4x4.CreateFromTranslation(-vector3(PS.CenterX, PS.CenterY, PS.CenterZ))* Matrix4x4.CreateFromAxisAngle(vector3(0, -1, 0), PS.RotateAngle * pi / 180.0f) * Matrix4x4.CreateFromTranslation(vector3(PS.CenterX, PS.CenterY, PS.CenterZ))";
const wchar_t* const ExpressionHelper::sc_Expression_Rotate3D_Z             = L"Matrix4x4.CreateFromAxisAngle(vector3(0, 0, -1), PS.RotateAngle * pi / 180.0f)";
const wchar_t* const ExpressionHelper::sc_Expression_Rotate3D_Z_CenterPoint = L"Matrix4x4.CreateFromTranslation(-vector3(PS.CenterX, PS.CenterY, PS.CenterZ))* Matrix4x4.CreateFromAxisAngle(vector3(0, 0, -1), PS.RotateAngle * pi / 180.0f) * Matrix4x4.CreateFromTranslation(vector3(PS.CenterX, PS.CenterY, PS.CenterZ))";

// 2D Skew transform optionally centered at (CenterX, CenterY), with angle given in degrees (converted to radians for DComp API)
const wchar_t* const ExpressionHelper::sc_Expression_Skew                = L"Matrix3x2.CreateSkew(PS.SkewAngleX * pi / 180.0f, PS.SkewAngleY * pi / 180.0f, Vector2(0, 0))";
const wchar_t* const ExpressionHelper::sc_Expression_Skew_CenterPoint    = L"Matrix3x2.CreateSkew(PS.SkewAngleX * pi / 180.0f, PS.SkewAngleY * pi / 180.0f, Vector2(PS.CenterX, PS.CenterY))";

// 2D arbitrary matrix transform
// Map CMILMatrix (3x2) to 4x4 matrix as follows:
//
//     | M11 M12 [0] |          | M11 M12  0   0 |
//     | M21 M22 [0] |    ->    | M21 M22  0   0 |
//     | Tx  Ty  [1] |          |  0   0   1   0 |
//                              | Tx  Ty   0   1 |
const wchar_t* const ExpressionHelper::sc_Expression_MatrixTransform    = L"Matrix3x2(PS.M11, PS.M12, PS.M21, PS.M22, PS.Tx, PS.Ty)";
const wchar_t* const ExpressionHelper::sc_Expression_MatrixTransform4x4 = L"Matrix4x4(PS.M11, PS.M12, 0.0, 0.0, PS.M21, PS.M22, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, PS.Tx, PS.Ty, 0.0, 1.0)";

// 3D Matrix does not support animation by component - just passes through a matrix parameter
const wchar_t* const ExpressionHelper::sc_Expression_MatrixTransform3D  = L"PS.Matrix";
const wchar_t* const ExpressionHelper::sc_propertyName_MatrixTransform3D_Matrix = L"Matrix";


// 2D CompositeTransform optionally centered at (CenterX, CenterY) and consisting of combination of Scale, Skew, Rotate, and Translate
// Order of operations
//      Shift to center point
//      Scale
//      Skew
//      Rotate
//      Undo shift
//      Translate
const wchar_t* const ExpressionHelper::sc_Expression_CompositeTransform                = L"PS.Scale * PS.Skew * PS.Rotate * PS.Translate";
const wchar_t* const ExpressionHelper::sc_Expression_CompositeTransform_CenterPoint    = L"Matrix3x2.CreateFromTranslation(-vector2(PS.CenterX, PS.CenterY)) * PS.Scale * PS.Skew * PS.Rotate * Matrix3x2.CreateFromTranslation(vector2(PS.CenterX, PS.CenterY)) * PS.Translate";
// For a simple, axis-aligned CompositeTransform with no center point
const wchar_t* const ExpressionHelper::sc_Expression_CompositeTransform_ScaleTranslate = L"Matrix3x2(PS.ScaleX, 0, 0, PS.ScaleY, PS.X, PS.Y)";
const wchar_t* const ExpressionHelper::sc_Expression_CompositeTransform_ScaleTranslate_4x4 = L"Matrix4x4(PS.ScaleX, 0, 0, 0,   0, PS.ScaleY, 0, 0,   0, 0, 1, 0,   PS.X, PS.Y, 0, 1)";

// 3D CompositeTransform optionally centered at (CenterX, CenterY, CenterZ) and consisting of combination of Scale, Rotate, and Translate
// Order of operations
//      Shift to center point
//      Scale
//      Rotate
//      Undo shift
//      Translate
const wchar_t* const ExpressionHelper::sc_Expression_CompositeTransform3D = L"PS.Scale * PS.RotateX * PS.RotateY * PS.RotateZ * PS.Translate";
const wchar_t* const ExpressionHelper::sc_Expression_CompositeTransform_CenterPoint3D = L"Matrix4x4.CreateFromTranslation(-vector3(PS.CenterX, PS.CenterY, PS.CenterZ)) * PS.Scale * PS.RotateX * PS.RotateY * PS.RotateZ * Matrix4x4.CreateFromTranslation(vector3(PS.CenterX, PS.CenterY, PS.CenterZ)) * PS.Translate";

// Plane Projection
// There are 4 conceptual components to Plane projection (captured in the high-level expression "PlaneProjection")
// 1. LocalXYZ : local 3D translation (applied before the 3D Rotation)
// 2. Rotation : 3D Rotation around X, Y or Z axes. It also parametrized by an animatable center point (CenterX, CenterY, CenterZ), where CenterX, CenterY
//               are which is given in [0,1] relative coordinates, while CenterZ is given in absolute coordinates.
//               In order to capture this in an expression, we need to:
//                   (1) translate to/from the center point around the rotations
//                   (2) and scale to/from absolute coordinates (using element's width and height) for each of the translations
//               These are captured by "RotateCenter" / "UndoRotateCenter" expressions.
//               Note that RotateCenter/UndoRotateCenter can lead to division by 0, so we have special variants for the case where width, height, or both are 0.
//               In order to do the centering operation, we need to translate by the negative of the center  offset. If that offset is animated, we need the negative
//               of the entire animation, which we get by multiplying -1 in both scales.
// 3. GlobalXYZ : global 3D translation (applied after the 3D Rotation)
// 4. Perspective : projection into 2D space
const wchar_t* const ExpressionHelper::sc_Expression_PlaneProjection                                     = L"PS.LocalXYZ * PS.Rotation * PS.GlobalXYZ * PS.Perspective";
const wchar_t* const ExpressionHelper::sc_Expression_PlaneProjection_Rotation                            = L"PS.RotateCenter * PS.RotateX * PS.RotateY * PS.RotateZ * PS.UndoRotateCenter";
const wchar_t* const ExpressionHelper::sc_Expression_PlaneProjection_RotateCenter                        = L"Matrix4x4.CreateFromScale(vector3(-1 / PS.Width, -1 / PS.Height, 1)) * Matrix4x4.CreateFromTranslation(vector3(PS.CenterX, PS.CenterY, 0)) * Matrix4x4.CreateFromScale(vector3(-PS.Width, -PS.Height, 1))";
const wchar_t* const ExpressionHelper::sc_Expression_PlaneProjection_UndoRotateCenter                    = L"Matrix4x4.CreateFromScale(vector3(1 / PS.Width, 1 / PS.Height, 1)) * Matrix4x4.CreateFromTranslation(vector3(PS.CenterX, PS.CenterY, 0)) * Matrix4x4.CreateFromScale(vector3(PS.Width, PS.Height, 1))";
const wchar_t* const ExpressionHelper::sc_Expression_PlaneProjection_Perspective                         = L"Matrix4x4.CreateFromTranslation(vector3(PS.Width / -2, PS.Height / -2, 0)) * PS.PerspectiveMatrix * Matrix4x4.CreateFromTranslation(vector3(PS.Width / 2, PS.Height / 2, 0))";
const wchar_t* const ExpressionHelper::sc_propertyName_PlaneProjection_LocalOffset                       = L"LocalXYZ";
const wchar_t* const ExpressionHelper::sc_propertyName_PlaneProjection_Rotation                          = L"Rotation";
const wchar_t* const ExpressionHelper::sc_propertyName_PlaneProjection_RotateCenter                      = L"RotateCenter";
const wchar_t* const ExpressionHelper::sc_propertyName_PlaneProjection_UndoRotateCenter                  = L"UndoRotateCenter";
const wchar_t* const ExpressionHelper::sc_propertyName_PlaneProjection_GlobalOffset                      = L"GlobalXYZ";
const wchar_t* const ExpressionHelper::sc_propertyName_PlaneProjection_Perspective                       = L"Perspective";
const wchar_t* const ExpressionHelper::sc_propertyName_PlaneProjection_ElementWidth                      = L"Width";
const wchar_t* const ExpressionHelper::sc_propertyName_PlaneProjection_ElementHeight                     = L"Height";
const wchar_t* const ExpressionHelper::sc_propertyName_PlaneProjection_PerspectiveMatrix                 = L"PerspectiveMatrix";

// 2D TransformGroup - an arbitrary collection of 2D transforms
// Here we define constants for small TransformGroup expressions, rest generated dynamically
const wchar_t* const ExpressionHelper::sc_Expression_TransformGroup1     = L"PS.Expr0";
const wchar_t* const ExpressionHelper::sc_Expression_TransformGroup2     = L"PS.Expr0 * PS.Expr1";
const wchar_t* const ExpressionHelper::sc_Expression_TransformGroup3     = L"PS.Expr0 * PS.Expr1 * PS.Expr2";
const wchar_t* const ExpressionHelper::sc_TransformGroupPropertyNameRoot = L"Expr";

// RenderTransformOrigin-related
const wchar_t* const ExpressionHelper::sc_RtoFull3x2        = L"Matrix3x2.CreateFromTranslation(-vector2(PS.X, PS.Y)) * PS.Expression * Matrix3x2.CreateFromTranslation(vector2(PS.X, PS.Y))";
const wchar_t* const ExpressionHelper::sc_RtoExpression     = L"Expression";

// Redirection "Post" Transform - applies a transform above the Offset
const wchar_t* const ExpressionHelper::sc_Expression_PostRedirectionTransform = L"Matrix4x4.CreateFromTranslation(V.Offset) * PS.Redir * Matrix4x4.CreateFromTranslation(-Vector3(V.Offset.X, V.Offset.Y, 0))";

// DManip-related
const wchar_t* const ExpressionHelper::sc_Expression_DMTransform = L"PS.Matrix";    // Copies the DManip transform from DManip PropertySet into LOCAL PropertySet

// LocalTransform PropertySet and properties
// >> Note this is always a 4x4 matrix since Visual.TransformMatrix is a Matrix4x4.
// >> It's possible for an app to use the same transform as a RenderTransform and a Clip.Transform, since transforms are shareable resources.
//    Since clips require a 3x2 matrix, the RenderTransform must use 3x2 matrices as well. Convert it to 4x4 matrix here in the final local transform expression.
const wchar_t* const ExpressionHelper::sc_Expression_LocalTransform_Projection         = L"LOCAL.Projection";
const wchar_t* const ExpressionHelper::sc_Expression_LocalTransform_Transform3D        = L"LOCAL.Transform3D";
const wchar_t* const ExpressionHelper::sc_Expression_LocalTransform_Render             = L"Matrix4x4(LOCAL.Render)";
const wchar_t* const ExpressionHelper::sc_Expression_LocalTransform_TTRender           = L"Matrix4x4(LOCAL.TransitionTargetRender)";
const wchar_t* const ExpressionHelper::sc_Expression_LocalTransform_FlowDirection      = L"LOCAL.FlowDirection";
const wchar_t* const ExpressionHelper::sc_Expression_LocalTransform_DManip             = L"Matrix4x4.CreateFromTranslation(V.Offset) * LOCAL.DManip * Matrix4x4.CreateFromTranslation(-V.Offset)";
const wchar_t* const ExpressionHelper::sc_Expression_LocalTransform_Redir              = L"LOCAL.PostRedir";
const wchar_t* const ExpressionHelper::sc_LocalTransformPropertySetName                = L"LOCAL";                     // PropertySet containing LocalTransformMatrix components
const wchar_t* const ExpressionHelper::sc_propertyName_Projection                      = L"Projection";                // Property holding legacy Projection matrix (PlaneProjection or Matrix3DProjection)
const wchar_t* const ExpressionHelper::sc_propertyName_Transform3D                     = L"Transform3D";               // Property holding Transform3D matrix (CompositeTransform3D or PerspectiveTransform3D)
const wchar_t* const ExpressionHelper::sc_propertyName_RenderTransform                 = L"Render";                    // Property holding RenderTransform matrix
const wchar_t* const ExpressionHelper::sc_propertyName_LocalDManipTransform            = L"DManip";                    // Property holding DManipTransform matrix
const wchar_t* const ExpressionHelper::sc_propertyName_TransitionTargetRenderTransform = L"TransitionTargetRender";    // Property holding TransitionTargetRenderTransform matrix
const wchar_t* const ExpressionHelper::sc_propertyName_FlowDirectionTransform          = L"FlowDirection";             // Property holding FlowDirectionTransform matrix
const wchar_t* const ExpressionHelper::sc_propertyName_RedirectionTransform            = L"Redir";                     // Property holding RedirectionTransform matrix
const wchar_t* const ExpressionHelper::sc_propertyName_PostRedirectionTransform        = L"PostRedir";                 // Property holding Post-RedirectionTransform matrix (includes Offset adjustment)

// The property on WUComp::IVisual where we set the local transform
const wchar_t* const ExpressionHelper::sc_propertyName_TransformMatrix = L"TransformMatrix";

const wchar_t* const ExpressionHelper::sc_Expression_Offset  = L"Vector3(PS.X, PS.Y, 0)";
const wchar_t* const ExpressionHelper::sc_OffsetPropertyName = L"Offset";     // The Visual.Offset property. Hard-coded by DComp.

// Other Visual property names
const wchar_t* const ExpressionHelper::sc_RotationPropertyName = L"RotationAngleInDegrees";
const wchar_t* const ExpressionHelper::sc_ScalePropertyName = L"Scale";
const wchar_t* const ExpressionHelper::sc_TransformMatrixPropertyName = sc_propertyName_TransformMatrix;
const wchar_t* const ExpressionHelper::sc_CenterPointPropertyName = L"CenterPoint";
const wchar_t* const ExpressionHelper::sc_RotationAxisPropertyName = L"RotationAxis";

const wchar_t* const ExpressionHelper::sc_Expression_Opacity = L"PS.Opacity";
const wchar_t* const ExpressionHelper::sc_Expression_TransitionTargetOpacity = L"PS.POpacity * PS.TTOpacity";
const wchar_t* const ExpressionHelper::sc_OpacityPropertyName                = L"Opacity";   // The Visual.Opacity property. Hard-coded by DComp.

// DManip provides 4x4 matrix, ICompositionClip2.TransformMatrix expects 3x2 matrix - convert it with below expression
const wchar_t* const ExpressionHelper::sc_ClipPrimaryTransformPropertySet = L"Primary";
const wchar_t* const ExpressionHelper::sc_Clip_propertyName_Matrix        = L"Matrix";
const wchar_t* const ExpressionHelper::sc_Expression_DManipClipTransform  = L"Matrix3x2(Primary.Matrix._11, Primary.Matrix._12, Primary.Matrix._21, Primary.Matrix._22, Primary.Matrix._41, Primary.Matrix._42)";

const wchar_t* const ExpressionHelper::sc_propertyName_Color = L"Color";
