# TODO — Ordered Goals Planner

## Numeric planning support

The solver currently cannot find plans for problems with numeric goals
like `(>= (money alice) 1000000)` when reaching them requires executing
the same action many times.

### Repetitive action limit

In `src/orderedgoalsplanner.cpp` (`_goalToPlanRec`, around line 1009),
each action invocation string is limited to **2 occurrences** per plan.
This makes sense for symbolic planning (prevents infinite loops) but
blocks any problem requiring iterative progress toward a numeric target.

**Ideas:**

- Add a separate, higher limit for actions whose effects modify a numeric
  fluent referenced in the current goal. The limit could be derived from
  `(targetValue - currentValue) / deltaPerAction` with a safety margin.
- Alternatively, detect when an action monotonically progresses toward a
  numeric goal and exempt it from the duplicate limit, relying on the
  goal-satisfaction check (`isGoalSatisfied`) to stop the loop.
- Introduce a configurable `maxActionsPerGoal` parameter on the `Goal` or
  `Problem` level so users can raise the ceiling for known-expensive goals.

### Backward chaining heuristic for numeric goals

`_lookForAPossibleEffect` (line 670) uses `isASimpleFactObjective()` as a
fast path. For numeric comparison goals (`>=`, `<=`, `>`, `<`) this
returns `false`, falling through to the succession-based search. The
succession path works structurally (matching fluent names) but:

- `getValue()` on an uninstantiated parameter returns `nullopt`, so the
  computed effect value (e.g. `currentMoney + 40000`) is lost. The
  callback only sees the fluent name with no value.
- `_checkConditionAndFillParameters` then calls `canBeModifiedBy`, which
  does a structural check but does not verify whether the resulting value
  would actually satisfy the `>=` comparison.

**Ideas:**

- When the effect is `INCREASE`/`DECREASE` and the goal is
  `SUPERIOR_OR_EQUAL`/`INFERIOR_OR_EQUAL` on the same fluent, compute
  the post-action value using the **instantiated** parameter (after
  parameter unification) and check whether it satisfies the numeric
  condition. This bridges the gap between structural matching and
  actual numeric reasoning.
- Add a `canNumericallyProgress` helper that, given the current fluent
  value, the delta, and the target comparison, returns whether applying
  the action moves toward the goal. Use this in `_lookForAPossibleEffect`
  alongside the structural match.

### Forward search fallback for numeric goals

For goals that inherently require long action chains (e.g., earning money
over many work cycles), backward chaining is a poor fit. A forward search
component could complement it:

- If backward chaining yields no plan and the goal involves numeric
  comparisons, fall back to a bounded forward search (BFS/DFS with a step
  limit) that greedily applies actions progressing toward the target.
- Use the backward-chaining heuristic to prune the forward search space:
  only consider actions that the succession cache identifies as relevant.

## PDDL feature completeness

### :metric support

The `(:metric minimize (total-cost))` block is currently skipped during
parsing (`deserializefrompddl.cpp`). To actually support metric
optimization:

- Parse and store the metric expression.
- Use it in plan cost evaluation (`_extractPlanCost`) to prefer cheaper
  plans when multiple solutions exist.
- Support `total-cost` as a special fluent automatically incremented by
  action costs.

### :durative-actions

The solver has partial support for durations (`action.duration`) but does
not support PDDL `:durative-actions` syntax (`(:durative-action ... :duration (= ?duration 5) ...)`). Full support would require:

- Parsing `at start`, `at end`, `over all` conditions and effects.
- A temporal planning layer on top of the current state-space search.

### Derived predicates / axioms

The ontology has `SetOfDerivedPredicates` but derived predicates are not
fully integrated into the search. Axiom evaluation during world-state
expansion would enable richer domain modeling.

## Code quality

### Test coverage for numeric planning

Add test cases in `tests/` for:

- PDDL files with numeric fluents, `increase`/`decrease` effects, and
  numeric comparison goals.
- `when` effects with numeric conditions.
- Round-trip serialization (parse → serialize → parse) of numeric
  expressions.

### WorldState construction cost in conditionForWhen

The `forAll` method in `WorldStateModificationNode` creates a temporary
`WorldState` from `SetOfFacts` each time a `conditionForWhen` is
evaluated. If `when` effects are frequent, consider:

- Adding a `forAll` overload that accepts `const WorldState&` directly.
- Caching the temporary `WorldState` across evaluations in the same
  `applyEffect` call.
