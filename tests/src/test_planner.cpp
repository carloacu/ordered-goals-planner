#include <gtest/gtest.h>
#include <orderedgoalsplanner/orderedgoalsplanner.hpp>
#include <orderedgoalsplanner/types/predicate.hpp>
#include <orderedgoalsplanner/types/setofcallbacks.hpp>
#include <orderedgoalsplanner/types/setofconstfacts.hpp>
#include <orderedgoalsplanner/types/setofevents.hpp>
#include <orderedgoalsplanner/util/serializer/deserializefrompddl.hpp>
#include <orderedgoalsplanner/util/trackers/goalsremovedtracker.hpp>
#include <orderedgoalsplanner/util/print.hpp>
#include <orderedgoalsplanner/util/serializer/serializeinpddl.hpp>
#include "docexamples/test_planningDummyExample.hpp"
#include "docexamples/test_planningExampleWithAPreconditionSolve.hpp"


namespace
{
const auto _now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
const std::map<ogp::SetOfEventsId, ogp::SetOfEvents> _emptySetOfEvents;
const std::string _fact_a = "fact_a";
const std::string _fact_b = "fact_b";
const std::string _fact_c = "fact_c";
const ogp::SetOfCallbacks _emptyCallbacks;


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
                                                   const ogp::SetOfCallbacks& pCallbacks = {},
                                                   const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {},
                                                   const ogp::Historical* pGlobalHistorical = nullptr)
{
  auto plan = ogp::planForMoreImportantGoalPossible(pProblem, pDomain, pCallbacks, true, pNow, pGlobalHistorical);
  if (!plan.empty())
    return plan.front();
  return ogp::ActionInvocationWithGoal("", std::map<ogp::Parameter, ogp::Entity>(), {}, 0);
}

ogp::Goal _pddlGoal(const std::string& pStr,
                    const ogp::Ontology& pOntology,
                    const ogp::SetOfEntities& pObjects,
                    int pMaxTimeToKeepInactive = -1,
                    const std::string& pGoalGroupId = "") {
  std::size_t pos = 0;
  return *ogp::pddlToGoal(pStr, pos, pOntology, pObjects, pMaxTimeToKeepInactive, pGoalGroupId);
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


void _simplest_plan_possible()
{
  const std::string action1 = "action1";
  std::map<std::string, ogp::Action> actions;

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("type1 type2 - entity");
  ontology.constants = ogp::SetOfEntities::fromPddl("toto - type1\n"
                                                    "titi - type2", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("pred_a(?e - entity)\n"
                                                      "pred_b\n", ontology.types);

  const ogp::SetOfEntities entities;

  std::vector<ogp::Parameter> parameters(1, ogp::Parameter::fromStr("?pa - type1", ontology.types));
  ogp::Action actionObj1(ogp::strToCondition("pred_a(?pa)", ontology, entities, parameters),
                         ogp::strToWsModification("pred_b", ontology, entities, parameters));
  actionObj1.parameters = std::move(parameters);
  actions.emplace(action1, actionObj1);

  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {ogp::Goal::fromStr("pred_b", ontology, entities)}, ontology.constants);
  problem.worldState.addFact(ogp::Fact("pred_a(toto)", false, ontology, entities, {}), problem.goalStack, setOfEventsMap,
                             _emptyCallbacks, ontology, entities, _now);

  EXPECT_EQ("action1(?pa -> toto)", _lookForAnActionToDo(problem, domain).actionInvocation.toStr());
}



void _wrong_condition_type()
{
  const std::string action1 = "action1";
  std::map<std::string, ogp::Action> actions;

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity\n"
                                             "type1 - entity\n"
                                             "type2 - entity");
  ontology.constants = ogp::SetOfEntities::fromPddl("toto - type1\n"
                                                    "titi - type2", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("pred_a(?e - entity)\n"
                                                      "pred_b\n", ontology.types);

  const ogp::SetOfEntities entities;

  std::vector<ogp::Parameter> parameters(1, ogp::Parameter::fromStr("?pa - type1", ontology.types));
  ogp::Action actionObj1(ogp::strToCondition("pred_a(?pa)", ontology, entities, parameters),
                         ogp::strToWsModification("pred_b", ontology, entities, parameters));
  actionObj1.parameters = std::move(parameters);
  actions.emplace(action1, actionObj1);

  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {ogp::Goal::fromStr("pred_b", ontology, entities)}, ontology.constants);
  problem.worldState.addFact(ogp::Fact("pred_a(titi)", false, ontology, entities, {}), problem.goalStack, setOfEventsMap,
                             _emptyCallbacks, ontology, entities, _now);

  EXPECT_EQ("", _lookForAnActionToDo(problem, domain).actionInvocation.toStr());
}


void _number_type()
{
  const std::string action1 = "action1";
  std::map<std::string, ogp::Action> actions;

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity");
  ontology.constants = ogp::SetOfEntities::fromPddl("toto - entity", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("pred_a(?e - entity) - number\n"
                                                      "pred_b", ontology.types);

  const ogp::SetOfEntities entities;

  ogp::Action actionObj1(ogp::strToCondition("pred_a(toto)=10", ontology, entities, {}),
                         ogp::strToWsModification("pred_b", ontology, entities, {}));
  actions.emplace(action1, actionObj1);

  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {ogp::Goal::fromStr("pred_b", ontology, entities)}, ontology.constants);
  EXPECT_EQ("", _lookForAnActionToDo(problem, domain).actionInvocation.toStr());
  problem.worldState.addFact(ogp::Fact("pred_a(toto)=10", false, ontology, entities, {}), problem.goalStack, setOfEventsMap,
                             _emptyCallbacks, ontology, entities, _now);
  EXPECT_EQ("10", problem.worldState.factsMapping().getFluentValue(ogp::Fact::fromPddl("(pred_a toto)", ontology, entities, {}, 0, nullptr, true))->value);

  _setGoalsForAPriority(problem, {ogp::Goal::fromStr("pred_b", ontology, entities)}, ontology.constants);
  EXPECT_EQ(action1, _lookForAnActionToDo(problem, domain).actionInvocation.toStr());
}


