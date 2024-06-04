// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Windows.Markup;
    using System.Xaml;
    using CodeGen;
    using DirectUI;
    using XamlDom;

    [ContentProperty("FieldDefinition")]
    internal class ConnectionIdElement
    {
        private List<EventAssignment> eventAssignments;
        private List<BindAssignment> bindAssignments;
        private List<BoundEventAssignment> boundEventAssignments;

        public ApiInformation ApiInformation { get; }
        public PhaseAssignment PhaseAssignment { get; }

        // Set to true if the connection ID element is a named element under a template which is also used in a binding.
        // Needed to rewrite the markup we pass to GenXBF to have a connection ID on it.
        public bool IsBoundNamedTemplateElement
        {
            get
            {
                return HasRootNamedElementStep && IsTemplateChild;
            }
        }

        // Set to true if this element is bound to by elements from different namescopes/bind universes.
        public bool IsUsedByOtherScopes { get; set; } = false;

        public bool HasRewritableAttributes
        {
            get
            {
                return HasEventAssignments || HasBoundEventAssignments || HasBindAssignments
                    || HasFieldDefinition || IsBoundNamedTemplateElement || IsBindingRoot || 
                    IsUsedByOtherScopes || (Type.IsDerivedFromFrameworkTemplate() && BindUniverse.NeededForOuterScopeElement);
            }
        }

        private ConnectionIdElement(XamlDomObject domObject, BindUniverse bindUniverse, XamlFileCodeInfo fileCodeInfo, XamlClassCodeInfo classCodeInfo, XamlType dataRootType)
        {
            this.Type = domObject.Type;
            this.ApiInformation = domObject.ApiInformation;

            this.ConnectionId = classCodeInfo.NextConnectionId;
            this.LineNumberInfo = new LineNumberInfo(domObject);
            this.ParentFileCodeInfo = fileCodeInfo;
            this.Children = new List<ConnectionIdElement>();

            //This has to be set before the BindAssignments are created since it's used in the BindAssignment constructor
            this.DefaultBindMode = DomHelper.GetDefaultBindMode(domObject);

            XamlDomMember nameMember = DomHelper.GetAliasedMemberNode(domObject, XamlLanguage.Name, forcePass1Eval: true);
            if (nameMember != null)
            {
                this.ElementName = DomHelper.GetStringValueOfProperty(nameMember);
            }

            if (bindUniverse == null)
            {
                bool isFileRoot = !DomHelper.UnderANamescope(domObject) && !DomHelper.IsDerivedFromControlTemplate(domObject) && !DomHelper.IsDerivedFromDataTemplate(domObject);
                bindUniverse = new BindUniverse(this, dataRootType, isFileRoot, classCodeInfo.ClassName.ShortName);
            }
            this.BindUniverse = bindUniverse;

            if (!String.IsNullOrEmpty(this.ElementName))
            {
                this.BindUniverse.NamedElements.Add(this);
            }

            foreach (XamlDomMember phaseMember in from mem in domObject.MemberNodes
                                                  where DomHelper.IsPhaseMember(mem.Member)
                                                  select mem)
            {
                this.PhaseAssignment = new PhaseAssignment(phaseMember, this);
                bindUniverse.AddPhase(this.PhaseAssignment);
                fileCodeInfo.HasPhaseAssignments = true;
            }

            foreach (XamlDomMember bindMember in from mem in domObject.MemberNodes
                                                 where mem.Items.Count == 1 &&
                                                     mem.Item is XamlDomObject &&
                                                     !mem.Member.IsEvent &&
                                                     domObject.SchemaContext is DirectUISchemaContext &&
                                                     ((XamlDomObject)mem.Item).Type == ((DirectUISchemaContext)domObject.SchemaContext).DirectUIXamlLanguage.BindExtension
                                                 select mem)
            {
                BindAssignment bindAssignment = BindAssignment.Create(bindMember, bindUniverse, this);
                BindAssignments.Add(bindAssignment);
                bindUniverse.BindAssignments.Add(bindAssignment);
                fileCodeInfo.BindStatus |= bindAssignment.BindStatus;
            }

            this.CanBeInstantiatedLater = DomHelper.CanBeInstantiatedLater(domObject);
            this.IsUnloadableRoot = DomHelper.HasLoadOrDeferLoadStrategyMember(domObject);
        }

        public ConnectionIdElement(
            XamlDomObject domObject,
            BindUniverse bindUniverse,
            XamlFileCodeInfo fileCodeInfo,
            XamlClassCodeInfo classCodeInfo,
            XamlType dataRootType,
            bool skipFieldDefinition)
            : this(domObject, bindUniverse, fileCodeInfo, classCodeInfo, dataRootType)
        {
            Type = domObject.Type;

            if (!skipFieldDefinition && !string.IsNullOrEmpty(this.ElementName))
            {
                this.FieldDefinition = new FieldDefinition(domObject);
            }

            foreach (XamlDomMember eventMember in from member in domObject.MemberNodes
                                                  where member.Member.IsEvent
                                                  select member)
            {
                if (eventMember.Member.IsAttachable)
                {
                    // We throw (rather than return) the error because the validator should have caught this already.
                    throw new ArgumentException("Attached Events are not supported");
                }
                else if (eventMember.Item is XamlDomObject && domObject.SchemaContext is DirectUISchemaContext && ((XamlDomObject)eventMember.Item).Type == ((DirectUISchemaContext)domObject.SchemaContext).DirectUIXamlLanguage.BindExtension)
                {
                    BoundEventAssignment beAssignment = new BoundEventAssignment(eventMember, this.BindUniverse, this);
                    this.BoundEventAssignments.Add(beAssignment);
                    this.BindUniverse.BoundEventAssignments.Add(beAssignment);
                    fileCodeInfo.BindStatus |= BindStatus.HasEventBinding;
                }
                else
                {
                    EventAssignment eventAssignment = new EventAssignment(eventMember);
                    this.EventAssignments.Add(eventAssignment);
                    fileCodeInfo.HasEventAssignments = true;
                }
            }
        }

        // Constructor and unknown Local Type, Pass1.  (named fields only)
        //
        public ConnectionIdElement(
            XamlDomObject domObject,
            BindUniverse bindUniverse,
            XamlFileCodeInfo fileCodeInfo,
            XamlClassCodeInfo classCodeInfo,
            XamlType dataRootType,
            bool skipFieldDefinition,
            string clrPath)
            : this(domObject, bindUniverse, fileCodeInfo, classCodeInfo, dataRootType)
        {
            Type = null;

            if (!skipFieldDefinition && !string.IsNullOrEmpty(this.ElementName))
            {
                this.FieldDefinition = new FieldDefinition(domObject, clrPath);
            }
        }

        public LineNumberInfo LineNumberInfo { get; set; }
        public XamlFileCodeInfo ParentFileCodeInfo { get; private set; }
        public XamlType Type { get; set; }
        public int ConnectionId { get; }
        public string ElementName { get; }
        public FieldDefinition FieldDefinition { get; set; }
        public bool HasFieldDefinition { get { return this.FieldDefinition != null; } }
        public RootNamedElementStep RootNamedElementStep { get; set; }
        public bool HasRootNamedElementStep { get { return this.RootNamedElementStep != null; } }
        public BindUniverse BindUniverse { get; set; }
        public bool HasEventAssignments { get { return this.eventAssignments != null && this.eventAssignments.Count > 0; } }
        public bool CanBeInstantiatedLater { get; private set; } // Does this node or any ancestors have x:DeferLoadStrategy or x:Load?
        public bool IsUnloadableRoot { get; private set; } // Does this node have the x:DeferLoadStrategy or x:Load?
        public string DefaultBindMode { get; private set; } //The default bind mode for this element - could be inherited from an ancestor.
        public IList<ConnectionIdElement> Children { get; }

        public List<EventAssignment> EventAssignments
        {
            get
            {
                if (this.eventAssignments == null)
                {
                    this.eventAssignments = new List<EventAssignment>();
                }
                return this.eventAssignments;
            }
        }

        // Normally, there is only 1 BindAssignment that matches the  Input property (i.e. Text="{x:Bind Foo}" ) and is
        // also marked required. The exception to this case is in Function binding, where multiple property steps are
        // used as parameters. See TryGetValidationContextStep for more details on how this is handled.
        public IEnumerable<BindAssignment> InputPropertyBindAssignments
        {
            get
            {
                return TwoWayBindAssignments.Where(ba => ba.IsInputPropertyAssignment);
            }
        }

        public bool IsValueRequired
        {
            get
            {
                return TryGetValidationContextStep(out PropertyStep bindStep) && bindStep.IsValueRequired;
            }
        }

        // Try to get the bind step associated with this validation control
        public bool TryGetValidationContextStep(out PropertyStep bindStep)
        {
            PropertyStep foundStep = null;
            if (this.Type.ImplementsIInputValidationControl())
            {
                foreach (var inputAssignment in InputPropertyBindAssignments)
                {
                    // In the unlikely case that there are multiple conditional bind statements on the input property, the first
                    // assignment wins for validation.
                    if (inputAssignment.PathStep is PropertyStep propertyStep)
                    {
                        foundStep = propertyStep;
                    }
                    else if(inputAssignment.PathStep is FunctionStep function)
                    {
                        var propertyParams = function.Parameters.OfType<FunctionPathParam>().Where(p => p.Path is PropertyStep);
                        // If there are multiple property paths in the function, use the first one that is required. Otherwise,
                        // just use the first one.
                        if (propertyParams.Any(param => param.Path.IsValueRequired))
                        {
                            foundStep = propertyParams.First(param => param.Path.IsValueRequired).Path as PropertyStep;
                        }
                        else
                        {
                            foundStep = propertyParams.FirstOrDefault()?.Path as PropertyStep;
                        }
                    }
                }
            }
            bindStep = foundStep;
            return foundStep != null;
        }

        public bool HasBindAssignments
        {
            get
            {
                return this.bindAssignments != null && this.bindAssignments.Count > 0;
            }
        }

        public IList<BindAssignment> BindAssignments
        {
            get
            {
                if (this.bindAssignments == null)
                {
                    this.bindAssignments = new List<BindAssignment>();
                }
                return this.bindAssignments;
            }
        }

        public IEnumerable<BindAssignment> TwoWayBindAssignments
        {
            get { return BindAssignments.Where(ba => ba.IsTrackingTarget); }
        }

        public bool IsBindingRoot
        {
            get
            {
                if (this.BindUniverse == null)
                {
                    return false;
                }
                return (this.BindUniverse.RootElement == this && !this.BindUniverse.RootElement.Type.IsDerivedFromDataTemplate() && 
                    (this.BindUniverse.HasBindAssignments || this.BindUniverse.HasBoundEventAssignments || this.BindUniverse.NeededForOuterScopeElement));
            }
        }

        public bool IsBindingFileRoot
        {
            get
            {
                return this.IsBindingRoot && this.BindUniverse.IsFileRoot;
            }
        }

        public bool IsTemplateChild
        {
            get
            {
                if (this.BindUniverse == null)
                {
                    return false;
                }

                XamlType rootElementType = this.BindUniverse.RootElement.Type;

                return rootElementType.IsDerivedFromControlTemplate() || rootElementType.IsDerivedFromDataTemplate();
            }
        }

        /// <summary>
        /// Needs a connect case if it's a user named field in the file root namescope (we don't need this for named elements in templates), or has events.
        /// </summary>
        public bool NeedsConnectCase
        {
            get
            {
                return this.FieldDefinition != null || this.EventAssignments.Count > 0;
            }
        }

        public bool HasPhase
        {
            get
            {
                return this.PhaseAssignment != null;
            }
        }

        public bool HasBoundEventAssignments { get { return this.boundEventAssignments != null && this.boundEventAssignments.Count > 0; } }
        public List<BoundEventAssignment> BoundEventAssignments
        {
            get
            {
                if (boundEventAssignments == null)
                {
                    boundEventAssignments = new List<BoundEventAssignment>();
                }
                return boundEventAssignments;
            }
        }

        public bool IsWeakRef
        {
            get { return this.IsBindingRoot && !this.Type.IsDerivedFromControlTemplate(); }
        }

        public bool NeedsNullCheckBeforeSetValue
        {
            get { return this.IsWeakRef || this.CanBeInstantiatedLater; }
        }

        public string ObjectCodeName
        {
            get { return string.Format("obj{0}", ConnectionId); }
        }

        public string ElementCodeName
        {
            get { return string.Format("element{0}", ConnectionId); }
        }

        public string ElementTemplatedParentCodeName
        {
            get
            {
                System.Diagnostics.Debug.Assert(this.BindUniverse.RootElement.Type.IsDerivedFromControlTemplate());
                return $"templatedParent{this.BindUniverse.RootElement.ConnectionId}";
            }
        }

        public XamlType TemplatedParentType
        {
            get
            {
                System.Diagnostics.Debug.Assert(this.BindUniverse.RootElement.Type.IsDerivedFromControlTemplate());
                return this.BindUniverse.DataRootType;
            }
        }

        public LanguageSpecificString ReferenceExpression
        {
            get
            {
                return this.IsWeakRef ?
                    new LanguageSpecificString(
                        () => $"this->{this.ObjectCodeName}.Resolve<{this.Type.CppCXName(false)}>()",
                        () => $"this->{this.ObjectCodeName}.get()",
                        () => string.Format("(this.{0}.Target as {1})", this.ObjectCodeName, this.Type.CSharpName()),
                        () => string.Format("TryCast(Me.{0}.Target, {1})", this.ObjectCodeName, this.Type.VBName()))
                        :
                    new LanguageSpecificString(
                        () => $"this->{this.ObjectCodeName}",
                        () => $"{this.ObjectCodeName}",
                        () => $"this.{this.ObjectCodeName}",
                        () => $"Me.{this.ObjectCodeName}");
            }
        }

        public LanguageSpecificString GetMemberGetExpression(BindAssignmentBase bindAssignment)
        {
            return bindAssignment.IsAttachable ?
                new LanguageSpecificString(
                    () => string.Format("{0}::Get{1}({2})", bindAssignment.MemberDeclaringType.CppCXName(false), bindAssignment.MemberName, this.ReferenceExpression.CppCXName()),
                    () => $"{bindAssignment.MemberDeclaringType.CppWinRTName()}::Get{bindAssignment.MemberName}({this.ReferenceExpression.CppWinRTName()})",
                    () => string.Format("{0}.Get{1}({2})", bindAssignment.MemberDeclaringType.CSharpName(), bindAssignment.MemberName, this.ReferenceExpression.CSharpName()),
                    () => string.Format("{0}.Get{1}({2})", bindAssignment.MemberDeclaringType.VBName(), bindAssignment.MemberName, this.ReferenceExpression.VBName()))
                    :
                new LanguageSpecificString(
                    () => string.Format("{0}->{1}", this.ReferenceExpression.CppCXName(), bindAssignment.MemberName),
                    () => $"{this.ReferenceExpression.CppWinRTName()}.{bindAssignment.MemberName}()",
                    () => string.Format("{0}.{1}", this.ReferenceExpression.CSharpName(), bindAssignment.MemberName),
                    () => string.Format("{0}.{1}", this.ReferenceExpression.VBName(), bindAssignment.MemberName));
        }

        internal string LineNumberAndXamlFile
        {
            get
            {
                return string.Format("{0} line {1}", ParentFileCodeInfo.ApparentRelativePath, LineNumberInfo.StartLineNumber);
            }
        }

        public IEnumerable<ConnectionIdElement> ElementAndAllChildren
        {
            get
            {
                // Return this element.
                yield return this;

                foreach (var child in AllChildren)
                {
                    // Return this element's children.
                    yield return child;
                }
            }
        }

        public IEnumerable<ConnectionIdElement> AllChildren
        {
            get
            {
                foreach (var child in Children.SelectMany(c => c.ElementAndAllChildren).Distinct())
                {
                    // Return this element's children.
                    yield return child;
                }
            }
        }
    }
}
