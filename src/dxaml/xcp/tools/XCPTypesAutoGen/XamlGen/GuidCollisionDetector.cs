// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using OM.Visitors;
using System;
using System.Linq;
using System.Collections.Generic;

namespace XamlGen
{
    class GuidCollisionDetector: OMVisitor
    {
        private HashSet<Guid> _guids = new HashSet<Guid>();
        private HashSet<TypeDefinition> _types = new HashSet<TypeDefinition>();

        private GuidCollisionDetector(OMContextView view)
            : base(view)
        {
        }

        internal static void Check(OMContext context)
        {
            new GuidCollisionDetector(context.GetView()).Run();
        }

        protected override void VisitType(TypeDefinition type)
        {
            // Exclude some types for now because they're violating the validation and there's 
            // no easy way to resolve those conflicts.
            if (type.FullName.Equals($"{Helper.GetRootNamespace()}.Controls.AutoSuggestBoxSuggestionChosenEventArgs", StringComparison.OrdinalIgnoreCase))
            {
                return;
            }

            base.VisitType(type);

            // We're deliberately not checking IReferenceGuids right now because it's intentionally re-using existing guids.
            IEnumerable<Guid> allGuids = type.Guids.FactoryGuids.Values
                .Concat(type.Guids.InterfaceGuids.Values)
                .Concat(type.Guids.OverrideGuids.Values)
                .Concat(type.Guids.ProtectedGuids.Values)
                .Concat(type.Guids.StaticsGuids.Values);

            foreach (Guid g in allGuids.Where(g => g != Guid.Empty))
            {
                if (!_guids.Add(g))
                {
                    throw new InvalidOperationException(string.Format("Type '{0}' has a GUID collision. {1} is not unique!", type.FullName, g));
                }
            }

            if (type.Guids.ClassGuid != Guid.Empty && !_guids.Add(type.Guids.ClassGuid))
            {
                throw new InvalidOperationException(string.Format("Type '{0}' has a GUID collision. {1} is not unique!", type.FullName, type.Guids.ClassGuid));
            }
        }
    }
}
