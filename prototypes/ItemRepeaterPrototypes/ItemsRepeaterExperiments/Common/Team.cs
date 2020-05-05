using System.Collections.Generic;

namespace ItemsRepeaterExperiments.Common
{
    public class Team : IRandomlyInitializable
    {

        public string Name { get; private set; }

        public IList<Person> Members { get; private set; }

        public IList<Team> SubTeams { get; private set; }

        public string MembersString { get; private set; }
        
        public void CreateRandomInstance(int index)
        {
            Name = "Team #" + index;
            Members = DataSourceCreator<Person>.CreateRandomizedList(10);

            foreach (var member in Members)
            { 
                MembersString += member.FirstName + "\n"; 
            }

            SubTeams = new List<Team>();
        }

        public void CreateRandomNestedInstance(int index, int nestingLevel, int leafCount)
        {
            Name = "Team #" + index;
            Members = DataSourceCreator<Person>.CreateRandomizedList(10);

            foreach (var member in Members)
            {
                MembersString += member.FirstName + "\n";
            }

            if(nestingLevel > 0)
            {
                SubTeams = DataSourceCreator<Team>.CreateRandomizedList(10, nestingLevel - 1,leafCount);
            }
            else
            {
                SubTeams = new List<Team>();
                for(int i=0;i<leafCount;i++)
                {
                    var newTeam = new Team();
                    newTeam.CreateRandomInstance(i);
                    SubTeams.Add(newTeam);
                }
            }
        }

        public void CreateRandomNestedInstance(int ownerIndex,int index,int nestingLevel,int leafCount)
        {
            Name ="Owner #" + ownerIndex + ",Team #" + index;

            Members = DataSourceCreator<Person>.CreateRandomizedList(10);

            SubTeams = DataSourceCreator<Team>.CreateRandomizedList(10, nestingLevel - 1, leafCount);
        }

    }
}
