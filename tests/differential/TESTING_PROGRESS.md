# NLBNavigator differential testing — persistent progress log

> Read this file first when resuming this long-running task after a chat reset or context loss.
> Update it in the same working PR branch after every meaningful testing stage. GitHub PR and CI are the live source of truth; this file is the durable human-readable handoff.

Last updated: 2026-11-05 (Asia/Bishkek)
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

- `.github/workflows/nlb-sdl-test.yml`: builds `nlb_sdl`, prepares pinned story/export/theme, validates definitions, runs the shared opening route, robber/bar routes and independent forest/traveler/bypass routes in both runtimes, compares reports and uploads diagnostics.
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

Latest successful full workflow: CI [#41901754643, run 126](https://github.com/Antokolos/NLBNavigator/actions/runs/41901754643), HEAD `2cc7ace2613d6929d7ac1d55a3389fefc0d8d60f`.

- Workflow: **SUCCESS**. Build, smoke-test, shared route, all previously covered forest/traveler/bypass routes, all three new guard routes in both runtimes, all comparisons, INSTEAD reference validation and artifact upload passed.
- All six guard runtime reports (three routes × two engines): `ok=true`, `errors=[]`. All three guard comparison reports: `ok=true`, `errors=[]`.
- Text-check counts: talk 28 per runtime; attack 26; low-wolfiness escape 27. Comparison reports cover 35, 33 and 34 checkpoints respectively.
- NLB state-log validators: **163 records** (high-wolves talk), **151** (high-wolves attack), **141** (low-wolfiness escape). All validators report `ok`.
- Matching talk outcome: page27 UUID `2eb72ffe-865a-4e51-aa9c-60af1b055fc3`, `27.png` / `images/27.jpg`, `wolfiness=2,hp=3,barExit=true,forestExit=true,wolvesTalk=true,guardDone=false`.
- Matching attack outcome: page28 UUID `d45d6de2-e019-4a32-87f8-5f97ab2fcbf9`, `28.png` / `images/28.jpg`, `wolfiness=2,hp=3,barExit=true,forestExit=true,wolvesTalk=false,guardDone=false`.
- Matching low-wolfiness outcome: page1.8 UUID `433426b4-2e3f-4461-ab7a-e87119808471`, `1.8.png` / `images/1.8.jpg`, `wolfiness=0,hp=3,barExit=true,forestExit=true,wolvesTalk=false,guardDone=false`.
- Runtime traces confirm page1.7 has **3 portions in both engines**, and page26 has **8 portions in both engines**. This supersedes the source-block-count estimate from earlier notes.
- INSTEAD session logs record achievement calls `INFO_FOR_GUARD` and `GUARD_NOT_A_PROBLEM` on the corresponding routes. This proves the calls occurred in those sessions, but does not verify persistent achievement storage or parity with NLBNavigator.
- Artifact: `nlb-sdl-test-logs`, ID `13363351670`. Cross-renderer pixel metrics remain advisory; comparisons are based on route semantics, page identity, resources, text and state.

Remaining:
1. Continue the low-wolfiness path from page1.8 to page1.9, then cover the hawthorn/rowan/viburnum weapon choices and their results.
2. Add the alternative police/hospital start and determine runtime effects of the corpse choice on page14.
3. Cover villain approach, page36 tagged-background selection and all six endings.
4. Add read-only achievement-state capture/read-back in both runtimes; current INSTEAD session logs prove only that achievement calls executed.
5. Resolve Issue #37's page8 pagination contract separately; do not convert it into a pixel gate.

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

CI #120 confirmed the four traveler-decision variants; CI #121 confirmed all four page22 bypass routes. Most recently, CI #126 (**SUCCESS**, HEAD `2cc7ace2613d6929d7ac1d55a3389fefc0d8d60f`) confirmed all three guard routes in INSTEAD and NLBNavigator, with all three comparisons passing. See the CI #126 entry below for exact page UUIDs, images, variables and state-log counts.

Next: continue from page1.8 to page1.9 and test weapon selection, then cover the alternative police/hospital start and endings. Achievement state read-back and page36 tag-dependent background remain separate tasks.

PR #32 remains Draft/open/unmerged. Do not modify `main` or merge without explicit approval.

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


### 2026-11-03 — CI #118: NLB choice panel not opened

- CI #41599355620 (run 118), HEAD `db531735164e132bf24b74acfb08778bb7385e39`, built successfully and passed setup plus the shared route comparison.
- All four new forest routes passed in INSTEAD, including both page16 choices and both page17 choices; the INSTEAD right branches reached page24/page25 as expected.
- The first NLBNavigator route failed at `clickchoice:0`. The last saved NLB state-log record shows page16 and both choices in the model (`Налево`, `Направо`), but `screen=page`; the screenshot has only the VN page text and no choices overlay. Thus the earlier assumption that model choices imply a visible choice panel was wrong.
- In NLBNavigator, the first click on a completed VN page must call `advanceToChoices()`, which sets `m_showChoices`; only then does `clickchoice:N` find the `Hit::Kind::Choice` rectangle created by `renderChoicesScreen()`.
- Corrective route-only change: the forest choices reveal checkpoint now clicks once in **both** engines (`advance.clicks.instead=1`, `advance.clicks.nlbnavigator=1`) and expects NLB `screen=choices`; after that the separate `clickchoice:0/1` selects the visible panel option.
- No story or engine code was changed. The first INSTEAD route input x-coordinate correction remains intact (x_ratio=0.23 only on forest link selections; baseline prefix choices retain original coordinates).
- CI #118 artifact ID `13215290301` contains NLB page16 choices screenshot/state log, confirming the above diagnosis.
- The corrected four route configs need a fresh CI before any NLB forest branch is marked passed.


### 2026-11-04 — CI #119: page16/page17 forest directions verified

- CI [#41602166825](https://github.com/Antokolos/NLBNavigator/actions/runs/41602166825), run 119, HEAD `417bfc58dd7e41ca44beafcbc00cefa4e459dde7`: **SUCCESS, 29/29 steps**.
- Artifact `nlb-sdl-test-logs`, ID `13216749996`, 26,045,630 bytes.
- All eight runtime route reports (four INSTEAD, four NLBNavigator) have `ok=true`, `errors=[]`; all four comparisons have `ok=true`, `errors=[]`.
- NLB state-log validators passed: 127, 130, 124 and 127 records for wolfiness 2/left, 2/right, 0/left and 0/right.
- Matching final checkpoints: page18 for both left routes, page24 for wolfiness 2/right (`wolvesTalk=true`), page25 for wolfiness 0/right (`wolvesTalk=false`). Every route has `barExit=true,forestExit=false,guardDone=false`.
- The NLB choice-panel reveal correction is verified. Pixel metrics between renderers remain advisory rather than pixel-perfect gates.
- This checkpoint is the verified baseline for the traveler continuation below.

### 2026-11-04 — CI #120: traveler decisions verified in both engines

- CI [#41695602249](https://github.com/Antokolos/NLBNavigator/actions/runs/41695602249), run 120, HEAD `a7b0411f36b61bf81bb8cef9970be15955638b07`: **SUCCESS, 29/29 steps**.
- Artifact `nlb-sdl-test-logs`, ID `13262693170`, 29,922,904 bytes.
- All eight runtime route reports and all four comparison reports have `ok=true`, `errors=[]`; every text checkpoint passed (24–26 checks per route).
- NLB state-log validation passed with 139, 151, 136 and 139 records for the four wolfiness/traveler variants.
- Matching end states: page20 after eating (w=2→3,hp=3→4) on high branch; page21 after declining (w=2,hp=3,wolvesTalk=true); page21 after declining (w=0,hp=3); page20 after eating on low branch (w=0→1,hp=3→4). UUIDs/backgrounds are recorded in the branch map and PR description.
- All final checkpoints agree on `barExit=true,forestExit=false,guardDone=false`; pixel differences remain advisory.

### 2026-11-04 — right-turn bypass routes added; awaiting CI

- `wolfman-forest-wolfiness-two-page18-bypass`: wolfiness=2, origin page 18, `wolvesTalk=false`.
- `wolfman-forest-wolfiness-two-page24-bypass`: wolfiness=2, origin page 24, `wolvesTalk=true`.
- `wolfman-forest-wolfiness-zero-page18-bypass`: wolfiness=0, origin page 18, `wolvesTalk=false`.
- `wolfman-forest-wolfiness-zero-page25-bypass`: wolfiness=0, origin page 25, `wolvesTalk=false`.
- Each expects page22 UUID `86893ac1-6506-4fdd-bbfc-8008d6ad6b31`, `22.png` / `images/22.jpg`, text fragment “дом врага уже совсем близко”, and `forestExit=true`.
- Workflow runs every route independently in INSTEAD and NLBNavigator and creates one comparison per route. No story or engine source was changed.


### 2026-11-04 — CI #121: page22 bypass and forest exit verified

- CI [#41704728633](https://github.com/Antokolos/NLBNavigator/actions/runs/41704728633), run 121, HEAD `c77f1ddb9a369dd528ac2cecb3c0b8046c8b08cf`: **SUCCESS, 41/41 steps**.
- Artifact `nlb-sdl-test-logs`, ID `13268556774`, 56,239,646 bytes.
- All four new bypass routes passed in INSTEAD and NLBNavigator; all four earlier direction/traveler routes also passed as part of the suite.
- All 8 bypass runtime reports: `ok=true`, `errors=[]`; text checks passed (23–25 per route). All 4 comparisons: `ok=true`, `errors=[]`.
- NLB state-log validation passed: 130, 145, 133 and 139 records for page18/w=2, page24/w=2, page18/w=0 and page25/w=0 bypass routes.
- INSTEAD trace and NLB state-log agree on page22 UUID `86893ac1-6506-4fdd-bbfc-8008d6ad6b31`, background `images/22.jpg` / `22.png`, visible text and `forestExit=true`. They also preserve `wolfiness`, `hp`, `barExit`, `wolvesTalk` and `guardDone` as expected.
- No story source, export, engine code or pinned input changed. Inter-renderer pixel metrics remain advisory.

### 2026-11-04 — guard branch routes added; awaiting CI

- `wolfman-guard-high-wolves-talk`: high wolfiness, `wolvesTalk=true`; page22 → page1.7 → page26 → page27.
- `wolfman-guard-high-wolves-attack`: high wolfiness, `wolvesTalk=false`; page22 → page1.7 → page26 → page28.
- `wolfman-guard-low-wolfiness-escape`: low wolfiness; page22 → page1.7 → page1.8.
- The tests verify page UUID, NLB/INSTEAD image, visible text and all six tracked variables at each new checkpoint. The high branches advance through the page26 dialog before checking page27/page28.
- These routes are configured for the page1.7 pagination split found in the pinned inputs: NLB `text/ru` has two nonempty text blocks, while the INSTEAD export's `dsc` has three nonempty `pn` segments. The page1.7 branch step therefore sends 2 clicks in NLBNavigator and 3 in INSTEAD; CI must confirm these runtime counts before marking the guard branch covered.
- These routes are static test definitions until the next CI passes; do not count them as runtime coverage yet.
- Next: inspect every route report, all three comparisons, the INSTEAD trace and NLB state-log; use artifacts to diagnose failures without replaying the already confirmed CI #121 bypass paths.


### 2026-11-04 — guard route pagination alignment

- Static input comparison found that page1.7 has two nonempty text blocks in the pinned NLB book but three nonempty `pn` segments in the INSTEAD export.
- Updated both high-wolfiness guard route definitions to advance 2 clicks in NLBNavigator and 3 in INSTEAD before expecting page26.
- This is a route-input alignment based on the pinned texts, not a story/engine modification. CI #122 was superseded while still in setup; the corrected commit will be verified by the next run.


### 2026-11-04 — CI #123: guard route pagination count diagnosed

- CI [#41837368137](https://github.com/Antokolos/NLBNavigator/actions/runs/41837368137), run 123, HEAD `0fbe9e19cdfa5ba046c8bb0d3a639034d3923756`: **FAILURE** at the first new guard route. Build, smoke test, shared route, all four forest direction/traveler routes, all four page22 bypass routes and their comparisons completed successfully.
- The INSTEAD run of `wolfman-guard-high-wolves-talk` reached page26 UUID `0e3e04d8-03e7-459d-8136-e7c135100101` with `wolfiness=2,hp=3,barExit=true,forestExit=true,wolvesTalk=true,guardDone=false`. The failed checkpoint was still on `images/26.jpg`, at INSTEAD portion 7 of 8; the final visible text was «Не знаю. Но очень надеюсь.».
- This is an off-by-one in the route input, not evidence of a game mismatch: the route sent seven INSTEAD clicks, which reached the last visible page26 portion but did not leave the page. The pinned NLB page26 text has seven blocks; the captured INSTEAD trace has eight nonempty portions. The required page26 advance is therefore **INSTEAD 8 / NLBNavigator 7**.
- The same incorrect count existed in the alternate high-wolfiness route. The low-wolfiness route also had an incomplete page1.7 advance: after entering page1.7, the conditional branch requires **INSTEAD 3 / NLBNavigator 2** clicks, not 1/1.
- Corrected all three route actions to those per-runtime counts. Since CI #123 stopped at the first guard route, it did not run the NLBNavigator version of that route, either remaining guard route, or any guard comparison. Guard branch behavior and achievements remain unverified.
- Artifact `nlb-sdl-test-logs`, ID `13332131136`, contains the route report, screenshots, full/partial INSTEAD trace and progress checkpoint. CI #122 was cancelled; no game data, INSTEAD export, engine source or `main` was changed by this test correction.
- A fresh CI run is required before marking any guard branch covered.


### 2026-11-04 — CI #124: guard transition reached; text assertion corrected

- CI [#41856596471](https://github.com/Antokolos/NLBNavigator/actions/runs/41856596471), run 124, HEAD `78e949f361bd2c32c866f066deb845e6a61b98d4`: **FAILURE** at the text assertion for the first guard-talk checkpoint. Build, smoke test, all previous forest routes, all four page22 bypass routes and their comparisons passed.
- The corrected page26 click counts worked: INSTEAD completed page26 (portion 7/8) and transitioned to page27 UUID `2eb72ffe-865a-4e51-aa9c-60af1b055fc3`, background `images/27.jpg`. The trace has no errors and records the expected variables `wolfiness=2,hp=3,barExit=true,forestExit=true,wolvesTalk=true,guardDone=false`.
- The remaining error was a text-checkpoint mismatch: the route expected «где найти оборотней», which occurs later on page27, while the first displayed portion is «Вспомнив недавнюю встречу, вы воскликнули:». The CI screenshot confirms that exact first portion. Updated both runtime text expectations to the text visible on initial entry.
- The INSTEAD session log records `Setting achievement 'INFO_FOR_GUARD'...` followed by `Achievement set.`. This confirms that the achievement call ran in this session; persistence/read-back of achievement state remains unverified.
- Static review found the same premature text expectation in the high-wolfiness fight route: page28 first shows «Ты пропустишь меня?», while «придётся драться со мной» appears later. Updated the checkpoint to the first visible text to avoid an equivalent false failure.
- The low-wolfiness route did not run in CI #124 because the workflow stopped at the first failure. Its 3/2 page1.7 click correction remains in place but still awaits runtime verification.
- Artifact: `nlb-sdl-test-logs`, ID `13340908124`. No gameplay/story source, INSTEAD export, or main branch changes.


### 2026-11-04 — CI #125: runtime paginator counts corrected

- CI [#41880453967](https://github.com/Antokolos/NLBNavigator/actions/runs/41880453967), run 125, HEAD `d8f303460b9734ae5d9194b2c8bd8f7d21048d53`: **FAILURE** in the NLBNavigator execution of `wolfman-guard-high-wolves-talk`. Build, smoke-test, all previous forest routes and all four page22 bypass routes/comparisons passed.
- The INSTEAD guard-talk route completed successfully: page22 → page1.7 → page26 → page27. The final checkpoint is page27 UUID `2eb72ffe-865a-4e51-aa9c-60af1b055fc3`, background `images/27.jpg`, with `wolfiness=2,hp=3,barExit=true,forestExit=true,wolvesTalk=true,guardDone=false`. Its route report has `ok=true`, `errors=[]`.
- NLB state-log validation also passed (`161 records`), but its captured paginator state disproved the earlier static estimate:
  - page1.7 UUID `5848772f-134b-412e-9e5e-f9d2feedb3b0` has runtime `portion.count=3`; after two clicks it remains on page1.7 at portion 2/3. The conditional branch is entered on the **third** click. The NLB source's two text blocks do not imply two paginator portions because one block is internally split.
  - page26 UUID `0e3e04d8-03e7-459d-8136-e7c135100101` has runtime `portion.count=8`; after seven clicks it remains at portion 6/8. The eighth click advances to page27.
- The INSTEAD trace independently confirms 3 portions on page1.7 and 8 on page26. Therefore the correct route counts are now **3/3 for page1.7** and **8/8 for page26** (INSTEAD/NLBNavigator), not the previously inferred 3/2 and 8/7. Updated all three guard routes, including the low-wolfiness page1.7 branch.
- The NLB route failed at both checkpoints only because it received too few clicks; it had not yet reached page26 or page27 when those expectations were evaluated. No gameplay mismatch is demonstrated. The page28 route and low-wolfiness route still need fresh execution.
- Artifact: `nlb-sdl-test-logs`, ID `13354465374`. The INSTEAD session log records `INFO_FOR_GUARD` being set; persisted achievement-state read-back is not yet tested.
- These corrections change only differential-test route JSON and documentation. No story/export/engine source or `main` changes.


### 2026-11-05 — CI #126: all three guard routes verified in both engines

- CI [#41901754643](https://github.com/Antokolos/NLBNavigator/actions/runs/41901754643), run 126, HEAD `2cc7ace2613d6929d7ac1d55a3389fefc0d8d60f`: **SUCCESS**.
- Build and smoke-test passed. The shared route, all earlier forest direction/traveler/bypass routes, the three guard routes in both engines, all comparisons, INSTEAD reference validation and artifact upload passed.
- For each guard route, both runtime route reports have `ok=true`, `errors=[]`; the three comparison reports also have `ok=true`, `errors=[]`.
- `wolfman-guard-high-wolves-talk`:
  - INSTEAD and NLB finish on page27 UUID `2eb72ffe-865a-4e51-aa9c-60af1b055fc3`, backgrounds `images/27.jpg` / `27.png`.
  - Both report `wolfiness=2,hp=3,barExit=true,forestExit=true,wolvesTalk=true,guardDone=false`.
  - 28 text checks per runtime; comparison report: 35 checkpoints, 28 text checks. NLB state-log: 163 records, validation `ok`.
  - INSTEAD session log records `INFO_FOR_GUARD` and `Achievement set.`.
- `wolfman-guard-high-wolves-attack`:
  - INSTEAD and NLB finish on page28 UUID `d45d6de2-e019-4a32-87f8-5f97ab2fcbf9`, backgrounds `images/28.jpg` / `28.png`.
  - Both report `wolfiness=2,hp=3,barExit=true,forestExit=true,wolvesTalk=false,guardDone=false`.
  - 26 text checks per runtime; comparison report: 33 checkpoints, 26 text checks. NLB state-log: 151 records, validation `ok`.
  - INSTEAD session log records `GUARD_NOT_A_PROBLEM` and `Achievement set.`.
- `wolfman-guard-low-wolfiness-escape`:
  - INSTEAD and NLB finish on page1.8 UUID `433426b4-2e3f-4461-ab7a-e87119808471`, backgrounds `images/1.8.jpg` / `1.8.png`.
  - Both report `wolfiness=0,hp=3,barExit=true,forestExit=true,wolvesTalk=false,guardDone=false`.
  - 27 text checks per runtime; comparison report: 34 checkpoints, 27 text checks. NLB state-log: 141 records, validation `ok`.
- Runtime paginator state confirms page1.7 has 3 portions and page26 has 8 portions in both engines. All guard route JSONs now use those verified click counts.
- Achievement-call logs are runtime evidence that the two relevant INSTEAD calls executed. Persistent achievement read-back and NLBNavigator achievement parity are **not** asserted.
- Artifact `nlb-sdl-test-logs`, ID `13363351670`. Cross-renderer pixel metrics remain advisory and are not a pixel-perfect gate.
- No story source, INSTEAD export, game engine or `main` was modified by this test stage. PR #32 remains Draft/open/unmerged.

Next action: run the pending low-wolfiness continuation through page1.9; after runtime validation, advance to page30, select the only available hawthorn option, and verify the resulting guard outcome.


### 2026-11-05 — CI #127 and low-wolfiness continuation prepared

- CI [#41915650980](https://github.com/Antokolos/NLBNavigator/actions/runs/41915650980), run 127, HEAD `2f0e05f14908a19acb2f96594b198e6226a3cd0c`: **SUCCESS**. This was a documentation-only revalidation after the CI #126 coverage update; all 50 job steps passed, including build, smoke test, existing route runs, guard routes, comparisons, INSTEAD export validation and artifact upload.
- Artifact: `nlb-sdl-test-logs`, ID `13369413410`, 77,716,384 bytes.
- Inspection of CI #126's low-wolfiness trace confirms that page1.8 UUID `433426b4-2e3f-4461-ab7a-e87119808471` has **5 runtime portions in both engines**. The captured start state was portion 0/5; all six tracked variables matched at `wolfiness=0,hp=3,barExit=true,forestExit=true,wolvesTalk=false,guardDone=false`.
- Extended `wolfman-guard-low-wolfiness-escape.json` with two checkpoints: after 4 additional advances, verify the final page1.8 portion (index 4/5); after one more advance, verify page1.9 UUID `8c0da93f-8301-442b-a9fd-60d68d9610d0`, image `1.9.png` / `images/1.9.jpg`, first visible text and all six variables.
- The source export shows the automatic transition from page1.8 through its empty choice wrapper to page1.9. The new runtime checkpoints are **pending the next full CI run**; page1.9 is not yet counted as confirmed coverage.
- No NLB story source, INSTEAD export, engine code or `main` was changed. This is test-route and documentation work only.

Next: inspect both runtime traces for the new page1.8/page1.9 checkpoints and use the captured page1.9 paginator count to extend the route through page30 and the low-wolfiness hawthorn choice.


### 2026-11-05 — CI #128: page1.8 completion and page1.9 verified

- CI [#41928063316](https://github.com/Antokolos/NLBNavigator/actions/runs/41928063316), run 128, HEAD `13953100518e89774f27bda72db90edd80e5434b`: **SUCCESS, 52/52 steps**. Build, smoke-test, all existing routes, all guard routes in both engines, comparisons, INSTEAD export validation and artifact upload passed.
- Artifact: `nlb-sdl-test-logs`, ID `13373614763`, 78,205,900 bytes.
- The extended low-wolfiness route reports `ok=true`, `errors=[]` in INSTEAD and NLBNavigator; the comparison report also has `ok=true`, `errors=[]`. It contains **29 text checks** per runtime and 36 comparison checkpoints. NLB state-log validation: **150 records**, `ok`.
- Both engines completed page1.8 UUID `433426b4-2e3f-4461-ab7a-e87119808471` at portion **4/5**, with the final phrase “сзади уже никого не было”, then advanced to page1.9 UUID `8c0da93f-8301-442b-a9fd-60d68d9610d0`.
- Page1.9 has **3 runtime portions in both engines**. Its first visible text is “Дрожа, вы улеглись под кустом…”. At entry, both runtimes preserve `wolfiness=0,hp=3,barExit=true,forestExit=true,wolvesTalk=false,guardDone=false`. The NLB state-log and INSTEAD scene trace agree on UUID, image `1.9.png` / `images/1.9.jpg`, first portion and variables.
- Extended the low-wolfiness route again: it now checks page1.9 final portion (index 2/3), advances to page30 (`e0d11a50-50d2-4521-be11-10267f8aab92`), expects the only hawthorn choice, selects it and expects page33 (`bc738a80-9890-4416-bead-3bbc8f533412`) with `hp=2`. These four additional checkpoints are **pending CI #129**; their static expectations are not yet runtime-confirmed.
- The source indicates page33 should then transition to page35b (`6e8c1c0f-abd8-4e5c-83a5-7e3e003c7f46`), setting `guardDone=true` and calling `RIGHT_CHOICE`; page33 runtime pagination still needs observation before adding the exact advances.
- No NLB story source, INSTEAD export, engine code or `main` was changed. PR #32 remains Draft/open/unmerged.


### 2026-11-05 — CI #130: page30 hawthorn choice and page33 verified

- CI [#42001378961](https://github.com/Antokolos/NLBNavigator/actions/runs/42001378961), run 130, HEAD `50371498f4c0badc17d72d2196e365c254075159`: **SUCCESS, 51/51 steps**. Build, smoke-test, all earlier routes, all three guard routes in both runtimes, comparisons, INSTEAD export validation and artifact upload passed.
- Artifact `nlb-sdl-test-logs`, ID `13407082565`, 79,230,675 bytes.
- Low-wolfiness route reports: INSTEAD `ok=true`, `errors=[]`; NLBNavigator `ok=true`, `errors=[]`. Comparison report: `ok=true`, `errors=[]`, 40 route checkpoints and 32 text checks, all passed. NLBNavigator state-log validation: **163 records**, `ok`.
- The route completes page1.9 (3 portions in each runtime), reaches page30 UUID `e0d11a50-50d2-4521-be11-10267f8aab92` and sees the only available choice, «Выбрать боярышник».
- After correcting the INSTEAD click coordinate from `y_ratio=0.36` to `0.27`, both engines select the hawthorn action and reach page33 UUID `bc738a80-9890-4416-bead-3bbc8f533412`, backgrounds `33.png` / `images/33.jpg`. Both show the first text portion beginning «Соорудив примитивное оружие». All six tracked variables match: `wolfiness=0,hp=2,barExit=true,forestExit=true,wolvesTalk=false,guardDone=false`. This runtime-confirms the `hp -= 1` effect of choosing the hawthorn stake.
- Page33 has **2 runtime portions in both engines**. Its second INSTEAD text portion includes «Оборотень кинулся на вас… налетел грудью прямо на острую палку». The new route adds a check for that final portion, followed by the transition to page35b UUID `6e8c1c0f-abd8-4e5c-83a5-7e3e003c7f46`, background `35b.png` / `images/35b.jpg`, expected text «Путь был свободен», `guardDone=true` and achievement `RIGHT_CHOICE`. These two new checkpoints are **pending CI #131**.
- Static verification against the pinned INSTEAD export confirms that page33's choices wrapper automatically walks to page35b when `guardDone=false`; page35b's entry calls `RIGHT_CHOICE` and sets `guardDone=true`. The next CI must verify that behavior at runtime in both implementations and inspect the INSTEAD session log for the achievement call.
- Cross-renderer pixel metrics remain advisory; semantic text, page/resource identity and tracked variables are the comparison gate. No story source, INSTEAD export, engine code or `main` was changed. PR #32 remains Draft/open/unmerged.
 
Next: inspect the two new page33/page35b checkpoints in CI #131. After confirming page35b, continue into the villain module and test the actual background selected by page36's tag before covering its six endings.
