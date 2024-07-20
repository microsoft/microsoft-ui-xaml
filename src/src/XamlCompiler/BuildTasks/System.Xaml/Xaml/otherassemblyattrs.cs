// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//
// This file specifies various assembly level attributes.
//

using System.Runtime.CompilerServices;
using System.Security;
using System.Security.Permissions;

[assembly:DependencyAttribute("mscorlib,", LoadHint.Always)]
[assembly:DependencyAttribute("System,", LoadHint.Always)]
[assembly:DependencyAttribute("System.Xml,", LoadHint.Sometimes)]

#if TARGETTING35SP1
[assembly:SecurityCritical] //needed to run critical code
#endif
//Specify the minimum security permissions needed for System.Xaml
[assembly: SecurityPermission(SecurityAction.RequestMinimum, Execution = true)]

[assembly:System.Windows.Markup.XmlnsDefinition("http://schemas.microsoft.com/winfx/2006/xaml", "System.Windows.Markup")]
