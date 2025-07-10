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

## 📚 Documentation

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
