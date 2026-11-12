# NLBNavigator

NLBNavigator is a visual-novel reader/player. The repository also contains a differential test suite for comparing the NLBNavigator runtime with the INSTEAD reference implementation.

## Differential testing

- [Test suite overview and execution instructions](tests/differential/README.md)
- [Persistent testing progress and CI handoff](tests/differential/TESTING_PROGRESS.md)
- [Wolfman branch and ending map](tests/differential/WOLFMAN_BRANCH_MAP.md)

Hospital route update is being validated in PR #44 after merging `main` at `0d05fd69547cccd4dd8d589f53f8a14713aa73a1`. CI #162 supplied runtime evidence for page4/page14/page5 pagination and state; the next PR #44 CI run is expected to execute all 16 catalog routes. See `tests/differential/TESTING_PROGRESS.md` for the handoff and evidence.