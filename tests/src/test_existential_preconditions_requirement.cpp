#include <gtest/gtest.h>
#include <orderedgoalsplanner/orderedgoalsplanner.hpp>
#include <orderedgoalsplanner/types/setofcallbacks.hpp>
#include <orderedgoalsplanner/types/setofevents.hpp>
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


static const std::vector<ogp::Parameter> _emptyParameters;


ogp::Fact _fact(const std::string& pStr,
                const ogp::Ontology& pOntology,
                const ogp::SetOfEntities& pObjects,
                const std::vector<ogp::Parameter>& pParameters = {}) {
  return ogp::Fact(pStr, false, pOntology, pObjects, pParameters);
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

ogp::Goal _pddlGoal(const std::string& pStr,
                    const ogp::Ontology& pOntology,
                    const ogp::SetOfEntities& pObjects,
                    int pMaxTimeToKeepInactive = -1,
                    const std::string& pGoalGroupId = "") {
  std::size_t pos = 0;
  return *ogp::pddlToGoal(pStr, pos, pOntology, pObjects, pMaxTimeToKeepInactive, pGoalGroupId);
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
              const std::map<ogp::SetOfEventsId, ogp::SetOfEvents>& pSetOfEvents = _emptySetOfEvents,
              const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {},
              const ogp::SetOfEntities& pObjects = ogp::SetOfEntities()) {
  pWorldState.addFact(_fact(pFactStr, pOntology, pObjects), pGoalStack, pSetOfEvents, _emptyCallbacks, pOntology, pObjects, pNow);
}


void _removeFact(ogp::WorldState& pWorldState,
                 const std::string& pFactStr,
                 ogp::GoalStack& pGoalStack,
                 const ogp::Ontology& pOntology,
                 const std::map<ogp::SetOfEventsId, ogp::SetOfEvents>& pSetOfEvents = _emptySetOfEvents,
                 const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {},
                 const ogp::SetOfEntities& pObjects = ogp::SetOfEntities()) {
  pWorldState.removeFact(_fact(pFactStr, pOntology, pObjects), pGoalStack, pSetOfEvents, _emptyCallbacks, pOntology, pObjects, pNow);
}

bool _hasFact(ogp::WorldState& pWorldState,
              const std::string& pFactStr,
              const ogp::Ontology& pOntology,
              const ogp::SetOfEntities& pObjects = ogp::SetOfEntities()) {
  return pWorldState.hasFact(_fact(pFactStr, pOntology, pObjects));
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

ogp::ActionInvocationWithGoal _lookForAnActionToDoConst(const ogp::Problem& pProblem,
                                                        const ogp::Domain& pDomain,
                                                        const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {},
                                                        const ogp::Historical* pGlobalHistorical = nullptr)
{
  auto problem = pProblem;
  auto plan = ogp::planForMoreImportantGoalPossible(problem, pDomain, true, pNow, pGlobalHistorical);
  if (!plan.empty())
    return plan.front();
  return ogp::ActionInvocationWithGoal("", std::map<ogp::Parameter, ogp::Entity>(), {}, 0);
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





void _checkSimpleExists()
{
  const std::string action1 = "action1";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("location");
  ontology.constants = ogp::SetOfEntities::fromPddl("kitchen - location", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + "(?l - location)\n" +
                                                      _fact_b, ontology.types);

  std::map<std::string, ogp::Action> actions;
  ogp::Action actionObj1(_condition_fromStr("exists(?l - location, " + _fact_a + "(?l))", ontology),
                         _worldStateModification_fromStr(_fact_b, ontology));
  actions.emplace(action1, actionObj1);

  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal(_fact_b, ontology)}, ontology.constants);

  EXPECT_EQ("", _lookForAnActionToDoConst(problem, domain, _now).actionInvocation.toStr());
  _addFact(problem.worldState, _fact_a + "(kitchen)", problem.goalStack, ontology, setOfEventsMap, _now);
  EXPECT_EQ(action1, _lookForAnActionToDo(problem, domain, _now).actionInvocation.toStr());
}


void _existsCondition()
{
  const std::string action1 = "action1";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("t1\n"
                                             "t2");
  ontology.constants = ogp::SetOfEntities::fromPddl("v1a v1b - t1\n"
                                                    "v2a - t2\n", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("fact_a(?t - t1)\n"
                                                      "fact_b", ontology.types);

  std::map<std::string, ogp::Action> actions;
  ogp::Action actionObj1(_condition_fromPddl("(exists (?t - t1) (fact_a ?t))", ontology),
                         _worldStateModification_fromPddl("(fact_b)", ontology));
  actions.emplace(action1, actionObj1);

  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_pddlGoal("(fact_b)", ontology, {})}, ontology.constants);
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());

  _addFact(problem.worldState, "fact_a(v1a)", problem.goalStack, ontology, setOfEventsMap, _now);
  _setGoalsForAPriority(problem, {_pddlGoal("(fact_b)", ontology, {})}, ontology.constants);
  EXPECT_EQ(action1, _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  _removeFact(problem.worldState, "fact_b", problem.goalStack, ontology, setOfEventsMap, _now);

  _addFact(problem.worldState, "fact_a(v1b)", problem.goalStack, ontology, setOfEventsMap, _now);
  _setGoalsForAPriority(problem, {_pddlGoal("(fact_b)", ontology, {})}, ontology.constants);
  EXPECT_EQ(action1, _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
}



void _checkExistsInGoalThatCanBeSatisfied()
{
  const std::string action1 = "action1";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("t1\n"
                                             "t2");
  ontology.constants = ogp::SetOfEntities::fromPddl("v1a v1b v1c - t1\n"
                                                    "v2a - t2\n", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + "(?v1 - t1) - t2\n" +
                                                      _fact_b + "(?v1 - t1)", ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> actionParameters{_parameter("?v1 - t1", ontology), _parameter("?v2 - t2", ontology)};
  ogp::Action actionObj1(_condition_fromStr(_fact_b + "(?v1)", ontology, actionParameters),
                         _worldStateModification_fromStr(_fact_a + "(?v1)=?v2", ontology, actionParameters));
  actionObj1.parameters = std::move(actionParameters);
  actions.emplace(action1, actionObj1);

  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_pddlGoal("(exists (?v1 - t1) (= (" + _fact_a + " ?v1) v2a))", ontology, problem.objects)}, ontology.constants);
  _addFact(problem.worldState, _fact_b + "(v1b)", problem.goalStack, ontology, setOfEventsMap, _now);
  EXPECT_EQ("action1(?v1 -> v1b, ?v2 -> v2a)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
}



void _checkExistsInGoalWithSubType()
{
  const std::string action1 = "action1";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("sub_entity - entity");
  ontology.constants = ogp::SetOfEntities::fromPddl("v1a - sub_entity", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + " - entity", ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> actionParameters{_parameter("?p - entity", ontology)};
  ogp::Action actionObj1({}, _worldStateModification_fromStr(_fact_a + "=?p", ontology, actionParameters));
  actionObj1.parameters = std::move(actionParameters);
  actions.emplace(action1, actionObj1);

  ogp::Domain domain(std::move(actions), ontology);
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_pddlGoal("(exists (?v - sub_entity) (= (" + _fact_a + ") ?v))", ontology, problem.objects)}, ontology.constants);
  EXPECT_EQ("action1(?p -> v1a)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
}


void _checkExistsInGoalWithSubTypeAndAnotherConstantWithParentType()
{
  const std::string action1 = "action1";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("sub_entity sub_entity2 - entity");
  ontology.constants = ogp::SetOfEntities::fromPddl("v1a - sub_entity\n"
                                                    "vent - sub_entity2", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + " - entity", ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> actionParameters{_parameter("?p - entity", ontology)};
  ogp::Action actionObj1({}, _worldStateModification_fromStr(_fact_a + "=?p", ontology, actionParameters));
  actionObj1.parameters = std::move(actionParameters);
  actions.emplace(action1, actionObj1);

  ogp::Domain domain(std::move(actions), ontology);
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_pddlGoal("(exists (?v - sub_entity) (= (" + _fact_a + ") ?v))", ontology, problem.objects)}, ontology.constants);
  EXPECT_EQ("action1(?p -> v1a)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
}


void _checkExistsInGoalWithListExpression()
{
  const std::string action1 = "action1";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("t1\n"
                                             "t2");
  ontology.constants = ogp::SetOfEntities::fromPddl("v1a v1b v1c - t1\n"
                                                    "v2a - t2\n", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + "(?v1 - t1) - t2\n" +
                                                      _fact_b + "(?v1 - t1)", ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> actionParameters{_parameter("?v1 - t1", ontology), _parameter("?v2 - t2", ontology)};
  ogp::Action actionObj1({}, _worldStateModification_fromStr(_fact_a + "(?v1)=?v2", ontology, actionParameters));
  actionObj1.parameters = std::move(actionParameters);
  actions.emplace(action1, actionObj1);

  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_pddlGoal("(exists (?v1 - t1) (and (" + _fact_b + " ?v1) (= (" + _fact_a + " ?v1) v2a)))", ontology, problem.objects)}, ontology.constants);
  _addFact(problem.worldState, _fact_b + "(v1b)", problem.goalStack, ontology, setOfEventsMap, _now);
  EXPECT_EQ("action1(?v1 -> v1b, ?v2 -> v2a)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
}


void _checkExistsInGoalThatCannotBeSatisfied()
{
  const std::string action1 = "action1";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("t1\n"
                                             "t2");
  ontology.constants = ogp::SetOfEntities::fromPddl("v1a v1b v1c - t1\n"
                                                    "v2a - t2\n", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + "(?v1 - t1) - t2", ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> actionParameters{_parameter("?v1 - t1", ontology), _parameter("?v2 - t2", ontology)};
  ogp::Action actionObj1({}, _worldStateModification_fromStr(_fact_a + "(?v1)=?v2", ontology, actionParameters));
  actionObj1.parameters = std::move(actionParameters);
  actions.emplace(action1, actionObj1);

  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_pddlGoal("(exists (?v1 - t1) (= (" + _fact_a + " ?v1) undefined))", ontology, problem.objects)}, ontology.constants);
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());

  _setGoalsForAPriority(problem, {_pddlGoal("(exists (?v1 - t1) (= (" + _fact_a + " ?v1) undefined))", ontology, problem.objects)}, ontology.constants);
  _addFact(problem.worldState, _fact_a + "(v1b)=v2a", problem.goalStack, ontology, setOfEventsMap, _now);
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
}

