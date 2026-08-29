// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Xaml;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    using CodeGen;
    using Properties;
    using Utilities;
    using XamlDom;

    internal class BindAssignmentBase : ILineNumberAndErrorInfo, IXamlTypeResolver
    {
        protected XamlDomObject bindItem;
        protected XamlDomMember bindMember;

        public ApiInformation ApiInformation { get; }
        public BindUniverse BindUniverse { get; }
        public ConnectionIdElement ConnectionIdElement { get; }
        public BindPathStep PathStep { get; protected set; }
        public LineNumberInfo LineNumberInfo { get; }

        public BindAssignmentBase(XamlDomMember domMember, BindUniverse bindUniverse, ConnectionIdElement connectionIdElement)
        {
            bindMember = domMember;

            ApiInformation = domMember.ApiInformation ?? domMember.Parent.ApiInformation;
            LineNumberInfo = new LineNumberInfo(domMember);
            bindItem = domMember.Item as XamlDomObject;
            BindUniverse = bindUniverse;
            ConnectionIdElement = connectionIdElement;

            // We now need to always keep the root of a template around, so we can:
            // - unhook DataContextChanged
            // - call FindName on it for x:Load
            if (!bindUniverse.IsFileRoot &&
                !bindUniverse.BoundElements.Contains(bindUniverse.RootElement))
            {
                bindUniverse.BoundElements.Add(bindUniverse.RootElement);
            }
            if (!bindUniverse.BoundElements.Contains(connectionIdElement))
            {
                bindUniverse.BoundElements.Add(connectionIdElement);
            }
        }

        public XamlCompileError GetAttributeProcessingError()
        {
            return new XamlRewriterErrorCompiledBindingLongForm(
                LineNumberInfo.StartLineNumber,
                LineNumberInfo.StartLinePosition);
        }

        public virtual LanguageSpecificString ObjectDeferredAssignmentCodeName => new LanguageSpecificString(() => string.Format("{0}{1}DeferredValue", ConnectionIdElement.ObjectCodeName, MemberName));

        public string MemberName => bindMember.Member.Name;

        public virtual string MemberFullName => string.Format("{0}.{1}", MemberDeclaringType.UnderlyingType.FullName, MemberName);

        public virtual XamlType MemberType => bindMember.Member.Type;

        public virtual XamlType MemberDeclaringType => bindMember.Member.DeclaringType;

        public virtual bool IsAttachable => bindMember.Member.IsAttachable;

        public virtual bool IsInputPropertyAssignment
        {
            get
            {
                var contextProperty = bindMember.Parent.Type.TryGetInputPropertyName();
                return contextProperty == MemberName;
            }
        }

        public virtual bool HasSetValueHelper => true;

        public virtual bool HasDeferredValueProxy => true;

        public virtual int LineNumber => bindMember.StartLineNumber;

        public virtual int ColumnNumber => bindMember.StartLinePosition;

        public BindPathStep ParseBindPath(IList<string> warnings)
        {
            var path = GetBindingPath(bindItem);
            return ParseBindPath(path, warnings);
        }

        public BindPathStep ParseBindPath(string path, IList<string> warnings)
        {
            // Text="{x:Bind}" - short circuit empty paths
            if (path.Length == 0)
            {
                return BindUniverse.RootStep;
            }
            // for x:Bind, the contract is to use a conditional on the 
            // member assegnment - directly - if any of the types inside
            // the x:Bind expression are conditional.
            var memberApiInformation = bindItem.Parent.ApiInformation;
            try
            {
                return BindPathStep.Parse(path, memberApiInformation, BindUniverse, this, warnings);
            }
            catch (ParseException e)
            {
                throw new CompiledBindingParseException(
                    path,
                    e.Message,
                    ColumnNumber);
            }
        }

        protected static string GetBindingPath(XamlDomObject bindItem)
        {
            string implicitPath = GetImplicitPath(bindItem);
            string explicitPath = DomHelper.GetStringValueOfProperty(bindItem.GetMemberNode(KnownStrings.Path));

            // it's not valid for the path to be set twice via implicit and
            // explicit assignment like "{x:Bind Username, Path=MyViewModel}"
            if (!string.IsNullOrEmpty(implicitPath) && !string.IsNullOrEmpty(explicitPath))
            {
                throw new CompiledBindingParseException(
                    explicitPath,
                    ResourceUtilities.FormatString(XamlCompilerResources.BindPathParser_PathSetTwice),
                    bindItem.StartLinePosition);
            }

            return !string.IsNullOrEmpty(explicitPath) ? explicitPath
                 : !string.IsNullOrEmpty(implicitPath) ? implicitPath
                 : string.Empty;
        }

        private static string GetImplicitPath(XamlDomObject bindItem)
        {
            // example: {x:Bind MyViewModel.Username}
            XamlDomMember pparams = bindItem.GetMemberNode(XamlLanguage.PositionalParameters);
            if (pparams != null)
            {
                if (pparams.Items.Count == 0)
                {
                    return string.Empty;
                }
                else if (pparams.Items.Count == 1)
                {
                    XamlDomValue keyValue = pparams.Items[0] as XamlDomValue;
                    if (keyValue != null)
                    {
                        return keyValue.Value as string;
                    }
                }
                else if (pparams.Items.Count > 1)
                {
                    throw new CompiledBindingParseException(
                        string.Empty,
                        ResourceUtilities.FormatString(XamlCompilerResources.BindAssignment_InvalidPropertyPathSyntax),
                        bindItem.StartLinePosition);
                }
            }
            return null;
        }

        public XamlType ResolveXmlName(string name)
        {
            return bindItem.ResolveXmlName(name);
        }

        public XamlType ResolveType(Type type)
        {
            return bindItem.SchemaContext.GetXamlType(type);
        }

        public bool CanAssignDirectlyTo(XamlType source, XamlType destination)
        {
            return source.CanAssignDirectlyTo(destination);
        }
        public bool CanInlineConvert(XamlType source, XamlType destination)
        {
            return source.CanInlineConvert(destination);
        }
    }
}
