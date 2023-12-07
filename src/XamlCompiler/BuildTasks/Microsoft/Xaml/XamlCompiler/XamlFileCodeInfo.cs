// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    using System.Collections.Generic;

    internal class XamlFileCodeInfo : IXamlFileCodeInfo
    {
        private List<ConnectionIdElement> connectionIdElements = new List<ConnectionIdElement>();
        private List<DataTypeAssignment> dataTypeAssignments = new List<DataTypeAssignment>();
        private List<StrippableMember> strippableMembers = new List<StrippableMember>();
        private List<StrippableObject> strippableObjects = new List<StrippableObject>();
        private List<StrippableNamespace> strippableNamespaces = new List<StrippableNamespace>();

        public XamlFileCodeInfo()
        {
            this.HasEventAssignments = false;
            this.BindStatus = BindStatus.None;
        }

        public string SourceXamlGivenPath { get; set; }
        public string XamlOutputFilename { get; set; }
        public bool HasEventAssignments { get; set; }
        public BindStatus BindStatus { get; set; }
        public xPropertyInfo XPropertyInfo { get; set; }
        public List<ConnectionIdElement> ConnectionIdElements { get { return connectionIdElements; } }
        public List<DataTypeAssignment> DataTypeAssignments { get { return dataTypeAssignments; } }
        public List<StrippableMember> StrippableMembers { get { return strippableMembers; } }
        public List<StrippableObject> StrippableObjects { get { return strippableObjects; } }
        public List<StrippableNamespace> StrippableNamespaces { get { return strippableNamespaces; } }

        // Fullpath is used for the #pragma checksum()
        public string FullPathToXamlFile { get; set; }

        // Used to make the LoadComponentURI  (apparent because of <Link> data)
        public string ApparentRelativePath { get; set; }

        // "Relative Path from Code to XAML" is used for the #line pragmas.
        public string RelativePathFromGeneratedCodeToXamlFile { get; set; }

        public bool HasPhaseAssignments { get; set; }
    }
}