void _checkExistsWithActionParameterInvolved()
{
  const std::string action1 = "action1";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity\n"
                                             "location");
  ontology.constants = ogp::SetOfEntities::fromPddl("pen - entity\n"
                                                    "kitchen - location", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + "(?l - location, ?e - entity)\n" +
                                                      _fact_b, ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> actionParameters{_parameter("?obj - entity", ontology)};
  ogp::Action actionObj1(_condition_fromStr("exists(?l - location, " + _fact_a + "(?l, ?obj))", ontology, actionParameters),
                         _worldStateModification_fromStr(_fact_b, ontology));
  actionObj1.parameters = std::move(actionParameters);
  actions.emplace(action1, actionObj1);

  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal(_fact_b, ontology)}, ontology.constants);

  EXPECT_EQ("", _lookForAnActionToDoConst(problem, domain, _now).actionInvocation.toStr());
  _addFact(problem.worldState, _fact_a + "(kitchen, pen)", problem.goalStack, ontology, setOfEventsMap, _now);
  EXPECT_EQ(action1 + "(?obj -> pen)", _lookForAnActionToDo(problem, domain, _now).actionInvocation.toStr());
}


void _checkExistsWithManyFactsInvolved()
{
  const std::string action1 = "action1";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity\n"
                                             "location\n"
                                             "robot");
  ontology.constants = ogp::SetOfEntities::fromPddl("self - robot\n"
                                                    "bottle mouse pen - entity\n"
                                                    "bedroom kitchen livingroom - location", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + "(?r - robot, ?l - location)\n" +
                                                      _fact_b + "(?e - entity, ?l - location)\n" +
                                                      _fact_c, ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> actionParameters{_parameter("?obj - entity", ontology)};
  ogp::Action actionObj1(_condition_fromStr("exists(?l - location, " + _fact_a + "(self, ?l) & " + _fact_b + "(?obj, ?l))", ontology, actionParameters),
                         _worldStateModification_fromStr(_fact_c, ontology, actionParameters));
  actionObj1.parameters = std::move(actionParameters);
  actions.emplace(action1, actionObj1);

  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal(_fact_c, ontology)}, ontology.constants);

  _addFact(problem.worldState, _fact_b + "(mouse, bedroom)", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_b + "(bottle, kitchen)", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_b + "(pen, livingroom)", problem.goalStack, ontology, setOfEventsMap, _now);
  EXPECT_EQ("", _lookForAnActionToDoConst(problem, domain, _now).actionInvocation.toStr());
  _addFact(problem.worldState, _fact_a + "(self, kitchen)", problem.goalStack, ontology, setOfEventsMap, _now);
  EXPECT_EQ(action1 + "(?obj -> bottle)", _lookForAnActionToDo(problem, domain, _now).actionInvocation.toStr());
}


