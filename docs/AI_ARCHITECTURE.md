# Autonomous AI Development Architecture

## 1. Control plane

The control plane owns persistent project state, task queues, approvals, agent leases, test results and release state.

Recommended components:
- Python + FastAPI
- PostgreSQL
- Redis
- Git/GitHub
- Docker
- isolated Windows test VM for desktop UI testing

## 2. Agent loop

`Director -> task -> agent -> workspace -> build -> test -> evidence -> Director`

An agent may continue autonomously when the next action is deterministic and within its permissions.

Failures return to the Director with:
- reproduction steps;
- logs;
- changed files;
- test results;
- screenshots when relevant;
- suggested next action.

## 3. Real application verification

For desktop applications the verification environment must contain a real executable.

The Computer/Vision agent:
- starts the executable;
- observes the desktop;
- interacts with controls;
- captures screenshots;
- checks expected states;
- records failures.

Visual comparison is evidence for QA, not a substitute for executable testing.

## 4. Isolation

Coding agents operate in disposable workspaces/containers where possible.

The Windows GUI tester operates in a dedicated VM/sandbox.

Production credentials are never exposed to coding agents.

## 5. Repository workflow

main
  <- pull requests
feature/*
bugfix/*
agent/*

Every autonomous task should produce a traceable commit/PR and machine-readable test evidence.

## 6. Approval states

AUTO:
agent may continue.

APPROVAL_REQUIRED:
agent pauses and presents a concise decision.

BLOCKED:
missing credentials, environment or requirement.

DONE:
definition of done satisfied.

## 7. First implementation milestone

The first milestone is the autonomous foundation, not a redesign of MyCam Pro:
- persistent project state;
- task model;
- agent state model;
- approval model;
- build/test result model;
- GitHub integration;
- execution sandbox interface;
- evidence storage;
- Director loop.
