# Agents & Copilot Instructions for zarfld/OpenAvnu

This file defines **working principles** for humans and AI assistants contributing to this repository.  
It ensures the codebase remains **correct, lean, and alive**: no dead code, no broken builds, no guesswork.

`TODO.md` in the root is the **living backlog**.  
This file (`agents.md`) is the **permanent contract** for contribution style and repository health.

---

## 0) Purpose
- Maintain a **spec-accurate**, **well-documented**, and **clean** implementation of AVB/TSN stack components.  
- Prevent dead code and abandoned forks.  
- Ensure all contributions are **traceable to specs** (IEEE / Intel datasheets).  
- Keep the repository **organized**: docs cleanly structured, no clutter.

---

## 1) Non-Negotiables
- **Understand first**: read the architecture and patterns before coding. Summarize assumptions in the PR.
- **No fake production code**: no stubs, no simplified placeholders in productive paths. (Test harnesses may exist but must use real HAL patterns.)
- **Spec over assumption**: implement only from spec or documented analysis. If unclear → ask, don’t guess.
- **Truth in labeling**: no unproven features. Validate with asserts, checks, or test coverage.
- **Fix in place**: refactor instead of duplicating. **All code must compile**; remove or revive—never abandon.
- **Intel HAL = source of truth**: every register access cites the datasheet section/revision.
- **Guard I/O**: always use masks/range checks derived from specs.
- **Document functions**: Doxygen required for all public APIs and HAL functions.
- **One implementation rule**: avoid duplicate code. Centralize logic in HAL/lib.
- **No file suffix churn**: never commit `_new`, `_fixed`, `_copy` variants. Use incremental commits.

---

## 2) Repository Layout

```

/docs/                # Specs, ADRs, register maps, HW notes
/docs/completed/      # Validated, permanent docs
/docs/wip/            # Work in progress
/docs/status/         # Status overviews
/docs/archive/        # Historical references
/include/             # Public headers (registers, API defs)
/lib/                 # Shared libraries
/daemons/             # gPTP, AVDECC, other daemons
/apps/                # AVTP applications and demos
/scripts/             # Run/CI helper scripts
/tests/               # Test harnesses (real access patterns)
/ci/                  # CI/CD configs and helper scripts
TODO.md               # Living backlog (keep updated!)
agents.md             # This ruleset

````

- **Register definitions** live only in `/include/intel_regs.h`.  
- No magic numbers in code.

---

## 3) Branching & Commits

- **Trunk-based flow**: short feature branches, protected `main`.
- **Clean submit rules**:
  - Each commit compiles across supported targets and passes CI.
  - Small, single-purpose diffs; no WIP noise.
  - No dead/commented code. Remove unused files.
  - Run formatter + static analysis before commit.
  - Update docs/tests; reference spec section + issue ID in commit message.
  - Use feature flags or compat layers when incremental changes risk breakage.

- **Commit style**: Conventional Commits (`feat:`, `fix:`, `refactor:`, `docs:`, `test:`, `ci:`, …).

---

## 4) OpenAvnu-Specific Rules

### AVDECC (lib/daemons)
- All AECP/ACMP/ADP/state-machine changes must pass **32/32 protocol tests**.
- CI runs full AVDECC protocol suite on every PR.
- No divergence of state machines—evolution must remain spec-accurate.

### Intel HAL (lib/hal)
- Every function cites **Intel datasheet section/rev**.
- All reads/writes validated with masks/range checks.
- Wrapper functions (`mmio_read32`, `mmio_write32_masked`, …) are mandatory.
- No raw `*(volatile*)` accesses in driver logic.

### gPTP (submodule)
- Submodule reference must be consistent and buildable.
- CI ensures `run_gptp.sh` launches and AVTP apps can bind to it.
- Document changes in `/docs/completed/gPTP_Integration.md`.

---

## 5) No Dead Code – Enforcement

**Compiler/Linker flags**
- `-Wall -Wextra -Werror -Wunused -Wunreachable-code`
- `-ffunction-sections -fdata-sections -Wl,--gc-sections`

**Static analysis**
- `clang-tidy` + `cppcheck` run in CI.
- Fail CI on warnings.

**Repo hygiene**
- `/tools/scan_copies.sh` blocks suffix-files (`*_new`, `*_copy`, …).
- `/tools/spec_refs_check.py` ensures register writes cite spec sections.

**Coverage sanity**
- Coverage tracking highlights never-executed utilities → flagged for removal/refactor.

---

## 6) Documentation Standards

- **Doxygen required** for all exported functions and HW accessors.
- CI runs doxygen; warnings treated as errors.
- Keep `/docs/spec-index.md` updated (registers → datasheet sections/rev).

**Template:**
```c
/**
 * @brief  Programs RX descriptor ring base
 * @param  n   Adapter handle
 * @param  dma DMA address (aligned per spec)
 * @return 0 on success, -EINVAL on misalignment
 * @hwctx Requires device in reset-held state; see Intel I210 Spec §7.2
 */