void _doAnActionToSatisfyAnExists()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity\n"
                                             "location\n"
                                             "robot");
  ontology.constants = ogp::SetOfEntities::fromPddl("self - robot\n"
                                                    "bottle mouse pen - entity\n"
                                                    "bedroom kitchen livingroom - location", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + "(?r - robot, ?l - location)\n" +
                                                      _fact_b + "(?e - entity, ?l - location)\n" +
                                                      _fact_c + "(?e - entity)", ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> act1Parameters{_parameter("?obj - entity", ontology)};
  ogp::Action actionObj1(_condition_fromStr("exists(?l - location, " + _fact_a + "(self, ?l) & " + _fact_b + "(?obj, ?l))", ontology, act1Parameters),
                         _worldStateModification_fromStr(_fact_c + "(?obj)", ontology, act1Parameters));
  actionObj1.parameters = std::move(act1Parameters);
  actions.emplace(action1, actionObj1);

  std::vector<ogp::Parameter> act2Parameters{_parameter("?loc - location", ontology)};
  ogp::Action actionObj2({}, _worldStateModification_fromStr(_fact_a + "(self, ?loc)", ontology, act2Parameters));
  actionObj2.parameters = std::move(act2Parameters);
  actions.emplace(action2, actionObj2);

  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal(_fact_c + "(mouse)", ontology)}, ontology.constants);

  _addFact(problem.worldState, _fact_b + "(bottle, kitchen)", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_b + "(mouse, bedroom)", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_b + "(pen, livingroom)", problem.goalStack, ontology, setOfEventsMap, _now);
  EXPECT_EQ(action2 + "(?loc -> bedroom)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ(action1 + "(?obj -> mouse)", _lookForAnActionToDo(problem, domain, _now).actionInvocation.toStr());
}


