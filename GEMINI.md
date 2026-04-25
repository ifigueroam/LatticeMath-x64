# Project Architecture & Rules

- **Universal Documentation Mandate:** After every successful task (functionality, bug fix, improvement, 
or change), you MUST automatically update the following four files as the final step of the session:
    1.  **README.md (User Level):** Add new features, implementation or specifications, and usage 
instructions. Explain the project and its most important features. **Standardization Rule:** 
BEFORE any commit or push to GitHub, APPEND new detailed phases and milestones by synchronizing with 
`Docs/TRACKLOG.md` and `Docs/DEVLOG.md`. Each new entry MUST replicate the existing structural criteria 
(Objective, Architectural Transition, Mechanism, Impact) to maintain a consistent documentation 
reference standard.
    2.  **DEVLOG.md (Technical Level):** APPEND new development milestones tagged by date to 
`Docs/DEVLOG.md`, following a strictly standardized structure in descending chronological order. 
Each entry MUST replicate existing criteria:
        a. **ANALYSIS AND DISCOVERY:** Exhaustive identification of the problem, root cause, 
technical constraints, system impact, proposed solution, and mathematical/hardware mechanism.
        b. **TECHNICAL SOLUTION:** Comprehensive description of the goal, phase relation (mapping 
to Docs/TRACKLOG.md), fix implemented, verbose reasoning, implementation details (including 
representative code snippets), and empirical results or test execution logs.
        c. **SCIENTIFIC REFERENCES:** ALL research sources and troubleshooting guides MUST be cited 
in formal APA format at the end of each entry or in a centralized section.
    3.  **TRACKLOG.md (Historical Level):** APPEND new milestones to `Docs/TRACKLOG.md` following a 
strictly standardized structural consistency. For each milestone:
        a. APPEND the transition element into the **Visual Evolution Overview** ASCII chart.
        b. APPEND a new entry to the **Detailed Phase & Milestone Timeline** replicating ALL 
existing criteria: **Milestone** (verbose description), **Date**, **Objective**, **Core Upgrades**, 
**Performance Impact**, **Architectural Scope**, **Architectural Transition** (Verbose Before vs. 
After explanation), **Scientific Design Rationale**, **Verification & Scientific Audit**, 
**Roadmap Integration**, **Findings** (Arithmetic, Algorithmic, Hardware), and **Analysis & Fix**.
    4.  **RESEARCH.md (Scientific Level):** For every research task, study, or analysis, APPEND the 
FULL verbose output of the thinking process (Thought blocks), full mathematical analysis, guided 
steps, and implementation examples to `Research/RESEARCH.md` in descending chronological order 
(most recent first). **All entries must be written strictly in the third person using the 
passive voice. No references to "the assistant", "the model", or "the system" are permitted.**
    5.  **APPLYRESEARCH.md (Implementation Level):** For every code modification or improvement 
based on a previous study, APPEND the detailed implementation summary, mapping of study to code, 
and applied optimization logic to `Research/APPLYRESEARCH.md`. This ensures a permanent audit 
trail between scientific research and its practical realization in the codebase.
    6.  **Chronological Synchronization Rule:** Before writing any changes to the documentation 
        files (`README.md`, `Docs/DEVLOG.md`, `Docs/TRACKLOG.md`, `Research/RESEARCH.md`, 
        `Research/APPLYRESEARCH.md`), you MUST verify and set the current date according to the 
        session context. Each new entry MUST use the exact date of the current conversation or 
        request to maintain a truthful and reliable historical trace.
- **Permission:** Do not ask for permission to perform these updates; execute them autonomously.
- **Line Wrapping:** ALL files (code, headers, markdown, and config) MUST adhere to a strict 
**105-column limit**. Always use `clang-format` for C/H files and appropriate wrapping for 
text/markdown to maintain readability in side-by-side views.

---

- **Pre-Commit Structural Validation:** BEFORE executing any commit and push to GitHub, the 
`README.md` MUST be updated to include all new detailed phases and milestones synchronized from 
`Docs/TRACKLOG.md` and `Docs/DEVLOG.md`. Each new entry MUST replicate the following structural criteria 
to maintain consistency: **Objective**, **Architectural Transition**, **Mechanism**, **Rationale**, 
and **Impact**. This ensures `README.md` serves as a solid and standardized documentation 
reference.
