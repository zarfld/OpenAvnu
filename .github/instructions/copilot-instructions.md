---
applyTo: '**'
---

# 🛠️ Repository Rules and Best Practices

These rules define coding standards, collaboration protocols, and repository hygiene for working in this project. They apply to all files and folders (`applyTo: '**'`).

---

## ✅ General Principles

* **Real-World Testing**
  Never mark a feature or fix as *"ready"*, *"done"*, or *"complete"* until it has passed all relevant tests, including **real hardware testing** if applicable.

* **Maintain `Todo.md`**

  * When a task is completed, **remove it from `Todo.md`**.
  * Log completed tasks in `docs/completed/`.
  * Newly discovered tasks or requirements must be added to `Todo.md`.

* **Manage Documentation Lifecycle**

  * **Consolidate** duplicate or redundant task descriptions.
  * **Move completed** task descriptions to `docs/completed/`.
  * **Archive outdated** or irrelevant content to `docs/archive/`.

## ✍️ How to Specify a Task (Copilot-Friendly)

- **Be specific** – Describe clearly *what* the function/component should do.
- **Add context** – Mention libraries, inputs, outputs, dependencies, edge cases.
- **Use docstrings or comments** – Define the intent and constraints inside the function.
- **Define "done"** – Include completion criteria:
  - ✅ Input/output examples or test cases
  - ✅ Error handling requirements
  - ✅ Side effects (e.g., file written, DB updated)
  - ✅ Preconditions and postconditions
  - ✅ Optional: Checklist or TODO block

### 📋 Task Template

**What**  
> _Short, clear description of what needs to be implemented or fixed._

**Context**  
> _Relevant inputs, outputs, modules, libraries, or dependencies involved._

**Done When**
- [ ] Correctly handles specified input/output cases
- [ ] All edge cases and errors are accounted for
- [ ] Code is tested (unit/integration as appropriate)
- [ ] Side effects (e.g., logs written, state updated) are verified
- [ ] Meets performance or timing constraints (if any)

**Information Resources**  
> _Where to find definitions, constants, limits, tolerances, and related documentation:_

- 📄 `docs/specs/<file>.md`
- 📘 [External standard or datasheet link]
- 📂 Source code: `src/module/submodule/xyz.c`
- 🧪 `tests/<area>` for example inputs and edge cases
- 🔧 `config/defaults.yaml` for default settings
- 🧠 `docs/insights/` for known quirks and lessons learned

**Optional Notes**
> _Any blockers, uncertainty, or open questions to resolve before implementation._

---

## 🌿 Branch and Commit Conventions

* **Branch Naming**
  Use clear, conventional prefixes:

  ```
  feature/*    for new features  
  bugfix/*     for bug fixes  
  hotfix/*     for critical patches  
  refactor/*   for code restructuring  
  doc/*        for documentation-only changes
  ```

