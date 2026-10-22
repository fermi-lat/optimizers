# optimizers: minimizer details

This page explains how each `Optimizer` subclass interprets `tol`/`tolType`,
what its return code means, and how it computes uncertainties and
covariance. These differences are the main reason fits give different
results when only the `optimizer` choice changes. For the overview, see
[README.md](README.md).

All findings are from reading `src/*.cxx` at the commit in the README front
matter. Statements about Fortran internals come from the routine
documentation (MINUIT manual, PORT and L-BFGS-B headers in the `.f`
sources) and were not checked at runtime.

## Common protocol

Every wrapper:
1. Reads the free parameters (`getFreeParams`), passing their scaled
   values and bounds to the engine.
2. Evaluates `f = -stat.value()` and `g = -stat.getFreeDerivs()` at
   each point the engine asks for, after `stat.setFreeParamValues(x)`.
3. Writes the final `x` back with `setFreeParamValues`, and returns
   `getRetCode()`.

`find_min` = minimize and compute what is needed for errors.
`find_min_only` = minimize only. Which extra work is skipped:

| Class | `find_min` | `find_min_only` |
|-------|-----------|-----------------|
| `Minuit` | `MINIMIZE` then `HESSE` | `MINIMIZE` only |
| `NewMinuit` | `MnMinimize` then `MnHesse` | `MnMinimize` only |
| `ModNewton`, `Drmngb`, `Drmnfb` | Minimize, then invert the engine's own Cholesky factor into `m_uncertainty` | `setNeedCovariance(false)` then `find_min`. **The flag stays false afterwards** for that object. |
| `Lbfgs`, `Powell`, `OptPP` | same as `find_min_only` | — |

`tolType` is `optimizers::ABSOLUTE` (default) or `RELATIVE`. `Likelihood`
exposes these as `toltype=ABS|REL`, with `ftol` default 1e-3.

## Tolerances

| Class | ABSOLUTE `tol` means | RELATIVE `tol` means |
|-------|----------------------|----------------------|
| `Minuit` | MINUIT tolerance = `2000·tol`. MINUIT's MIGRAD stops at EDM < ~1e-3·tolerance·UP-scale, so the EDM target is of order `tol`. | `2000·tol·|value|` at the starting point. |
| `NewMinuit` | `m_tolerance = 1000·tol/Up = 2000·tol`. Minuit2 then uses `effective = m_tolerance·Up` and an EDM target of `0.002·effective` (`ModularFunctionMinimizer.cxx`, `VariableMetricBuilder.cxx`). With Up = 0.5 the **EDM target is 2·tol**. | The same, multiplied by `|value|` at the start. |
| `ModNewton`/`Drmngb`/`Drmnfb` | PORT's relative-function tolerance `V(RFCTOL)` is set to 3e-16 (effectively off). The wrapper instead stops when two successive function values differ by < `tol` after a successful step (`iv[28] == 4`). | `V(RFCTOL) = tol`. PORT's own relative convergence test applies. |
| `Lbfgs` | L-BFGS-B `factr` = machine ε, so its relative test is ~ε², effectively off. `pgtol` defaults to 0. The wrapper stops at a `NEW_X` step when `|Δf| < tol`. | `factr = ε·tol`. L-BFGS-B stops when the relative reduction is ≤ `factr·ε`, which is ε²·tol. That is **far stricter than intended**, so REL with Lbfgs usually runs until another criterion stops it (from reading the code; not checked at runtime). |
| `Powell` | `|Δf| ≤ tol` per full sweep through the directions. | `|Δf| ≤ tol·½(|f_old|+|f_new|)`. |

`m_maxEval` (`setMaxEval`) is passed as the call limit to MINUIT
(`MINIMIZE maxcalls`), Minuit2 (`maxfcn`) and PORT (`IV(MXFCAL)`). For
L-BFGS-B it is compared with `isave[33]`, the iteration count. For Powell
it is the iteration limit.

## Return codes

`0` always means converged and OK. The other codes depend on the
optimizer:

