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
const ogp::SetOfCallbacks _emptyCallbacks;

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


void _addFact(ogp::WorldState& pWorldState,
              const std::string& pFactStr,
              ogp::GoalStack& pGoalStack,
              const ogp::Ontology& pOntology,
              const std::map<ogp::SetOfEventsId, ogp::SetOfEvents>& pSetOfEvents = _emptySetOfEvents,
              const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {}) {
  pWorldState.addFact(_fact(pFactStr, pOntology), pGoalStack, pSetOfEvents, _emptyCallbacks, pOntology, ogp::SetOfEntities(), pNow);
}

ogp::ActionInvocationWithGoal _lookForAnActionToDo(ogp::Problem& pProblem,
                                                  const ogp::Domain& pDomain,
                                                  const ogp::SetOfCallbacks& pCallbacks = {},
                                                  const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {},
                                                  const ogp::Historical* pGlobalHistorical = nullptr)
{
  auto plan = ogp::planForMoreImportantGoalPossible(pProblem, pDomain, pCallbacks, true, pNow, pGlobalHistorical);
  if (!plan.empty())
    return plan.front();
  return ogp::ActionInvocationWithGoal("", std::map<ogp::Parameter, ogp::Entity>(), {}, 0);
}


std::string _lookForAnActionToDoInParallelThenNotifyToStr(
    ogp::Problem& pProblem,
    const ogp::Domain& pDomain,
    const ogp::SetOfCallbacks& pCallbacks = {},
    const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {})
{
  auto actionsToDoInParallel = ogp::actionsToDoInParallelNow(pProblem, pDomain, pCallbacks, pNow);
  for (auto& currAction : actionsToDoInParallel.actions)
  {
    notifyActionStarted(pProblem, pDomain, _emptyCallbacks, currAction, pNow);
    notifyActionDone(pProblem, pDomain, _emptyCallbacks, currAction, pNow);
  }
  return ogp::planToStr(actionsToDoInParallel.actions);
}

std::string _parallelPlanStr(ogp::Problem& pProblem,
                             const ogp::Domain& pDomain,
                             const ogp::SetOfCallbacks& pCallbacks = {},
                             const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {},
                             ogp::Historical* pGlobalHistorical = nullptr)
{
  return ogp::parallelPlanToStr(ogp::parallelPlanForEveryGoals(pProblem, pDomain, pCallbacks, pNow, pGlobalHistorical));
}

std::string _parallelPlanPddl(ogp::Problem& pProblem,
                              const ogp::Domain& pDomain,
                              const ogp::SetOfCallbacks& pCallbacks = {},
                              const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {},
                              ogp::Historical* pGlobalHistorical = nullptr)
{
  return ogp::parallelPlanToPddl(ogp::parallelPlanForEveryGoals(pProblem, pDomain, pCallbacks, pNow, pGlobalHistorical), pDomain);
}


void _goalsToDoInParallel()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  const std::string action3 = "action3";
  const std::string action4 = "action4";
  const std::string action5 = "action5";
  const std::string action6 = "action6";
  const std::string action7 = "action7";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity");
  ontology.constants = ogp::SetOfEntities::fromPddl("val1 val2 val3 val4 - entity", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + " - entity\n" +
                                                     _fact_b + "\n" +
                                                     _fact_c + "\n" +
                                                     _fact_d + "\n" +
                                                     _fact_e + "\n" +
                                                     _fact_f + "\n" +
                                                     _fact_g, ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> act1Parameters{_parameter("?obj - entity", ontology)};
  ogp::Action actionObj1({}, _worldStateModification_fromStr(_fact_a + "=?obj", ontology, act1Parameters));
  actionObj1.parameters = std::move(act1Parameters);
  actions.emplace(action1, actionObj1);

  ogp::Action actionObj2(_condition_fromStr(_fact_a + "=val1", ontology),
                        _worldStateModification_fromStr(_fact_b + " & " + _fact_c, ontology));
  actions.emplace(action2, actionObj2);

  ogp::Action actionObj3(_condition_fromStr(_fact_a + "=val2", ontology),
                        _worldStateModification_fromStr(_fact_b + " & " + _fact_c, ontology));
  actions.emplace(action3, actionObj3);

  ogp::Action actionObj4(_condition_fromStr(_fact_a + "=val3 & !" + _fact_c, ontology),
                        _worldStateModification_fromStr("!" + _fact_d + " & " + _fact_f, ontology));
  actions.emplace(action4, actionObj4);

  ogp::Action actionObj5(_condition_fromStr(_fact_a + "=val4", ontology),
                        _worldStateModification_fromStr(_fact_b + " & " + _fact_c, ontology));
  actions.emplace(action5, actionObj5);

  ogp::Action actionObj6(_condition_fromStr(_fact_b + " & !" + _fact_d + " & " + _fact_g, ontology),
                        _worldStateModification_fromStr(_fact_e, ontology));
  actions.emplace(action6, actionObj6);

  ogp::Action actionObj7(_condition_fromStr(_fact_f, ontology),
                        _worldStateModification_fromStr(_fact_g, ontology));
  actions.emplace(action7, actionObj7);

  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();

  ogp::Problem problem;
  _addFact(problem.worldState, _fact_d, problem.goalStack, ontology, setOfEventsMap, _now);
  _setGoalsForAPriority(problem, {_goal(_fact_e, ontology)}, ontology.constants);

  EXPECT_EQ(action1 + "(?obj -> val3)", _lookForAnActionToDo(problem, domain).actionInvocation.toStr());

  EXPECT_EQ(action1 + "(?obj -> val3)", _lookForAnActionToDoInParallelThenNotifyToStr(problem, domain));
  EXPECT_EQ(action4, _lookForAnActionToDoInParallelThenNotifyToStr(problem, domain));
  EXPECT_EQ(action7 + _sep + action1 + "(?obj -> val1)", _lookForAnActionToDoInParallelThenNotifyToStr(problem, domain));
  EXPECT_EQ(action2, _lookForAnActionToDoInParallelThenNotifyToStr(problem, domain));
  EXPECT_EQ(action6, _lookForAnActionToDoInParallelThenNotifyToStr(problem, domain));
  EXPECT_EQ("", _lookForAnActionToDoInParallelThenNotifyToStr(problem, domain));
}

