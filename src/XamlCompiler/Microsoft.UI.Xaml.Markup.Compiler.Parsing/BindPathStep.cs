// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Antlr4.Runtime;
using Antlr4.Runtime.Tree;
using System.Collections.Generic;
using System.Linq;
using System.Xaml;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    using Parsing;

    public abstract class BindPathStep
    {
        private BindStatus bindStatus = BindStatus.None;
        private List<IBindAssignment> bindAssignments = new List<IBindAssignment>();
        private Dictionary<string, BindPathStep> children = new Dictionary<string, BindPathStep>();
        private List<int> distinctPhases = null;

        // Dependents are steps that depend on this step, for instance, a FunctionStep
        // would be a dependent of this step, if the FunctionStep would have this step as an argument.
        // Dependencies are the reverse. If this step would be a FunctionStep, depenencies
        // would list its parameters, if those are BindSteps.
        private Dictionary<string, BindPathStep> dependents = new Dictionary<string, BindPathStep>();
        private Dictionary<string, BindPathStep> dependencies = new Dictionary<string, BindPathStep>();

        public abstract string UniqueName { get; }

        public virtual XamlType ValueType { get; }
        public BindPathStep Parent { get; }

        public IEnumerable<BindPathStep> Parents
        {
            get
            {
                var parents = new List<BindPathStep>();
                if (Parent != null)
                {
                    parents.AddRange(Parent.Parents);
                    parents.Add(Parent);
                }
                return parents;
            }
        }

        public ApiInformation ApiInformation { get; }

        public bool UpdateNeedsBindingsVariable => Children.OfType<RootNamedElementStep>()
            .Any(step => !string.IsNullOrEmpty(step.UpdateCallParamOverride));

        public BindPathStep(XamlType valueType, BindPathStep parent, ApiInformation apiInformation)
        {
            ValueType = valueType;
            Parent = parent;
            ApiInformation = apiInformation;
        }

        public string CodeName
        {
            get
            {
                string apiInformationHash = ApiInformation == null ? "" : "_" + ((uint)ApiInformation.UniqueName.GetHashCode()).ToString();
                if (string.IsNullOrEmpty(Parent?.CodeName))
                {
                    return UniqueName + apiInformationHash;
                }
                else
                {
                    return Parent.CodeName + "_" + UniqueName + apiInformationHash;
                }
            }
        }

        public BindStatus BindStatus
        {
            get => bindStatus;
            private set
            {
                bindStatus = value;
                if (Parent != null)
                {
                    Parent.BindStatus |= value;
                }
                foreach (var dependency in dependencies.Values)
                {
                    dependency.BindStatus |= bindStatus;
                }
            }
        }

        public IEnumerable<IBindAssignment> BindAssignments => bindAssignments;

        public bool RequiresChildNotification => IsTrackingSource && (
                            ValueType.ImplementsINotifyPropertyChanged() ||
                            ValueType.ImplementsINotifyCollectionChanged() ||
                            ValueType.ImplementsIObservableVector() ||
                            ValueType.ImplementsIObservableMap() ||
                            HasTrackingDPs);

        public bool NeedsUpdateChildListeners =>
                // Non static/method/function steps
                IsIncludedInUpdate && RequiresChildNotification && (
                    this is PropertyStep ||
                    this is CastStep ||
                    this is RootNamedElementStep ||
                    this is RootStep ||
                    this is ArrayIndexStep ||
                    this is MapIndexStep);

        public bool HasTrackingDPs => TrackingSteps.OfType<DependencyPropertyStep>().Count() > 0;

        public bool IsTrackingSource => BindStatus.HasFlag(BindStatus.TracksSource);

        public IEnumerable<BindPathStep> Children => children.Values;

        public IEnumerable<BindPathStep> TrackingSteps => Children.Concat(Dependents).Where(c => c.IsTrackingSource);

        /// <summary>
        /// Dependents are steps that depend on this step, for instance, 
        /// a FunctionStep would be a dependent of this step, if the FunctionStep
        /// would have this step as an argument.
        /// </summary>
        public IEnumerable<BindPathStep> Dependents => dependents.Values;

        public IEnumerable<BindPathStep> Dependencies => dependencies.Values;

        internal IList<BindPathStep> ParentsAndSelf
        {
            get
            {
                IList<BindPathStep> results = Parent != null ?
                    Parent.ParentsAndSelf : new List<BindPathStep>();
                results.Add(this);
                return results;
            }
        }

        public List<int> DistinctPhases
        {
            get
            {
                if (distinctPhases == null)
                {
                    distinctPhases = new List<int>
                    {
                        0
                    };
                    foreach (var child in BindAssignments)
                    {
                        int phase = child.ComputedPhase;
                        if (!distinctPhases.Contains(phase))
                        {
                            distinctPhases.Add(phase);
                        }
                    }
                    foreach (var childStep in Children)
                    {
                        foreach (int phase in childStep.DistinctPhases)
                        {
                            if (!distinctPhases.Contains(phase))
                            {
                                distinctPhases.Add(phase);
                            }
                        }
                    }
                    distinctPhases.Sort();
                }
                return distinctPhases;
            }
        }

        public virtual bool ValueTypeIsConditional => (ValueType as IXamlTypeMeta).HasApiInformation;

        /// <summary>
        /// AssociatedBindAssignments returns any BindAssignments associated with this step. This means any assignments
        /// this step is directly connected to, or any BindAssignments that one of its Dependents
        /// (i.e. parameters in a FunctionStep) are associated with.
        /// </summary>
        public virtual IEnumerable<IBindAssignment> AssociatedBindAssignments
        {
            get
            {
                foreach (var assignment in BindAssignments)
                {
                    yield return assignment;
                }

                foreach (var dep in Dependents)
                {
                    foreach (var assignment in dep.BindAssignments)
                    {
                        yield return assignment;
                    }
                }
            }
        }
        /// <summary>
        /// IsValueRequired indicates if the step is annoted with the [System.ComponentModel.DataAnnotations.RequiredAttribute]
        /// </summary>
        public virtual bool IsValueRequired => false;

        public bool ImplementsINPC => ValueType.ImplementsINotifyPropertyChanged();

        public bool ImplementsINCC => ValueType.ImplementsINotifyCollectionChanged();

        public bool ImplementsINDEI => ValueType.ImplementsINotifyDataErrorInfo();

        public bool ImplementsIObservableVector => ValueType.ImplementsIObservableVector();

        public bool ImplementsIObservableMap => ValueType.ImplementsIObservableMap();

        public virtual bool IsIncludedInUpdate => BindStatus.HasFlag(BindStatus.HasBinding);

        public string PhaseList => string.Join(":", DistinctPhases);

        public virtual bool NeedsCheckForNull => ValueType.IsNullable;

        public void AddBindAssignment(IBindAssignment bindAssignment)
        {
            bindAssignments.Add(bindAssignment);
            BindStatus |= bindAssignment.BindStatus;
        }

        public void AddChild(BindPathStep step)
        {
            children[step.CodeName] = step;
        }

        public void AddDependent(BindPathStep dependent)
        {
            // Ex: this is a function param BindPathStep and dependent is a FunctionStep
            dependent.BindStatus |= BindStatus;
            dependents[dependent.CodeName] = dependent;
            dependent.dependencies[CodeName] = this;
        }

        public string TryGetValueCodeName => string.Format("TryGet_{0}", CodeName);

        public static BindPathStep Parse(string bindPath, ApiInformation apiInformation, IBindUniverse bindUniverse, IXamlTypeResolver resolver, IList<string> warnings)
        {
            // Antlr parsing engine setup
            var input = new AntlrInputStream(bindPath);
            var lexer = new BindingPathLexer(input);
            var tokens = new CommonTokenStream(lexer);
            var parser = new BindingPathParser(tokens);

            // redirect parsing errors as VS exceptions instead of console output
            // and disable the automatic error recovery handler
            parser.RemoveErrorListeners();
            parser.AddErrorListener(new ParseErrorListener());
            parser.ErrorHandler = new BailErrorStrategy();

            // set our custom callback override to build up the BindPathSteps during parse
            var listener = new BindingPathListener(bindPath, apiInformation, bindUniverse, resolver) { Warnings = warnings };

            // parse the path input
            var walker = new ParseTreeWalker();
            var parsedPath = parser.path();
            walker.Walk(listener, parsedPath);

            // confirm nothing left unparsed at end of path string
            lexer.ConfirmInputFullyConsumed();

            return parsedPath.PathStep;
        }
    }
}