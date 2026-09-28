// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler.Optimization
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using XamlDom;

    internal sealed class XamlOptimizationContext
    {
        private readonly string[] lines;

        public XamlOptimizationContext(XamlDomObject root, string[] lines)
        {
            Root = root ?? throw new ArgumentNullException(nameof(root));
            this.lines = (string[])(lines ?? throw new ArgumentNullException(nameof(lines))).Clone();
        }

        public XamlDomObject Root { get; }

        public FixedSourceInfo GetSourceInfo(XamlDomObject node)
        {
            return XamlSourceInfoFixer.GetFixedSourceInfo(new StrippableObject(node), lines);
        }

        public string ReadMarkup(SourcePos start, SourcePos end)
        {
            return XamlSourceInfoFixer.ReadMarkup(start, end, lines).TrimEnd('\r', '\n');
        }

        public bool Contains(int line, int column)
        {
            return line > 0 && line <= lines.Length && column > 0 && column <= lines[line - 1].Length;
        }
    }

    // Edits only erase characters, preserving every line and column in the original DOM.
    internal sealed class XamlOptimizationEdit
    {
        public XamlOptimizationEdit(SourcePos start, SourcePos end)
        {
            StartLine = start.Row;
            StartColumn = start.Col;
            EndLine = end.Row;
            EndColumn = end.Col;
        }

        public int StartLine { get; }
        public int StartColumn { get; }
        public int EndLine { get; }
        public int EndColumn { get; }
    }

    internal enum XamlOptimizationReason
    {
        Lowered,
        UnsupportedTypeOrProperty,
        DecoratedValue,
        UnsupportedScope,
        UnsupportedLiteral,
        UnsupportedSourceShape
    }

    internal sealed class XamlOptimizationDecision
    {
        public XamlOptimizationDecision(string ruleId, XamlDomObject node, XamlOptimizationReason reason)
        {
            RuleId = ruleId;
            Line = node.StartLineNumber;
            Column = node.StartLinePosition;
            Reason = reason;
        }

        public string RuleId { get; }
        public int Line { get; }
        public int Column { get; }
        public XamlOptimizationReason Reason { get; }
    }

    internal sealed class XamlOptimizationResult
    {
        public XamlOptimizationResult(IEnumerable<XamlOptimizationEdit> edits, IEnumerable<XamlOptimizationDecision> decisions)
        {
            Edits = edits.ToList().AsReadOnly();
            Decisions = decisions.ToList().AsReadOnly();
        }

        public IReadOnlyList<XamlOptimizationEdit> Edits { get; }
        public IReadOnlyList<XamlOptimizationDecision> Decisions { get; }
    }

    internal interface IXamlOptimizationRule
    {
        XamlOptimizationResult Analyze(XamlOptimizationContext context);
    }

    internal static class XamlOptimizationPipeline
    {
        private static readonly IXamlOptimizationRule[] Rules = { new InlineThicknessPaddingRule() };

        public static XamlOptimizationResult Analyze(XamlDomObject root, string[] lines)
        {
            var context = new XamlOptimizationContext(root, lines);
            var edits = new List<XamlOptimizationEdit>();
            var decisions = new List<XamlOptimizationDecision>();

            // All rules analyze the same validated DOM. Overlapping proposals are an error,
            // not an invitation to apply original source coordinates to already-shifted text.
            foreach (var rule in Rules)
            {
                var result = rule.Analyze(context);
                edits.AddRange(result.Edits);
                decisions.AddRange(result.Decisions);
            }

            var orderedEdits = edits.OrderBy(e => e.StartLine).ThenBy(e => e.StartColumn).ToList();
            XamlOptimizationEdit previous = null;
            foreach (var edit in orderedEdits)
            {
                if (!context.Contains(edit.StartLine, edit.StartColumn) ||
                    !context.Contains(edit.EndLine, edit.EndColumn) ||
                    edit.EndLine < edit.StartLine ||
                    (edit.EndLine == edit.StartLine && edit.EndColumn < edit.StartColumn) ||
                    (previous != null && (edit.StartLine < previous.EndLine ||
                        (edit.StartLine == previous.EndLine && edit.StartColumn <= previous.EndColumn))))
                {
                    throw new InvalidOperationException("Invalid or overlapping XAML optimization source edits.");
                }
                previous = edit;
            }

            return new XamlOptimizationResult(orderedEdits, decisions);
        }
    }
}
