## Test environments

* local R installation, R 4.5.1
* GitHub Actions R-CMD-check on main (macOS release, Windows release,
  Ubuntu devel/release/oldrel): passing
* R-hub (platforms; pending)
* win-builder (R-release and R-devel; pending)

## R CMD check results

Local `R CMD check --no-manual --no-build-vignettes --ignore-vignettes`
results for cgmguru 1.1.1:

0 errors | 0 warnings | 0 notes

## CRAN check status

The current CRAN checks for cgmguru 1.1.0 show an ERROR in the test suite on
r-oldrel macOS x86_64. This submission addresses that failure.

The failing tests compared `sensor_wear()` fixed-window results directly
against `iglu::active_percent()` manual-window `start_date` values. Those
manual windows use calendar-day arithmetic and can shift by one hour across
DST-sensitive timezones. The tests now compare the fixed-window
observed/expected reading counts directly, which is the calculation performed
by `sensor_wear()`.

## Package update

This release updates cgmguru to version 1.1.1.

Changes in this release include:

* Stabilized `sensor_wear()` tests across DST-sensitive timezones by comparing
  fixed-window observed/expected reading counts instead of timezone-dependent
  one-to-one `start_date` values from `iglu::active_percent()` manual windows.
* Expanded the package-level `cgmguru` vignette into a practical CGM analysis
  guide covering sensor wear, event summaries, event-grid inspection, GRID
  analysis, postprandial maxima workflows, excursions, visualization, and
  larger datasets.
