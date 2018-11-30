// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace MUXControlsTestApp
{
    public class Ingredient
    {
        public string Name { get; set; }
        public string Amount { get; set; }
        public string Photo { get; set; }

        public Ingredient() { }

        public Ingredient(string name, string amount, string photo)
        {
            this.Name = name;
            this.Amount = amount;
            this.Photo = photo;
        }
    }
}
