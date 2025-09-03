applyTo: '**'
---

# 🛠️ Repository Rules and Best Practices (OpenAvnu fork)

This file defines **coding standards, collaboration protocols, and repository hygiene** for this project.  
It applies to all files and folders (`applyTo: '**'`).  

👉 `TODO.md` in the root is the **living backlog**.  
👉 `agents.md` (this file) is the **permanent ruleset** for contributions.

---

## ✅ General Principles

* **Real-World Testing**  
  Never mark a feature or fix as *ready* until it has passed **all relevant tests**, including hardware-level testing when applicable.

* **Build with CMake**  
  All builds must go through `cmake`.  
  Dependencies must be properly specified in `CMakeLists.txt`.  
  No broken or half-configured build files may be committed.

* **No Assumptions – Verify Everything**  
  Always prove behavior, inputs, outputs, and side effects with tests, logs, or direct inspection.  
  If something is unclear → investigate until confirmed.

* **Documentation is Mandatory**  
  - Every function, class, and module must include Doxygen comments.  
  - For HAL and AVDECC code, cite the **exact IEEE or Intel datasheet section/revision**.  
  - Keep explanations short, clear, and in English.

* **Version Control Discipline**  
  - Use Git branches (`feature/*`, `bugfix/*`, …).  
  - Follow [Conventional Commits](https://www.conventionalcommits.org/).  
  - Each commit must compile and pass CI checks.

* **Code Quality**  
  - Follow consistent style (naming, formatting).  
  - Run formatter + static analysis before committing.  
  - No dead code, no commented-out code.

* **Maintain `TODO.md`**  
  - Add new tasks here.  
  - Remove completed items, and log them under `docs/completed/`.  
  - Consolidate duplicates and clean regularly.

* **Documentation Lifecycle**  
  - `docs/completed/` → finished tasks and verified docs  
  - `docs/work-in-progress/` → ongoing work, implementation plans  
  - `docs/status/` → high-level progress and status reports  
  - `docs/archive/` → obsolete or historical content

---

## 🔒 OpenAvnu-Specific Rules

### AVDECC (lib/daemons)
- Any PR touching **AECP, ACMP, ADP, or state machines** must pass the **32/32 AVDECC protocol test suite** before merge.  
- No duplicate or alternative state machines — refactor in place.  
- State transitions must remain spec-accurate.

### Intel HAL (lib/hal)
- All register access must:
  - Use wrapper functions (`mmio_read32`, `mmio_write32_masked`, …).  
  - Validate writes with masks/ranges from specs.  
  - Include Doxygen comments with **datasheet section/revision**.  
- No raw `*(volatile*)` casts in production code.  
- All constants defined centrally in `/include/intel_regs.h`.  

### gPTP (submodule)
- gPTP must build automatically via submodule.  
- CI must launch `run_gptp.sh` and confirm AVTP apps bind correctly.  
- Any changes documented in `docs/completed/gPTP_Integration.md`.

---

## ✍️ How to Specify a Task

When adding to `TODO.md`, creating an issue, or drafting a PR, use this structure:

**What**  
> _Clear description of the feature or fix._

**Context**  
> _Inputs, outputs, dependencies, related docs/specs._

**Done When**
- [ ] Handles all specified I/O cases  
- [ ] Covers edge cases and errors  
- [ ] Has unit/integration tests  
- [ ] Passes hardware tests where relevant  
- [ ] Meets performance/timing constraints  

**Resources**  
- 📄 `docs/specs/...`  
- 📘 Intel datasheet / IEEE standard  
- 📂 Source code under `src/`, `lib/`, `daemons/`  
- 🧪 `tests/...`

---

## 🌿 Branch and Commit Conventions

**Branch names**
```

feature/\*    new features
bugfix/\*     bug fixes
hotfix/\*     urgent patches
refactor/\*   code restructuring
doc/\*        documentation-only

```

**Commits**
- Use Conventional Commits, e.g.:  
  - `feat: Add RX ring setup`  
  - `fix: Correct ACMP transition timing`  
  - `docs: Update gPTP integration notes`  
- Reference spec section + issue ID whenever relevant.

---

## 🔍 Code Quality and Reviews

* **Code Reviews**
  - Required for all non-trivial PRs.  
  - HAL/AVDECC PRs must include at least one reviewer familiar with the spec.

* **Static Analysis & Linting**
  - `clang-tidy` and `cppcheck` must run clean.  
  - Treat warnings as errors.

* **Experimental Work**
  - Prefix with `EXPERIMENTAL:` in commits and docs.  
  - Must be isolated from production paths.

---

## 🧪 Testing Rules

- **Coverage**  
  - New code must include unit tests; aim for ≥80% coverage where feasible.  

- **Integration Tests**  
  - Cover critical AVTP, AVDECC, and gPTP workflows.  
  - Integration tools live under `testing/integration/...`.  

- **Test Documentation**  
  - Store test definitions in `docs/tests/`.  
  - Results go in `docs/tests/results/` with date + summary.  

- **Machine Documentation**  
  - Store configs in `docs/machine/` (hardware, OS, dependencies).  

- **Automation**  
  - Add tests to CI and VSCode tasks.json.  
  - PRs must show results in CI.  

- **Mocks & Simulations**  
  - Allowed only in tests, not production.  
  - Document source, generation method, and limitations.

---

## 📚 Documentation Rules

- Use English.  
- Use `docs/decisions/` for ADRs.  
- Use `docs/concepts/YYYY-MM-DD_short-title.md` for new proposals.  
- Use `docs/insights/` to record debugging lessons:  
```

docs/insights/YYYY-MM-DD\_short-title.md

```
Each file must include: context, observation, root cause, resolution, takeaways.

---

## 🧠 Concepts and Design Proposals

- Review existing ADRs and completed docs before drafting.  
- New proposals must include background, problem, prior decisions, proposed solution, alternatives, risks, next steps.  
- Submit concepts as PRs for early discussion.  
- Abstractions must be reusable, generic, and extensible — not tightly coupled.  

---

## 🚫 Dead Code and Hygiene Enforcement

* **CI compiler/linker flags**  
- `-Wall -Wextra -Werror -Wunused -Wunreachable-code`  
- `-ffunction-sections -fdata-sections -Wl,--gc-sections`  

* **Repo hygiene scripts**  
- `/tools/scan_copies.sh`: fails CI if `*_new`, `*_copy`, `*_fixed` files exist.  
- `/tools/spec_refs_check.py`: ensures each register access cites a spec section.  

* **Doxygen**  
- Must build without warnings (CI fails otherwise).  

---

## ✅ Review Checklist

- [ ] Builds across all CI targets, no warnings  
- [ ] No dead or commented-out code  
- [ ] No `_new`, `_copy`, `_fixed` files  
- [ ] All HAL/driver code cites datasheet sections  
- [ ] Doxygen present and valid  
- [ ] Tests + docs updated  
- [ ] AVDECC 32/32 tests pass  
- [ ] gPTP launch verified  
- [ ] PR is small, focused, and justified

