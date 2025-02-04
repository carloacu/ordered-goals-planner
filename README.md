# Ordered Goals Planner


## Description

This is C++ library to do planification PDDL.<br/>
The specificity of this planner is that it can handles goals sorted by priorities.<br/>

For specification about the PDDL language, please look at:<br/>
https://en.wikipedia.org/wiki/Planning_Domain_Definition_Language

A Kotlin version for Android (that needs to be updated) is also available here https://github.com/carloacu/contextualplanner-android


## Ordered goals requirement

For this it managed a problem requirement called `:ordered-goals`.<br/>
Each pieces of goals inside the `ordered-list` function will be satisfied one after another.<br/>
And before each piece of goal the effect defined in `:effect-between-goals` will be applied.

Example:
```lisp
  (:requirements :ordered-goals)
  [...]

  (:ordered-goals
    :effect-between-goals
      (not (has-grasped-an-object-for-current-goal))

    :goals
      (ordered-list
        (at-object box1 locationC)
        (at robot1 locationA)
      )
  )
```

The planner will first focus on the goal `(at-object box1 locationC)` and then on the goal `(at robot1 locationA)`.<br/>
Even if the plan for satisfying the first goal is chosen in consideration of helping to minimize the following goals.

Before each of these 2 goals the planner will apply the effect `(not (has-grasped-an-object-for-current-goal))`.


## Build

Go to the root directory of this repository and do

```bash
cmake -B build ./ && make -C build -j4
```

If you want to build in debug with the tests you can do


```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_ORDERED_GOALS_PLANNER_TESTS=ON ./ && make -C build -j4
```


## Quickstart

There is a PDDL example in `data/simple`.

After the compilation you can test the planner by doing

```bash
./build/bin/oderedgoalsplanner -d data/simple/domain.pddl -p data/simple/problem.pddl
```

The output should be:

```bash
00: (move robot1 locationA locationB) [1]
01: (pick-up robot1 box1 locationB) [1]
02: (move robot1 locationB locationC) [1]
03: (drop robot1 box1 locationC) [1]
04: (move robot1 locationC locationA) [1]
```

## Features

