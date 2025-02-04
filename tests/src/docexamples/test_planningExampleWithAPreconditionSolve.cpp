#include "test_planningExampleWithAPreconditionSolve.hpp"
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
