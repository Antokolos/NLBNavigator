# NLBNavigator differential testing — persistent progress log

> Read this file first when resuming this long-running task after a chat reset or context loss.
> Update it in the same working PR branch after every meaningful testing stage. GitHub PR and CI are the live source of truth; this file is the durable human-readable handoff.

Last updated: 2026-11-02 (Asia/Bishkek)
Repository: https://github.com/Antokolos/NLBNavigator
Working PR: https://github.com/Antokolos/NLBNavigator/pull/32
Working branch: `ci/nlb-sdl-wolfman-state-trace`
Target branch: `main`

## 1. Non-negotiable project rules

- Goal: execute the same story paths in NLBNavigator and INSTEAD, compare navigation, branch decisions, visible text, page identity, images/resources, game variables, inventory/objects and UI, and document every real discrepancy with reproducible evidence.
- PR #32 must remain **Draft** until the owner explicitly approves. Do not merge it.
- Do not push changes to `main` or otherwise modify the default branch.
- Do not edit source story repositories as part of differential testing. Use pinned references and test-only overlays/adapters.
- Pixel differences between the two renderers are diagnostic/advisory, not a pixel-perfect CI gate.
- Distinguish a game/runtime mismatch from a test-input, timing, runner, or expected-value problem. Avoid filing cascading downstream failures as separate issues.
- Use clean, independent runs for alternative branches.
- Each stage should end with: factual report, exact commit/run references, outstanding work, next action.

## 2. Repository and pinned test inputs

NLBNavigator repository: `Antokolos/NLBNavigator`

PR #32:
- Title: `Адаптировать differential runner к state-log и VN trace`
- State: open, Draft, not merged.
- Target: `main`
- Working branch: `ci/nlb-sdl-wolfman-state-trace`
- At the start of this log update, branch HEAD was `abf3a48d01d14ef9333222f04adc07d07eb638c8`. After this log/route commit, always query PR #32 for the newest actual HEAD; do not assume this SHA remains current.

Pinned source inputs from `tests/differential/catalog.json`:
- NLB book `nlbproject/wolfman`: `888b4b7a7a564693274f53341728ee9f3b4c2879`
- INSTEAD export `nlbproject/instead-am_i_a_monster_vn`: `5f8cd646905a25800afab46bd2ce17616f0d6494`
- INSTEAD base theme `instead-hub/instead`: `1a07e0ffc0a1e8402b9d47a104d88d3f33204a7e`
- `wolfman` and `instead-am_i_a_monster_vn` are the same story, two packages (owner-confirmed).
- The repository catalog also contains five more paired stories and one NLB-only book; see `tests/differential/catalog.json`.

## 3. Test infrastructure already present

- `.github/workflows/nlb-sdl-test.yml`: builds `nlb_sdl`, prepares pinned story/export/theme, validates definitions, runs opening and two extended wolfman routes in both runtimes, compares reports and uploads diagnostics.
- `tests/differential/run_route.py`: common route runner/adapters and comparisons.
- `tests/differential/instead_trace.lua`: test-only INSTEAD adapter; wraps outer `iface.cmd`; uses isolated overlay and per-run `-appdata`.
- `tests/differential/catalog.json`: pinned source catalog.
- `tests/differential/routes/wolfman-opening.json`
- `tests/differential/routes/wolfman-eat-robber.json`
- `tests/differential/routes/wolfman-chase-robber.json`
- `tests/differential/validate_catalog.py`
- `tools/check_state_log.py` and `docs/state-log.md`: structured NLB state snapshots after script commands/checkpoints.
- `tests/differential/README.md`: execution instructions and current described route coverage.

## 4. Confirmed baseline before the latest expansion

CI #41329653783 (run 107) passed on commit `3a92316a0f4f5f3075ff840ab9f22bbaf98b2d5d`:
https://github.com/Antokolos/NLBNavigator/actions/runs/41329653783

