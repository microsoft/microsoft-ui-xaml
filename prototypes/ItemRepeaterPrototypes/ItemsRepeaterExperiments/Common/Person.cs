using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ItemsRepeaterExperiments.Common
{
    public class Person : IRandomlyInitializable
    {
        private static string[] names = new string[]
        {
            "Jerry","Ranjesh","Steven","Jevan","Savoy","Yulia","Stephen","Karen",
            "Ana","Kai","Stephen","Kiki","Ryan","Michael","Miguel","Mike","Jesse",
            "Keith","Dilip","Tammy","Chigusa","Marcel","Luke","Régis","Chris","Canhua",
            "Bill","Claire","Jen","Sergio","Jay","Matt","Jeremy","Sebastian","Fons",
            "David","Immo","Jerome","Neil","Dustin","Kayla"
        };

        public string CompleteName => FirstName + " " + LastName;

        public string FirstName { get; private set; }
        public string LastName { get; private set; }

        public void CreateRandomInstance(int index)
        {
            FirstName = names[new Random().Next(0,names.Length)];
        }
    }
}