void _planWithActionThenEventWithValue()
{
  const std::string action1 = "action1";
  std::map<std::string, ogp::Action> actions;

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity");
  ontology.constants = ogp::SetOfEntities::fromPddl("toto titi - entity", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("pred_a - entity\n"
                                                      "pred_b(?e - entity)", ontology.types);

  const ogp::SetOfEntities entities;

  ogp::Action actionObj1({},
                         ogp::strToWsModification("pred_a=toto", ontology, entities, {}));
  actions.emplace(action1, actionObj1);

  ogp::SetOfEvents setOfEvents;
  std::vector<ogp::Parameter> eventParameters{ogp::Parameter::fromStr("?e - entity", ontology.types)};
  ogp::Event event(ogp::strToCondition("pred_a=?e", ontology, entities, eventParameters),
                   ogp::strToWsModification("pred_b(?e)", ontology, entities, eventParameters));
  event.parameters = std::move(eventParameters);
  setOfEvents.add(event);

  ogp::Domain domain(std::move(actions), {}, std::move(setOfEvents));
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {ogp::Goal::fromStr("pred_b(toto)", ontology, entities)}, ontology.constants);
  EXPECT_EQ(action1, _lookForAnActionToDo(problem, domain).actionInvocation.toStr());
}


void _planWithActionThenEventWithAssign()
{
  const std::string action1 = "action1";
  std::map<std::string, ogp::Action> actions;

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity\n"
                                             "other_type");
  ontology.constants = ogp::SetOfEntities::fromPddl("toto titi - entity\n"
                                                    "v - other_type", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("pred_a - other_type\n"
                                                      "pred_b(?e - entity) - other_type\n"
                                                      "pred_c - other_type\n"
                                                      "pred_d - other_type", ontology.types);

  const ogp::SetOfEntities entities;

  std::vector<ogp::Parameter> actionParameters{ogp::Parameter::fromStr("?e - entity", ontology.types)};
  ogp::Action actionObj1({},
                         ogp::strToWsModification("assign(pred_a, pred_b(?e))", ontology, entities, actionParameters));
  actionObj1.parameters = std::move(actionParameters);
  actions.emplace(action1, actionObj1);

  ogp::SetOfEvents setOfEvents;
  std::vector<ogp::Parameter> eventParameters{ogp::Parameter::fromStr("?t - other_type", ontology.types)};
  ogp::Event event(ogp::strToCondition("pred_a=?t", ontology, entities, eventParameters),
                   ogp::strToWsModification("pred_d=?t", ontology, entities, eventParameters));
  event.parameters = std::move(eventParameters);
  setOfEvents.add(event);

  ogp::Domain domain(std::move(actions), {}, std::move(setOfEvents));
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {ogp::Goal::fromStr("pred_d=v", ontology, entities)}, ontology.constants);
  problem.worldState.addFact(ogp::Fact("pred_b(toto)=v", false, ontology, entities, {}), problem.goalStack, setOfEventsMap,
                             _emptyCallbacks, ontology, entities, _now);
  EXPECT_EQ(action1 + "(?e -> toto)", _lookForAnActionToDo(problem, domain).actionInvocation.toStr());
}


void _valueEqualityInPrecoditionOfAnAction()
{
  std::map<std::string, ogp::Action> actions;

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity\n"
                                             "other_type\n"
                                             "lol");
  ontology.constants = ogp::SetOfEntities::fromPddl("toto titi - entity\n"
                                                    "v - other_type\n"
                                                    "lol_val - lol", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("pred_a - other_type\n"
                                                      "pred_b(?e - entity) - other_type\n"
                                                      "pred_c(?l - lol) - other_type\n"
                                                      "pred_d(?l - lol)", ontology.types);
  const ogp::SetOfEntities entities;

  const std::string action1 = "action1";
  std::vector<ogp::Parameter> actionParameters{ogp::Parameter::fromStr("?e - entity", ontology.types)};
  ogp::Action actionObj1({},
                         ogp::strToWsModification("assign(pred_a, pred_b(?e))", ontology, entities, actionParameters));
  actionObj1.parameters = std::move(actionParameters);
  actions.emplace(action1, actionObj1);

  const std::string action2 = "action2";
  std::vector<ogp::Parameter> action2Parameters{ogp::Parameter::fromStr("?l - lol", ontology.types)};
  ogp::Action actionObj2(ogp::strToCondition("=(pred_a, pred_c(?l))", ontology, entities, action2Parameters),
                         ogp::strToWsModification("pred_d(?l)", ontology, entities, action2Parameters));
  actionObj2.parameters = std::move(action2Parameters);
  actions.emplace(action2, actionObj2);

  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {ogp::Goal::fromStr("pred_d(lol_val)", ontology, entities)}, ontology.constants);
  problem.worldState.addFact(ogp::Fact("pred_b(toto)=v", false, ontology, entities, {}), problem.goalStack, setOfEventsMap,
                             _emptyCallbacks, ontology, entities, _now);
  problem.worldState.addFact(ogp::Fact("pred_c(lol_val)=v", false, ontology, entities, {}), problem.goalStack, setOfEventsMap,
                             _emptyCallbacks, ontology, entities, _now);
  EXPECT_EQ(action1 + "(?e -> toto)", _lookForAnActionToDo(problem, domain).actionInvocation.toStr());
}


