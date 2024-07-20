// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using XamlOM;

namespace Windows.Foundation
{
    [Imported("uriruntimeclass.idl")]
    [DefaultInterfaceName("IUriRuntimeClass")]
    [WindowsTypePattern(Order = 1)]
    [NativeCreationMethodName("Create", Order = 2)]
    [ClassFlags(HasTypeConverter = true)]
    public sealed class Uri
    {
    }

    [Imported]
    [WindowsTypePattern]
    [TypeTable(IsExcludedFromVisualTree = true, IsExcludedFromReferenceTrackerWalk = true)]
    [TypeFlags(HasSpecialBoxer = true)]
    public struct DateTime
    {
    }

    [Imported]
    [WindowsTypePattern]
    public interface IAsyncOperation<T>
    {
    }

    [Imported]
    [WindowsTypePattern]
    public interface IReference<T>
    {
    }

    [Imported]
    [WindowsTypePattern]
    public interface IAsyncAction
    {
    }

    // This only exists for Windows 8/Blue compat. We need an "EventHandler" entry in our type table, because 
    // old app's generated IXamlTypes say that Windows.Foundation.EventHandler is the base class of delegates.
    [Imported]
    [HideFromOldCodeGen]
    [TypeTable(ForceInclude = true, ExcludeGuid = true)]
    [DXamlName("EventHandler")]
    [DXamlTypeTableName("EventHandler")]
    public class EventHandlerStub
    {
    }

    [Imported]
    [WindowsTypePattern]
    [CodeGen(CodeGenLevel.LookupOnly)]
    [DXamlName("EventHandler")]
    public delegate void EventHandler<TArgs>(object sender, TArgs args);

    [Imported]
    [WindowsTypePattern]
    public delegate void TypedEventHandler<TSender, TArgs>(TSender sender, TArgs args);

    [ExternalIdl]
    [TypeFlags(IsDXamlSystemType = true, HasSpecialBoxer = true)]
    [NativeName("CTimeSpan")]
    [CoreBaseType(typeof(Microsoft.UI.Xaml.DependencyObject))]
    [ClassFlags(HasTypeConverter = true)]
    [SimpleType(OM.SimpleTypeKind.Value)]
    [Guids(ClassGuid = "fcffbe72-fe62-4c99-b0c2-2521a93cd1a3")]
    public struct TimeSpan
    {
        [NativeStorageType(OM.ValueType.valueDouble)]
        [OffsetFieldName("m_rTimeSpan")]
        public Windows.Foundation.Double Seconds
        {
            get;
            set;
        }
    }

    [ExternalIdl]
    [TypeFlags(IsDXamlSystemType = true, HasSpecialBoxer = true)]
    [NativeInteropName("XPOINTF")]
    [BuiltinStruct("CPoint")]
    [TypeTable(IsExcludedFromVisualTree = true, IsExcludedFromReferenceTrackerWalk = true)]
    [Guids(ClassGuid = "ef75db67-32e5-4762-9e7f-4818c2c32ccd")]
    public struct Point
    {
        [CoreType(typeof(Windows.Foundation.Double))]
        [NativeStorageType(OM.ValueType.valuePoint)]
        [OffsetFieldName("m_pt")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public object ContentProperty
        {
            get;
            set;
        }

        [CoreType(typeof(Windows.Foundation.Double))]
        [NativeStorageType(OM.ValueType.valueFloat)]
        [OffsetFieldName("m_pt.x")]
        public Windows.Foundation.Float X
        {
            get;
            set;
        }

        [CoreType(typeof(Windows.Foundation.Double))]
        [NativeStorageType(OM.ValueType.valueFloat)]
        [OffsetFieldName("m_pt.y")]
        public Windows.Foundation.Float Y
        {
            get;
            set;
        }
    }

    [ExternalIdl]
    [TypeFlags(IsDXamlSystemType = true)]
    [BuiltinStruct("CSize")]
    [TypeTable(IsExcludedFromVisualTree = true, IsExcludedFromReferenceTrackerWalk = true)]
    [Guids(ClassGuid = "aa0177ea-da79-4d27-8aea-c8d7e448dcb9")]
    public struct Size
    {
        [CoreType(typeof(Windows.Foundation.Double))]
        [NativeStorageType(OM.ValueType.valueFloat)]
        [OffsetFieldName("m_size.height")]
        public Windows.Foundation.Float Height
        {
            get;
            set;
        }

