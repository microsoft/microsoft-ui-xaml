// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;

namespace OM.Visitors
{
    internal class IdlImportVisitor : OMVisitor
    {
        private HashSet<string> _imports = new HashSet<string>();

        public IdlImportVisitor(OMContextView view)
            : base(view)
        {
        }

        internal static IEnumerable<string> GetImports(OMContextView view)
        {
            IdlImportVisitor visitor = new IdlImportVisitor(view);
            visitor.Run();
            return visitor._imports;
        }

        protected override void VisitType(TypeDefinition type)
        {
            if (type.IsImported || type.IdlTypeInfo.IsImported)
            {
                if (!string.IsNullOrEmpty(type.IdlTypeInfo.FileName) && !type.IdlTypeInfo.IsExcluded)
                {
                    _imports.Add(type.IdlTypeInfo.FileName);
                }

                // Don't walk imported types.
                return;
            }

            base.VisitType(type);
        }
    }
}