void _checkForAllEffectAtStart()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity\n"
                                             "location\n"
                                             "robot");
  ontology.constants = ogp::SetOfEntities::fromPddl("self - robot\n"
                                                    "bottle mouse pen - entity\n"
                                                    "bedroom entrance kitchen livingroom - location", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + "(?r - robot, ?l - location)\n" +
                                                      _fact_b + "(?e - entity, ?l - location)\n" +
                                                      _fact_c + "(?e - entity)", ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> act1Parameters{_parameter("?obj - entity", ontology)};
  ogp::Action actionObj1(_condition_fromStr("exists(?l - location, " + _fact_a + "(self, ?l) & " + _fact_b + "(?obj, ?l))", ontology, act1Parameters),
                         _worldStateModification_fromStr(_fact_c + "(?obj)", ontology, act1Parameters));
  actionObj1.parameters = std::move(act1Parameters);
  actions.emplace(action1, actionObj1);

  std::vector<ogp::Parameter> act2Parameters{_parameter("?loc - location", ontology)};
  ogp::Action actionObj2({}, _worldStateModification_fromStr(_fact_a + "(self, ?loc)", ontology, act2Parameters));
  actionObj2.effect.worldStateModificationAtStart = _worldStateModification_fromStr("forall(?l - location, when(" + _fact_a + "(self, ?l), not(" + _fact_a + "(self, ?l))))", ontology, act2Parameters);
  actionObj2.parameters = std::move(act2Parameters);
  actions.emplace(action2, actionObj2);

  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal(_fact_c + "(mouse)", ontology)}, ontology.constants);

  _addFact(problem.worldState, _fact_a + "(self, entrance)", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_b + "(bottle, kitchen)", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_b + "(mouse, bedroom)", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_b + "(pen, livingroom)", problem.goalStack, ontology, setOfEventsMap, _now);
  EXPECT_TRUE(_hasFact(problem.worldState, _fact_a + "(self, entrance)", ontology));
  EXPECT_EQ(action2 + "(?loc -> bedroom)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_FALSE(_hasFact(problem.worldState, _fact_a + "(self, entrance)", ontology)); // removed by the effect at start of action2
  EXPECT_EQ(action1 + "(?obj -> mouse)", _lookForAnActionToDo(problem, domain, _now).actionInvocation.toStr());
}


