# Claude Code review: differential testing of INSTEAD and NLBNavigator

## Context

We are building a reproducible differential test suite for **the same visual-novel story** running in two engines:

- **INSTEAD reference:** `nlbproject/instead-am_i_a_monster_vn`
- **NLBNavigator package:** `nlbproject/wolfman`
- These are two packages of the same story (confirmed by the project maintainer).
- NLBNavigator repository: `Antokolos/NLBNavigator`
- Working branch: `ci/nlb-sdl-wolfman`
- Tracking issue: [#28](https://github.com/Antokolos/NLBNavigator/issues/28)
- Current CI workflow: `.github/workflows/nlb-sdl-test.yml`
- Route runner: `tests/differential/run_route.py`
- INSTEAD trace adapter: `tests/differential/instead_trace.lua`
- Current route: `tests/differential/routes/wolfman-opening.json`

The first end-to-end GitHub Actions run completed successfully on 2026-10-30:
- Run: https://github.com/Antokolos/NLBNavigator/actions/runs/41061737108
- Tested commit: `166c6890eb7ef5526a283863d8667527a42e2412`

That is only a **working baseline for the opening route**, not completion of the full comparison task.

## Current test architecture

A JSON route describes a sequence of actions (`capture`, `key`, `click`), waits, screenshots, and expectations. The same route is executed in both runtimes.

For NLBNavigator, the runner currently validates:
- screenshot existence, dimensions, non-trivial color count and visual change;
- page UUID and image suffix from `[screen]` diagnostic log records;
- expected text fragments in the playthrough log.

For INSTEAD, the injected Lua adapter wraps `iface.fmt`, calls the original formatter, returns its exact original return values, and writes a separate trace containing:
- command, current page UUID/name, background;
- rendered text and individual text components;
- `_wolfiness` and `_hp` values.

The adapter must not change the text displayed by INSTEAD. This was an important bug fix: earlier attempts to use `game._lastdisp` / `nlb:lasttext()` produced empty or unsuitable text. The current adapter preserves the original `rendered, extra` return values and keeps a separate last non-empty story rendering so an inventory refresh does not overwrite it.

The current opening route checks:
1. title screen;
2. Space advances to the first story page;
3. a click in the middle of the screen advances to the next text portion of the same page.

Both Space and click have been made to work in CI. The click uses absolute X11 screen coordinates in the INSTEAD automation.

## Questions for you

Please review the current implementation and advise us as the person most familiar with NLBNavigator and its INSTEAD compatibility.

### 1. Correctness of the comparison model

1. Is executing one shared action route against both packages the right foundation, or are there important differences in how INSTEAD and NLBNavigator interpret input/state that require separate adapters or route semantics?
2. Which internal concepts are the most reliable cross-engine equivalence points: page UUID, current location, image/background, paginator position, displayed text, inventory, variables, choices, or something else?
3. Are there known cases where the same story state legitimately has different page IDs, text fragments, background names, or transition timing between the INSTEAD export and the NLBNavigator package?
4. What should be treated as a real behavioral regression versus a harmless implementation/rendering difference?

### 2. INSTEAD instrumentation

Please inspect `tests/differential/instead_trace.lua` in particular.

1. Is wrapping `iface.fmt` the right/safest point to observe what the player actually sees? Are there other hooks (paginator, display, room transition, input dispatch, game loop) that would provide more authoritative information?
2. Is the current method of discovering a UUID by scanning globals for the location object reliable? Is there a canonical INSTEAD/STEAD2 API or field for the current room/page identity?
3. Can we reliably record the current paginator portion/index and whether more text remains on the current page?
4. What is the safest way to capture current background/image, choices/objects, inventory, and story variables without changing game behavior?
5. Could wrapping `iface.fmt` miss text that is rendered through another path, or record intermediate/duplicate output? How should we handle that?
6. Are there lifecycle or module-loading details that could make the injected adapter run too early or too late?

### 3. NLBNavigator instrumentation

Please identify the best stable diagnostic/test interface in NLBNavigator.

1. Are the current `[screen]` log records and playthrough text logs sufficient, or should we add a structured machine-readable state snapshot?
2. Where would you instrument the engine to expose current page UUID, image/background, paginator position, current text, choice list, inventory and relevant game variables?
3. Can the test runner observe state without coupling tests to internal implementation details or affecting timing?
4. Which current NLBNavigator APIs/classes/modules should the test author study first?

### 4. Deterministic input and route design

1. What is the most reliable way to advance text in both engines: Space, mouse click, or a dedicated test-only command? Are there clickable hotspots or modal states that make center-screen clicks unsafe?
2. How should routes represent choices, links, inventory actions, menu navigation, and other interactive story commands?
3. How can we ensure each test starts from a known clean state (fresh process, save reset, deterministic random seed, clock control, disabled autosave, etc.)?
4. Should one route exercise both Space and click in the same session, and how should we verify that the two inputs have equivalent effects?
5. What is the best strategy for branch coverage without creating brittle, excessively long scripts?

### 5. What to compare

Please recommend a practical comparison policy for:

- page identity and transitions;
- visible text and text pagination;
- background/foreground images and image scaling;
- choices, hotspots and interactive objects;
- inventory and story variables (in this story, for example, `_wolfiness` and `_hp`);
- audio/sound events, if feasible;
- visual output, including fonts, antialiasing, text panel gradients and layout.

We already noticed a possible visual difference: NLBNavigator appears to render the text background as a solid fill while INSTEAD uses a gradient. The INSTEAD theme has `fl` and `fr` images that may be relevant. Please explain how this theme behavior is intended to work and where NLBNavigator should reproduce it.

Should screenshot comparison use exact pixels, image masks/regions, perceptual metrics, or visual checks only after semantic state has been compared? How should expected differences be documented?

### 6. CI and test-suite architecture

1. Is the current GitHub Actions workflow structured appropriately for reproducibility?
2. Which versions/commits/environment details should be pinned and recorded?
3. What artifacts should every run retain (screenshots, trace, logs, state snapshots, diff images, route metadata)?
4. How should failures be classified so that an input/automation failure is not mistaken for a game-engine mismatch?
5. Would you recommend a small set of focused routes or one long playthrough? How should route fixtures and expected results be organized?

### 7. Suggested next steps

Please give us a prioritized plan:
- the most valuable instrumentation improvements;
- the next 3–5 test scenarios/routes to implement;
- any correctness issues you see in the current runner or adapter;
- tests that should be added before we expand coverage.

Where possible, point to exact source files, functions, classes, or INSTEAD APIs and explain why. Concrete code-level suggestions or pseudocode are especially welcome.

## Important constraints

- The goal is to find and report **all meaningful discrepancies** between the two engines for this story, not merely to make CI green.
- Preserve normal game behavior: instrumentation must be observational and must not alter rendering, input handling, timing, or state.
- Avoid pixel-perfect requirements where differences are expected due to font rasterization or platform rendering; nevertheless, meaningful layout/theme regressions must be detected.
- Please distinguish facts confirmed from source code from hypotheses that need an experiment.
- If you need more context, tell us exactly which files, logs, screenshots, or runtime experiments would help.

Thank you!
