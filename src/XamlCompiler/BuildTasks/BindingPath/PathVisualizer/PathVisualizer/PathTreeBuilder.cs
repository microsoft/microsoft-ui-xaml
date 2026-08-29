// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Antlr4.Runtime;
using Antlr4.Runtime.Tree;
using System.Collections.Generic;

namespace Microsoft { namespace Xaml { namespace XamlCompiler {
    public class BindPathStep { }
    public class FunctionParam { }
} } }

namespace PathVisualizer
{
    /// <summary>
    /// BailErrorStrategy
    /// Disables Antlr's automatic error recovery mechanisms following the example in
    /// The Definitive Antlr 4 Reference - section 9.5 on page 172.
    /// </summary>
    internal class BailErrorStrategy : DefaultErrorStrategy
    {
        public override void Recover(Parser recognizer, RecognitionException e) { throw e; }

        public override IToken RecoverInline(Parser recognizer)
        {
            throw new InputMismatchException(recognizer);
        }

        public override void Sync(Parser recognizer) { }
    }

    internal static class BindingPathLexerExtensions
    {
        public static string GetUnconsumed(this BindingPathLexer lexer)
        {
            if (!lexer.HitEOF)
            {
                var path = lexer.InputStream.ToString();
                return lexer.Token == null ? path : path.Substring(lexer.Token.StartIndex);
            }

            return string.Empty;
        }
    }

    public class PathTreeNode
    {
        public string Name { get; set; }
        public string Data { get; set; }

        public List<PathTreeNode> Children { get; } = new List<PathTreeNode>();
    }

    public class PathTree
    {
        public PathTreeNode Nodes { get; set; }

        public string Remainder { get; set; }
    }

    internal class PathTreeBuilder
    {
        public static PathTree ParsePath(string path)
        {
            var ret = new PathTree();

            // Text="{x:Bind}" - short circuit empty paths
            if (0 == path.Length) { }

            // Antlr parsing engine setup
            var input = new AntlrInputStream(path);
            var lexer = new BindingPathLexer(input);
            var tokens = new CommonTokenStream(lexer);
            var parser = new BindingPathParser(tokens);

            // redirect parsing errors as VS exceptions instead of console output
            // and disable the automatic error recovery handler
            parser.ErrorHandler = new BailErrorStrategy();

            // build the tree of nodes
            ret.Nodes = ProcessNode(parser, parser.path());

            // confirm nothing left unparsed at end of path string
            ret.Remainder = lexer.GetUnconsumed();

            return ret;
        }

        private static PathTreeNode ProcessNode(Parser p, IParseTree t)
        {
            var ret = new PathTreeNode();

            // Pre-Order traversal
            if (t is IRuleNode)
            {
                var r = t as IRuleNode;

                ret.Name = p.RuleNames[r.RuleContext.RuleIndex];
                ret.Data = r.GetText();

                for (int iChild = 0; iChild < r.ChildCount; iChild++)
                {
                    ret.Children.Add(ProcessNode(p, r.GetChild(iChild)));
                }
            }
            else if (t is ITerminalNode)
            {
                var m = t as ITerminalNode;

                ret.Data = t.GetText();
            }
            else
            {

            }

            return ret;
        }
    }
}