// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class ExpressionHelper
{
public:

    // TODO: WinComp: Eventually we can should add getters here that can dynamically build and return an HSTRING expression
    //                For now, most of the expessions are constants, allow them to be accessed directly.

    // Note that expressions that produce a matrix, we maintain two forms:
    // the default 3x2 form (eg sc_Expression_Translate) and the additional 4x4 form (sc_Expression_Translate4x4).
    // 3x2 is hte default since most of our transforms are actually 2D and these lighter expressions are more performant.
    // 4x4 forms are available for cases where the expression really needs to be 4x4 (eg due to being combined with a 3D transform,
    // or due to targeting a WUC API that expects Matrix4x4).

    // Constant Expressions and their components (Parameter names, Property/PropertySet names, etc)
    // All of these are "const wchar_t* const" so they go in the read-only part of the DLL (.rdata) instead of read/write (.data)
    static const wchar_t* const sc_paramName_PropertySet;
    static const wchar_t* const sc_paramName_Visual;
    static const wchar_t* const sc_paramName_CenterX;
    static const wchar_t* const sc_paramName_CenterY;
    static const wchar_t* const sc_paramName_CenterZ;
    static const wchar_t* const sc_paramName_ScaleX;
    static const wchar_t* const sc_paramName_ScaleY;
    static const wchar_t* const sc_paramName_ScaleZ;
    static const wchar_t* const sc_paramName_TranslateX;
    static const wchar_t* const sc_paramName_TranslateY;
    static const wchar_t* const sc_paramName_TranslateZ;
    static const wchar_t* const sc_paramName_RotateAngle;
    static const wchar_t* const sc_paramName_SkewAngleX;
    static const wchar_t* const sc_paramName_SkewAngleY;
    static const wchar_t* const sc_paramName_M11;
    static const wchar_t* const sc_paramName_M12;
    static const wchar_t* const sc_paramName_M21;
    static const wchar_t* const sc_paramName_M22;
    static const wchar_t* const sc_paramName_TX;
    static const wchar_t* const sc_paramName_TY;
    static const wchar_t* const sc_paramName_Scale;
    static const wchar_t* const sc_paramName_Skew;
    static const wchar_t* const sc_paramName_Rotate;
    static const wchar_t* const sc_paramName_RotateX;
    static const wchar_t* const sc_paramName_RotateY;
    static const wchar_t* const sc_paramName_RotateZ;
    static const wchar_t* const sc_paramName_Translate;
    static const wchar_t* const sc_paramName_OffsetX;
    static const wchar_t* const sc_paramName_OffsetY;
    static const wchar_t* const sc_paramName_Opacity;
    static const wchar_t* const sc_paramName_PrependOpacity;
    static const wchar_t* const sc_paramName_TransitionTargetOpacity;
    static const wchar_t* const sc_Expression_Translate;
    static const wchar_t* const sc_Expression_Translate3D;
    static const wchar_t* const sc_Expression_Scale;
    static const wchar_t* const sc_Expression_Scale_CenterPoint;
    static const wchar_t* const sc_Expression_Scale3D;
    static const wchar_t* const sc_Expression_Scale_CenterPoint3D;
    static const wchar_t* const sc_Expression_Rotate;
    static const wchar_t* const sc_Expression_Rotate_CenterPoint;
    static const wchar_t* const sc_Expression_Rotate3D_X;
    static const wchar_t* const sc_Expression_Rotate3D_X_CenterPoint;
    static const wchar_t* const sc_Expression_Rotate3D_Y;
    static const wchar_t* const sc_Expression_Rotate3D_Y_CenterPoint;
    static const wchar_t* const sc_Expression_Rotate3D_Z;
    static const wchar_t* const sc_Expression_Rotate3D_Z_CenterPoint;
    static const wchar_t* const sc_Expression_Skew;
    static const wchar_t* const sc_Expression_Skew_CenterPoint;
    static const wchar_t* const sc_Expression_MatrixTransform;
    static const wchar_t* const sc_Expression_MatrixTransform4x4;
    static const wchar_t* const sc_Expression_MatrixTransform3D;
    static const wchar_t* const sc_propertyName_MatrixTransform3D_Matrix;
    static const wchar_t* const sc_Expression_CompositeTransform;
    static const wchar_t* const sc_Expression_CompositeTransform_CenterPoint;
    static const wchar_t* const sc_Expression_CompositeTransform_ScaleTranslate;
    static const wchar_t* const sc_Expression_CompositeTransform_ScaleTranslate_4x4;
    static const wchar_t* const sc_Expression_CompositeTransform3D;
    static const wchar_t* const sc_Expression_CompositeTransform_CenterPoint3D;
    static const wchar_t* const sc_Expression_PlaneProjection;
    static const wchar_t* const sc_Expression_PlaneProjection_Rotation;
    static const wchar_t* const sc_Expression_PlaneProjection_RotateCenter;
    static const wchar_t* const sc_Expression_PlaneProjection_UndoRotateCenter;
    static const wchar_t* const sc_Expression_PlaneProjection_Perspective;
    static const wchar_t* const sc_propertyName_PlaneProjection_LocalOffset;
    static const wchar_t* const sc_propertyName_PlaneProjection_Rotation;
    static const wchar_t* const sc_propertyName_PlaneProjection_RotateCenter;
    static const wchar_t* const sc_propertyName_PlaneProjection_UndoRotateCenter;
    static const wchar_t* const sc_propertyName_PlaneProjection_GlobalOffset;
    static const wchar_t* const sc_propertyName_PlaneProjection_Perspective;
    static const wchar_t* const sc_propertyName_PlaneProjection_ElementWidth;
    static const wchar_t* const sc_propertyName_PlaneProjection_ElementHeight;
    static const wchar_t* const sc_propertyName_PlaneProjection_PerspectiveMatrix;
    static const wchar_t* const sc_Expression_TransformGroup1;
    static const wchar_t* const sc_Expression_TransformGroup2;
    static const wchar_t* const sc_Expression_TransformGroup3;
    static const wchar_t* const sc_Expression_PostRedirectionTransform;
    static const wchar_t* const sc_TransformGroupPropertyNameRoot;
    static const wchar_t* const sc_RtoFull3x2;
    static const wchar_t* const sc_RtoExpression;
    static const wchar_t* const sc_Expression_DMTransform;
    static const wchar_t* const sc_Expression_LocalTransform_Projection;
    static const wchar_t* const sc_Expression_LocalTransform_Transform3D;
    static const wchar_t* const sc_Expression_LocalTransform_Render;
    static const wchar_t* const sc_Expression_LocalTransform_TTRender;
    static const wchar_t* const sc_Expression_LocalTransform_FlowDirection;
    static const wchar_t* const sc_Expression_LocalTransform_DManip;
    static const wchar_t* const sc_Expression_LocalTransform_Redir;
    static const wchar_t* const sc_LocalTransformPropertySetName;
    static const wchar_t* const sc_propertyName_Transform3D;
    static const wchar_t* const sc_propertyName_Projection;
    static const wchar_t* const sc_propertyName_RenderTransform;
    static const wchar_t* const sc_propertyName_LocalDManipTransform;
    static const wchar_t* const sc_propertyName_TransitionTargetRenderTransform;
    static const wchar_t* const sc_propertyName_FlowDirectionTransform;
    static const wchar_t* const sc_propertyName_RedirectionTransform;
    static const wchar_t* const sc_propertyName_PostRedirectionTransform;
    static const wchar_t* const sc_propertyName_TransformMatrix;
    static const wchar_t* const sc_propertyName_Color;
    static const wchar_t* const sc_Expression_Offset;
    static const wchar_t* const sc_OffsetPropertyName;
    static const wchar_t* const sc_RotationPropertyName;
    static const wchar_t* const sc_ScalePropertyName;
    static const wchar_t* const sc_TransformMatrixPropertyName;
    static const wchar_t* const sc_CenterPointPropertyName;
    static const wchar_t* const sc_RotationAxisPropertyName;
    static const wchar_t* const sc_Expression_Opacity;
    static const wchar_t* const sc_Expression_TransitionTargetOpacity;
    static const wchar_t* const sc_OpacityPropertyName;
    static const wchar_t* const sc_ClipPrimaryTransformPropertySet;
    static const wchar_t* const sc_Clip_propertyName_Matrix;
    static const wchar_t* const sc_Expression_DManipClipTransform;
};
