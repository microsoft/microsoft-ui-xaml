// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using XamlOM;

namespace Microsoft.UI.Xaml.Core.Direct
{
    [DXamlIdlGroup("coretypes2")]
    [ContractVersion(1)]
    public sealed class XamlDirectContract : Contract { }

    [NativeName("XamlTypeIndex")]
    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true, IsStableTypeIndex = true)]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromNewTypeTable = true)]
    [DXamlIdlGroup("coretypes2")]
    [Platform(typeof(XamlDirectContract), 1)]
    [VelocityFeature("Feature_XamlDirect")]
    public enum XamlTypeIndex
    {
    }

    [NativeName("XamlPropertyIndex")]
    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true, IsStablePropertyIndex = true)]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromNewTypeTable = true)]
    [DXamlIdlGroup("coretypes2")]
    [Platform(typeof(XamlDirectContract), 1)]
    [VelocityFeature("Feature_XamlDirect")]
    public enum XamlPropertyIndex
    {
    }

    [NativeName("XamlEventIndex")]
    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true, IsStableEventIndex = true)]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromNewTypeTable = true)]
    [DXamlIdlGroup("coretypes2")]
    [Platform(typeof(XamlDirectContract), 1)]
    [VelocityFeature("Feature_XamlDirect")]
    public enum XamlEventIndex
    {
    }

    [DXamlIdlGroup("coretypes2")]
    [Platform(typeof(XamlDirectContract), 1)]
    [VelocityFeature("Feature_XamlDirect")]
    public interface IXamlDirectObject
    {
    }

    [CodeGen(partial: true)]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    [DXamlIdlGroup("coretypes2")]
    [Platform(typeof(XamlDirectContract), 1)]
    [Guids(ClassGuid = "6a38cf51-578f-4551-b5ef-6b3cf208b35f")]
    [VelocityFeature("Feature_XamlDirect")]
    public sealed class XamlDirect
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static XamlDirect GetDefault()
        {
            return default(XamlDirect);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Foundation.Object GetObject(IXamlDirectObject xamlDirectObject)
        {
            return default(Windows.Foundation.Object);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public IXamlDirectObject GetXamlDirectObject(Windows.Foundation.Object @object)
        {
            return default(IXamlDirectObject);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public IXamlDirectObject CreateInstance(XamlTypeIndex typeIndex)
        {
            return default(IXamlDirectObject);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void SetObjectProperty(IXamlDirectObject xamlDirectObject, XamlPropertyIndex propertyIndex, [Optional] Windows.Foundation.Object value)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void SetXamlDirectObjectProperty(IXamlDirectObject xamlDirectObject, XamlPropertyIndex propertyIndex, [Optional] IXamlDirectObject value)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void SetBooleanProperty(IXamlDirectObject xamlDirectObject, XamlPropertyIndex propertyIndex, bool value)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void SetDoubleProperty(IXamlDirectObject xamlDirectObject, XamlPropertyIndex propertyIndex, double value)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void SetInt32Property(IXamlDirectObject xamlDirectObject, XamlPropertyIndex propertyIndex, int value)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void SetStringProperty(IXamlDirectObject xamlDirectObject, XamlPropertyIndex propertyIndex, [Optional] string value)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void SetDateTimeProperty(IXamlDirectObject xamlDirectObject, XamlPropertyIndex propertyIndex, Windows.Foundation.DateTime value)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void SetPointProperty(IXamlDirectObject xamlDirectObject, XamlPropertyIndex propertyIndex, Windows.Foundation.Point value)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void SetRectProperty(IXamlDirectObject xamlDirectObject, XamlPropertyIndex propertyIndex, Windows.Foundation.Rect value)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void SetSizeProperty(IXamlDirectObject xamlDirectObject, XamlPropertyIndex propertyIndex, Windows.Foundation.Size value)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void SetTimeSpanProperty(IXamlDirectObject xamlDirectObject, XamlPropertyIndex propertyIndex, Windows.Foundation.TimeSpan value)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void SetColorProperty(IXamlDirectObject xamlDirectObject, XamlPropertyIndex propertyIndex, Windows.UI.Color value)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void SetCornerRadiusProperty(IXamlDirectObject xamlDirectObject, XamlPropertyIndex propertyIndex, Microsoft.UI.Xaml.CornerRadius value)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void SetDurationProperty(IXamlDirectObject xamlDirectObject, XamlPropertyIndex propertyIndex, Microsoft.UI.Xaml.Duration value)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void SetGridLengthProperty(IXamlDirectObject xamlDirectObject, XamlPropertyIndex propertyIndex, Microsoft.UI.Xaml.GridLength value)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void SetThicknessProperty(IXamlDirectObject xamlDirectObject, XamlPropertyIndex propertyIndex, Microsoft.UI.Xaml.Thickness value)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void SetMatrixProperty(IXamlDirectObject xamlDirectObject, XamlPropertyIndex propertyIndex, Microsoft.UI.Xaml.Media.Matrix value)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void SetMatrix3DProperty(IXamlDirectObject xamlDirectObject, XamlPropertyIndex propertyIndex, Microsoft.UI.Xaml.Media.Media3D.Matrix3D value)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void SetEnumProperty(IXamlDirectObject xamlDirectObject, XamlPropertyIndex propertyIndex, uint value)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Foundation.Object GetObjectProperty(IXamlDirectObject xamlDirectObject, XamlPropertyIndex propertyIndex)
        {
            return default(Windows.Foundation.Object);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public IXamlDirectObject GetXamlDirectObjectProperty(IXamlDirectObject xamlDirectObject, XamlPropertyIndex propertyIndex)
        {
            return default(IXamlDirectObject);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public bool GetBooleanProperty(IXamlDirectObject xamlDirectObject, XamlPropertyIndex propertyIndex)
        {
            return default(bool);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public double GetDoubleProperty(IXamlDirectObject xamlDirectObject, XamlPropertyIndex propertyIndex)
        {
            return default(double);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public int GetInt32Property(IXamlDirectObject xamlDirectObject, XamlPropertyIndex propertyIndex)
        {
            return default(int);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public string GetStringProperty(IXamlDirectObject xamlDirectObject, XamlPropertyIndex propertyIndex)
        {
            return default(string);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Foundation.DateTime GetDateTimeProperty(IXamlDirectObject xamlDirectObject, XamlPropertyIndex propertyIndex)
        {
            return default(Windows.Foundation.DateTime);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Foundation.Point GetPointProperty(IXamlDirectObject xamlDirectObject, XamlPropertyIndex propertyIndex)
        {
            return default(Windows.Foundation.Point);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Foundation.Rect GetRectProperty(IXamlDirectObject xamlDirectObject, XamlPropertyIndex propertyIndex)
        {
            return default(Windows.Foundation.Rect);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Foundation.Size GetSizeProperty(IXamlDirectObject xamlDirectObject, XamlPropertyIndex propertyIndex)
        {
            return default(Windows.Foundation.Size);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Foundation.TimeSpan GetTimeSpanProperty(IXamlDirectObject xamlDirectObject, XamlPropertyIndex propertyIndex)
        {
            return default(Windows.Foundation.TimeSpan);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.UI.Color GetColorProperty(IXamlDirectObject xamlDirectObject, XamlPropertyIndex propertyIndex)
        {
            return default(Windows.UI.Color);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Microsoft.UI.Xaml.CornerRadius GetCornerRadiusProperty(IXamlDirectObject xamlDirectObject, XamlPropertyIndex propertyIndex)
        {
            return default(Microsoft.UI.Xaml.CornerRadius);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Microsoft.UI.Xaml.Duration GetDurationProperty(IXamlDirectObject xamlDirectObject, XamlPropertyIndex propertyIndex)
        {
            return default(Microsoft.UI.Xaml.Duration);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Microsoft.UI.Xaml.GridLength GetGridLengthProperty(IXamlDirectObject xamlDirectObject, XamlPropertyIndex propertyIndex)
        {
            return default(Microsoft.UI.Xaml.GridLength);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Microsoft.UI.Xaml.Thickness GetThicknessProperty(IXamlDirectObject xamlDirectObject, XamlPropertyIndex propertyIndex)
        {
            return default(Microsoft.UI.Xaml.Thickness);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Microsoft.UI.Xaml.Media.Matrix GetMatrixProperty(IXamlDirectObject xamlDirectObject, XamlPropertyIndex propertyIndex)
        {
            return default(Microsoft.UI.Xaml.Media.Matrix);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Microsoft.UI.Xaml.Media.Media3D.Matrix3D GetMatrix3DProperty(IXamlDirectObject xamlDirectObject, XamlPropertyIndex propertyIndex)
        {
            return default(Microsoft.UI.Xaml.Media.Media3D.Matrix3D);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public uint GetEnumProperty(IXamlDirectObject xamlDirectObject, XamlPropertyIndex propertyIndex)
        {
            return default(uint);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void ClearProperty(IXamlDirectObject xamlDirectObject, XamlPropertyIndex propertyIndex)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public uint GetCollectionCount(IXamlDirectObject xamlDirectObject)
        {
            return default(uint);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public IXamlDirectObject GetXamlDirectObjectFromCollectionAt(IXamlDirectObject xamlDirectObject, uint index)
        {
            return default(IXamlDirectObject);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void AddToCollection(IXamlDirectObject xamlDirectObject, IXamlDirectObject value)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void InsertIntoCollectionAt(IXamlDirectObject xamlDirectObject, uint index, IXamlDirectObject value)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public bool RemoveFromCollection(IXamlDirectObject xamlDirectObject, IXamlDirectObject value)
        {
            return false;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void RemoveFromCollectionAt(IXamlDirectObject xamlDirectObject, uint index)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void ClearCollection(IXamlDirectObject xamlDirectObject)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DXamlOverloadName("AddEventHandler")]
        [DXamlName("AddEventHandler")]
        [DefaultOverload]
        public void AddEventHandler(IXamlDirectObject xamlDirectObject, XamlEventIndex eventIndex, Windows.Foundation.Object handler)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DXamlOverloadName("AddEventHandler")]
        [DXamlName("AddEventHandler_HandledEventsToo")]
        public void AddEventHandler(IXamlDirectObject xamlDirectObject, XamlEventIndex eventIndex, Windows.Foundation.Object handler, bool handledEventsToo)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void RemoveEventHandler(IXamlDirectObject xamlDirectObject, XamlEventIndex eventIndex, Windows.Foundation.Object handler)
        {
        }

        internal XamlDirect() { }
    }
}
