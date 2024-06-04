// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Linq;

namespace Microsoft.UI.Xaml.Markup.Compiler.CodeGen
{
    internal class Definition
    {
        private IEnumerable<TypeGenInfo> _typeInfos;

        public XamlProjectInfo ProjectInfo { get; }
        public  XamlSchemaCodeInfo SchemaInfo { get; }

        public Definition(XamlProjectInfo projectInfo, XamlSchemaCodeInfo schemaInfo)
        {
            this.ProjectInfo = projectInfo;
            this.SchemaInfo = schemaInfo;
        }

        public IEnumerable<TypeGenInfo> TypeInfos
        {
            get
            {
                if (_typeInfos == null)
                {
                    // Add Types
                    Dictionary<String, TypeGenInfo> entries = new Dictionary<String, TypeGenInfo>();
                    foreach (var entry in SchemaInfo.TypeTable)
                    {
                        entries.Add(entry.StandardName, new TypeGenInfo(entry, ProjectInfo.GenerateIncrementalTypeInfo));
                    }
                    foreach (var entry in SchemaInfo.UserTypeInfo)
                    {
                        entries[entry.StandardName].UserTypeInfo = entry;
                    }

                    // Order the data according to the type length so it can be quicly accessed at runtime
                    _typeInfos = entries.Values.OrderBy(x => x.StandardName.Length);
                }
                return _typeInfos;
            }
        }

    }
}
