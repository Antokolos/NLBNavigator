# NLBNavigator

NLBNavigator is a visual-novel reader/player. The repository also contains a differential test suite for comparing the NLBNavigator runtime with the INSTEAD reference implementation.

## Differential testing

- [Test suite overview and execution instructions](tests/differential/README.md)
- [Persistent testing progress and CI handoff](tests/differential/TESTING_PROGRESS.md)
- [Wolfman branch and ending map](tests/differential/WOLFMAN_BRANCH_MAP.md)

Latest verified run: [CI #147](https://github.com/Antokolos/NLBNavigator/actions/runs/42537254808) — SUCCESS on `88bbcbcb`. It passed all 53 steps and produced 26 successful runtime reports (13 scenarios × 2 engines), 13 successful cross-runtime comparisons, 592 runtime text checks and 768 captured checkpoints. The branch has since integrated the latest `main` at `620f2ee9`; CI #148 is required to verify that newer Source tree. See tests/differential/TESTING_PROGRESS.md for the current handoff and remaining coverage.
