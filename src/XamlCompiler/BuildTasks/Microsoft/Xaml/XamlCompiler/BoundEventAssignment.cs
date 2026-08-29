// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    using System.Reflection;
    using System.Collections.Generic;
    using CodeGen;
    using Properties;    
    using Utilities;
    using XamlDom;

    internal class BoundEventAssignment : BindAssignmentBase
    {
        private IEnumerable<Parameter> parameters;

        public IEnumerable<Parameter> Parameters
        {
            get
            {
                if (parameters == null)
                {
                    parameters = MemberType.UnderlyingType.GetInvokeParameters();
                }
                return parameters;
            }
        }

        public string EventHandlerCodeName { get { return ConnectionIdElement.ObjectCodeName + MemberName; } }

        public BoundEventAssignment(XamlDomMember bindMember, BindUniverse bindUniverse, ConnectionIdElement connectionIdElement)
            : base(bindMember, bindUniverse, connectionIdElement)
        {
        }

        public IEnumerable<XamlCompileErrorBase> ParsePath()
        {
            var issues = new List<XamlCompileErrorBase>();
            try
            {
                var warnings = new List<string>();
                this.PathStep = ParseBindPath(warnings);
                foreach (var warning in warnings)
                {
                    issues.Add(new XamlXBindParseWarning(bindItem, warning));
                }
            }
            catch (CompiledBindingParseException ex)
            {
                issues.Add(new XamlXBindParseError(this.bindItem, ex));
                return issues;
            }

            if (!ValidateEvent(issues, this.PathStep))
            {
                return issues;
            }

            // Success code path
            return issues;
        }

        private bool ValidateEvent(IList<XamlCompileErrorBase> issues, BindPathStep leafStep)
        {
            if (leafStep.Parent != null)
            {
                foreach (BindPathStep step in leafStep.Parent.ParentsAndSelf)
                {
                    if (step is MethodStep)
                    {
                        issues.Add(new BindAssignmentValidationError(this.bindItem, ResourceUtilities.FormatString(
                            XamlCompilerResources.BoundEventAssignment_NonLeafMethod, ((MethodStep)step).MethodName)));
                        return false;
                    }
                }
            }
            if (leafStep is PropertyStep || leafStep is RootNamedElementStep)
            {
                if (leafStep.ValueType.IsDelegate())
                {
                    if (leafStep.ValueType.Name == this.MemberType.Name) return true;
                }
                issues.Add(new BindAssignmentValidationError(this.bindItem, ResourceUtilities.FormatString(
                    XamlCompilerResources.BoundEventAssignment_NonDelegateProperty, this.MemberName, this.MemberType.Name)));
                return false;
            }
            else if (leafStep is MethodStep)
            {
                MethodStep methodStep = leafStep as MethodStep;
                if (methodStep.IsOverloaded)
                {
                    issues.Add(new BindAssignmentValidationError(this.bindItem, ResourceUtilities.FormatString(
                        XamlCompilerResources.BoundEventAssignment_NoOverloads)));
                    return false;
                }
                MethodInfo invokeMethod = this.MemberType.UnderlyingType.GetMethod("Invoke");
                var invokeArgs = invokeMethod.GetParameters();

                if (methodStep.Parameters.Count != 0 && methodStep.Parameters.Count != invokeArgs.Length)
                {
                    issues.Add(new BindAssignmentValidationError(this.bindItem, ResourceUtilities.FormatString(
                        XamlCompilerResources.BoundEventAssignment_InvalidSignature, this.MemberName)));
                    return false;
                }
                for (int i = 0; i < methodStep.Parameters.Count; i++)
                {
                    if (invokeArgs[i].ParameterType != methodStep.Parameters[i].ParameterType && !invokeArgs[i].ParameterType.IsSubclassOf(methodStep.Parameters[i].ParameterType))
                    {
                        issues.Add(new BindAssignmentValidationError(this.bindItem, ResourceUtilities.FormatString(
                            XamlCompilerResources.BoundEventAssignment_SignatureMismatch, 
                            this.MemberName, i, methodStep.Parameters[i].ParameterType.Name, invokeArgs[i].ParameterType.Name)));
                        return false;
                    }
                }
            }
            return true;
        }
    }
}
