#include <gtest/gtest.h>
#include <orderedgoalsplanner/orderedgoalsplanner.hpp>
#include <orderedgoalsplanner/types/setofcallbacks.hpp>
#include <orderedgoalsplanner/util/serializer/deserializefrompddl.hpp>

namespace
{
const ogp::SetOfCallbacks _emptyCallbacks;

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



void _test_effect_beween_goals()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";

  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr("fact_a\n"
                                                      "fact_b", ontology.types);

  std::map<std::string, ogp::Action> actions;
  ogp::Action actionObj1({}, _worldStateModification_fromPddl("(fact_a)", ontology));
  actions.emplace(action1, actionObj1);

  ogp::Action actionObj2(_condition_fromPddl("(not (fact_a))", ontology),
                         _worldStateModification_fromPddl("(fact_b)", ontology));
  actions.emplace(action2, actionObj2);

  ogp::Domain domain(std::move(actions), ontology);
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_pddlGoal("(fact_a)", ontology, problem.objects),
                                  _pddlGoal("(fact_b)", ontology, problem.objects)}, ontology.constants);
  problem.goalStack.effectBeweenGoals = _worldStateModification_fromPddl("(not (fact_a))", ontology);

  EXPECT_EQ(action1, _lookForAnActionToDoThenNotify(problem, domain).actionInvocation.toStr());
  EXPECT_EQ(action2, _lookForAnActionToDoThenNotify(problem, domain).actionInvocation.toStr());
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain).actionInvocation.toStr());
}

}



TEST(Planner, test_ordered_goals_requirement)
{
  _test_effect_beween_goals();
}
