"""Проверяет экран сразу после команды слота напрямую по JSONL и TSV."""
import json

from matrix76 import OUT, NLB, INSTEAD, scene, read_instead_trace, trace_sequence_valid, ordered_choices

PAGE = "8c1fa8c6-8027-43c8-8d8b-0bf454d5e717"
DOCTOR = "d937c5c1-766c-4581-a03e-736560b0cc8f"
CORPSE = "a2b6a777-a95c-4518-8623-ec797a483c46"
EXPECTED = {
    "page": PAGE, "screen": "choices", "index": 1, "count": 2, "more": False,
    "background": "3", "visible": ordered_choices,
    "variables": dict(wolfiness="0", hp="2", barExit="false", forestExit="false",
                      wolvesTalk="false", guardDone="false"),
}
proof = {}
for case_name, verbs, branch, choice_action in (
    ("choices", ["save", "load", "save"], DOCTOR, "clickchoice:0"),
    ("choices-restart", ["load"], CORPSE, "clickchoice:1"),
):
    case = OUT / case_name
    nlb = [json.loads(line) for line in (case / NLB / "state-log.jsonl").read_text().splitlines()]
    sdl = [dict(verb="save" if prev["command"] == "key:f5" else "load",
                index=row["index"], state=scene(row, NLB), ok=row["ok"] and prev["ok"])
           for prev, row in zip(nlb, nlb[1:])
           if prev["command"] in ("key:f5", "key:f9") and row["command"] == "menu:1"]
    assert [row["verb"] for row in sdl] == verbs and all(row["ok"] for row in sdl), sdl
    for row in sdl:
        assert row["state"] == EXPECTED, row

    tsv = read_instead_trace(case / INSTEAD / "instead-state.tsv")
    assert trace_sequence_valid(tsv), case
    operations = [idx for idx, r in enumerate(tsv)
                  if r["kind"] == "scene" and r["command"].endswith("/save9")]
    states = []
    for index in operations:
        row = tsv[index]
        verb = row["command"].split(" ", 1)[0]
        if verb == "save":
            # SDL+INSTEAD снимки проверяются по свежей записи команды. Пустой rendered
            # у записи INSTEAD дополняется текстом последней подходящей сцены.
            last = next(p for p in reversed(tsv[:index])
                        if p["kind"] == "scene" and p["screen_kind"] == "choices" and p["rendered"].strip())
            assert scene(last, INSTEAD) == EXPECTED, last
            checked = scene(dict(row, rendered=last["rendered"]), INSTEAD)
        else:
            checked = scene(row, INSTEAD)
        assert checked == EXPECTED, row
        states.append(dict(verb=verb, seq=row["seq"], empty_rendered=not row["rendered"], state=checked))
    assert [r["verb"] for r in states] == verbs, states

    # Последняя команда слота оставляет готовую к выбору панель.
    after = [r for r in nlb if r["index"] > sdl[-1]["index"]
             and not r["command"].startswith(("shot:", "settle:"))]
    assert after[0]["command"] == choice_action and after[0]["ok"] and after[0]["pageId"] == branch, after[0]
    instead_branch = next(r for r in tsv[operations[-1] + 1:]
                          if r["kind"] == "scene" and r["command"].startswith("act "))
    assert instead_branch["page_id"] == branch, instead_branch
    proof[case_name] = {NLB: sdl, INSTEAD: states, "destination": branch,
                        "choice_index": after[0]["index"], "choice_seq": instead_branch["seq"]}
(OUT / "choices-primary-proof.json").write_text(json.dumps(proof, ensure_ascii=False, indent=2) + "\n")
print("Проверены четыре команды слота и два прямых перехода по исходным JSONL/TSV.")