Supported [PDDL 3.1](https://helios.hud.ac.uk/scommv/IPC-14/repository/kovacs-pddl-3.1-2011.pdf)
requirements:

- [x] `:strips`
- [x] `:typing`
- [x] `:negative-preconditions`
- [x] `:disjunctive-preconditions`
- [x] `:equality`
- [x] `:existential-preconditions`
- [x] `:universal-preconditions`
- [x] `:quantified-preconditions`
- [x] `:conditional-effects`
- [x] `:fluents`
- [x] `:numeric-fluents`
- [x] `:object-fluents`
- [x] `:adl`
- [x] `:durative-actions`
- [ ] `:duration-inequalities`
- [x] `:derived-predicates`
- [ ] `:timed-initial-literals`
- [ ] `:preferences`
- [ ] `:constraints`
- [ ] `:action-costs`
- [x] `:domain-axioms`

And the problem requirement specific for this lib

- [x] `:ordered-goals`

### Definition of words

 * Action: Axiomatic thing that the bot can do
 * Domain: Set of all the actions that the bot can do.
 * Fact: Axiomatic knowledge
 * World: Set of facts currently true
 * Goal: Characteristics that the world should have. It is the motivation of the bot for doing actions to respect these characteristics.
 * Problem: Current world, goal for the world and historical of actions done.


### Drawbacks of existing planners

The plannification language has some drawbacks when it is applied to chatbot or to social robotics.

 * The choice of actions to do is led by minimizing the planning cost. (a planning cost is equal to the number of actions to do generaly)
   But for chatbot or social robotics we are often more interested about the most revelant action for the current context than shortest number of actions.

 * The associated algorithms are often costly. But in chatbot and social robotics we need reactivity.



### Improvements that this library is trying to solve


The improvements for chatbot and social robotics are:

 * The actions to do are chosen because each of them bring a significant step toward the goal resolution and
   because they are revelant according to the context. (context = facts already present in world)
   In other words, the planner tries to find a path to the goal that is the most revelant according to the context.

 * As we do not have to find the shortest path (because we focus about the context instead) and as we only need to find the next
   action, there is the possibility to have a solution much more optimized. For chatbot or social robotics it is important
   to be reactive even if the domain and the goals are bigs.



## Code documentation


[Here](include/orderedgoalsplanner/orderedgoalsplanner.hpp) are the documented headers of the main functions.

### Types

Here are the types providec by this library:

 * [Action](include/orderedgoalsplanner/types/action.hpp): Axiomatic thing that the bot can do.
 * [Domain](include/orderedgoalsplanner/types/domain.hpp): Set of all the actions that the bot can do.
 * [Expression](include/orderedgoalsplanner/types/expression.hpp): Expression for making arithmetic comparisons between facts.
 * [Fact](include/orderedgoalsplanner/types/fact.hpp): Axiomatic knowledge that can be contained in the world.
 * [Goal](include/orderedgoalsplanner/types/goal.hpp): A characteristic that the world should have. It is the motivation of the bot for doing actions to respect this characteristic of the world.
 * [Historical](include/orderedgoalsplanner/types/historical.hpp): Container of the actions already done.
 * [Problem](include/orderedgoalsplanner/types/historical.hpp): Current world, goal for the world and historical of actions done.
 * [SetOfFacts](include/orderedgoalsplanner/types/setoffacts.hpp): Container of a set of fact modifications to apply in the world.
 * [WorldModification](include/orderedgoalsplanner/types/worldmodification.hpp): Specification of a modification of the world.



## Examples of usage


Here is an example with only one action to do:

```cpp
#include <map>
#include <memory>
#include <assert.h>
#include <orderedgoalsplanner/orderedgoalsplanner.hpp>
#include <orderedgoalsplanner/types/setofcallbacks.hpp>
#include <orderedgoalsplanner/util/serializer/deserializefrompddl.hpp>


void planningDummyExample()
{
  // Fact
  const std::string userIsGreeted = "user_is_greeted";

  // Action identifier
  const std::string sayHi = "say_hi";

  // Current clock to set to different functions
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());

  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(userIsGreeted, ontology.types);

  // Initialize the domain with an action
  std::map<ogp::ActionId, ogp::Action> actions;
  actions.emplace(sayHi, ogp::Action({}, ogp::strToWsModification(userIsGreeted, ontology, {}, {})));
  ogp::Domain domain(actions, ontology);

  // Initialize the problem with the goal to satisfy
  ogp::Problem problem;
  problem.goalStack.setGoals({ogp::Goal::fromStr(userIsGreeted, ontology, {})}, problem.worldState, ontology.constants, problem.objects, now);

  // Look for an action to do
  ogp::SetOfCallbacks setOfCallbacks;
  auto planResult1 = ogp::planForMoreImportantGoalPossible(problem, domain, setOfCallbacks, true, now);
  assert(!planResult1.empty());
  const auto& firstActionInPlan = planResult1.front();
  assert(sayHi == firstActionInPlan.actionInvocation.actionId); // The action found is "say_hi", because it is needed to satisfy the preconditions of "ask_how_I_can_help"
  // When the action is finished we notify the planner
  ogp::notifyActionDone(problem, domain, setOfCallbacks, firstActionInPlan, now);

  // Look for the next action to do
  auto planResult2 = ogp::planForMoreImportantGoalPossible(problem, domain, setOfCallbacks, true, now);
  assert(planResult2.empty()); // No action found
}
```


Here is an example with two actions to do and with the usage of preconditions:


```cpp
#include <map>
#include <memory>
#include <assert.h>
#include <orderedgoalsplanner/orderedgoalsplanner.hpp>
#include <orderedgoalsplanner/types/setofcallbacks.hpp>
#include <orderedgoalsplanner/util/serializer/deserializefrompddl.hpp>


void planningExampleWithAPreconditionSolve()
{
  // Facts
  const std::string userIsGreeted = "user_is_greeted";
  const std::string proposedOurHelpToUser = "proposed_our_help_to_user";

  // Action identifiers
  const std::string sayHi = "say_hi";
  const std::string askHowICanHelp = "ask_how_I_can_help";

  // Current clock to set to different functions
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());

  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(userIsGreeted + "\n" +
                                                      proposedOurHelpToUser, ontology.types);

  // Initialize the domain with a set of actions
  std::map<ogp::ActionId, ogp::Action> actions;
  actions.emplace(sayHi, ogp::Action({}, ogp::strToWsModification(userIsGreeted, ontology, {}, {})));
  actions.emplace(askHowICanHelp, ogp::Action(ogp::strToCondition(userIsGreeted, ontology, {}, {}),
                                              ogp::strToWsModification(proposedOurHelpToUser, ontology, {}, {})));
  ogp::Domain domain(actions, ontology);

  // Initialize the problem with the goal to satisfy
  ogp::Problem problem;
  problem.goalStack.setGoals({ogp::Goal::fromStr(proposedOurHelpToUser, ontology, {})}, problem.worldState, ontology.constants, problem.objects, now);

  // Look for an action to do
  ogp::SetOfCallbacks setOfCallbacks;
  auto planResult1 = ogp::planForMoreImportantGoalPossible(problem, domain, setOfCallbacks, true, now);
  assert(!planResult1.empty());
  const auto& firstActionInPlan1 = planResult1.front();
  assert(sayHi == firstActionInPlan1.actionInvocation.actionId); // The action found is "say_hi", because it is needed to satisfy the preconditions of "ask_how_I_can_help"
  // When the action is finished we notify the planner
  ogp::notifyActionDone(problem, domain, setOfCallbacks, firstActionInPlan1, now);

  // Look for the next action to do
  auto planResult2 = ogp::planForMoreImportantGoalPossible(problem, domain, setOfCallbacks, true, now);
  assert(!planResult2.empty());
  const auto& firstActionInPlan2 = planResult2.front();
  assert(askHowICanHelp == firstActionInPlan2.actionInvocation.actionId); // The action found is "ask_how_I_can_help"
  // When the action is finished we notify the planner
  ogp::notifyActionDone(problem, domain, setOfCallbacks, firstActionInPlan2, now);

  // Look for the next action to do
  auto planResult3 = ogp::planForMoreImportantGoalPossible(problem, domain, setOfCallbacks, true, now);
  assert(planResult3.empty()); // No action found
}
```
