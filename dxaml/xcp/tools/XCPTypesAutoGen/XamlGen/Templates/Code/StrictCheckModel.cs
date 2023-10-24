// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System.Collections.Generic;


namespace XamlGen.Templates.Code
{
    public class StrictCheckModel
    {
        public OM.MemberDefinition MemberDefinition
        {
            get;
            private set;
        }

        public bool ForceIndent
        {
            get;
            private set;
        }

        public bool UseIfcReturn
        {
            get;
            private set;
        }

        private StrictCheckModel()
        {
        }

        public static StrictCheckModel Create(OM.MemberDefinition memberDefinition)
        {
            return new StrictCheckModel() { MemberDefinition = memberDefinition };
        }

        public static StrictCheckModel Create(OM.MemberDefinition memberDefinition, bool forceIndent)
        {
            return new StrictCheckModel() { MemberDefinition = memberDefinition, ForceIndent = forceIndent };
        }

        public static StrictCheckModel Create(OM.MemberDefinition memberDefinition, bool forceIndent, bool useIfcReturn)
        {
            return new StrictCheckModel() { MemberDefinition = memberDefinition, ForceIndent = forceIndent, UseIfcReturn = useIfcReturn };
        }
    }
}