        [CoreType(typeof(Windows.Foundation.Double))]
        [NativeStorageType(OM.ValueType.valueFloat)]
        [OffsetFieldName("m_size.width")]
        public Windows.Foundation.Float Width
        {
            get;
            set;
        }
    }

    [ExternalIdl]
    [TypeFlags(IsDXamlSystemType = true)]
    [NativeInteropName("XRECTF")]
    [BuiltinStruct("CRect")]
    [TypeTable(IsExcludedFromVisualTree = true, IsExcludedFromReferenceTrackerWalk = true)]
    [Guids(ClassGuid = "95a62048-162b-4669-bdaa-de957aa6b47d")]
    public struct Rect
    {
        [CoreType(typeof(Windows.Foundation.Double))]
        [NativeStorageType(OM.ValueType.valueFloat)]
        [OffsetFieldName("m_rc.X")]
        public Windows.Foundation.Float X
        {
            get;
            set;
        }

        [CoreType(typeof(Windows.Foundation.Double))]
        [NativeStorageType(OM.ValueType.valueFloat)]
        [OffsetFieldName("m_rc.Y")]
        public Windows.Foundation.Float Y
        {
            get;
            set;
        }

        [CoreType(typeof(Windows.Foundation.Double))]
        [NativeStorageType(OM.ValueType.valueFloat)]
        [OffsetFieldName("m_rc.Width")]
        public Windows.Foundation.Float Width
        {
            get;
            set;
        }

        [CoreType(typeof(Windows.Foundation.Double))]
        [NativeStorageType(OM.ValueType.valueFloat)]
        [OffsetFieldName("m_rc.Height")]
        public Windows.Foundation.Float Height
        {
            get;
            set;
        }
    }

    [Imported]
    [CodeGen(CodeGenLevel.LookupOnly)]
    [TypeTable(IsExcludedFromCore = true, IsExcludedFromDXaml = true, IsExcludedFromNewTypeTable = true)]
    [ClassFlags(IsPrimitive = true)]
    public struct HRESULT
    {
    }

    [ExternalIdl("inspectable.idl")]
    [CodeGen(CodeGenLevel.LookupOnly)]
    [TypeFlags(IsDXamlSystemType = true)]
    [DXamlTypeTableName("Object")]
    [DXamlName("IInspectable", "IInspectable")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(IsPrimitive = true)]
    [NativeCreationMethodName("")]
    public class Object
    {
    }

    [ExternalIdl]
    [DXamlName("DOUBLE")]
    [NativeName("CDouble")]
    [CoreBaseType(typeof(Microsoft.UI.Xaml.DependencyObject))]
    [ClassFlags(HasTypeConverter = true, IsPrimitive = true)]
    [CustomNames(PrimitiveCppName = "DOUBLE")]
    [SimpleType(OM.SimpleTypeKind.Value)]
    public struct Double
    {
        [NativeStorageType(OM.ValueType.valueFloat)]
        [OffsetFieldName("m_eValue")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public object ContentProperty
        {
            get;
            set;
        }
    }

    [ExternalIdl]
    [DXamlTypeTableName("Single")]
    [DXamlName("FLOAT")]
    [NativeName("XFLOAT")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(IsPrimitive = true, HasTypeConverter = false)]
    [CustomNames(PrimitiveCppName = "FLOAT", RealTypeName = "Single")]
    [SimpleType(OM.SimpleTypeKind.Value)]
    public struct Float
    {
    }

    [ExternalIdl]
    [DXamlName("INT")]
    [NativeInteropName("XINT32")]
    [NativeName("CInt32")]
    [CoreBaseType(typeof(Microsoft.UI.Xaml.DependencyObject))]
    [ClassFlags(HasTypeConverter = true, IsPrimitive = true)]
    [CustomNames(PrimitiveCppName = "INT")]
    [SimpleType(OM.SimpleTypeKind.Value)]
    public struct Int32
    {
        [NativeStorageType(OM.ValueType.valueSigned)]
        [OffsetFieldName("m_iValue")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public object ContentProperty
        {
            get;
            set;
        }
    }

