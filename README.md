# NLBNavigator

NLBNavigator is a visual-novel reader/player. The repository also contains a differential test suite for comparing the NLBNavigator runtime with the INSTEAD reference implementation.

## Differential testing

- [Test suite overview and execution instructions](tests/differential/README.md)
- [Persistent testing progress and CI handoff](tests/differential/TESTING_PROGRESS.md)
- [Wolfman branch and ending map](tests/differential/WOLFMAN_BRANCH_MAP.md)

Latest verified run before the main-branch integration: [CI #145](https://github.com/Antokolos/NLBNavigator/actions/runs/42520125450) — SUCCESS on 29d3d9cb. It passed 26 runtime route reports (13 scenarios × 2 engines) and 13 cross-runtime comparisons. The differential branch has since been updated with current main; a fresh integration run is required. See tests/differential/TESTING_PROGRESS.md for the current handoff and remaining coverage.
