// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Antlr4.Runtime;
using Antlr4.Runtime.Tree;
using System.Collections.Generic;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    using Parsing;

    public class ConditionalNamespace
    {
        public string UnconditionalNamespace { get; }
        public ApiInformation ApiInfo { get; }
        public Platform PlatConditional { get; }

        public ConditionalNamespace(string unconditionalNamespace, ApiInformation apiInfo, Platform targetPlat)
        {
            UnconditionalNamespace = unconditionalNamespace;
            ApiInfo = apiInfo;
            PlatConditional = targetPlat;
        }

        private static Dictionary<string, ConditionalNamespace> cache = new Core.InstanceCache<string, ConditionalNamespace>();

        public static ConditionalNamespace Parse(string namespaceFullName)
        {
            if (cache.ContainsKey(namespaceFullName))
            {
                return cache[namespaceFullName];
            }

            // Antlr parsing engine setup
            var input = new AntlrInputStream(namespaceFullName);
            var lexer = new ConditionalNamespaceLexer(input);
            var tokens = new CommonTokenStream(lexer);
            var parser = new ConditionalNamespaceParser(tokens);

            // redirect parsing errors as VS exceptions instead of console output
            // and disable the automatic error recovery handler
            parser.RemoveErrorListeners();
            parser.AddErrorListener(new Parsing.ParseErrorListener());
            parser.ErrorHandler = new Parsing.BailErrorStrategy();

            // set our custom callback override to build up the BindPathSteps during parse
            var listener = new Parsing.ConditionalNamespaceListener(namespaceFullName);

            // parse the input
            var walker = new ParseTreeWalker();
            var parsedObject = parser.expression();
            walker.Walk(listener, parsedObject);

            // confirm nothing left unparsed at end of path string
            lexer.ConfirmInputFullyConsumed();

            var parsedValues = new ConditionalNamespace(parsedObject.GetChild(0).GetText(), parsedObject.ApiInformation, parsedObject.TargetPlatform);
            cache[namespaceFullName] = parsedValues;
            return parsedValues;
        }
    }
}