void _2actionsInParallel()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";

  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + "\n" +
                                                      _fact_b, ontology.types);

  std::map<std::string, ogp::Action> actions;
  actions.emplace(action1, ogp::Action({}, _worldStateModification_fromStr(_fact_a, ontology)));
  actions.emplace(action2, ogp::Action({}, _worldStateModification_fromStr(_fact_b, ontology)));

  ogp::Domain domain(std::move(actions), ontology);
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal(_fact_a + " & " + _fact_b, ontology)}, ontology.constants);
  EXPECT_EQ("action1, action2", _parallelPlanStr(problem, domain));
}


void _2actionsNotInParallelBecauseFrom2DifferentSkills()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";

  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + "\n" +
                                                      _fact_b, ontology.types);

  std::map<std::string, ogp::Action> actions;
  actions.emplace(action1, ogp::Action({}, _worldStateModification_fromStr(_fact_a, ontology)));
  actions.emplace(action2, ogp::Action({}, _worldStateModification_fromStr(_fact_b, ontology)));

  ogp::Domain domain(std::move(actions), ontology);
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal(_fact_a, ontology), _goal(_fact_b, ontology)}, ontology.constants);
  EXPECT_EQ("action1\n"
            "action2", _parallelPlanStr(problem, domain));
}


void _moreThan2GoalsInParallel()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  const std::string action3 = "action3";
  const std::string action4 = "action4";

  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + "\n" +
                                                      _fact_b + "\n" +
                                                      _fact_c + "\n" +
                                                      _fact_d, ontology.types);

  std::map<std::string, ogp::Action> actions;
  actions.emplace(action1, ogp::Action({}, _worldStateModification_fromStr(_fact_a, ontology)));
  actions.emplace(action2, ogp::Action(_condition_fromStr(_fact_a, ontology),
                                       _worldStateModification_fromStr(_fact_b, ontology)));
  actions.emplace(action3, ogp::Action({}, _worldStateModification_fromStr(_fact_c, ontology)));
  actions.emplace(action4, ogp::Action({}, _worldStateModification_fromStr(_fact_d, ontology)));

  ogp::Domain domain(std::move(actions), ontology);
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal(_fact_a + " & " + _fact_b + " & " + _fact_c + " & " + _fact_d, ontology)}, ontology.constants);
  auto problem2 = problem;

  EXPECT_EQ("action1, action3, action4\n"
            "action2", _parallelPlanStr(problem, domain));

  EXPECT_EQ("00: (action1) [1]\n"
            "00: (action3) [1]\n"
            "00: (action4) [1]\n"
            "01: (action2) [1]\n", _parallelPlanPddl(problem2, domain));
}


void _goalsToDoInParallelWithConflitingEffects()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity");
  ontology.constants = ogp::SetOfEntities::fromPddl("v1 v2 - entity", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + " - entity\n" +
                                                      _fact_b + "\n" +
                                                      _fact_c, ontology.types);

  std::map<std::string, ogp::Action> actions;
  actions.emplace(action1, ogp::Action({}, _worldStateModification_fromStr(_fact_a + "=v1 & " + _fact_b, ontology)));
  actions.emplace(action2, ogp::Action({}, _worldStateModification_fromStr(_fact_a + "=v2 & " + _fact_c, ontology)));

  ogp::Domain domain(std::move(actions), ontology);
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal(_fact_b + " & " + _fact_c, ontology)}, ontology.constants);

  EXPECT_EQ("action1\n"
            "action2", _parallelPlanStr(problem, domain));
}


}



TEST(Planner, test_parallelPlan)
{
  _goalsToDoInParallel();
  _2actionsInParallel();
  _2actionsNotInParallelBecauseFrom2DifferentSkills();
  _moreThan2GoalsInParallel();
  _goalsToDoInParallelWithConflitingEffects();
}
