// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;

namespace XamlOM
{
    public enum ThreadingModel
    {
        Both = 0,
        STA = 1,
        MTA = 2,
    }

    /// <summary>
    /// Specifies the threading model of this type.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class, Inherited = false)]
    public class ThreadingModelAttribute : Attribute, NewBuilders.IClassBuilder, NewBuilders.IEndClassBuilder, NewBuilders.IOrderedBuilder
    {
        public ThreadingModel Model
        {
            get;
            private set;
        }

        public ThreadingModelAttribute(ThreadingModel model)
        {
            Model = model;

            // By default, make sure we execute last, so that we can update the AllowCrossThreadAccess flag on *all* members, including 
            // ones copied from interfaces.
            Order = int.MaxValue;
        }

        public void BuildNewClass(OM.ClassDefinition definition, Type source)
        {
            definition.ThreadingModel = (OM.ThreadingModelKind)(int)Model;

            if (Model == ThreadingModel.Both || Model == ThreadingModel.MTA)
            {
                definition.XamlClassFlags.IsFreeThreaded = true;
            }
        }

        public void BuildEndClass(OM.ClassDefinition definition, Type source)
        {
            if (Model != ThreadingModel.STA)
            {
                foreach (OM.MemberDefinition member in definition.AllPMEs)
                {
                    member.AllowCrossThreadAccess = true;
                }
            }
        }

        public int Order
        {
            get;
            set;
        }
    }
}
