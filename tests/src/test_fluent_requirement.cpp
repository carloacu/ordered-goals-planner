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

bool _isTrue(const std::string& pConditionPddlStr,
             const ogp::Ontology& pOntology,
             const ogp::WorldState& pWorldState,
             const ogp::SetOfEntities& pObjects)
{
  std::size_t pos = 0;
  return ogp::pddlToCondition(pConditionPddlStr, pos, pOntology, {}, {})->isTrue(pWorldState, pOntology.constants, pObjects, {}, {});
}

ogp::Fact _fact(const std::string& pStr,
                const ogp::Ontology& pOntology,
                const ogp::SetOfEntities& pObjects,
                const std::vector<ogp::Parameter>& pParameters = {}) {
  return ogp::Fact(pStr, true, pOntology, pObjects, pParameters);
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


void _addFact(ogp::WorldState& pWorldState,
              const std::string& pFactStr,
              ogp::GoalStack& pGoalStack,
              const ogp::Ontology& pOntology,
              const ogp::SetOfEntities& pObjects,
              const std::map<ogp::SetOfEventsId, ogp::SetOfEvents>& pSetOfEvents = _emptySetOfEvents,
              const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {}) {
  pWorldState.addFact(_fact(pFactStr, pOntology, pObjects), pGoalStack, pSetOfEvents, _emptyCallbacks, pOntology, pObjects, pNow);
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



void _test_set_a_fluent_value_to_undefined()
{
  const std::string action1 = "action1";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity");
  ontology.constants = ogp::SetOfEntities::fromPddl("v - entity", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("fact_a - entity", ontology.types);

  std::map<std::string, ogp::Action> actions;
  ogp::Action actionObj1({}, _worldStateModification_fromPddl("(assign fact_a undefined)", ontology));
  actions.emplace(action1, actionObj1);

  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  _addFact(problem.worldState, "(= (fact_a) v)", problem.goalStack, ontology, problem.objects, setOfEventsMap, _now);
  _setGoalsForAPriority(problem, {_pddlGoal("(= (fact_a) undefined)", ontology)}, ontology.constants);
  EXPECT_EQ(action1, _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
}


void _test_set_a_fluent_value_to_undefined_with_sub_types()
{
  const std::string action1 = "action1";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("type - entity\n"
                                             "sub_type - type");
  ontology.constants = ogp::SetOfEntities::fromPddl("v - sub_type", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("fact_a - entity", ontology.types);

  std::map<std::string, ogp::Action> actions;
  ogp::Action actionObj1({}, _worldStateModification_fromPddl("(assign fact_a undefined)", ontology));
  actions.emplace(action1, actionObj1);

  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  _addFact(problem.worldState, "(= (fact_a) v)", problem.goalStack, ontology, problem.objects, setOfEventsMap, _now);
  _setGoalsForAPriority(problem, {_pddlGoal("(= (fact_a) undefined)", ontology)}, ontology.constants);
  EXPECT_EQ(action1, _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
}


void _test_fluent_for_location_with_sub_types()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("location\n"
                                             "robot checkpoint - physical_object\n"
                                             "charging_zone - checkpoint");
  ontology.constants = ogp::SetOfEntities::fromPddl("self - robot", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("declared_location(?po - physical_object) - location\n"
                                                      "goal", ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> actionParameters{_parameter("?cp - checkpoint", ontology)};
  ogp::Action actionObj1({}, _worldStateModification_fromPddl("(assign (declared_location self) (declared_location ?cp))", ontology, actionParameters));
  actionObj1.parameters = std::move(actionParameters);
  actions.emplace(action1, actionObj1);

  std::vector<ogp::Parameter> action2Parameters{_parameter("?cz - charging_zone", ontology)};
  ogp::Action actionObj2(_condition_fromPddl("(= (declared_location self) (declared_location ?cz))", ontology, action2Parameters),
                         _worldStateModification_fromPddl("(goal)", ontology, action2Parameters));
  actionObj2.parameters = std::move(action2Parameters);
  actions.emplace(action2, actionObj2);

  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  problem.objects.add(ogp::Entity::fromDeclaration("charging_zone_loc - location", ontology.types));
  problem.objects.add(ogp::Entity::fromDeclaration("a_checkpoint_loc - location", ontology.types));
  problem.objects.add(ogp::Entity::fromDeclaration("a_checkpoint - checkpoint", ontology.types));
  problem.objects.add(ogp::Entity::fromDeclaration("pod - charging_zone", ontology.types));
  _addFact(problem.worldState, "(= (declared_location a_checkpoint) a_checkpoint_loc)", problem.goalStack, ontology, problem.objects, setOfEventsMap, _now);
  _addFact(problem.worldState, "(= (declared_location pod) charging_zone_loc)", problem.goalStack, ontology, problem.objects, setOfEventsMap, _now);

  _setGoalsForAPriority(problem, {_pddlGoal("(goal)", ontology)}, ontology.constants);
  EXPECT_EQ(action1 + "(?cp -> pod)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ(action2 + "(?cz -> pod)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
}

}



TEST(Planner, test_fluent_requirement)
{
  _test_set_a_fluent_value_to_undefined();
  _test_set_a_fluent_value_to_undefined_with_sub_types();
  _test_fluent_for_location_with_sub_types();
}