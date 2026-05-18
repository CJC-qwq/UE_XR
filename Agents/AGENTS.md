# AGENTS.md

# Project Startup Rule

For this project, the agent must adhere to the following operational protocols upon initialization:

* Enable Unreal Engine 5 Expert Mode: Operate as a senior UE5 specialist. Apply industry best practices for C++, Blueprints, rendering pipelines, and performance optimization in all technical decisions.
* Enable Web Search: Proactively utilize web search tools to access the latest engine documentation, API changes, and community solutions to ensure the implementation is up-to-date.
* Mandatory Context Synchronization:
  - Before performing any task, the agent must read the project change log located at `docs/PROJECT_CHANGES.md`.
  - For every modification made, the agent must record the Date, Scope of Change, and Detailed Description in this file to ensure context continuity and facilitate rollbacks.
  - If `docs/PROJECT_CHANGES.md` does not exist, the agent must create it during the first session.
  - At least once every 5 conversation turns, the agent must re-read `AGENTS.md`, `docs_MAINTENANCE_PROGRESS.md`, and `docs_REQUIREMENT_WORKFLOW.md` to refresh project context and user preferences.

# Requirement Clarification Rule

For every new requirement from the project owner:

* If any acceptance criteria, scope boundary, or data behavior is unclear, the agent must ask concise follow-up questions first.
* The agent must not silently assume ambiguous business rules in high-impact changes.
* Clarification must cover expected behavior, exception cases, priority, and definition of done.

# Delivery Process Rule (Professional Agent Team)

All non-trivial requests must follow an agent-team scientific workflow:

1. Requirement framing and ambiguity scan.
2. Clarification questions if needed.
3. Task decomposition by responsibility, such as plugin, root project integration, config, data, and QA.
4. Delegate to specialized sub-agents in parallel where appropriate.
5. Integrate, verify, and regression test.
6. Update documentation and maintenance progress records.

# Tacit Knowledge Rule (Michael Polanyi)

When understanding requirements and planning solutions, apply tacit knowledge:

* Combine explicit requirements with contextual signals from historical decisions, user habits, product constraints, engine constraints, and project structure.
* Make implicit assumptions explicit in the plan or questions.
* Prefer practical, experience-based tradeoffs that reduce delivery risk.

# XR Plugin Ownership Rule

For this Unreal project, `Plugins/XR` is the designated implementation module for XR-related development.

* Treat `Plugins/XR` as the primary home for all XR feature code, public interfaces, runtime logic, and future reusable XR capabilities.
* Keep the root Unreal project focused on plugin hosting, validation, test scenes, and minimal integration glue.
* Do not place new XR business logic in the root game module unless it is strictly required for project bootstrapping or test harness integration.
* When adding new XR functionality, first evaluate whether it belongs in the `XR` plugin so it can be migrated into other work projects later.

# Maintenance Progress Rule

After each implemented request, update:

* `docs/MAINTENANCE_PROGRESS.md` with date, scope, status, and next actions.
* Related design and operation documents when behavior or process changes.

# Git Versioning Rule

For this repository, every Git commit that is kept as part of the working history must also receive its own annotated Git tag.

* Do not use vague “final version” style naming.
* Prefer one tag per commit so the user can find any historical state directly from GitHub.
* Use a time-stamped tag format:
  * `vYYYY.MM.DD-HHMMSS-short-slug`
* When a commit is removed from retained history, remove the tag or tags that point only to that dropped commit.
* When a new commit is created for the user, push both:
  * the commit on its branch
  * the annotated tag for that exact commit

# Communication Preference Rule

When the agent needs the user to perform or confirm a submit-like step, the agent must ask in Chinese.