void _testIncrementOfVariables()
{
  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("");
  ontology.constants = ogp::SetOfEntities::fromPddl("", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("numberOfQuestion - number\n"
                                                      "maxNumberOfQuestions - number\n"
                                                      "ask_all_the_questions\n"
                                                      "finished_to_ask_questions", ontology.types);
  const ogp::SetOfEntities entities;

  const std::string action_askQuestion1 = "ask_question_1";
  const std::string action_askQuestion2 = "ask_question_2";
  const std::string action_finisehdToAskQuestions = "finish_to_ask_questions";
  const std::string action_sayQuestionBilan = "say_question_bilan";

  std::map<std::string, ogp::Action> actions;
  const ogp::Action actionQ1({}, ogp::strToWsModification("ask_all_the_questions & add(numberOfQuestion, 1)", ontology, entities, {}));
  const ogp::Action actionFinishToActActions(ogp::strToCondition("equals(numberOfQuestion, maxNumberOfQuestions)", ontology, entities, {}),
                                             ogp::strToWsModification("ask_all_the_questions", ontology, entities, {}));
  const ogp::Action actionSayQuestionBilan(ogp::strToCondition("ask_all_the_questions", ontology, entities, {}),
                                           ogp::strToWsModification("finished_to_ask_questions", ontology, entities, {}));
  actions.emplace(action_askQuestion1, actionQ1);
  actions.emplace(action_askQuestion2, ogp::Action({}, ogp::strToWsModification("ask_all_the_questions & add(numberOfQuestion, 1)", ontology, entities, {})));
  actions.emplace(action_finisehdToAskQuestions, actionFinishToActActions);
  actions.emplace(action_sayQuestionBilan, actionSayQuestionBilan);
  ogp::Domain domain(std::move(actions), ontology);

  std::string initFactsStr = "numberOfQuestion=0 & maxNumberOfQuestions=3";
  ogp::Problem problem;
  problem.worldState.modify(&*ogp::strToWsModification(initFactsStr, ontology, entities, {}), problem.goalStack, _emptySetOfEvents, _emptyCallbacks, ontology, entities, _now);
  assert(ogp::strToCondition(initFactsStr, ontology, entities, {})->isTrue(problem.worldState, ontology.constants, problem.objects));
  assert(!actionFinishToActActions.precondition->isTrue(problem.worldState, ontology.constants, problem.objects));
  assert(!actionSayQuestionBilan.precondition->isTrue(problem.worldState, ontology.constants, problem.objects));
  assert(ogp::strToCondition("equals(maxNumberOfQuestions, numberOfQuestion + 3)", ontology, entities, {})->isTrue(problem.worldState, ontology.constants, problem.objects));
  assert(!ogp::strToCondition("equals(maxNumberOfQuestions, numberOfQuestion + 4)", ontology, entities, {})->isTrue(problem.worldState, ontology.constants, problem.objects));
  assert(ogp::strToCondition("equals(maxNumberOfQuestions, numberOfQuestion + 4 - 1)", ontology, entities, {})->isTrue(problem.worldState, ontology.constants, problem.objects));
  for (std::size_t i = 0; i < 3; ++i)
  {
    _setGoalsForAPriority(problem, {ogp::Goal::fromStr("finished_to_ask_questions", ontology, entities)}, ontology.constants);
    auto actionToDo = _lookForAnActionToDo(problem, domain).actionInvocation.toStr();
    if (i == 0 || i == 2)
      EXPECT_EQ(action_askQuestion1, actionToDo);
    else
      EXPECT_EQ(action_askQuestion2, actionToDo);
    problem.historical.notifyActionDone(actionToDo);
    auto itAction = domain.actions().find(actionToDo);
    assert(itAction != domain.actions().end());
    problem.worldState.modify(&*itAction->second.effect.worldStateModification, problem.goalStack,
                              _emptySetOfEvents, _emptyCallbacks, ontology, entities, _now);
    problem.worldState.modify(&*ogp::strToWsModification("!ask_all_the_questions", ontology, entities, {}),
                              problem.goalStack, _emptySetOfEvents, _emptyCallbacks, ontology, entities, _now);
  }
  assert(actionFinishToActActions.precondition->isTrue(problem.worldState, ontology.constants, problem.objects));
  assert(!actionSayQuestionBilan.precondition->isTrue(problem.worldState, ontology.constants, problem.objects));
  _setGoalsForAPriority(problem, {ogp::Goal::fromStr("finished_to_ask_questions", ontology, entities)}, ontology.constants);
  auto actionToDo = _lookForAnActionToDo(problem, domain).actionInvocation.toStr();
  EXPECT_EQ(action_finisehdToAskQuestions, actionToDo);
  problem.historical.notifyActionDone(actionToDo);
  auto itAction = domain.actions().find(actionToDo);
  assert(itAction != domain.actions().end());
  problem.worldState.modify(&*itAction->second.effect.worldStateModification, problem.goalStack,
                            _emptySetOfEvents, _emptyCallbacks, ontology, entities, _now);
  EXPECT_EQ(action_sayQuestionBilan, _lookForAnActionToDo(problem, domain).actionInvocation.toStr());
  assert(actionFinishToActActions.precondition->isTrue(problem.worldState, ontology.constants, problem.objects));
  assert(actionSayQuestionBilan.precondition->isTrue(problem.worldState, ontology.constants, problem.objects));
  problem.worldState.modify(&*actionSayQuestionBilan.effect.worldStateModification, problem.goalStack,
                            _emptySetOfEvents, _emptyCallbacks, ontology, entities, _now);
}


void _actionWithParametersInPreconditionsAndEffects()
{
  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("");
  ontology.constants = ogp::SetOfEntities::fromPddl("", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("isEngaged(?hid - number)\n"
                                                      "isHappy(?hid - number)", ontology.types);
  const ogp::SetOfEntities entities;

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> parameters(1, ogp::Parameter::fromStr("?human - number", ontology.types));
  ogp::Action joke(ogp::strToCondition("isEngaged(?human)", ontology, entities, parameters),
                   ogp::strToWsModification("isHappy(?human)", ontology, entities, parameters));
  joke.parameters = std::move(parameters);
  const std::string action1 = "action1";
  actions.emplace(action1, joke);

  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();

  ogp::Problem problem;
  problem.worldState.addFact(ogp::Fact("isEngaged(1)", false, ontology, entities, {}), problem.goalStack, setOfEventsMap,
                             _emptyCallbacks, ontology, entities, _now);

  _setGoalsForAPriority(problem, {ogp::Goal::fromStr("isHappy(1)", ontology, entities)}, ontology.constants);
  const auto& plan = ogp::planForEveryGoals(problem, domain, _emptyCallbacks, _now);
  EXPECT_EQ(action1 + "(?human -> 1)", ogp::planToStr(plan));
  EXPECT_EQ("00: (" + action1 + " 1) [1]\n", ogp::planToPddl(plan, domain));
}


void _testQuiz()
{
  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("");
  ontology.constants = ogp::SetOfEntities::fromPddl("", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("numberOfQuestion - number\n"
                                                      "maxNumberOfQuestions - number\n"
                                                      "ask_all_the_questions\n"
                                                      "finished_to_ask_questions", ontology.types);
  const ogp::SetOfEntities entities;

  const std::string action_askQuestion1 = "ask_question_1";
  const std::string action_askQuestion2 = "ask_question_2";
  const std::string action_sayQuestionBilan = "say_question_bilan";

  std::map<std::string, ogp::Action> actions;
  ogp::ProblemModification questionEffect(ogp::strToWsModification("add(numberOfQuestion, 1)", ontology, entities, {}));
  questionEffect.potentialWorldStateModification = ogp::strToWsModification("ask_all_the_questions", ontology, entities, {});
  const ogp::Action actionQ1({}, questionEffect);
  const ogp::Action actionSayQuestionBilan(ogp::strToCondition("ask_all_the_questions", ontology, entities, {}),
                                           ogp::strToWsModification("finished_to_ask_questions", ontology, entities, {}));
  actions.emplace(action_askQuestion1, actionQ1);
  actions.emplace(action_askQuestion2, ogp::Action({}, questionEffect));
  actions.emplace(action_sayQuestionBilan, actionSayQuestionBilan);

  ogp::Domain domain(std::move(actions), {},
                     ogp::Event(ogp::strToCondition("equals(numberOfQuestion, maxNumberOfQuestions)", ontology, entities, {}),
                                ogp::strToWsModification("ask_all_the_questions", ontology, entities, {})));

  auto initFacts = ogp::strToWsModification("numberOfQuestion=0 & maxNumberOfQuestions=3", ontology, entities, {});

  ogp::Problem problem;

  _setGoalsForAPriority(problem, {ogp::Goal::fromStr("finished_to_ask_questions", ontology, entities)}, ontology.constants);
  auto& setOfEventsMap = domain.getSetOfEvents();
  problem.worldState.modify(&*initFacts, problem.goalStack, setOfEventsMap, _emptyCallbacks, {}, {}, _now);
  for (std::size_t i = 0; i < 3; ++i)
  {
    auto actionToDo = _lookForAnActionToDo(problem, domain).actionInvocation.toStr();
    if (i == 0 || i == 2)
      EXPECT_EQ(action_askQuestion1, actionToDo);
    else
      EXPECT_EQ(action_askQuestion2, actionToDo);
    problem.historical.notifyActionDone(actionToDo);
    auto itAction = domain.actions().find(actionToDo);
    assert(itAction != domain.actions().end());
    problem.worldState.modify(&*itAction->second.effect.worldStateModification,
                              problem.goalStack, setOfEventsMap, _emptyCallbacks, {}, {}, _now);
  }

  auto actionToDo = _lookForAnActionToDo(problem, domain).actionInvocation.toStr();
  EXPECT_EQ(action_sayQuestionBilan, actionToDo);
}



void _doNextActionThatBringsToTheSmallerCost()
{
  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("location\n"
                                             "object\n"
                                             "robot");
  ontology.constants = ogp::SetOfEntities::fromPddl("me - robot\n"
                                                    "obj1 obj2 - object\n"
                                                    "livingRoom kitchen bedroom - location", ontology.types);

  ontology.predicates = ogp::SetOfPredicates::fromStr("objectGrabable(?o - object)\n"
                                                      "locationOfRobot(?r - robot) - location\n"
                                                      "locationOfObject(?o - object) - location\n"
                                                      "grab(?r - robot) - object", ontology.types);
  const std::string action_navigate = "navigate";
  const std::string action_grab = "grab";
  const std::string action_ungrab = "ungrab";

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> navParameters{ogp::Parameter::fromStr("?targetPlace - location", ontology.types)};
  ogp::Action navAction({}, ogp::strToWsModification("locationOfRobot(me)=?targetPlace", ontology, {}, navParameters));
  navAction.parameters = std::move(navParameters);
  actions.emplace(action_navigate, navAction);

  std::vector<ogp::Parameter> grabParameters{ogp::Parameter::fromStr("?object - object", ontology.types)};
  ogp::Action grabAction(ogp::strToCondition("equals(locationOfRobot(me), locationOfObject(?object)) & !grab(me)=*", ontology, {}, grabParameters),
                         ogp::strToWsModification("grab(me)=?object", ontology, {}, grabParameters));
  grabAction.parameters = std::move(grabParameters);
  actions.emplace(action_grab, grabAction);

  std::vector<ogp::Parameter> ungrabParameters{ogp::Parameter::fromStr("?object - object", ontology.types)};
  ogp::Action ungrabAction({}, ogp::strToWsModification("!grab(me)=?object", ontology, {}, ungrabParameters));
  ungrabAction.parameters = std::move(ungrabParameters);
  actions.emplace(action_ungrab, ungrabAction);

  ogp::SetOfEvents setOfEvents;
  {
    std::vector<ogp::Parameter> eventParameters{ogp::Parameter::fromStr("?object - object", ontology.types), ogp::Parameter::fromStr("?location - location", ontology.types)};
    ogp::Event event(ogp::strToCondition("locationOfRobot(me)=?location & grab(me)=?object & objectGrabable(?object)", ontology, {}, eventParameters),
                     ogp::strToWsModification("locationOfObject(?object)=?location", ontology, {}, eventParameters));
    event.parameters = std::move(eventParameters);
    setOfEvents.add(event);
  }

  ogp::Domain domain(std::move(actions), {}, std::move(setOfEvents));
  auto& setOfEventsMap = domain.getSetOfEvents();

  ogp::Problem problem;
  auto& entities = problem.objects;
  problem.worldState.addFact(ogp::Fact("objectGrabable(obj1)", false, ontology, entities, {}), problem.goalStack, setOfEventsMap,
                             _emptyCallbacks, ontology, entities, _now);
  problem.worldState.addFact(ogp::Fact("objectGrabable(obj2)", false, ontology, entities, {}), problem.goalStack, setOfEventsMap,
                             _emptyCallbacks, ontology, entities, _now);
  problem.worldState.addFact(ogp::Fact("locationOfRobot(me)=livingRoom", false, ontology, entities, {}), problem.goalStack, setOfEventsMap,
                             _emptyCallbacks, ontology, entities, _now);
  problem.worldState.addFact(ogp::Fact("grab(me)=obj2", false, ontology, entities, {}), problem.goalStack, setOfEventsMap,
                             _emptyCallbacks, ontology, entities, _now);
  problem.worldState.addFact(ogp::Fact("locationOfObject(obj2)=livingRoom", false, ontology, entities, {}), problem.goalStack, setOfEventsMap,
                             _emptyCallbacks, ontology, entities, _now);
  problem.worldState.addFact(ogp::Fact("locationOfObject(obj1)=kitchen", false, ontology, entities, {}), problem.goalStack, setOfEventsMap,
                             _emptyCallbacks, ontology, entities, _now);
  auto secondProblem = problem;
  auto thirdProblem = problem;
  auto fourthProblem = problem;
  // Here it will will be quicker for the second goal if we ungrab the obj2 right away
  _setGoalsForAPriority(problem, {ogp::Goal::fromStr("locationOfObject(obj1)=bedroom & !grab(me)=obj1", ontology, entities),
                                  ogp::Goal::fromStr("locationOfObject(obj2)=livingRoom & !grab(me)=obj2", ontology, entities)}, ontology.constants);
  const auto& plan = ogp::planForEveryGoals(problem, domain, _emptyCallbacks, _now);
  const std::string planStartingWithUngrab = "ungrab(?object -> obj2)\n"
                                             "navigate(?targetPlace -> kitchen)\n"
                                             "grab(?object -> obj1)\n"
                                             "navigate(?targetPlace -> bedroom)\n"
                                             "ungrab(?object -> obj1)";
  EXPECT_EQ(planStartingWithUngrab, ogp::planToStr(plan, "\n"));

  // Here it will will be quicker for the second goal if we move the obj2 to the kitchen
  _setGoalsForAPriority(secondProblem, {ogp::Goal::fromStr("locationOfObject(obj1)=bedroom & !grab(me)=obj1", ontology, entities),
                                        ogp::Goal::fromStr("locationOfObject(obj2)=kitchen & !grab(me)=obj2", ontology, entities)}, ontology.constants);
  const auto& secondPlan = ogp::planForEveryGoals(secondProblem, domain, _emptyCallbacks, _now);
  const std::string planStartingWithNavigate = "navigate(?targetPlace -> kitchen)\n"
                                               "ungrab(?object -> obj2)\n"
                                               "grab(?object -> obj1)\n"
                                               "navigate(?targetPlace -> bedroom)\n"
                                               "ungrab(?object -> obj1)";
  EXPECT_EQ(planStartingWithNavigate, ogp::planToStr(secondPlan, "\n"));

  // Exactly the same checks but !grab(me) part of goal before
  // ---------------------------------------------------------
  // Here it will will be quicker for the second goal if we ungrab the obj2 right away
  _setGoalsForAPriority(thirdProblem, {ogp::Goal::fromStr("!grab(me)=obj1 & locationOfObject(obj1)=bedroom", ontology, entities),
                                       ogp::Goal::fromStr("!grab(me)=obj2 & locationOfObject(obj2)=livingRoom", ontology, entities)}, ontology.constants);
  EXPECT_EQ(planStartingWithUngrab, ogp::planToStr(ogp::planForEveryGoals(thirdProblem, domain, _emptyCallbacks, _now), "\n"));

  // Here it will will be quicker for the second goal if we move the obj2 to the kitchen
  _setGoalsForAPriority(fourthProblem, {ogp::Goal::fromStr("!grab(me)=obj1 & locationOfObject(obj1)=bedroom", ontology, entities),
                                        ogp::Goal::fromStr("!grab(me)=obj2 & locationOfObject(obj2)=kitchen", ontology, entities)}, ontology.constants);
  EXPECT_EQ(planStartingWithNavigate, ogp::planToStr(ogp::planForEveryGoals(fourthProblem, domain, _emptyCallbacks, _now), "\n"));
}


void _satisfyGoalWithSuperiorOperator()
{
  const std::string action1 = "action1";
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr("fact_a - number\n"
                                                      "fact_b", ontology.types);

  ogp::SetOfConstFacts timelessFacts;
  timelessFacts.add(ogp::Fact("fact_b", false, ontology, {}, {}));

  std::map<std::string, ogp::Action> actions;
  actions.emplace(action1, ogp::Action(ogp::strToCondition("fact_b", ontology, {}, {}),
                                       ogp::strToWsModification("fact_a=100", ontology, {}, {})));
  ogp::Domain domain(std::move(actions), ontology, {}, {}, timelessFacts);
  auto& setOfEventsMap = domain.getSetOfEvents();

  ogp::Problem problem(&timelessFacts.setOfFacts());
  auto& entities = problem.objects;
  problem.worldState.addFact(ogp::Fact("fact_a=10", false, ontology, entities, {}), problem.goalStack, setOfEventsMap,
                             _emptyCallbacks, ontology, entities, _now);
  _setGoalsForAPriority(problem, {ogp::Goal::fromStr("fact_a>50", ontology, entities)}, ontology.constants);

  EXPECT_EQ(action1, _lookForAnActionToDoThenNotify(problem, domain, _emptyCallbacks, _now).actionInvocation.toStr());
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _emptyCallbacks, _now).actionInvocation.toStr());
}


void _parameterToFillFromConditionOfFirstAction()
{
  const std::string action1 = "action1";
  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("location\n"
                                             "chargingZone");
  ontology.constants = ogp::SetOfEntities::fromPddl("cz - chargingZone\n"
                                                    "czLocation - location", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("locationOfRobot - location\n"
                                                      "declaredLocationOfChargingZone(?cz - chargingZone) - location\n"
                                                      "batteryLevel - number", ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> actionParameters{ogp::Parameter::fromStr("?cz - chargingZone", ontology.types)};
  ogp::Action action1Obj(ogp::strToCondition("=(locationOfRobot, declaredLocationOfChargingZone(?cz))", ontology, {}, actionParameters),
                         ogp::strToWsModification("batteryLevel=100", ontology, {}, actionParameters));
  action1Obj.parameters = std::move(actionParameters);
  actions.emplace(action1, action1Obj);
  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();

  ogp::Problem problem;
  auto& entities = problem.objects;
  problem.worldState.addFact(ogp::Fact("locationOfRobot=czLocation", false, ontology, entities, {}), problem.goalStack, setOfEventsMap,
                             _emptyCallbacks, ontology, entities, _now);
  problem.worldState.addFact(ogp::Fact("declaredLocationOfChargingZone(cz)=czLocation", false, ontology, entities, {}), problem.goalStack, setOfEventsMap,
                             _emptyCallbacks, ontology, entities, _now);
  problem.worldState.addFact(ogp::Fact("batteryLevel=40", false, ontology, entities, {}), problem.goalStack, setOfEventsMap,
                             _emptyCallbacks, ontology, entities, _now);
  _setGoalsForAPriority(problem, {ogp::Goal::fromStr("batteryLevel=100", ontology, entities)}, ontology.constants);

  EXPECT_EQ(action1 + "(?cz -> cz)", _lookForAnActionToDo(problem, domain, _emptyCallbacks, _now).actionInvocation.toStr());
}


void _planToMove()
{
  const std::string action1 = "action1";
  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("location\n"
                                             "graspable_obj - object");
  ontology.constants = ogp::SetOfEntities::fromPddl("loc1 - location\n"
                                                    "bottle - graspable_obj", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("locationOfRobot - location\n"
                                                      "locationOf(?o - object) - location", ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> actionParameters{ogp::Parameter::fromStr("?o - object", ontology.types)};
  ogp::Action action1Obj({}, ogp::strToWsModification("assign(locationOfRobot, locationOf(?o))", ontology, {}, actionParameters));
  action1Obj.parameters = std::move(actionParameters);
  actions.emplace(action1, action1Obj);
  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();

  ogp::Problem problem;
  auto& objects = problem.objects;
  problem.worldState.addFact(ogp::Fact("locationOf(bottle)=loc1", false, ontology, objects, {}), problem.goalStack, setOfEventsMap,
                             _emptyCallbacks, ontology, objects, _now);
  _setGoalsForAPriority(problem, {ogp::Goal::fromStr("locationOfRobot=loc1", ontology, objects)}, ontology.constants);

  EXPECT_EQ(action1 + "(?o -> bottle)", _lookForAnActionToDo(problem, domain, _emptyCallbacks, _now).actionInvocation.toStr());
}

void _disjunctiveGoal()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr("fact_a\n"
                                                      "fact_b", ontology.types);

  std::map<std::string, ogp::Action> actions;
  actions.emplace(action1, ogp::Action({}, ogp::strToWsModification("fact_a", ontology, {}, {})));
  actions.emplace(action2, ogp::Action({}, ogp::strToWsModification("fact_b", ontology, {}, {})));
  ogp::Domain domain(std::move(actions), ontology);

  ogp::Problem problem;
  auto& entities = problem.objects;
  _setGoalsForAPriority(problem, {ogp::Goal::fromStr("or(fact_a, fact_b)", ontology, entities)}, ontology.constants);

  auto firstActionStr = _lookForAnActionToDoThenNotify(problem, domain, _emptyCallbacks, _now).actionInvocation.toStr();
  if (firstActionStr != action1 && firstActionStr != action2)
    EXPECT_TRUE(false);
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _emptyCallbacks, _now).actionInvocation.toStr());
}


void _disjunctivePrecondition()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  const std::string action3 = "action3";
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr("fact_a\n"
                                                      "fact_b\n"
                                                      "fact_c", ontology.types);

  std::map<std::string, ogp::Action> actions;
  actions.emplace(action1, ogp::Action({}, ogp::strToWsModification("fact_a", ontology, {}, {})));
  actions.emplace(action2, ogp::Action({}, ogp::strToWsModification("fact_b", ontology, {}, {})));
  actions.emplace(action3, ogp::Action(ogp::strToCondition("or(fact_a, fact_b)", ontology, {}, {}),
                                       ogp::strToWsModification("fact_c", ontology, {}, {})));
  ogp::Domain domain(std::move(actions), ontology);

  ogp::Problem problem;
  auto& entities = problem.objects;
  _setGoalsForAPriority(problem, {ogp::Goal::fromStr("fact_c", ontology, entities)}, ontology.constants);

  auto firstActionStr = _lookForAnActionToDoThenNotify(problem, domain, _emptyCallbacks, _now).actionInvocation.toStr();
  if (firstActionStr != action1 && firstActionStr != action2)
    EXPECT_TRUE(false);
  EXPECT_EQ(action3, _lookForAnActionToDoThenNotify(problem, domain, _emptyCallbacks, _now).actionInvocation.toStr());
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _emptyCallbacks, _now).actionInvocation.toStr());
}