void _existsWithValue()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity\n"
                                             "location\n"
                                             "robot");
  ontology.constants = ogp::SetOfEntities::fromPddl("self - robot\n"
                                                    "pen - entity\n"
                                                    "livingroom - location", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + "(?r - robot) - location\n" +
                                                      _fact_b + "(?e - entity) - location\n" +
                                                      _fact_c + "(?e - entity)", ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> act1Parameters{_parameter("?obj - entity", ontology)};
  ogp::Action actionObj1(_condition_fromStr("exists(?l - location, " + _fact_a + "(self)=?l & " + _fact_b + "(?obj)=?l)", ontology, act1Parameters),
                         _worldStateModification_fromStr(_fact_c + "(?obj)", ontology, act1Parameters));
  actionObj1.parameters = std::move(act1Parameters);
  actions.emplace(action1, actionObj1);

  std::vector<ogp::Parameter> act2Parameters{_parameter("?loc - location", ontology)};
  ogp::Action actionObj2({}, _worldStateModification_fromStr(_fact_a + "(self)=?loc", ontology, act2Parameters));
  actionObj2.parameters = std::move(act2Parameters);
  actions.emplace(action2, actionObj2);

  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal(_fact_c + "(pen)", ontology)}, ontology.constants);

  _addFact(problem.worldState, _fact_b + "(pen)=livingroom", problem.goalStack, ontology, setOfEventsMap, _now);
  EXPECT_EQ(action2 + "(?loc -> livingroom)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ(action1 + "(?obj -> pen)", _lookForAnActionToDo(problem, domain, _now).actionInvocation.toStr());
}


void _notExists()
{
  const std::string action1 = "action1";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("location\n"
                                             "robot");
  ontology.constants = ogp::SetOfEntities::fromPddl("self - robot\n"
                                                    "kitchen - location", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + "(?r - robot, ?l - location)\n" +
                                                      _fact_b, ontology.types);

  std::map<std::string, ogp::Action> actions;
  ogp::Action actionObj1(_condition_fromStr("not(exists(?l - location, " + _fact_a + "(self, ?l)))", ontology),
                         _worldStateModification_fromStr(_fact_b, ontology));
  actions.emplace(action1, actionObj1);

  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal(_fact_b, ontology)}, ontology.constants);

  EXPECT_EQ(action1, _lookForAnActionToDo(problem, domain, _now).actionInvocation.toStr());
  _addFact(problem.worldState, _fact_a + "(self, kitchen)", problem.goalStack, ontology, setOfEventsMap, _now);
  EXPECT_EQ("", _lookForAnActionToDo(problem, domain, _now).actionInvocation.toStr());
}


