# NLBNavigator

NLBNavigator is a visual-novel reader/player. The repository also contains a differential test suite for comparing the NLBNavigator runtime with the INSTEAD reference implementation.

## Differential testing

Распределение ролей и ответственности участников приведено в [таблице ролей](tests/differential/TESTING_PROGRESS.md#участники-и-роли).


- [Test suite overview and execution instructions](tests/differential/README.md)
- [Persistent testing progress and CI handoff](tests/differential/TESTING_PROGRESS.md)
- [Wolfman branch and ending map](tests/differential/WOLFMAN_BRANCH_MAP.md)

PR #44 hospital routes are confirmed by CI #171 on HEAD `c94eeabc0f569c15f3e3c5ac6653c46a40140ce9`: all 16 permanent catalog routes passed in both engines, with 16 successful comparisons. Hospital pagination, runtime state and local paired runs by the Reviewer are documented in `tests/differential/TESTING_PROGRESS.md`.