void _forallWithImplyAndFluentValuesEqualityOnPrecondition()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity return_type");
  ontology.predicates = ogp::SetOfPredicates::fromStr("fact_1(?p - entity) - return_type\n"
                                                      "fact_2(?p - entity) - return_type\n"
                                                      "fact_3(?p - entity) - return_type\n"
                                                      "goal", ontology.types);
  ontology.constants = ogp::SetOfEntities::fromPddl("r1 r2 r3 r4 r5 - return_type", ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> action1Parameters{ogp::Parameter::fromStr("?p1 - entity", ontology.types), ogp::Parameter::fromStr("?p2 - return_type", ontology.types)};
  ogp::Action actionObj1({}, _worldStateModification_fromPddl("(assign (fact_1 ?p1) ?p2)", ontology, action1Parameters));
  actionObj1.parameters = std::move(action1Parameters);
  actions.emplace(action1, actionObj1);

  ogp::Action actionObj2(_condition_fromPddl("(forall (?e - entity) (imply (= (fact_3 ?e) r2) (= (fact_1 ?e) (fact_2 ?e))))", ontology),
                         _worldStateModification_fromPddl("(goal)", ontology));
  actions.emplace(action2, actionObj2);

  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  problem.objects = ogp::SetOfEntities::fromPddl("v1 v2 v3 v4 - entity", ontology.types);
  problem.worldState.addFact(ogp::Fact("fact_3(v1)=r2", false, ontology, problem.objects, {}), problem.goalStack, setOfEventsMap,
                             _emptyCallbacks, ontology, problem.objects, _now);
  problem.worldState.addFact(ogp::Fact("fact_3(v3)=r2", false, ontology, problem.objects, {}), problem.goalStack, setOfEventsMap,
                             _emptyCallbacks, ontology, problem.objects, _now);
  problem.worldState.addFact(ogp::Fact("fact_2(v1)=r3", false, ontology, problem.objects, {}), problem.goalStack, setOfEventsMap,
                             _emptyCallbacks, ontology, problem.objects, _now);
  problem.worldState.addFact(ogp::Fact("fact_2(v3)=r4", false, ontology, problem.objects, {}), problem.goalStack, setOfEventsMap,
                             _emptyCallbacks, ontology, problem.objects, _now);

  _setGoalsForAPriority(problem, {_pddlGoal("(goal)", ontology, problem.objects)}, ontology.constants);
  auto actionInvocation1 = _lookForAnActionToDoThenNotify(problem, domain, _emptyCallbacks, _now).actionInvocation.toStr();
  auto actionInvocation2 = _lookForAnActionToDoThenNotify(problem, domain, _emptyCallbacks, _now).actionInvocation.toStr();

  std::set<std::string> potentialResults{"action1(?p1 -> v1, ?p2 -> r3)", "action1(?p1 -> v3, ?p2 -> r4)"};
  EXPECT_NE(actionInvocation1, actionInvocation2);
  EXPECT_TRUE(potentialResults.count(actionInvocation1) > 0);
  EXPECT_TRUE(potentialResults.count(actionInvocation2) > 0);
  EXPECT_EQ(action2, _lookForAnActionToDoThenNotify(problem, domain, _emptyCallbacks, _now).actionInvocation.toStr());
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _emptyCallbacks, _now).actionInvocation.toStr());
}


