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

Status: observed, reproducible in the pinned revisions; Issue created during this stage. Issue #37: https://github.com/Antokolos/NLBNavigator/issues/37 — `VN pagination differs on long paragraphs (wolfman page 8: 15 vs 17 portions)`.

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
- philosopher choice click for NLB is moved into the first visible choice's hit area (ratio x=0.26, y=0.22 at 1920x1080; verified against the generated choice-panel geometry in `GameScreen::renderChoicesScreen` (panel x≈312, first hit-row begins x≈322/y≈228)) instead of the previous center-screen click x=0.5/y=0.5.
These adjustments are **test-route alignment only**; they do not change engine or story behavior. Their correctness is now confirmed by CI #41408858828 (run 113): both extended INSTEAD and NLBNavigator branches completed, and both comparisons passed with `errors: []`.

## 7. Current confirmed wolfman coverage and remaining validation

The latest successful run for the current branch head is CI #41408858828 (run 113):
https://github.com/Antokolos/NLBNavigator/actions/runs/41408858828

- HEAD: `9df55828fea027e7c7d7a2085f19a3756a799fdb`.
- Workflow: **success**, 25/25 steps.
- Build, setup, shared opening route, all four extended runs (eat/chase in both engines), both comparisons, and INSTEAD export validation passed.
- eat: 26 checkpoints in each engine; chase: 27 checkpoints in each engine.
- INSTEAD route reports and NLBNavigator route reports all have `ok: true`, `errors: []`.
- Comparison reports: eat 26 paired checkpoints and chase 27; both `ok: true`, `errors: []`. Cross-renderer pixel metrics remain advisory.
- NLBNavigator state-log validation: eat **121 records**, chase **118 records**. Both files validate successfully.
- Verified end checkpoints: eat page 16 (`wolfiness=2`, `hp=3`); chase page 17 (`wolfiness=0`, `hp=3`).
- Artifact `nlb-sdl-test-logs`, ID `13125976222`, available from the CI run (12.8 MB). It contains route reports, state logs, trace TSV, progress files, screenshots and comparison outputs.

The route configs still have **26 / 27 checkpoints**. They do not yet choose left/right on page16/page17 and do not cover downstream forest, traveler, guard, weapon, hospital or ending branches. Static source graph and the next route matrix are documented in `tests/differential/WOLFMAN_BRANCH_MAP.md`.

Remaining:
1. Add focused paired routes for the first forest decisions and confirm each action in the trace.
2. Extend state telemetry to `barExit`, `forestExit`, `wolvesTalk`, `guardDone` after verifying active module variable visibility.
3. Add hospital/police branch and positive-wolfiness kitchen alternative.
4. Cover the traveler choices, guard decisions, weapons and all terminal endings.
5. Add achievement capture and page36 tagged-background verification.
6. Resolve Issue #37's pagination contract separately; don't hide the 15/17 difference or turn it into a pixel gate.

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

1. Read `tests/differential/WOLFMAN_BRANCH_MAP.md` before authoring a new route.
2. Run the four new forest routes added in this commit. CI #115 already verified the four telemetry flags; the new runs continue to page18/page24/page25 and compare the actual state and content.
3. Author a minimal route for page16 left/right (and the matching low-wolfiness page17 choices) with explicit semantic assertions. Run each in both engines.
4. Then add traveler, guard, weapon and ending paths; keep each clean route small enough to diagnose.
5. Append the resulting SHA/CI/coverage changes here after every material stage.

PR #32 remains Draft/open/unmerged. Never modify `main` or merge without explicit approval.

### 2026-11-02 — successful extended differential run

- CI #41408858828 (run 113), HEAD `9df55828fea027e7c7d7a2085f19a3756a799fdb`: **success, 25/25 steps**.
- Both extended routes passed in INSTEAD and NLBNavigator. Eat: 26 checkpoints / 121 NLB state-log records. Chase: 27 checkpoints / 118 NLB state-log records.
- Both comparison reports: `ok=true`, `errors=[]`; pixel metrics advisory.
- Final checkpoints agree: page16 / `wolfiness=2,hp=3`; page17 / `wolfiness=0,hp=3`.
- Artifact ID `13125976222` (`nlb-sdl-test-logs`).
- Issue #37 remains open for the known 15-versus-17 portion split on page8.
- Static source exploration expanded the story map through page41/page42 and found the police/hospital route; details are in `WOLFMAN_BRANCH_MAP.md`. These downstream paths are not yet runtime-tested.
- NLB source variable names confirmed: `barExit`, `guardDone`, `forestExit`, `wolvesTalk` (plus `hp`, `wolfiness`). Runtime state-log still records only `hp,wolfiness`.


