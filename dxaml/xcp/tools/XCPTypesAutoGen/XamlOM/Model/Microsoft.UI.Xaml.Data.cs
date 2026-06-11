// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using System;
using XamlOM;

namespace Microsoft.UI.Xaml.Data
{
    [AttributeUsage(AttributeTargets.Class)]
    public class BindableAttribute : Attribute
    {
    }

    [DXamlIdlGroup("coretypes2")]
    [HideFromOldCodeGen]
    public interface ICollectionViewFactory
    {
        [ReturnTypeParameterName("result")]
        ICollectionView CreateView();
    }

    [HideFromOldCodeGen]
    public interface ISupportIncrementalLoading
    {
        [ReturnTypeParameterName("operation")]
        Windows.Foundation.IAsyncOperation<LoadMoreItemsResult> LoadMoreItemsAsync(uint count);

        [OrderHint(0)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        bool HasMoreItems { get; }
    }

    [CodeGen(CodeGenLevel.LookupOnly)]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    [HandWritten]
    [Guids(ClassGuid = "e67bf85d-ca76-4f23-a5f4-fdc42492ea3c")]
    public static class BindingOperations
    {
        public static void SetBinding(DependencyObject target, DependencyProperty dp, BindingBase binding)
        {
        }
    }

    [FrameworkTypePattern]
    [TypeTable(IsExcludedFromNewTypeTable = true)]
    [Guids(ClassGuid = "9ce9134c-3878-4a0f-9346-044b7c15e3a8")]
    public class PropertyChangedEventArgs
    {
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.String PropertyName
        {
            get;
            internal set;
        }

        [FactoryMethodName("CreateInstance")]
        public PropertyChangedEventArgs([Optional] Windows.Foundation.String name) { }
    }

    public interface INotifyPropertyChanged
    {
        event Microsoft.UI.Xaml.Data.PropertyChangedEventHandler PropertyChanged;
    }

    [ClassFlags(CoreTypeIsExternalObjectReference = true, IsMarkupExtension = true)]
    [OldCodeGenBaseType(typeof(Microsoft.UI.Xaml.DependencyObject))]
    [NativeName("CRelativeSource")]
    [Guids(ClassGuid = "b874679c-e5c6-4b65-9793-7529c3b31592")]
    public class RelativeSource
     : MarkupExtensionBase
    {
        [DependencyPropertyModifier(Modifier.Internal)]
        [OffsetFieldName("m_eMode")]
        public Microsoft.UI.Xaml.Data.RelativeSourceMode Mode
        {
            get;
            set;
        }

        public RelativeSource() { }
    }

    public interface IValueConverter
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Object Convert(Windows.Foundation.Object value, Windows.UI.Xaml.Interop.TypeName targetType, [Optional] Windows.Foundation.Object parameter, [Optional] Windows.Foundation.String language);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Object ConvertBack(Windows.Foundation.Object value, Windows.UI.Xaml.Interop.TypeName targetType, [Optional] Windows.Foundation.Object parameter, [Optional] Windows.Foundation.String language);
    }

    [TypeTable(IsExcludedFromCore = true)]
    [OldCodeGenBaseType(typeof(DependencyObject))]
    [NativeName("CBindingBase")]
    [Guids(ClassGuid = "1654f710-d87e-43c4-a2ce-70be6d813ec8")]
    public class BindingBase
     : Microsoft.UI.Xaml.MarkupExtensionBase
    {
        public BindingBase() { }
    }

