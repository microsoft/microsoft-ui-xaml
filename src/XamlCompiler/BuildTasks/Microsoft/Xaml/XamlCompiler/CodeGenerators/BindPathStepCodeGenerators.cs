// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;

namespace Microsoft.UI.Xaml.Markup.Compiler.CodeGen
{
    internal interface IBindPathStepCodeGen
    {
        ICodeGenOutput PathExpression { get; }
        ICodeGenOutput UpdateCallParam { get; }
        ICodeGenOutput PathSetExpression(ICodeGenOutput input);
        ICodeGenOutput MemberAccessOperator { get; }
    }

    internal static class BindPathStepCodeGenExtensions
    {
        public static Dictionary<BindPathStep, IBindPathStepCodeGen> generatorCache = 
            new Core.InstanceCache<BindPathStep, IBindPathStepCodeGen>();

        public static IBindPathStepCodeGen CodeGen(this BindPathStep instance)
        {
            IBindPathStepCodeGen codeGen = null;
            if (!generatorCache.TryGetValue(instance, out codeGen))
            {
                if (instance is RootStep rootStep)
                {
                    codeGen = new RootStepCodeGenerator() { Instance = rootStep };
                }
                else if (instance is StaticRootStep staticRootStep)
                {
                    codeGen = new StaticRootStepCodeGenerator() { Instance = staticRootStep };
                }
                else if (instance is RootNamedElementStep rootNamedStep)
                {
                    codeGen = new RootNamedElementStepCodeGenerator() { Instance = rootNamedStep };
                }
                else if (instance is FieldStep fieldStep)
                {
                    codeGen = new FieldStepCodeGenerator() { Instance = fieldStep };
                }
                else if (instance is CastStep castStep)
                {
                    codeGen = new CastStepCodeGenerator() { Instance = castStep };
                }
                else if (instance is ArrayIndexStep arrayIndexStep)
                {
                    codeGen = new ArrayIndexStepCodeGenerator() { Instance = arrayIndexStep };
                }
                else if (instance is MapIndexStep mapIndexStep)
                {
                    codeGen = new MapIndexStepCodeGenerator() { Instance = mapIndexStep };
                }
                else if (instance is AttachedPropertyStep attachedProperty)
                {
                    codeGen = new AttachedPropertyStepCodeGenerator() { Instance = attachedProperty };
                }
                else if (instance is DependencyPropertyStep dependencyPropertyStep)
                {
                    codeGen = new DependencyPropertyStepCodeGenerator<DependencyPropertyStep>() { Instance = dependencyPropertyStep };
                }
                else if (instance is PropertyStep propertyStep)
                {
                    codeGen = new PropertyStepCodeGenerator<PropertyStep>() { Instance = propertyStep };
                }
                else if (instance is MethodStep methodStep)
                {
                    codeGen = new MethodStepCodeGenerator() { Instance = methodStep };
                }
                else if (instance is FunctionStep functionStep)
                {
                    codeGen = new FunctionStepCodeGenerator() { Instance = functionStep };
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

    internal abstract class BindPathStepCodeGenerator<T> :  CodeGeneratorBase<T>, IBindPathStepCodeGen where T : BindPathStep
    {
        public virtual ICodeGenOutput MemberAccessOperator
        {
            get
            {
                return new LanguageSpecificString(
                    () => Instance.ValueType.UnderlyingType.IsValueType ? "." : "->",
                    () => ".",
                    () => ".",
                    () => ".");
            }
        }

        public abstract ICodeGenOutput PathExpression { get; }

        public virtual ICodeGenOutput PathSetExpression(ICodeGenOutput value)
        {
            var pathExpression = Instance.CodeGen().PathExpression;
            string cppWinrtNameNoParen = pathExpression.CppWinRTName().TrimEnd(')', '(');
            return new LanguageSpecificString(
                () => $"{pathExpression.CppCXName()} = {value.CppCXName()}",
                () => $"{cppWinrtNameNoParen}({value.CppWinRTName()})",
                () => $"{pathExpression.CSharpName()} = {value.CSharpName()}",
                () => $"{pathExpression.VBName()} = {value.VBName()}");
        }

        public virtual ICodeGenOutput UpdateCallParam
        {
            // Derived steps should all be implementing UpdateCallParam.
            get { throw new NotImplementedException(); }
        }
    }

    internal class RootStepCodeGenerator : BindPathStepCodeGenerator<RootStep>
    {
        public override ICodeGenOutput PathExpression
        {
            get
            {
                if (Instance.IsElementRoot)
                {
                    return new LanguageSpecificString(
                        () => "this",
                        () => "",
                        () => "this",
                        () => "Me");
                }
                else
                {
                    return new LanguageSpecificString(
                        () => "this->GetDataRoot()",
                        () => "GetDataRoot()",
                        () => "this.dataRoot",
                        () => "Me.dataRoot");
                }
            }
        }
    }

    internal class StaticRootStepCodeGenerator : BindPathStepCodeGenerator<StaticRootStep>
    {
        public override ICodeGenOutput MemberAccessOperator
        {
            get { return new LanguageSpecificString(() => "::", () => "::", () => ".", () => "."); }
        }

        public override ICodeGenOutput PathExpression
        {
            get
            {
                var valueType = Instance.ValueType ;
                return new LanguageSpecificString(
                    () => valueType.CppCXName(false),
                    () => valueType.CppWinRTName(),
                    valueType.CSharpName,
                    valueType.VBName);
            }
        }
    }

    internal class RootNamedElementStepCodeGenerator : BindPathStepCodeGenerator<RootNamedElementStep>
    {
        public override ICodeGenOutput PathExpression
        {
            get
            {
                var parentPathExpression = Instance.Parent.CodeGen().PathExpression;
                var parentMemberAccessOperator = Instance.Parent.CodeGen().MemberAccessOperator;
                string fieldName = !string.IsNullOrEmpty(Instance.UpdateCallParamOverride) ? Instance.UpdateCallParamOverride : Instance.FieldName;

                return new LanguageSpecificString(
                    () => parentPathExpression.CppCXName() + parentMemberAccessOperator.CppCXName() + fieldName,
                    () => $"{parentPathExpression.CppWinRTName()}{parentMemberAccessOperator.CppWinRTName()}{fieldName}()",
                    () => parentPathExpression.CSharpName() + "." + fieldName,
                    () => parentPathExpression.VBName() + "." + fieldName);
            }
        }

        public override ICodeGenOutput UpdateCallParam
        {
            get
            {
                if (!string.IsNullOrEmpty(Instance.UpdateCallParamOverride))
                {
                    // Used when the RootNamedElementStep represents a named element in a template.
                    // Without this our update param string looks like 'obj.MyField', but named elements
                    // in a template don't exist as fields - instead, this override is given their
                    // object code name (e.g. 'obj5') and we use that with the bindings variable which
                    // should exist wherever this step is used (so instead of 'obj.MyField' we use
                    // 'bindings.obj5').
                    return new LanguageSpecificString(
                        () => $"{Instance.UpdateCallParamOverride}",
                        () => $"{Instance.UpdateCallParamOverride}",
                        () => $"{KnownStrings.UpdateParamBindingsName}.{Instance.UpdateCallParamOverride}",
                        () => $"{KnownStrings.UpdateParamBindingsName}.{Instance.UpdateCallParamOverride}");
                }
                else
                {
                    return new LanguageSpecificString(
                        () => $"{KnownStrings.UpdateParamName}->{Instance.FieldName}",
                        () => $"::winrt::get_self<{Instance.Parent.ValueType.CppWinRTLocalElseRef()}>({KnownStrings.UpdateParamName})->{Instance.FieldName}()",
                        () => $"{KnownStrings.UpdateParamName}.{Instance.FieldName}",
                        () => $"{KnownStrings.UpdateParamName}.{Instance.FieldName}");
                }
            }
        }
    }

    internal class AttachedPropertyStepCodeGenerator : DependencyPropertyStepCodeGenerator<AttachedPropertyStep>
    {
        public override ICodeGenOutput PathExpression
        {
            get
            {
                var parentPathExpression = Instance.Parent.CodeGen().PathExpression;
                return new LanguageSpecificString(
                    () => string.Format("{0}::Get{1}({2})", Instance.OwnerType.CppCXName(false), Instance.PropertyName, parentPathExpression.CppCXName()),
                    () => $"{Instance.OwnerType.CppWinRTName()}::Get{Instance.PropertyName}({parentPathExpression.CppWinRTName()})",
                    () => string.Format("{0}.Get{1}({2})", Instance.OwnerType.CSharpName(), Instance.PropertyName, parentPathExpression.CSharpName()),
                    () => string.Format("{0}.Get{1}({2})", Instance.OwnerType.VBName(), Instance.PropertyName, parentPathExpression.VBName()));
            }
        }

        public override ICodeGenOutput PathSetExpression(ICodeGenOutput value)
        {
            var parentPathExpression = Instance.Parent.CodeGen().PathExpression;
            return new LanguageSpecificString(
                () => $"{Instance.OwnerType.CppCXName(false)}::Set{Instance.PropertyName}({parentPathExpression.CppCXName()}, {value.CppCXName()})",
                () => $"{Instance.OwnerType.CppWinRTName()}::Set{Instance.PropertyName}({parentPathExpression.CppWinRTName()}, {value.CppWinRTName()})",
                () => $"{Instance.OwnerType.CSharpName()}.Set{Instance.PropertyName}({parentPathExpression.CSharpName()}, {value.CSharpName()})",
                () => $"{Instance.OwnerType.VBName()}.Set{Instance.PropertyName}({parentPathExpression.VBName()}, {value.VBName()})");
        }

        public override ICodeGenOutput UpdateCallParam
        {
            get
            {
                return new LanguageSpecificString(
                    () => string.Format("{0}::Get{1}({2})", Instance.OwnerType.CppCXName(false), Instance.PropertyName, KnownStrings.UpdateParamName),
                    () => $"{Instance.OwnerType.CppWinRTName()}::Get{Instance.PropertyName}({KnownStrings.UpdateParamName})",
                    () => string.Format("{0}.Get{1}({2})", Instance.OwnerType.CSharpName(), Instance.PropertyName, KnownStrings.UpdateParamName),
                    () => string.Format("{0}.Get{1}({2})", Instance.OwnerType.VBName(), Instance.PropertyName, KnownStrings.UpdateParamName));
            }
        }
    }

    internal class CastStepCodeGenerator : BindPathStepCodeGenerator<CastStep>
    {
        public override ICodeGenOutput PathExpression
        {
            get
            {
                var parentPathExpression = Instance.Parent.CodeGen().PathExpression;
                return new LanguageSpecificString(
                    () => string.Format("(safe_cast<{0}>({1}))", Instance.ValueType.CppCXName(), parentPathExpression.CppCXName()),
                    () => Instance.Parent.ValueType.CppWinRTCast(Instance.ValueType, parentPathExpression.CppWinRTName()),
                    () => string.Format("(({0}){1})", Instance.ValueType.CSharpName(), parentPathExpression.CSharpName()),
                    () => string.Format("{2}({1}, {0})", Instance.ValueType.VBName(), parentPathExpression.VBName(), Instance.Parent.ValueType.GetVBCastName(Instance.ValueType.UnderlyingType)));
            }
        }

        public override ICodeGenOutput UpdateCallParam
        {
            get
            {
                if (Instance.ValueType.IsString())
                {
                    return new LanguageSpecificString(
                        () => string.Format("{0}->ToString()", KnownStrings.UpdateParamName),
                        () => string.Format("::winrt::to_hstring({0})", KnownStrings.UpdateParamName),
                        () => string.Format("{0}.ToString()", KnownStrings.UpdateParamName),
                        () => string.Format("{0}.ToString()", KnownStrings.UpdateParamName));
                }
                else
                {
                    return Instance.Parent.ValueType.GetInlineConversionExpression(Instance.ValueType,
                        new LanguageSpecificString(() => KnownStrings.UpdateParamName));
                }
            }
        }
    }

    internal class PropertyStepCodeGenerator<T> : BindPathStepCodeGenerator<T> where T : PropertyStep
    {
        public override ICodeGenOutput PathExpression
        {
            get
            {
                var parentPathExpression = Instance.Parent.CodeGen().PathExpression;
                var parentMemberAccessOperator = Instance.Parent.CodeGen().MemberAccessOperator;
                return new LanguageSpecificString(
                    () => parentPathExpression.CppCXName() + parentMemberAccessOperator.CppCXName() + Instance.PropertyName,
                    () => $"{parentPathExpression.CppWinRTName()}{parentMemberAccessOperator.CppWinRTName()}{Instance.PropertyName}()",
                    () => parentPathExpression.CSharpName() + "." + Instance.PropertyName,
                    () => parentPathExpression.VBName() + "." + Instance.PropertyName);
            }
        }

        public override ICodeGenOutput UpdateCallParam
        {
            get
            {
                var parentPathExpression = Instance.Parent.CodeGen().PathExpression;
                var parentMemberAccessOperator = Instance.Parent.CodeGen().MemberAccessOperator;
                return new LanguageSpecificString(
                    () => $"{(Instance.Parent is StaticRootStep ? parentPathExpression.CppCXName() : KnownStrings.UpdateParamName)}{parentMemberAccessOperator.CppCXName()}{Instance.PropertyName}",
                    () => $"{(Instance.Parent is StaticRootStep ? parentPathExpression.CppWinRTName() : KnownStrings.UpdateParamName)}{parentMemberAccessOperator.CppWinRTName()}{Instance.PropertyName}()",
                    () => $"{(Instance.Parent is StaticRootStep ? parentPathExpression.CSharpName() : KnownStrings.UpdateParamName)}.{Instance.PropertyName}",
                    () => $"{(Instance.Parent is StaticRootStep ? parentPathExpression.VBName() : KnownStrings.UpdateParamName)}.{Instance.PropertyName}");
            }
        }
    }

    internal class FieldStepCodeGenerator : PropertyStepCodeGenerator<FieldStep>
    {
        public override ICodeGenOutput PathExpression
        {
            get
            {
                var parentPathExpression = Instance.Parent.CodeGen().PathExpression;
                var parentMemberAccessOperator = Instance.Parent.CodeGen().MemberAccessOperator;
                return new LanguageSpecificString(
                    () => parentPathExpression.CppCXName() + parentMemberAccessOperator.CppCXName() + Instance.FieldName,
                    () => $"{parentPathExpression.CppWinRTName()}{parentMemberAccessOperator.CppWinRTName()}{Instance.FieldName}",
                    () => parentPathExpression.CSharpName() + "." + Instance.FieldName,
                    () => parentPathExpression.VBName() + "." + Instance.FieldName);
            }
        }

        public override ICodeGenOutput UpdateCallParam
        {
            get
            {
                var parentPathExpression = Instance.Parent.CodeGen().PathExpression;
                var parentMemberAccessOperator = Instance.Parent.CodeGen().MemberAccessOperator;
                return new LanguageSpecificString(
                    () => $"{(Instance.Parent is StaticRootStep ? parentPathExpression.CppCXName() : KnownStrings.UpdateParamName)}{parentMemberAccessOperator.CppCXName()}{Instance.FieldName}",
                    () => $"{(Instance.Parent is StaticRootStep ? parentPathExpression.CppWinRTName() : KnownStrings.UpdateParamName)}{parentMemberAccessOperator.CppWinRTName()}{Instance.FieldName}",
                    () => $"{(Instance.Parent is StaticRootStep ? parentPathExpression.CSharpName() : KnownStrings.UpdateParamName)}.{Instance.FieldName}",
                    () => $"{(Instance.Parent is StaticRootStep ? parentPathExpression.VBName() : KnownStrings.UpdateParamName)}.{Instance.FieldName}");
            }
        }
    }

    internal class DependencyPropertyStepCodeGenerator<T> : PropertyStepCodeGenerator<T> where T : DependencyPropertyStep
    {
    }

    internal class ArrayIndexStepCodeGenerator : BindPathStepCodeGenerator<ArrayIndexStep>
    {
        public override ICodeGenOutput PathExpression
        {
            get
            {
                var parentPathExpression = Instance.Parent.CodeGen().PathExpression;
                return new LanguageSpecificString(
                    () => string.Format("safe_cast<::Windows::Foundation::Collections::IVector<{0}>^>({1})->GetAt({2})",
                        Instance.ValueType.CppCXName(), parentPathExpression.CppCXName(), Instance.Index),
                    () => string.Format("{1}.as<::winrt::Windows::Foundation::Collections::IVector<{0}>>().GetAt({2})",
                        Instance.ValueType.CppWinRTName(), parentPathExpression.CppWinRTName(), Instance.Index),
                    () => string.Format("{0}[{1}]", parentPathExpression.CSharpName(), Instance.Index),
                    () => string.Format("{0}({1})", parentPathExpression.VBName(), Instance.Index));
            }
        }

        public override ICodeGenOutput PathSetExpression(ICodeGenOutput value)
        {
            var parentPathExpression = Instance.Parent.CodeGen().PathExpression;
            return new LanguageSpecificString(
                () => $"safe_cast<::Windows::Foundation::Collections::IVector<{Instance.ValueType.CppCXName()}>^>" +
                      $"({parentPathExpression.CppCXName()})->SetAt({Instance.Index}, {value.CppCXName()})",
                () => $"{parentPathExpression.CppWinRTName()}.as<::winrt::Windows::Foundation::Collections" +
                      $"::IVector<{Instance.ValueType.CppWinRTName()}>>().SetAt({Instance.Index}, {value.CppWinRTName()})",
                () => $"{PathExpression.CSharpName()} = {value.CSharpName()}",
                () => $"{PathExpression.VBName()} = {value.VBName()}");
        }

        public override ICodeGenOutput UpdateCallParam
        {
            get
            {
                return new LanguageSpecificString(
                    () => string.Format("safe_cast<::Windows::Foundation::Collections::IVector<{0}>^>({1})->GetAt({2})",
                        Instance.ValueType.CppCXName(), KnownStrings.UpdateParamName, Instance.Index),
                    () => $"{KnownStrings.UpdateParamName}.as <::winrt::Windows::Foundation::Collections::IVector<{Instance.ValueType.CppWinRTName()}>>().GetAt({Instance.Index})",
                    () => string.Format("{0}[{1}]", KnownStrings.UpdateParamName, Instance.Index),
                    () => string.Format("{0}({1})", KnownStrings.UpdateParamName, Instance.Index));
            }
        }
    }

    internal class MapIndexStepCodeGenerator : BindPathStepCodeGenerator<MapIndexStep>
    {
        public override ICodeGenOutput PathExpression
        {
            get
            {
                var parentPathExpression = Instance.Parent.CodeGen().PathExpression;
                return new LanguageSpecificString(
                    () => string.Format("safe_cast<::Windows::Foundation::Collections::IMap<::Platform::String^, {0}>^>({1})->Lookup(\"{2}\")",
                        Instance.ValueType.CppCXName(), parentPathExpression.CppCXName(), Instance.Key),
                    () => string.Format("{1}.as<::winrt::Windows::Foundation::Collections::IMap<::winrt::hstring, {0}>>().Lookup(\"{2}\")",
                        Instance.ValueType.CppWinRTName(), parentPathExpression.CppWinRTName(), Instance.Key),
                    () => string.Format("{0}[\"{1}\"]", parentPathExpression.CSharpName(), Instance.Key),
                    () => string.Format("{0}(\"{1}\")", parentPathExpression.VBName(), Instance.Key));
            }
        }

        public override ICodeGenOutput PathSetExpression(ICodeGenOutput value)
        {
            var pathExpression = Instance.CodeGen().PathExpression;
            var parentPathExpression = Instance.Parent.CodeGen().PathExpression;
            return new LanguageSpecificString(
                () => $"safe_cast<::Windows::Foundation::Collections::IMap<::Platform::String^, {Instance.ValueType.CppCXName()}>^>" +
                      $"({parentPathExpression.CppCXName()})->Insert(\"{Instance.Key}\", {value.CppCXName()})",
                () => $"{parentPathExpression.CppWinRTName()}.as<::winrt::Windows::Foundation::Collections::" +
                      $"IMap<::winrt::hstring, {Instance.ValueType.CppWinRTName()}>>().Insert(L\"{Instance.Key}\", {value.CppWinRTName()})",
                () => $"{pathExpression.CSharpName()} = {value.CSharpName()}",
                () => $"{pathExpression.VBName()} = {value.VBName()}");
        }

        public override ICodeGenOutput UpdateCallParam
        {
            get
            {
                return new LanguageSpecificString(
                    () => $"safe_cast<::Windows::Foundation::Collections::IMap<::Platform::String^, {Instance.ValueType.CppCXName()}>^>({KnownStrings.UpdateParamName})->Lookup(\"{Instance.Key}\")",
                    () => $"{KnownStrings.UpdateParamName}.as<::winrt::Windows::Foundation::Collections::IMap<::winrt::hstring, {Instance.ValueType.CppWinRTName()}>>().Lookup(L\"{Instance.Key}\")",
                    () => string.Format("{0}[\"{1}\"]", KnownStrings.UpdateParamName, Instance.Key),
                    () => string.Format("{0}(\"{1}\")", KnownStrings.UpdateParamName, Instance.Key));
            }
        }
    }

    internal class MethodStepCodeGenerator : BindPathStepCodeGenerator<MethodStep>
    {
        public override ICodeGenOutput PathExpression
        {
            get
            {
                var parentPathExpression = Instance.Parent.CodeGen().PathExpression;
                var parentMemberAccessOperator = Instance.Parent.CodeGen().MemberAccessOperator;
                var paramList = Instance.Parameters.ForCall();
                return new LanguageSpecificString(
                    () => $"{parentPathExpression.CppCXName()}{parentMemberAccessOperator.CppCXName()}{Instance.MethodName}({paramList})",
                    () => $"{parentPathExpression.CppWinRTName()}{parentMemberAccessOperator.CppWinRTName()}{Instance.MethodName}({paramList})",
                    () => $"{parentPathExpression.CSharpName()}.{Instance.MethodName}({paramList})",
                    () => $"{parentPathExpression.VBName()}.{Instance.MethodName}({paramList})"
                    );
            }
        }
    }

    internal class FunctionStepCodeGenerator : BindPathStepCodeGenerator<FunctionStep>
    {
        public override ICodeGenOutput PathExpression
        {
            get { return Instance.Method.CodeGen().PathExpression; }
        }

        public override ICodeGenOutput UpdateCallParam
        {
            get { return new LanguageSpecificString(() => String.Empty); }
        }
    }
}