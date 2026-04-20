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
  return ogp::Fact(pStr, true, pOntology, {}, pParameters);
}


ogp::Parameter _parameter(const std::string& pStr,
                          const ogp::Ontology& pOntology) {
  return ogp::Parameter::fromStr(pStr, pOntology.types);
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

void _addFact(ogp::WorldState& pWorldState,
              const std::string& pFactStr,
              ogp::GoalStack& pGoalStack,
              const ogp::Ontology& pOntology,
              const ogp::SetOfEntities& pObjects,
              const std::map<ogp::SetOfEventsId, ogp::SetOfEvents>& pSetOfEvents = _emptySetOfEvents,
              const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {}) {
  pWorldState.addFact(_fact(pFactStr, pOntology), pGoalStack, pSetOfEvents, _emptyCallbacks, pOntology, pObjects, pNow);
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


void _callActionToUpdateFactWithAFluentInParameter()
{
  const std::string action1 = "action1";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("t");
  ontology.predicates = ogp::SetOfPredicates::fromStr("fact_1(?p - t)\n"
                                                      "fact_2() - t", ontology.types);
  ontology.constants = ogp::SetOfEntities::fromPddl("a b - t", ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> actionParameters{_parameter("?p - t", ontology)};
  ogp::Action actionObj1({}, _worldStateModification_fromPddl("(fact_1 ?p)", ontology, actionParameters));
  actionObj1.parameters = std::move(actionParameters);
  actions.emplace(action1, actionObj1);

  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  _addFact(problem.worldState, "(= (fact_2) b)", problem.goalStack, ontology, problem.objects, setOfEventsMap);

  auto goalsStr = "(fact_1 (fact_2))";
  _setGoalsForAPriority(problem, {_pddlGoal(goalsStr, ontology, problem.objects)}, ontology.constants);
  EXPECT_EQ(action1 + "(?p -> b)", _lookForAnActionToDoThenNotify(problem, domain).actionInvocation.toStr());
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain).actionInvocation.toStr());
}


void _impossibleFactWithAFluentInParameter()
{
  const std::string action1 = "action1";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("t");
  ontology.predicates = ogp::SetOfPredicates::fromStr("fact_1(?p - t)\n"
                                                      "fact_2() - t", ontology.types);
  ontology.constants = ogp::SetOfEntities::fromPddl("a b - t", ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> actionParameters{_parameter("?p - t", ontology)};
  ogp::Action actionObj1({}, _worldStateModification_fromPddl("(fact_1 ?p)", ontology, actionParameters));
  actionObj1.parameters = std::move(actionParameters);
  actions.emplace(action1, actionObj1);

  ogp::Domain domain(std::move(actions), ontology);
  ogp::Problem problem;

  auto goalsStr = "(fact_1 (fact_2))";
  _setGoalsForAPriority(problem, {_pddlGoal(goalsStr, ontology, problem.objects)}, ontology.constants);
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain).actionInvocation.toStr());
}



}



TEST(Planner, test_fluentInParameter)
{
  _callActionToUpdateFactWithAFluentInParameter();
  _impossibleFactWithAFluentInParameter();
}
