// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Antlr4.Runtime.Misc;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Xaml;

namespace Microsoft.UI.Xaml.Markup.Compiler.Parsing
{
    /// <summary>
    /// BindingPathListener overrides the generated BindingPathBaseListener class
    /// to provide implementations for the different parsing rules specified in the
    /// BindingPath.g4 file.
    /// </summary>
    internal class BindingPathListener : BindingPathBaseListener
    {
        private readonly string pathExpression;
        private IXamlTypeResolver resolver;
        private IBindUniverse bindUniverse;
        private readonly ApiInformation apiInformation;

        public IList<string> Warnings { get; set; }

        public BindingPathListener(
            [NotNull] string pathExpression,
            ApiInformation apiInformation,
            [NotNull] IBindUniverse bindUniverse,
            [NotNull] IXamlTypeResolver resolver
            )
        {
            this.apiInformation = apiInformation;
            this.pathExpression = pathExpression;
            this.resolver = resolver;
            this.bindUniverse = bindUniverse;
        }

        #region Identifiers
        public override void ExitPathIdentifier([NotNull] BindingPathParser.PathIdentifierContext context)
        {
            string identifier = context.IDENTIFIER().GetText();
            BindPathStep step = ResolveNameOnRoot(identifier);
            context.PathStep = bindUniverse.EnsureUniquePathStep(step);
        }

        public override void ExitPathDotIdentifier([NotNull] BindingPathParser.PathDotIdentifierContext context)
        {
            BindPathStep parent = EnsureNotFunction(context.path().PathStep);
            string identifier = context.IDENTIFIER().GetText();

            BindPathStep step = ResolveNameOnType(identifier, parent.ValueType, parent);
            context.PathStep = bindUniverse.EnsureUniquePathStep(step);
        }

        public override void ExitPathStaticIdentifier([NotNull] BindingPathParser.PathStaticIdentifierContext context)
        {
            string propertyName = context.IDENTIFIER().GetText();
            BindPathStep staticStep = CreateStaticRootStep(context.static_type());
            BindPathStep step = ResolveNameOnType(propertyName, staticStep.ValueType, staticStep);

            context.PathStep = bindUniverse.EnsureUniquePathStep(step);
        }
        #endregion

        #region Indexers
        public override void ExitPathIndexer([NotNull] BindingPathParser.PathIndexerContext context)
        {
            BindPathStep parent = EnsureNotFunction(context.path().PathStep);
            string indexValue = context.Digits().GetText();

            int index;
            if (!int.TryParse(indexValue, out index))
            {
                throw new ParseException(ErrorMessages.ExpectingDigit);
            }

            if (!parent.ValueType.IsIntegerIndexable())
            {
                throw new ParseException(ErrorMessages.UnexpectedArrayIndexer);
            }

            ArrayIndexStep step = new ArrayIndexStep(index, parent.ValueType.ItemType, parent, apiInformation);
            context.PathStep = bindUniverse.EnsureUniquePathStep(step);
        }

        public override void ExitPathStringIndexer([NotNull] BindingPathParser.PathStringIndexerContext context)
        {
            BindPathStep parent = EnsureNotFunction(context.path().PathStep);
            string keyValue = UnescapeAndStripQuotes(context.QuotedString().GetText());

            if (!parent.ValueType.IsStringIndexable())
            {
                throw new ParseException(ErrorMessages.UnexpectedArrayIndexer);
            }

            MapIndexStep step = new MapIndexStep(keyValue, parent.ValueType.ItemType, parent, apiInformation);
            context.PathStep = bindUniverse.EnsureUniquePathStep(step);
        }
        #endregion

