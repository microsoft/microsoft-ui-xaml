// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Antlr4.Runtime.Misc;
using System;
using System.Collections.Generic;
using System.Linq;

namespace Microsoft.UI.Xaml.Markup.Compiler.Parsing
{
    /// <summary>
    /// ConditionalNamespaceListener
    /// Overrides the generated ConditionalNamespaceBaseListener class to provide implementations
    /// for the different parsing rules specified in the ConditionalNamespace.g4 file.  This allows
    /// us to build up the BindPathSteps that will be returned from BindPathParser.ParsePath.
    /// </summary>
    internal class ConditionalNamespaceListener : ConditionalNamespaceBaseListener
    {
        private readonly string expresion;

        public ConditionalNamespaceListener([NotNull] string expresion)
        {
            this.expresion = expresion;
        }

        public override void ExitExpression([NotNull] ConditionalNamespaceParser.ExpressionContext context)
        {
            context.TargetPlatform = context.query_string().TargetPlatform;
            context.ApiInformation = context.query_string().ApiInformation;
        }

        public override void ExitQuery_string([NotNull] ConditionalNamespaceParser.Query_stringContext context)
        {
            var componentArray = context.query_string_component();
            Platform targPlat = Platform.Any;
            ApiInformation apiInfo = null;

            foreach (var component in componentArray)
            {
                if (component.ApiInformation != null)
                {
                    // If apiInfo isn't null, we've already parsed an ApiInformation string
                    // for this namespace but found another. Can't have duplicates, so error.
                    if (apiInfo != null)
                    {
                        throw new ParseException(ErrorMessages.MultipleNamespaceConditionalStatements);
                    }
                    apiInfo = component.ApiInformation;
                }

                if (component.TargetPlatform != Platform.Any)
                {
                    // If targPlat isn't null, we've already parsed an TargetPlatform string for this namespace but found another.  Can't have duplicates, so error
                    if (targPlat != Platform.Any)
                    {
                        throw new ParseException(ErrorMessages.MultipleTargetPlatforms);
                    }
                    targPlat = component.TargetPlatform;
                }
            }

            context.ApiInformation = apiInfo;
            context.TargetPlatform = targPlat;
        }

        public override void ExitQueryStringApiInformation([NotNull] ConditionalNamespaceParser.QueryStringApiInformationContext context)
        {
            context.ApiInformation = context.api_information().ApiInformation;
        }

        public override void ExitQueryStringTargetPlatform([NotNull] ConditionalNamespaceParser.QueryStringTargetPlatformContext context)
        {
            context.TargetPlatform = context.target_platform_func().TargetPlatform;
        }

        public override void ExitTarget_platform_func([NotNull] ConditionalNamespaceParser.Target_platform_funcContext context)
        {
            string targetPlatformString = context.target_platform_value().GetText(); //Should be something like "UWP", etc.
            context.TargetPlatform = (Platform)(Enum.Parse(typeof(Platform), targetPlatformString));
        }

        public override void ExitFunction_param([NotNull] ConditionalNamespaceParser.Function_paramContext context)
        {
            // Compute Value using ( and ) character positions from the parent api_information context.
            // The grammar's function_param rule cannot tokenize digits or commas, so we read
            // directly from the character stream between the parentheses to capture all characters.
            var parentCtx = context.Parent as ConditionalNamespaceParser.Api_informationContext;
            if (parentCtx != null)
            {
                var openParen = parentCtx.GetToken(ConditionalNamespaceParser.LPAREN, 0);  // '('
                var closeParen = parentCtx.GetTokens(ConditionalNamespaceParser.RPAREN).LastOrDefault(); // last ')'
                if (openParen != null && closeParen != null)
                {
                    int startPos = openParen.Symbol.StopIndex + 1;
                    int endPos = closeParen.Symbol.StartIndex - 1;
                    if (endPos >= startPos)
                    {
                        context.Value = openParen.Symbol.InputStream.GetText(
                            new Interval(startPos, endPos));
                    }
                    else
                    {
                        context.Value = string.Empty;
                    }
                }
            }
        }

        public override void ExitApi_information([NotNull] global::ConditionalNamespaceParser.Api_informationContext context)
        {
            var identifiers = context.IDENTIFIER();
            string prefix, methodName;

            if (identifiers.Length == 2)
            {
                prefix = identifiers[0].GetText();
                methodName = identifiers[1].GetText();
            }
            else
            {
                prefix = string.Empty;
                methodName = identifiers[0].GetText();
            }
            var functionParam = context.function_param();
            List<ApiInformationParameter> parameters = null;
            if (functionParam != null && !string.IsNullOrEmpty(functionParam.Value))
            {
                parameters = new List<ApiInformationParameter>();
                if(string.IsNullOrEmpty(prefix))
                {
                    var functionParamValues = functionParam.Value.Split(',');
                    foreach (var value in functionParamValues)
                    {
                        parameters.Add(new ApiInformationParameter(value.Trim()));
                    }
                }
                else
                {
                    parameters.Add(new ApiInformationParameter(functionParam.Value));
                }
            }
            try
            {
                if (!string.IsNullOrEmpty(prefix))
                {
                    context.ApiInformation = new ApiInformation(methodName, prefix);
                }
                else
                {
                    context.ApiInformation = new ApiInformation(methodName);
                }
            }
            catch (ArgumentException)
            {
                throw new ParseException(ErrorMessages.UnrecognizedApiInformation, methodName);
            }

            try
            {
                context.ApiInformation.SetParameters(parameters);
            }
            catch (ArgumentException)
            {
                throw new ParseException(ErrorMessages.UnmatchedApiInformationParameters, methodName);
            }
        }


    }
}