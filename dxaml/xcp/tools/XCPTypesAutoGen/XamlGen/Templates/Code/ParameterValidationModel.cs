// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System.Collections.Generic;


namespace XamlGen.Templates.Code
{
    public class ParameterValidationModel
    {
        public bool HasReturnType
        {
            get
            {
                return ReturnType != null && !ReturnType.IsVoid;
            }
        }

        public IEnumerable<OM.ParameterDefinition> Parameters
        {
            get;
            private set;
        }

        public OM.TypeReference ReturnType
        {
            get;
            private set;
        }

        private ParameterValidationModel()
        {
        }

        public static ParameterValidationModel Create(OM.ConstructorDefinition ctor)
        {
            return new ParameterValidationModel()
            {
                Parameters = ctor.Parameters
            };
        }

        public static ParameterValidationModel Create(OM.MethodDefinition method)
        {
            return new ParameterValidationModel()
            {
                Parameters = method.Parameters,
                ReturnType = method.ReturnType
            };
        }
    }
}
