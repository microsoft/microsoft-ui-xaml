// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;

namespace XamlGen.Templates.Code.Framework.Headers
{
    public abstract class ForwarderMemberModel<T> where T : MemberDefinition
    {
        public T Member
        {
            get;
            protected set;
        }

        protected ForwarderRequestedInterface ForwarderRequestedInterface
        {
            get;
            set;
        }

        protected ForwarderMemberModel()
        {
        }
    }
}
