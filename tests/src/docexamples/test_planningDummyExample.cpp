#include "test_planningDummyExample.hpp"
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

