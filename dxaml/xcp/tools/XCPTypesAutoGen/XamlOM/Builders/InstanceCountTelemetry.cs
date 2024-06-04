// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Reflection;

namespace XamlOM
{
    /// <summary>
    /// Specifies whether to send instance count telemetry.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class, Inherited = false)]
    public class InstanceCountTelemetryAttribute :
        Attribute,
        NewBuilders.IClassBuilder
    {
        public void BuildNewClass(OM.ClassDefinition definition, Type source)
        {
            definition.InstanceCountTelemetry = true;
        }
    }
}