        #region Casts
        public override void ExitPathDotAttached([NotNull] BindingPathParser.PathDotAttachedContext context)
        {
            string typeName;
            string propertyName;
            BindingPathParser.Static_typeContext static_type = context.attached_expr().static_type();
            if (static_type != null)
            {
                typeName = static_type.GetText();
                propertyName = context.attached_expr().IDENTIFIER()[0].GetText();
            }
            else
            {
                typeName = context.attached_expr().IDENTIFIER()[0].GetText();
                propertyName = context.attached_expr().IDENTIFIER()[1].GetText();
            }

            XamlType type = resolver.ResolveXmlName(typeName);
            if (type == null)
            {
                throw new ParseException(ErrorMessages.TypeNotFound, typeName);
            }

            XamlMember attachableProperty = type.GetAttachableMember(propertyName);
            BindPathStep parent = EnsureNotFunction(context.path().PathStep);

            BindPathStep step;
            if (null != attachableProperty && attachableProperty.IsNameValid)
            {
                step = new AttachedPropertyStep(propertyName, attachableProperty.Type, type, parent, apiInformation);
            }
            else
            {
                XamlType valueType = GetDirectPropertyOrFieldType(type, propertyName);
                if (valueType != null)
                {
                    BindPathStep castStep = bindUniverse.EnsureUniquePathStep(new CastStep(type, parent, apiInformation));
                    PropertyStep propStep = new PropertyStep(propertyName, valueType, castStep, apiInformation);

                    step = propStep;
                }
                else
                {
                    throw new ParseException(ErrorMessages.PropertyNotFound, propertyName, typeName);
                }
            }
            context.PathStep = bindUniverse.EnsureUniquePathStep(step);
        }

        public override void ExitPathCastInvalid([NotNull] BindingPathParser.PathCastInvalidContext context)
        {
            throw new ParseException(ErrorMessages.CastCannotStartWithAttachedProperty);
        }

        public override void ExitPathCast([NotNull] BindingPathParser.PathCastContext context)
        {
            // (local:foo) or (Button) - no path, just the cast
            // "<CheckBox IsChecked='{x:Bind (bool)}'/>"

            context.PathStep = CreateCastStep(context.cast_expr(), bindUniverse.RootStep);
        }

        public override void ExitPathCastPath([NotNull] BindingPathParser.PathCastPathContext context)
        {
            // (string)foo.bar - casts the leaf of the expr
            //"<TextBlock Text='{x:Bind (sys:String)SomeButton.Tag}'/>"

            context.PathStep = CreateCastStep(context.cast_expr(), EnsureNotFunction(context.path().PathStep));
        }

        public override void ExitPathCastPathParen([NotNull] BindingPathParser.PathCastPathParenContext context)
        {
            // ((string)foo.bar) - casts the leaf of the expr, usable for sub sections of the path
            // "<TextBlock Text='{x:Bind ((sys:String)SomeButton.Tag).Length}'/>"

            context.PathStep = CreateCastStep(context.cast_expr(), EnsureNotFunction(context.path().PathStep));
        }
        #endregion

        #region Functions
        public override void ExitPathFunction([NotNull] BindingPathParser.PathFunctionContext context)
        {
            string methodName = context.function().IDENTIFIER().GetText();
            BindPathStep resolvedStep = ResolveNameOnRoot(methodName);
            BindingPathParser.Function_paramContext[] paramArray = context.function().function_param();

            context.PathStep = CreateFunctionStep(resolvedStep as MethodStep, paramArray);
        }

        public override void ExitPathStaticFuction([NotNull] BindingPathParser.PathStaticFuctionContext context)
        {
            BindPathStep parent = CreateStaticRootStep(context.static_type());
            string methodName = context.function().IDENTIFIER().GetText();
            BindPathStep resolvedStep = ResolveNameOnType(methodName, parent.ValueType, parent);
            BindingPathParser.Function_paramContext[] paramArray = context.function().function_param();

            context.PathStep = CreateFunctionStep(resolvedStep as MethodStep, paramArray);
        }

        public override void ExitPathPathToFunction([NotNull] BindingPathParser.PathPathToFunctionContext context)
        {
            BindPathStep parent = EnsureNotFunction(context.path().PathStep);
            string methodName = context.function().IDENTIFIER().GetText();
            BindPathStep resolvedStep = ResolveNameOnType(methodName, parent.ValueType, parent);
            BindingPathParser.Function_paramContext[] paramArray = context.function().function_param();

            context.PathStep = CreateFunctionStep(resolvedStep as MethodStep, paramArray);
        }
        #endregion

