// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using Microsoft.UI.Xaml.Markup;
using XamlOM;

namespace Microsoft.UI.Xaml
{
    // Experimental. Conditional setters: a Trigger applies its Setters while its condition holds.
    // See docs\api-specs\Triggers\Trigger-spec.md.

    [Comment("A collection of Condition objects.")]
    [Platform("Feature_ExperimentalApi", typeof(Microsoft.UI.Xaml.WinUIContract), Microsoft.UI.Xaml.WinUIContract.Experimental)]
    [ClassFlags(HasBaseTypeInDXamlInterface = false)]
    [NativeName("CConditionCollection")]
    [Guids(ClassGuid = "747452c0-7236-4905-a9e4-28ba815dd81a")]
    public sealed class ConditionCollection
     : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<Condition>
    {
        [NativeStorageType(OM.ValueType.valueObject)]
        public Microsoft.UI.Xaml.Condition ContentProperty
        {
            get;
            set;
        }

        public ConditionCollection() { }
    }

    [Comment("Describes a property value that must hold for a MultiTrigger to apply its setters.")]
    [Platform("Feature_ExperimentalApi", typeof(Microsoft.UI.Xaml.WinUIContract), Microsoft.UI.Xaml.WinUIContract.Experimental)]
    [NativeName("CCondition")]
    [Guids(ClassGuid = "133b14e0-450f-431d-b4e8-70f3fb17c91e")]
    public sealed class Condition
     : Microsoft.UI.Xaml.DependencyObject
    {
        [Comment("The name of the property to test, for example IsSelected or RowContext.PreviousRelation.")]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueString)]
        [OffsetFieldName("m_strProperty")]
        public Windows.Foundation.String Property
        {
            get;
            set;
        }

        [Comment("The value the property must equal.")]
        [OffsetFieldName("m_vValue")]
        public Windows.Foundation.Object Value
        {
            get;
            set;
        }

        public Condition() { }
    }

    [Comment("Applies its setters while a single property has a given value.")]
    [Platform("Feature_ExperimentalApi", typeof(Microsoft.UI.Xaml.WinUIContract), Microsoft.UI.Xaml.WinUIContract.Experimental)]
    [NativeName("CTrigger")]
    [ContentProperty("Setters")]
    [Guids(ClassGuid = "644b7497-20c2-4c5f-9af4-a096c044c2d0")]
    public sealed class Trigger
     : Microsoft.UI.Xaml.TriggerBase
    {
        [Comment("The name of the property to test, for example IsSelected or RowContext.PreviousRelation.")]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueString)]
        [OffsetFieldName("m_strProperty")]
        public Windows.Foundation.String Property
        {
            get;
            set;
        }

        [Comment("The value the property must equal.")]
        [OffsetFieldName("m_vValue")]
        public Windows.Foundation.Object Value
        {
            get;
            set;
        }

        [Comment("The setters applied while the condition holds.")]
        [PropertyFlags(IsReadOnlyExceptForParser = true, IsValueCreatedOnDemand = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_pSetters")]
        public Microsoft.UI.Xaml.SetterBaseCollection Setters
        {
            get;
            set;
        }

        public Trigger() { }
    }

    [Comment("Applies its setters while all of its conditions hold.")]
    [Platform("Feature_ExperimentalApi", typeof(Microsoft.UI.Xaml.WinUIContract), Microsoft.UI.Xaml.WinUIContract.Experimental)]
    [NativeName("CMultiTrigger")]
    [ContentProperty("Setters")]
    [Guids(ClassGuid = "e00f0d4d-7ab2-47b2-a496-991fe67a287d")]
    public sealed class MultiTrigger
     : Microsoft.UI.Xaml.TriggerBase
    {
        [Comment("The conditions that must all hold.")]
        [PropertyFlags(IsReadOnlyExceptForParser = true, IsValueCreatedOnDemand = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_pConditions")]
        public Microsoft.UI.Xaml.ConditionCollection Conditions
        {
            get;
            set;
        }

        [Comment("The setters applied while all conditions hold.")]
        [PropertyFlags(IsReadOnlyExceptForParser = true, IsValueCreatedOnDemand = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_pSetters")]
        public Microsoft.UI.Xaml.SetterBaseCollection Setters
        {
            get;
            set;
        }

        public MultiTrigger() { }
    }
}