At that checkpoint:
- `nlb_sdl` built successfully.
- Both robber branches passed in both engines through the zoo, truckers' bar, and the conditional transition after the bar.
- INSTEAD executed 19 control steps in each branch.
- NLBNavigator state-log had 85 records for eat and 87 for chase.
- Final branch states matched:
  - eat: page 8, UUID `b2f0838e-3068-4850-ad2f-4944075de38b`, `wolfiness=1`, `hp=2`;
  - chase: page 12, UUID `94e84eb0-0ba0-475f-b839-71da445fa595`, `wolfiness=0`, `hp=3`.
- Both comparison reports had `errors: []`. Cross-renderer pixel diffs remained advisory.
- Inventory line `Волчность: 0 Жизнь: 3` is expected UI, not a regression (see PR history and merged prior work).

## 5. Latest extended run and exact failure

CI #41340223549 (run 111), HEAD `abf3a48d01d14ef9333222f04adc07d07eb638c8`, completed with **failure**:
https://github.com/Antokolos/NLBNavigator/actions/runs/41340223549

What passed:
- configure/build/smoke test for `nlb_sdl`;
- checkout of all pinned inputs and route-definition validation;
- common opening route in INSTEAD and NLBNavigator and screenshot comparison;
- both extended INSTEAD branches (eat and chase) completed.

What failed/skipped:
- `Run eat-robber branch in NLBNavigator` failed validation;
- NLBNavigator chase and branch comparison steps were skipped because of fail-fast workflow behavior.

Artifact:
- `nlb-sdl-test-logs`, artifact ID `13097153851`, available from the CI run above.
- Extracted locally during diagnosis; relevant files:
  - `ci-artifacts/robbery-eat/instead/route-report.json`
  - `ci-artifacts/robbery-eat/nlbnavigator/route-report.json`
  - `ci-artifacts/robbery-eat/nlbnavigator/state-log.jsonl`
  - checkpoint screenshots in the same `nlbnavigator` directory.

## 6. Confirmed discrepancy: VN paragraph pagination on wolfman page 8

Status: observed, reproducible in the pinned revisions; Issue created during this stage. See the issue link on the PR/repository Issues page; search by title:
`VN pagination differs on long paragraphs (wolfman page 8: 15 vs 17 portions)`.

Affected page:
- UUID: `b2f0838e-3068-4850-ad2f-4944075de38b`
- NLB image suffix: `8.png`; INSTEAD background: `images/8.jpg`
- INSTEAD reports 15 non-empty visible portions (indices 0–14).
- NLBNavigator state-log reports 17 portions (indices 0–16).

Root evidence:
- Pinned NLB source text: `pages/34e50ebc-ea2b-4714-81a1-ae8fca22c399/module/pages/b2f0838e-3068-4850-ad2f-4944075de38b/text/ru`.
- Pinned INSTEAD export: `module_ru.lua`; the two philosopher speeches are each emitted as one `pn(...)` paragraph.
- Pinned INSTEAD `modules/paginator.lua` uses paragraph delimiter `\n\n` to create portions.
- NLBNavigator `Source/NLBPresentation/Private/nlb/presentation/GameScreen.cpp`, helper `splitIntoPortions`, currently uses sentence splitting with a threshold of 100 codepoints and a tail-warning threshold of 330.
- The INSTEAD trace shows these as single portions; NLBNavigator splits each of the two long speeches into two portions:
  1. Speech beginning `Видите ли, я не всегда работал дальнобойщиком...` splits after `Но жить на зарплату преподавателя...`.
  2. Speech beginning `Что самое интересное, -- продолжал философ...` splits after `...кол из боярышника.`.
- Remaining contents are present; the net difference is exactly two extra NLB advances.
- The route then stopped two advances too early; failures in later page UUID, screenshot, choice, text and variable checks were cascading checkpoint misalignment, not independent bugs.
- This is a functional/navigation/pagination discrepancy, not a pixel-diff issue. The issue asks the owner/maintainers to establish the desired contract rather than presupposing which engine should change.