    [ExternalIdl]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlName("BYTE")]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true, IsExcludedFromNewTypeTable = false)]
    [ClassFlags(IsPrimitive = true, IsVisibleInXAML = false)]
    [CustomNames(PrimitiveCppName = "BYTE", RealTypeName = "UInt8")]
    public struct Byte
    {
    }

    [ExternalIdl]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlName("INT16")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(IsPrimitive = true)]
    [CustomNames(PrimitiveCppName = "INT16")]
    public struct Int16
    {
    }

    [ExternalIdl]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlName("UINT16")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(IsPrimitive = true)]
    [CustomNames(PrimitiveCppName = "UINT16")]
    public struct UInt16
    {
    }

    [ExternalIdl]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlName("WCHAR")]
    [TypeTable(IsExcludedFromCore = true, ExcludeGuid = true)]
    [ClassFlags(IsPrimitive = true)]
    [CustomNames(PrimitiveCppName = "WCHAR", RealTypeName = "Char")]
    public struct Char16
    {
    }

    [ExternalIdl]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlName("GUID")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(IsPrimitive = true)]
    [CustomNames(PrimitiveCppName = "GUID")]
    public struct Guid
    {
    }

    [ExternalIdl]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlName("UINT")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(IsPrimitive = true, IsVisibleInXAML = false)]
    [CustomNames(PrimitiveCppName = "UINT")]
    public struct UInt32
    {
    }

    [ExternalIdl(IdlTypeName = "UINT64")]
    [DXamlInterfaceName("UINT64")]
    [DXamlName("UINT64")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(IsPrimitive = true)]
    [CustomNames(PrimitiveCppName = "UINT64")]
    public struct UInt64
    {
    }

    [ExternalIdl(IdlTypeName = "LONGLONG")]
    [DXamlInterfaceName("LONGLONG")]
    [DXamlName("INT64")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(IsPrimitive = true)]
    [CustomNames(PrimitiveCppName = "INT64")]
    public struct Int64
    {
    }

    [NativeName("CEnumerated")]
    [TypeTable(IsExcludedFromDXaml = true, ExcludeGuid = true)]
    [ClassFlags(HasTypeConverter = true, IsPrimitive = true, IsTypeConverter = true, IsHiddenFromIdl = true)]
    internal abstract class Enumerated : Microsoft.UI.Xaml.DependencyObject
    {
        internal Enumerated() { }
    }

    [ExternalIdl(IdlTypeName = "boolean")]
    [DXamlInterfaceName("boolean")]
    [DXamlName("BOOLEAN")]
    [NativeInteropName("bool")]
    [NativeName("CBoolean")]
    [CoreBaseType(typeof(Microsoft.UI.Xaml.DependencyObject))]
    [ClassFlags(HasTypeConverter = true, IsPrimitive = true)]
    [CustomNames(PrimitiveCoreName = "bool", PrimitiveCppName = "BOOLEAN", AbiGenericArgumentName = "bool")]
    public struct Boolean
    {
    }

    [ExternalIdl]
    [DXamlName("HSTRING")]
    [NativeName("CString")]
    [CoreBaseType(typeof(Microsoft.UI.Xaml.DependencyObject))]
    [ClassFlags(HasTypeConverter = true, IsPrimitive = true)]
    [CustomNames(PrimitiveCoreName = "xstring_ptr", PrimitiveCppName = "HSTRING")]
    public struct String
    {
        [NativeStorageType(OM.ValueType.valueString)]
        [OffsetFieldName("m_strString")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public object ContentProperty
        {
            get;
            set;
        }
    }

    [Imported("windows.foundation.idl")]
    [WindowsTypePattern]
    public interface IClosable
    {
    }

    [Imported("windows.foundation.idl")]
    [WindowsTypePattern]
    public sealed class Deferral
    {
    }

}