### 2026-11-02 — state telemetry expansion (awaiting CI)

- Added `barExit`, `forestExit`, `wolvesTalk`, `guardDone` to NLB `--state-vars` alongside `wolfiness,hp`.
- Extended INSTEAD trace TSV from 20 to 24 columns with the same four flags; updated parser columns.
- Updated INSTEAD expected-value handling so JSON booleans compare against lowercase Lua `true/false` strings.
- Added the expected flags to page10, forest-entry, and page16/page17 terminal checkpoints in both routes: `barExit=true`, `forestExit=false`, `wolvesTalk=false`, `guardDone=false`.
- Updated README and branch map. The current code commit must be followed by CI; do not mark telemetry verified until state logs/trace and both comparisons pass.


### 2026-11-03 — CI #115 telemetry verified; forest routes added

- CI #41412052519 (run 115), HEAD `3fe8603a5ffc7e37dad58196af400ec20562cce1`: **SUCCESS, 25/25 steps**.
- Artifact: `nlb-sdl-test-logs`, ID `13126997642`, 12,789,839 bytes.
- INSTEAD TSV in both extended branch runs has 24 columns ending with `wolfiness,hp,barExit,forestExit,wolvesTalk,guardDone`.
- NLB state-log validates: **121 records** (eat) and **118 records** (chase); each record includes all six variables.
- At page10, forest entry, and page16/page17 terminal checkpoints, both runtimes report:
  - page16/eat: `wolfiness=2,hp=3,barExit=true,forestExit=false,wolvesTalk=false,guardDone=false`;
  - page17/chase: `wolfiness=0,hp=3,barExit=true,forestExit=false,wolvesTalk=false,guardDone=false`.
- Both extended branch reports and both comparison reports have `ok=true`, `errors=[]`. The four flags are verified runtime facts.
- Added four independent clean-state route configs for page16/page17 left/right. Each replays the matching previously validated robber/bar prefix because each route must launch a separate fresh session.
- Added runner support for an `advance` action with zero clicks in one runtime, provided the other runtime advances. INSTEAD clicks once to expose its choices overlay; NLBNavigator captures the existing model choices without an extra click.
- Updated workflow to execute and compare all four routes. New CI is pending.


### 2026-11-03 — CI #116 first forest route: coordinate miss

- CI #41597493394 (run 116), HEAD `5cf6c480b754b934382280ef4209d9f24be8d17d`, failed at step `Run forest wolfiness two left in INSTEAD`. Build, all 11 preparation/validation steps, and the shared route plus screenshot comparison passed.
- The independent route reached all 27 checkpoints through page16 and correctly recorded the choices screen:
  - page UUID `3792ac23-4da9-4539-a393-95780a98771a`;
  - rendered links `<a:act 01>Налево</a>` and `<a:act 02>Направо</a>`;
  - flags `wolfiness=2,hp=3,barExit=true,forestExit=false,wolvesTalk=false,guardDone=false`.
- The final click used screen coordinate (499,292), recorded as `click true,1,499,292`, but the trace had no `act 01` and no new scene. The screenshot shows that point just to the right of the short “Налево” hit area; changed pixels were only 614, so it was a click miss, not a game transition.
- Artifact ID `13213121943` (`nlb-sdl-test-logs`) contains the full INSTEAD trace, partial trace, progress JSON and the two relevant screenshots.
- Corrective change: all four page16/page17 direction routes now use `instead.x_ratio=0.23` (screen x≈442 on 1920px width), centered within both short link labels; y remains 0.27 for row 1 and 0.36 for row 2. This correction is test-input-only; no game or source story code changed.
- New CI on the corrective commit is required before marking any of the four forest routes passed.