* **Commit Message Format**
  Follow [Conventional Commits](https://www.conventionalcommits.org/):

  ```
  feat: Add login form validation  
  fix: Correct rounding error in tax calculator  
  chore: Update npm dependencies  
  docs: Rewrite usage section of README  
  refactor: Simplify caching logic
  ```

---

## 🔍 Code Quality and Reviews

* **Code Reviews**
  All non-trivial changes must undergo **peer review** before merging into the main branch.

* **Static Analysis and Linting**
  Code must pass **all linters and static analysis checks** before being committed or merged.

* **Mark Experimental Work**
  Clearly prefix experimental features or unstable prototypes with `EXPERIMENTAL:` in:

  * Commit messages
  * Documentation
  * Code comments, where applicable

---
## Testing
* **Test Coverage**
  All new features and bug fixes must include unit tests with at least 80% coverage.
* **Integration Tests**
  Ensure integration tests cover all critical workflows and interactions between components.
* **Test Documentation**
  Document test cases and expected outcomes in `docs/tests/`:
  * Include test setup instructions
  * Describe test scenarios and expected results
* **Test Results**
  Store test results in `docs/tests/results/`:
  * Include date and time of test runs
  * Summary of results
  * Any issues or failures encountered

  * **Test Environment**
  before running tests, ensure the environment is set up correctly:
  * Use `docs/machine/` to get Machine-specific information. (create if not exists)
  * respect machine capabilities and limitations
  * Document any specific configurations or dependencies required for testing in `docs/tests/setup.md`
* **Test Automation**
  Automate tests using CI/CD pipelines where possible. Ensure:
  * All tests run automatically on pull requests
  * Test results are reported back to the pull request  
* **Test Documentation**
  Document test cases and expected outcomes in `docs/tests/`: 
  * Include test setup instructions
  * Describe test scenarios and expected results
* **Test Results**
  Store test results in `docs/tests/results/`:
  * Include date and time of test runs
  * Summary of results
  * Any issues or failures encountered
* **mock and simulated data**
 For testing with real data, the use of mock or simulated data is not allowed!
  Use mock or simulated data for testing when real data is not available, only. Document:
  * Source of mock data
  * How it was generated
  * Any limitations or assumptions made
* **integration tests**
  Ensure integration tests cover all critical workflows and interactions between components.
  * integration test tools should be stored in `testing/integration/<module>_<phase>_<test_type>_<description>` (information about the test, e.g. `docs/tests/integration/avb_milan_phase2_production_testing_complete.md`)
 * when creating integration tests ensure that the prepared test is capable to test all supported environment variants (all supported OS, all supported hardware, etc.)
* should be added to the vscode tasks.json to run the tests automatically
* should be added to the CI/CD pipeline to run the tests automatically on pull requests



* **Module Testing**
  Each module should have its own test suite:
  * Include unit tests for individual functions or classes
  * Integration tests for module interactions
  * End-to-end tests for complete workflows
---
## 📚 Documentation

 * documentation should be written in english
 * Use `docs/` for all project documentation. 

* **Design Decision Logs**
  Use `docs/decisions/` to document key architecture or design decisions. Use the [ADR pattern](https://adr.github.io/) for consistency.

* **Technical Debt and Refactoring**

  * Mark areas needing cleanup in code using `TODO (refactor)`
  * Schedule regular refactoring sessions to resolve tech debt items from `Todo.md`

* **Completed Taskss**
  Use `docs/completed/` to log completed tasks, features, or fixes. Include:

  * Date of completion
  * Description of work done
  * Any relevant links or references
* **Archive Outdated Content**
  Move outdated or irrelevant documentation to `docs/archive/` to keep the main docs clean and focused.

* **Work In Progress (WIP) Documentation**
  Use `docs/work-in-progress/` for ongoing work that is not yet ready for review or merging. Include:

  * Clear status updates
  * Next steps or blockers
  * Links to related issues or discussions

* **Implementation Plans**
if we come up with implementation Plans we should init a new file in `docs/work-in-progress/` folder
  Use `docs/work-in-progress/` for detailed implementation plans. Include:

  * Background and context
  * Problem statement
  * Proposed solution
  * Risks and open questions
  * Next steps or milestones

  which should keep maintained while work is in progress.

* **Reports and Insights**
  Use `docs/reports/` for any testresults, reports, metrics, or performance analyses. Include:

  * Date of report
  * Summary of findings
  * Any relevant charts or data visualizations

* **Machine Documentation**
  Use `docs/machine/` for Machine-specific documentation. Include:
  * Machine configuration details
  * Hardware specifications
  * Software versions and dependencies
  * Known issues or limitations



---
## 🧠 Concept and Design Proposal Rules

When proposing a new **concept**, **feature**, or **architecture**:

- **Respect Existing Design Decisions**  
- Review `docs/decisions/` and `docs/completed/` before starting any concept.  
- Avoid contradicting past decisions unless you explicitly justify and document the changes.

- **Create Structured Concept Documents**  
- Use the `docs/concepts/` folder.  
- Naming convention:  
  ```
  docs/concepts/YYYY-MM-DD_short-title.md
  ```
- Each file should include the following sections:
  - **Background** – context and motivation  
  - **Problem Statement** – what needs solving  
  - **Prior Decisions** – list of related ADRs or completed items  
  - **Proposed Solution** – clear, focused proposal  
  - **Alternatives Considered**  
  - **Risks and Open Questions**  
  - **Next Steps** – implementation or review plan

- **Ensure Real Abstractions**  
Abstraction layers must:
- Be **generic and reusable**  
- **Hide implementation details** cleanly  
- Be **extensible** without breaking existing interfaces  
- **Avoid tight coupling** between layers or modules  
- Document **extension mechanisms** (e.g., callbacks, interfaces, hooks)

- **Discuss Before You Build**  
- Submit a concept as a pull request or internal review before beginning implementation.  
- Encourage feedback and iteration early in the process.
- Do not rely on assumptions. Always verify behavior, inputs, and results through tests, logs, documentation, or direct inspection. If something is unclear, investigate until you are certain.

⚠️ Avoid Assumptions – Verify Everything
Do not rely on assumptions. Always verify behavior, inputs, outputs, and side effects through testing, logs, or direct inspection. If something is unclear — investigate until it’s confirmed.

Document Lessons Learned
------------------------

To avoid repeating mistakes and to enable optimized workflows, all important discoveries and debugging insights must be documented.

Use the 'docs/insights/' Folder:
- Store each lesson learned or technical insight as a separate file in docs/insights/.

Purpose:
- Prevent recurring mistakes and inefficient trial-and-error
- Capture non-obvious behavior or system quirks
- Enable better decision-making and onboarding
- Serve as a long-term reference for debugging or optimizing

File Naming Convention:
- docs/insights/YYYY-MM-DD_short-title.md

Each Insight File Should Contain:
- Context        – What problem was being worked on
- Observation    – What was discovered or unexpected
- Root Cause     – (if identified)
- Resolution     – What fixed or improved the situation
- Takeaways      – Key points to remember or apply in the future

Guideline:
- If something took more than 15 minutes to figure out and might happen again — write it down.
