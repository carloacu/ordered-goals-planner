#include <gtest/gtest.h>
#include <orderedgoalsplanner/orderedgoalsplanner.hpp>
#include <orderedgoalsplanner/types/parallelplan.hpp>
#include <orderedgoalsplanner/types/setofcallbacks.hpp>
#include <orderedgoalsplanner/util/serializer/deserializefrompddl.hpp>

namespace
{
const std::map<ogp::SetOfEventsId, ogp::SetOfEvents> _emptySetOfEvents;
const std::string _sep = ", ";
const std::unique_ptr<std::chrono::steady_clock::time_point> _now = {};

const std::string _fact_a = "fact_a";
const std::string _fact_b = "fact_b";
const std::string _fact_c = "fact_c";
const std::string _fact_d = "fact_d";
const std::string _fact_e = "fact_e";
const std::string _fact_f = "fact_f";
const std::string _fact_g = "fact_g";


static const std::vector<ogp::Parameter> _emptyParameters;


ogp::Fact _fact(const std::string& pStr,
               const ogp::Ontology& pOntology,
               const std::vector<ogp::Parameter>& pParameters = {}) {
  return ogp::Fact(pStr, false, pOntology, {}, pParameters);
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


ogp::ActionInvocationWithGoal _lookForAnActionToDo(ogp::Problem& pProblem,
                                                  const ogp::Domain& pDomain,
                                                  const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {},
                                                  const ogp::Historical* pGlobalHistorical = nullptr)
{
  auto plan = ogp::planForMoreImportantGoalPossible(pProblem, pDomain, true, pNow, pGlobalHistorical);
  if (!plan.empty())
    return plan.front();
  return ogp::ActionInvocationWithGoal("", std::map<ogp::Parameter, ogp::Entity>(), {}, 0);
}

std::string _parallelPlanStr(ogp::Problem& pProblem,
                             const ogp::Domain& pDomain,
                             const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {},
                             ogp::Historical* pGlobalHistorical = nullptr)
{
  return ogp::parallelPlanToStr(ogp::parallelPlanForEveryGoals(pProblem, pDomain, pNow, pGlobalHistorical));
}

std::string _parallelPlanPddl(ogp::Problem& pProblem,
                              const ogp::Domain& pDomain,
                              const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {},
                              ogp::Historical* pGlobalHistorical = nullptr)
{
  return ogp::parallelPlanToPddl(ogp::parallelPlanForEveryGoals(pProblem, pDomain, pNow, pGlobalHistorical), pDomain);
}


std::string _lookForAnActionToDoInParallelThenNotifyToStr(
    ogp::Problem& pProblem,
    const ogp::Domain& pDomain,
    const ogp::SetOfCallbacks& pCallbacks,
    const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {})
{
  auto actionsToDoInParallel = ogp::actionsToDoInParallelNow(pProblem, pDomain, pNow);
  for (auto& currAction : actionsToDoInParallel.actions)
  {
    notifyActionStarted(pProblem, pDomain, pCallbacks, currAction, pNow);
    notifyActionDone(pProblem, pDomain, pCallbacks, currAction, pNow);
  }
  return ogp::planToStr(actionsToDoInParallel.actions);
}


void _test_callbacks()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("e_a e_b - entity");
  ontology.constants = ogp::SetOfEntities::fromPddl("v1 v2 - entity\n"
                                                    "e_a1 e_a2 - e_a\n"
                                                    "e_b1 e_b2 - e_b", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + " - entity\n" +
                                                      _fact_b + "\n" +
                                                      _fact_c + "(?e - e_a) - e_b", ontology.types);

  ogp::SetOfCallbacks callbacks;
  std::size_t nbOfCallback1 = 0;
  callbacks.add(ogp::ConditionToCallback(_condition_fromStr(_fact_a + "=v1", ontology), [&]() { ++nbOfCallback1; }));
  std::size_t nbOfCallback2 = 0;
  callbacks.add(ogp::ConditionToCallback(_condition_fromStr(_fact_a + "=v2 & " + _fact_c + "(e_a2)=e_b2", ontology), [&]() { ++nbOfCallback2; }));


  std::map<std::string, ogp::Action> actions;
  actions.emplace(action1, ogp::Action({}, _worldStateModification_fromStr(_fact_a + "=v1 & " + _fact_b, ontology)));
  actions.emplace(action2, ogp::Action(_condition_fromStr(_fact_a + "=v1", ontology), _worldStateModification_fromStr(_fact_a + "=v2 & " + _fact_c + "(e_a2)=e_b2", ontology)));

  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;

  auto fact1 = _fact(_fact_a + "=v1", ontology);
  auto fact2 = _fact(_fact_a + "=v2", ontology);
  EXPECT_EQ(0, nbOfCallback1);
  problem.worldState.addFact(fact1, problem.goalStack, setOfEventsMap, callbacks, ontology, ogp::SetOfEntities(), _now);
  EXPECT_EQ(1, nbOfCallback1);
  problem.worldState.addFact(fact1, problem.goalStack, setOfEventsMap, callbacks, ontology, ogp::SetOfEntities(), _now);
  EXPECT_EQ(1, nbOfCallback1);
  problem.worldState.removeFact(fact1, problem.goalStack, setOfEventsMap, callbacks, ontology, ogp::SetOfEntities(), _now);
  EXPECT_EQ(1, nbOfCallback1);
  problem.worldState.addFact(fact1, problem.goalStack, setOfEventsMap, callbacks, ontology, ogp::SetOfEntities(), _now);
  EXPECT_EQ(2, nbOfCallback1);
  problem.worldState.addFact(fact2, problem.goalStack, setOfEventsMap, callbacks, ontology, ogp::SetOfEntities(), _now);
  EXPECT_EQ(2, nbOfCallback1);
  problem.worldState.removeFact(fact2, problem.goalStack, setOfEventsMap, callbacks, ontology, ogp::SetOfEntities(), _now);
  EXPECT_EQ(2, nbOfCallback1);
  problem.worldState.addFact(fact2, problem.goalStack, setOfEventsMap, callbacks, ontology, ogp::SetOfEntities(), _now);
  EXPECT_EQ(2, nbOfCallback1);
  problem.worldState.addFact(fact1, problem.goalStack, setOfEventsMap, callbacks, ontology, ogp::SetOfEntities(), _now);
  EXPECT_EQ(3, nbOfCallback1);
  problem.worldState.removeFact(fact1, problem.goalStack, setOfEventsMap, callbacks, ontology, ogp::SetOfEntities(), _now);
  EXPECT_EQ(3, nbOfCallback1);
  nbOfCallback1 = 0;

  _setGoalsForAPriority(problem, {_goal(_fact_c + "(e_a2)=e_b2", ontology)}, ontology.constants);
  EXPECT_EQ(0, nbOfCallback1);
  EXPECT_EQ(action1, _lookForAnActionToDoInParallelThenNotifyToStr(problem, domain, callbacks, _now));
  EXPECT_EQ(1, nbOfCallback1);
  EXPECT_EQ(0, nbOfCallback2);
  EXPECT_EQ(action2, _lookForAnActionToDoInParallelThenNotifyToStr(problem, domain, callbacks, _now));
  EXPECT_EQ(1, nbOfCallback1);
  EXPECT_EQ(1, nbOfCallback2);
  EXPECT_EQ("", _lookForAnActionToDoInParallelThenNotifyToStr(problem, domain, callbacks, _now));
  EXPECT_EQ(1, nbOfCallback1);
  EXPECT_EQ(1, nbOfCallback2);
  nbOfCallback1 = 0;
  nbOfCallback2 = 0;

  problem.worldState.removeFact(fact2, problem.goalStack, setOfEventsMap, callbacks, ontology, ogp::SetOfEntities(), _now);
  EXPECT_EQ(0, nbOfCallback2);
  problem.worldState.addFact(fact2, problem.goalStack, setOfEventsMap, callbacks, ontology, ogp::SetOfEntities(), _now);
  EXPECT_EQ(1, nbOfCallback2);
  EXPECT_EQ(0, nbOfCallback1);
  nbOfCallback1 = 0;
  nbOfCallback2 = 0;

  {
    std::size_t pos = 0;
    problem.worldState.modifyFactsFromPddl("(= (fact_a) v1)", pos, problem.goalStack, setOfEventsMap, callbacks, ontology, ogp::SetOfEntities(), _now);
  }
  EXPECT_EQ(1, nbOfCallback1);
  EXPECT_EQ(0, nbOfCallback2);
  {
    std::size_t pos = 0;
    problem.worldState.modifyFactsFromPddl("(= (fact_a) v2)", pos, problem.goalStack, setOfEventsMap, callbacks, ontology, ogp::SetOfEntities(), _now);
  }
  EXPECT_EQ(1, nbOfCallback1);
  EXPECT_EQ(1, nbOfCallback2);
  {
    std::size_t pos = 0;
    problem.worldState.modifyFactsFromPddl("(= (fact_c e_a2) e_b1)", pos, problem.goalStack, setOfEventsMap, callbacks, ontology, ogp::SetOfEntities(), _now);
  }
  EXPECT_EQ(1, nbOfCallback1);
  EXPECT_EQ(1, nbOfCallback2);
  {
    std::size_t pos = 0;
    problem.worldState.modifyFactsFromPddl("(= (fact_a) v1)", pos, problem.goalStack, setOfEventsMap, callbacks, ontology, ogp::SetOfEntities(), _now);
  }
  EXPECT_EQ(2, nbOfCallback1);
  EXPECT_EQ(1, nbOfCallback2);
  {
    std::size_t pos = 0;
    problem.worldState.modifyFactsFromPddl("(= (fact_c e_a2) e_b2)\n(= (fact_a) v2)", pos, problem.goalStack, setOfEventsMap, callbacks, ontology, ogp::SetOfEntities(), _now);
  }
  EXPECT_EQ(2, nbOfCallback1);
  EXPECT_EQ(2, nbOfCallback2);
}


}



TEST(Planner, test_callbacks)
{
  _test_callbacks();
}
