// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Text;

namespace OM
{
    public class EnumVersion : VersionedElement
    {
        public EnumDefinition ActualEnum
        {
            get;
            private set;
        }

        public EnumVersion(EnumDefinition actualEnum, int version)
        {
            ActualEnum = actualEnum;
            Version = version;
        }

        public EnumDefinition GetProjection()
        {
            EnumDefinition projection = new EnumDefinition();
            ShallowCopyProperties(ActualEnum, projection);
            projection.IsVersionProjection = true;
            projection.Version = Version;
            projection.Values.AddRange(ActualEnum.Values.Where(c => c.Version == Version));
            projection.SupportedContracts.AddRange(SupportedContracts);
            return projection;
        }

        private void ShallowCopyProperties(EnumDefinition from, EnumDefinition to)
        {
            foreach (PropertyInfo property in typeof(ClassDefinition).GetProperties().Where(p => p.CanRead && p.CanWrite))
            {
                object value = property.GetValue(from, null);
                property.SetValue(to, value, null);
            }
        }
    }
}
