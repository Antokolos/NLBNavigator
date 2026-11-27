"""Загружает оба сохранения экрана выбора из двух новых профилей."""
import hashlib
import json
from pathlib import Path

from execute import ROOT, ISLOT, NLBSLOT, pair

matrix = ROOT / "matrix"
source = matrix / "choices"
target = matrix / "choices-restart"
spec = json.loads((matrix / "a1-restart.json").read_text())
spec["id"] = "wolfman-save-choices-restart"
spec["steps"] = spec["steps"][:2]
spec["steps"][1]["expect"].update({
    "screen": "choices",
    "instead_screen_kind": "choices",
    "choices_contains": ["Съесть врача", "Съесть труп"],
    "instead_choices_contains": ["Съесть врача", "Съесть труп"],
    "variables": {"wolfiness": 0, "hp": 2},
    "portion_index": 1,
    "portion_count": 2,
    "instead_portion_index": 1,
    "instead_portion_count": 2,
})
route = matrix / "choices-restart.json"
with route.open("x") as stream:
    json.dump(spec, stream, ensure_ascii=False, indent=2)
    stream.write("\n")

slots = {"nlbnavigator": Path("nlbnavigator-home") / NLBSLOT, "instead": ISLOT}
seeds = {side: source / side / rel for side, rel in slots.items()}
codes = pair(route, target, 375, seeds)

transfers = {}
for side, source_slot in seeds.items():
    new_slot = target / side / slots[side]
    source_bytes, new_bytes = source_slot.read_bytes(), new_slot.read_bytes()
    assert source_bytes == new_bytes
    transfers[side] = {
        "source": str(source_slot.relative_to(ROOT)),
        "copy": str(new_slot.relative_to(ROOT)),
        "bytes": len(source_bytes),
        "sha256": hashlib.sha256(source_bytes).hexdigest(),
    }
(target / "slots.json").write_text(json.dumps(transfers, ensure_ascii=False, indent=2) + "\n")
raise SystemExit(int(any(codes.values())))