int rx_ring_program(struct nic* n, uint64_t dma);
````

---

## 7) Testing & Validation

* Tests use real HAL APIs.
* Offline tests may use **buffer-backed fake HAL** under `/tests/hal_backing/`.

  > Production code must never link against fake HAL.
* CI includes:

  * AVDECC 32/32 tests
  * gPTP startup test
  * Static analysis
  * Doxygen build
* Smoke tests on hardware (Intel NICs) run before protected branch merges.

---

## 8) Reviews, Ownership, ADRs

* **CODEOWNERS** per module: at least 1 owner + 1 peer review required.
* **ADR** required for architecture changes → add to `/docs/adrs/`.
* Breaking changes: add migration steps + changelog entry.

---

## 9) PR Template (add to `.github/pull_request_template.md`)

```
### What & Why
- Summary:
- Issue(s):

### Spec references
- IEEE section(s):
- Intel datasheet section(s):

### Affected areas
- AVDECC / HAL / gPTP / Docs / Other:

### Validation
- Build & CI checks passed?
- AVDECC tests 32/32 passed?
- gPTP launch verified?

### Docs
- Doxygen updated?
- /docs/* updated (completed/wip/status)?

### Cleanliness
- Dead/commented code removed? ☐
- No *_new or *_copy files? ☐
```

---

## 10) CI Minimum Matrix

* **Builds**: GCC + Clang, Debug/Release, `-Werror`, `--gc-sections`
* **Static**: clang-tidy, cppcheck
* **Docs**: doxygen (fail on warning)
* **Format**: clang-format
* **Tests**: AVDECC suite, gPTP launch
* **Hygiene**: run `scan_copies.sh`, `spec_refs_check.py`

---

## 11) AI Assistant Guardrails

When suggesting code or docs, assistants must:

1. Reuse existing modules before proposing new ones.
2. Modify in place; propose small, compiling diffs.
3. Always include Doxygen + spec citation for HW functions.
4. Use `/include/intel_regs.h` constants only (no magic numbers).
5. Update docs/tests in the same PR.
6. Never propose `_new` or `_fixed` files.
7. Provide a build-check list with patch suggestions.

---

## 12) Deprecation & Removal

* Mark deprecated APIs with `DEPRECATED` macro + tracking issue.
* Remove deprecated code within one minor release window.
* Archive docs in `/docs/archive/` if obsolete.

---

### Quick Review Checklist

* [ ] Builds cleanly across matrix, no warnings.
* [ ] No dead/commented code or suffix-files.
* [ ] Spec references present for all HW touches.
* [ ] Doxygen present and accurate.
* [ ] Tests/docs updated.
* [ ] AVDECC tests 32/32 passed.
* [ ] gPTP launch verified.
* [ ] Small, focused diff with clear rationale.

---



