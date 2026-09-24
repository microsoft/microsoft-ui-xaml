// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;

namespace BindTestbed
{
    internal class LanguageSpecific
    {
        public int IntField;
        public string StringField;

        public LanguageSpecific()
        {
            InitializeValues();
        }

        internal void InitializeValues()
        {
            StringField = "Simple string field";
            IntField = 2;
        }

        internal void UpdateValues()
        {
            this.IntField = IntField + 1 % 10;

            string ms = " " + DateTime.Now.Millisecond.ToString();
            this.StringField += ms;
        }

        // TODO: Move to a C#-specific model because indexers are not supported in WinRT.
        //public Employee this[int index] { get { return _reports[index]; } }
        //public Employee this[string name]
        //{
        //    get
        //    {
        //        return (from Employee e in _reports where e.FirstName == name select e).First();
        //    }
        //}

    }
}