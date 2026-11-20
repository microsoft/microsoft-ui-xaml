import unittest

from workload import Catalog


class CatalogTests(unittest.TestCase):
    def test_case_insensitive_matches_and_missing_labels(self):
        catalog = Catalog(["Button", "TextBox", "Slider"])
        self.assertEqual((0,), catalog.find("BUTTON"))
        self.assertEqual((1,), catalog.find("textbox"))
        self.assertEqual((), catalog.find("ToggleSwitch"))

    def test_duplicate_matches_keep_input_order(self):
        catalog = Catalog(["Button", "Other", "BUTTON", "button"])
        self.assertEqual((0, 2, 3), catalog.find("bUtToN"))

    def test_empty_catalog_and_empty_label(self):
        self.assertEqual((), Catalog([]).find(""))
        self.assertEqual((1,), Catalog(["Button", ""]).find(""))

    def test_labels_are_consumed_from_an_iterable_at_construction(self):
        labels = ["Button", "Slider"]
        catalog = Catalog(label for label in labels)
        labels.append("TextBox")
        self.assertEqual((0,), catalog.find("button"))
        self.assertEqual((0,), catalog.find("BUTTON"))
        self.assertEqual((), catalog.find("TextBox"))

    def test_unicode_casefold_matching_is_preserved(self):
        catalog = Catalog(["Stra\u00dfe", "STRASSE", "Other"])
        self.assertEqual((0, 1), catalog.find("strasse"))

    def test_matching_does_not_trim_whitespace_or_punctuation(self):
        catalog = Catalog([" Button ", "Button", "Button!"])
        self.assertEqual((0,), catalog.find(" BUTTON "))
        self.assertEqual((1,), catalog.find("button"))
        self.assertEqual((2,), catalog.find("BUTTON!"))


if __name__ == "__main__":
    unittest.main()
