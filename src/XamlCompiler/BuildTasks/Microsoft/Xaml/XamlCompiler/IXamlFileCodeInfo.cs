// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    using System.Collections.Generic;

    internal interface IXamlFileCodeInfo
    {
        string ApparentRelativePath { get; set; }
        BindStatus BindStatus { get; set; }
        List<ConnectionIdElement> ConnectionIdElements { get; }
        string FullPathToXamlFile { get; set; }
        bool HasEventAssignments { get; set; }
        string RelativePathFromGeneratedCodeToXamlFile { get; set; }
        string SourceXamlGivenPath { get; set; }
        List<DataTypeAssignment> DataTypeAssignments { get; }
        bool HasPhaseAssignments { get; set; }
        List<StrippableMember> StrippableMembers {get; }
        List<StrippableObject> StrippableObjects {get; }
        List<StrippableNamespace> StrippableNamespaces { get; }
        xPropertyInfo XPropertyInfo { get; set; }
    }
}