// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;

namespace Microsoft.UI.Xaml.Markup.Compiler.CodeGen
{
    public interface IBindPathParameterCodeGen
    {
        ICodeGenOutput PathExpression { get; }
    }

    internal static class BindPathParameterCodeGenExtensions
    {
        public static Dictionary<Parameter, IBindPathParameterCodeGen> generatorCache =
           new Core.InstanceCache<Parameter, IBindPathParameterCodeGen>();

        public static IBindPathParameterCodeGen CodeGen(this Parameter instance)
        {
            IBindPathParameterCodeGen codeGen = null;
            if (!generatorCache.TryGetValue(instance, out codeGen))
            {
                if (instance is FunctionNumberParam numberParam)
                {
                    codeGen = new FunctionNumberParamCodeGenerator() { Instance = numberParam };
                }
                else if (instance is FunctionNullValueParam nullValueParam)
                {
                    codeGen = new FunctionNullValueParamCodeGenerator() { Instance = nullValueParam };
                }
                else if (instance is FunctionStringParam stringParam)
                {
                    codeGen = new FunctionStringParamCodeGenerator() { Instance = stringParam };
                }
                else if (instance is FunctionBoolParam boolParam)
                {
                    codeGen = new FunctionBoolParamCodeGenerator() { Instance = boolParam };
                }
                else if (instance is FunctionPathParam pathParam)
                {
                    codeGen = new FunctionPathParamCodeGenerator() { Instance = pathParam };
                }
                else
                {
                    throw new NotImplementedException("Code generator not found!");
                }
                generatorCache.Add(instance, codeGen);
            }
            return codeGen;
        }
    }


    internal abstract class ParameterCodeGenerator<T> : CodeGeneratorBase<T>, IBindPathParameterCodeGen where T : Parameter
    {
        public virtual ICodeGenOutput PathExpression { get; }
    }

    internal class FunctionBoolParamCodeGenerator : ParameterCodeGenerator<FunctionBoolParam>
    {
        public override ICodeGenOutput PathExpression
        {
            get
            {
                return new LanguageSpecificString(
                    () => Instance.Value ? "true" : "false",
                    () => Instance.Value ? "true" : "false",
                    () => Instance.Value ? "true" : "false",
                    () => Instance.Value ? "True" : "False");
            }
        }
    }

    internal class FunctionNumberParamCodeGenerator : ParameterCodeGenerator<FunctionNumberParam>
    {
        public override ICodeGenOutput PathExpression
        {
            get
            {
                return new LanguageSpecificString(
                    () =>
                    {
                        string suffix = "";
                        if (Instance.ValueType.IsFloat())
                        {
                            suffix = "F";
                        }
                        return Instance.Value + suffix;
                    });
            }
        }
    }

    internal class FunctionStringParamCodeGenerator : ParameterCodeGenerator<FunctionStringParam>
    {
        public override ICodeGenOutput PathExpression
        {
            get
            {
                return new LanguageSpecificString(
                    () => Instance.Value.Quotenate(),
                    () => $"L{Instance.Value.Quotenate()}",
                    () => Instance.Value.Quotenate(),
                    () => Instance.Value.Quotenate());
            }
        }
    }

    internal class FunctionPathParamCodeGenerator : ParameterCodeGenerator<FunctionPathParam>
    {
        public override ICodeGenOutput PathExpression
        {
            get { return Instance.Path.CodeGen().PathExpression; }
        }
    }

    internal class FunctionNullValueParamCodeGenerator: ParameterCodeGenerator<FunctionNullValueParam>
    {
        public override ICodeGenOutput PathExpression
        {
            get { return LanguageSpecificString.Null; }
        }
    }
}