### Route correction staged in the current branch update

In `tests/differential/routes/wolfman-eat-robber.json`:
- checkpoint `wolfiness-positive-final-portion` now expects NLB portion index 16/count 17, while INSTEAD remains index 14/count 15;
- NLB route advances 16 times at this point, INSTEAD 14 times;
- philosopher choice click for NLB is moved into the first visible choice's hit area (ratio x=0.13, y=0.12 at 1920x1080) instead of the previous center-screen click x=0.5/y=0.5.
These adjustments are **test-route alignment only**. They do not change engine or story behavior. Their correctness still requires a new CI run.

## 7. Other wolfman route coverage and remaining validation

At the current stage, the extended JSON routes contain:
- eat-robber route: 26 checkpoints, from title/opening through page 8, philosopher choice, page 10 and the `wolfiness > 1` forest outcome (page 16).
- chase-robber route: 27 checkpoints, from title/opening through page 12, kitchen choice, page 10 and the `wolfiness <= 1` forest outcome (page 17).

Do not claim these extended NLB branches are fully tested until a current CI run passes both.
After the corrected eat route:
1. Inspect the new CI result and artifacts.
2. Confirm the philosopher selection actually changes state to the expected page and variables.
3. Inspect the NLB chase run; do not assume its portion count matches INSTEAD.
4. Fix any further route alignment/input problems one at a time.
5. Compare both branches only after both runtimes pass; verify report `errors: []` and state-log validation.
6. Inspect remaining choices in wolfman and expand branch coverage until every reachable branch/ending is covered or explicitly marked unsupported/unreachable with evidence.

## 8. Issue #28 and broader suite

Issue #28 remains open:
https://github.com/Antokolos/NLBNavigator/issues/28

It tracks the long-term goal:
- one reproducible route set per paired story;
- coverage inventory for every reachable branch and terminal ending;
- structured checkpoint evidence including page, text, choices, variables, resources and screenshot;
- functional/text/state/resource mismatches separated from visual-only differences;
- pinned commits/runtime versions for repeatability.

Six paired stories in the catalog:
`wolfman`, `barbarian_caves_demo`, `chaperonrougehub`, `servant-of-ra`, `Frontier`, `wild-island-quest`.
`bookU` is NLB-only/unpaired in the current source catalog.
Only wolfman currently has authored shared routes. The other five paired stories still require route definitions and adapters.

The original Issue #28 checkbox list is stale relative to the implementation; update it only after new verification, not by claiming complete coverage prematurely.

## 9. Handoff / recovery procedure

For every new chat or after a context reset:
1. Read this file first.
2. Fetch PR #32 and confirm it is still Draft/open/unmerged.
3. Query the actual latest HEAD of `ci/nlb-sdl-wolfman-state-trace`.
4. Fetch CI runs for that branch and identify the newest run associated with that HEAD; inspect job steps and artifacts, not only the PR description.
5. Read the latest relevant issue(s) and this file's last section.
6. Continue at the first unfinished item. Never repeat the confirmed baseline run unless a code/test change requires rerunning it.
7. After each material milestone, append dated status with commit SHA, CI URL and outcome; when a discrepancy is found, create one evidence-based issue and reference it here.
8. Keep this journal in the working PR branch. Never merge PR #32 or push to `main` without explicit approval.

## 10. Current next action

Run the updated wolfman differential workflow from the latest PR branch head. First inspect whether the eat route now:
- reaches page 8 final portion at NLB index 16/count 17 and INSTEAD index 14/count 15;
- reaches page 13 choice screen;
- selects `Съесть философа` using the adjusted coordinate;
- reaches the expected page 9 state (`wolfiness=2`, `hp=3`), page 10 and forest outcome.

Then inspect chase branch and update this journal with actual results.
