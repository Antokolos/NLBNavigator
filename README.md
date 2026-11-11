# NLBNavigator

NLBNavigator is a visual-novel reader/player. The repository also contains a differential test suite for comparing the NLBNavigator runtime with the INSTEAD reference implementation.

## Differential testing

- [Test suite overview and execution instructions](tests/differential/README.md)
- [Persistent testing progress and CI handoff](tests/differential/TESTING_PROGRESS.md)
- [Wolfman branch and ending map](tests/differential/WOLFMAN_BRANCH_MAP.md)

Latest verified run on `main`: [CI #152](https://github.com/Antokolos/NLBNavigator/actions/runs/42822208217) — SUCCESS on `2e1441a846ccb771ba43cba3bb217a395c960876`. All 15 route jobs passed, including paired runtime execution and comparisons for both robber branches. Artifact: `nlb-sdl-test-logs` (ID `13770101592`). CI #151 was the successful PR #46 run. See tests/differential/TESTING_PROGRESS.md for the current handoff and remaining coverage.
