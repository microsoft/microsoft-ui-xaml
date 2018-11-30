// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.ObjectModel;

namespace MUXControlsTestApp
{
    public sealed partial class ListViewItemPage : TestPage
    {
        private ObservableCollection<Ingredient> Ingredients = new ObservableCollection<Ingredient>();

        public ListViewItemPage()
        {
            this.InitializeComponent();
            PopulateData();
        }

        private void PopulateData()
        {
            this.Ingredients.Add(new Ingredient("Banana", "10 lbs", "Assets/ingredient1.png"));
            this.Ingredients.Add(new Ingredient("Strawberry", "27 lbs", "Assets/ingredient2.png"));
            this.Ingredients.Add(new Ingredient("Lemon", "12 lbs", "Assets/ingredient5.png"));
            this.Ingredients.Add(new Ingredient("Orange", "54 lbs", "Assets/ingredient3.png"));
            this.Ingredients.Add(new Ingredient("Vanilla", "2 lbs", "Assets/ingredient8.png"));
            this.Ingredients.Add(new Ingredient("Mint", "5 lbs", "Assets/ingredient4.png"));
            this.Ingredients.Add(new Ingredient("Banana", "9 lbs", "Assets/ingredient1.png"));
            this.Ingredients.Add(new Ingredient("Strawberry", "21 lbs", "Assets/ingredient2.png"));
            this.Ingredients.Add(new Ingredient("Lemon", "10 lbs", "Assets/ingredient5.png"));
            this.Ingredients.Add(new Ingredient("Orange", "57 lbs", "Assets/ingredient3.png"));
            this.Ingredients.Add(new Ingredient("Vanilla", "1 lbs", "Assets/ingredient8.png"));
            this.Ingredients.Add(new Ingredient("Mint", "4 lbs", "Assets/ingredient4.png"));
            this.Ingredients.Add(new Ingredient("Banana", "7 lbs", "Assets/ingredient1.png"));
            this.Ingredients.Add(new Ingredient("Strawberry", "5 lbs", "Assets/ingredient2.png"));
            this.Ingredients.Add(new Ingredient("Lemon", "4 lbs", "Assets/ingredient5.png"));
            this.Ingredients.Add(new Ingredient("Orange", "76 lbs", "Assets/ingredient3.png"));
            this.Ingredients.Add(new Ingredient("Vanilla", "3 lbs", "Assets/ingredient8.png"));
            this.Ingredients.Add(new Ingredient("Mint", "3 lbs", "Assets/ingredient4.png"));

            this.IngredientList.ItemsSource = this.Ingredients;
        }
    }
}
