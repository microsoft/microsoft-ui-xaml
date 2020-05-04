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

            for(int i = 0; i < 10; i++)
            {
                var newTeam = new Team();
                newTeam.CreateRandomInstance(index, i);
                SubTeams.Add(newTeam);
            }
        }

        public void CreateRandomInstance(int ownerIndex,int index)
        {
            Name = "Owner #" + ownerIndex + ", Team #" + index;

            Members = DataSourceCreator<Person>.CreateRandomizedList(10);

            SubTeams = new List<Team>();
        }

    }
}
