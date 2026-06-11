// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using XamlOM;
using XamlOM.NewBuilders;

namespace Windows.Foundation.Numerics
{
    [Imported]
    [WindowsTypePattern]
    [TypeTable(IsExcludedFromVisualTree = true, IsExcludedFromReferenceTrackerWalk = true)]
    [SimpleType(OM.SimpleTypeKind.ValueByRef)]
    public struct Vector2
    {
    }

    [Imported]
    [WindowsTypePattern]
    [TypeTable(IsExcludedFromVisualTree = true, IsExcludedFromReferenceTrackerWalk = true)]
    [SimpleType(OM.SimpleTypeKind.ValueByRef)]
    public struct Vector3
    {
    }

    [Imported]
    [WindowsTypePattern]
    [TypeTable(IsExcludedFromVisualTree = true, IsExcludedFromReferenceTrackerWalk = true)]
    [SimpleType(OM.SimpleTypeKind.ValueByRef, DefaultValue = "{ 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f}")] // default is identity matrix
    public struct Matrix3x2
    {
    }

    [Imported]
    [WindowsTypePattern]
    [TypeTable(IsExcludedFromVisualTree = true, IsExcludedFromReferenceTrackerWalk = true)]
    [SimpleType(OM.SimpleTypeKind.ValueByRef, DefaultValue = "{ 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f }")] // default is identity matrix
    public struct Matrix4x4
    {
    }

    [Imported]
    [WindowsTypePattern]
    [TypeTable(IsExcludedFromVisualTree = true, IsExcludedFromReferenceTrackerWalk = true, ForceInclude = true)]
    [SimpleType(OM.SimpleTypeKind.ValueByRef)]
    public struct Quaternion
    {
    }

}
