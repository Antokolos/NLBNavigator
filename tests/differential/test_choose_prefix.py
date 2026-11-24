"""Действие choose (выбор варианта по тексту) и общий префикс маршрутов (routes/fragments)."""

import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
sys.path.insert(0, str(Path(__file__).resolve().parent / "ci"))

import run_route  # noqa: E402
import select_routes  # noqa: E402
from route_validation import expand_prefix, validate_route  # noqa: E402

HERE = Path(__file__).resolve().parent
EXAMPLE = HERE / "examples" / "wolfman-police-hospital-doctor-choose.json"
CHOICES = '<a:act 01>Съесть врача</a>\n\n <a:act 02>Съесть "труп" -- быстро</a>\n\n \n'


class ChooseTest(unittest.TestCase):
    def test_instead_choice_index_by_text(self):
        self.assertEqual(run_route.instead_choice_index(CHOICES, "Съесть врача"), (0, 1))
        # текст INSTEAD — после типографики, фрагмент маршрута — без неё: сравнение после нормализации
        self.assertEqual(run_route.instead_choice_index(CHOICES, "быстро"), (1, 1))
        self.assertEqual(run_route.instead_choice_index(CHOICES, "Съесть"), (None, 2))
        self.assertEqual(run_route.instead_choice_index(CHOICES, "Убежать"), (None, 0))

    def test_script_uses_choice_text(self):
        route = run_route.load_route(EXAMPLE)
        script, _, _ = run_route.script_for_nlbnavigator(route, Path("/tmp/out"))
        self.assertIn("clickchoice:text=Попытаться прорваться\n", script)
        self.assertIn("clickchoice:text=Съесть врача\n", script)

    def test_validation(self):
        choices = {"id": "c", "action": {"type": "advance_until", "until": "choices", "max_clicks": 5,
                                         "x_ratio": 0.5, "y_ratio": 0.5, "expect_pages": [{"page_id": "p"}]}}

        def errors(text, previous=choices):
            return validate_route({"steps": [previous, {"id": "s", "action": {"type": "choose", "text": text}}]})

        self.assertEqual(errors("Съесть врача"), [])
        for bad in ("", "Я -- монстр", "«труп»", '"труп"', "1. Съесть врача"):
            self.assertTrue(errors(bad), bad)
        page = {"id": "c", "action": {"type": "click", "x_ratio": 0.5, "y_ratio": 0.5}, "expect": {"screen": "page"}}
        self.assertTrue(any("экран вариантов" in e for e in errors("Съесть врача", page)))
        on_choices = dict(page, expect={"screen": "choices"})
        self.assertEqual(errors("Съесть врача", on_choices), [])


class PrefixTest(unittest.TestCase):
    def write(self, directory, name, data):
        path = Path(directory) / "fragments" / (name + ".json")
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps(data, ensure_ascii=False), encoding="utf-8")

    def step(self, step_id, **extra):
        return dict({"id": step_id, "screenshot": step_id + ".png", "action": {"type": "capture"}}, **extra)

    def test_nested_prefixes_are_expanded_in_order(self):
        with tempfile.TemporaryDirectory() as directory:
            self.write(directory, "a", {"id": "a", "steps": [self.step("one")]})
            self.write(directory, "b", {"id": "b", "prefix": "a",
                                        "steps": [self.step("two", visual_change_from="one")]})
            route, errors = expand_prefix({"prefix": "b", "steps": [self.step("three")]}, directory)
            self.assertEqual(errors, [])
            self.assertEqual([s["id"] for s in route["steps"]], ["a.one", "b.two", "three"])
            self.assertEqual(route["steps"][0]["screenshot"], "a.one.png")
            self.assertEqual(route["steps"][1]["visual_change_from"], "b.one")
            self.assertEqual(route["prefix_chain"], ["a", "b"])

    def test_errors(self):
        with tempfile.TemporaryDirectory() as directory:
            self.write(directory, "a", {"id": "a", "prefix": "b", "steps": []})
            self.write(directory, "b", {"id": "b", "prefix": "a", "steps": []})
            self.write(directory, "wrong", {"id": "other", "steps": []})
            for prefix, fragment in (("a", "цикл"), ("missing", "missing"), ("wrong", "id"), ("../x", "латиниц")):
                _, errors = expand_prefix({"prefix": prefix, "steps": []}, directory)
                self.assertTrue(any(fragment in e for e in errors), (prefix, errors))

    def test_example_route_with_prefix(self):
        route = run_route.load_route(EXAMPLE)
        self.assertEqual(route["prefix_chain"], ["wolfman-police-start"])
        self.assertEqual(route["steps"][0]["id"], "wolfman-police-start.title")
        self.assertEqual(len(route["steps"]), 7)

    def test_changed_fragment_selects_routes_with_it(self):
        routes = [{"id": "x", "path": str(EXAMPLE.relative_to(select_routes.ROOT))}]
        chosen, _, _ = select_routes.select(routes, ["tests/differential/routes/fragments/wolfman-police-start.json"])
        self.assertEqual([r["id"] for r in chosen], ["x"])
        chosen, _, _ = select_routes.select(routes, ["tests/differential/routes/fragments/other.json"])
        self.assertEqual(chosen, [])

    def test_timeouts_count_prefix_steps(self):
        timeouts = select_routes.timeouts(str(EXAMPLE.relative_to(select_routes.ROOT)))
        route = run_route.load_route(EXAMPLE)
        clicks = sum(s["action"].get("max_clicks", 0) for s in route["steps"] if s["action"]["type"] == "advance_until")
        self.assertEqual(timeouts["instead_timeout"], 120 + 8 * len(route["steps"]) + 3 * clicks)


if __name__ == "__main__":
    unittest.main()