        #region Function Parameters
        public override void ExitFunctionParameterInvalid([NotNull] BindingPathParser.FunctionParameterInvalidContext context)
        {
            throw new ParseException(ErrorMessages.FunctionAsParameter);
        }

        public override void ExitFunctionParamPath([NotNull] BindingPathParser.FunctionParamPathContext context)
        {
            BindPathStep value = context.path().PathStep;
            context.Param = new FunctionPathParam(value);
        }

        public override void ExitFunctionParamBool([NotNull] BindingPathParser.FunctionParamBoolContext context)
        {
            string value = context.boolean_value().GetText();
            context.Param = new FunctionBoolParam(bool.Parse(value.Replace("x:", string.Empty)));
        }

        public override void ExitFunctionParamNumber([NotNull] BindingPathParser.FunctionParamNumberContext context)
        {
            string value = context.decimal_value().GetText();
            context.Param = new FunctionNumberParam(value);
        }

        public override void ExitFunctionParamString([NotNull] BindingPathParser.FunctionParamStringContext context)
        {
            string value = UnescapeAndStripQuotes(context.QuotedString().GetText());
            context.Param = new FunctionStringParam(value);
        }

        public override void ExitFunctionParamNullValue([NotNull] BindingPathParser.FunctionParamNullValueContext context)
        {
            context.Param = new FunctionNullValueParam();
        }
        #endregion

        private BindPathStep CreateStaticRootStep(BindingPathParser.Static_typeContext static_type)
        {
            string namespaceQualifier = static_type.namespace_qualifier()?.GetText();
            string typeName = static_type.IDENTIFIER().GetText();
            if (namespaceQualifier != null)
            {
                typeName = namespaceQualifier + typeName;
            }

            XamlType type = resolver.ResolveXmlName(typeName);
            if (type == null)
            {
                throw new ParseException(ErrorMessages.TypeNotFound, typeName);
            }
            return bindUniverse.EnsureUniquePathStep(new StaticRootStep(type, apiInformation));
        }

        private BindPathStep CreateCastStep(BindingPathParser.Cast_exprContext castExp, BindPathStep parentStep)
        {
            BindingPathParser.Static_typeContext static_type = castExp.static_type();
            string targetTypeName = static_type != null ? static_type.GetText() : castExp.IDENTIFIER().GetText();

            XamlType targetType = resolver.ResolveXmlName(targetTypeName);
            if (targetType == null)
            {
                throw new ParseException(ErrorMessages.TypeNotFound, targetTypeName);
            }

            XamlType sourceType = parentStep.ValueType;
            if (!resolver.CanInlineConvert(sourceType, targetType))
            {
                throw new ParseException(ErrorMessages.InvalidCast, sourceType, targetType);
            }

            return bindUniverse.EnsureUniquePathStep(new CastStep(targetType, parentStep, apiInformation));
        }

        private BindPathStep CreateFunctionStep(MethodStep method, BindingPathParser.Function_paramContext[] paramArray)
        {
            if (method == null)
            {
                throw new ParseException(ErrorMessages.ExpectingMethod);
            }

            List<FunctionParam> parameters = new List<FunctionParam>();
            foreach (BindingPathParser.Function_paramContext param in paramArray)
            {
                parameters.Add(param.Param);
            }

            // Parameter validation
            if (parameters.Count() != method.Parameters.Count())
            {
                if (method.IsOverloaded)
                {
                    try
                    {
                        method = method.GetOverload(parameters.Count());
                        method = bindUniverse.EnsureUniquePathStep(method) as MethodStep;
                    }
                    catch (ArgumentException)
                    {
                        throw new ParseException(ErrorMessages.NoMatchingOverload, parameters.Count());
                    }
                }
                else
                {
                    throw new ParseException(ErrorMessages.MissmatchedParameterCount);
                }
            }

            for (int iParam = 0; iParam < parameters.Count; iParam++)
            {
                Parameter methodParam = method.Parameters[iParam];
                FunctionParam funcParam = parameters[iParam];

                // Parameters are not ref/out
                if (methodParam.IsOut || methodParam.ParameterType.IsByRef)
                {
                    throw new ParseException(ErrorMessages.UnsuportedOutParameter, methodParam.Position + 1);
                }

                XamlType actualParamType = resolver.ResolveType(methodParam.ParameterType);
                try
                {
                    funcParam.SetParameterInfo(methodParam, actualParamType);
                }
                catch (ArgumentException)
                {
                    throw new ParseException(ErrorMessages.InvalidParameter, methodParam.Position + 1);
                }

                FunctionPathParam pathParam = methodParam as FunctionPathParam;
                if (pathParam != null && !resolver.CanAssignDirectlyTo(pathParam.ValueType, actualParamType))
                {
                    throw new ParseException(ErrorMessages.InvalidParameter, methodParam.Position + 1);
                }
            }

            // Static vs. Instance validation
            if (method.Parent is StaticRootStep && !method.IsStatic)
            {
                throw new ParseException(ErrorMessages.ExpectingStaticFunction, method.MethodName, method.OwnerType.Name);
            }

            return bindUniverse.EnsureUniquePathStep(new FunctionStep(method, parameters, apiInformation));
        }

