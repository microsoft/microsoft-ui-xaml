// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Xaml;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    internal class BindUniverse : IBindUniverse
    {
        private IEnumerable<string> distinctConvertersUsed;
        private readonly string parentClassShortName;
        public IList<BindUniverse> Children = new List<BindUniverse>();
        public BindUniverse Parent = null;

        public Dictionary<string, BindPathStep> BindPathSteps = new Dictionary<string, BindPathStep>();
        public BindPathStep RootStep { get; private set; } = null;
        public IList<ConnectionIdElement> NamedElements { get; }

        internal ConnectionIdElement RootElement { get; private set; }
        internal List<BindAssignment> BindAssignments = new List<BindAssignment>();
        internal List<BoundEventAssignment> BoundEventAssignments = new List<BoundEventAssignment>();
        internal List<ConnectionIdElement> BoundElements = new List<ConnectionIdElement>();
        internal List<ConnectionIdElement> OuterScopeBoundElements = new List<ConnectionIdElement>();
        internal IList<ConnectionIdElement> UnloadableBindingSourceElements = new List<ConnectionIdElement>();
        private IList<ConnectionIdElement> directParentsOfUnloadables = new List<ConnectionIdElement>();
        internal Dictionary<int, List<PhaseAssignment>> PhaseAssignments = new Dictionary<int, List<PhaseAssignment>>();

        public bool NeededForOuterScopeElement = false;

        internal bool IsFileRoot { get; private set; }
        internal XamlType DataRootType { get; private set; }
        private IEnumerable<FieldDefinition> rootFieldDefinitions;

        // The normal RootStep assumes the objects we've bound to are on the data context, and for two-way bindings
        // will try to do the source-object property set based off the data context.  However, named element binding sources
        // don't actually exist on the data context, so instead we will set their root step to a separate root, ElementRootStep.
        public BindPathStep ElementRootStep { get; private set; } = null;
        public BindPathStep MakeOrGetRootStepOutOfScope()
        {
            if (ElementRootStep == null)
            {
                ElementRootStep = new RootStep(DataRootType, true);
            }

            return ElementRootStep;
        }

        internal BindUniverse(ConnectionIdElement rootElement, XamlType dataRootType, bool isFileRoot, string classShortName)
        {
            RootElement = rootElement;
            IsFileRoot = isFileRoot;
            DataRootType = dataRootType;
            parentClassShortName = classShortName;
            NamedElements = new List<ConnectionIdElement>();
        }

        private void ProcessRootNamedElementSteps(System.Version targetPlatformMinVersion, List<XamlCompileErrorBase> issues)
        {
            // We need to find those elements which may or may not have an x:Bind,
            // but which are sources for other bindings. We need to invoke their
            // Update_ function so their bindings will be re-evaluated on connect.
            foreach (var step in BindPathSteps.Values.OfType<RootNamedElementStep>())
            {
                BindUniverse currentUniverse = this;
                while (currentUniverse != null)
                {
                    if (currentUniverse != this)
                    {
                        currentUniverse.NeededForOuterScopeElement = true;
                    }

                    var outerScopeNamedElementList = currentUniverse.RootElement.ElementAndAllChildren.Where(e => e.ElementName == step.FieldName);
                    if (outerScopeNamedElementList.Any())
                    {
                        var element = outerScopeNamedElementList.First();

                        if (currentUniverse == this)
                        {
                            // Case where we've bound to a named element within our scope

                            // Associate all elements with their RootNamedElementStep
                            element.RootNamedElementStep = step;

                            // When binding to a named element within a template, we need to register the named element as something that's been bound to
                            if (!element.BindUniverse.IsFileRoot && !BoundElements.Contains(element))
                            {
                                BoundElements.Add(element);
                            }

                            if (element.CanBeInstantiatedLater)
                            {
                                UnloadableBindingSourceElements.Add(element);
                            }
                        }
                        else
                        {
                            // Case where we've bound to a named element outside of our scope
                            element.IsUsedByOtherScopes = true;

                            // Register the element in the descendant scope binding to it
                            if (!OuterScopeBoundElements.Contains(element))
                            {
                                // Add the outer scope's element to the current scope's list of bound elements
                                OuterScopeBoundElements.Add(element);
                            }

                            NeededForOuterScopeElement = true;

                            // Ensure the element is registered as a bound element in this scope.
                            if (!BoundElements.Contains(element))
                            {
                                BoundElements.Add(element);
                            }

                            // Ensure the element is also registered in the scope where it's defined as an element that's been bound to.
                            if (!currentUniverse.BoundElements.Contains(element))
                            {
                                currentUniverse.BoundElements.Add(element);
                            }
                        }

                        break;
                    }

                    currentUniverse = currentUniverse.Parent;
                }
            }
        }

        internal IEnumerable<XamlCompileErrorBase> Parse(XamlClassCodeInfo classCodeInfo, System.Version targetPlatformMinVersion)
        {
            var issues = new List<XamlCompileErrorBase>();

            if (IsFileRoot)
            {
                rootFieldDefinitions = classCodeInfo.FieldDeclarations;
            }
            RootStep = new RootStep(DataRootType);

            BindPathSteps[""] = RootStep;

            foreach (BindAssignment bindAssignment in BindAssignments)
            {
                var parseIssues = bindAssignment.ParsePath();
                issues.AddRange(parseIssues);
            }

            foreach (BoundEventAssignment boundEventAssignment in BoundEventAssignments)
            {
                var parseIssues = boundEventAssignment.ParsePath();
                issues.AddRange(parseIssues);
            }

            ProcessRootNamedElementSteps(targetPlatformMinVersion, issues);

            foreach (var potentialParent in RootElement.AllChildren)
            {
                // If potentialParent has an x:Bind on x:Load element as an immediate child, it may qualify.
                if (potentialParent.Children.Any(c => ElementsWithBoundLoadAssignments.Any(e => c == e)))
                {
                    // Now, potentialParent needs to have an x:Bind on x:Load parent or be one itself.
                    if (ElementsWithBoundLoadAssignments.Any(e => e.ElementAndAllChildren.Any(c => c == potentialParent)))
                    {
                        directParentsOfUnloadables.Add(potentialParent);
                    }
                }
            }
            return issues;
        }

        internal void AddPhase(PhaseAssignment phase)
        {
            List<PhaseAssignment> phaseList;
            if (!PhaseAssignments.TryGetValue(phase.Phase, out phaseList))
            {
                phaseList = new List<PhaseAssignment>();
                PhaseAssignments.Add(phase.Phase, phaseList);
            }
            phaseList.Add(phase);
        }

        internal int GetNextPhase(int currentPhase)
        {
            int nextPhase = -1;

            foreach (var key in PhaseAssignments.Keys)
            {
                if ((key > currentPhase) && (nextPhase == -1 || key < nextPhase))
                {
                    nextPhase = key;
                }
            }

            return nextPhase;
        }

        public BindPathStep EnsureUniquePathStep(BindPathStep step)
        {
            var stepCodeName = step.CodeName;

            if (BindPathSteps.ContainsKey(stepCodeName))
            {
                return BindPathSteps[stepCodeName];
            }
            else
            {
                BindPathSteps[stepCodeName] = step;
                step.Parent?.AddChild(step);
            }

            return step;
        }

        public string BindingsClassName
        {
            get
            {
                Debug.Assert(!string.IsNullOrEmpty(parentClassShortName), "BindingsClassName should never be called before Parse");
                return string.Format("{0}_obj{1}_Bindings", parentClassShortName, RootElement.ConnectionId);
            }
        }

        public string BindingsTrackingClassName
        {
            get
            {
                Debug.Assert(!string.IsNullOrEmpty(parentClassShortName), "BindingsTrackingClassName should never be called before Parse");
                return string.Format("{0}_obj{1}_BindingsTracking", parentClassShortName, RootElement.ConnectionId);
            }
        }

        public bool HasBindings => BindPathSteps.Values.Count > 0;

        public bool HasBindAssignments => BindAssignments.Count > 0;

        public bool HasBoundEventAssignments => BoundEventAssignments.Count > 0;

        public bool HasFunctionBindings => BindPathSteps.Where(s => s.Value is FunctionStep).Count() > 0;

        internal bool NeedsIDataTemplateExtension => !IsFileRoot && !RootElement.Type.IsDerivedFromControlTemplate();

        internal bool NeedsIDataTemplateComponent => !IsFileRoot;

        internal IEnumerable<string> DistinctConvertersUsed
        {
            get
            {
                if (distinctConvertersUsed == null)
                {
                    distinctConvertersUsed = BindAssignments.Where(bindAssignment => bindAssignment.BindStatus.HasFlag(BindStatus.HasConverter)).Select(bindAssignment => bindAssignment.Converter).Distinct();
                }
                return distinctConvertersUsed;
            }
        }

        public IEnumerable<BindPathStep> TryGetValueSteps
        {
            get
            {
                Dictionary<string, BindPathStep> steps = new Dictionary<string, BindPathStep>();
                foreach (FunctionStep functionStep in BindPathSteps.Values.OfType<FunctionStep>().Where(s => s.RequiresSafeParameterRetrieval))
                {
                    foreach (var param in functionStep.Parameters.OfType<FunctionPathParam>().Where(p => p.HasTryGetValue))
                    {
                        AddTryGetValueStep(steps, param.Path);
                    }
                }
                return steps.Values;
            }
        }

        private void AddTryGetValueStep(Dictionary<string, BindPathStep> steps, BindPathStep step)
        {
            if (step.IsIncludedInUpdate)
            {
                steps[step.TryGetValueCodeName] = step;
                if (step.Parent != null)
                {
                    AddTryGetValueStep(steps, step.Parent);
                }
            }
        }

        internal IEnumerable<ConnectionIdElement> ElementsWithDisconnectCase => ElementsWithConnectCase.Where(ele => ele.CanBeInstantiatedLater);

        internal IEnumerable<ConnectionIdElement> ElementsWithBoundLoadAssignments => BoundElements.Where(ele => ele.BindAssignments.OfType<BoundLoadAssignment>().Any());

        public IEnumerable<ConnectionIdElement> ElementsWithConnectCase => BoundElements.Union(OuterScopeBoundElements).Union(UnloadableBindingSourceElements).Union(directParentsOfUnloadables).Distinct().OrderBy(e => e.ConnectionId);

        public IEnumerable<ConnectionIdElement> ElementsWithConnectCaseInLocalScope => ElementsWithConnectCase.Where(ele => ele.BindUniverse == this);

        internal bool NeedsCompleteUpdate => HasFunctionBindings || ElementsWithBoundLoadAssignments.Any() || UnloadableBindingSourceElements.Any();

        public bool NeedsCppBindingTrackingClass => BindPathSteps.Values.Where(s => s.ValueType.ImplementsIObservableVector() || s.ValueType.ImplementsIObservableMap()).Any();

        public bool NeedsBindingsTracking
        {
            get
            {
                return RootStep.IsTrackingSource || BindPathSteps.Values.OfType<StaticRootStep>().Any() || (ElementRootStep != null && ElementRootStep.IsTrackingSource);
            }
        }

        public IEnumerable<BindPathStep> INDEIPathSteps => BindPathSteps.Values.Where(step => step.IsIncludedInUpdate && step.IsTrackingSource && step.ImplementsINDEI);

        public XamlType GetNamedElementType(string name, out string objectCodeName)
        {
            BindUniverse currentUniverse = this;
            while (currentUniverse != null)
            {
                var element = currentUniverse.NamedElements?.Where(e => e.ElementName == name).FirstOrDefault();
                if (element != null)
                {
                    objectCodeName = element.ObjectCodeName;
                    return element.Type;
                }

                currentUniverse = currentUniverse.Parent;
            }
            objectCodeName = null;
            return null;
        }

        public XamlType GetNamedFieldType(string name)
        {
            var fieldDefinition = rootFieldDefinitions?.Where(f => f.FieldName == name).FirstOrDefault();
            if (fieldDefinition != null)
            {
                return fieldDefinition.FieldXamlType;
            }
            return null;
        }
    }
}