| Class | Codes |
|-------|-------|
| `Minuit` | `0`: `MINIMIZE` returned 0 and MNSTAT `ISTAT == 3` (full accurate covariance). `1`: `MINIMIZE` error flag 4 (abnormal termination, usually the call limit). `100 + ISTAT`: converged command but the covariance is not accurate (`ISTAT` 0 = not calculated, 1 = diagonal approximation, 2 = forced positive-definite). `110`: any other command error. `getQuality()` returns `ISTAT`. `getDistance()` returns EDM. |
| `NewMinuit` | `0`: all checks good. `1`: call limit reached. Otherwise `100 + bitmask` of failed conditions (see below). `getQuality()` returns Minuit2's `CovarianceStatus()` (0–3, with −1 mapped to 0), which is on the same scale as MINUIT's ISTAT. `getDistance()` returns EDM. |
| `ModNewton` (factory `DRMNGB`/`DRMNFB`) | `0`: PORT `IV(1)` ∈ 3–6 (x-, relative-, both-, or absolute-function convergence) or the wrapper's ABSOLUTE test. `1`: `IV(1) == 9` (function-evaluation limit). `100 + IV(1)` otherwise. The PORT codes are listed in the `ModNewtonReturnCodes` enum: 7 = singular convergence, 8 = false convergence, 10 = iteration limit, 63+ = errors. **No exception is thrown.** |
| `Drmngb`/`Drmnfb` (direct use only) | The raw PORT `IV(1)`, so **3–6 mean success**, not 0. The wrapper's ABSOLUTE test returns 6. `IV(1) > 6` **throws** `Exception("DRMNGB error", code)`. |
| `Lbfgs` | `LBFGS_NORMAL` = 0 (`CONV…` or the wrapper's absolute test). `LBFGS_TOOMANY` = 3. `ABNO`/`ERROR`/unknown task strings **throw** `Exception` with codes 1, 2 or 4. `getErrorString()` returns the L-BFGS-B task text. |
| `Powell` | `0` converged. `1` iteration limit. `brent` throws after 100 iterations. |
| `OptPP` | Always `0`. It is a stub. |

### NewMinuit status bitmask

`NewMinuit::checkResults` builds a 9-bit value with one bit for each
*good* condition, then returns `100 + (~good & 0x1FF)`. Each bit set in
`code − 100` is a **failed** condition:

| Bit (value) | Failed condition |
|-------------|------------------|
| 8 (256) | `!IsValid()` |
| 7 (128) | `!HasValidParameters()` |
| 6 (64) | `!HasValidCovariance()` |
| 5 (32) | `!HasAccurateCovar()` |
| 4 (16) | `!HasPosDefCovar()` |
| 3 (8) | `HasMadePosDefCovar()` |
| 2 (4) | `HesseFailed()` |
| 1 (2) | `!HasCovariance()` |
| 0 (1) | `IsAboveMaxEdm()` |

Caveats:
- The code is computed in `find_min_only`, **before** `find_min` runs
  HESSE. It is not recomputed after HESSE, so after `find_min` the return
  code describes the MIGRAD covariance, not the HESSE one. `getQuality()`
  reads the current state, so it reflects HESSE.
- If `!min.IsValid()`, `find_min_only` sets the code and then **throws**
  `Exception("Minuit abnormal termination. No convergence?")`, so callers
  see the exception and not the code (pyLikelihood users see a Python
  exception). `hesse()` also throws if the covariance is still invalid
  afterwards.
- Richer diagnostics are available on the object: `userState()`,
  `userCovarianceStatus()`, `userEdm()`, `userNFcn()`, `minuitIsAccurate()`,
  `minuitHesseFailed()`, `userGlobalCC()`, `userHessian()`,
  `minuitInvHessian()` and others.

## Uncertainties and covariance

| Class | `getUncertainty(useBase=false)` | `getUncertainty(true)` | `covarianceMatrix()` |
|-------|----------------------------------|------------------------|----------------------|
| `Minuit` | MINUIT parabolic errors (`mnerrs_` `EPARAB`), saved at the end of `minimize`. They come from HESSE after `find_min`, or from the MIGRAD approximation after `find_min_only`. | Base Hessian | `mnemat_` (MINUIT's internal error matrix for the variable parameters) |
| `NewMinuit` | `sqrt(diag(UserCovariance))`. Runs HESSE first if the covariance is not valid. | Base Hessian | `UserCovariance()`, running HESSE if needed |
| `ModNewton`, `Drmngb`, `Drmnfb` | **Always the base Hessian.** The PORT-derived `m_uncertainty` computed in `find_min` is overwritten. | Base Hessian | base: throws `std::runtime_error` |
| `Lbfgs`, `Powell` | Base Hessian | Base Hessian | throws |

**Base Hessian** (`Optimizer::getUncertainty`, `computeHessian`):
- Forward differences of the **analytic gradient**, with step `1e-7 ×
  value` (or 1e-7 if the value is 0). The step is flipped to the other
  side if it would leave the bounds.
- The result is Cholesky-decomposed (Numerical Recipes `choldc`). This
  throws `Exception("…Imaginary diagonal element…")` if the matrix is not
  positive-definite (for example, at a parameter pinned on a bound or a
  degenerate model).
- It is then inverted with the f2c LAPACK `dpptri_`, and the errors are
  √diag.
- The full covariance is computed but **not kept**. Only the diagonal is
  returned.

All errors are in scaled parameter units, and only for free parameters,
in free-parameter order. For `Likelihood`/pyLikelihood fits, the caller
(not optimizers) stores them with `Parameter::setError`.

Errors correspond to Δ(−log L) = 0.5 only because MINUIT and Minuit2 are
given `UP`/`ErrorDef` = 0.5. The base Hessian assumes the statistic is a
log-likelihood: σ² = diag(H⁻¹) with H = −∂²(value).

## MINOS and contours

Only `Minuit` and `NewMinuit` implement these. For the others the base
class throws.

- `Minos(n, level=1, numericDeriv=false)` returns `(lower, upper)`, with
  `lower` negative. `n` is the 0-based index among the free parameters.
  `level` is in units of Δχ² = 2Δ(log L): `ErrorDef`/`UP` is set to
  `level/2` for the call and then back to 0.5.
- **`Minuit::Minos`** runs `MINOS maxcalls n+1`. If either error is
  exactly 0 while the parameter is not at a bound, it is treated as a
  failure and **retried once with `SET NOGRAD`** (numerical derivatives).
  Gradient use is then restored. Failures do not throw.
- **`NewMinuit::Minos`** uses `StMnMinos`, a local copy of
  `Minuit2::MnMinos`/`MnFunctionCross` changed to respect user bounds and
  bound-induced scaling. It **throws** if either side is not valid.
  Because it throws before restoring `ErrorDef`, a failed `Minos` with
  `level != 1` leaves the `myFCN`/`FunctionMinimum` `ErrorDef` at
  `level/2` for later calls on the same object (from reading the code;
  not checked at runtime).
- `NewMinuit::minos_lower_error` / `minos_upper_error(n, level, tol)` call
  `StMnMinos::Lower_valid`/`Upper_valid` for one side at a time with the
  given tolerance. Only this package's test calls them. pyLikelihood's
  `minosError` uses `Minos(n, level)`.
- `MnContour(par1, par2, level, npts)` only **prints** the contour to
  stdout (`MNC` for Minuit, `MnContours` for NewMinuit). It returns
  nothing usable.
- `Minuit::doCmd(string)` passes any MINUIT command through. `SET
  STRATEGY` is set this way by `setStrategy`. `NewMinuit::setStrategy`
  replaces its `MnStrategy`. The default for both is 1.

## Choosing an optimizer (practical notes)

- **`NEWMINUIT`** is the most robust choice for large models: no
  parameter limit, rich status, MINOS that respects bounds. It throws on
  an invalid minimum instead of returning a code.
- **`MINUIT`** (the `gtlike` default) is limited to 50 free parameters and
  uses global state. MINOS falls back to numeric derivatives.
- **`DRMNGB`** (the default `optimizer` in pyLikelihood's
  `BinnedAnalysis`/`UnbinnedAnalysis` constructors) and **`DRMNFB`**
  go through `ModNewton`. They are fast and bound-respecting, but give
  only base-Hessian errors, no covariance matrix and no MINOS. Users often
  fit with DRMNGB first and refine with NEWMINUIT; this is common
  practice, not something the code enforces.
- **`LBFGS`** and **`POWELL`** are rarely used. Powell does not respect
  bounds, and an `OutOfBounds` exception ends the fit.
