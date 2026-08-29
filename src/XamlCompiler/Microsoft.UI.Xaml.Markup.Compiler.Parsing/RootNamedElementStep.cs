// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Xaml;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    /// <summary>
    /// RootNamedElementStep represents a bind path step that is the first field or named element in the path.
    /// This can either refer to one of:
    ///     1. A field on the file root
    ///     2. A named element on the file root (note: this is equivalent to 1 as we
    ///        codegen a field for all named elements in the file root namescope)
    ///     3. A field on the template root
    ///     4. A named element within a template
    /// </summary>
    internal class RootNamedElementStep : BindPathStep
    {
        public string FieldName { get; private set; }

        public string UpdateCallParamOverride { get; private set; }

        public RootNamedElementStep(string fieldName, XamlType fieldType, BindPathStep parent, ApiInformation apiInformation, string updateParamOverride)
            : this(fieldName, fieldType, parent, apiInformation)
        {
            UpdateCallParamOverride = updateParamOverride;
        }

        public RootNamedElementStep(string fieldName, XamlType fieldType, BindPathStep parent, ApiInformation apiInformation)
            : base(fieldType, parent, apiInformation)
        {
            FieldName = fieldName;
        }

        public override string UniqueName => FieldName;
    }
}
