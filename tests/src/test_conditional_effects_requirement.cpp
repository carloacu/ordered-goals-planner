#include <gtest/gtest.h>
#include <orderedgoalsplanner/orderedgoalsplanner.hpp>
#include <orderedgoalsplanner/types/setofcallbacks.hpp>
#include <orderedgoalsplanner/types/setofevents.hpp>
#include <orderedgoalsplanner/util/serializer/deserializefrompddl.hpp>

namespace
{
const std::map<ogp::SetOfEventsId, ogp::SetOfEvents> _emptySetOfEvents;
const std::unique_ptr<std::chrono::steady_clock::time_point> _now = {};
const ogp::SetOfCallbacks _emptyCallbacks;


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


ogp::Goal _pddlGoal(const std::string& pStr,
                    const ogp::Ontology& pOntology,
                    int pMaxTimeToKeepInactive = -1,
                    const std::string& pGoalGroupId = "") {
  std::size_t pos = 0;
  return *ogp::pddlToGoal(pStr, pos, pOntology, {}, pMaxTimeToKeepInactive, pGoalGroupId);
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

bool _hasFact(ogp::WorldState& pWorldState,
              const std::string& pFactStr,
              const ogp::Ontology& pOntology) {
  return pWorldState.hasFact(_fact(pFactStr, pOntology));
}


void _addFact(ogp::WorldState& pWorldState,
              const std::string& pFactStr,
              ogp::GoalStack& pGoalStack,
              const ogp::Ontology& pOntology,
              const std::map<ogp::SetOfEventsId, ogp::SetOfEvents>& pSetOfEvents = _emptySetOfEvents,
              const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {}) {
  pWorldState.addFact(_fact(pFactStr, pOntology), pGoalStack, pSetOfEvents, _emptyCallbacks, pOntology, ogp::SetOfEntities(), pNow);
}



ogp::ActionInvocationWithGoal _lookForAnActionToDoThenNotify(
    ogp::Problem& pProblem,
    const ogp::Domain& pDomain,
    const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {})
{
  auto plan = ogp::planForMoreImportantGoalPossible(pProblem, pDomain, true, pNow);
  if (!plan.empty())
  {
    auto& firstActionInPlan = plan.front();
    notifyActionStarted(pProblem, pDomain, _emptyCallbacks, firstActionInPlan, pNow);
    notifyActionDone(pProblem, pDomain, _emptyCallbacks, firstActionInPlan, pNow);
    return firstActionInPlan;
  }
  return ogp::ActionInvocationWithGoal("", std::map<ogp::Parameter, ogp::Entity>(), {}, 0);
}



void _applyWhenEffectWithValidatedCondition()
{
  const std::string action1 = "action1";

  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr("fact_a\n"
                                                      "fact_b\n"
                                                      "fact_c", ontology.types);

  std::map<std::string, ogp::Action> actions;
  ogp::Action actionObj1({}, _worldStateModification_fromPddl("(and (fact_c) (when (fact_a) (fact_b)))", ontology));
  actions.emplace(action1, actionObj1);

  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_pddlGoal("(fact_c)", ontology)}, ontology.constants);
  _addFact(problem.worldState, "fact_a", problem.goalStack, ontology, setOfEventsMap, _now);
  EXPECT_EQ(action1, _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_TRUE(_hasFact(problem.worldState, "fact_b", ontology));
}

void _doNotApplyWhenEffectWithNotValidatedCondition()
{
  const std::string action1 = "action1";

  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr("fact_a\n"
                                                      "fact_b\n"
                                                      "fact_c", ontology.types);

  std::map<std::string, ogp::Action> actions;
  ogp::Action actionObj1({}, _worldStateModification_fromPddl("(and (fact_c) (when (fact_a) (fact_b)))", ontology));
  actions.emplace(action1, actionObj1);

  ogp::Domain domain(std::move(actions), ontology);
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_pddlGoal("(fact_c)", ontology)}, ontology.constants);
  EXPECT_EQ(action1, _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_FALSE(_hasFact(problem.worldState, "fact_b", ontology));
}


void _whenToSatisfyThenGoal()
{
  const std::string action1 = "action1";

  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr("fact_a\n"
                                                      "fact_b", ontology.types);

  std::map<std::string, ogp::Action> actions;
  ogp::Action actionObj1({}, _worldStateModification_fromPddl("(when (fact_a) (fact_b))", ontology));
  actions.emplace(action1, actionObj1);

  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_pddlGoal("(fact_b)", ontology)}, ontology.constants);
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  _addFact(problem.worldState, "fact_a", problem.goalStack, ontology, setOfEventsMap, _now);

  _setGoalsForAPriority(problem, {_pddlGoal("(fact_b)", ontology)}, ontology.constants);
  EXPECT_EQ(action1, _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
}


void _useWhenEffectInsideAPath()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";

  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr("fact_a\n"
                                                      "fact_b\n"
                                                      "fact_c", ontology.types);

  std::map<std::string, ogp::Action> actions;
  ogp::Action actionObj1({}, _worldStateModification_fromPddl("(when (fact_a) (fact_b))", ontology));
  actions.emplace(action1, actionObj1);
  actions.emplace(action2, ogp::Action(_condition_fromPddl("(fact_b)", ontology),
                                       _worldStateModification_fromPddl("(fact_c)", ontology)));

  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_pddlGoal("(fact_c)", ontology)}, ontology.constants);
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  _addFact(problem.worldState, "fact_a", problem.goalStack, ontology, setOfEventsMap, _now);

  _setGoalsForAPriority(problem, {_pddlGoal("(fact_c)", ontology)}, ontology.constants);
  EXPECT_EQ(action1, _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ(action2, _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
}


void _whenRemoveAFact()
{
  const std::string action1 = "action1";

  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr("fact_a\n"
                                                      "fact_b", ontology.types);

  std::map<std::string, ogp::Action> actions;
  ogp::Action actionObj1({}, _worldStateModification_fromPddl("(when (fact_a) (not fact_b))", ontology));
  actions.emplace(action1, actionObj1);

  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_pddlGoal("(not (fact_b))", ontology)}, ontology.constants);
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  _addFact(problem.worldState, "fact_a", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, "fact_b", problem.goalStack, ontology, setOfEventsMap, _now);

  _setGoalsForAPriority(problem, {_pddlGoal("(not (fact_b))", ontology)}, ontology.constants);
  EXPECT_EQ(action1, _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_FALSE(_hasFact(problem.worldState, "fact_b", ontology));
}

}



TEST(Planner, test_contionalEffectsRequirement)
{
  _applyWhenEffectWithValidatedCondition();
  _doNotApplyWhenEffectWithNotValidatedCondition();
  _whenToSatisfyThenGoal();
  _useWhenEffectInsideAPath();
  _whenRemoveAFact();
}
