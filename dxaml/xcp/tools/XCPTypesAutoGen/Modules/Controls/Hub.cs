// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using XamlOM;
using Microsoft.UI.Xaml.Markup;

namespace Microsoft.UI.Xaml.Controls
{
    [FrameworkTypePattern]
    [CodeGen(partial: true)]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.ISemanticZoomInformation))]
    [ContentProperty("Sections")]
    [DXamlIdlGroup("Controls2")]
    [InstanceCountTelemetry]
    [Guids(ClassGuid = "12d34ed0-404d-4fd8-a026-156a08a5cfa9")]
    public class Hub
     : Microsoft.UI.Xaml.Controls.Control
    {
        public Windows.Foundation.Object Header
        {
            get;
            set;
        }

        public Microsoft.UI.Xaml.DataTemplate HeaderTemplate
        {
            get;
            set;
        }

        public Microsoft.UI.Xaml.Controls.Orientation Orientation
        {
            get;
            set;
        }

        public int DefaultSectionIndex
        {
            get;
            set;
        }

        [CollectionType(CollectionKind.Vector)]
        [DependencyPropertyModifier(Modifier.Private)]
        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.Controls.HubSectionCollection Sections
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [CollectionType(CollectionKind.Vector)]
        [DependencyPropertyModifier(Modifier.Private)]
        [ReadOnly]
        public Microsoft.UI.Xaml.Controls.HubSectionCollection SectionsInView
        {
            get;
            internal set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [CollectionType(CollectionKind.Observable)]
        [DependencyPropertyModifier(Modifier.Private)]
        [ReadOnly]
        public Microsoft.UI.Xaml.Controls.ItemCollection SectionHeaders
        {
            get;
            internal set;
        }

        public event Microsoft.UI.Xaml.Controls.HubSectionHeaderClickEventHandler SectionHeaderClick;

        public event Microsoft.UI.Xaml.Controls.SectionsInViewChangedEventHandler SectionsInViewChanged;

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void ScrollToSection(Microsoft.UI.Xaml.Controls.HubSection section)
        {
        }
    }

    [FrameworkTypePattern]
    [CodeGen(partial: true)]
    [ContentProperty("ContentTemplate")]
    [DXamlIdlGroup("Controls2")]
    [Guids(ClassGuid = "f5a5728f-e2e8-42e4-9490-2f5df7dc6235")]
    public class HubSection
     : Microsoft.UI.Xaml.Controls.Control
    {
        public Windows.Foundation.Object Header
        {
            get;
            set;
        }

        public Microsoft.UI.Xaml.DataTemplate HeaderTemplate
        {
            get;
            set;
        }

        public Microsoft.UI.Xaml.DataTemplate ContentTemplate
        {
            get;
            set;
        }

        public Windows.Foundation.Boolean IsHeaderInteractive
        {
            get;
            set;
        }
    }

    [CodeGen(partial: true)]
    [DXamlIdlGroup("Controls2")]
    [NativeName("CHubSectionCollection")]
    [ClassFlags(HasBaseTypeInDXamlInterface = false)]
    [Guids(ClassGuid = "3df2d721-6551-47aa-8447-43da41afa38b")]
    public sealed class HubSectionCollection
     : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<HubSection>
    {
        [CoreType(typeof(Microsoft.UI.Xaml.DependencyObject))]
        [NativeStorageType(ValueType.valueObject)]
        public Microsoft.UI.Xaml.Controls.HubSection ContentProperty
        {
            get;
            set;
        }

        internal HubSectionCollection() { }
    }

    [DXamlIdlGroup("Controls2")]
    [TypeFlags(IsCreateableFromXAML = false)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "3b8c4614-a2a4-409e-8820-6de2ec88138e")]
    public sealed class HubSectionHeaderClickEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        [DependencyPropertyModifier(Modifier.Private)]
        public Microsoft.UI.Xaml.Controls.HubSection Section
        {
            get;
            internal set;
        }

        public HubSectionHeaderClickEventArgs() { }
    }

    [DXamlIdlGroup("Controls2")]
    public delegate void HubSectionHeaderClickEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Controls.HubSectionHeaderClickEventArgs e);

    [CodeGen(partial: true)]
    [DXamlIdlGroup("Controls2")]
    [TypeFlags(IsCreateableFromXAML = false)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "01c46c61-6ba8-4a3f-bda3-d081f6b37bb3")]
    public class SectionsInViewChangedEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true)]
        [CollectionType(CollectionKind.Vector)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Microsoft.UI.Xaml.Controls.HubSectionCollection AddedSections
        {
            get;
            internal set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true)]
        [CollectionType(CollectionKind.Vector)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Microsoft.UI.Xaml.Controls.HubSectionCollection RemovedSections
        {
            get;
            internal set;
        }

        internal SectionsInViewChangedEventArgs() { }
    }

    [DXamlIdlGroup("Controls2")]
    public delegate void SectionsInViewChangedEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Controls.SectionsInViewChangedEventArgs e);

}
