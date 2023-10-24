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

        public override void ExitApi_information([NotNull] global::ConditionalNamespaceParser.Api_informationContext context)
        {
            var methodName = context.IDENTIFIER().GetText();
            var paramArray = context.function_param();

            List<ApiInformationParameter> parameters = null;
            if (paramArray.Any())
            {
                parameters = new List<ApiInformationParameter>();
                foreach (var param in paramArray)
                {
                    parameters.Add(param.ApiInformationParameter);
                }
            }
            try
            {
                context.ApiInformation = new ApiInformation(methodName);
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

        public override void ExitFunction_param([NotNull] ConditionalNamespaceParser.Function_paramContext context)
        {
            var paramValue = context.GetText();
            context.ApiInformationParameter = new ApiInformationParameter(paramValue);
        }
    }
}