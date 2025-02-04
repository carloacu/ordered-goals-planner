#include <gtest/gtest.h>
#include <orderedgoalsplanner/orderedgoalsplanner.hpp>
#include <orderedgoalsplanner/types/parallelplan.hpp>
#include <orderedgoalsplanner/types/setofcallbacks.hpp>
#include <orderedgoalsplanner/util/serializer/deserializefrompddl.hpp>

namespace
{
const std::unique_ptr<std::chrono::steady_clock::time_point> _now = {};
const std::map<ogp::SetOfEventsId, ogp::SetOfEvents> _emptySetOfEvents;
const ogp::SetOfCallbacks _emptyCallbacks;

const std::string _fact_a = "fact_a";
const std::string _fact_b = "fact_b";
const std::string _fact_c = "fact_c";

ogp::Fact _fact(const std::string& pStr,
               const ogp::Ontology& pOntology,
               const std::vector<ogp::Parameter>& pParameters = {}) {
  return ogp::Fact(pStr, false, pOntology, {}, pParameters);
}

void _addFact(ogp::WorldState& pWorldState,
              const std::string& pFactStr,
              ogp::GoalStack& pGoalStack,
              const ogp::Ontology& pOntology,
              const std::map<ogp::SetOfEventsId, ogp::SetOfEvents>& pSetOfEvents = _emptySetOfEvents,
              const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {}) {
  pWorldState.addFact(_fact(pFactStr, pOntology), pGoalStack, pSetOfEvents, _emptyCallbacks, pOntology, ogp::SetOfEntities(), pNow);
}

void _removeFact(ogp::WorldState& pWorldState,
                 const std::string& pFactStr,
                 ogp::GoalStack& pGoalStack,
                 const ogp::Ontology& pOntology,
                 const std::map<ogp::SetOfEventsId, ogp::SetOfEvents>& pSetOfEvents = _emptySetOfEvents,
                 const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {}) {
  pWorldState.removeFact(_fact(pFactStr, pOntology), pGoalStack, pSetOfEvents, _emptyCallbacks, pOntology, ogp::SetOfEntities(), pNow);
}


ogp::Parameter _parameter(const std::string& pStr,
                         const ogp::Ontology& pOntology) {
  return ogp::Parameter::fromStr(pStr, pOntology.types);
}

ogp::Goal _goal(const std::string& pStr,
               const ogp::Ontology& pOntology,
               int pMaxTimeToKeepInactive = -1,
               const std::string& pGoalGroupId = "") {
  return ogp::Goal::fromStr(pStr, pOntology, {}, pMaxTimeToKeepInactive, pGoalGroupId);
}

std::unique_ptr<ogp::Condition> _condition_fromStr(const std::string& pConditionStr,
                                                  const ogp::Ontology& pOntology,
                                                  const std::vector<ogp::Parameter>& pParameters = {}) {
  return ogp::strToCondition(pConditionStr, pOntology, {}, pParameters);
}

std::unique_ptr<ogp::WorldStateModification> _worldStateModification_fromStr(const std::string& pStr,
                                                                            const ogp::Ontology& pOntology,
                                                                            const std::vector<ogp::Parameter>& pParameters = {}) {
  return ogp::strToWsModification(pStr, pOntology, {}, pParameters);
}

void _setGoalsForAPriority(ogp::Problem& pProblem,
                           const std::vector<ogp::Goal>& pGoals,
                           const ogp::SetOfEntities& pConstants,
                           const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {},
                           int pPriority = ogp::GoalStack::getDefaultPriority())
{
  pProblem.goalStack.setGoals(pGoals, pProblem.worldState, pConstants, pProblem.objects, pNow, pPriority);
}

bool _evaluate(ogp::ParallelPan& pPlan,
               ogp::Problem& pProblem,
               const ogp::Domain& pDomain)
{
  auto plan = pPlan;
  auto problem = pProblem;
  return ogp::evaluate(plan, problem, pDomain);
}


void _testEvaluateWithPlanNotValidAnymore()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";

  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + "\n" +
                                                      _fact_b + "\n" +
                                                      _fact_c, ontology.types);

  std::map<std::string, ogp::Action> actions;
  actions.emplace(action1, ogp::Action({}, _worldStateModification_fromStr(_fact_a, ontology)));
  actions.emplace(action2, ogp::Action(_condition_fromStr(_fact_a + " & " + _fact_c, ontology),
                                       _worldStateModification_fromStr(_fact_b, ontology)));

  ogp::Domain domain(std::move(actions), ontology);
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal(_fact_b, ontology)}, ontology.constants);
  _addFact(problem.worldState, _fact_c,  problem.goalStack, ontology);

  auto copiedProblem = problem;
  auto parallelPlan = ogp::parallelPlanForEveryGoals(copiedProblem, domain, _emptyCallbacks, _now, nullptr);
  ASSERT_FALSE(parallelPlan.actionsToDoInParallel.empty());
  EXPECT_EQ(action1, ogp::planToStr(parallelPlan.actionsToDoInParallel.front().actions));

  EXPECT_TRUE(_evaluate(parallelPlan, problem, domain));
  _removeFact(problem.worldState, _fact_c,  problem.goalStack, ontology);
  EXPECT_FALSE(_evaluate(parallelPlan, problem, domain));
}

