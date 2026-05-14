## Test environments

* local R installation, R 4.5.1
* R-hub (platforms; pending)
* win-builder (R-release and R-devel; pending)

## R CMD check results

Local `R CMD check --no-manual --no-build-vignettes --ignore-vignettes`
results for cgmguru 1.0.0:

0 errors | 0 warnings | 0 notes

## Package update

This release updates cgmguru to version 1.0.0.

Changes in this release include:

* Added iglu-compatible event-grid interpolation to the event detection
  pipeline.
* Added the standalone `interpolate_cgm()` helper.
* Added `sensor_wear()` and included observed-data sensor wear in
  `detect_all_events()` summary output.
* Updated `detect_all_events()` summary metrics to use the interpolated event
  grid after gap masking.
* Added iglu parity and interpolation-focused tests.
