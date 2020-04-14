using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Windows.UI.Xaml.Controls;

namespace ItemsRepeaterExperiments.Common
{
    public class Team : IRandomlyInitializable
    {

        public string Name { get; private set; }

        public IList<Person> Members { get; private set; }

        public string MembersString { get; private set; }
        public void CreateRandomInstance(int index)
        {
            Name = "Team #" + index;
            Members = DataSourceCreator<Person>.CreateRandomizedList(10);

            foreach (var member in Members)
            { 
                MembersString += member.FirstName + "\n"; 
            }
        }
    }
}