void _notEqualUndefinedGoal()
{
  const std::string action1 = "action1";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("return_type");
  ontology.predicates = ogp::SetOfPredicates::fromStr("fact_1 - return_type", ontology.types);
  ontology.constants = ogp::SetOfEntities::fromPddl("r1 r2 - return_type", ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> action1Parameters{ ogp::Parameter::fromStr("?p - return_type", ontology.types)};
  ogp::Action actionObj1({}, _worldStateModification_fromPddl("(assign (fact_1) ?p)", ontology, action1Parameters));
  actionObj1.parameters = std::move(action1Parameters);
  actions.emplace(action1, actionObj1);

  ogp::Domain domain(std::move(actions), ontology);
  ogp::Problem problem;

  _setGoalsForAPriority(problem, {_pddlGoal("(not (= (fact_1) undefined))", ontology, problem.objects)}, ontology.constants);
  auto actionInvocation = _lookForAnActionToDoThenNotify(problem, domain, _emptyCallbacks, _now).actionInvocation.toStr();
  EXPECT_TRUE(actionInvocation == "action1(?p -> r1)" || actionInvocation == "action1(?p -> r2)");
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _emptyCallbacks, _now).actionInvocation.toStr());
}


void _notEqualUndefinedGoalAndAssignation()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("return_type");
  ontology.predicates = ogp::SetOfPredicates::fromStr("fact_1 - return_type\n"
                                                      "fact_2 - return_type", ontology.types);
  ontology.constants = ogp::SetOfEntities::fromPddl("r1 r2 - return_type", ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> action1Parameters{ ogp::Parameter::fromStr("?p - return_type", ontology.types)};
  ogp::Action actionObj1({}, _worldStateModification_fromPddl("(assign (fact_1) ?p)", ontology, action1Parameters));
  actionObj1.parameters = std::move(action1Parameters);
  actions.emplace(action1, actionObj1);

  std::vector<ogp::Parameter> action2Parameters{ ogp::Parameter::fromStr("?p - return_type", ontology.types)};
  ogp::Action actionObj2({}, _worldStateModification_fromPddl("(assign (fact_2) ?p)", ontology, action2Parameters));
  actionObj2.parameters = std::move(action2Parameters);
  actions.emplace(action2, actionObj2);

  ogp::Domain domain(std::move(actions), ontology);
  ogp::Problem problem;

  _setGoalsForAPriority(problem, {_pddlGoal("(and (not (= (fact_1) undefined)) (= (fact_2) (fact_1)) )", ontology, problem.objects)}, ontology.constants);
  auto actionInvocation1 = _lookForAnActionToDoThenNotify(problem, domain, _emptyCallbacks, _now).actionInvocation.toStr();
  EXPECT_TRUE(actionInvocation1 == "action1(?p -> r1)" || actionInvocation1 == "action1(?p -> r2)");
  auto actionInvocation2 = _lookForAnActionToDoThenNotify(problem, domain, _emptyCallbacks, _now).actionInvocation.toStr();
  EXPECT_TRUE(actionInvocation2 == "action2(?p -> r1)" || actionInvocation2 == "action2(?p -> r2)");
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _emptyCallbacks, _now).actionInvocation.toStr());
}



