// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace OM
{
    public class AttachedPropertyDefinition : DependencyPropertyDefinition
    {
        /// <summary>
        /// Returns PropertyDefinition.DeclaringType for normal properties, and AttachedPropertyDefinition.TargetType for attached properties.
        /// </summary>
        public override TypeReference EffectiveTargetType
        {
            get
            {
                return TargetType;
            }
        }

        public override string GetterName
        {
            get
            {
                return "Get" + Name + "Static";
            }
        }

        /// <summary>
        /// Gets whether this attached property generates an instance field on the target type.
        /// </summary>
        public bool HasAttachedField
        {
            get
            {
                return !string.IsNullOrEmpty(FieldName);
            }
        }

        public override string SetterName
        {
            get
            {
                return "Set" + Name + "Static";
            }
        }

        public Idl.IdlAttachedPropertyInfo IdlAPInfo
        {
            get
            {
                return (Idl.IdlAttachedPropertyInfo)IdlPropertyInfo;
            }
        }

        public override string FieldClassName
        {
            get
            {
                return TargetType.Type.CoreName;
            }
        }

        public TypeReference TargetType
        {
            get;
            set;
        }

        protected override Idl.IdlPropertyInfo CreateIdlPropertyInfo()
        {
            return new Idl.IdlAttachedPropertyInfo(this);
        }
    }
}
