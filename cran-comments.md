## Test environments

* local R installation, R 4.4.2
* R-hub (platforms; pending)
* win-builder (R-release and R-devel; pending)

## R CMD check results

R CMD check results for cgmguru 0.2.0:

0 errors | 0 warnings | 2 notes

Local notes:

* The local macOS check reported `unable to verify current time` when checking
  future file timestamps.
* The local HTML manual validator reported HTML5 validation issues for
  R-generated manual pages (for example, `<main>` tags). The PDF manual builds
  successfully.

## Package update

This is an update to cgmguru 0.2.0.

Changes in this release include:

* Added preset Level 1, Level 2, and Extended event definitions for
  hypoglycemic and hyperglycemic event detection.
* Updated event boundary reporting so detailed event outputs end at the final
  dysglycemic reading immediately before confirmed recovery.
* Renamed public output index columns from plural to singular forms.
* Added tests for pre-recovery event boundaries.
