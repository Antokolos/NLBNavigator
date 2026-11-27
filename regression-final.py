"""Проверяет итог 22 маршрутов и повторяет только неуспешные команды."""
import concurrent.futures
import json
import time

from execute import ROOT, REPO, engine, stock_compare

cases = json.loads((REPO / "tests/differential/catalog.json").read_text())["routes"]
tail = ROOT / "regression-tail"
for _ in range(600):
    if (ROOT / "regression-tail.exit").is_file() and (ROOT / "regression-retry/results.json").is_file():
        break
    time.sleep(3)
else:
    raise RuntimeError("Нет результатов всех рабочих очередей")

results = {}
failed = []
for item in cases:
    name = item["id"]
    paths = {kind: ROOT / kind / name for kind in ("regression", "regression-tail", "regression-retry")}
    first = paths["regression"] if (paths["regression"] / "exits.json").is_file() else paths["regression-tail"]
    summary = first / "exits.json"
    original = json.loads(summary.read_text()) if summary.is_file() else {}
    attempts = [{"path": str(first.relative_to(ROOT)), "exit": original}]
    selected = {
        side: first / side for side in ("nlbnavigator", "instead")
        if original.get(side) == 0
    }
    retry = paths["regression-retry"]
    if (retry / "instead/command.exit").is_file() and int((retry / "instead/command.exit").read_text()) == 0:
        selected["instead"] = retry / "instead"
        attempts.append({"path": str(retry.relative_to(ROOT)), "exit": {
            "instead": 0, "compare": int((retry / "compare.exit").read_text())}})
        good = original.get("nlbnavigator") == 0 and attempts[-1]["exit"]["compare"] == 0
    else:
        good = len(original) == 3 and not any(original.values())
    results[name] = {"ok": good, "attempts": attempts}
    if not good:
        failed.append((item, selected))

def retry_case(entry, display):
    item, good = entry
    path = ROOT / "regression-final" / item["id"]
    path.mkdir(parents=True)
    route = REPO / item["path"]
    outputs = {}
    for side in ("nlbnavigator", "instead"):
        if side in good:
            (path / side).symlink_to(good[side], target_is_directory=True)
            outputs[side] = 0
        else:
            outputs[side] = engine(route, path, side, display)
    outputs["compare"] = stock_compare(route, path)
    return item["id"], str(path.relative_to(ROOT)), outputs

def lane(index):
    return [retry_case(item, 390 + 20 * index) for item in failed[index::2]]

with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
    for group in pool.map(lane, range(2)):
        for name, path, codes in group:
            results[name]["attempts"].append({"path": path, "exit": codes})
            results[name]["ok"] = not any(codes.values())
(ROOT / "regression-verdict.json").write_text(json.dumps(results, ensure_ascii=False, indent=2) + "\n")
print(f"PASS {sum(item['ok'] for item in results.values())}/{len(results)}", flush=True)
raise SystemExit(int(not all(item["ok"] for item in results.values())))
