// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Linq;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    public class FunctionStep : BindPathStep
    {
        private readonly string paramsHashCode;

        public List<FunctionParam> Parameters { get; }
        public MethodStep Method { get; }

        public FunctionStep(MethodStep method, ApiInformation apiInformation)
            : base(method.ValueType, method.Parent, apiInformation)
        {
            Method = method;
            Parameters = new List<FunctionParam>();
            paramsHashCode = "";
        }

        public FunctionStep(MethodStep method, IEnumerable<FunctionParam> parameters, ApiInformation apiInformation)
            : this(method, apiInformation)
        {
            Parameters.AddRange(parameters);
            paramsHashCode = ((uint)string.Concat(Parameters.Select(p => p.CodeName).ToArray()).GetHashCode()).ToString();

            foreach (var pathParam in Parameters.OfType<FunctionPathParam>())
            {
                pathParam.Path.AddDependent(this);
            }
        }

        public override string UniqueName => string.Format("{0}_{1}", Method.UniqueName, paramsHashCode);

        public bool RequiresSafeParameterRetrieval => true;

        /// <summary>
        /// The step producing the instance the method is invoked on, or null when that instance is
        /// already known to be non null wherever this function can be scheduled from.
        /// A function binding is scheduled either as a child of that step, which is only updated
        /// while its own value is non null, or as a dependent of one of its path parameters. So the
        /// instance only needs to be retrieved safely when a parameter sits outside of its path.
        /// </summary>
        public BindPathStep InstanceStep
        {
            get
            {
                if (Parent is StaticRootStep)
                {
                    return null;
                }

                bool isScheduledOutsideInstancePath = Parameters
                    .OfType<FunctionPathParam>()
                    .Any(param => !param.Path.Parents.Any(step => step.CodeName == Parent.CodeName));

                return isScheduledOutsideInstancePath ? Parent : null;
            }
        }

        /// <summary>
        /// ValueTypeIsConditional indicates if a step if of a conditional type. It is different than
        /// checking ApiInformation on the step itself, which is used to tell to not use that step unless the 
        /// ApiInformation check is satisfied. This checks against the step value type itself.
        /// ex: a conditional namespace root field step.
        /// </summary>
        public override bool ValueTypeIsConditional
        {
            get { return base.ValueTypeIsConditional || Parameters.OfType<FunctionPathParam>().Where(p => p.Path.ValueTypeIsConditional).Any(); }
        }

        public override bool IsValueRequired => Parameters.OfType<FunctionPathParam>().Where(p => p.Path.IsValueRequired).Any();
    }
}