        private XamlType GetDirectPropertyOrFieldType(XamlType sourceType, string propertyName)
        {
            XamlMember property = sourceType.GetMember(propertyName);
            if (property != null && property.IsNameValid && property.UnderlyingMember.MemberType == System.Reflection.MemberTypes.Property)
            {
                return property.Type;
            }

            PropertyInfo propertyInfo = sourceType.UnderlyingType.GetProperty(propertyName);
            if (propertyInfo != null)
            {
                return sourceType.SchemaContext.GetXamlType(propertyInfo.PropertyType);
            }

            FieldInfo fieldInfo = sourceType.UnderlyingType.GetField(propertyName);
            if (fieldInfo != null)
            {
                return sourceType.SchemaContext.GetXamlType(fieldInfo.FieldType);
            }

            return null;
        }

        private static string UnescapeAndStripQuotes(string quotedString)
        {
            string result = quotedString;
            if ((quotedString.StartsWith("'") && quotedString.EndsWith("'")) ||
                (quotedString.StartsWith("\"") && quotedString.EndsWith("\"")))
            {
                result = quotedString.Substring(1, quotedString.Length - 2);
            }
            return result.Replace("^'", "'").Replace("^\"", "\"");
        }

        private BindPathStep EnsureNotFunction(BindPathStep step)
        {
            if (step is FunctionStep)
            {
                throw new ParseException(ErrorMessages.FunctionNotLeaf);
            }
            return step;
        }

        private BindPathStep ResolveNameOnRoot(string name)
        {
            BindPathStep dataTypeMemberStep = null;
            BindPathStep namedElementStep = null;

            string objectCodeName;
            XamlType namedElementType = bindUniverse.GetNamedElementType(name, out objectCodeName);
            bool fieldExists = bindUniverse.GetNamedFieldType(name) != null;

            // Check for a named elements first. For templates, the datatype resolution has higher priority,
            // but we do this first so we can throw the original datatype resolution exception.
            if (namedElementType != null)
            {
                string updateParamOverride = fieldExists ? null : objectCodeName;
                BindPathStep rootStep = fieldExists ? bindUniverse.RootStep : bindUniverse.MakeOrGetRootStepOutOfScope();

                namedElementStep = new RootNamedElementStep(name, namedElementType, rootStep, apiInformation, updateParamOverride);
            }

            // Try resolving the name as a field, property, method, etc. on the data type.
            // If we could also resolve the step as a named element, we should suppress any
            // parse exceptions thrown here as they aren't actually issues - we'd use
            // the named element resolution instead.
            try
            {
                dataTypeMemberStep = ResolveNameOnType(name, bindUniverse.RootStep.ValueType, bindUniverse.RootStep);
            }
            catch(ParseException)
            {
                // Rethrow the parse exception if we also couldn't resolve as a named element,
                // as it means we genuinely couldn't resolve the named element step as anything valid.
                // Otherwise, suppress the exception as we'll just use namedElementStep.
                if (namedElementStep == null)
                {
                    throw;
                }

                // If we could resolve the path as a named element, but not a valid member,
                // check whether a member with the name exists at all. We could've correctly
                // failed to retrieve it because the member was invalid (e.g. didn't have a
                // public getter), but the user may think they have bound to that incorrectly
                // configured member instead of the named element. Raise a warning if we
                // detect that case.
                if (bindUniverse.RootStep.ValueType.GetMember(name) != null)
                {
                    Warnings?.Add(string.Format(ErrorMessages.UnbindableMemberConflict, name,
                        bindUniverse.RootStep.ValueType.UnderlyingType.FullName));
                }
            }

            // For templates, if we found both a datatype field and named element with the name,
            // raise a warning we'll always use the datatype field. We ignore the warning for
            // the file root, since named elements will always generate a field on the root
            // page/control, so we always see this behavior. It's OK since the field always
            // refers to the matching named element.
            if (dataTypeMemberStep != null && namedElementStep != null && !fieldExists)
            {
                Warnings?.Add(string.Format(ErrorMessages.UsingNamedElement, name,
                       bindUniverse.RootStep.ValueType.UnderlyingType.FullName));
            }

            if (fieldExists)
            {
                // For non-templates, we prioritize named elements over fields on the root.
                return namedElementStep ?? dataTypeMemberStep;
            }
            else
            {
                // For templates, we do the opposite.
                return dataTypeMemberStep ?? namedElementStep;
            }
        }

