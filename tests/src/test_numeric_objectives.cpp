#include <gtest/gtest.h>
#include <orderedgoalsplanner/orderedgoalsplanner.hpp>
#include <orderedgoalsplanner/types/setofcallbacks.hpp>
#include <orderedgoalsplanner/util/serializer/deserializefrompddl.hpp>

namespace
{
const std::map<ogp::SetOfEventsId, ogp::SetOfEvents> _emptySetOfEvents;
const ogp::SetOfCallbacks _emptyCallbacks;

ogp::Fact _fact(const std::string& pStr,
                const ogp::Ontology& pOntology,
                const ogp::SetOfEntities& pObjects,
                const std::vector<ogp::Parameter>& pParameters = {}) {
  return ogp::Fact(pStr, true, pOntology, pObjects, pParameters);
}

void _addFact(ogp::WorldState& pWorldState,
              const std::string& pFactStr,
              ogp::GoalStack& pGoalStack,
              const ogp::Ontology& pOntology,
              const ogp::SetOfEntities& pObjects,
              const std::map<ogp::SetOfEventsId, ogp::SetOfEvents>& pSetOfEvents = _emptySetOfEvents,
              const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {}) {
  pWorldState.addFact(_fact(pFactStr, pOntology, pObjects), pGoalStack, pSetOfEvents, _emptyCallbacks, pOntology, pObjects, pNow);
}


ogp::Goal _pddlGoal(const std::string& pStr,
                    const ogp::Ontology& pOntology,
                    const ogp::SetOfEntities& pObjects,
                    int pMaxTimeToKeepInactive = -1,
                    const std::string& pGoalGroupId = "") {
  std::size_t pos = 0;
  return *ogp::pddlToGoal(pStr, pos, pOntology, pObjects, pMaxTimeToKeepInactive, pGoalGroupId);
}

std::unique_ptr<ogp::Condition> _condition_fromPddl(const std::string& pConditionStr,
                                                    const ogp::Ontology& pOntology,
                                                    const std::vector<ogp::Parameter>& pParameters = {}) {
  std::size_t pos = 0;
  return ogp::pddlToCondition(pConditionStr, pos, pOntology, {}, pParameters);
}

std::unique_ptr<ogp::WorldStateModification> _worldStateModification_fromPddl(const std::string& pStr,
                                                                              const ogp::Ontology& pOntology,
                                                                              const std::vector<ogp::Parameter>& pParameters = {}) {
  std::size_t pos = 0;
  return ogp::pddlToWsModification(pStr, pos, pOntology, {}, pParameters);
}

void _setGoalsForAPriority(ogp::Problem& pProblem,
                           const std::vector<ogp::Goal>& pGoals,
                           const ogp::SetOfEntities& pConstants,
                           const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {},
                           int pPriority = ogp::GoalStack::getDefaultPriority())
{
  pProblem.goalStack.setGoals(pGoals, pProblem.worldState, pConstants, pProblem.objects, pNow, pPriority);
}


ogp::ActionInvocationWithGoal _lookForAnActionToDoThenNotify(
    ogp::Problem& pProblem,
    const ogp::Domain& pDomain,
    const ogp::SetOfCallbacks& pCallbacks = {},
    const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {})
{
  auto plan = ogp::planForMoreImportantGoalPossible(pProblem, pDomain, pCallbacks, true, pNow);
  if (!plan.empty())
  {
    auto& firstActionInPlan = plan.front();
    notifyActionStarted(pProblem, pDomain, _emptyCallbacks, firstActionInPlan, pNow);
    notifyActionDone(pProblem, pDomain, _emptyCallbacks, firstActionInPlan, pNow);
    return firstActionInPlan;
  }
  return ogp::ActionInvocationWithGoal("", std::map<ogp::Parameter, ogp::Entity>(), {}, 0);
}



void _test_do_many_times_an_action_until_goal_success()
{
  const std::string action1 = "action1";

  ogp::Ontology ontology;
  {
    std::size_t pos = 0;
    ontology.predicates = ogp::SetOfPredicates::fromPddl("(fact_a) - number", pos, ontology.types);
  }

  std::map<std::string, ogp::Action> actions;
  ogp::Action actionObj1({}, _worldStateModification_fromPddl("(increase (fact_a) 1)", ontology));
  actions.emplace(action1, actionObj1);

  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  _addFact(problem.worldState, "(= (fact_a) 0)", problem.goalStack, ontology, problem.objects, setOfEventsMap);
  _setGoalsForAPriority(problem, {_pddlGoal("(>= (fact_a) 15)", ontology, problem.objects)}, ontology.constants);

  for (int i = 0; i < 15; ++i)
    EXPECT_EQ(action1, _lookForAnActionToDoThenNotify(problem, domain).actionInvocation.toStr());
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain).actionInvocation.toStr());
}


}



TEST(Planner, test_numeric_objectives)
{
  _test_do_many_times_an_action_until_goal_success();
}
