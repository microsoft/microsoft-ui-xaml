// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace OM
{
    public class AttachedEventDefinition : EventDefinition
    {
        public override string AddName
        {
            get
            {
                return "Add" + IdlEventInfo.Name + "Handler";
            }
        }
        public override string RemoveName
        {
            get
            {
                return "Remove" + IdlEventInfo.Name + "Handler";
            }
        }

        public TypeReference TargetType
        {
            get;
            set;
        }

        private static TypeReference VoidType = new TypeReference(null) { IsVoid = true };

        public override MethodDefinition GetAddMethod()
        {
            MethodDefinition addMethod = new MethodDefinition();
            Helper.ShallowCopyProperties<MemberDefinition>(this, addMethod);
            Helper.ShallowCopyProperties(IdlMemberInfo, addMethod.IdlMemberInfo);
            addMethod.Name = AddName;
            addMethod.IdlMethodInfo.Name = AddName;
            addMethod.Parameters.Add(new ParameterDefinition() { ParameterType = TargetType });
            addMethod.Parameters.Add(new ParameterDefinition() { ParameterType = EventHandlerType });
            addMethod.IsImplVirtual = XamlEventFlags.IsImplVirtual;
            addMethod.ReturnType = VoidType;
            return addMethod;
        }

        public override MethodDefinition GetRemoveMethod()
        {
            MethodDefinition removeMethod = new MethodDefinition();
            Helper.ShallowCopyProperties<MemberDefinition>(this, removeMethod);
            Helper.ShallowCopyProperties(IdlMemberInfo, removeMethod.IdlMemberInfo);
            removeMethod.Name = RemoveName;
            removeMethod.IdlMethodInfo.Name = RemoveName;
            removeMethod.IsImplVirtual = XamlEventFlags.IsImplVirtual;
            removeMethod.Parameters.Add(new ParameterDefinition() { ParameterType = TargetType });
            removeMethod.Parameters.Add(new ParameterDefinition() { ParameterType = EventHandlerType });
            removeMethod.ReturnType = VoidType;
            return removeMethod;
        }
    }
}