void _actionToSatisfyANotExists()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  const std::string action3 = "action3";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("location\n"
                                             "robot\n"
                                             "resource");
  ontology.constants = ogp::SetOfEntities::fromPddl("self - robot\n"
                                                    "kitchen - location\n"
                                                    "spec_rec - resource", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + "(?r - robot, ?l - location)\n" +
                                                      _fact_b + "\n" +
                                                      "busy(?r - resource)", ontology.types);

  std::map<std::string, ogp::Action> actions;
  ogp::Action actionObj1(_condition_fromStr("not(busy(spec_rec)) & not(exists(?l - location, " + _fact_a + "(self, ?l)))", ontology),
                         _worldStateModification_fromStr("not(busy(spec_rec)) & " + _fact_b, ontology));
  actions.emplace(action1, actionObj1);

  std::vector<ogp::Parameter> act2Parameters{_parameter("?loc - location", ontology)};
  ogp::Action actionObj2({}, _worldStateModification_fromStr("!" + _fact_a + "(self, ?loc)", ontology, act2Parameters));
  actionObj2.parameters = std::move(act2Parameters);
  actions.emplace(action2, actionObj2);

  std::vector<ogp::Parameter> act3Parameters{_parameter("?r - resource", ontology)};
  ogp::Action actionObj3({}, _worldStateModification_fromStr("not(busy(?r))", ontology, act3Parameters));
  actionObj3.parameters = std::move(act3Parameters);
  actions.emplace(action3, actionObj3);

  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal(_fact_b, ontology)}, ontology.constants);
  _addFact(problem.worldState, "busy(spec_rec)", problem.goalStack, ontology, setOfEventsMap, _now);

  EXPECT_EQ(action3 + "(?r -> spec_rec)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  _addFact(problem.worldState, _fact_a + "(self, kitchen)", problem.goalStack, ontology, setOfEventsMap, _now);
  EXPECT_EQ(action2 + "(?loc -> kitchen)", _lookForAnActionToDo(problem, domain, _now).actionInvocation.toStr());
}

void _existsInsideAPath()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("t1");
  ontology.constants = ogp::SetOfEntities::fromPddl("v1a v1b v1c - t1", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("fact_a(?t - t1)\n"
                                                      "fact_b", ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> actionParameters{_parameter("?v1 - t1", ontology)};
  ogp::Action actionObj1({}, _worldStateModification_fromPddl("(fact_a ?v1)", ontology, actionParameters));
  actionObj1.parameters = std::move(actionParameters);
  actions.emplace(action1, actionObj1);

  actions.emplace(action2, ogp::Action(_condition_fromPddl("(exists (?t - t1) (fact_a ?t))", ontology),
                                       _worldStateModification_fromPddl("(fact_b)", ontology)));

  ogp::Domain domain(std::move(actions), ontology);
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_pddlGoal("(fact_b)", ontology, problem.objects)}, ontology.constants);
  EXPECT_EQ("action1(?v1 -> v1a)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ("action2", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
}


