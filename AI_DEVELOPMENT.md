# MyCam Pro — Autonomous AI Development

This repository is being prepared for an autonomous development workflow.

## Goal

The AI must produce and maintain a real working application, not mockups only.

The development loop is:

1. Requirements
2. Architecture
3. Implementation
4. Build
5. Launch in an isolated test environment
6. Functional QA
7. Visual QA
8. Bug fixing
9. Rebuild
10. Regression testing
11. Release candidate

## Agent roles

- Director: owns project state and task orchestration.
- Architect: owns technical design and interfaces.
- Developer: changes production code.
- QA: creates and runs functional tests.
- Vision/Computer Agent: launches the real application and inspects/operates its UI.
- Visual QA: compares application screenshots against approved design references.
- Build/Release: produces distributable artifacts.

## Human approval

Human approval is required for:
- architecture changes with material impact;
- destructive repository operations;
- production deployment;
- security-sensitive changes;
- release publication unless explicitly switched to full-auto mode.

Routine coding, testing, rebuilding and bug fixing should not require chat interaction.

## Definition of Done

A task is not complete because code was generated. It is complete only when:
- the project builds;
- automated tests pass;
- the real application launches;
- required user flows pass;
- visual checks pass within configured thresholds;
- artifacts are produced;
- the result is recorded in Git.
