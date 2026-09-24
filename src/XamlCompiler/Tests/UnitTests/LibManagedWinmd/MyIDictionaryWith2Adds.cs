// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace LibManagedWinmd
{
    public sealed class MyIDictionaryWith2Adds : IDictionary<String, Object>
    {
        // I added this extra "Add" method to comfuse the XAML compiler's Schema.
        // In the future we may make it smart enought to deal with this.
        public void Add(string key, String value)
        {
            // the user might want to add extra code when adding a string
            // to what is really a dictionary of "Objects".
            value += "(" + value.Length.ToString() + ")";
            this.Add(key, (Object)value);
        }

        public void Add(string key, Object value)
        {
            throw new NotImplementedException();
        }

        public bool ContainsKey(string key)
        {
            throw new NotImplementedException();
        }

        public ICollection<string> Keys
        {
            get { throw new NotImplementedException(); }
        }

        public bool Remove(string key)
        {
            throw new NotImplementedException();
        }

        public bool TryGetValue(string key, out Object value)
        {
            throw new NotImplementedException();
        }

        public ICollection<Object> Values
        {
            get { throw new NotImplementedException(); }
        }

        public Object this[string key]
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public void Add(KeyValuePair<string, Object> item)
        {
            throw new NotImplementedException();
        }

        public void Clear()
        {
            throw new NotImplementedException();
        }

        public bool Contains(KeyValuePair<string, Object> item)
        {
            throw new NotImplementedException();
        }

        public void CopyTo(KeyValuePair<string, Object>[] array, int arrayIndex)
        {
            throw new NotImplementedException();
        }

        public int Count
        {
            get { throw new NotImplementedException(); }
        }

        public bool IsReadOnly
        {
            get { throw new NotImplementedException(); }
        }

        public bool Remove(KeyValuePair<string, Object> item)
        {
            throw new NotImplementedException();
        }

        public IEnumerator<KeyValuePair<string, Object>> GetEnumerator()
        {
            throw new NotImplementedException();
        }

        System.Collections.IEnumerator System.Collections.IEnumerable.GetEnumerator()
        {
            throw new NotImplementedException();
        }
    }
}
