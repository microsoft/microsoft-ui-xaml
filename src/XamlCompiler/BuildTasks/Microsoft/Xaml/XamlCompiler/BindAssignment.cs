// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Globalization;
using System.Linq;
using System.Xaml;
using System.Collections.Generic;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    using CodeGen;
    using XamlDom;
    using Properties;
    using Utilities;

    internal abstract class BindAssignment : BindAssignmentBase, IBindAssignment
    {
        private XamlDomMember fallbackMember;

        public static BindAssignment Create(XamlDomMember bindMember, BindUniverse bindUniverse, ConnectionIdElement connectionIdElement)
        {
            if (DomHelper.IsLoadMember(bindMember))
            {
                return new BoundLoadAssignment(bindMember, bindUniverse, connectionIdElement);
            }
            else
            {
                return new BoundPropertyAssignment(bindMember, bindUniverse, connectionIdElement);
            }
        }

        public BindPathStep BindBackStep { get; private set; }
        public BindStatus BindStatus { get; set; }
        public string BindBackPath { get; }
        public String TargetNullValue { get; }
        public String Converter { get; }
        public String ConverterParameter { get; }
        public String ConverterLanguage { get; }
        public UpdateSourceTrigger UpdateSourceTrigger { get; }

        protected BindAssignment(XamlDomMember bindMember, BindUniverse bindUniverse, ConnectionIdElement connectionIdElement)
            : base(bindMember, bindUniverse, connectionIdElement)
        {
            this.BindBackPath = DomHelper.GetStringValueOfProperty(this.bindItem.GetMemberNode(KnownStrings.BindBack));

            this.BindStatus = BindStatus.HasBinding;

            // Check for the mode's definition in the actual x:Bind member
            string mode = DomHelper.GetStringValueOfProperty(this.bindItem.GetMemberNode(KnownStrings.Mode));

            // If the mode wasn't specified on the x:Bind member, use the user-specified default bind mode for the object.
            // It's fine if the user didn't specify a default mode and it's null, the code below will behave as if it was OneTime.
            if (mode == null && connectionIdElement.DefaultBindMode != null)
            {
                mode = connectionIdElement.DefaultBindMode;
            }

            if (mode != null)
            {
                if (!mode.Equals(KnownStrings.OneTime, StringComparison.InvariantCultureIgnoreCase))
                {
                    this.BindStatus |= BindStatus.TracksSource;
                }
                if (mode.Equals(KnownStrings.TwoWay, StringComparison.InvariantCultureIgnoreCase))
                {
                    this.BindStatus |= BindStatus.TracksTarget;
                }
            }

            fallbackMember = this.bindItem.GetMemberNode(KnownStrings.FallbackValue);
            if (this.fallbackMember != null)
            {
                this.BindStatus |= BindStatus.HasFallbackValue;
            }

            this.TargetNullValue = DomHelper.GetStringValueOfProperty(this.bindItem.GetMemberNode(KnownStrings.TargetNullValue));
            if (this.TargetNullValue != null)
            {
                this.BindStatus |= BindStatus.HasTargetNullValue;
            }

            XamlDomMember member = this.bindItem.GetMemberNode(KnownStrings.Converter);
            if (member != null)
            {
                this.Converter = DomHelper.GetStaticResource_ResourceKey(member.Item as XamlDomObject);
                this.BindStatus |= BindStatus.HasConverter;
            }
            this.ConverterParameter = DomHelper.GetStringValueOfProperty(this.bindItem.GetMemberNode(KnownStrings.ConverterParameter));
            this.ConverterLanguage = DomHelper.GetStringValueOfProperty(this.bindItem.GetMemberNode(KnownStrings.ConverterLanguage));

            member = this.bindItem.GetMemberNode(KnownStrings.UpdateSourceTrigger);
            if (member != null)
            {
                UpdateSourceTrigger result;
                Enum.TryParse<UpdateSourceTrigger>(DomHelper.GetStringValueOfProperty(member), out result);
                this.UpdateSourceTrigger = result;
            }
        }

        public string CodeName
        {
            get { return String.Format(CultureInfo.InvariantCulture, "obj{0}_{1}", this.ConnectionIdElement.ConnectionId, this.MemberName); }
        }

        public int ComputedPhase
        {
            get { return (ConnectionIdElement?.PhaseAssignment != null) ? ConnectionIdElement.PhaseAssignment.Phase : 0; }
        }

        public bool IsTrackingSource
        {
            get { return this.BindStatus.HasFlag(BindStatus.TracksSource); }
        }

        public bool IsTrackingTarget
        {
            get { return this.BindStatus.HasFlag(BindStatus.TracksTarget); }
        }

        public LanguageSpecificString FallbackValueExpression
        {
            get
            {
                var fallbackMemberAsObject = this.fallbackMember.Item as XamlDomObject;
                if (fallbackMemberAsObject != null)
                {
                    if (fallbackMemberAsObject.Type.IsNullExtension())
                    {
                        return LanguageSpecificString.Null;
                    }
                }
                else
                {
                    var fallbackMemberValue = DomHelper.GetStringValueOfProperty(fallbackMember);
                    if (fallbackMemberValue != null)
                    {
                        if (MemberType.IsString())
                        {
                            return new LanguageSpecificString(
                                () => fallbackMemberValue.Quotenate(),
                                () => $"L{fallbackMemberValue.Quotenate()}",
                                () => fallbackMemberValue.Quotenate(),
                                () => fallbackMemberValue.Quotenate()
                                );
                        }
                        else
                        {
                            return MemberType.GetStringToThing(fallbackMemberValue.Quotenate(), true);
                        }
                    }
                }

                // Note: this case represents an invalid FallbackValue, like a StaticResource,
                // and we should never actually use this value.
                return null;
            }
        }

        internal LanguageSpecificString TargetNullValueExpression
        {
            get
            {
                if (MemberType.IsNullable)
                {
                    if (TargetNullValue == null)
                    {
                        return new LanguageSpecificString(
                            () => LanguageSpecificString.Null.CppCXName(),
                            () => "std::nullopt",
                            () => LanguageSpecificString.Null.CSharpName(),
                            () => LanguageSpecificString.Null.VBName()
                            ); ;
                    }
                    else
                    {
                        return new LanguageSpecificString(
                            () => TargetNullValue.Quotenate(),
                            () => $"L{TargetNullValue.Quotenate()}",
                            () => TargetNullValue.Quotenate(),
                            () => TargetNullValue.Quotenate()
                            );
                    }
                }
                else
                {
                    return new LanguageSpecificString(() => "");
                }
            }
        }

        public XamlType ValueType
        {
            get { return this.PathStep.ValueType; }
        }

        public bool NeedsBox
        {
            get
            {
                return ValueType.NeedsBoxUnbox() && MemberType.IsObject();
            }
        }

        public bool NeedsLostFocusForTwoWay
        {
            get
            {
                switch (this.UpdateSourceTrigger)
                {
                    case UpdateSourceTrigger.LostFocus:
                        return true;
                    case UpdateSourceTrigger.PropertyChanged:
                        return false;
                    case UpdateSourceTrigger.Default:
                        // Use previous method of deciding when to use LostFocus
                        return this.MemberName == KnownMembers.Text && this.ConnectionIdElement.Type.IsDerivedFromTextBox();
                    default:
                        throw new ArgumentException($"Unexpected UpdateSourceTrigger '{this.UpdateSourceTrigger}'");
                }
            }
        }

        public XamlType MemberTargetType
        {
            get { return bindMember.Member.TargetType; }
        }
        
        public string DisableFlagName
        {
            get { return $"is{ConnectionIdElement.ObjectCodeName}{MemberName}Disabled"; }
        }

        public LanguageSpecificString GetConverterExpression(LanguageSpecificString objectExpression, bool convertBack)
        {
            XamlType converterType = convertBack ? this.PathStep.ValueType : this.MemberType;
            string convertName = convertBack ? "ConvertBack" : "Convert";

            string cxConverter = String.Format(
                "safe_cast<{0}>(this->LookupConverter(\"{1}\")->{6}({5}, {2}::typeid, {3}, {4}))",
                converterType.CppCXName(),
                this.Converter,
                converterType.CppCXName(false),
                this.ConverterParameter == null ? "nullptr" : this.ConverterParameter.Quotenate(),
                this.ConverterLanguage == null ? "nullptr" : this.ConverterLanguage.Quotenate(),
                objectExpression.CppCXName(),
                convertName);

            string cppConverter = String.Format(
                    "::winrt::unbox_value<{0}>(LookupConverter(L\"{1}\").{6}(::winrt::box_value({5}), ::winrt::xaml_typename<{2}>(), {3}, {4}))",
                    converterType.CppWinRTName(),
                    this.Converter,
                    converterType.CppWinRTName(),
                    this.ConverterParameter == null ? "nullptr" : $"::winrt::box_value(::winrt::hstring(L\"{this.ConverterParameter}\"))",
                    this.ConverterLanguage == null ? "::winrt::hstring{}" : $"L\"{this.ConverterLanguage}\"",
                    objectExpression.CppWinRTName(),
                    convertName);

            string csConverter = String.Format(
                "({0})this.LookupConverter(\"{1}\").{5}({4}, typeof({0}), {2}, {3})",
                converterType.CSharpName(),
                this.Converter,
                this.ConverterParameter == null ? "null" : this.ConverterParameter.Quotenate(),
                this.ConverterLanguage == null ? "null" : this.ConverterLanguage.Quotenate(),
                objectExpression.CSharpName(),
                convertName);

            string vbConverter = String.Format(
                "DirectCast(Me.LookupConverter(\"{1}\").{5}({4}, GetType({0}), {2}, {3}),{0})",
                converterType.VBName(),
                this.Converter,
                this.ConverterParameter == null ? "Nothing" : this.ConverterParameter.Quotenate(),
                this.ConverterLanguage == null ? "Nothing" : this.ConverterLanguage.Quotenate(),
                objectExpression.VBName(),
                convertName);

            return new LanguageSpecificString(
                () => cxConverter,
                () => cppConverter,
                () => csConverter,
                () => vbConverter);
        }

        public LanguageSpecificString DirectAssignmentExpression(string expression)
        {
            return GetAssignmentExpression(ValueType, MemberType, new LanguageSpecificString(() => expression), false);
        }

        public LanguageSpecificString ReverseAssignmentExpression
        {
            get
            {
                var memberExpression = this.ConnectionIdElement.GetMemberGetExpression(this);
                return GetAssignmentExpression(MemberType, ValueType, memberExpression, true);
            }
        }

        private LanguageSpecificString GetAssignmentExpression(XamlType source, XamlType target, LanguageSpecificString expression, bool convertBack)
        {
            if (this.BindStatus.HasFlag(BindStatus.HasConverter))
            {
                return this.GetConverterExpression(expression, convertBack);
            }
            else if (source.CanAssignDirectlyTo(target))
            {
                return expression;
            }
            else if (source.CanInlineConvert(target))
            {
                return source.GetInlineConversionExpression(target, expression);
            }
            else if (target.IsString())
            {
                return source.ToStringWithNullCheckExpression(expression);
            }
            else if (source.IsString())
            {
                return target.GetStringToThing(expression);
            }
            else
            {
                throw new ArgumentException($"Don't know how to generate code for bind assignment at line {this.LineNumberInfo.StartLineNumber}");
            }
        }


        public IEnumerable<XamlCompileErrorBase> ParsePath()
        {
            var issues = new List<XamlCompileErrorBase>();
            string currentPath = "";
            try
            {
                var warnings = new List<string>();
                currentPath = GetBindingPath(this.bindItem);
                this.PathStep = ParseBindPath(warnings);
                if (!string.IsNullOrEmpty(this.BindBackPath))
                {
                    currentPath = this.BindBackPath;
                    this.BindBackStep = ParseBindPath(this.BindBackPath, warnings);
                }
                foreach(var warning in warnings)
                {
                    issues.Add(new XamlXBindParseWarning(bindItem, warning));
                }
            }
            catch(CompiledBindingParseException ex)
            {
                issues.Add(new XamlXBindParseError(this.bindItem, ex));
                return issues;
            }
            catch (ParseException ex)
            {
                issues.Add(new XamlXBindParseError(this, this.bindItem.StartLinePosition, currentPath, ex.Message));
                return issues;
            }

            ApplySpecialCaseCasting();
            this.PathStep.AddBindAssignment(this);

            if (!ValidatePathAssignment(issues) ||
                !ValidateMode(issues) ||
                !ValidateTypeCasting(issues) ||
                !ValidateBindBackAssignment(issues) ||
                !ValidateConverter(issues) ||
                !ValidateApiInformation(issues) ||
                !ValidateUpdateSourceTrigger(issues) ||
                !ValidateFallbackValue(issues))
            {
                return issues;
            }

            // Success code path
            return issues;
        }

        private bool ValidateConverter(IList<XamlCompileErrorBase> issues)
        { 
            // Verify that we have a converter if we really need one
            if (this.Converter == null)
            { 
                bool dontNeedConverter =
                    this.ValueType.IsString() ||    // Can use XamlBindingHelper::ConvertValue() which takes a string
                    this.MemberType.IsString();     // Can use .ToString() or ->ToString()

                bool canAssignTo = this.ValueType.CanAssignDirectlyTo(this.MemberType) || this.ValueType.CanBoxTo(this.MemberType);
                bool canAssignBack = this.MemberType.CanAssignDirectlyTo(this.ValueType) || this.MemberType.CanInlineConvert(this.ValueType);

                if (!dontNeedConverter && !this.IsTrackingTarget)
                {
                    // For One Time and One Way bindings, we can assign directly from model to property.
                    dontNeedConverter |= canAssignTo;
                }

                if (!dontNeedConverter && this.IsTrackingTarget)
                {
                    // For Two Way bindings, we can assign directly from both model to property and viceversa.
                    dontNeedConverter |= (canAssignTo && canAssignBack);
                }

                if (!dontNeedConverter)
                {
                    issues.Add(new BindAssignmentValidationError(this.bindItem, ResourceUtilities.FormatString(
                        XamlCompilerResources.BindAssignment_NeedsConverter, this.ValueType, this.MemberType)));
                    return false;
                }

                if (this.ConverterParameter != null)
                {
                    issues.Add(new BindAssignmentValidationError(this.bindItem, ResourceUtilities.FormatString(
                        XamlCompilerResources.BindAssignment_OrphanConverterParam, KnownStrings.ConverterParameter)));
                    return false;
                }
                if (this.ConverterLanguage != null)
                {
                    issues.Add(new BindAssignmentValidationError(this.bindItem, ResourceUtilities.FormatString(
                        XamlCompilerResources.BindAssignment_OrphanConverterParam, KnownStrings.ConverterLanguage)));
                    return false;
                }
            }

            if (this.Converter != null && this.PathStep is FunctionStep)
            {
                issues.Add(new BindAssignmentValidationError(this.bindItem,
                    XamlCompilerResources.BindAssignment_ConverterWithFunctionBindingNotSupported));
                return false;
            }
            return true;
        }

        private bool ValidatePathAssignment(IList<XamlCompileErrorBase> issues)
        {
            foreach (BindPathStep step in this.PathStep.ParentsAndSelf)
            {
                if (step is MethodStep)
                {
                    issues.Add(new BindAssignmentValidationError(this.bindItem,
                        XamlCompilerResources.BindPathParser_CantBindToMethods));
                    return false;
                }
            }

            if ( this.PathStep is FunctionStep && 
                !this.PathStep.ValueType.CanAssignDirectlyTo(this.MemberType))
            {
                if (!this.MemberType.IsString() || this.PathStep.ValueType.IsVoid())
                {
                    // Function return type must match the binding target, unless the target is
                    // a string so we can call ConvertValue on the return of the function.
                    issues.Add(new BindAssignmentValidationError(this.bindItem, ResourceUtilities.FormatString(
                        XamlCompilerResources.BindAssignment_FunctionReturnTypeInvalid,
                        this.PathStep.ValueType.UnderlyingType.FullName, this.MemberType)));
                    return false;
                }
            }
            return true;
        }

        private bool ValidateBindBackAssignment(IList<XamlCompileErrorBase> issues)
        {
            if (this.PathStep is FunctionStep && this.BindStatus.HasFlag(BindStatus.TracksTarget))
            {
                if (this.BindBackStep == null)
                {
                    // BindBack was expected but not found
                    issues.Add(new BindAssignmentValidationError(this.bindItem,
                        XamlCompilerResources.BindAssignment_BindBack_NotFound));
                    return false;
                }

                if ( !(this.BindBackStep is MethodStep))
                {
                    // BindBack was found but it's not a method
                    issues.Add(new BindAssignmentValidationError(this.bindItem,
                        XamlCompilerResources.BindAssignment_BindBack_NotMethod));
                    return false;
                }

                MethodStep bindBackMethodStep = this.BindBackStep as MethodStep;
                if ( bindBackMethodStep.Parameters.Count != 1 || 
                     !this.MemberType.CanAssignDirectlyTo(bindBackMethodStep.Parameters[0].ParameterType))
                {
                    // BindBack was found but it does not have the right parameters
                    issues.Add(new BindAssignmentValidationError(this.bindItem, ResourceUtilities.FormatString(
                        XamlCompilerResources.BindAssignment_BindBack_InvalidMethod, this.MemberType)));
                    return false;
                }
            }
            else
            {
                if (this.BindBackStep != null)
                {
                    // Unexpected BindBack member found on a binding that's not a two way function
                    issues.Add(new BindAssignmentValidationError(this.bindItem,
                        XamlCompilerResources.BindAssignment_BindBack_Unexpected));
                    return false;
                }
            }
            return true;
        }

        // This method looks like a hack, but we did it, so we can't take it out now.
        // There's probably a better way to do this. Marking for a future TODO.
        private void ApplySpecialCaseCasting()
        {
            // artificially append a cast for bool to visibility
            if (this.Converter == null && this.MemberType.UnderlyingType.FullName == KnownTypes.Visibility)
            {
                if (this.ValueType.IsBoolOrNullableBool())
                {
                    BindPathStep cast = new CastStep(this.MemberType, this.PathStep, this.ApiInformation);
                    this.PathStep = BindUniverse.EnsureUniquePathStep(cast);
                }
            }
        }

        private bool ValidateTypeCasting(IList<XamlCompileErrorBase> issues)
        {
            // not allowed to two way if the path leaf is a cast like in <TextBlock Text="{x:Bind (x:String), Mode=TwoWay}" />
            if (this.PathStep is CastStep && this.IsTrackingTarget)
            {
                issues.Add(new BindAssignmentValidationError(this.bindItem,
                    XamlCompilerResources.BindPathParser_CantTwoWayCastStep));
                return false;
            }
            return true;
        }

        private bool ValidateApiInformation(IList<XamlCompileErrorBase> issues)
        {
            if (this.ApiInformation == null)
            {
                // A bind assignment must have a conditional namespace on the member or type,
                // if any of the bind path steps, or function parameters are of conditional types.
                var pathStep = this.PathStep;
                while (pathStep != null)
                {
                    if (pathStep.ValueTypeIsConditional)
                    {
                        issues.Add(new BindAssignmentValidationError(this.bindItem,
                            XamlCompilerResources.BindAssignment_RequiresConditionalNamespace));
                        return false;
                    }
                    pathStep = pathStep.Parent;
                }
            }
            return true;
        }

        private bool ValidateUpdateSourceTrigger(IList<XamlCompileErrorBase> issues)
        {
            var member = this.bindItem.GetMemberNode(KnownStrings.UpdateSourceTrigger);
            if (member != null)
            {
                UpdateSourceTrigger updateSourceTrigger = UpdateSourceTrigger.Default;

                if (!Enum.TryParse<UpdateSourceTrigger>(DomHelper.GetStringValueOfProperty(member), out updateSourceTrigger))
                {
                    issues.Add(new BindAssignmentValidationError(this.bindItem,
                        XamlCompilerResources.BindAssignment_UpdateSourceTrigger_UnrecognizedValue));
                    return false;
                }

                if (updateSourceTrigger == UpdateSourceTrigger.Explicit)
                {
                    issues.Add(new BindAssignmentValidationError(this.bindItem,
                        XamlCompilerResources.BindAssignment_UpdateSourceTrigger_ExplicitUnsupported));
                    return false;
                }

                if (!BindStatus.HasFlag(BindStatus.TracksTarget))
                {
                    issues.Add(new BindAssignmentValidationError(this.bindItem,
                        XamlCompilerResources.BindAssignment_UpdateSourceTrigger_UpdateSourceTriggerOnlyWithTwoWay));
                    return false;
                }

                if (updateSourceTrigger == UpdateSourceTrigger.LostFocus)
                {
                    var lostFocus = this.MemberDeclaringType.GetMember(KnownStrings.LostFocus);
                    if (lostFocus == null || !lostFocus.IsEvent)
                    {
                        issues.Add(new BindAssignmentValidationError(this.bindItem, ResourceUtilities.FormatString(
                            XamlCompilerResources.BindAssignment_UpdateSourceTrigger_LostFocusEventRequired)));
                        return false;
                    }
                }

                if (updateSourceTrigger == UpdateSourceTrigger.PropertyChanged)
                {
                    if (!DomHelper.IsDependencyProperty(this.bindMember))
                    {
                        issues.Add(new BindAssignmentValidationError(this.bindItem, ResourceUtilities.FormatString(
                            XamlCompilerResources.BindAssignment_UpdateSourceTrigger_PropertyChangedOnlyOnDP)));
                        return false;
                    }
                }
            }
            return true;
        }

        private bool ValidateMode(IList<XamlCompileErrorBase> issues)
        {
            // Check that the path supports notificatoins for
            // OneWay, but not TwoWay bindings.
            if (BindStatus.HasFlag(BindStatus.TracksSource) && 
                !BindStatus.HasFlag(BindStatus.TracksTarget))
            {
                var steps = new List<BindPathStep>() { PathStep };
                steps.AddRange(PathStep.Dependencies);
                var stepsWithParents = steps.SelectMany(s => s.ParentsAndSelf);
                if (!stepsWithParents.Any(s => s.RequiresChildNotification))
                {
                    issues.Add(new BindAssignmentValidationWarning(this.bindItem, ErrorCode.WMC1506,
                        XamlCompilerResources.BindAssignment_OneWay_NoWay));
                    
                    // no return, this is a warning.
                }
            }
            return true;
        }

        private bool ValidateFallbackValue(IList<XamlCompileErrorBase> issues)
        {
            // If we have a FallbackMember set in markup, but it isn't a valid expression,
            // error
            if (this.fallbackMember != null && FallbackValueExpression == null)
            {
                issues.Add(new BindAssignmentValidationError(this.bindItem,
                    XamlCompilerResources.BindAssignment_InvalidFallbackValue));
                return false;
            }
            return true;
        }
    }
}