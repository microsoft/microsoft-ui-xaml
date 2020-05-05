namespace ItemsRepeaterExperiments.Common
{
    public interface IRandomlyInitializable
    {
        void CreateRandomInstance(int index);

        void CreateRandomNestedInstance(int index, int nestingLevel, int leafs);
    }
}