        private BindPathStep ResolveNameOnType(string name, XamlType type, BindPathStep parentStep)
        {
            MemberInfo[] memberInfos = GetMembers(type.UnderlyingType, name);
            if (memberInfos.Length == 0)
            {
                throw new ParseException(ErrorMessages.PropertyNotFound, name, type.Name);
            }

            switch (memberInfos[0].MemberType)
            {
                case MemberTypes.Property:
                    {
                        PropertyInfo propertyInfo = memberInfos[0] as PropertyInfo;
                        XamlType valueType = type.SchemaContext.GetXamlType(propertyInfo.PropertyType);
                        bool isDP = type.UnderlyingType.IsDependencyProperty(name);

                        if (isDP)
                        {
                            return new DependencyPropertyStep(name, valueType, parentStep, apiInformation);
                        }
                        else
                        {
                            MethodInfo getAccessor = propertyInfo.GetGetMethod(true);
                            if (getAccessor == null)
                            {
                                throw new ParseException(ErrorMessages.PropertyWithoutGet, name, type.Name);
                            }
                            if (!getAccessor.IsStatic && (parentStep is StaticRootStep))
                            {
                                throw new ParseException(ErrorMessages.ExpectingStaticProperty, name, type.Name);
                            }
                            return new PropertyStep(name, valueType, parentStep, apiInformation);
                        }
                    }
                case MemberTypes.Field:
                    {
                        XamlType valueType = type.SchemaContext.GetXamlType(((FieldInfo)memberInfos[0]).FieldType);
                        return new FieldStep(name, valueType, parentStep, apiInformation);
                    }
                case MemberTypes.Method:
                    {
                        return new MethodStep(memberInfos, type, parentStep, apiInformation);
                    }
                default:
                    {
                        string memberType = memberInfos[0].MemberType.ToString();
                        throw new ArgumentException($"Unexpected member type '{memberType}' when binding to '{name}'");
                    }
            }
        }

        private MemberInfo[] GetMembers(Type type, string name)
        {
            const BindingFlags dpFlags = BindingFlags.FlattenHierarchy |
                                         BindingFlags.GetField | BindingFlags.GetProperty |
                                         BindingFlags.Static | BindingFlags.Public;
            const BindingFlags memberFlags = dpFlags |
                                         BindingFlags.NonPublic | BindingFlags.Instance;

            MemberInfo[] memberInfos = type.GetMember(name, memberFlags);
            if (memberInfos.Length == 0)
            {
                // We also need to look at the list of interfaces, as that's how certain language specific types
                // like IList<T> or IVector<T> are projected.
                foreach (Type interfaceType in type.GetInterfaces())
                {
                    memberInfos = interfaceType.GetMember(name, memberFlags);
                    if (memberInfos.Length > 0)
                    {
                        return memberInfos;
                    }
                }
            }
            return memberInfos;
        }

    }
}