void _assignAFluentWithoutValue()
{
  const std::string action1 = "action1";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("return_type");
  ontology.predicates = ogp::SetOfPredicates::fromStr("fn_1 - return_type\n"
                                                      "fn_2 - return_type\n"
                                                      "pred_1", ontology.types);
  ontology.constants = ogp::SetOfEntities::fromPddl("r1 r2 - return_type", ontology.types);

  std::map<std::string, ogp::Action> actions;
  ogp::Action actionObj1({}, _worldStateModification_fromPddl("(and (pred_1) (assign (fn_1) (fn_2)))", ontology));
  actions.emplace(action1, actionObj1);

  ogp::Domain domain(std::move(actions), ontology);
  ogp::Problem problem;

  _setGoalsForAPriority(problem, {_pddlGoal("(pred_1)", ontology, problem.objects)}, ontology.constants);
  EXPECT_EQ("action1", _lookForAnActionToDoThenNotify(problem, domain, _emptyCallbacks, _now).actionInvocation.toStr());
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _emptyCallbacks, _now).actionInvocation.toStr());

  EXPECT_EQ("(pred_1)", worldStateToPddl(problem.worldState));
}


void _assignAFluentWithoutValueAndEventToResetValue()
{
  const std::string action1 = "action1";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("return_type");
  ontology.predicates = ogp::SetOfPredicates::fromStr("fn_1 - return_type\n"
                                                      "fn_2 - return_type\n"
                                                      "pred_1", ontology.types);
  ontology.constants = ogp::SetOfEntities::fromPddl("r1 r2 - return_type", ontology.types);

  std::map<std::string, ogp::Action> actions;
  ogp::Action actionObj1({}, _worldStateModification_fromPddl("(and (pred_1) (assign (fn_1) (fn_2)))", ontology));
  actions.emplace(action1, actionObj1);

  ogp::SetOfEvents setOfEvents;
  ogp::Event event(_condition_fromPddl("(= (fn_1) undefined)", ontology),
                   _worldStateModification_fromPddl("(assign (fn_1) r2)", ontology));
  setOfEvents.add(event);

  ogp::Domain domain(std::move(actions), ontology, setOfEvents);
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;

  _setGoalsForAPriority(problem, {_pddlGoal("(pred_1)", ontology, problem.objects)}, ontology.constants);
  EXPECT_EQ("action1", _lookForAnActionToDoThenNotify(problem, domain, _emptyCallbacks, _now).actionInvocation.toStr());
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _emptyCallbacks, _now).actionInvocation.toStr());

  EXPECT_EQ("(= (fn_1) r2)\n(pred_1)", worldStateToPddl(problem.worldState));
  problem.worldState.addFact(ogp::Fact("fn_1=r1", false, ontology, problem.objects, {}), problem.goalStack, setOfEventsMap,
                             _emptyCallbacks, ontology, problem.objects, _now);
  problem.worldState.removeFact(ogp::Fact("pred_1", false, ontology, problem.objects, {}), problem.goalStack, setOfEventsMap,
                                _emptyCallbacks, ontology, problem.objects, _now);
  EXPECT_EQ("(= (fn_1) r1)", worldStateToPddl(problem.worldState));

  _setGoalsForAPriority(problem, {_pddlGoal("(pred_1)", ontology, problem.objects)}, ontology.constants);
  EXPECT_EQ("action1", _lookForAnActionToDoThenNotify(problem, domain, _emptyCallbacks, _now).actionInvocation.toStr());
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _emptyCallbacks, _now).actionInvocation.toStr());
  EXPECT_EQ("(= (fn_1) r2)\n(pred_1)", worldStateToPddl(problem.worldState));

  // Check reassign value with event condition checking that value is undefined
  problem.worldState.addFact(ogp::Fact("fn_1=r2", false, ontology, problem.objects, {}), problem.goalStack, setOfEventsMap,
                             _emptyCallbacks, ontology, problem.objects, _now);
  EXPECT_EQ("(= (fn_1) r2)\n(pred_1)", worldStateToPddl(problem.worldState));
}


}



TEST(Planner, test_planner)
{
  planningDummyExample();
  planningExampleWithAPreconditionSolve();

  _simplest_plan_possible();
  _wrong_condition_type();
  _number_type();
  _planWithActionThenEventWithValue();
  _planWithActionThenEventWithAssign();
  _valueEqualityInPrecoditionOfAnAction();
  _testIncrementOfVariables();
  _actionWithParametersInPreconditionsAndEffects();
  _testQuiz();
  _doNextActionThatBringsToTheSmallerCost();
  _satisfyGoalWithSuperiorOperator();
  _parameterToFillFromConditionOfFirstAction();
  _planToMove();
  _disjunctiveGoal();
  _disjunctivePrecondition();
  _forallWithImplyAndFluentValuesEqualityOnPrecondition();
  _notEqualUndefinedGoal();
  _notEqualUndefinedGoalAndAssignation();
  _assignAFluentWithoutValue();
  _assignAFluentWithoutValueAndEventToResetValue();
}