    [CodeGen(partial: true)]
    [CoreBaseType(typeof(Microsoft.UI.Xaml.DependencyObject), NewCodeGenBaseType = typeof(BindingBase))]
    [ClassFlags(CoreTypeIsExternalObjectReference = true, IsISupportInitialize = true, IsMarkupExtension = true)]
    [Implements(typeof(Microsoft.UI.Xaml.ComponentModel.ISupportInitialize))]
    [NativeName("CBinding")]
    [Guids(ClassGuid = "9fe108de-6fec-4a38-972d-dd6015ab7621")]
    public class Binding
     : Microsoft.UI.Xaml.Data.BindingBase
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DependencyPropertyModifier(Modifier.Internal)]
        [CoreType(typeof(Windows.Foundation.String))]
        [OffsetFieldName("m_strPath")]
        public Microsoft.UI.Xaml.PropertyPath Path
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Internal)]
        public Microsoft.UI.Xaml.Data.BindingMode Mode
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Internal)]
        public Windows.Foundation.Object Source
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Internal)]
        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.Data.RelativeSource RelativeSource
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Internal)]
        public Windows.Foundation.String ElementName
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Internal)]
        public Microsoft.UI.Xaml.Data.IValueConverter Converter
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Internal)]
        public Windows.Foundation.Object ConverterParameter
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Internal)]
        public Windows.Foundation.String ConverterLanguage
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Internal)]
        public Windows.Foundation.Object FallbackValue
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Internal)]
        public Windows.Foundation.Object TargetNullValue
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Internal)]
        public Microsoft.UI.Xaml.Data.UpdateSourceTrigger UpdateSourceTrigger
        {
            get;
            set;
        }

        public Binding() { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    [Guids(ClassGuid = "dee7b57c-1574-4a67-9941-d17687d0a50d")]
    public abstract class BindingExpressionBase
    {
        internal BindingExpressionBase() { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    [Guids(ClassGuid = "0cefe851-8761-4e94-b5d3-60f4e7469ad8")]
    public class BindingExpression
     : Microsoft.UI.Xaml.Data.BindingExpressionBase
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Windows.Foundation.Object DataItem
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Microsoft.UI.Xaml.Data.Binding ParentBinding
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void UpdateSource()
        {
        }

        internal BindingExpression() { }
    }

    [Platform(typeof(PrivateApiContract), 1)]
    [TypeTable(IsExcludedFromCore = true, IsExcludedFromNewTypeTable = true, IsExcludedFromDXaml = true)]
    [EnumFlags(IsExcludedFromNative = true)]
    public enum EffectiveSourceType
    {
        None = 0,
        Binding_Source = 1,
        DataContext = 2,
        Mentor_DataContext = 3,
        Target = 4,
        TemplatedParent = 5,
        Mentor_TemplatedParent = 6,
        ElementName = 7,
        Mentor_ElementName = 8,
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "fd920d83-5697-4be3-b802-b49a78005ed1")]
    public class CurrentChangingEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Boolean Cancel
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Windows.Foundation.Boolean IsCancelable
        {
            get;
            private set;
        }

        public CurrentChangingEventArgs() { }

        [FactoryMethodName("CreateWithCancelableParameter")]
        public CurrentChangingEventArgs(Windows.Foundation.Boolean isCancelable) { }
    }

    [FrameworkTypePattern]
    [TypeTable(IsExcludedFromNewTypeTable = true)]
    public struct LoadMoreItemsResult
    {
        public Windows.Foundation.UInt32 Count { get; set; }
    }

    [FrameworkTypePattern]
    public interface ICustomProperty
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        Windows.UI.Xaml.Interop.TypeName Type
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        Windows.Foundation.String Name
        {
            get;
        }

        Windows.Foundation.Object GetValue(Windows.Foundation.Object target);
        void SetValue(Windows.Foundation.Object target, Windows.Foundation.Object value);

        Windows.Foundation.Object GetIndexedValue(Windows.Foundation.Object target, Windows.Foundation.Object index);
        void SetIndexedValue(Windows.Foundation.Object target, Windows.Foundation.Object value, Windows.Foundation.Object index);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [OrderHint(1)]
        Windows.Foundation.Boolean CanWrite
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [OrderHint(2)]
        Windows.Foundation.Boolean CanRead
        {
            get;
        }
    }

    [FrameworkTypePattern]
    public interface ICustomPropertyProvider
    {
        ICustomProperty GetCustomProperty(Windows.Foundation.String name);
        ICustomProperty GetIndexedProperty(Windows.Foundation.String name, Windows.UI.Xaml.Interop.TypeName type);
        Windows.Foundation.String GetStringRepresentation();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [OrderHint(1)]
        Windows.UI.Xaml.Interop.TypeName Type
        {
            get;
        }
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [Implements(typeof(Windows.Foundation.Collections.IObservableVector<Windows.Foundation.Object>))]
    public interface ICollectionView
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [ReadOnly]
        [PropertyKind(PropertyKind.PropertyOnly)]
        Windows.Foundation.Object CurrentItem
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [ReadOnly]
        [PropertyKind(PropertyKind.PropertyOnly)]
        Windows.Foundation.Int32 CurrentPosition
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [ReadOnly]
        [PropertyKind(PropertyKind.PropertyOnly)]
        Windows.Foundation.Boolean IsCurrentAfterLast
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [ReadOnly]
        [PropertyKind(PropertyKind.PropertyOnly)]
        Windows.Foundation.Boolean IsCurrentBeforeFirst
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [CollectionType(CollectionKind.Observable)]
        [ReadOnly]
        [PropertyKind(PropertyKind.PropertyOnly)]
        Microsoft.UI.Xaml.Controls.ItemCollection CollectionGroups
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Boolean HasMoreItems
        {
            get;
        }

        [CodeGen(CodeGenLevel.Idl)]
        [EventHandlerType(EventHandlerKind.TypedArgs)]
        event Microsoft.UI.Xaml.EventHandler CurrentChanged;

        [CodeGen(CodeGenLevel.Idl)]
        event Microsoft.UI.Xaml.Data.CurrentChangingEventHandler CurrentChanging;

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Boolean MoveCurrentTo([Optional] Windows.Foundation.Object item);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Boolean MoveCurrentToPosition(Windows.Foundation.Int32 index);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Boolean MoveCurrentToFirst();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Boolean MoveCurrentToLast();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Boolean MoveCurrentToNext();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Boolean MoveCurrentToPrevious();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.IAsyncOperation<Microsoft.UI.Xaml.Data.LoadMoreItemsResult> LoadMoreItemsAsync(Windows.Foundation.UInt32 count);
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [TypeTable(IsExcludedFromDXaml = false)]
    public interface ICollectionViewGroup
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [ReadOnly]
        [PropertyKind(PropertyKind.PropertyOnly)]
        Windows.Foundation.Object Group
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [CollectionType(CollectionKind.Observable)]
        [ReadOnly]
        [PropertyKind(PropertyKind.PropertyOnly)]
        Microsoft.UI.Xaml.Controls.ItemCollection GroupItems
        {
            get;
        }
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(partial: true)]
    [TypeTable(IsExcludedFromCore = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Data.ICollectionViewGroup), Order = 1)]
    [Implements(typeof(Microsoft.UI.Xaml.Data.ICustomPropertyProvider), Order = 2)]
    [Guids(ClassGuid = "d0133ca0-2b26-41cc-b566-5fae3a65aa65")]
    internal sealed class CollectionViewGroup : DependencyObject
    {
        public CollectionViewGroup() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(partial: true)]
    [TypeTable(IsExcludedFromCore = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Data.ICollectionView), Order = 1)]
    [Implements(typeof(Microsoft.UI.Xaml.Data.INotifyPropertyChanged), Order = 2)]
    [Implements(typeof(Microsoft.UI.Xaml.Data.ICustomPropertyProvider), Order = 3)]
    [Implements(typeof(Windows.Foundation.Collections.IVector<Windows.Foundation.Object>), Order = 4)]
    [Implements(typeof(Windows.Foundation.Collections.IIterable<Windows.Foundation.Object>), Order = 5)]
    [Guids(ClassGuid = "890bfa69-fe1b-4e08-a931-dcaa672da7b2")]
    internal abstract class CollectionView : DependencyObject
    {
        internal CollectionView() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(partial: true)]
    [TypeTable(IsExcludedFromCore = true)]
    [Guids(ClassGuid = "6aca7ab3-348f-4436-9c0a-0d59a2f96f5a")]
    internal class VectorCollectionView
     : Microsoft.UI.Xaml.Data.CollectionView
    {
        public VectorCollectionView() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(partial: true)]
    [TypeTable(IsExcludedFromCore = true)]
    [Guids(ClassGuid = "c3265e94-53a7-4d16-b89e-32048ee131b8")]
    internal sealed class VectorViewCollectionView
     : Microsoft.UI.Xaml.Data.CollectionView
    {
        public VectorViewCollectionView() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(partial: true)]
    [TypeTable(IsExcludedFromCore = true)]
    [Guids(ClassGuid = "e796d1d4-b265-4db3-80ca-4f42c0484556")]
    internal sealed class IterableCollectionView
     : Microsoft.UI.Xaml.Data.VectorCollectionView
    {
        public IterableCollectionView() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(partial: true)]
    [TypeTable(IsExcludedFromCore = true)]
    [Guids(ClassGuid = "f1c42e4b-1e00-4d22-873c-379d2629a48b")]
    internal sealed class GroupedDataCollectionView
     : Microsoft.UI.Xaml.Data.CollectionView
    {
        public GroupedDataCollectionView() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [CodeGen(partial: true)]
    [NativeName("CCollectionViewSource")]
    [Guids(ClassGuid = "a7dfaf16-d646-4018-99b6-a35729153b5f")]
    public sealed class CollectionViewSource
     : Microsoft.UI.Xaml.DependencyObject
    {
        [TypeTable(IsExcludedFromCore = true)]
        public Windows.Foundation.Object Source
        {
            get;
            set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        [ReadOnly]
        public Microsoft.UI.Xaml.Data.ICollectionView View
        {
            get;
            private set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        public Windows.Foundation.Boolean IsSourceGrouped
        {
            get;
            set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        public Microsoft.UI.Xaml.PropertyPath ItemsPath
        {
            get;
            set;
        }

        public CollectionViewSource() { }
    }

    [CodeGen(CodeGenLevel.CoreOnly)]
    [NativeName("CTemplateBindingExtension")]
    [ClassFlags(IsMarkupExtension = true)]
    [Guids(ClassGuid = "d8681455-2bf2-4d1c-9bc1-6763eec37fe4")]
    public sealed class TemplateBinding
     : Microsoft.UI.Xaml.MarkupExtensionBase
    {
        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_pSourceProperty")]
        public Microsoft.UI.Xaml.Internal.DependencyPropertyProxy Property
        {
            get;
            set;
        }

        public TemplateBinding() { }
    }

    [TypeTable(IsExcludedFromCore = true)]
    [EnumFlags(IsExcludedFromNative = true)]
    public enum BindingMode
    {
        OneWay = 1,
        OneTime = 2,
        TwoWay = 3,
    }

    [TypeTable(IsExcludedFromCore = true)]
    public enum RelativeSourceMode
    {
        None = 0,
        TemplatedParent = 1,
        Self = 2,
    }

    [TypeTable(IsExcludedFromCore = true)]
    [EnumFlags(IsExcludedFromNative = true)]
    public enum UpdateSourceTrigger
    {
        Default = 0,
        PropertyChanged = 1,
        Explicit = 2,
        LostFocus = 3,
    }

    public delegate void CurrentChangingEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Data.CurrentChangingEventArgs e);

    public delegate void PropertyChangedEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Data.PropertyChangedEventArgs e);

    [CodeGen(partial: true)]
    [DXamlIdlGroup("coretypes2")]
    [TypeFlags(IsCreateableFromXAML = false)]
    [ClassFlags(IsFreeThreaded = true)]
    [ThreadingModel(ThreadingModel.Both)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "075d4b8f-4a9d-4d2d-b308-b67b080b4a49")]
    public class ItemIndexRange
        : Windows.Foundation.Object
    {
        [PropertyKind(PropertyKind.PropertyOnly)]
        public int FirstIndex
        {
            get;
            private set;
        }

        [PropertyKind(PropertyKind.PropertyOnly)]
        public uint Length
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public int LastIndex
        {
            get;
            private set;
        }

        internal ItemIndexRange() { }

        public ItemIndexRange(int firstIndex, uint length) { }
    }

    [DXamlIdlGroup("Controls2")]
    [TypeTable(IsExcludedFromCore = true)]
    [Implements(typeof(Windows.Foundation.IClosable))]
    [HideFromOldCodeGen]
    public interface IItemsRangeInfo
    {
        void RangesChanged(Microsoft.UI.Xaml.Data.ItemIndexRange visibleRange, Windows.Foundation.Collections.IVectorView<ItemIndexRange> trackedItems);
    }

    [DXamlIdlGroup("Controls2")]
    [TypeTable(IsExcludedFromCore = true)]
    [HideFromOldCodeGen]
    public interface ISelectionInfo
    {
        void SelectRange(ItemIndexRange itemIndexRange);

        void DeselectRange(ItemIndexRange itemIndexRange);

        bool IsSelected(int index);

        Windows.Foundation.Collections.IVectorView<ItemIndexRange> GetSelectedRanges();
    }

    [FrameworkTypePattern]
    [DXamlIdlGroup("coretypes2")]
    [TypeTable(IsExcludedFromNewTypeTable = true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [Guids(ClassGuid = "be37806e-9c1d-42dc-b217-75d357865832")]
    public sealed class DataErrorsChangedEventArgs : EventArgs
    {
        [PropertyKind(PropertyKind.PropertyOnly)]
        public string PropertyName
        {
            get;
            set;
        }

        [FactoryMethodName("CreateInstance")]
        public DataErrorsChangedEventArgs(Windows.Foundation.String name) { }
    }

    [DXamlIdlGroup("coretypes2")]
    public interface INotifyDataErrorInfo
    {
        bool HasErrors
        {
            get;
        }

        [OrderHint(2)]
        Windows.Foundation.Collections.IIterable<object> GetErrors(Windows.Foundation.String propertyName);

        event Windows.Foundation.EventHandler<DataErrorsChangedEventArgs> ErrorsChanged;
    }
}

