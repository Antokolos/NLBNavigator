#!/usr/bin/env python3
import json
import unittest
from pathlib import Path

from route_validation import validate_route

ROOT = Path(__file__).resolve().parents[2]
POLICE_ROUTE = ROOT / "tests/differential/routes/wolfman-police-hospital-corpse.json"


def valid_route():
    return {
        "id": "test-route",
        "steps": [
            {"id": "start", "action": {"type": "capture"}, "expect": {"page_id": "page-a"}},
            {
                "id": "portion-0", "action": {"type": "advance"}, "visual_change_from": "start",
                "expect": {
                    "page_id": "page-a", "portion_index": 0, "portion_count": 2,
                    "instead_portion_index": 0, "instead_portion_count": 2,
                },
            },
            {
                "id": "portion-1", "action": {"type": "advance"}, "visual_change_from": "portion-0",
                "expect": {
                    "page_id": "page-a", "portion_index": 1, "portion_count": 2,
                    "instead_portion_index": 1, "instead_portion_count": 2,
                },
            },
            {
                "id": "choices", "action": {"type": "advance"},
                "expect": {
                    "page_id": "page-a", "screen": "choices", "instead_screen_kind": "choices",
                    "choices_contains": ["Option A"], "instead_choices_contains": ["Option A"],
                },
            },
            {
                "id": "select",
                "action": {"type": "select_choice", "index": 0, "instead": {"x_ratio": 0.2, "y_ratio": 0.2}},
            },
        ],
    }


class RouteValidationTests(unittest.TestCase):
    def assert_invalid(self, route, fragment):
        errors = validate_route(route)
        self.assertTrue(any(fragment in error for error in errors), errors)

    def test_root_must_be_object(self):
        self.assert_invalid([], "корень маршрута должен быть JSON-объектом")

    def test_action_must_be_object(self):
        route = valid_route()
        route["steps"][0]["action"] = ["advance"]
        self.assert_invalid(route, "action должен быть объектом")

    def test_expect_must_be_object(self):
        route = valid_route()
        route["steps"][0]["expect"] = ["page-a"]
        self.assert_invalid(route, "expect должен быть объектом")

    def test_valid_route_and_partial_portion_metadata(self):
        route = valid_route()
        route["steps"][2]["expect"].pop("portion_count")
        route["steps"][2]["expect"].pop("instead_portion_count")
        self.assertEqual([], validate_route(route))

    def test_duplicate_step_ids(self):
        route = valid_route()
        route["steps"][1]["id"] = "start"
        self.assert_invalid(route, "повторяющийся step id")

    def test_visual_change_must_reference_previous_step(self):
        route = valid_route()
        route["steps"][2]["visual_change_from"] = "start"
        self.assert_invalid(route, "visual_change_from")

    def test_visual_change_cannot_reference_missing_step(self):
        route = valid_route()
        route["steps"][1]["visual_change_from"] = "missing"
        self.assert_invalid(route, "visual_change_from")

    def test_select_choice_requires_choices_checkpoint(self):
        route = valid_route()
        route["steps"][3]["expect"]["screen"] = "page"
        self.assert_invalid(route, "перед select_choice нужен checkpoint")

    def test_portion_indices_must_increase_without_repeating(self):
        route = valid_route()
        route["steps"][2]["expect"]["portion_index"] = 0
        route["steps"][2]["expect"]["instead_portion_index"] = 0
        self.assert_invalid(route, "индекс должен строго увеличиваться")

    def test_portion_count_must_be_stable_on_page(self):
        route = valid_route()
        route["steps"][2]["expect"]["portion_count"] = 3
        self.assert_invalid(route, "не совпадает с предыдущим значением")

    def test_portion_indices_may_skip_unobserved_intermediate_checkpoints(self):
        route = valid_route()
        route["steps"][1]["expect"]["portion_count"] = 5
        route["steps"][1]["expect"]["instead_portion_count"] = 5
        route["steps"][2]["expect"]["portion_index"] = 4
        route["steps"][2]["expect"]["portion_count"] = 5
        route["steps"][2]["expect"]["instead_portion_index"] = 4
        route["steps"][2]["expect"]["instead_portion_count"] = 5
        self.assertEqual([], validate_route(route))

    def test_legacy_police_route_without_opening_choices_is_rejected(self):
        with POLICE_ROUTE.open("r", encoding="utf-8") as source:
            route = json.load(source)
        route["steps"] = [step for step in route["steps"] if step["id"] != "opening-choices"]
        self.assert_invalid(route, "перед select_choice нужен checkpoint")


if __name__ == "__main__":
    unittest.main()
