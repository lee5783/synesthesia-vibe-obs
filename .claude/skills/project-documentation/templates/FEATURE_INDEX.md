# [Feature Name]

**Analysis Date:** [YYYY-MM-DD]

## Directory Naming
Place this file at `docs/features/<feature>/INDEX.md` (or `docs/features/<feature>/<sub-feature>/INDEX.md`), where `<feature>` and `<sub-feature>` directory names are in `kebab-case`.

## Purpose
[What this feature does, in one or two sentences]

## User-Facing Behavior
- [What users (or calling code) observe]
- [Any important flows or invariants]

## Entry Points
- `[path]` - [How to reach/use this feature]

## Feature Structure
How this feature is laid out in the codebase (directories, layers, and responsibilities).

```
[relevant-root]/
├── `[dir-or-file]/` - [Role: UI, API, domain, infra, etc.]
├── `[dir-or-file]/` - [Role]
└── `[path]` - [Role]
```

**Module / layer breakdown**
- `[path]` - [What belongs here and how it relates to sibling modules]

**Where to add new code for this feature**
- [New UI / route / handler]: `[suggested path]`
- [New domain logic]: `[suggested path]`
- [Tests]: `[suggested path]`

## Core Implementation
- `[path]` - [What this module/function is responsible for]

## Data / Contracts
- [Inputs]
- [Outputs]
- [Key data types/schemas, if applicable]

## Dependencies and Integrations
- `[package/service]` - [Why it matters]

## Error / Edge Cases
- [Edge case] - [Expected behavior]
- [Failure mode] - [How it is handled]

## Related Files
- `[path]`

## Navigation

### Parent
- `[Parent Feature Name]`: `[docs/features/.../INDEX.md](docs/features/.../INDEX.md)`

### Direct Sub-features
- `[Sub-feature Name 1]`: `[docs/features/.../INDEX.md](docs/features/.../INDEX.md)`

