"""Перераспределяет ещё не начатые прогоны без перезапуска текущих процессов."""
import concurrent.futures
import json
import time

from execute import ROOT, REPO, pair

entries = json.loads((REPO / "tests/differential/catalog.json").read_text())["routes"]
first = ROOT / "regression"
tail = ROOT / "regression-tail"
tail.mkdir()
claimed = []
prefix = []
for lane in range(2):
    sequence = entries[lane::2]
    for index, item in enumerate(sequence):
        directory = first / item["id"] / "nlbnavigator"
        try:
            # Идущий раннер создаёт каталог сам. Резервирование нового каталога
            # гарантирует, что старый координатор завершится до повторного запуска маршрута.
            directory.mkdir(parents=True)
        except FileExistsError:
            continue
        claimed.extend(sequence[index:])
        prefix.extend(sequence[:index])
        break
    else:
        prefix.extend(sequence)
assert len({item["id"] for item in prefix + claimed}) == len(entries)
(tail / "schedule.json").write_text(json.dumps(
    {"initial_routes": [item["id"] for item in prefix],
     "new_routes": [item["id"] for item in claimed],
     "note": "Координатор первых двух очередей остановится на зарезервированном каталоге после завершения начатых маршрутов."},
    ensure_ascii=False, indent=2) + "\n")
for _ in range(150):
    if (ROOT / "regression.exit").exists():
        break
    time.sleep(3)
else:
    raise RuntimeError("Первый пул не освободил места за 450 секунд")

def lane(index):
    return {e["id"]: pair(REPO / e["path"], tail / e["id"], 310 + 20 * index)
            for e in claimed[index::3]}

with concurrent.futures.ThreadPoolExecutor(max_workers=3) as executor:
    results = dict(item for group in executor.map(lane, range(3)) for item in group.items())
(tail / "results.json").write_text(json.dumps(results, indent=2) + "\n")