void _checkExistsInGoalWithAndListWithANotStatement()
{
  const std::string action1 = "action1";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("user");
  ontology.predicates = ogp::SetOfPredicates::fromStr("pred_1(?u - user)\n"
                                                      "pred_2(?u - user)", ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> actionParameters{_parameter("?u - user", ontology)};
  ogp::Action actionObj1({}, _worldStateModification_fromPddl("(pred_1 ?u)", ontology, actionParameters));
  actionObj1.parameters = std::move(actionParameters);
  actions.emplace(action1, actionObj1);

  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  problem.objects = ogp::SetOfEntities::fromPddl("u1 u2 - user", ontology.types);
  _addFact(problem.worldState, "pred_2(u2)", problem.goalStack, ontology, setOfEventsMap, _now, problem.objects);

  _setGoalsForAPriority(problem, {_pddlGoal("(exists (?u - user) (and (pred_1 ?u) (not (pred_2 ?u))))", ontology, problem.objects)}, ontology.constants);
  EXPECT_EQ("action1(?u -> u1)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
}



void _existWithFluentValuesEquality()
{
  const std::string action1 = "action1";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity");
  ontology.predicates = ogp::SetOfPredicates::fromStr("fact_1(?p - entity) - entity\n"
                                                      "fact_2(?p - entity) - entity", ontology.types);
  ontology.constants = ogp::SetOfEntities::fromPddl("v1 v2 v3 v4 - entity", ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> actionParameters{_parameter("?p1 - entity", ontology), _parameter("?p2 - entity", ontology)};
  ogp::Action actionObj1({}, _worldStateModification_fromPddl("(assign (fact_1 ?p1) ?p2)", ontology, actionParameters));
  actionObj1.parameters = std::move(actionParameters);
  actions.emplace(action1, actionObj1);

  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  _addFact(problem.worldState, "fact_2(v3)=v4", problem.goalStack, ontology, setOfEventsMap, _now, problem.objects);


  _setGoalsForAPriority(problem, {_pddlGoal("(exists (?e - entity) (= (fact_1 ?e) (fact_2 ?e)))", ontology, problem.objects)}, ontology.constants);
  EXPECT_EQ("action1(?p1 -> v3, ?p2 -> v4)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
}



void _existWithImplyAndFluentValuesEquality()
{
  const std::string action1 = "action1";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity return_type");
  ontology.predicates = ogp::SetOfPredicates::fromStr("fact_1(?p - entity) - return_type\n"
                                                      "fact_2(?p - entity) - return_type\n"
                                                      "false", ontology.types);
  ontology.constants = ogp::SetOfEntities::fromPddl("r1 r2 - return_type", ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> actionParameters{_parameter("?p1 - entity", ontology), _parameter("?p2 - return_type", ontology)};
  ogp::Action actionObj1({}, _worldStateModification_fromPddl("(assign (fact_1 ?p1) ?p2)", ontology, actionParameters));
  actionObj1.parameters = std::move(actionParameters);
  actions.emplace(action1, actionObj1);

  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  problem.objects = ogp::SetOfEntities::fromPddl("v1 v2 - entity", ontology.types);
  _addFact(problem.worldState, "fact_2(v1)=r2", problem.goalStack, ontology, setOfEventsMap, _now, problem.objects);

  _setGoalsForAPriority(problem, {_pddlGoal("(exists (?e - entity) (imply (= (fact_2 ?e) r2) (= (fact_1 ?e) (fact_2 ?e))))", ontology, problem.objects)}, ontology.constants);
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());

  problem.objects = ogp::SetOfEntities::fromPddl("v1 - entity", ontology.types);
  _setGoalsForAPriority(problem, {_pddlGoal("(exists (?e - entity) (imply (= (fact_2 ?e) r2) (= (fact_1 ?e) (fact_2 ?e))))", ontology, problem.objects)}, ontology.constants);
  EXPECT_EQ("action1(?p1 -> v1, ?p2 -> r2)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
}


}



TEST(Planner, test_existentialPreconditionsRequirement)
{
  _checkSimpleExists();
  _existsCondition();
  _checkExistsInGoalThatCanBeSatisfied();
  _checkExistsInGoalWithSubType();
  _checkExistsInGoalWithSubTypeAndAnotherConstantWithParentType();
  _checkExistsInGoalWithListExpression();
  _checkExistsInGoalThatCannotBeSatisfied();
  _checkExistsWithActionParameterInvolved();
  _checkExistsWithManyFactsInvolved();
  _doAnActionToSatisfyAnExists();
  _checkForAllEffectAtStart();
  _existsWithValue();
  _notExists();
  _actionToSatisfyANotExists();
  _existsInsideAPath();
  _checkExistsInGoalWithAndListWithANotStatement();
  _existWithFluentValuesEquality();
  _existWithImplyAndFluentValuesEquality();
}