void _testEvaluateWithFirstActionAlreadyDone()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";

  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + "\n" +
                                                      _fact_b, ontology.types);

  std::map<std::string, ogp::Action> actions;
  actions.emplace(action1, ogp::Action({}, _worldStateModification_fromStr(_fact_a, ontology)));
  actions.emplace(action2, ogp::Action(_condition_fromStr(_fact_a, ontology),
                                       _worldStateModification_fromStr(_fact_b, ontology)));

  ogp::Domain domain(std::move(actions), ontology);
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal(_fact_b, ontology)}, ontology.constants);

  auto copiedProblem = problem;
  auto parallelPlan = ogp::parallelPlanForEveryGoals(copiedProblem, domain, _emptyCallbacks, _now, nullptr);
  ASSERT_FALSE(parallelPlan.actionsToDoInParallel.empty());
  EXPECT_EQ(action1, ogp::planToStr(parallelPlan.actionsToDoInParallel.front().actions));

  EXPECT_TRUE(_evaluate(parallelPlan, problem, domain));

  // Apply first action
  for (auto& currAction : parallelPlan.actionsToDoInParallel.front().actions)
  {
    ogp::notifyActionStarted(problem, domain, _emptyCallbacks, currAction, _now);
    ogp::notifyActionDone(problem, domain, _emptyCallbacks, currAction, _now);
    break;
  }

  EXPECT_FALSE(_evaluate(parallelPlan, problem, domain));
}

void _testEvaluateWithOneOfFirstActionAlreadyDone()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  const std::string action3 = "action3";

  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + "\n" +
                                                      _fact_b + "\n" +
                                                      _fact_c, ontology.types);

  std::map<std::string, ogp::Action> actions;
  actions.emplace(action1, ogp::Action({}, _worldStateModification_fromStr(_fact_a, ontology)));
  actions.emplace(action2, ogp::Action({}, _worldStateModification_fromStr(_fact_b, ontology)));
  actions.emplace(action3, ogp::Action(_condition_fromStr(_fact_a + " & " + _fact_b, ontology),
                                       _worldStateModification_fromStr(_fact_c, ontology)));

  ogp::Domain domain(std::move(actions), ontology);
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal(_fact_c, ontology)}, ontology.constants);

  auto copiedProblem = problem;
  auto parallelPlan = ogp::parallelPlanForEveryGoals(copiedProblem, domain, _emptyCallbacks, _now, nullptr);
  ASSERT_FALSE(parallelPlan.actionsToDoInParallel.empty());
  EXPECT_TRUE(action1 + ", " + action2 == ogp::planToStr(parallelPlan.actionsToDoInParallel.front().actions));

  EXPECT_TRUE(_evaluate(parallelPlan, problem, domain));

  // Apply first action
  for (auto& currAction : parallelPlan.actionsToDoInParallel.front().actions)
  {
    ogp::notifyActionStarted(problem, domain, _emptyCallbacks, currAction, _now);
    ogp::notifyActionDone(problem, domain, _emptyCallbacks, currAction, _now);
    break;
  }

  EXPECT_FALSE(_evaluate(parallelPlan, problem, domain));
}


void _testAlreadySatisfiedGoal()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";

  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + "\n" +
                                                      _fact_b + "\n" +
                                                      _fact_c, ontology.types);

  std::map<std::string, ogp::Action> actions;
  actions.emplace(action1, ogp::Action({}, _worldStateModification_fromStr(_fact_a, ontology)));
  actions.emplace(action2, ogp::Action(_condition_fromStr(_fact_a, ontology),
                                       _worldStateModification_fromStr(_fact_b + " & " + _fact_c, ontology)));

  ogp::Domain domain(std::move(actions), ontology);
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal(_fact_b, ontology)}, ontology.constants);

  auto copiedProblem = problem;
  auto parallelPlan = ogp::parallelPlanForEveryGoals(copiedProblem, domain, _emptyCallbacks, _now, nullptr);
  EXPECT_TRUE(_evaluate(parallelPlan, problem, domain));
  _addFact(problem.worldState, _fact_b,  problem.goalStack, ontology);
  EXPECT_FALSE(_evaluate(parallelPlan, problem, domain));
}


}



TEST(Planner, test_evaluator)
{
  _testEvaluateWithPlanNotValidAnymore();
  _testEvaluateWithFirstActionAlreadyDone();
  _testEvaluateWithOneOfFirstActionAlreadyDone();
  _testAlreadySatisfiedGoal();
}
