// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using System;
using Microsoft.UI.Xaml.Markup;
using XamlOM;

namespace Microsoft.UI.Xaml
{
    [CodeGen(partial: true)]
    [ClassFlags(HasBaseTypeInDXamlInterface = false)]
    [NativeName("CBrushTransition")]
    [Guids(ClassGuid = "8711bf52-367e-4ae7-adf2-3c1e4087ee89")]
    public class BrushTransition
        : DependencyObject
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public BrushTransition() {}

        [SimpleProperty(SimplePropertyStorage.Sparse, DefaultValue = "{ 1500000L }")]
        public Windows.Foundation.TimeSpan Duration
        {
            get;
            set;
        }
    }

    [EnumFlags(HasTypeConverter = true, AreValuesFlags = true)]
    [SimpleType(OM.SimpleTypeKind.Value)]
    public enum Vector3TransitionComponents
    {
        X = 1,
        Y = 2,
        Z = 4,
    }

    [CodeGen(partial: true)]
    [ClassFlags(HasBaseTypeInDXamlInterface = false)]
    [NativeName("CVector3Transition")]
    [Guids(ClassGuid = "a0eaff78-f11e-4ad0-8560-810f4a47786f")]
    public class Vector3Transition
        : DependencyObject
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Vector3Transition() {}

        [SimpleProperty(SimplePropertyStorage.Sparse, DefaultValue = "{ 3000000L }")]
        public Windows.Foundation.TimeSpan Duration
        {
            get;
            set;
        }

        [SimpleProperty(SimplePropertyStorage.Sparse, DefaultValue = "ABI::Microsoft::UI::Xaml::Vector3TransitionComponents::Vector3TransitionComponents_X | ABI::Microsoft::UI::Xaml::Vector3TransitionComponents::Vector3TransitionComponents_Y | ABI::Microsoft::UI::Xaml::Vector3TransitionComponents::Vector3TransitionComponents_Z")]
        public Vector3TransitionComponents Components
        {
            get;
            set;
        }
    }

    [CodeGen(partial: true)]
    [ClassFlags(HasBaseTypeInDXamlInterface = false)]
    [NativeName("CScalarTransition")]
    [Guids(ClassGuid = "1c025b3d-11ae-466e-8d95-587103dc1a8e")]
    public partial class ScalarTransition
        : DependencyObject
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public ScalarTransition() {}

        [SimpleProperty(SimplePropertyStorage.Sparse, DefaultValue = "{ 3000000L }")]
        public Windows.Foundation.TimeSpan Duration
        {
            get;
            set;
        }
    }
}
