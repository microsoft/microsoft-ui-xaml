// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    using System;
    using System.Collections.Generic;
    using System.Reflection;
    using System.Text;
    using CodeGen;
    using DirectUI;
    using XamlDom;

    internal class EventAssignment : ILineNumberAndErrorInfo
    {
        private IEnumerable<Parameter> parameters;
        public ApiInformation ApiInformation { get; }

        public EventAssignment(XamlDomMember domMember)
        {
            this.ApiInformation = domMember.ApiInformation ?? domMember.Parent.ApiInformation;

            this.LineNumberInfo = new LineNumberInfo(domMember);
            this.EventName = domMember.Member.Name;

            this.HandlerName = (domMember.Item as XamlDomValue).Value as string;
            this.Event = domMember.Member as DirectUIXamlMember;

            this.EventParamsForCppSignature = this.LookupEventParamsForCppSignature();

            this.EventType = new TypeForCodeGen(domMember.Member.Type);
            this.DeclaringType = new TypeForCodeGen(domMember.Member.DeclaringType);
        }

        // these types can be null in pass1 if it is a 'local' type.
        // only for use in pass2.
        public TypeForCodeGen EventType { get; set; }
        public TypeForCodeGen DeclaringType { get; set; }
        public string HandlerName { get; set; }
        public string EventParamsForCppSignature { get; set; }
        public DirectUIXamlMember Event { get; set; }
        public LineNumberInfo LineNumberInfo { get; set; }
        public string EventName { get; set; }

        internal string LookupEventParamsForCppSignature()
        {
            Type d = this.Event.Type.UnderlyingType;
            if (d.BaseType.FullName != typeof(MulticastDelegate).FullName)
            {
                return null;
            }

            MethodInfo handler = d.GetMethod(KnownMembers.Invoke);
            if (handler == null)
            {
                return null;
            }

            ParameterInfo[] handlerParameters = handler.GetParameters();
            if (handlerParameters == null)
            {
                return null;
            }

            StringBuilder signature = new StringBuilder("(");
            bool first = true;
            foreach (ParameterInfo param in handlerParameters)
            {
                string typeName = null;
                Type eventArgType = this.Event.DeclaringType.SchemaContext.GetXamlType(param.ParameterType).UnderlyingType;

                if (!first)
                {
                    signature.Append(", ");
                }
                else
                {
                    first = false;
                }

                if (XamlSchemaCodeInfo.GetGlobalizedFullNameForCppRefType(eventArgType, out typeName))
                {
                    typeName = string.Concat(typeName, "^*");
                    signature.Append(typeName);
                    continue;
                }

                typeName = XamlSchemaCodeInfo.GetFullGenericNestedName(eventArgType, ProgrammingLanguage.CppCX, true);

                if (eventArgType.IsArray)
                {
                    if (param.IsIn)
                    {
                        typeName = string.Concat("const ::Platform::Array<", typeName.Substring(0, typeName.Length - 2), ">");
                    }
                    else if (param.IsOut)
                    {
                        typeName = string.Concat("::Platform::WriteOnlyArray<", typeName.Substring(0, typeName.Length - 2), ">");
                    }
                }

                signature.Append(typeName);

                if (!eventArgType.IsValueType)
                {
                    signature.Append("^");
                }
            }

            signature.Append(")");

            return signature.ToString();
        }

        public XamlCompileError GetAttributeProcessingError()
        {
            return new XamlRewriterErrorEventLongForm(this.LineNumberInfo.StartLineNumber, this.LineNumberInfo.StartLinePosition);
        }

        public IEnumerable<Parameter> Parameters
        {
            get
            {
                if (parameters == null)
                {
                    parameters = EventType.UnderlyingType.TryGetInvokeParameters();
                }
                return parameters;
            }
        }
    }
}