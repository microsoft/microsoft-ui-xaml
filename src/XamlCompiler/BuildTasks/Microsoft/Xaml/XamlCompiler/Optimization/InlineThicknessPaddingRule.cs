// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler.Optimization
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Linq;
    using System.Text.RegularExpressions;
    using System.Xaml;
    using DirectUI;
    using XamlDom;

    internal sealed class InlineThicknessPaddingRule : IXamlOptimizationRule
    {
        private const string RuleId = "InlineThicknessPadding";
        private static readonly char[] XmlWhitespace = { ' ', '\t', '\r', '\n' };
        private static readonly Regex Number = new Regex(
            @"\A[+-]?(?:[0-9]+(?:\.[0-9]*)?|\.[0-9]+)(?:[eE][+-]?[0-9]+)?\z",
            RegexOptions.CultureInvariant);

        public XamlOptimizationResult Analyze(XamlOptimizationContext context)
        {
            var edits = new List<XamlOptimizationEdit>();
            var decisions = new List<XamlOptimizationDecision>();
            var scopeCache = new Dictionary<XamlDomObject, bool>();
            var schema = context.Root.SchemaContext as DirectUISchemaContext;
            var borderType = schema?.DirectUISystem.DirectUISystemGetType("Microsoft.UI.Xaml.Controls.Border", false);
            var thicknessType = schema?.DirectUISystem.DirectUISystemGetType("Microsoft.UI.Xaml.Thickness", false);

            foreach (var node in new XamlDomIterator(context.Root).DescendantsAndSelf())
            {
                if (node.Type?.Name != "Thickness")
                {
                    continue;
                }

                var member = node.Parent;
                var owner = member?.Parent;
                XamlOptimizationReason reason;
                if (borderType == null || thicknessType == null || node.Type.UnderlyingType != thicknessType ||
                    owner?.Type.UnderlyingType != borderType || owner.IsGetObject ||
                    member.Member.IsAttachable || member.Member.Name != "Padding" ||
                    member.Member.DeclaringType?.UnderlyingType != borderType ||
                    member.Items.Count != 1)
                {
                    reason = XamlOptimizationReason.UnsupportedTypeOrProperty;
                }
                else if (node.IsGetObject || node.Namespaces.Count != 0 ||
                    node.MemberNodes.Count != 1 || node.MemberNodes[0].Member != XamlLanguage.Initialization ||
                    node.MemberNodes[0].Items.Count != 1 || !(node.MemberNodes[0].Item is XamlDomValue))
                {
                    reason = XamlOptimizationReason.DecoratedValue;
                }
                // The exact built-in value type is already proven above. Its managed
                // projection can require codegen without being a custom type.
                else if (node.ApiInformation != null || HasUnsupportedScope(owner, scopeCache))
                {
                    reason = XamlOptimizationReason.UnsupportedScope;
                }
                else if (!IsLiteral((node.MemberNodes[0].Item as XamlDomValue).Value as string))
                {
                    reason = XamlOptimizationReason.UnsupportedLiteral;
                }
                else
                {
                    reason = AnalyzeSource(context, node, edits);
                }

                decisions.Add(new XamlOptimizationDecision(RuleId, node, reason));
            }

            return new XamlOptimizationResult(edits, decisions);
        }

        private static bool HasUnsupportedScope(XamlDomObject value, Dictionary<XamlDomObject, bool> cache)
        {
            var pending = new Stack<XamlDomObject>();
            var node = value;
            bool unsupported = false;
            while (node != null && !cache.TryGetValue(node, out unsupported))
            {
                pending.Push(node);
                node = node.Parent?.Parent;
            }

            while (pending.Count != 0)
            {
                node = pending.Pop();
                unsupported = unsupported || HasUnsupportedLocalScope(node);
                cache.Add(node, unsupported);
            }
            return cache[value];
        }

        private static bool HasUnsupportedLocalScope(XamlDomObject node)
        {
            if (node.ApiInformation != null || node.Parent?.ApiInformation != null ||
                !(node.Type is DirectUIXamlType type) || type.IsCodeGenType ||
                type.IsTemplateType || type.IsDictionary)
            {
                return true;
            }

            return node.MemberNodes.Any(member =>
                member.ApiInformation != null || member.Member.IsEvent || member.Member.IsAttachable ||
                (member.Member.IsDirective && member.Member != XamlLanguage.Initialization &&
                    member.Member != XamlLanguage.Class && member.Member != XamlLanguage.Base &&
                    member.Member != XamlLanguage.Items) ||
                member.Items.OfType<XamlDomObject>().Any(item =>
                    item.Type.IsMarkupExtension || item.Type.Name == "Binding"));
        }

        private static bool IsLiteral(string text)
        {
            if (text == null)
            {
                return false;
            }

            var components = text.Split(',');
            if (components.Length != 1 && components.Length != 2 && components.Length != 4)
            {
                return false;
            }

            foreach (var component in components)
            {
                var token = component.Trim(XmlWhitespace);
                if (!Number.IsMatch(token) ||
                    !double.TryParse(token, NumberStyles.Float, CultureInfo.InvariantCulture, out double number) ||
                    number < 0 || number > float.MaxValue)
                {
                    return false;
                }
            }
            return true;
        }

        private static XamlOptimizationReason AnalyzeSource(
            XamlOptimizationContext context, XamlDomObject node, List<XamlOptimizationEdit> edits)
        {
            if (!context.Contains(node.StartLineNumber, node.StartLinePosition) ||
                !context.Contains(node.EndLineNumber, node.EndLinePosition))
            {
                return XamlOptimizationReason.UnsupportedSourceShape;
            }

            var source = context.GetSourceInfo(node);
            if (source.SelfClosing || !context.Contains(source.EndClosingTag.Row, source.EndClosingTag.Col))
            {
                return XamlOptimizationReason.UnsupportedSourceShape;
            }

            var opening = context.ReadMarkup(source.StartOpeningTag, source.StartClosingTag);
            var closing = context.ReadMarkup(source.EndOpeningTag, source.EndClosingTag);
            if (!opening.StartsWith("<", StringComparison.Ordinal) || !opening.EndsWith(">", StringComparison.Ordinal) ||
                !closing.StartsWith("</", StringComparison.Ordinal) || !closing.EndsWith(">", StringComparison.Ordinal))
            {
                return XamlOptimizationReason.UnsupportedSourceShape;
            }

            var name = opening.Substring(1, opening.Length - 2).TrimEnd(XmlWhitespace);
            if (name.IndexOfAny(XmlWhitespace) >= 0 ||
                (name != "Thickness" && !name.EndsWith(":Thickness", StringComparison.Ordinal)) ||
                closing.Substring(2, closing.Length - 3).TrimEnd(XmlWhitespace) != name)
            {
                return XamlOptimizationReason.UnsupportedSourceShape;
            }

            // Inspect the original text as well as the DOM: comments, CDATA, entities and
            // processing instructions must not accidentally qualify after XML normalization.
            var content = context.ReadMarkup(source.StartClosingTag, source.EndOpeningTag);
            if (content.Length < 2 || !IsLiteral(content.Substring(1, content.Length - 2)))
            {
                return XamlOptimizationReason.UnsupportedSourceShape;
            }

            edits.Add(new XamlOptimizationEdit(source.StartOpeningTag, source.StartClosingTag));
            edits.Add(new XamlOptimizationEdit(source.EndOpeningTag, source.EndClosingTag));
            return XamlOptimizationReason.Lowered;
        }
    }
}
