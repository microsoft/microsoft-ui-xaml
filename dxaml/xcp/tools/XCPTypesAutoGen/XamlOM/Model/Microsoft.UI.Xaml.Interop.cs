// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using XamlOM;

namespace Microsoft.UI.Xaml.Interop
{
    [DXamlIdlGroup("Controls")]
    [CodeGen(CodeGenLevel.LookupOnly)]
    public delegate void BindableVectorChangedEventHandler(IBindableObservableVector vector, Windows.Foundation.Object e);

    [DXamlIdlGroup("Controls")]
    [TypeTable(IsExcludedFromDXaml = false, ForceInclude = true)]
    public interface IBindableIterator
    {
        Windows.Foundation.Object Current { get; }
        Windows.Foundation.Boolean HasCurrent { get; }

        Windows.Foundation.Boolean MoveNext();
    }

    [DXamlIdlGroup("Controls")]
    [TypeTable(IsExcludedFromDXaml = false, ForceInclude = true)]
    public interface IBindableIterable
    {
        IBindableIterator First();
    }

    [DXamlIdlGroup("Controls")]
    [TypeTable(IsExcludedFromDXaml = false, ForceInclude = true)]
    [Implements(typeof(IBindableIterable))]
    public interface IBindableVectorView
    {
        [OrderHint(0)]
        Windows.Foundation.Object GetAt(Windows.Foundation.UInt32 index);

        [OrderHint(1)]
        Windows.Foundation.UInt32 Size { get; }

        [OrderHint(2)]
        Windows.Foundation.Boolean IndexOf(Windows.Foundation.Object value, out Windows.Foundation.UInt32 index);
    }

    [DXamlIdlGroup("Controls")]
    [Implements(typeof(IBindableIterable))]
    [TypeTable(IsExcludedFromDXaml = false, ForceInclude = true)]
    public interface IBindableVector
    {
        [OrderHint(0)]
        Windows.Foundation.Object GetAt(Windows.Foundation.UInt32 index);

        [OrderHint(1)]
        Windows.Foundation.UInt32 Size { get; }

        [OrderHint(2)]
        IBindableVectorView GetView();

        [OrderHint(3)]
        Windows.Foundation.Boolean IndexOf(Windows.Foundation.Object value, out Windows.Foundation.UInt32 index);

        [OrderHint(4)]
        void SetAt(Windows.Foundation.UInt32 index, Windows.Foundation.Object value);

        [OrderHint(5)]
        void InsertAt(Windows.Foundation.UInt32 index, Windows.Foundation.Object value);

        [OrderHint(6)]
        void RemoveAt(Windows.Foundation.UInt32 index);

        [OrderHint(7)]
        void Append(Windows.Foundation.Object value);

        [OrderHint(8)]
        void RemoveAtEnd();

        [OrderHint(9)]
        void Clear();
    }

    [DXamlIdlGroup("Controls")]
    [CodeGen(CodeGenLevel.LookupOnly)]
    [TypeTable(IsExcludedFromDXaml = false, ForceInclude = true)]
    [Implements(typeof(IBindableVector))]
    public interface IBindableObservableVector
    {
        event BindableVectorChangedEventHandler VectorChanged;
    }

    [DXamlIdlGroup("Controls")]
    [TypeTable(IsExcludedFromCore = true)]
    [EnumFlags(IsExcludedFromNative = true)]
    public enum NotifyCollectionChangedAction
    {
        Add = 0,
        Remove = 1,
        Replace = 2,
        Move = 3,
        Reset = 4,
    }
    
    [DXamlIdlGroup("Controls")]
    [FrameworkTypePattern]
    [TypeTable(IsExcludedFromNewTypeTable = true)]
    [Guids(ClassGuid = "8daf9e4f-2d86-498d-a56f-5379ffe47b54")]
    public class NotifyCollectionChangedEventArgs
    {
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Microsoft.UI.Xaml.Interop.NotifyCollectionChangedAction Action
        {
            get;
            internal set;
        }

        [PropertyKind(PropertyKind.PropertyOnly)]
        public Microsoft.UI.Xaml.Interop.IBindableVector NewItems
        {
            get;
            internal set;
        }

        [PropertyKind(PropertyKind.PropertyOnly)]
        public Microsoft.UI.Xaml.Interop.IBindableVector OldItems
        {
            get;
            internal set;
        }

        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Int32 NewStartingIndex
        {
            get;
            internal set;
        }

        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Int32 OldStartingIndex
        {
            get;
            internal set;
        }

        internal NotifyCollectionChangedEventArgs() {}

        [FactoryMethodName("CreateInstanceWithAllParameters")]
        public NotifyCollectionChangedEventArgs(Microsoft.UI.Xaml.Interop.NotifyCollectionChangedAction action, [Optional] Microsoft.UI.Xaml.Interop.IBindableVector newItems, [Optional] Microsoft.UI.Xaml.Interop.IBindableVector oldItems, Windows.Foundation.Int32 newIndex, Windows.Foundation.Int32 oldIndex) {}
    }
    
    [DXamlIdlGroup("Controls")]
    public interface INotifyCollectionChanged
    {
        event Microsoft.UI.Xaml.Interop.NotifyCollectionChangedEventHandler CollectionChanged;

    }
    
    [DXamlIdlGroup("Controls")]
    public delegate void NotifyCollectionChangedEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Interop.NotifyCollectionChangedEventArgs e);
    
}

