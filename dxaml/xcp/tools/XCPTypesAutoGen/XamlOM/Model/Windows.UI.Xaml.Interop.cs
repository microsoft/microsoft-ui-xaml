// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using XamlOM;

namespace Windows.UI.Xaml.Interop
{
    [LiftedOptions(ExcludeFromLiftedCodegen = true)]
    [FrameworkTypePattern]
    public enum TypeKind
    {
        Primitive = 0,

        Metadata = 1,

        Custom = 2
    }

    [LiftedOptions(ExcludeFromLiftedCodegen = true)]
    [CustomNames(PrimitiveCoreName = "KnownTypeIndex")]
    [NativeName("CType")]
    [TypeFlags(IsCreateableFromXAML = false)]
    [ClassFlags(HasTypeConverter = true)]
    public struct TypeName
    {
        public Windows.Foundation.String Name { get; set; }

        public TypeKind Kind { get; set; }
    }
}

