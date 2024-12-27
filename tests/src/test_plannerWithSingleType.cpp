#include <gtest/gtest.h>
#include <orderedgoalsplanner/orderedgoalsplanner.hpp>
#include <orderedgoalsplanner/types/axiom.hpp>
#include <orderedgoalsplanner/types/predicate.hpp>
#include <orderedgoalsplanner/types/setofcallbacks.hpp>
#include <orderedgoalsplanner/types/setofevents.hpp>
#include <orderedgoalsplanner/util/serializer/deserializefrompddl.hpp>
#include <orderedgoalsplanner/util/trackers/goalsremovedtracker.hpp>
#include <orderedgoalsplanner/util/print.hpp>

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
const std::string _fact_punctual_p1 = ogp::Fact::getPunctualPrefix() + "fact_p1";
const std::string _fact_punctual_p2 = ogp::Fact::getPunctualPrefix() + "fact_p2";
const std::string _fact_punctual_p3 = ogp::Fact::getPunctualPrefix() + "fact_p3";
const std::string _fact_punctual_p4 = ogp::Fact::getPunctualPrefix() + "fact_p4";
const std::string _fact_punctual_p5 = ogp::Fact::getPunctualPrefix() + "fact_p5";
const std::string _fact_advertised = "advertised";
const std::string _fact_beginOfConversation = "begin_of_conversation";
const std::string _fact_presented = "presented";
const std::string _fact_greeted = "greeted";
const std::string _fact_is_close = "is_close";
const std::string _fact_hasQrCode = "has_qrcode";
const std::string _fact_hasCheckInPasword = "has_check_in_password";
const std::string _fact_userWantsToCheckedIn = "user_wants_to_checked_in";
const std::string _fact_checkedIn = "checked_in";
const std::string _fact_beSad = "be_sad";
const std::string _fact_beHappy = "be_happy";
const std::string _fact_engagedWithUser = "engaged_with_user";
const std::string _fact_userSatisfied = "user_satisfied";
const std::string _fact_robotLearntABehavior = "robot_learnt_a_behavior";
const std::string _fact_headTouched = "head_touched";
const std::string _fact_punctual_headTouched = ogp::Fact::getPunctualPrefix() + "head_touched";
const std::string _fact_punctual_checkedIn = ogp::Fact::getPunctualPrefix() + "checked_in";

const std::string _action_presentation = "presentation";
const std::string _action_askQuestion1 = "ask_question_1";
const std::string _action_askQuestion2 = "ask_question_2";
const std::string _action_greet = "greet";
const std::string _action_advertise = "advertise";
const std::string _action_checkIn = "check_in";
const std::string _action_joke = "joke";
const std::string _action_checkInWithRealPerson = "check_in_with_real_person";
const std::string _action_checkInWithQrCode = "check_in_with_qrcode";
const std::string _action_checkInWithPassword = "check_in_with_password";
const std::string _action_goodBoy = "good_boy";
const std::string _action_navigate = "navigate";
const std::string _action_welcome = "welcome";
const std::string _action_grab = "grab";
const std::string _action_ungrab = "ungrab";
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

ogp::Entity _entityDeclaration(const std::string& pStr,
                               const ogp::Ontology& pOntology) {
  return ogp::Entity::fromDeclaration(pStr, pOntology.types);
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
                           const std::vector<std::string>& pGoalStrs,
                           const ogp::Ontology& pOntology,
                           const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {},
                           int pPriority = ogp::GoalStack::getDefaultPriority())
{
  std::vector<ogp::Goal> goals;
  for (auto& currFactStr : pGoalStrs)
    goals.emplace_back(ogp::Goal::fromStr(currFactStr, pOntology, ogp::SetOfEntities()));
  pProblem.goalStack.setGoals(goals, pProblem.worldState, pOntology.constants, pProblem.objects, pNow, pPriority);
}


void _setGoalsForAPriority(ogp::Problem& pProblem,
                           const std::vector<ogp::Goal>& pGoals,
                           const ogp::SetOfEntities& pConstants,
                           const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {},
                           int pPriority = ogp::GoalStack::getDefaultPriority())
{
  pProblem.goalStack.setGoals(pGoals, pProblem.worldState, pConstants, pProblem.objects, pNow, pPriority);
}


void _setGoals(ogp::Problem& pProblem,
               const std::map<int, std::vector<std::string>>& pGoalStrs,
               const ogp::Ontology& pOntology,
               const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {})
{
  std::map<int, std::vector<ogp::Goal>> goals;
  for (auto& currGoals : pGoalStrs)
  {
    auto& goalsForAPrio = goals[currGoals.first];
    for (auto& currGoal : currGoals.second)
      goalsForAPrio.emplace_back(ogp::Goal::fromStr(currGoal, pOntology, ogp::SetOfEntities()));
  }
  pProblem.goalStack.setGoals(goals, pProblem.worldState, pOntology.constants, pProblem.objects, pNow);
}


void _setGoals(ogp::Problem& pProblem,
               const std::map<int, std::vector<ogp::Goal>>& pGoals,
               const ogp::SetOfEntities& pConstants,
               const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {})
{
  pProblem.goalStack.setGoals(pGoals, pProblem.worldState, pConstants, pProblem.objects, pNow);
}


void _addGoals(ogp::Problem& pProblem,
               const std::map<int, std::vector<std::string>>& pGoalStrs,
               const ogp::Ontology& pOntology,
               const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {})
{
  std::map<int, std::vector<ogp::Goal>> goals;
  for (auto& currGoals : pGoalStrs)
  {
    auto& goalsForAPrio = goals[currGoals.first];
    for (auto& currGoal : currGoals.second)
      goalsForAPrio.emplace_back(ogp::Goal::fromStr(currGoal, pOntology, ogp::SetOfEntities()));
  }
  pProblem.goalStack.addGoals(goals, pProblem.worldState, pOntology.constants, pProblem.objects, pNow);
}

void _addGoalsForAPriority(ogp::Problem& pProblem,
                           const std::vector<std::string>& pGoalStrs,
                           const ogp::Ontology& pOntology,
                           const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {},
                           int pPriority = ogp::GoalStack::getDefaultPriority())
{
  std::vector<ogp::Goal> goals;
  for (auto& currFactStr : pGoalStrs)
    goals.emplace_back(ogp::Goal::fromStr(currFactStr, pOntology, ogp::SetOfEntities()));
  pProblem.goalStack.addGoals(goals, pProblem.worldState, pOntology.constants, pProblem.objects, pNow, pPriority);
}


void _addGoalsForAPriority(ogp::Problem& pProblem,
                           const std::vector<ogp::Goal>& pGoals,
                           const ogp::SetOfEntities& pConstants,
                           const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {},
                           int pPriority = ogp::GoalStack::getDefaultPriority())
{
  pProblem.goalStack.addGoals(pGoals, pProblem.worldState, pConstants, pProblem.objects, pNow, pPriority);
}


void _setFacts(ogp::WorldState& pWorldState,
               const std::set<std::string>& pFactStrs,
               ogp::GoalStack& pGoalStack,
               const ogp::Ontology& pOntology,
               const std::map<ogp::SetOfEventsId, ogp::SetOfEvents>& pSetOfEvents = _emptySetOfEvents) {
  std::set<ogp::Fact> facts;
  for (auto& currFactStr : pFactStrs)
    facts.emplace(currFactStr, false, pOntology, ogp::SetOfEntities(), std::vector<ogp::Parameter>());
  pWorldState.setFacts(facts, pGoalStack, pSetOfEvents, _emptyCallbacks, pOntology, ogp::SetOfEntities(), _now);
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

bool _hasFact(ogp::WorldState& pWorldState,
              const std::string& pFactStr,
              const ogp::Ontology& pOntology) {
  return pWorldState.hasFact(_fact(pFactStr, pOntology));
}


std::string _solveStrConst(const ogp::Problem& pProblem,
                           const std::map<std::string, ogp::Action>& pActions,
                           const ogp::Ontology& pOntology,
                           ogp::Historical* pGlobalHistorical = nullptr)
{
  auto problem = pProblem;
  ogp::Domain domain(pActions, pOntology);
  return ogp::planToStr(ogp::planForEveryGoals(problem, domain, {}, pGlobalHistorical), _sep);
}

std::string _solveStrConst(const ogp::Problem& pProblem,
                           const ogp::Domain& pDomain,
                           ogp::Historical* pGlobalHistorical = nullptr)
{
  auto problem = pProblem;
  return ogp::planToStr(ogp::planForEveryGoals(problem, pDomain, {}, pGlobalHistorical), _sep);
}

std::string _solveStr(ogp::Problem& pProblem,
                      const std::map<std::string, ogp::Action>& pActions,
                      const ogp::Ontology& pOntology,
                      const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {},
                      ogp::Historical* pGlobalHistorical = nullptr)
{
  ogp::Domain domain(pActions, pOntology);
  return ogp::planToStr(ogp::planForEveryGoals(pProblem, domain, pNow, pGlobalHistorical), _sep);
}

std::string _solveStr(ogp::Problem& pProblem,
                      const ogp::Domain& pDomain,
                      const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {},
                      ogp::Historical* pGlobalHistorical = nullptr)
{
  return ogp::planToStr(ogp::planForEveryGoals(pProblem, pDomain, pNow, pGlobalHistorical), _sep);
}


std::string _getGoalsDoneDuringAPlannificationConst(const ogp::Problem& pProblem,
                                                    const ogp::Domain& pDomain,
                                                    ogp::Historical* pGlobalHistorical = nullptr)
{
  auto problem = pProblem;
  std::list<ogp::Goal> pGoalsDone;
  ogp::planForEveryGoals(problem, pDomain, {}, pGlobalHistorical, &pGoalsDone);
  return ogp::goalsToStr(pGoalsDone, _sep);
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

std::string _lookForAnActionToDoStr(ogp::Problem& pProblem,
                                    const ogp::Domain& pDomain,
                                    const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {},
                                    const ogp::Historical* pGlobalHistorical = nullptr)
{
  return _lookForAnActionToDo(pProblem, pDomain, pNow, pGlobalHistorical).actionInvocation.toStr();
}

std::string _lookForAnActionToDoConstStr(const ogp::Problem& pProblem,
                                         const ogp::Domain& pDomain,
                                         const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {},
                                         const ogp::Historical* pGlobalHistorical = nullptr)
{
  return _lookForAnActionToDoConst(pProblem, pDomain, pNow, pGlobalHistorical).actionInvocation.toStr();
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


std::string _lookForAnActionToDoInParallelThenNotifyToStr(
    ogp::Problem& pProblem,
    const ogp::Domain& pDomain,
    const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {})
{
  auto actionsToDoInParallel = ogp::actionsToDoInParallelNow(pProblem, pDomain, pNow);
  for (auto& currAction : actionsToDoInParallel.actions)
  {
    notifyActionStarted(pProblem, pDomain, _emptyCallbacks, currAction, pNow);
    notifyActionDone(pProblem, pDomain, _emptyCallbacks, currAction, pNow);
  }
  return ogp::planToStr(actionsToDoInParallel.actions);
}




void _test_createEmptyGoal()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr("goal_name", ontology.types);
  _goal("goal_name", ontology, -1, "");
}

void _test_goalToStr()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr("a\n"
                                                      "b\n"
                                                      "condition\n"
                                                      "goal_name", ontology.types);

  EXPECT_EQ("persist(a & b)", _goal("persist(a & b)", ontology).toStr());
  EXPECT_EQ("imply(condition, goal_name)", _goal("imply(condition, goal_name)", ontology).toStr());
  EXPECT_EQ("persist(imply(condition, goal_name))", _goal("persist(imply(condition, goal_name))", ontology).toStr());
  EXPECT_EQ("oneStepTowards(goal_name)", _goal("oneStepTowards(goal_name)", ontology).toStr());
}

void _test_factToStr()
{
  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("human");
  ontology.constants = ogp::SetOfEntities::fromPddl("h1 - human", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("isEngaged(?h - human)", ontology.types);
  EXPECT_EQ("isEngaged(h1)", _fact("isEngaged(h1)", ontology).toStr());
}




void _test_conditionParameters()
{
  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("location\n"
                                             "entity\n"
                                             "robot - physical_object");
  ontology.constants = ogp::SetOfEntities::fromPddl("me self - robot\n"
                                                    "pen - physical_object\n"
                                                    "entrance kitchen - location", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("location(?r - robot) - location\n"
                                                      "grab(?r - robot, ?e - entity)\n"
                                                      "sf\n"
                                                      "ent - entity\n"
                                                      "n1 - number\n"
                                                      "n2 - number\n"
                                                      "a\n"
                                                      "b\n"
                                                      "loc - number\n"
                                                      "at(?po - physical_object, ?l - location)\n"
                                                      "distance(?po - physical_object, ?l - location) - number", ontology.types);

  ASSERT_FALSE(_condition_fromStr("", ontology).operator bool());
  std::vector<ogp::Parameter> parameters = {_parameter("?target - location", ontology), _parameter("?object - entity", ontology)};
  std::map<ogp::Parameter, ogp::Entity> parametersToEntities = {{_parameter("?target - location", ontology), _entityDeclaration("kitchen - location", ontology)}, {_parameter("?object - location", ontology), _entityDeclaration("chair - entity", ontology)}};
  EXPECT_EQ("location(me)=kitchen & grab(me, chair)",
            _condition_fromStr("location(me)=?target & grab(me, ?object)", ontology, parameters)->clone(&parametersToEntities)->toStr());
  EXPECT_EQ("location(me)=kitchen & grab(me, chair)",
            _condition_fromStr("and(location(me)=?target, grab(me, ?object))", ontology, parameters)->clone(&parametersToEntities)->toStr());
  EXPECT_EQ("location(me)=kitchen & grab(me, chair) & sf",
            _condition_fromStr("and(location(me)=?target, grab(me, ?object), sf)", ontology, parameters)->clone(&parametersToEntities)->toStr());
  EXPECT_EQ("location(me)=kitchen | grab(me, chair) | sf",
            _condition_fromStr("location(me)=?target | grab(me, ?object) | sf", ontology, parameters)->clone(&parametersToEntities)->toStr());
  EXPECT_EQ("location(me)=kitchen | grab(me, chair) | sf",
            _condition_fromStr("or(location(me)=?target, grab(me, ?object), sf)", ontology, parameters)->clone(&parametersToEntities)->toStr());
  EXPECT_EQ("equals(n1, n2 + 3)", _condition_fromStr("equals(n1, n2 + 3)", ontology)->toStr());
  EXPECT_EQ("!sf", _condition_fromStr("!sf", ontology)->toStr());
  EXPECT_EQ("!sf", _condition_fromStr("not(sf)", ontology)->toStr());
  EXPECT_EQ("n1!=2", _condition_fromStr("n1!=2", ontology)->toStr());
  EXPECT_EQ("n1>3", _condition_fromStr("n1>3", ontology)->toStr());
  EXPECT_EQ("n1>3", _condition_fromStr(">(n1, 3)", ontology)->toStr());
  EXPECT_EQ("n1<3", _condition_fromStr("<(n1, 3)", ontology)->toStr());
  EXPECT_EQ("!a=*", _condition_fromStr("=(a, undefined)", ontology)->toStr());
  EXPECT_EQ("!location(me)=*", _condition_fromStr("=(location(me), undefined)", ontology)->toStr());
  EXPECT_EQ("!location(me)=*", _condition_fromStr("=(location(me), undefined)", ontology)->toStr());
  EXPECT_EQ("equals(n1, n2)", _condition_fromStr("=(n1, n2())", ontology)->toStr());
  EXPECT_EQ("location(me)=entrance", _condition_fromStr("=(location(me), entrance)", ontology)->toStr());
  EXPECT_EQ("equals(location(me), loc)", _condition_fromStr("=(location(me), loc())", ontology)->toStr());
  EXPECT_EQ("equals(n1, n2)", _condition_fromStr("equals(n1, n2())", ontology)->toStr());
  EXPECT_EQ("equals(location(me), loc)", _condition_fromStr("equals(location(me), loc())", ontology)->toStr());
  EXPECT_EQ("exists(?l - location, at(self, ?l))", _condition_fromStr("exists(?l - location, at(self, ?l))", ontology)->toStr());
  EXPECT_EQ("exists(?l - location, at(self, ?l) & at(pen, ?l))", _condition_fromStr("exists(?l - location, at(self, ?l) & at(pen, ?l))", ontology)->toStr());
  EXPECT_EQ("!exists(?l - location, at(self, ?l))", _condition_fromStr("not(exists(?l - location, at(self, ?l)))", ontology)->toStr());
  EXPECT_EQ("!(equals(distance(pen, entrance), distance(self, kitchen)))", _condition_fromStr("not(equals(distance(pen, entrance), distance(self, kitchen)))", ontology)->toStr());
  EXPECT_EQ("!(equals(distance(pen, entrance), distance(self, kitchen)))", _condition_fromStr("not(=(distance(pen, entrance), distance(self, kitchen)))", ontology)->toStr());
}

void _test_wsModificationToStr()
{
  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("location\n"
                                             "entity\n"
                                             "robot - physical_object");
  ontology.constants = ogp::SetOfEntities::fromPddl("me - robot\n"
                                                    "sweets - entity\n"
                                                    "target - location", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("location(?r - robot) - location\n"
                                                      "locationEntity(?e - entity) - location\n"
                                                      "checkPointLocation - location\n"
                                                      "grab(?e - entity)\n"
                                                      "is_near(?e - entity, ?r - robot)\n"
                                                      "a\n"
                                                      "n1 - number\n"
                                                      "n2 - number", ontology.types);

  ASSERT_FALSE(_worldStateModification_fromStr("", ontology).operator bool());
  EXPECT_EQ("location(me)=target", _worldStateModification_fromStr("location(me)=target", ontology)->toStr());
  EXPECT_EQ("location(me)=target & grab(sweets)", _worldStateModification_fromStr("location(me)=target & grab(sweets)", ontology)->toStr());
  EXPECT_EQ("location(me)=target & grab(sweets)", _worldStateModification_fromStr("and(location(me)=target, grab(sweets))", ontology)->toStr());
  EXPECT_EQ("assign(n1, n2 + 3)", _worldStateModification_fromStr("assign(n1, n2 + 3)", ontology)->toStr());
  EXPECT_EQ("assign(n1, n2 + 4 - 1)", _worldStateModification_fromStr("set(n1, n2 + 4 - 1)", ontology)->toStr()); // set is depecated
  EXPECT_EQ("increase(n1, 1)", _worldStateModification_fromStr("add(n1, 1)", ontology)->toStr());
  EXPECT_EQ("increase(n1, 1)", _worldStateModification_fromStr("increase(n1, 1)", ontology)->toStr());
  EXPECT_EQ("decrease(n1, 2)", _worldStateModification_fromStr("decrease(n1, 2)", ontology)->toStr());
  EXPECT_EQ("!a", _worldStateModification_fromStr("!a", ontology)->toStr());
  EXPECT_EQ("!a", _worldStateModification_fromStr("not(a)", ontology)->toStr());
  EXPECT_EQ("!n1=*", _worldStateModification_fromStr("assign(n1, undefined)", ontology)->toStr());
  EXPECT_EQ("forall(?e - entity, when(grab(?e), is_near(?e, me)))", _worldStateModification_fromStr("forall(?e - entity, when(grab(?e), is_near(?e, me)))", ontology)->toStr());
  EXPECT_EQ("forall(?e - entity, when(grab(?e), !is_near(?e, me)))", _worldStateModification_fromStr("forall(?e - entity, when(grab(?e), not(is_near(?e, me))))", ontology)->toStr());
  EXPECT_EQ("assign(location(me), locationEntity(sweets))", _worldStateModification_fromStr("assign(location(me), locationEntity(sweets))", ontology)->toStr());
  EXPECT_EQ("assign(location(me), locationEntity(sweets))", _worldStateModification_fromStr("set(location(me), locationEntity(sweets))", ontology)->toStr()); // set is depecated
  EXPECT_EQ("assign(location(me), checkPointLocation)", _worldStateModification_fromStr("assign(location(me), checkPointLocation())", ontology)->toStr()); // c() means that c is a predicate
  EXPECT_EQ("location(me)=target", _worldStateModification_fromStr("assign(location(me), target)", ontology)->toStr());
  EXPECT_EQ("assign(location(me), checkPointLocation)", _worldStateModification_fromStr("set(location(me), checkPointLocation())", ontology)->toStr()); // set is depecated
  EXPECT_EQ("assign(location(me), checkPointLocation)", _worldStateModification_fromStr("set(location(me), checkPointLocation)", ontology)->toStr()); // set is depecated
}

void _test_invertCondition()
{
  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("location\n"
                                             "entity\n"
                                             "robot - physical_object");
  ontology.constants = ogp::SetOfEntities::fromPddl("me self - robot\n"
                                                    "chair - entity\n"
                                                    "pen - physical_object\n"
                                                    "kitchen entrance - location", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("location(?r - robot) - location\n"
                                                      "locationEntity(?e - entity) - location\n"
                                                      "grab(?r - robot, ?e - entity)\n"
                                                      "distance(?po - physical_object, ?l - location) - number\n"
                                                      "se", ontology.types);

  EXPECT_EQ("!location(me)=kitchen | grab(me, chair) | !se", _condition_fromStr("location(me)=kitchen & !grab(me, chair) & se)", ontology)->clone(nullptr, true)->toStr());
  EXPECT_EQ("!location(me)=kitchen & !grab(me, chair) & se", _condition_fromStr("location(me)=kitchen | grab(me, chair) | !se", ontology)->clone(nullptr, true)->toStr());
  EXPECT_EQ("equals(distance(pen, entrance), distance(self, kitchen))", _condition_fromStr("not(=(distance(pen, entrance), distance(self, kitchen)))", ontology)->clone(nullptr, true)->toStr());
}


void _test_checkCondition()
{
  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity");
  ontology.constants = ogp::SetOfEntities::fromPddl("b c d - entity", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("a - entity", ontology.types);

  ogp::WorldState worldState;
  ogp::SetOfEntities objects;
  ogp::GoalStack goalStack;
  std::map<ogp::SetOfEventsId, ogp::SetOfEvents> setOfEvents;
  worldState.addFact(_fact("a=c", ontology), goalStack, setOfEvents, _emptyCallbacks, {}, {}, {});
  EXPECT_TRUE(_condition_fromStr("a!=b", ontology)->isTrue(worldState, ontology.constants, objects));
  EXPECT_FALSE(_condition_fromStr("a!=c", ontology)->isTrue(worldState, ontology.constants, objects));
  EXPECT_EQ(1, worldState.facts().size());
  worldState.addFact(_fact("a!=c", ontology), goalStack, setOfEvents, _emptyCallbacks, {}, {}, {});
  EXPECT_EQ(1, worldState.facts().size());
  EXPECT_FALSE(_condition_fromStr("a!=b", ontology)->isTrue(worldState, ontology.constants, objects));
  EXPECT_TRUE(_condition_fromStr("a!=c", ontology)->isTrue(worldState, ontology.constants, objects));
  worldState.addFact(_fact("a!=b", ontology), goalStack, setOfEvents, _emptyCallbacks, {}, {}, {});
  EXPECT_EQ(2, worldState.facts().size());
  EXPECT_TRUE(_condition_fromStr("a!=b", ontology)->isTrue(worldState, ontology.constants, objects));
  EXPECT_TRUE(_condition_fromStr("a!=c", ontology)->isTrue(worldState, ontology.constants, objects));
  worldState.addFact(_fact("a=d", ontology), goalStack, setOfEvents, _emptyCallbacks, {}, {}, {});
  EXPECT_EQ(1, worldState.facts().size());
  EXPECT_TRUE(_condition_fromStr("a!=b", ontology)->isTrue(worldState, ontology.constants, objects));
  EXPECT_TRUE(_condition_fromStr("a!=c", ontology)->isTrue(worldState, ontology.constants, objects));
  EXPECT_TRUE(_condition_fromStr("a=d", ontology)->isTrue(worldState, ontology.constants, objects));
  EXPECT_FALSE(_condition_fromStr("a!=d", ontology)->isTrue(worldState, ontology.constants, objects));
  worldState.addFact(_fact("a!=c", ontology), goalStack, setOfEvents, _emptyCallbacks, {}, {}, {});
  EXPECT_EQ(1, worldState.facts().size());
  EXPECT_TRUE(_condition_fromStr("a=d", ontology)->isTrue(worldState, ontology.constants, objects));
}


void _automaticallyRemoveGoalsWithAMaxTimeToKeepInactiveEqualTo0()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_advertised + "\n" +
                                                      _fact_beHappy + "\n" +
                                                      _fact_checkedIn, ontology.types);
  ogp::GoalStack goalStack;
  ogp::WorldState worldState;
  ogp::SetOfEntities objects;
  EXPECT_EQ(0u, goalStack.goals().size());
  goalStack.pushBackGoal(_goal(_fact_advertised, ontology), worldState, ontology.constants, objects, {}, 10);
  goalStack.pushBackGoal(_goal(_fact_beHappy, ontology), worldState, ontology.constants, objects, {}, 9);
  EXPECT_EQ(2u, goalStack.goals().size());
  goalStack.pushBackGoal(_goal(_fact_checkedIn, ontology, 0), worldState, ontology.constants, objects, {}, 9);
  EXPECT_EQ(2u, goalStack.goals().size());
  EXPECT_EQ(1u, goalStack.goals().find(9)->second.size());
}


void _maxTimeToKeepInactiveEqualTo0UnderAnAlreadySatisfiedGoal()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + "\n" +
                                                      _fact_checkedIn, ontology.types);
  ogp::GoalStack goalStack;
  ogp::WorldState worldState;
  ogp::SetOfEntities objects;
  EXPECT_EQ(0u, goalStack.goals().size());
  goalStack.pushBackGoal(_goal("persist(!" + _fact_a + ")", ontology), worldState, ontology.constants, objects, {}, 10);
  EXPECT_EQ(1u, goalStack.goals().size());
  goalStack.pushBackGoal(_goal(_fact_checkedIn, ontology, 0), worldState, ontology.constants, objects, {}, 9);
  EXPECT_EQ(2u, goalStack.goals().size());
}

void _noPreconditionGoalImmediatlyReached()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr("be_happy", ontology.types);

  std::map<ogp::ActionId, ogp::Action> actions;
  actions.emplace(_action_goodBoy, ogp::Action({},
                                               _worldStateModification_fromStr("be_happy", ontology)));
  ogp::Domain domain(std::move(actions), ontology);

  ogp::Problem problem;
  _setGoalsForAPriority(problem, {"be_happy"}, ontology);
  EXPECT_EQ(_action_goodBoy, _lookForAnActionToDoStr(problem, domain));
  EXPECT_TRUE(!problem.goalStack.goals().empty());
  EXPECT_TRUE(!_hasFact(problem.worldState, "be_happy", ontology));
  _addFact(problem.worldState, "be_happy", problem.goalStack, ontology);
  EXPECT_TRUE(_hasFact(problem.worldState, "be_happy", ontology));
}


void _removeGoalWhenItIsSatisfiedByAnAction()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr("be_happy", ontology.types);

  std::map<ogp::ActionId, ogp::Action> actions;
  actions.emplace(_action_goodBoy, ogp::Action({},
                                               _worldStateModification_fromStr("be_happy", ontology)));
  ogp::Domain domain(std::move(actions), ontology);

  ogp::Problem problem;
  _setGoalsForAPriority(problem, {"be_happy"}, ontology);

  auto plannerResult = _lookForAnActionToDoThenNotify(problem, domain);
  EXPECT_EQ(_action_goodBoy, plannerResult.actionInvocation.toStr());
  EXPECT_EQ("be_happy", plannerResult.fromGoal->toStr());
  EXPECT_EQ(10, plannerResult.fromGoalPriority);
  EXPECT_TRUE(problem.goalStack.goals().empty());
}


void _removeAnAction()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr("be_happy", ontology.types);

  std::map<ogp::ActionId, ogp::Action> actions;
  actions.emplace(_action_goodBoy, ogp::Action({},
                                               _worldStateModification_fromStr("be_happy", ontology)));
  ogp::Domain domain(std::move(actions), ontology);

  ogp::Problem problem;
  _setGoalsForAPriority(problem, {"be_happy"}, ontology);
  EXPECT_EQ(_action_goodBoy, _lookForAnActionToDoConstStr(problem, domain));
  domain.removeAction(_action_goodBoy);
  EXPECT_EQ("", _lookForAnActionToDoConstStr(problem, domain));
}


void _removeSomeGoals()
{
  const std::string goalGroupId = "greetAndCheckIn";
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr("be_happy\n"
                                                      "checked_in\n"
                                                      "greeted", ontology.types);

  std::map<ogp::ActionId, ogp::Action> actions;
  actions.emplace(_action_greet, ogp::Action({}, _worldStateModification_fromStr("greeted", ontology)));
  actions.emplace(_action_checkIn, ogp::Action({}, _worldStateModification_fromStr("checked_in", ontology)));
  actions.emplace(_action_goodBoy, ogp::Action({}, _worldStateModification_fromStr("be_happy", ontology)));
  ogp::Domain domain(std::move(actions), ontology);

  ogp::Problem problem;
  _setGoalsForAPriority(problem, {"be_happy"}, ontology);
  ogp::GoalsRemovedTracker goalsRemovedTracker(problem.goalStack);
  std::set<std::string> goalsRemoved;
  auto onGoalsRemovedConnection = goalsRemovedTracker.onGoalsRemoved.connectUnsafe([&](const std::set<std::string>& pGoalsRemoved) {
    goalsRemoved = pGoalsRemoved;
  });
  problem.goalStack.pushFrontGoal(_goal("checked_in", ontology, -1, goalGroupId), problem.worldState, ontology.constants, problem.objects, {});
  problem.goalStack.pushFrontGoal(_goal("greeted", ontology, -1, goalGroupId), problem.worldState, ontology.constants, problem.objects, {});
  EXPECT_EQ(_action_greet, _lookForAnActionToDoConstStr(problem, domain));
  EXPECT_EQ(0, goalsRemoved.size());
  EXPECT_TRUE(problem.goalStack.removeGoals(goalGroupId, problem.worldState, ontology.constants, problem.objects, {}));
  EXPECT_EQ(2, goalsRemoved.size());
  EXPECT_FALSE(problem.goalStack.removeGoals(goalGroupId, problem.worldState, ontology.constants, problem.objects, {}));
  EXPECT_EQ(_action_goodBoy, _lookForAnActionToDoConstStr(problem, domain));
  EXPECT_EQ(_action_goodBoy, _lookForAnActionToDoConstStr(problem, domain));
  problem.goalStack.clearGoals(problem.worldState, ontology.constants, problem.objects, {});
  EXPECT_EQ("", _lookForAnActionToDoConstStr(problem, domain));
  onGoalsRemovedConnection.disconnect();
}


void _notifyGoalRemovedWhenItIsImmediatlyRemoved()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + "\n" +
                                                      _fact_b + "\n" +
                                                      _fact_c + "\n" +
                                                      _fact_d + "\n" +
                                                      _fact_e, ontology.types);

  ogp::Problem problem;
  ogp::GoalsRemovedTracker goalsRemovedTracker(problem.goalStack);
  std::set<std::string> goalsRemoved;
  auto onGoalsRemovedConnection = goalsRemovedTracker.onGoalsRemoved.connectUnsafe([&](const std::set<std::string>& pGoalsRemoved) {
    goalsRemoved = pGoalsRemoved;
  });

  problem.goalStack.addGoals({{10, {_goal(_fact_a, ontology)}}}, problem.worldState, ontology.constants, problem.objects, {});

  problem.goalStack.addGoals({{9, {_goal(_fact_b, ontology, 0)}}}, problem.worldState, ontology.constants, problem.objects, {});
  EXPECT_EQ(1u, goalsRemoved.size());
  EXPECT_EQ(_fact_b, *goalsRemoved.begin());
  goalsRemoved.clear();

  problem.goalStack.pushBackGoal(_goal(_fact_c, ontology, 0), problem.worldState, ontology.constants, problem.objects, {}, 9);
  EXPECT_EQ(1u, goalsRemoved.size());
  EXPECT_EQ(_fact_c, *goalsRemoved.begin());
  goalsRemoved.clear();

  problem.goalStack.pushFrontGoal(_goal(_fact_d, ontology, 0), problem.worldState, ontology.constants, problem.objects, {}, 9);
  EXPECT_EQ(1u, goalsRemoved.size());
  EXPECT_EQ(_fact_d, *goalsRemoved.begin());
  goalsRemoved.clear();

  problem.goalStack.pushFrontGoal(_goal(_fact_e, ontology, 0), problem.worldState, ontology.constants, problem.objects, {}, 11);
  EXPECT_EQ(0u, goalsRemoved.size());
  problem.goalStack.changeGoalPriority(_fact_e, 9, true, problem.worldState, ontology.constants, problem.objects, {});
  EXPECT_EQ(1u, goalsRemoved.size());
  EXPECT_EQ(_fact_e, *goalsRemoved.begin());
  goalsRemoved.clear();

  problem.goalStack.setGoals({{10, {_goal(_fact_a, ontology, 0)}}, {9, {_goal(_fact_b, ontology, 0)}}}, problem.worldState, ontology.constants, problem.objects, {});
  EXPECT_EQ(1u, goalsRemoved.size());
  EXPECT_EQ(_fact_b, *goalsRemoved.begin());
  onGoalsRemovedConnection.disconnect();
}



void _handlePreconditionWithNegatedFacts()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_checkedIn + "\n" +
                                                      _fact_greeted + "\n" +
                                                      _fact_userSatisfied, ontology.types);

  std::map<std::string, ogp::Action> actions;
  actions.emplace(_action_greet, ogp::Action(_condition_fromStr("!" + _fact_checkedIn, ontology),
                                             _worldStateModification_fromStr(_fact_greeted, ontology)));
  actions.emplace(_action_joke, ogp::Action(_condition_fromStr("!" + _fact_checkedIn, ontology),
                                            _worldStateModification_fromStr(_fact_userSatisfied, ontology)));
  actions.emplace(_action_goodBoy, ogp::Action(_condition_fromStr(_fact_greeted + " & " + _fact_userSatisfied, ontology),
                                               _worldStateModification_fromStr(_fact_checkedIn, ontology)));
  ogp::Domain domain(std::move(actions), ontology);

  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_fact_checkedIn}, ontology);
  auto actionToDo = _lookForAnActionToDoStr(problem, domain);
  EXPECT_TRUE(actionToDo == _action_greet || actionToDo == _action_joke);
}


void _testWithNegatedAccessibleFacts()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  const std::string action3 = "action3";

  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + "\n" +
                                                      _fact_b + "\n" +
                                                      _fact_c + "\n" +
                                                      _fact_d + "\n" +
                                                      _fact_e, ontology.types);
  std::map<std::string, ogp::Action> actions;
  actions.emplace(action1, ogp::Action(_condition_fromStr(_fact_e + " & !" + _fact_b, ontology),
                                       _worldStateModification_fromStr("!" + _fact_c, ontology)));
  actions.emplace(action2, ogp::Action({}, _worldStateModification_fromStr("!" + _fact_b, ontology)));
  actions.emplace(action3, ogp::Action(_condition_fromStr(_fact_a + " & !" + _fact_c, ontology),
                                       _worldStateModification_fromStr(_fact_d, ontology)));
  ogp::Domain domain(std::move(actions), ontology);

  ogp::Problem problem;
  _setFacts(problem.worldState, {_fact_a, _fact_b, _fact_c}, problem.goalStack, ontology);
  _setGoalsForAPriority(problem, {_fact_d}, ontology);
  EXPECT_EQ("", _lookForAnActionToDoConstStr(problem, domain));
  _addFact(problem.worldState, _fact_e,  problem.goalStack, ontology);
  EXPECT_EQ(action2, _lookForAnActionToDoStr(problem, domain));
}

void _noPlanWithALengthOf2()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_greeted + "\n" +
                                                      _fact_beHappy, ontology.types);

  std::map<std::string, ogp::Action> actions;
  actions.emplace(_action_greet, ogp::Action({}, _worldStateModification_fromStr(_fact_greeted, ontology)));
  actions.emplace(_action_goodBoy, ogp::Action(_condition_fromStr(_fact_greeted, ontology),
                                               _worldStateModification_fromStr(_fact_beHappy, ontology)));
  ogp::Domain domain(std::move(actions), ontology);

  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_fact_beHappy}, ontology);
  EXPECT_EQ(_action_greet, _lookForAnActionToDoConstStr(problem, domain));
  EXPECT_EQ(_action_greet + _sep +
            _action_goodBoy, _solveStr(problem, domain));
}


void _noPlanWithALengthOf3()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_checkedIn + "\n" +
                                                      _fact_greeted + "\n" +
                                                      _fact_beHappy, ontology.types);

  std::map<std::string, ogp::Action> actions;
  actions.emplace(_action_greet, ogp::Action({}, _worldStateModification_fromStr(_fact_greeted, ontology)));
  actions.emplace(_action_checkIn, ogp::Action(_condition_fromStr(_fact_greeted, ontology),
                                               _worldStateModification_fromStr(_fact_checkedIn, ontology)));
  actions.emplace(_action_goodBoy, ogp::Action(_condition_fromStr(_fact_checkedIn, ontology),
                                               _worldStateModification_fromStr(_fact_beHappy, ontology)));
  ogp::Domain domain(std::move(actions), ontology);

  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_fact_beHappy}, ontology);
  EXPECT_EQ(_action_greet, _lookForAnActionToDoConstStr(problem, domain));
  EXPECT_EQ(_action_greet + _sep +
            _action_checkIn + _sep +
            _action_goodBoy, _solveStr(problem, domain));
}

void _2preconditions()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_checkedIn + "\n" +
                                                      _fact_greeted + "\n" +
                                                      _fact_beHappy, ontology.types);

  std::map<std::string, ogp::Action> actions;
  actions.emplace(_action_greet, ogp::Action({}, _worldStateModification_fromStr(_fact_greeted, ontology)));
  actions.emplace(_action_checkIn, ogp::Action({}, _worldStateModification_fromStr(_fact_checkedIn, ontology)));
  actions.emplace(_action_goodBoy, ogp::Action(_condition_fromStr(_fact_greeted + " & " + _fact_checkedIn, ontology),
                                               _worldStateModification_fromStr(_fact_beHappy, ontology)));
  ogp::Domain domain(std::move(actions), ontology);

  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_fact_beHappy}, ontology);
  EXPECT_EQ(_action_checkIn, _lookForAnActionToDoConstStr(problem, domain));
  EXPECT_EQ(_action_checkIn + _sep +
            _action_greet + _sep +
            _action_goodBoy, _solveStr(problem, domain));
}

void _2Goals()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_checkedIn + "\n" +
                                                      _fact_greeted + "\n" +
                                                      _fact_beHappy, ontology.types);

  std::map<std::string, ogp::Action> actions;
  actions.emplace(_action_greet, ogp::Action({}, _worldStateModification_fromStr(_fact_greeted, ontology)));
  actions.emplace(_action_checkIn, ogp::Action({}, _worldStateModification_fromStr(_fact_checkedIn, ontology)));
  actions.emplace(_action_goodBoy, ogp::Action(_condition_fromStr(_fact_greeted + " & " + _fact_checkedIn, ontology),
                                               _worldStateModification_fromStr(_fact_beHappy, ontology)));
  ogp::Domain domain(std::move(actions), ontology);

  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_fact_greeted, _fact_beHappy}, ontology);
  EXPECT_EQ(_action_greet, _lookForAnActionToDoConstStr(problem, domain));
  EXPECT_EQ(_action_greet + _sep +
            _action_checkIn + _sep +
            _action_goodBoy, _solveStr(problem, domain));
}

void _2UnrelatedGoals()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_checkedIn + "\n" +
                                                      _fact_greeted + "\n" +
                                                      _fact_beHappy, ontology.types);

  std::map<std::string, ogp::Action> actions;
  actions.emplace(_action_greet, ogp::Action({}, _worldStateModification_fromStr(_fact_greeted, ontology)));
  actions.emplace(_action_checkIn, ogp::Action({}, _worldStateModification_fromStr(_fact_checkedIn, ontology)));
  actions.emplace(_action_goodBoy, ogp::Action(_condition_fromStr(_fact_checkedIn, ontology),
                                               _worldStateModification_fromStr(_fact_beHappy, ontology)));
  ogp::Domain domain(std::move(actions), ontology);

  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_fact_greeted, _fact_beHappy}, ontology);
  EXPECT_EQ(_action_greet, _lookForAnActionToDoConstStr(problem, domain));
  EXPECT_EQ(_action_greet + _sep +
            _action_checkIn + _sep +
            _action_goodBoy, _solveStr(problem, domain));
}


void _impossibleGoal()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_checkedIn + "\n" +
                                                      _fact_greeted + "\n" +
                                                      _fact_beHappy, ontology.types);

  std::map<std::string, ogp::Action> actions;
  actions.emplace(_action_checkIn, ogp::Action({}, _worldStateModification_fromStr(_fact_checkedIn, ontology)));
  actions.emplace(_action_goodBoy, ogp::Action(_condition_fromStr(_fact_checkedIn, ontology),
                                               _worldStateModification_fromStr(_fact_beHappy, ontology)));
  ogp::Domain domain(std::move(actions), ontology);

  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_fact_greeted, _fact_beHappy}, ontology);
  EXPECT_EQ(_action_checkIn, _lookForAnActionToDoConstStr(problem, domain));
  EXPECT_EQ(_action_checkIn + _sep +
            _action_goodBoy, _solveStr(problem, domain));
}


void _privigelizeTheActionsThatHaveManyPreferedInContext()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_checkedIn + "\n" +
                                                      _fact_greeted + "\n" +
                                                      _fact_hasQrCode + "\n" +
                                                      _fact_hasCheckInPasword + "\n" +
                                                      _fact_beHappy, ontology.types);

  std::map<std::string, ogp::Action> actions;
  actions.emplace(_action_greet, ogp::Action({}, _worldStateModification_fromStr(_fact_greeted, ontology)));
  actions.emplace(_action_checkIn, ogp::Action({}, _worldStateModification_fromStr(_fact_checkedIn, ontology)));
  actions.emplace(_action_checkInWithQrCode, ogp::Action(_condition_fromStr(_fact_hasQrCode, ontology),
                                                         _worldStateModification_fromStr(_fact_checkedIn, ontology),
                                                         _condition_fromStr(_fact_hasQrCode, ontology)));
  actions.emplace(_action_checkInWithPassword, ogp::Action(_condition_fromStr(_fact_hasCheckInPasword, ontology),
                                                           _worldStateModification_fromStr(_fact_checkedIn, ontology),
                                                           _condition_fromStr(_fact_hasCheckInPasword, ontology)));
  actions.emplace(_action_goodBoy, ogp::Action(_condition_fromStr(_fact_greeted + " & " + _fact_checkedIn, ontology),
                                               _worldStateModification_fromStr(_fact_beHappy, ontology)));
  ogp::Domain domain(std::move(actions), ontology);

  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_fact_greeted, _fact_beHappy}, ontology);
  EXPECT_EQ(_action_greet, _lookForAnActionToDoConstStr(problem, domain));
  EXPECT_EQ(_action_greet + _sep +
            _action_checkIn + _sep +
            _action_goodBoy, _solveStrConst(problem, domain));

  _setFacts(problem.worldState, {_fact_hasQrCode}, problem.goalStack, ontology);
  EXPECT_EQ(_action_greet, _lookForAnActionToDoConstStr(problem, domain));
  EXPECT_EQ(_action_greet + _sep +
            _action_checkInWithQrCode + _sep +
            _action_goodBoy, _solveStrConst(problem, domain));

  _setFacts(problem.worldState, {_fact_hasCheckInPasword}, problem.goalStack, ontology);
  EXPECT_EQ(_action_greet, _lookForAnActionToDoConstStr(problem, domain));
  EXPECT_EQ(_action_greet + _sep +
            _action_checkInWithPassword + _sep +
            _action_goodBoy, _solveStrConst(problem, domain));
}

void _preconditionThatCannotBeSolved()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_checkedIn + "\n" +
                                                      _fact_greeted + "\n" +
                                                      _fact_hasQrCode + "\n" +
                                                      _fact_hasCheckInPasword + "\n" +
                                                      _fact_beHappy, ontology.types);

  std::map<std::string, ogp::Action> actions;
  actions.emplace(_action_greet, ogp::Action({}, _worldStateModification_fromStr(_fact_greeted, ontology)));
  actions.emplace(_action_checkInWithQrCode, ogp::Action(_condition_fromStr(_fact_hasQrCode, ontology),
                                                         _worldStateModification_fromStr(_fact_checkedIn, ontology)));
  actions.emplace(_action_checkInWithPassword, ogp::Action(_condition_fromStr(_fact_hasCheckInPasword, ontology),
                                                           _worldStateModification_fromStr(_fact_checkedIn, ontology)));
  actions.emplace(_action_goodBoy, ogp::Action(_condition_fromStr(_fact_greeted + " & " + _fact_checkedIn, ontology),
                                               _worldStateModification_fromStr(_fact_beHappy, ontology)));
  ogp::Domain domain(std::move(actions), ontology);

  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_fact_beHappy}, ontology);
  EXPECT_TRUE(_lookForAnActionToDoStr(problem, domain).empty());
}


void _preferInContext()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_checkedIn + "\n" +
                                                      _fact_greeted + "\n" +
                                                      _fact_hasQrCode + "\n" +
                                                      _fact_hasCheckInPasword + "\n" +
                                                      _fact_beHappy, ontology.types);

  std::map<std::string, ogp::Action> actions;
  actions.emplace(_action_greet, ogp::Action({}, _worldStateModification_fromStr(_fact_greeted, ontology)));
  actions.emplace(_action_checkInWithQrCode, ogp::Action({},
                                                         _worldStateModification_fromStr(_fact_checkedIn, ontology),
                                                         _condition_fromStr(_fact_hasQrCode, ontology)));
  actions.emplace(_action_checkInWithPassword, ogp::Action({},
                                                           _worldStateModification_fromStr(_fact_checkedIn, ontology),
                                                           _condition_fromStr(_fact_hasCheckInPasword, ontology)));
  actions.emplace(_action_goodBoy, ogp::Action(_condition_fromStr(_fact_greeted + " & " + _fact_checkedIn, ontology),
                                               _worldStateModification_fromStr(_fact_beHappy, ontology)));

  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_fact_beHappy}, ontology);
  EXPECT_EQ(_action_greet + _sep +
            _action_checkInWithPassword + _sep +
            _action_goodBoy, _solveStrConst(problem, actions, ontology));

  _setFacts(problem.worldState, {_fact_hasQrCode}, problem.goalStack, ontology);
  EXPECT_EQ(_action_checkInWithQrCode + _sep +
            _action_greet + _sep +
            _action_goodBoy, _solveStrConst(problem, actions, ontology));

  _setFacts(problem.worldState, {_fact_hasCheckInPasword}, problem.goalStack, ontology);
  EXPECT_EQ(_action_checkInWithPassword + _sep +
            _action_greet + _sep +
            _action_goodBoy, _solveStrConst(problem, actions, ontology));

  actions.emplace(_action_checkIn, ogp::Action({}, _worldStateModification_fromStr(_fact_checkedIn, ontology)));
  _setFacts(problem.worldState, {}, problem.goalStack, ontology);
  EXPECT_EQ(_action_checkIn + _sep +
            _action_greet + _sep +
            _action_goodBoy, _solveStrConst(problem, actions, ontology));

  _setFacts(problem.worldState, {_fact_hasQrCode}, problem.goalStack, ontology);
  EXPECT_EQ(_action_checkInWithQrCode + _sep +
            _action_greet + _sep +
            _action_goodBoy, _solveStrConst(problem, actions, ontology));

  _setFacts(problem.worldState, {_fact_hasCheckInPasword}, problem.goalStack, ontology);
  EXPECT_EQ(_action_checkInWithPassword + _sep +
            _action_greet + _sep +
            _action_goodBoy, _solveStrConst(problem, actions, ontology));

  actions.emplace(_action_checkInWithRealPerson, ogp::Action({},
                                                             _worldStateModification_fromStr(_fact_checkedIn, ontology),
                                                             _condition_fromStr("!" + _fact_hasQrCode, ontology)));
  _setFacts(problem.worldState, {}, problem.goalStack, ontology);
  EXPECT_EQ(_action_checkInWithRealPerson + _sep +
            _action_greet + _sep +
            _action_goodBoy, _solveStrConst(problem, actions, ontology));
}


void _preferWhenPreconditionAreCloserToTheRealFacts()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_beginOfConversation + "\n" +
                                                      _fact_checkedIn + "\n" +
                                                      _fact_greeted + "\n" +
                                                      _fact_beHappy + "\n" +
                                                      _fact_presented, ontology.types);

  std::map<std::string, ogp::Action> actions;
  actions.emplace(_action_greet, ogp::Action({},
                                             _worldStateModification_fromStr(_fact_greeted + "&" + _fact_presented, ontology),
                                             _condition_fromStr(_fact_beginOfConversation, ontology)));
  actions.emplace(_action_presentation, ogp::Action({}, _worldStateModification_fromStr(_fact_presented, ontology)));
  actions.emplace(_action_checkIn, ogp::Action({}, _worldStateModification_fromStr(_fact_checkedIn, ontology)));
  actions.emplace(_action_goodBoy, ogp::Action(_condition_fromStr(_fact_presented + "&" + _fact_checkedIn, ontology),
                                               _worldStateModification_fromStr(_fact_beHappy, ontology)));

  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_fact_beHappy}, ontology);
  EXPECT_EQ(_action_checkIn + _sep +
            _action_presentation + _sep +
            _action_goodBoy, _solveStrConst(problem, actions, ontology));

  _setFacts(problem.worldState, {_fact_beginOfConversation}, problem.goalStack, ontology);
  EXPECT_EQ(_action_greet + _sep +
            _action_checkIn + _sep +
            _action_goodBoy, _solveStr(problem, actions, ontology));
}


void _avoidToDo2TimesTheSameActionIfPossble()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_checkedIn + "\n" +
                                                      _fact_greeted + "\n" +
                                                      _fact_beHappy + "\n" +
                                                      _fact_presented, ontology.types);

  std::map<std::string, ogp::Action> actions;
  actions.emplace(_action_greet, ogp::Action({},
                                             _worldStateModification_fromStr(_fact_greeted + "&" + _fact_presented, ontology)));
  actions.emplace(_action_presentation, ogp::Action({},
                                                    _worldStateModification_fromStr(_fact_presented, ontology)));
  actions.emplace(_action_checkIn, ogp::Action({}, _worldStateModification_fromStr(_fact_checkedIn, ontology)));
  actions.emplace(_action_goodBoy, ogp::Action(_condition_fromStr(_fact_presented + "&" + _fact_checkedIn, ontology),
                                               _worldStateModification_fromStr(_fact_beHappy, ontology)));

  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_fact_beHappy}, ontology);
  EXPECT_EQ(_action_checkIn + _sep +
            _action_greet + _sep +
            _action_goodBoy, _solveStrConst(problem, actions, ontology));

  _setGoalsForAPriority(problem, {_fact_greeted}, ontology);
  EXPECT_EQ(_action_greet, _solveStr(problem, actions, ontology));

  _setFacts(problem.worldState, {}, problem.goalStack, ontology);
  _setGoalsForAPriority(problem, {_fact_beHappy}, ontology);
  EXPECT_EQ(_action_checkIn + _sep +
            _action_presentation + _sep +
            _action_goodBoy, _solveStr(problem, actions, ontology));
}


void _takeHistoricalIntoAccount()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_checkedIn + "\n" +
                                                      _fact_greeted + "\n" +
                                                      _fact_beHappy + "\n" +
                                                      _fact_presented, ontology.types);

  std::map<std::string, ogp::Action> actions;
  actions.emplace(_action_greet, ogp::Action({}, _worldStateModification_fromStr(_fact_greeted + "&" + _fact_presented, ontology)));
  actions.emplace(_action_presentation, ogp::Action({}, _worldStateModification_fromStr(_fact_presented, ontology)));
  actions.emplace(_action_checkIn, ogp::Action({}, _worldStateModification_fromStr(_fact_checkedIn, ontology)));
  actions.emplace(_action_goodBoy, ogp::Action(_condition_fromStr(_fact_presented + "&" + _fact_checkedIn, ontology),
                                               _worldStateModification_fromStr(_fact_beHappy, ontology)));

  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_fact_beHappy}, ontology);
  EXPECT_EQ(_action_checkIn + _sep +
            _action_greet + _sep +
            _action_goodBoy, _solveStrConst(problem, actions, ontology, &problem.historical));

  EXPECT_EQ(_action_presentation + _sep +
            _action_checkIn + _sep +
            _action_goodBoy, _solveStrConst(problem, actions, ontology, &problem.historical));
}


void _goDoTheActionThatHaveTheMostPreferInContextValidated()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_checkedIn + "\n" +
                                                      _fact_advertised + "\n" +
                                                      _fact_beHappy + "\n" +
                                                      _fact_is_close, ontology.types);

  std::map<std::string, ogp::Action> actions;
  actions.emplace(_action_advertise, ogp::Action({}, _worldStateModification_fromStr(_fact_advertised, ontology)));
  actions.emplace(_action_checkIn, ogp::Action(_condition_fromStr(_fact_is_close, ontology),
                                               _worldStateModification_fromStr(_fact_checkedIn, ontology),
                                               _condition_fromStr(_fact_is_close, ontology)));
  actions.emplace(_action_goodBoy, ogp::Action(_condition_fromStr(_fact_advertised + "&" + _fact_checkedIn, ontology),
                                               _worldStateModification_fromStr(_fact_beHappy, ontology)));
  ogp::Domain domain(std::move(actions), ontology);

  ogp::Problem problem;
  _setFacts(problem.worldState, {_fact_is_close}, problem.goalStack, ontology);
  _setGoalsForAPriority(problem, {_fact_beHappy}, ontology);
  EXPECT_EQ(_action_checkIn, _lookForAnActionToDoStr(problem, domain));
}


void _checkNotInAPrecondition()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_checkedIn + "\n" +
                                                      _fact_beHappy + "\n" +
                                                      _fact_greeted, ontology.types);

  std::map<std::string, ogp::Action> actions;
  actions.emplace(_action_greet, ogp::Action(_condition_fromStr("!" + _fact_checkedIn, ontology),
                                             _worldStateModification_fromStr(_fact_greeted, ontology)));
  ogp::Domain domain(std::move(actions), ontology);

  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_fact_greeted}, ontology);
  EXPECT_EQ(_action_greet, _lookForAnActionToDoConstStr(problem, domain));
  problem.worldState.modify(&*_worldStateModification_fromStr(_fact_checkedIn, ontology), problem.goalStack,
                            _emptySetOfEvents, _emptyCallbacks, {}, {}, _now);
  EXPECT_EQ(std::string(), _lookForAnActionToDoConstStr(problem, domain));
}


void _checkClearGoalsWhenItsAlreadySatisfied()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_greeted, ontology.types);

  ogp::Problem problem;
  _setFacts(problem.worldState, {_fact_greeted}, problem.goalStack, ontology);
  _setGoalsForAPriority(problem, {_fact_greeted}, ontology);
  EXPECT_EQ(1, problem.goalStack.goals().size());
  ogp::Domain domain;
  _lookForAnActionToDo(problem, domain);
  EXPECT_EQ(0, problem.goalStack.goals().size());
}


void _checkActionHasAFact()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + "\n" +
                                                      _fact_b + "\n" +
                                                      _fact_c + "\n" +
                                                      _fact_d + "\n" +
                                                      _fact_e + "\n" +
                                                      _fact_f + "\n" +
                                                      _fact_g, ontology.types);

  ogp::ProblemModification effect(_worldStateModification_fromStr(_fact_a + " & !" + _fact_b, ontology));
  effect.potentialWorldStateModification = _worldStateModification_fromStr(_fact_c, ontology);
  effect.goalsToAdd[ogp::GoalStack::getDefaultPriority()] = {_goal(_fact_d, ontology)};
  const ogp::Action action(_condition_fromStr(_fact_e, ontology),
                           effect,
                           _condition_fromStr(_fact_f, ontology));
  EXPECT_TRUE(action.hasFact(_fact(_fact_a, ontology)));
  EXPECT_TRUE(action.hasFact(_fact(_fact_b, ontology)));
  EXPECT_TRUE(action.hasFact(_fact(_fact_c, ontology)));
  EXPECT_TRUE(action.hasFact(_fact(_fact_d, ontology)));
  EXPECT_TRUE(action.hasFact(_fact(_fact_e, ontology)));
  EXPECT_TRUE(action.hasFact(_fact(_fact_f, ontology)));
  EXPECT_FALSE(action.hasFact(_fact(_fact_g, ontology)));
}


void _precoditionEqualEffect()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_beHappy, ontology.types);

  std::map<ogp::ActionId, ogp::Action> actions;
  actions.emplace(_action_goodBoy, ogp::Action(_condition_fromStr(_fact_beHappy, ontology),
                                               _worldStateModification_fromStr(_fact_beHappy, ontology)));
  ogp::Domain domain(std::move(actions), ontology);

  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_fact_beHappy}, ontology);
  EXPECT_TRUE(_lookForAnActionToDoStr(problem, domain).empty());
}


void _addGoalEvenForEmptyAction()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a, ontology.types);

  std::map<ogp::ActionId, ogp::Action> actions;
  ogp::Action act1Obj({}, {});
  act1Obj.effect.goalsToAddInCurrentPriority.push_back(_goal(_fact_a, ontology));
  actions.emplace(action1, act1Obj);
  actions.emplace(action2, ogp::Action({}, _worldStateModification_fromStr(_fact_a, ontology)));

  ogp::Domain domain(std::move(actions), ontology);
  ogp::Problem problem;
  EXPECT_TRUE(problem.goalStack.goals().empty());
  ogp::notifyActionDone(problem, domain, _emptyCallbacks, ogp::ActionInvocationWithGoal(action1, std::map<ogp::Parameter, ogp::Entity>(), {}, 0), {});
  EXPECT_FALSE(problem.goalStack.goals().empty());
}

void _circularDependencies()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_greeted + "\n" +
                                                      _fact_checkedIn + "\n" +
                                                      _fact_hasCheckInPasword + "\n" +
                                                      _fact_beHappy, ontology.types);

  std::map<std::string, ogp::Action> actions;
  actions.emplace(_action_greet, ogp::Action({}, _worldStateModification_fromStr(_fact_greeted, ontology)));
  actions.emplace(_action_checkIn, ogp::Action(_condition_fromStr(_fact_greeted, ontology),
                                               _worldStateModification_fromStr(_fact_checkedIn, ontology)));
  actions.emplace("check-in-pwd", ogp::Action(_condition_fromStr(_fact_hasCheckInPasword, ontology),
                                              _worldStateModification_fromStr(_fact_checkedIn, ontology)));
  actions.emplace("inverse-of-check-in-pwd", ogp::Action(_condition_fromStr(_fact_checkedIn, ontology),
                                                         _worldStateModification_fromStr(_fact_hasCheckInPasword, ontology)));
  ogp::Domain domain(std::move(actions), ontology);

  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_fact_beHappy}, ontology);
  EXPECT_EQ("", _lookForAnActionToDoConstStr(problem, domain));
}


void _triggerActionThatRemoveAFact()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_beSad + "\n" +
                                                      _fact_beHappy, ontology.types);

  std::map<std::string, ogp::Action> actions;
  actions.emplace(_action_joke, ogp::Action(_condition_fromStr(_fact_beSad, ontology),
                                            _worldStateModification_fromStr("!" + _fact_beSad, ontology)));
  actions.emplace(_action_goodBoy, ogp::Action(_condition_fromStr("!" + _fact_beSad, ontology),
                                               _worldStateModification_fromStr(_fact_beHappy, ontology)));

  ogp::Historical historical;
  ogp::Problem problem;
  _addFact(problem.worldState, _fact_beSad, problem.goalStack, ontology);
  _setGoalsForAPriority(problem, {_fact_beHappy}, ontology);
  EXPECT_EQ(_action_joke + _sep +
            _action_goodBoy, _solveStr(problem, actions, ontology, _now, &historical));
}


void _actionWithConstantValue()
{
  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("location");
  ontology.constants = ogp::SetOfEntities::fromPddl("kitchen - location", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("place - location", ontology.types);

  std::map<std::string, ogp::Action> actions;
  ogp::Action navigate({}, _worldStateModification_fromStr("place=kitchen", ontology));
  actions.emplace(_action_navigate, navigate);

  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal("place=kitchen", ontology)}, ontology.constants);
  EXPECT_EQ(_action_navigate, _solveStr(problem, actions, ontology));
}


void _actionWithParameterizedValue()
{
  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("location");
  ontology.constants = ogp::SetOfEntities::fromPddl("kitchen - location", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("place - location", ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> parameters(1, _parameter("?target - location", ontology));
  ogp::Action navigate({}, _worldStateModification_fromStr("place=?target", ontology, parameters));
  navigate.parameters = std::move(parameters);
  actions.emplace(_action_navigate, navigate);

  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal("place=kitchen", ontology)}, ontology.constants);
  EXPECT_EQ(_action_navigate + "(?target -> kitchen)", _solveStr(problem, actions, ontology));
}


void _actionWithParameterizedParameter()
{
  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("human");
  ontology.constants = ogp::SetOfEntities::fromPddl("h1 - human", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("isHappy(?h - human)", ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> parameters(1, _parameter("?human - human", ontology));
  ogp::Action joke({}, _worldStateModification_fromStr("isHappy(?human)", ontology, parameters));
  joke.parameters = std::move(parameters);
  actions.emplace(_action_joke, joke);

  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal("isHappy(h1)", ontology)}, ontology.constants);
  EXPECT_EQ(_action_joke + "(?human -> h1)", _solveStr(problem, actions, ontology));
}


void _actionWithParametersInPreconditionsAndEffectsWithoutSolution()
{
  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("human");
  ontology.constants = ogp::SetOfEntities::fromPddl("h1 h2 - human", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("isEngaged(?h - human)\n"
                                                      "isHappy(?h - human)", ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> parameters(1, _parameter("?human - human", ontology));
  ogp::Action joke(_condition_fromStr("isEngaged(?human)", ontology, parameters),
                   _worldStateModification_fromStr("isHappy(?human)", ontology, parameters));
  joke.parameters = std::move(parameters);
  actions.emplace(_action_joke, joke);

  ogp::Problem problem;
  _addFact(problem.worldState, "isEngaged(h2)", problem.goalStack, ontology);
  _setGoalsForAPriority(problem, {_goal("isHappy(h1)", ontology)}, ontology.constants);
  EXPECT_EQ("", _solveStr(problem, actions, ontology));
}

void _actionWithParametersInsideThePath()
{
  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("location");
  ontology.constants = ogp::SetOfEntities::fromPddl("entrance kitchen - location", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("place - location\n"
                                                      "welcomePeople", ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> navigateParameters(1, _parameter("?target - location", ontology));
  ogp::Action navigateAction({},
                             _worldStateModification_fromStr("place=?target", ontology, navigateParameters));
  navigateAction.parameters = std::move(navigateParameters);
  actions.emplace(_action_navigate, navigateAction);

  actions.emplace(_action_welcome,
                  ogp::Action(_condition_fromStr("place=entrance", ontology),
                              _worldStateModification_fromStr("welcomePeople", ontology)));

  ogp::Problem problem;
  _addFact(problem.worldState, "place=kitchen", problem.goalStack, ontology);
  _setGoalsForAPriority(problem, {_goal("welcomePeople", ontology)}, ontology.constants);
  EXPECT_EQ(_action_navigate + "(?target -> entrance)" + _sep +
            _action_welcome, _solveStr(problem, actions, ontology));
  EXPECT_TRUE(_hasFact(problem.worldState, "place=entrance", ontology));
  EXPECT_FALSE(_hasFact(problem.worldState, "place=kitchen", ontology));
}


void _testPersistGoal()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr("welcomePeople", ontology.types);

  std::map<std::string, ogp::Action> actions;
  actions.emplace(_action_welcome, ogp::Action({}, _worldStateModification_fromStr("welcomePeople", ontology)));

  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal("welcomePeople", ontology)}, ontology.constants);
  EXPECT_EQ(1, problem.goalStack.goals().size());
  EXPECT_EQ(_action_welcome, _solveStr(problem, actions, ontology));
  EXPECT_EQ(0, problem.goalStack.goals().size());
  EXPECT_EQ("", _solveStr(problem, actions, ontology));
  EXPECT_EQ(0, problem.goalStack.goals().size());

  problem = ogp::Problem();
  _setGoalsForAPriority(problem, {_goal("persist(welcomePeople)", ontology)}, ontology.constants);
  EXPECT_EQ(1, problem.goalStack.goals().size());
  EXPECT_EQ(_action_welcome, _solveStr(problem, actions, ontology));
  EXPECT_EQ(1, problem.goalStack.goals().size());
  EXPECT_EQ("", _solveStr(problem, actions, ontology));
  EXPECT_EQ(1, problem.goalStack.goals().size());
}


void _testPersistImplyGoal()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_greeted + "\n" +
                                                      _fact_checkedIn, ontology.types);

  std::map<std::string, ogp::Action> actions;
  actions.emplace(_action_greet, ogp::Action({}, _worldStateModification_fromStr(_fact_greeted, ontology)));
  actions.emplace(_action_checkIn, ogp::Action({}, _worldStateModification_fromStr(_fact_checkedIn, ontology)));

  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal("persist(imply(" + _fact_greeted + ", " + _fact_checkedIn + "))", ontology)}, ontology.constants);
  EXPECT_EQ("", _solveStr(problem, actions, ontology));
  _addFact(problem.worldState, _fact_greeted, problem.goalStack, ontology);
  EXPECT_EQ(_action_checkIn, _solveStr(problem, actions, ontology));
}


void _testImplyGoal()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_greeted + "\n" +
                                                      _fact_checkedIn, ontology.types);

  std::map<std::string, ogp::Action> actions;
  actions.emplace(_action_greet, ogp::Action({}, _worldStateModification_fromStr(_fact_greeted, ontology)));
  actions.emplace(_action_checkIn, ogp::Action({}, _worldStateModification_fromStr(_fact_checkedIn, ontology)));

  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal("imply(" + _fact_greeted + ", " + _fact_checkedIn + ")", ontology)}, ontology.constants);
  EXPECT_EQ("", _solveStr(problem, actions, ontology));
  // It is not a persistent goal it is removed
  _addFact(problem.worldState, _fact_greeted, problem.goalStack, ontology);
  EXPECT_EQ("", _solveStr(problem, actions, ontology));
}


void _checkPreviousBugAboutSelectingAnInappropriateAction()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_engagedWithUser + "\n" +
                                                      _fact_userSatisfied + "\n" +
                                                      _fact_robotLearntABehavior + "\n" +
                                                      _fact_advertised, ontology.types);

  std::map<std::string, ogp::Action> actions;
  actions.emplace(_action_askQuestion1, ogp::Action(_condition_fromStr(_fact_engagedWithUser, ontology),
                                                    _worldStateModification_fromStr(_fact_userSatisfied, ontology),
                                                    _condition_fromStr("!" + _fact_robotLearntABehavior, ontology)));
  actions.emplace(_action_checkIn, ogp::Action({},
                                               _worldStateModification_fromStr("!" + _fact_robotLearntABehavior + " & " + _fact_advertised, ontology)));
  ogp::Problem problem;
  _setFacts(problem.worldState, {_fact_engagedWithUser}, problem.goalStack, ontology);
  _setGoalsForAPriority(problem, {"persist(" + _fact_userSatisfied + ")"}, ontology);
  EXPECT_EQ(_action_askQuestion1, _solveStr(problem, actions, ontology));
  _removeFact(problem.worldState, _fact_userSatisfied, problem.goalStack, ontology);
  EXPECT_EQ(_action_askQuestion1, _solveStr(problem, actions, ontology));
}


void _dontLinkActionWithPreferredInContext()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_engagedWithUser + "\n" +
                                                      _fact_userSatisfied + "\n" +
                                                      _fact_checkedIn, ontology.types);

  std::map<std::string, ogp::Action> actions;
  actions.emplace(_action_askQuestion1, ogp::Action({},
                                                    _worldStateModification_fromStr(_fact_userSatisfied, ontology),
                                                    _condition_fromStr(_fact_checkedIn, ontology)));
  actions.emplace(_action_checkIn, ogp::Action(_condition_fromStr(_fact_engagedWithUser, ontology),
                                               _worldStateModification_fromStr(_fact_checkedIn, ontology)));

  ogp::Problem problem;
  _setFacts(problem.worldState, {_fact_engagedWithUser}, problem.goalStack, ontology);
  _setGoalsForAPriority(problem, {_fact_userSatisfied}, ontology);
  EXPECT_EQ(_action_askQuestion1, _solveStr(problem, actions, ontology));
}


void _checkPriorities()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_greeted + "\n" +
                                                      _fact_beHappy + "\n" +
                                                      _fact_checkedIn, ontology.types);

  std::map<ogp::ActionId, ogp::Action> actions;
  actions.emplace(_action_greet, ogp::Action({}, _worldStateModification_fromStr(_fact_greeted, ontology)));
  actions.emplace(_action_checkIn, ogp::Action({}, _worldStateModification_fromStr(_fact_checkedIn, ontology)));
  actions.emplace(_action_goodBoy, ogp::Action({}, _worldStateModification_fromStr(_fact_beHappy, ontology)));

  ogp::Problem problem;
  _setGoals(problem, {{10, {_fact_greeted}}, {9, {_fact_beHappy}}}, ontology);
  EXPECT_EQ(_action_greet + _sep +
            _action_goodBoy, _solveStr(problem, actions, ontology));
}


void _stackablePropertyOfGoals()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_greeted + "\n" +
                                                      _fact_beHappy + "\n" +
                                                      _fact_checkedIn + "\n" +
                                                      _fact_presented, ontology.types);

  std::map<ogp::ActionId, ogp::Action> actions;
  actions.emplace(_action_greet, ogp::Action({}, _worldStateModification_fromStr(_fact_greeted, ontology)));
  actions.emplace(_action_checkIn, ogp::Action({}, _worldStateModification_fromStr(_fact_checkedIn, ontology)));
  actions.emplace(_action_goodBoy, ogp::Action({}, _worldStateModification_fromStr(_fact_beHappy, ontology)));
  actions.emplace(_action_presentation, ogp::Action({}, _worldStateModification_fromStr(_fact_presented, ontology)));

  ogp::Problem problem;
  _setGoals(problem, {{10, {_goal(_fact_greeted, ontology, 0)}}, {9, {_goal(_fact_checkedIn, ontology, 0), _goal(_fact_beHappy, ontology)}}}, ontology.constants);
  EXPECT_EQ(_action_greet + _sep +
            _action_goodBoy, _solveStr(problem, actions, ontology));

  ogp::Problem problem2;
  _setGoals(problem2, {{10, {_goal(_fact_greeted, ontology, 0)}}, {9, {_goal(_fact_checkedIn, ontology, 0), _goal(_fact_beHappy, ontology)}}}, ontology.constants);
  problem2.goalStack.pushFrontGoal(_goal(_fact_presented, ontology), problem2.worldState, ontology.constants, problem2.objects, {}, 10);
  EXPECT_EQ(_action_presentation + _sep +
            _action_goodBoy, _solveStr(problem2, actions, ontology));
}



void _doNotRemoveAGoalWithMaxTimeToKeepInactiveEqual0BelowAGoalWithACondotionNotSatisfied()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_greeted + "\n" +
                                                      _fact_beHappy + "\n" +
                                                      _fact_checkedIn + "\n" +
                                                      _fact_presented, ontology.types);

  std::map<ogp::ActionId, ogp::Action> actions;
  actions.emplace(_action_greet, ogp::Action({}, _worldStateModification_fromStr(_fact_greeted, ontology)));
  actions.emplace(_action_checkIn, ogp::Action({}, _worldStateModification_fromStr(_fact_checkedIn, ontology)));
  actions.emplace(_action_goodBoy, ogp::Action({}, _worldStateModification_fromStr(_fact_beHappy, ontology)));
  actions.emplace(_action_presentation, ogp::Action({}, _worldStateModification_fromStr(_fact_presented, ontology)));

  // Even if _fact_checkedIn has maxTimeToKeepInactive equal to 0, it is not removed because the goal with a higher priority is inactive.
  ogp::Problem problem;
  _setGoals(problem, {{10, {_goal("imply(" + _fact_presented + ", " + _fact_greeted + ")", ontology, 0)}}, {9, {_goal(_fact_checkedIn, ontology, 0), _goal(_fact_beHappy, ontology)}}}, ontology.constants);
  EXPECT_EQ(_action_checkIn + _sep +
            _action_goodBoy, _solveStr(problem, actions, ontology));

  ogp::Problem problem2;
  _addFact(problem2.worldState, _fact_presented, problem2.goalStack, ontology); // The difference here is that the condition of the first goal is satisfied
  _setGoals(problem2, {{10, {_goal("imply(" + _fact_presented + ", " + _fact_greeted + ")", ontology, 0)}}, {9, {_goal(_fact_checkedIn, ontology, 0), _goal(_fact_beHappy, ontology)}}}, ontology.constants);
  EXPECT_EQ(_action_greet + _sep +
            _action_goodBoy, _solveStr(problem2, actions, ontology));


  ogp::Problem problem3;
  _setGoals(problem3, {{10, {_goal("imply(" + _fact_presented + ", " + _fact_greeted + ")", ontology, 0)}}, {9, {_goal(_fact_checkedIn, ontology, 0), _goal(_fact_beHappy, ontology)}}}, ontology.constants);
  _addFact(problem3.worldState, _fact_presented, problem3.goalStack, ontology); // The difference here is that the condition is validated after the add of the goal
  EXPECT_EQ(_action_greet + _sep +
            _action_goodBoy, _solveStr(problem3, actions, ontology));


  ogp::Problem problem4;
  _setGoals(problem4, {{10, {_goal("imply(" + _fact_presented + ", " + _fact_greeted + ")", ontology, 0)}}, {9, {_goal(_fact_checkedIn, ontology, 0), _goal(_fact_beHappy, ontology)}}}, ontology.constants);
  _addFact(problem4.worldState, _fact_presented, problem4.goalStack, ontology); // Here _fact_checkedIn goal shoud be removed from the stack
  _removeFact(problem4.worldState, _fact_presented, problem4.goalStack, ontology); // The difference here is that the condition was validated only punctually
  EXPECT_EQ(_action_goodBoy, _solveStr(problem4, actions, ontology));
}



void _checkMaxTimeToKeepInactiveForGoals()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_greeted + "\n" +
                                                      _fact_checkedIn, ontology.types);

  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<ogp::ActionId, ogp::Action> actions;
  actions.emplace(_action_greet, ogp::Action({}, _worldStateModification_fromStr(_fact_greeted, ontology)));
  actions.emplace(_action_checkIn, ogp::Action({}, _worldStateModification_fromStr(_fact_checkedIn, ontology)));


  ogp::Problem problem;
  _setGoals(problem, {{10, {_goal(_fact_greeted, ontology), _goal(_fact_checkedIn, ontology, 60)}}}, ontology.constants, now);
  EXPECT_EQ(_action_greet + _sep +
            _action_checkIn, _solveStr(problem, actions, ontology, now));


  ogp::Problem problem2;
  _setGoals(problem2, {{10, {_goal(_fact_greeted, ontology), _goal(_fact_checkedIn, ontology, 60)}}}, ontology.constants, now);
  now = std::make_unique<std::chrono::steady_clock::time_point>(*now + std::chrono::seconds(100));
  EXPECT_EQ(_action_greet, _solveStr(problem2, actions, ontology, now));
}



void _changePriorityOfGoal()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_greeted + "\n" +
                                                      _fact_checkedIn + "\n" +
                                                      _fact_userSatisfied, ontology.types);

  std::map<ogp::ActionId, ogp::Action> actions;
  actions.emplace(_action_greet, ogp::Action({}, _worldStateModification_fromStr(_fact_greeted, ontology)));
  actions.emplace(_action_checkIn, ogp::Action({}, _worldStateModification_fromStr(_fact_checkedIn, ontology)));

  std::map<int, std::vector<ogp::Goal>> goalsFromSubscription;
  ogp::Problem problem;
  auto onGoalsChangedConnection = problem.goalStack.onGoalsChanged.connectUnsafe([&](const std::map<int, std::vector<ogp::Goal>>& pGoals) {
    goalsFromSubscription = pGoals;
  });
  ogp::GoalsRemovedTracker goalsRemovedTracker(problem.goalStack);
  std::set<std::string> goalsRemoved;
  auto onGoalsRemovedConnection = goalsRemovedTracker.onGoalsRemoved.connectUnsafe([&](const std::set<std::string>& pGoalsRemoved) {
    goalsRemoved = pGoalsRemoved;
  });

  _setGoals(problem, {{9, {_fact_userSatisfied}}, {10, {_fact_greeted, _fact_checkedIn}}}, ontology);
  {
    auto& goals = problem.goalStack.goals();
    EXPECT_EQ(goalsFromSubscription, goals);
    EXPECT_TRUE(goalsRemoved.empty());
    EXPECT_EQ(1, goals.find(9)->second.size());
    EXPECT_EQ(2, goals.find(10)->second.size());
  }

  problem.goalStack.changeGoalPriority(_fact_checkedIn, 9, true, problem.worldState, ontology.constants, problem.objects, _now);
  {
    auto& goals = problem.goalStack.goals();
    EXPECT_EQ(goalsFromSubscription, goals);
    EXPECT_TRUE(goalsRemoved.empty());
    EXPECT_EQ(2, goals.find(9)->second.size());
    EXPECT_EQ(_fact_checkedIn, goals.find(9)->second[0].toStr());
    EXPECT_EQ(_fact_userSatisfied, goals.find(9)->second[1].toStr());
    EXPECT_EQ(1, goals.find(10)->second.size());
  }

  _setGoals(problem, {{9, {_fact_userSatisfied}}, {10, {_fact_greeted, _fact_checkedIn}}}, ontology);
  problem.goalStack.changeGoalPriority(_fact_checkedIn, 9, false, problem.worldState, ontology.constants, problem.objects, _now);
  {
    auto& goals = problem.goalStack.goals();
    EXPECT_EQ(goalsFromSubscription, goals);
    EXPECT_TRUE(goalsRemoved.empty());
    EXPECT_EQ(2, goals.find(9)->second.size());
    EXPECT_EQ(_fact_userSatisfied, goals.find(9)->second[0].toStr());
    EXPECT_EQ(_fact_checkedIn, goals.find(9)->second[1].toStr());
    EXPECT_EQ(1, goals.find(10)->second.size());
  }

  _setGoals(problem, {{10, {_fact_greeted, _fact_checkedIn}}}, ontology);
  problem.goalStack.changeGoalPriority(_fact_checkedIn, 9, true, problem.worldState, ontology.constants, problem.objects, _now);
  {
    auto& goals = problem.goalStack.goals();
    EXPECT_EQ(goalsFromSubscription, goals);
    EXPECT_EQ(1, goalsRemoved.size());
    EXPECT_EQ(_fact_userSatisfied, *goalsRemoved.begin());
    EXPECT_EQ(1, goals.find(9)->second.size());
    EXPECT_EQ(_fact_checkedIn, goals.find(9)->second[0].toStr());
    EXPECT_EQ(1, goals.find(10)->second.size());
  }
  onGoalsRemovedConnection.disconnect();
  onGoalsChangedConnection.disconnect();
}


void _factChangedNotification()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_greeted + "\n" +
                                                      _fact_checkedIn + "\n" +
                                                      _fact_userSatisfied + "\n" +
                                                      _fact_punctual_p1 + "\n" +
                                                      _fact_beginOfConversation, ontology.types);

  std::map<ogp::ActionId, ogp::Action> actions;
  actions.emplace(_action_greet, ogp::Action({}, _worldStateModification_fromStr(_fact_greeted, ontology)));
  actions.emplace(_action_checkIn, ogp::Action({}, _worldStateModification_fromStr(_fact_checkedIn + "&" + _fact_punctual_p1, ontology)));
  ogp::Domain domain(std::move(actions), ontology);

  std::map<ogp::Fact, bool> factsChangedFromSubscription;
  ogp::Problem problem;
  _addFact(problem.worldState, _fact_beginOfConversation, problem.goalStack, ontology);
  auto factsChangedConnection = problem.worldState.onFactsChanged.connectUnsafe([&](const std::map<ogp::Fact, bool>& pFacts) {
    factsChangedFromSubscription = pFacts;
  });
  std::list<ogp::Fact> punctualFactsAdded;
  auto onPunctualFactsConnection = problem.worldState.onPunctualFacts.connectUnsafe([&](const std::set<ogp::Fact>& pFacts) {
    punctualFactsAdded.insert(punctualFactsAdded.end(), pFacts.begin(), pFacts.end());
  });
  std::list<ogp::Fact> factsAdded;
  auto onFactsAddedConnection = problem.worldState.onFactsAdded.connectUnsafe([&](const std::set<ogp::Fact>& pFacts) {
    factsAdded.insert(factsAdded.end(), pFacts.begin(), pFacts.end());
  });
  std::list<ogp::Fact> factsRemoved;
  auto onFactsRemovedConnection = problem.worldState.onFactsRemoved.connectUnsafe([&](const std::set<ogp::Fact>& pFacts) {
    factsRemoved.insert(factsRemoved.end(), pFacts.begin(), pFacts.end());
  });

  _setGoals(problem, {{9, {_fact_userSatisfied}}, {10, {_fact_greeted, _fact_checkedIn}}}, ontology);
  {
    static const std::map<ogp::Fact, bool> res;
    EXPECT_EQ(res, factsChangedFromSubscription);
  }

  auto plannerResult =_lookForAnActionToDoThenNotify(problem, domain);
  EXPECT_EQ(_action_greet, plannerResult.actionInvocation.actionId);
  {
    std::map<ogp::Fact, bool> res = {{_fact(_fact_beginOfConversation, ontology), true},
                                     {_fact(_fact_greeted, ontology), true}};
    EXPECT_EQ(res, factsChangedFromSubscription);
  }
  {
    static const std::list<ogp::Fact> res;
    EXPECT_EQ(res, punctualFactsAdded);
  }
  {
    std::list<ogp::Fact> res = {_fact(_fact_greeted, ontology)};
    EXPECT_EQ(res, factsAdded);
  }
  factsAdded.clear();
  {
    static const std::list<ogp::Fact> res;
    EXPECT_EQ(res, factsRemoved);
  }

  plannerResult =_lookForAnActionToDoThenNotify(problem, domain);
  EXPECT_EQ(_action_checkIn, plannerResult.actionInvocation.actionId);
  {
    std::map<ogp::Fact, bool> res = {{_fact(_fact_beginOfConversation, ontology), true},
                                     {_fact(_fact_greeted, ontology), true},
                                     {_fact(_fact_checkedIn, ontology), true}};
    EXPECT_EQ(res, factsChangedFromSubscription);
  }
  {
    std::list<ogp::Fact> res = {_fact(_fact_punctual_p1, ontology)};
    EXPECT_EQ(res, punctualFactsAdded);
  }
  {
    std::list<ogp::Fact> res = {_fact(_fact_checkedIn, ontology)};
    EXPECT_EQ(res, factsAdded);
  }
  {
    static const std::list<ogp::Fact> res;
    EXPECT_EQ(res, factsRemoved);
  }
  _removeFact(problem.worldState, _fact_greeted, problem.goalStack, ontology);
  {
    std::map<ogp::Fact, bool> res = {{_fact(_fact_beginOfConversation, ontology), true},
                                     {_fact(_fact_checkedIn, ontology), true}};
    EXPECT_EQ(res, factsChangedFromSubscription);
  }
  {
    std::list<ogp::Fact> res = {_fact(_fact_punctual_p1, ontology)};
    EXPECT_EQ(res, punctualFactsAdded);
  }
  {
    std::list<ogp::Fact> res = {_fact(_fact_checkedIn, ontology)};
    EXPECT_EQ(res, factsAdded);
  }
  {
    std::list<ogp::Fact> res = {_fact(_fact_greeted, ontology)};
    EXPECT_EQ(res, factsRemoved);
  }

  onFactsRemovedConnection.disconnect();
  onPunctualFactsConnection.disconnect();
  onFactsAddedConnection.disconnect();
  factsChangedConnection.disconnect();
}




void _checkEvents()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_headTouched + "\n" +
                                                      _fact_checkedIn, ontology.types);

  std::map<ogp::ActionId, ogp::Action> actions;
  actions.emplace(_action_checkIn, ogp::Action({}, _worldStateModification_fromStr(_fact_checkedIn, ontology)));
  ogp::Domain domain(std::move(actions), ontology);

  ogp::Problem problem;
  EXPECT_EQ("", _solveStr(problem, domain, _now));
  // Event: if (_fact_headTouched) then remove(_fact_headTouched) and addGoal(_fact_checkedIn)
  domain.addSetOfEvents(ogp::Event(_condition_fromStr(_fact_headTouched, ontology),
                                   _worldStateModification_fromStr("!" + _fact_headTouched, ontology),
                                   _emptyParameters, {{{9, {_goal(_fact_checkedIn, ontology)}}}}));
  EXPECT_EQ("", _solveStr(problem, domain, _now));
  auto& setOfEventsMap = domain.getSetOfEvents();
  _addFact(problem.worldState, _fact_headTouched, problem.goalStack, ontology, setOfEventsMap, _now);
  EXPECT_TRUE(!_hasFact(problem.worldState, _fact_headTouched, ontology)); // removed because of the event
  EXPECT_EQ(_action_checkIn, _solveStr(problem, domain, _now));
}



void _checkEventsWithImply()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_headTouched + "\n" +
                                                      _fact_checkedIn + "\n" +
                                                      _fact_userWantsToCheckedIn, ontology.types);

  std::map<ogp::ActionId, ogp::Action> actions;
  actions.emplace(_action_checkIn, ogp::Action({}, _worldStateModification_fromStr(_fact_checkedIn, ontology)));

  // Event: if (_fact_headTouched) then add(_fact_userWantsToCheckedIn) and remove(_fact_headTouched)
  ogp::Domain domain(std::move(actions), ontology,
                     ogp::Event(_condition_fromStr(_fact_headTouched, ontology),
                                _worldStateModification_fromStr(_fact_userWantsToCheckedIn + " & !" + _fact_headTouched, ontology)));

  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal("persist(imply(" + _fact_userWantsToCheckedIn + ", " + _fact_checkedIn + "))", ontology)}, ontology.constants);
  EXPECT_EQ("", _solveStr(problem, domain, _now));
  auto& setOfReferencesMap = domain.getSetOfEvents();
  _addFact(problem.worldState, _fact_headTouched, problem.goalStack, ontology, setOfReferencesMap, _now);
  EXPECT_TRUE(!_hasFact(problem.worldState, _fact_headTouched, ontology)); // removed because of the event
  EXPECT_EQ(_action_checkIn, _solveStr(problem, domain, _now));
}


void _checkEventWithPunctualCondition()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_punctual_headTouched + "\n" +
                                                      _fact_userWantsToCheckedIn, ontology.types);

  std::map<ogp::ActionId, ogp::Action> actions;
  actions.emplace(_action_checkIn, ogp::Action({}, _worldStateModification_fromStr("!" + _fact_userWantsToCheckedIn, ontology)));

  // Event: if (_fact_punctual_headTouched) then add(_fact_userWantsToCheckedIn)
  ogp::Domain domain(std::move(actions), ontology,
                     ogp::Event(_condition_fromStr(_fact_punctual_headTouched, ontology),
                                _worldStateModification_fromStr(_fact_userWantsToCheckedIn, ontology)));

  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal("persist(!" + _fact_userWantsToCheckedIn + ")", ontology)}, ontology.constants);
  EXPECT_EQ("", _solveStr(problem, domain, _now));
  auto& setOfReferencesMap = domain.getSetOfEvents();
  _addFact(problem.worldState, _fact_punctual_headTouched, problem.goalStack, ontology, setOfReferencesMap, _now);
  EXPECT_TRUE(!_hasFact(problem.worldState, _fact_punctual_headTouched, ontology)); // because it is a punctual fact
  EXPECT_EQ(_action_checkIn, _solveStr(problem, domain, _now));
}


void _checkEventAtEndOfAPlan()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_punctual_headTouched + "\n" +
                                                      _fact_userWantsToCheckedIn + "\n" +
                                                      _fact_punctual_checkedIn, ontology.types);

  std::map<ogp::ActionId, ogp::Action> actions;
  actions.emplace(_action_checkIn, ogp::Action({}, _worldStateModification_fromStr(_fact_punctual_checkedIn, ontology)));

  ogp::SetOfEvents setOfEvents;
  setOfEvents.add(ogp::Event(_condition_fromStr(_fact_punctual_headTouched, ontology),
                             _worldStateModification_fromStr(_fact_userWantsToCheckedIn, ontology)));
  setOfEvents.add(ogp::Event(_condition_fromStr(_fact_punctual_checkedIn, ontology),
                             _worldStateModification_fromStr("!" + _fact_userWantsToCheckedIn, ontology)));
  ogp::Domain domain(std::move(actions), ontology, std::move(setOfEvents));

  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal("persist(!" + _fact_userWantsToCheckedIn + ")", ontology)}, ontology.constants);
  EXPECT_EQ("", _solveStr(problem, domain, _now));
  auto& setOfReferencesMap = domain.getSetOfEvents();
  _addFact(problem.worldState, _fact_punctual_headTouched, problem.goalStack, ontology, setOfReferencesMap, _now);
  EXPECT_TRUE(!_hasFact(problem.worldState, _fact_punctual_headTouched, ontology)); // because it is a punctual fact
  EXPECT_EQ(_action_checkIn, _solveStr(problem, domain, _now));
}


void _checkEventInsideAPlan()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";

  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + "\n" +
                                                      _fact_b + "\n" +
                                                      _fact_c + "\n" +
                                                      _fact_d, ontology.types);

  std::map<ogp::ActionId, ogp::Action> actions;
  actions.emplace(action1, ogp::Action({}, _worldStateModification_fromStr(_fact_a, ontology)));
  actions.emplace(action2, ogp::Action(_condition_fromStr(_fact_c, ontology), _worldStateModification_fromStr(_fact_d, ontology)));
  ogp::Domain domain(std::move(actions), ontology);

  {
    ogp::SetOfEvents setOfEvents;
    setOfEvents.add(ogp::Event(_condition_fromStr(_fact_a, ontology),
                               _worldStateModification_fromStr(_fact_b, ontology)));
    setOfEvents.add(ogp::Event(_condition_fromStr(_fact_b + "&" + _fact_d, ontology),
                               _worldStateModification_fromStr(_fact_c, ontology)));
    domain.addSetOfEvents(std::move(setOfEvents));
  }

  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal(_fact_d, ontology)}, ontology.constants);
  EXPECT_EQ("", _solveStrConst(problem, domain));

  domain.addSetOfEvents(ogp::Event(_condition_fromStr(_fact_b, ontology),
                                   _worldStateModification_fromStr(_fact_c, ontology)));

  EXPECT_EQ(action1 + _sep + action2, _solveStrConst(problem, domain)); // check with a copy of the problem
  EXPECT_TRUE(!_hasFact(problem.worldState, _fact_a, ontology));
  EXPECT_TRUE(!_hasFact(problem.worldState, _fact_b, ontology));
  EXPECT_TRUE(!_hasFact(problem.worldState, _fact_c, ontology));
  EXPECT_TRUE(!_hasFact(problem.worldState, _fact_d, ontology));
  EXPECT_EQ(action1 + _sep + action2, _solveStr(problem, domain));
  EXPECT_TRUE(_hasFact(problem.worldState, _fact_a, ontology));
  EXPECT_TRUE(_hasFact(problem.worldState, _fact_b, ontology));
  EXPECT_TRUE(_hasFact(problem.worldState, _fact_c, ontology));
  EXPECT_TRUE(_hasFact(problem.worldState, _fact_d, ontology));
}


void _checkEventThatAddAGoal()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  const std::string action3 = "action3";
  const std::string action4 = "action4";
  const std::string action5 = "action5";

  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + "\n" +
                                                      _fact_b + "\n" +
                                                      _fact_c + "\n" +
                                                      _fact_d + "\n" +
                                                      _fact_e + "\n" +
                                                      _fact_f + "\n" +
                                                      _fact_g, ontology.types);

  std::map<ogp::ActionId, ogp::Action> actions;
  actions.emplace(action1, ogp::Action({}, _worldStateModification_fromStr(_fact_a, ontology)));
  actions.emplace(action2, ogp::Action(_condition_fromStr(_fact_c, ontology),
                                       _worldStateModification_fromStr(_fact_d, ontology)));
  actions.emplace(action3, ogp::Action(_condition_fromStr(_fact_c + "&" + _fact_f, ontology),
                                       _worldStateModification_fromStr(_fact_e, ontology)));
  actions.emplace(action4, ogp::Action(_condition_fromStr(_fact_b, ontology),
                                       _worldStateModification_fromStr(_fact_f, ontology)));
  actions.emplace(action5, ogp::Action(_condition_fromStr(_fact_b, ontology),
                                       _worldStateModification_fromStr(_fact_g, ontology)));
  ogp::SetOfEvents setOfEvents;
  setOfEvents.add(ogp::Event(_condition_fromStr(_fact_a, ontology),
                             _worldStateModification_fromStr(_fact_b, ontology),
                             _emptyParameters, {{ogp::GoalStack::getDefaultPriority(), {_goal(_fact_e, ontology)}}}));
  setOfEvents.add(ogp::Event(_condition_fromStr(_fact_b, ontology),
                             _worldStateModification_fromStr(_fact_c, ontology)));
  ogp::Domain domain(std::move(actions), ontology, std::move(setOfEvents));

  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal("imply(" + _fact_g + ", " + _fact_d + ")", ontology)}, ontology.constants);
  EXPECT_EQ("", _solveStrConst(problem, domain));
  auto& setOfEventsMap = domain.getSetOfEvents();
  _addFact(problem.worldState, _fact_g, problem.goalStack, ontology, setOfEventsMap, _now);
  EXPECT_EQ(action1 + _sep + action4 + _sep + action3 + _sep + action2, _solveStr(problem, domain));
}


void _testGetNotSatisfiedGoals()
{
  auto goal1 = "persist(!" + _fact_a + ")";
  auto goal2 = "persist(" + _fact_b + ")";
  auto goal3 = "imply(" + _fact_c + ", " + _fact_d + ")";
  auto goal4 = "persist(imply(!" + _fact_c + ", " + _fact_d + "))";

  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + "\n" +
                                                      _fact_b + "\n" +
                                                      _fact_c + "\n" +
                                                      _fact_d, ontology.types);

  ogp::Problem problem;
  _addGoalsForAPriority(problem, {goal1}, ontology, {}, ogp::GoalStack::getDefaultPriority() + 1);
  _addGoalsForAPriority(problem, {goal2, goal3, goal4}, ontology);

  EXPECT_EQ(goal1 + ", " + goal2 + ", " + goal3 + ", " + goal4, ogp::printGoals(problem.goalStack.goals()));
  EXPECT_EQ(goal2 + ", " + goal4, ogp::printGoals(problem.goalStack.getNotSatisfiedGoals(problem.worldState, ontology.constants, problem.objects)));
  _addFact(problem.worldState, _fact_a, problem.goalStack, ontology);
  EXPECT_EQ(goal1 + ", " + goal2 + ", " + goal4, ogp::printGoals(problem.goalStack.getNotSatisfiedGoals(problem.worldState, ontology.constants, problem.objects)));
  _addFact(problem.worldState, _fact_c, problem.goalStack, ontology);
  EXPECT_EQ(goal1 + ", " + goal2 + ", " + goal3, ogp::printGoals(problem.goalStack.getNotSatisfiedGoals(problem.worldState, ontology.constants, problem.objects)));
  _addFact(problem.worldState, _fact_d, problem.goalStack, ontology);
  EXPECT_EQ(goal1 + ", " + goal2, ogp::printGoals(problem.goalStack.getNotSatisfiedGoals(problem.worldState, ontology.constants, problem.objects)));
  _removeFact(problem.worldState, _fact_a, problem.goalStack, ontology);
  EXPECT_EQ(goal2, ogp::printGoals(problem.goalStack.getNotSatisfiedGoals(problem.worldState, ontology.constants, problem.objects)));
  _addFact(problem.worldState, _fact_b, problem.goalStack, ontology);
  EXPECT_EQ("", ogp::printGoals(problem.goalStack.getNotSatisfiedGoals(problem.worldState, ontology.constants, problem.objects)));
  _removeFact(problem.worldState, _fact_d, problem.goalStack, ontology);
  EXPECT_EQ(goal3, ogp::printGoals(problem.goalStack.getNotSatisfiedGoals(problem.worldState, ontology.constants, problem.objects)));
}



void _testGoalUnderPersist()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";

  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + "\n" +
                                                      _fact_b + "\n" +
                                                      _fact_c + "\n" +
                                                      _fact_d + "\n" +
                                                      _fact_e, ontology.types);

  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  std::map<std::string, ogp::Action> actions;
  actions.emplace(action1, ogp::Action({}, _worldStateModification_fromStr(_fact_b, ontology)));
  actions.emplace(action2, ogp::Action({}, _worldStateModification_fromStr(_fact_c, ontology)));
  ogp::Domain domain(std::move(actions), ontology);

  {
    ogp::Problem problem;
    _addGoalsForAPriority(problem, {"persist(!" + _fact_a + ")"}, ontology, now, ogp::GoalStack::getDefaultPriority() + 2);
    _addGoalsForAPriority(problem, {_goal(_fact_b, ontology, 0)}, ontology.constants, now, ogp::GoalStack::getDefaultPriority());
    EXPECT_EQ(action1, _lookForAnActionToDoStr(problem, domain));
  }

  {
    ogp::Problem problem;
    _addGoalsForAPriority(problem, {"persist(!" + _fact_a + ")"}, ontology, now, ogp::GoalStack::getDefaultPriority() + 2);
    _addGoalsForAPriority(problem, {_goal(_fact_b, ontology, 0)}, ontology.constants, now, ogp::GoalStack::getDefaultPriority());
    problem.goalStack.removeFirstGoalsThatAreAlreadySatisfied(problem.worldState, ontology.constants, problem.objects, {});
    EXPECT_EQ(action1, _lookForAnActionToDoStr(problem, domain));
  }

  {
    ogp::Problem problem;
    _addGoalsForAPriority(problem, {"persist(!" + _fact_a + ")"}, ontology, now, ogp::GoalStack::getDefaultPriority() + 2);
    _addGoalsForAPriority(problem, {_goal(_fact_b, ontology, 0)}, ontology.constants, now, ogp::GoalStack::getDefaultPriority());
    _addFact(problem.worldState, _fact_a, problem.goalStack, ontology);
    EXPECT_EQ("", _lookForAnActionToDoStr(problem, domain));
  }

  {
    ogp::Problem problem;
    _addGoalsForAPriority(problem, {"persist(" + _fact_c + ")"}, ontology, now, ogp::GoalStack::getDefaultPriority() + 2);
    EXPECT_EQ(action2, _lookForAnActionToDoThenNotify(problem, domain).actionInvocation.actionId);
    EXPECT_EQ("", _lookForAnActionToDoStr(problem, domain));
    _addGoalsForAPriority(problem, {_goal(_fact_b, ontology, 0)}, ontology.constants, now, ogp::GoalStack::getDefaultPriority());
    EXPECT_EQ(action1, _lookForAnActionToDoStr(problem, domain));
  }

  {
    ogp::Problem problem;
    _addGoalsForAPriority(problem, {"persist(" + _fact_c + ")"}, ontology, now, ogp::GoalStack::getDefaultPriority() + 2);
    EXPECT_EQ(action2, _lookForAnActionToDoThenNotify(problem, domain).actionInvocation.actionId);
    problem.goalStack.removeFirstGoalsThatAreAlreadySatisfied(problem.worldState, ontology.constants, problem.objects, {});
    _addGoalsForAPriority(problem, {_goal(_fact_b, ontology, 0)}, ontology.constants, now, ogp::GoalStack::getDefaultPriority());
    EXPECT_EQ(action1, _lookForAnActionToDoStr(problem, domain));
  }

  {
    ogp::Problem problem;
    _addGoalsForAPriority(problem, {_fact_c}, ontology, now, ogp::GoalStack::getDefaultPriority() + 2);
    EXPECT_EQ(action2, _lookForAnActionToDoThenNotify(problem, domain).actionInvocation.actionId);
    problem.goalStack.removeFirstGoalsThatAreAlreadySatisfied(problem.worldState, ontology.constants, problem.objects, {});
    _addGoalsForAPriority(problem, {_goal(_fact_b, ontology, 0)}, ontology.constants, now, ogp::GoalStack::getDefaultPriority());
    EXPECT_EQ(action1, _lookForAnActionToDoStr(problem, domain));
  }

  {
    ogp::Problem problem;
    problem.goalStack.pushBackGoal({_goal("persist(!" + _fact_e + ")", ontology)}, problem.worldState, ontology.constants, problem.objects, now, ogp::GoalStack::getDefaultPriority() + 2);
    problem.goalStack.pushBackGoal(_goal(_fact_c, ontology), problem.worldState, ontology.constants, problem.objects, now, ogp::GoalStack::getDefaultPriority() + 2);
    EXPECT_EQ(action2, _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.actionId);
    problem.goalStack.removeFirstGoalsThatAreAlreadySatisfied(problem.worldState, ontology.constants, problem.objects, now);
    _addGoalsForAPriority(problem, {_goal(_fact_b, ontology, 1)}, ontology.constants, now, ogp::GoalStack::getDefaultPriority());
    EXPECT_EQ(action1, _lookForAnActionToDoStr(problem, domain, now));
    EXPECT_EQ(action1, _lookForAnActionToDoStr(problem, domain, now));

    _removeFact(problem.worldState, _fact_c, problem.goalStack, ontology);
    problem.goalStack.pushBackGoal(_goal(_fact_c, ontology), problem.worldState, ontology.constants, problem.objects, now, ogp::GoalStack::getDefaultPriority() + 2);
    auto plannerResult = _lookForAnActionToDo(problem, domain, now);
    EXPECT_EQ(action2, plannerResult.actionInvocation.actionId);

    now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now() + std::chrono::minutes(5));
    ogp::notifyActionDone(problem, domain, _emptyCallbacks, plannerResult, now);

    problem.goalStack.removeFirstGoalsThatAreAlreadySatisfied(problem.worldState, ontology.constants, problem.objects, now);
    EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, now).actionInvocation.actionId); // Not action1 because it was inactive for too long
  }

}


void _checkLinkedEvents()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_userWantsToCheckedIn + "\n" +
                                                      _fact_a + "\n" +
                                                      _fact_punctual_p1 + "\n" +
                                                      _fact_punctual_p2 + "\n" +
                                                      _fact_punctual_p3 + "\n" +
                                                      _fact_punctual_p4 + "\n" +
                                                      _fact_punctual_p5 + "\n" +
                                                      _fact_e, ontology.types);

  ogp::SetOfEvents setOfEvents;
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal("persist(!" + _fact_userWantsToCheckedIn + ")", ontology)}, ontology.constants);
  setOfEvents.add(ogp::Event(_condition_fromStr(_fact_punctual_p2, ontology),
                             _worldStateModification_fromStr(_fact_a, ontology)));
  setOfEvents.add(ogp::Event(_condition_fromStr(_fact_punctual_p5, ontology),
                             _worldStateModification_fromStr(_fact_punctual_p2 + "&" + _fact_punctual_p3, ontology)));
  setOfEvents.add(ogp::Event(_condition_fromStr(_fact_punctual_p4, ontology),
                             _worldStateModification_fromStr(_fact_punctual_p5 + "&" + _fact_punctual_p1, ontology)));

  std::map<ogp::SetOfEventsId, ogp::SetOfEvents> setOfEventsMap = {{"soe", setOfEvents}};
  EXPECT_FALSE(_hasFact(problem.worldState, _fact_a, ontology));
  _addFact(problem.worldState, _fact_punctual_p4, problem.goalStack, ontology, setOfEventsMap, _now);
  EXPECT_TRUE(_hasFact(problem.worldState, _fact_a, ontology));
}



void _oneStepTowards()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_checkedIn + "\n" +
                                                      _fact_greeted + "\n" +
                                                      _fact_a + "\n" +
                                                      _fact_b + "\n" +
                                                      _fact_beHappy, ontology.types);

  std::map<ogp::ActionId, ogp::Action> actions;
  ogp::ProblemModification greetPbModification;
  greetPbModification.potentialWorldStateModification = _worldStateModification_fromStr(_fact_greeted, ontology);
  actions.emplace(_action_greet, ogp::Action({}, greetPbModification));
  actions.emplace(_action_goodBoy, ogp::Action({}, _worldStateModification_fromStr(_fact_beHappy, ontology)));
  static const std::string actionb = "actionb";
  actions.emplace(actionb, ogp::Action({}, _worldStateModification_fromStr(_fact_b, ontology)));
  ogp::Domain domain(std::move(actions), ontology);

  ogp::Problem problem;
  auto implyGoal = _goal("persist(imply(" + _fact_a + ", " + _fact_b + "))", ontology, 0);
  _setGoals(problem, {{11, {implyGoal}},
                      {10, {_goal("oneStepTowards(" + _fact_greeted + ")", ontology, 0)}},
                      {9, {_goal(_fact_checkedIn, ontology, 0), _goal(_fact_beHappy, ontology)}}}, ontology.constants, _now);
  problem.goalStack.removeFirstGoalsThatAreAlreadySatisfied(problem.worldState, ontology.constants, problem.objects, _now);
  EXPECT_EQ(_action_greet, _lookForAnActionToDoStr(problem, domain, _now));
  EXPECT_EQ(_action_greet, _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.actionId);
  EXPECT_EQ(_action_goodBoy, _lookForAnActionToDoStr(problem, domain, _now));
  EXPECT_EQ(_action_goodBoy, _lookForAnActionToDoStr(problem, domain, _now));
  _addFact(problem.worldState, _fact_a, problem.goalStack, ontology);
  EXPECT_EQ(actionb, _lookForAnActionToDoStr(problem, domain, _now));
  EXPECT_EQ(actionb + _sep + _action_goodBoy, _solveStrConst(problem, domain));
  EXPECT_EQ(implyGoal.toStr() + _sep + _fact_beHappy, _getGoalsDoneDuringAPlannificationConst(problem, domain));
}


void _infrenceLinksFromManyEventsSets()
{
  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + "\n" +
                                                      _fact_b + "\n" +
                                                      _fact_c + "\n" +
                                                      _fact_d + "\n" +
                                                      _fact_punctual_p2 + "\n" +
                                                      _fact_punctual_p1, ontology.types);

  const std::string action1 = "action1";
  const std::string action2 = "action2";
  std::map<ogp::ActionId, ogp::Action> actions;
  ogp::ProblemModification pbModification;
  pbModification.potentialWorldStateModification = _worldStateModification_fromStr(_fact_d, ontology);
  actions.emplace(action1, ogp::Action({}, pbModification));
  actions.emplace(action2, ogp::Action({}, _worldStateModification_fromStr(_fact_c, ontology)));
  ogp::Domain domain(std::move(actions), ontology);

  EXPECT_TRUE(ogp::GoalStack::getDefaultPriority() >= 1);
  auto lowPriority = ogp::GoalStack::getDefaultPriority() - 1;
  domain.addSetOfEvents(ogp::Event(_condition_fromStr(_fact_punctual_p2, ontology),
                                   {}, _emptyParameters,
                                   {{lowPriority, {_goal("oneStepTowards(" + _fact_d + ")", ontology)}}}));
  ogp::Problem problem;
  _setGoals(problem, {{lowPriority, {_goal("oneStepTowards(" + _fact_d + ")", ontology, 0)}}}, ontology.constants);

  {
    ogp::SetOfEvents setOfEvents2;
    setOfEvents2.add(ogp::Event(_condition_fromStr(_fact_punctual_p1, ontology),
                                _worldStateModification_fromStr(_fact_b + "&" + _fact_punctual_p2, ontology)));
    setOfEvents2.add(ogp::Event(_condition_fromStr(_fact_b, ontology),
                                {}, _emptyParameters,
                                {{ogp::GoalStack::getDefaultPriority(), {_goal("oneStepTowards(" + _fact_c + ")", ontology)}}}));
    domain.addSetOfEvents(setOfEvents2);
  }

  auto& setOfEventsMap = domain.getSetOfEvents();
  EXPECT_EQ(action1, _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.actionId);
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.actionId);
  _addFact(problem.worldState, _fact_punctual_p1, problem.goalStack, ontology, setOfEventsMap, _now);
  EXPECT_EQ(action2, _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.actionId);
  EXPECT_EQ(action1, _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.actionId);
}


void _factValueModification()
{
  const std::string action1 = "action1";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity");
  ontology.constants = ogp::SetOfEntities::fromPddl("a b - entity", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + " - entity\n" +
                                                      _fact_b, ontology.types);

  std::map<ogp::ActionId, ogp::Action> actions;
  actions.emplace(action1, ogp::Action({}, _worldStateModification_fromStr("!" + _fact_b, ontology)));
  ogp::Domain domain(std::move(actions), ontology);

  ogp::Problem problem;
  _setGoals(problem, {{10, {_goal("persist(imply(" + _fact_a + "=a, " + "!" + _fact_b + "))", ontology, 0)}}}, ontology.constants);

  _addFact(problem.worldState, _fact_b, problem.goalStack, ontology);
  EXPECT_EQ("", _lookForAnActionToDoStr(problem, domain, _now));
  _addFact(problem.worldState, _fact_a + "=a", problem.goalStack, ontology);
  EXPECT_EQ(action1, _lookForAnActionToDoStr(problem, domain, _now));
  _addFact(problem.worldState, _fact_a + "=b", problem.goalStack, ontology);
  EXPECT_EQ("", _lookForAnActionToDoStr(problem, domain, _now));
}


void _removeGoaWhenAnActionFinishesByAddingNewGoals()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";

  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + "\n" +
                                                      _fact_b, ontology.types);

  std::map<ogp::ActionId, ogp::Action> actions;
  ogp::ProblemModification wm(_worldStateModification_fromStr(_fact_a, ontology));
  wm.goalsToAddInCurrentPriority.push_back(_goal(_fact_b, ontology, 0));
  actions.emplace(action1, ogp::Action({}, wm));
  actions.emplace(action2, ogp::Action({}, _worldStateModification_fromStr(_fact_b, ontology)));
  ogp::Domain domain(std::move(actions), ontology);

  ogp::Problem problem;
  ogp::GoalsRemovedTracker goalsRemovedTracker(problem.goalStack);
  std::set<std::string> goalsRemoved;
  auto onGoalsRemovedConnection = goalsRemovedTracker.onGoalsRemoved.connectUnsafe([&](const std::set<std::string>& pGoalsRemoved) {
    goalsRemoved = pGoalsRemoved;
  });
  _setGoals(problem, {{27, {_fact_a}}}, ontology, _now);

  EXPECT_EQ(action1, _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.actionId);
  EXPECT_EQ(1u, goalsRemoved.size());
  EXPECT_EQ(_fact_a, *goalsRemoved.begin());
  onGoalsRemovedConnection.disconnect();

  // The only remaning goal is the goal we just added
  EXPECT_EQ(1u, problem.goalStack.goals().size());
  EXPECT_EQ(27, problem.goalStack.goals().begin()->first);
  EXPECT_EQ(1u, problem.goalStack.goals().begin()->second.size());
  EXPECT_EQ(_fact_b, problem.goalStack.goals().begin()->second.begin()->toStr());
}


void _setWsModification()
{
  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("loc_type\n"
                                             "entity");
  ontology.constants = ogp::SetOfEntities::fromPddl("me object - entity\n"
                                                    "corridor kitchen - loc_type", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("location(?o - entity) - loc_type", ontology.types);

  std::map<std::string, ogp::Action> actions;
  ogp::Action navAction({}, _worldStateModification_fromStr("set(location(me), location(object))", ontology));
  actions.emplace(_action_navigate, navAction);

  ogp::Problem problem;
  _addFact(problem.worldState, "location(me)=corridor", problem.goalStack, ontology);
  _addFact(problem.worldState, "location(object)=kitchen", problem.goalStack, ontology);
  _setGoalsForAPriority(problem, {_goal("location(me)=kitchen", ontology)}, ontology.constants);
  EXPECT_EQ(_action_navigate, _solveStr(problem, actions, ontology));
}


void _forAllWsModification()
{
  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("loc_type\n"
                                             "entity");
  ontology.constants = ogp::SetOfEntities::fromPddl("me object1 object2 - entity\n"
                                                    "corridor kitchen - loc_type", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("location(?o - entity) - loc_type\n"
                                                      "grab(?e - entity, ?o - entity)", ontology.types);

  const std::string action1 = "action1";
  std::map<std::string, ogp::Action> actions;
  ogp::Action navAction({}, _worldStateModification_fromStr("forall(?obj - entity, grab(me, ?obj), set(location(?obj), location(me)))", ontology));
  actions.emplace(action1, navAction);

  ogp::Problem problem;
  _addFact(problem.worldState, "location(me)=corridor", problem.goalStack, ontology);
  _addFact(problem.worldState, "location(object1)=kitchen", problem.goalStack, ontology);
  _addFact(problem.worldState, "grab(me, object1)", problem.goalStack, ontology);
  _addFact(problem.worldState, "grab(me, object2)", problem.goalStack, ontology);

  _setGoalsForAPriority(problem, {_goal("location(object2)=corridor", ontology)}, ontology.constants);
  EXPECT_EQ(action1, _solveStr(problem, actions, ontology));
}


void _actionNavigationAndGrabObjectWithParameters()
{
  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("loc_type\n"
                                             "entity");
  ontology.constants = ogp::SetOfEntities::fromPddl("me sweets - entity\n"
                                                    "corridor kitchen - loc_type", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("location(?o - entity) - loc_type\n"
                                                      "grab(?e - entity, ?o - entity)", ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> navParameters(1, _parameter("?targetLocation - loc_type", ontology));
  ogp::Action navAction({}, _worldStateModification_fromStr("location(me)=?targetLocation", ontology, navParameters));
  navAction.parameters = std::move(navParameters);
  actions.emplace(_action_navigate, navAction);

  std::vector<ogp::Parameter> grabParameters(1, _parameter("?object - entity", ontology));
  ogp::Action grabAction(_condition_fromStr("equals(location(me), location(?object))", ontology, grabParameters),
                         _worldStateModification_fromStr("grab(me, ?object)", ontology, grabParameters));
  grabAction.parameters = std::move(grabParameters);
  actions.emplace(_action_grab, grabAction);

  ogp::Problem problem;
  _addFact(problem.worldState, "location(me)=corridor", problem.goalStack, ontology);
  _addFact(problem.worldState, "location(sweets)=kitchen", problem.goalStack, ontology);
  const auto& setOfFacts = problem.worldState.factsMapping();
  EXPECT_EQ("kitchen", setOfFacts.getFluentValue(_fact("location(sweets)=*", ontology))->value);
  _setGoalsForAPriority(problem, {_goal("grab(me, sweets)", ontology)}, ontology.constants);
  EXPECT_EQ(_action_navigate + "(?targetLocation -> kitchen), " + _action_grab + "(?object -> sweets)",
            _solveStr(problem, actions, ontology));
}


void _actionNavigationAndGrabObjectWithParameters2()
{
  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("loc_type\n"
                                             "entity");
  ontology.constants = ogp::SetOfEntities::fromPddl("me sweets - entity\n"
                                                    "corridor kitchen - loc_type", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("location(?o - entity) - loc_type\n"
                                                      "grab(?e - entity, ?o - entity)", ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> navParameters(1, _parameter("?targetLocation - loc_type", ontology));
  ogp::Action navAction({}, _worldStateModification_fromStr("location(me)=?targetLocation", ontology, navParameters));
  navAction.parameters = std::move(navParameters);
  actions.emplace(_action_navigate, navAction);

  std::vector<ogp::Parameter> grabParameters(1, _parameter("?object - entity", ontology));
  ogp::Action grabAction(_condition_fromStr("equals(location(?object), location(me))", ontology, grabParameters),
                         _worldStateModification_fromStr("grab(me, ?object)", ontology, grabParameters));
  grabAction.parameters = std::move(grabParameters);
  actions.emplace(_action_grab, grabAction);

  ogp::Problem problem;
  _addFact(problem.worldState, "location(me)=corridor", problem.goalStack, ontology);
  _addFact(problem.worldState, "location(sweets)=kitchen", problem.goalStack, ontology);
  const auto& setOfFacts = problem.worldState.factsMapping();
  EXPECT_EQ("kitchen", setOfFacts.getFluentValue(_fact("location(sweets)=*", ontology))->value);
  _setGoalsForAPriority(problem, {_goal("grab(me, sweets)", ontology)}, ontology.constants);
  EXPECT_EQ(_action_navigate + "(?targetLocation -> kitchen), " + _action_grab + "(?object -> sweets)",
            _solveStr(problem, actions, ontology));
}



void _moveObject()
{
  const std::string actionNavigate2 = "actionNavigate2";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("loc_type\n"
                                             "entity");
  ontology.constants = ogp::SetOfEntities::fromPddl("me sweets - entity\n"
                                                    "corridor kitchen bedroom - loc_type", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("location(?o - entity) - loc_type\n"
                                                      "grab(?e - entity, ?o - entity)", ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> navParameters(1, _parameter("?targetLocation - loc_type", ontology));
  ogp::Action navAction({}, _worldStateModification_fromStr("location(me)=?targetLocation", ontology, navParameters));
  navAction.parameters = std::move(navParameters);
  actions.emplace(_action_navigate, navAction);

  std::vector<ogp::Parameter> nav2Parameters{_parameter("?targetLocation - loc_type", ontology), _parameter("?object - entity", ontology)};
  ogp::Action navAction2(_condition_fromStr("grab(me, ?object)", ontology, nav2Parameters),
                         _worldStateModification_fromStr("location(me)=?targetLocation & location(?object)=?targetLocation", ontology, nav2Parameters));
  navAction2.parameters = std::move(nav2Parameters);
  actions.emplace(actionNavigate2, navAction2);

  std::vector<ogp::Parameter> grabParameters(1, _parameter("?object - entity", ontology));
  ogp::Action grabAction(_condition_fromStr("equals(location(me), location(?object))", ontology, grabParameters),
                         _worldStateModification_fromStr("grab(me, ?object)", ontology, grabParameters));
  grabAction.parameters = std::move(grabParameters);
  actions.emplace(_action_grab, grabAction);

  ogp::Problem problem;
  _addFact(problem.worldState, "location(me)=corridor", problem.goalStack, ontology);
  _addFact(problem.worldState, "location(sweets)=kitchen", problem.goalStack, ontology);
  _setGoalsForAPriority(problem, {_goal("location(sweets)=bedroom", ontology)}, ontology.constants);
  EXPECT_EQ(_action_navigate + "(?targetLocation -> kitchen), " + _action_grab + "(?object -> sweets), " +
            actionNavigate2 + "(?object -> sweets, ?targetLocation -> bedroom)",
            _solveStr(problem, actions, ontology));
}


void _moveAndUngrabObject()
{
  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("loc_type\n"
                                             "robot\n"
                                             "entity");
  ontology.constants = ogp::SetOfEntities::fromPddl("me - robot\n"
                                                    "sweets - entity\n"
                                                    "kitchen bedroom - loc_type", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("locationOfRobot(?r - robot) - loc_type\n"
                                                      "locationOfObj(?o - entity) - loc_type\n"
                                                      "grab(?e - robot, ?o - entity)", ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> navParameters(1, _parameter("?targetLocation - loc_type", ontology));
  ogp::Action navAction({}, _worldStateModification_fromStr("locationOfRobot(me)=?targetLocation", ontology, navParameters));
  navAction.parameters = std::move(navParameters);
  actions.emplace(_action_navigate, navAction);

  std::vector<ogp::Parameter> grabParameters(1, _parameter("?object - entity", ontology));
  ogp::Action grabAction(_condition_fromStr("equals(locationOfRobot(me), locationOfObj(?object))", ontology, grabParameters),
                         _worldStateModification_fromStr("grab(me, ?object)", ontology, grabParameters));
  grabAction.parameters = std::move(grabParameters);
  actions.emplace(_action_grab, grabAction);

  std::vector<ogp::Parameter> ungrabParameters(1, _parameter("?object - entity", ontology));
  ogp::Action ungrabAction({}, _worldStateModification_fromStr("!grab(me, ?object)", ontology, ungrabParameters));
  ungrabAction.parameters = std::move(ungrabParameters);
  actions.emplace(_action_ungrab, ungrabAction);

  ogp::Problem problem;
  ogp::SetOfEvents setOfEvents;
  std::vector<ogp::Parameter> eventParameters{_parameter("?targetLocation - loc_type", ontology), _parameter("?object - entity", ontology)};
  ogp::Event event(_condition_fromStr("locationOfRobot(me)=?targetLocation & grab(me, ?object)", ontology, eventParameters),
                   _worldStateModification_fromStr("locationOfObj(?object)=?targetLocation", ontology, eventParameters));
  event.parameters = std::move(eventParameters);
  setOfEvents.add(event);
  ogp::Domain domain(std::move(actions), ontology, std::move(setOfEvents));

  auto& setOfEventsMap = domain.getSetOfEvents();
  _addFact(problem.worldState, "locationOfObj(sweets)=kitchen", problem.goalStack, ontology, setOfEventsMap, _now);
  _setGoalsForAPriority(problem, {_goal("locationOfObj(sweets)=bedroom & !grab(me, sweets)", ontology)}, ontology.constants);

  EXPECT_EQ(_action_navigate + "(?targetLocation -> kitchen)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ(_action_grab + "(?object -> sweets)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ(_action_navigate + "(?targetLocation -> bedroom)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ(_action_ungrab + "(?object -> sweets)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_FALSE(_hasFact(problem.worldState, "locationOfObj(sweets)=kitchen", ontology));
}


void _failToMoveAnUnknownObject()
{
  const std::string actionWhereIsObject = "actionWhereIsObject";
  const std::string actionLeavePod = "actionLeavePod";
  const std::string actionRanomWalk = "actionRanomWalk";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("loc_type\n"
                                             "robot\n"
                                             "entity");
  ontology.constants = ogp::SetOfEntities::fromPddl("me - robot\n"
                                                    "sweets - entity\n"
                                                    "kitchen bedroom - loc_type", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("locationOfRobot(?r - robot) - loc_type\n"
                                                      "locationOfObj(?o - entity) - loc_type\n"
                                                      "grab(?e - robot, ?o - entity)\n"
                                                      "charging(?r - robot)\n"
                                                      "isLost", ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> navParameters(1, _parameter("?targetLocation - loc_type", ontology));
  ogp::Action navAction(_condition_fromStr("!isLost & !charging(me)", ontology, navParameters),
                        _worldStateModification_fromStr("locationOfRobot(me)=?targetLocation", ontology, navParameters));
  navAction.parameters = std::move(navParameters);
  actions.emplace(_action_navigate, navAction);

  ogp::Action randomWalkAction(_condition_fromStr("!charging(me)", ontology),
                               _worldStateModification_fromStr("!isLost", ontology));
  actions.emplace(actionRanomWalk, randomWalkAction);

  ogp::Action leavePodAction({}, _worldStateModification_fromStr("!charging(me)", ontology));
  actions.emplace(actionLeavePod, leavePodAction);

  std::vector<ogp::Parameter> whereIsObjectParameters{_parameter("?object - entity", ontology), _parameter("?aLocation - loc_type", ontology)};
  ogp::Action whereIsObjectAction(_condition_fromStr("!locationOfObj(?object)=*", ontology, whereIsObjectParameters),
                                  _worldStateModification_fromStr("locationOfObj(?object)=?aLocation", ontology, whereIsObjectParameters));
  whereIsObjectAction.parameters = std::move(whereIsObjectParameters);
  actions.emplace(actionWhereIsObject, whereIsObjectAction);

  std::vector<ogp::Parameter> grabParameters{_parameter("?object - entity", ontology)};
  ogp::Action grabAction(_condition_fromStr("equals(locationOfRobot(me), locationOfObj(?object))", ontology, grabParameters),
                         _worldStateModification_fromStr("grab(me, ?object)", ontology, grabParameters));
  grabAction.parameters = std::move(grabParameters);
  actions.emplace(_action_grab, grabAction);

  std::vector<ogp::Parameter> ungrabParameters{_parameter("?object - entity", ontology)};
  ogp::Action ungrabAction({}, _worldStateModification_fromStr("!grab(me, ?object)", ontology, ungrabParameters));
  ungrabAction.parameters = std::move(ungrabParameters);
  actions.emplace(_action_ungrab, ungrabAction);

  ogp::SetOfEvents setOfEvents;
  std::vector<ogp::Parameter> eventParameters{_parameter("?targetLocation - loc_type", ontology), _parameter("?object - entity", ontology)};
  ogp::Event event(_condition_fromStr("locationOfRobot(me)=?targetLocation & grab(me, ?object)", ontology, eventParameters),
                   _worldStateModification_fromStr("locationOfObj(?object)=?targetLocation", ontology, eventParameters));
  event.parameters = std::move(eventParameters);
  setOfEvents.add(event);
  ogp::Domain domain(std::move(actions), ontology, std::move(setOfEvents));

  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal("locationOfObj(sweets)=bedroom & !grab(me, sweets)", ontology)}, ontology.constants);

  auto& setOfEventsMap = domain.getSetOfEvents();
  _addFact(problem.worldState, "charging(me)", problem.goalStack, ontology, setOfEventsMap, _now);
  EXPECT_EQ(actionWhereIsObject + "(?aLocation -> bedroom, ?object -> sweets)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  _addFact(problem.worldState, "locationOfObj(sweets)=kitchen", problem.goalStack, ontology, setOfEventsMap, _now);
  _setGoalsForAPriority(problem, {_goal("locationOfObj(sweets)=bedroom & !grab(me, sweets)", ontology)}, ontology.constants);
  EXPECT_EQ(actionLeavePod, _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ(_action_navigate + "(?targetLocation -> kitchen)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ(_action_grab + "(?object -> sweets)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ(_action_navigate + "(?targetLocation -> bedroom)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ(_action_ungrab + "(?object -> sweets)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
}


void _completeMovingObjectScenario()
{
  const std::string actionWhereIsObject = "actionWhereIsObject";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("loc_type\n"
                                             "robot\n"
                                             "entity");
  ontology.constants = ogp::SetOfEntities::fromPddl("me - robot\n"
                                                    "sweets bottle - entity\n"
                                                    "kitchen bedroom entrance - loc_type", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("locationOfRobot(?r - robot) - loc_type\n"
                                                      "locationOfObject(?o - entity) - loc_type\n"
                                                      "grab(?e - robot) - entity\n"
                                                      "pathIsBlocked\n"
                                                      "lost(?r - robot)", ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> navParameters{_parameter("?targetPlace - loc_type", ontology)};
  ogp::Action navAction(_condition_fromStr("!lost(me) & !pathIsBlocked", ontology, navParameters),
                        _worldStateModification_fromStr("locationOfRobot(me)=?targetPlace", ontology, navParameters));
  navAction.parameters = std::move(navParameters);
  actions.emplace(_action_navigate, navAction);

  std::vector<ogp::Parameter> grabParameters{_parameter("?object - entity", ontology)};
  ogp::Action grabAction(_condition_fromStr("equals(locationOfRobot(me), locationOfObject(?object)) & not(exists(?o - entity, grab(me)=?o))", ontology, grabParameters),
                         _worldStateModification_fromStr("grab(me)=?object", ontology, grabParameters));
  grabAction.parameters = std::move(grabParameters);
  actions.emplace(_action_grab, grabAction);

  std::vector<ogp::Parameter> ungrabParameters{_parameter("?object - entity", ontology)};
  ogp::Action ungrabAction({}, _worldStateModification_fromStr("!grab(me)=?object", ontology, ungrabParameters));
  ungrabAction.parameters = std::move(ungrabParameters);
  actions.emplace(_action_ungrab, ungrabAction);

  std::vector<ogp::Parameter> whereIsObjectParameters{_parameter("?object - entity", ontology), _parameter("?aLocation - loc_type", ontology)};
  ogp::Action whereIsObjectAction(_condition_fromStr("!locationOfObject(?object)=*", ontology, whereIsObjectParameters),
                                  _worldStateModification_fromStr("locationOfObject(?object)=?aLocation", ontology, whereIsObjectParameters));
  whereIsObjectAction.parameters = std::move(whereIsObjectParameters);
  actions.emplace(actionWhereIsObject, whereIsObjectAction);

  ogp::SetOfEvents setOfEvents;
  std::vector<ogp::Parameter> eventParameters{_parameter("?object - entity", ontology), _parameter("?location - loc_type", ontology)};
  ogp::Event event(_condition_fromStr("locationOfRobot(me)=?location & grab(me)=?object", ontology, eventParameters),
                   _worldStateModification_fromStr("locationOfObject(?object)=?location", ontology, eventParameters));
  event.parameters = std::move(eventParameters);
  setOfEvents.add(event);
  ogp::Domain domain(std::move(actions), ontology, std::move(setOfEvents));

  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal("locationOfObject(sweets)=bedroom & !grab(me)=sweets", ontology)}, ontology.constants);

  EXPECT_EQ(actionWhereIsObject + "(?aLocation -> bedroom, ?object -> sweets)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  auto& setOfEventsMap = domain.getSetOfEvents();
  _addFact(problem.worldState, "locationOfObject(sweets)=kitchen", problem.goalStack, ontology, setOfEventsMap, _now);
  _setGoalsForAPriority(problem, {_goal("locationOfObject(sweets)=bedroom & !grab(me)=sweets", ontology)}, ontology.constants);
  EXPECT_EQ(_action_navigate + "(?targetPlace -> kitchen)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ(_action_grab + "(?object -> sweets)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ(_action_navigate + "(?targetPlace -> bedroom)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ(_action_ungrab + "(?object -> sweets)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  _setGoalsForAPriority(problem, {_goal("locationOfObject(bottle)=entrance & !grab(me)=bottle", ontology)}, ontology.constants);
  EXPECT_EQ(actionWhereIsObject + "(?aLocation -> entrance, ?object -> bottle)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
}


void _moveAnObject()
{
  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("loc_type\n"
                                             "entity");
  ontology.constants = ogp::SetOfEntities::fromPddl("sweets bottle entrance_checkpoint - entity\n"
                                                    "kitchen bedroom entrance - loc_type", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("locationOfRobot - loc_type\n"
                                                      "locationOfObject(?o - entity) - loc_type\n"
                                                      "grab - entity", ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> navParameters{_parameter("?e - entity", ontology)};
  ogp::Action navAction({}, _worldStateModification_fromStr("assign(locationOfRobot, locationOfObject(?e))", ontology, navParameters));
  navAction.parameters = std::move(navParameters);
  actions.emplace(_action_navigate, navAction);

  std::vector<ogp::Parameter> grabParameters{_parameter("?object - entity", ontology)};
  ogp::Action grabAction(_condition_fromStr("equals(locationOfRobot, locationOfObject(?object))", ontology, grabParameters),
                         _worldStateModification_fromStr("grab=?object", ontology, grabParameters));
  grabAction.parameters = std::move(grabParameters);
  actions.emplace(_action_grab, grabAction);

  ogp::SetOfEvents setOfEvents;
  std::vector<ogp::Parameter> eventParameters{_parameter("?object - entity", ontology), _parameter("?location - loc_type", ontology)};
  ogp::Event event(_condition_fromStr("locationOfRobot=?location & grab=?object", ontology, eventParameters),
                   _worldStateModification_fromStr("locationOfObject(?object)=?location", ontology, eventParameters));
  event.parameters = std::move(eventParameters);
  setOfEvents.add(event);
  ogp::Domain domain(std::move(actions), ontology, std::move(setOfEvents));

  ogp::Problem problem;
  auto& setOfEventsMap = domain.getSetOfEvents();
  _addFact(problem.worldState, "locationOfObject(sweets)=kitchen", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, "locationOfObject(entrance_checkpoint)=entrance", problem.goalStack, ontology, setOfEventsMap, _now);
  _setGoalsForAPriority(problem, {_goal("locationOfObject(sweets)=entrance", ontology)}, ontology.constants);
  EXPECT_EQ(_action_navigate + "(?e -> sweets)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ(_action_grab + "(?object -> sweets)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ(_action_navigate + "(?e -> entrance_checkpoint)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
}

void _matchSkillWithObjectsWithASubType()
{
  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("sub_loc_type - loc_type");
  ontology.constants = ogp::SetOfEntities::fromPddl("kitchen - sub_loc_type", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("locationOfRobot - loc_type", ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> navParameters{_parameter("?targetPlace - loc_type", ontology)};
  ogp::Action navAction({},
                        _worldStateModification_fromStr("locationOfRobot=?targetPlace", ontology, navParameters));
  navAction.parameters = std::move(navParameters);
  actions.emplace(_action_navigate, navAction);

  ogp::Domain domain(std::move(actions), ontology);

  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal("locationOfRobot=kitchen", ontology)}, ontology.constants);
  EXPECT_EQ(_action_navigate + "(?targetPlace -> kitchen)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
}


void _eventWithANegatedFactWithParameter()
{
  const std::string actionUngrabLeftHand = "actionUngrabLeftHand";
  const std::string actionUngrabRightHand = "actionUngrabRightHand";
  const std::string actionUngrabBothHands = "actionUngrabBothHands";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("loc_type\n"
                                             "robot\n"
                                             "entity");
  ontology.constants = ogp::SetOfEntities::fromPddl("me - robot\n"
                                                    "sweets bottle glass - entity", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("grab(?e - robot, ?e - entity)\n"
                                                      "hasTwoHandles(?e - entity)\n"
                                                      "grabLeftHand(?r - robot) - entity\n"
                                                      "grabRightHand(?r - robot) - entity", ontology.types);
  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> ungrabLeftParameters{_parameter("?object - entity", ontology)};
  ogp::Action ungrabLeftAction(_condition_fromStr("!hasTwoHandles(?object)", ontology, ungrabLeftParameters),
                               _worldStateModification_fromStr("!grabLeftHand(me)=?object", ontology, ungrabLeftParameters));
  ungrabLeftAction.parameters = std::move(ungrabLeftParameters);
  actions.emplace(actionUngrabLeftHand, ungrabLeftAction);

  std::vector<ogp::Parameter> ungrabRightParameters{_parameter("?object - entity", ontology)};
  ogp::Action ungrabRightAction(_condition_fromStr("!hasTwoHandles(?object)", ontology, ungrabRightParameters),
                                _worldStateModification_fromStr("!grabRightHand(me)=?object", ontology, ungrabRightParameters));
  ungrabRightAction.parameters = std::move(ungrabRightParameters);
  actions.emplace(actionUngrabRightHand, ungrabRightAction);

  std::vector<ogp::Parameter> ungrabBothParameters{_parameter("?object - entity", ontology)};
  ogp::Action ungrabBothAction(_condition_fromStr("hasTwoHandles(?object)", ontology, ungrabBothParameters),
                               _worldStateModification_fromStr("!grabLeftHand(me)=?object & !grabRightHand(me)=?object", ontology, ungrabBothParameters));
  ungrabBothAction.parameters = std::move(ungrabBothParameters);
  actions.emplace(actionUngrabBothHands, ungrabBothAction);

  ogp::SetOfEvents setOfEvents;
  std::vector<ogp::Parameter> eventParameters{_parameter("?object - entity", ontology)};
  ogp::Event event(_condition_fromStr("!grabLeftHand(me)=?object & !grabRightHand(me)=?object", ontology, eventParameters),
                   _worldStateModification_fromStr("!grab(me, ?object)", ontology, eventParameters));
  event.parameters = std::move(eventParameters);
  setOfEvents.add(event);

  ogp::Domain domain(std::move(actions), ontology, std::move(setOfEvents));
  auto& setOfEventsMap = domain.getSetOfEvents();

  ogp::Problem problem;
  _addFact(problem.worldState, "hasTwoHandles(sweets)", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, "grabLeftHand(me)=sweets", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, "grabRightHand(me)=sweets", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, "grab(me, sweets)", problem.goalStack, ontology, setOfEventsMap, _now);
  _setGoalsForAPriority(problem, {_goal("!grab(me, sweets)", ontology)}, ontology.constants);

  EXPECT_EQ(actionUngrabBothHands + "(?object -> sweets)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_FALSE(_hasFact(problem.worldState, "grab(me, sweets)", ontology));

  _addFact(problem.worldState, "grabLeftHand(me)=bottle", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, "grab(me, bottle)", problem.goalStack, ontology, setOfEventsMap, _now);
  _setGoalsForAPriority(problem, {_goal("!grab(me, bottle)", ontology)}, ontology.constants);
  EXPECT_EQ(actionUngrabLeftHand + "(?object -> bottle)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());

  _addFact(problem.worldState, "grabLeftHand(me)=bottle", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, "grab(me, bottle)", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, "grabRightHand(me)=glass", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, "grab(me, glass)", problem.goalStack, ontology, setOfEventsMap, _now);
  _setGoalsForAPriority(problem, {_goal("!grab(me, glass)", ontology)}, ontology.constants);
  EXPECT_EQ(actionUngrabRightHand + "(?object -> glass)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_FALSE(_hasFact(problem.worldState, "grab(me, glass)", ontology));
}


void _actionWithANegatedFactNotTriggeredIfNotNecessary()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  const std::string action3 = "action3";

  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + "\n" +
                                                      _fact_b + "\n" +
                                                      _fact_c + "\n" +
                                                      _fact_d + "\n" +
                                                      _fact_e, ontology.types);

  std::map<std::string, ogp::Action> actions;
  actions.emplace(action1, ogp::Action({},
                                       _worldStateModification_fromStr("!" + _fact_a, ontology),
                                       _condition_fromStr(_fact_c + " & " + _fact_d, ontology)));

  actions.emplace(action2, ogp::Action(_condition_fromStr("!" + _fact_a + " & " + _fact_e, ontology),
                                       _worldStateModification_fromStr(_fact_b, ontology)));

  actions.emplace(action3, ogp::Action({},
                                       _worldStateModification_fromStr(_fact_e, ontology)));

  ogp::Domain domain(std::move(actions), ontology);
  ogp::Problem problem;
  _addFact(problem.worldState, _fact_c, problem.goalStack, ontology);
  _addFact(problem.worldState, _fact_d, problem.goalStack, ontology);
  _setGoalsForAPriority(problem, {_goal(_fact_b, ontology)}, ontology.constants);

  EXPECT_EQ(action3, _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ(action2, _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
}



void _useTwoTimesAnEvent()
{
  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity");
  ontology.constants = ogp::SetOfEntities::fromPddl("obj1 obj2 - entity", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + "\n" +
                                                      _fact_b + "(?e - entity)\n" +
                                                      _fact_c + "(?e - entity)", ontology.types);

  ogp::SetOfEvents setOfEvents;
  std::vector<ogp::Parameter> eventParameters{_parameter("?object - entity", ontology)};
  ogp::Event event(_condition_fromStr(_fact_a + " & " + _fact_b + "(?object)", ontology, eventParameters),
                   _worldStateModification_fromStr(_fact_c + "(?object)", ontology, eventParameters));
  event.parameters = std::move(eventParameters);
  setOfEvents.add(event);

  std::map<std::string, ogp::Action> actions;
  ogp::Domain domain(std::move(actions), ontology, std::move(setOfEvents));

  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  _addFact(problem.worldState, _fact_b + "(obj1)", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_b + "(obj2)", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_a, problem.goalStack, ontology, setOfEventsMap, _now);
  EXPECT_TRUE(_hasFact(problem.worldState, _fact_c + "(obj1)", ontology));
  EXPECT_TRUE(_hasFact(problem.worldState, _fact_c + "(obj2)", ontology));
}


void _linkWithAnyValueInCondition()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity");
  ontology.constants = ogp::SetOfEntities::fromPddl("toto - entity", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + " - entity\n" +
                                                      _fact_b, ontology.types);

  std::map<std::string, ogp::Action> actions;
  actions.emplace(action1, ogp::Action(_condition_fromStr("!" + _fact_a + "=*", ontology),
                                       _worldStateModification_fromStr(_fact_b, ontology)));

  std::vector<ogp::Parameter> act2Parameters{_parameter("?aVal - entity", ontology)};
  ogp::Action act2({}, _worldStateModification_fromStr("!" + _fact_a + "=?aVal", ontology, act2Parameters));
  act2.parameters = std::move(act2Parameters);
  actions.emplace(action2, act2);

  ogp::Domain domain(std::move(actions), ontology);
  ogp::Problem problem;
  _addFact(problem.worldState, _fact_a + "=toto", problem.goalStack, ontology);
  _setGoalsForAPriority(problem, {_goal(_fact_b, ontology)}, ontology.constants);
  EXPECT_EQ(action2 + "(?aVal -> toto)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ(action1, _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
}


void _removeAFactWithAnyValue()
{
  const std::string action1 = "action1";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity");
  ontology.constants = ogp::SetOfEntities::fromPddl("toto - entity", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + "\n" +
                                                      _fact_b + " - entity", ontology.types);

  std::map<std::string, ogp::Action> actions;
  actions.emplace(action1, ogp::Action({}, _worldStateModification_fromStr(_fact_a + " & !" + _fact_b + "=*", ontology)));
  ogp::Domain domain(std::move(actions), ontology);
  ogp::Problem problem;
  _addFact(problem.worldState, _fact_b + "=toto", problem.goalStack, ontology);
  _setGoalsForAPriority(problem, {_goal(_fact_a, ontology)}, ontology.constants);
  EXPECT_EQ(action1, _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_FALSE(_hasFact(problem.worldState, _fact_b + "=toto", ontology));
}



void _notDeducePathIfTheParametersOfAFactAreDifferents()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  const std::string action3 = "action3";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity");
  ontology.constants = ogp::SetOfEntities::fromPddl("e1 e2 - entity", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + "(?e - entity)\n" +
                                                      _fact_b + "\n" +
                                                      _fact_c + "\n" +
                                                      _fact_d, ontology.types);

  std::map<ogp::ActionId, ogp::Action> actions;
  actions.emplace(action1, ogp::Action({},
                                       _worldStateModification_fromStr(_fact_a + "(e1)", ontology),
                                       _condition_fromStr(_fact_c, ontology)));
  actions.emplace(action2, ogp::Action(_condition_fromStr(_fact_a + "(e2)", ontology),
                                       _worldStateModification_fromStr(_fact_b, ontology)));
  actions.emplace(action3, ogp::Action(_condition_fromStr(_fact_b, ontology),
                                       _worldStateModification_fromStr(_fact_d, ontology)));
  ogp::Domain domain(std::move(actions), ontology);

  ogp::Problem problem;
  _addFact(problem.worldState, _fact_a + "(e2)", problem.goalStack, ontology);
  _addFact(problem.worldState, _fact_c, problem.goalStack, ontology);
  _setGoalsForAPriority(problem, {_goal(_fact_d, ontology)}, ontology.constants);

  EXPECT_EQ(action2, _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.actionId);
}


void _checkPreferInContext()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";

  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + "\n" +
                                                      _fact_b, ontology.types);

  std::map<std::string, ogp::Action> actions;
  actions.emplace(action1, ogp::Action({}, _worldStateModification_fromStr(_fact_a, ontology), _condition_fromStr(_fact_b, ontology)));
  actions.emplace(action2, ogp::Action({}, _worldStateModification_fromStr(_fact_a, ontology)));

  ogp::Problem problem;
  _addFact(problem.worldState, _fact_b, problem.goalStack, ontology);
  _setGoalsForAPriority(problem, {_fact_a}, ontology);
  EXPECT_EQ(action1, _solveStrConst(problem, actions, ontology, &problem.historical));
  EXPECT_EQ(action1, _solveStrConst(problem, actions, ontology, &problem.historical));
}


void _checkPreferHighImportanceOfNotRepeatingIt()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";

  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + "\n" +
                                                      _fact_b, ontology.types);

  std::map<std::string, ogp::Action> actions;
  auto action1Obj = ogp::Action({}, _worldStateModification_fromStr(_fact_a, ontology), _condition_fromStr(_fact_b, ontology));
  action1Obj.highImportanceOfNotRepeatingIt = true;
  actions.emplace(action1, action1Obj);
  auto action2Obj = ogp::Action({}, _worldStateModification_fromStr(_fact_a, ontology));
  action2Obj.highImportanceOfNotRepeatingIt = true;
  actions.emplace(action2, action2Obj);

  ogp::Problem problem;
  _addFact(problem.worldState, _fact_b, problem.goalStack, ontology);
  _setGoalsForAPriority(problem, {_fact_a}, ontology);
  EXPECT_EQ(action1, _solveStrConst(problem, actions, ontology, &problem.historical));
  EXPECT_EQ(action2, _solveStrConst(problem, actions, ontology, &problem.historical));
}


void _actionWithFactWithANegatedFact()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  const std::string action3 = "action3";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity");
  ontology.constants = ogp::SetOfEntities::fromPddl("a b - entity", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + " - entity\n" +
                                                      _fact_b + "\n" +
                                                      _fact_c + "\n" +
                                                      _fact_d + "\n" +
                                                      _fact_e, ontology.types);

  std::map<std::string, ogp::Action> actions;
  actions.emplace(action1, ogp::Action({},
                                       _worldStateModification_fromStr(_fact_a + "=a", ontology)));

  actions.emplace(action2, ogp::Action(_condition_fromStr(_fact_a + "!=b & " + _fact_d, ontology),
                                       _worldStateModification_fromStr(_fact_c, ontology)));

  actions.emplace(action3, ogp::Action({},
                                       _worldStateModification_fromStr(_fact_d, ontology),
                                       _condition_fromStr(_fact_e, ontology)));

  ogp::Domain domain(std::move(actions), ontology);
  ogp::Problem problem;
  _addFact(problem.worldState, _fact_e, problem.goalStack, ontology);
  _setGoalsForAPriority(problem, {_goal(_fact_c, ontology)}, ontology.constants);

  EXPECT_EQ(action3, _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ(action1, _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ(action2, _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
}


void _negatedFactValueInWorldState()
{
  const std::string action1 = "action1";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity");
  ontology.constants = ogp::SetOfEntities::fromPddl("a b c - entity", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + " - entity\n" +
                                                      _fact_b, ontology.types);

  std::map<std::string, ogp::Action> actions;
  actions.emplace(action1, ogp::Action(_condition_fromStr(_fact_a + "!=b", ontology),
                                       _worldStateModification_fromStr(_fact_b, ontology)));
  ogp::Domain domain(std::move(actions), ontology);

  {
    ogp::Problem problem;
    _setGoalsForAPriority(problem, {_goal(_fact_b, ontology)}, ontology.constants);
    EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  }

  {
    ogp::Problem problem;
    _addFact(problem.worldState, _fact_a + "=b", problem.goalStack, ontology);
    _setGoalsForAPriority(problem, {_goal(_fact_b, ontology)}, ontology.constants);
    EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  }

  {
    ogp::Problem problem;
    _addFact(problem.worldState, _fact_a + "=c", problem.goalStack, ontology);
    _setGoalsForAPriority(problem, {_goal(_fact_b, ontology)}, ontology.constants);

    EXPECT_EQ(action1, _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
    EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  }

  {
    ogp::Problem problem;
    _addFact(problem.worldState, _fact_a + "!=b", problem.goalStack, ontology);
    _setGoalsForAPriority(problem, {_goal(_fact_b, ontology)}, ontology.constants);

    EXPECT_EQ(action1, _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
    EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  }
}


void _problemThatUseADomainThatChangedSinceLastUsage()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";

  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + "\n" +
                                                      _fact_b, ontology.types);

  std::map<std::string, ogp::Action> actions;
  actions.emplace(action1, ogp::Action(_condition_fromStr(_fact_a, ontology),
                                       _worldStateModification_fromStr(_fact_b, ontology)));
  ogp::Domain domain(std::move(actions), ontology);

  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal(_fact_b, ontology)}, ontology.constants);
  EXPECT_EQ("", _lookForAnActionToDoStr(problem, domain, _now)); // set problem cache about domain

  domain.addAction(action2, ogp::Action({},
                                        _worldStateModification_fromStr(_fact_a, ontology)));

  _setGoalsForAPriority(problem, {_goal(_fact_b, ontology)}, ontology.constants);
  EXPECT_EQ(action2, _lookForAnActionToDoStr(problem, domain, _now)); // as domain as changed since last time the problem cache should be regenerated
}


void _checkFilterFactInCondition()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity\n"
                                             "location");
  ontology.constants = ogp::SetOfEntities::fromPddl("obj1 obj2 - entity\n"
                                                    "loc1 loc2 - location", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + "(?e - entity)\n" +
                                                      _fact_b + "(?e - entity, ?l - location)\n" +
                                                      _fact_c, ontology.types);

  std::map<std::string, ogp::Action> actions;

  std::vector<ogp::Parameter> act1Parameters{_parameter("?obj - entity", ontology)};
  ogp::Action actionObj1({}, _worldStateModification_fromStr(_fact_a + "(?obj)", ontology, act1Parameters));
  actionObj1.parameters = std::move(act1Parameters);
  actions.emplace(action1, actionObj1);

  std::vector<ogp::Parameter> act2Parameters{_parameter("?obj - entity", ontology), _parameter("?loc - location", ontology)};
  ogp::Action actionObj2(_condition_fromStr(_fact_b + "(?obj, ?loc) & " + _fact_a + "(?obj)", ontology, act2Parameters),
                         _worldStateModification_fromStr(_fact_c, ontology, act2Parameters));
  actionObj2.parameters = std::move(act2Parameters);
  actions.emplace(action2, actionObj2);
  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();

  ogp::Problem problem;
  _addFact(problem.worldState, _fact_b + "(obj1, loc1)", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_b + "(obj1, loc2)", problem.goalStack, ontology, setOfEventsMap, _now);
  _setGoalsForAPriority(problem, {_goal(_fact_c, ontology)}, ontology.constants);
  EXPECT_EQ(action1 + "(?obj -> obj1)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ(action2 + "(?loc -> loc1, ?obj -> obj1)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
}


void _checkFilterFactInConditionAndThenPropagate()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  const std::string action3 = "action3";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity\n"
                                             "location");
  ontology.constants = ogp::SetOfEntities::fromPddl("obj1 obj2 - entity\n"
                                                    "loc1 loc2 - location", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + " - entity\n" +
                                                      _fact_b + "(?e - entity, ?l - location)\n" +
                                                      _fact_c + "(?l - location)\n" +
                                                      _fact_d + "(?l - location)", ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> act1Parameters{_parameter("?obj - entity", ontology)};
  ogp::Action actionObj1({}, _worldStateModification_fromStr(_fact_a + "=?obj", ontology, act1Parameters));
  actionObj1.parameters = std::move(act1Parameters);
  actions.emplace(action1, actionObj1);

  std::vector<ogp::Parameter> act2Parameters{_parameter("?obj - entity", ontology), _parameter("?loc - location", ontology)};
  ogp::Action actionObj2(_condition_fromStr(_fact_b + "(?obj, ?loc) & " + _fact_a + "=?obj", ontology, act2Parameters),
                         _worldStateModification_fromStr(_fact_c + "(?loc)", ontology, act2Parameters));
  actionObj2.parameters = std::move(act2Parameters);
  actions.emplace(action2, actionObj2);

  std::vector<ogp::Parameter> act3Parameters{_parameter("?loc - location", ontology)};
  ogp::Action actionObj3(_condition_fromStr(_fact_c + "(?loc)", ontology, act3Parameters),
                         _worldStateModification_fromStr(_fact_d + "(?loc)", ontology, act3Parameters));
  actionObj3.parameters = std::move(act3Parameters);
  actions.emplace(action3, actionObj3);

  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();

  ogp::Problem problem;
  _addFact(problem.worldState, _fact_b + "(obj1, loc1)", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_b + "(obj2, loc2)", problem.goalStack, ontology, setOfEventsMap, _now);
  _setGoalsForAPriority(problem, {_goal(_fact_d + "(loc2)", ontology)}, ontology.constants);
  EXPECT_EQ(action1 + "(?obj -> obj2)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ(action2 + "(?loc -> loc2, ?obj -> obj2)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ(action3 + "(?loc -> loc2)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
}


void _checkOutputValueOfLookForAnActionToDo()
{
  const std::string action1 = "action1";

  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + "\n" +
                                                      _fact_b, ontology.types);

  std::map<std::string, ogp::Action> actions;
  actions.emplace(action1, ogp::Action({}, _worldStateModification_fromStr(_fact_a, ontology)));

  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal(_fact_a, ontology)}, ontology.constants);

  {
    ogp::LookForAnActionOutputInfos lookForAnActionOutputInfos;
    auto res = ogp::planForMoreImportantGoalPossible(problem, domain, true, _now, nullptr, &lookForAnActionOutputInfos);
    assert(!res.empty());
    EXPECT_EQ(ogp::PlannerStepType::IN_PROGRESS, lookForAnActionOutputInfos.getType());
    EXPECT_EQ(0, lookForAnActionOutputInfos.nbOfSatisfiedGoals());
  }

  {
    _addFact(problem.worldState, _fact_a, problem.goalStack, ontology, setOfEventsMap, _now);
    ogp::LookForAnActionOutputInfos lookForAnActionOutputInfos;
    auto res = ogp::planForMoreImportantGoalPossible(problem, domain, true, _now, nullptr, &lookForAnActionOutputInfos);
    assert(res.empty());
    EXPECT_EQ(ogp::PlannerStepType::FINISHED_ON_SUCCESS, lookForAnActionOutputInfos.getType());
    EXPECT_EQ(1, lookForAnActionOutputInfos.nbOfSatisfiedGoals());
  }

  {
    _setGoalsForAPriority(problem, {_goal(_fact_b, ontology)}, ontology.constants);
    ogp::LookForAnActionOutputInfos lookForAnActionOutputInfos;
    auto res = ogp::planForMoreImportantGoalPossible(problem, domain, true, _now, nullptr, &lookForAnActionOutputInfos);
    assert(res.empty());
    EXPECT_EQ(ogp::PlannerStepType::FINISEHD_ON_FAILURE, lookForAnActionOutputInfos.getType());
    EXPECT_EQ(0, lookForAnActionOutputInfos.nbOfSatisfiedGoals());
  }
}


void _hardProblemThatNeedsToBeSmart()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  const std::string action3 = "action3";
  const std::string action4 = "action4";
  const std::string action5 = "action5";
  const std::string action6 = "action6";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity");
  ontology.constants = ogp::SetOfEntities::fromPddl("val1 val2 val3 val4 - entity", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + " - entity\n" +
                                                      _fact_b + "\n" +
                                                      _fact_c + "\n" +
                                                      _fact_d + "\n" +
                                                      _fact_e + "\n" +
                                                      _fact_f, ontology.types);

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

  ogp::Action actionObj6(_condition_fromStr(_fact_b + " & !" + _fact_d, ontology),
                         _worldStateModification_fromStr(_fact_e, ontology));
  actions.emplace(action6, actionObj6);

  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();

  ogp::Problem problem;
  _addFact(problem.worldState, _fact_d, problem.goalStack, ontology, setOfEventsMap, _now);
  _setGoalsForAPriority(problem, {_goal(_fact_e, ontology)}, ontology.constants);

  EXPECT_EQ(action1 + "(?obj -> val3)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ(action4, _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ(action1 + "(?obj -> val1)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ(action2, _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ(action6, _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
}




void _hardProblemThatNeedsToBeSmartWithAnEvent()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  const std::string action3 = "action3";
  const std::string action4 = "action4";
  const std::string action5 = "action5";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity");
  ontology.constants = ogp::SetOfEntities::fromPddl("val1 val2 val3 val4 - entity", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + " - entity\n" +
                                                      _fact_b + "\n" +
                                                      _fact_c + "\n" +
                                                      _fact_d + "\n" +
                                                      _fact_e + "\n" +
                                                      _fact_f, ontology.types);

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

  ogp::Action actionObj4(_condition_fromStr(_fact_a + "=val4", ontology),
                         _worldStateModification_fromStr(_fact_b + " & " + _fact_c, ontology));
  actions.emplace(action4, actionObj4);

  ogp::Action actionObj5(_condition_fromStr(_fact_b + " & !" + _fact_d, ontology),
                         _worldStateModification_fromStr(_fact_e, ontology));
  actions.emplace(action5, actionObj5);

  ogp::SetOfEvents setOfEvents;
  {
    ogp::Event event1(_condition_fromStr(_fact_a + "=val3 & !" + _fact_c, ontology),
                      _worldStateModification_fromStr("!" + _fact_d + " & " + _fact_f, ontology));
    setOfEvents.add(event1);
  }


  ogp::Domain domain(std::move(actions), ontology, std::move(setOfEvents));
  auto& setOfEventsMap = domain.getSetOfEvents();

  ogp::Problem problem;
  _addFact(problem.worldState, _fact_d, problem.goalStack, ontology, setOfEventsMap, _now);
  _setGoalsForAPriority(problem, {_goal(_fact_e, ontology)}, ontology.constants);

  EXPECT_EQ(action1 + "(?obj -> val3)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ(action1 + "(?obj -> val1)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ(action2, _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ(action5, _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
}



void _checkOverallEffectDuringParallelisation()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  const std::string action3 = "action3";

  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + "\n" +
                                                      _fact_b + "\n" +
                                                      _fact_c + "\n" +
                                                      _fact_d + "\n" +
                                                      _fact_e, ontology.types);

  std::map<std::string, ogp::Action> actions;
  ogp::Action actionObj1(_condition_fromStr("!" + _fact_d, ontology),
                         _worldStateModification_fromStr(_fact_a + " & !" + _fact_d, ontology));
  actionObj1.effect.worldStateModificationAtStart = _worldStateModification_fromStr(_fact_d + " & " + _fact_e, ontology);
  actions.emplace(action1, actionObj1);

  ogp::Action actionObj2(_condition_fromStr("!" + _fact_d, ontology),
                         _worldStateModification_fromStr(_fact_b + " & !" + _fact_d, ontology));
  actionObj2.effect.worldStateModificationAtStart = _worldStateModification_fromStr(_fact_d, ontology);
  actions.emplace(action2, actionObj2);

  ogp::Action actionObj3(_condition_fromStr(_fact_a + " & " + _fact_b, ontology),
                         _worldStateModification_fromStr(_fact_c, ontology));
  actions.emplace(action3, actionObj3);

  ogp::Domain domain(std::move(actions), ontology);
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal(_fact_c, ontology)}, ontology.constants);

  EXPECT_EQ(action1, _lookForAnActionToDoInParallelThenNotifyToStr(problem, domain, _now));
  EXPECT_TRUE(_hasFact(problem.worldState, _fact_e, ontology));
}


void _orInCondition()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  const std::string action3 = "action3";

  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + "\n" +
                                                      _fact_b + "\n" +
                                                      _fact_c, ontology.types);

  std::map<std::string, ogp::Action> actions;
  ogp::Action actionObj1(_condition_fromStr(_fact_a + " & " + _fact_b, ontology),
                         _worldStateModification_fromStr(_fact_c, ontology));
  actions.emplace(action1, actionObj1);

  ogp::Action actionObj2(_condition_fromStr(_fact_a + " | " + _fact_b, ontology),
                         _worldStateModification_fromStr(_fact_c, ontology));
  actions.emplace(action2, actionObj2);

  ogp::Action actionObj3(_condition_fromStr(_fact_a + " & " + _fact_b, ontology),
                         _worldStateModification_fromStr(_fact_c, ontology));
  actions.emplace(action3, actionObj3);

  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal(_fact_c, ontology)}, ontology.constants);
  _addFact(problem.worldState, _fact_a, problem.goalStack, ontology, setOfEventsMap, _now);

  EXPECT_EQ(action2, _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ("", _lookForAnActionToDo(problem, domain, _now).actionInvocation.toStr());
}


void _axioms()
{
  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("object");
  ontology.constants = ogp::SetOfEntities::fromPddl("book titi - object", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + "(?o - object)\n" +
                                                      _fact_b + "(?o - object)\n" +
                                                      _fact_c + "(?o - object)\n" +
                                                      _fact_d + "(?o - object)\n" +
                                                      _fact_e, ontology.types);

  std::map<std::string, ogp::Action> actions;
  ogp::SetOfEvents setOfEvents;
  std::vector<ogp::Parameter> derPred1Parameters{_parameter("?o - object", ontology)};
  ogp::Axiom axiom(_condition_fromStr(_fact_a + "(?o)" + " & " + _fact_b + "(?o)", ontology, derPred1Parameters),
                   _fact(_fact_c + "(?o)", ontology, derPred1Parameters), derPred1Parameters);
  for (auto& currEvent : axiom.toEvents(ontology, {}))
    setOfEvents.add(currEvent);

  std::vector<ogp::Parameter> derPred2Parameters{_parameter("?o - object", ontology)};
  ogp::Axiom derivedPredicate2(_condition_fromStr(_fact_a + "(?o)" + " | " + _fact_b + "(?o)", ontology, derPred2Parameters),
                               _fact(_fact_d + "(?o)", ontology, derPred2Parameters), derPred2Parameters);
  for (auto& currEvent : derivedPredicate2.toEvents(ontology, {}))
    setOfEvents.add(currEvent);

  ogp::Domain domain(std::move(actions), ontology, std::move(setOfEvents));
  auto& setOfEventsMap = domain.getSetOfEvents();

  ogp::Problem problem;
  _addFact(problem.worldState, _fact_e, problem.goalStack, ontology, setOfEventsMap, _now);

  EXPECT_FALSE(_hasFact(problem.worldState, _fact_c + "(book)", ontology));
  EXPECT_FALSE(_hasFact(problem.worldState, _fact_d + "(book)", ontology));
  _addFact(problem.worldState, _fact_a + "(book)", problem.goalStack, ontology, setOfEventsMap, _now);
  EXPECT_FALSE(_hasFact(problem.worldState, _fact_c + "(book)", ontology));
  EXPECT_TRUE(_hasFact(problem.worldState, _fact_d + "(book)", ontology));
  _addFact(problem.worldState, _fact_b + "(book)", problem.goalStack, ontology, setOfEventsMap, _now);
  EXPECT_TRUE(_hasFact(problem.worldState, _fact_c + "(book)", ontology));
  EXPECT_TRUE(_hasFact(problem.worldState, _fact_d + "(book)", ontology));
  EXPECT_FALSE(_hasFact(problem.worldState, _fact_c + "(titi)", ontology));
  EXPECT_FALSE(_hasFact(problem.worldState, _fact_d + "(titi)", ontology));
  _removeFact(problem.worldState, _fact_a + "(book)", problem.goalStack, ontology, setOfEventsMap, _now);
  EXPECT_FALSE(_hasFact(problem.worldState, _fact_c + "(book)", ontology));
  EXPECT_TRUE(_hasFact(problem.worldState, _fact_d + "(book)", ontology));
  _addFact(problem.worldState, _fact_a + "(titi)", problem.goalStack, ontology, setOfEventsMap, _now);
  EXPECT_FALSE(_hasFact(problem.worldState, _fact_c + "(book)", ontology));
  EXPECT_TRUE(_hasFact(problem.worldState, _fact_d + "(book)", ontology));
  EXPECT_FALSE(_hasFact(problem.worldState, _fact_c + "(titi)", ontology));
  EXPECT_TRUE(_hasFact(problem.worldState, _fact_d + "(titi)", ontology));
}


void _assignAnotherValueToSatisfyNotGoal()
{
  const std::string action1 = "action1";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity");
  ontology.constants = ogp::SetOfEntities::fromPddl("toto titi - entity", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + " - entity", ontology.types);

  std::map<std::string, ogp::Action> actions;
  actions.emplace(action1, ogp::Action({}, _worldStateModification_fromStr(_fact_a + "=toto", ontology)));

  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal("!" + _fact_a + "=titi", ontology)}, ontology.constants);

  EXPECT_EQ("", _lookForAnActionToDoConst(problem, domain, _now).actionInvocation.toStr());
  _addFact(problem.worldState, _fact_a + "=titi", problem.goalStack, ontology, setOfEventsMap, _now);
  EXPECT_EQ(action1, _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
}


void _assignUndefined()
{
  const std::string action1 = "action1";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity");
  ontology.constants = ogp::SetOfEntities::fromPddl("toto titi - entity", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + " - entity", ontology.types);

  std::map<std::string, ogp::Action> actions;
  actions.emplace(action1, ogp::Action({}, _worldStateModification_fromStr("assign(" + _fact_a + ", undefined)", ontology)));

  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal("!" + _fact_a + "=titi", ontology)}, ontology.constants);

  EXPECT_EQ("", _lookForAnActionToDoConst(problem, domain, _now).actionInvocation.toStr());
  _addFact(problem.worldState, _fact_a + "=titi", problem.goalStack, ontology, setOfEventsMap, _now);
  EXPECT_EQ(1u, problem.worldState.facts().size());
  EXPECT_EQ(action1, _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ(0u, problem.worldState.facts().size()); // because assign undefined is done like a fact removal
}


void _assignAFact()
{
  const std::string action1 = "action1";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity\n"
                                             "param");
  ontology.constants = ogp::SetOfEntities::fromPddl("aVal anotherVal valGoal - entity\n"
                                                    "p1 p2 p3 - param", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + " - entity\n" +
                                                      _fact_b + "(?p - param) - entity", ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> action1Parameters{_parameter("?p - param", ontology)};
  ogp::Action action1Obj({}, _worldStateModification_fromStr("assign(" + _fact_a + ", " + _fact_b + "(?p))", ontology, action1Parameters));
  action1Obj.parameters = std::move(action1Parameters);
  actions.emplace(action1, action1Obj);

  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal(_fact_a + "=valGoal", ontology)}, ontology.constants);
  _addFact(problem.worldState, _fact_b + "(p1)=aVal", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_b + "(p2)=valGoal", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_b + "(p3)=anotherVal", problem.goalStack, ontology, setOfEventsMap, _now);

  EXPECT_EQ(action1 + "(?p -> p2)", _lookForAnActionToDo(problem, domain, _now).actionInvocation.toStr());
}


void _assignAFactToAction()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity\n"
                                             "param");
  ontology.constants = ogp::SetOfEntities::fromPddl("aVal anotherVal valGoal - entity\n"
                                                    "p1 p2 p3 - param", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + " - entity\n" +
                                                      _fact_b + "(?p - param) - entity\n" +
                                                      _fact_c, ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> action1Parameters{_parameter("?p - param", ontology)};
  ogp::Action action1Obj({}, _worldStateModification_fromStr("assign(" + _fact_a + ", " + _fact_b + "(?p))", ontology, action1Parameters));
  action1Obj.parameters = std::move(action1Parameters);
  actions.emplace(action1, action1Obj);
  actions.emplace(action2, ogp::Action(_condition_fromStr(_fact_a + "=valGoal", ontology),
                                       _worldStateModification_fromStr(_fact_c, ontology)));

  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal(_fact_c, ontology)}, ontology.constants);
  _addFact(problem.worldState, _fact_b + "(p1)=aVal", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_b + "(p2)=valGoal", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_b + "(p3)=anotherVal", problem.goalStack, ontology, setOfEventsMap, _now);

  EXPECT_EQ(action1 + "(?p -> p2)", _lookForAnActionToDo(problem, domain, _now).actionInvocation.toStr());
}


void _assignAFactThenCheckEqualityWithAnotherFact()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity\n"
                                             "param");
  ontology.constants = ogp::SetOfEntities::fromPddl("aVal anotherVal valGoal v1 v2 v3 - entity\n"
                                                    "p1 p2 p3 pc1 pc2 pc3 - param", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + " - entity\n" +
                                                      _fact_b + "(?p - param) - entity\n" +
                                                      _fact_c + "(?p - param) - entity\n" +
                                                      _fact_d, ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> action1Parameters{_parameter("?p - param", ontology)};
  ogp::Action action1Obj({}, _worldStateModification_fromStr("assign(" + _fact_a + ", " + _fact_b + "(?p))", ontology, action1Parameters));
  action1Obj.parameters = std::move(action1Parameters);
  actions.emplace(action1, action1Obj);
  std::vector<ogp::Parameter> action2Parameters{_parameter("?pc - param", ontology)};
  ogp::Action action2Obj(_condition_fromStr("=(" + _fact_a + ", " + _fact_c + "(?pc))", ontology, action2Parameters),
                         _worldStateModification_fromStr(_fact_d, ontology, action2Parameters));
  action2Obj.parameters = std::move(action2Parameters);
  actions.emplace(action2, action2Obj);

  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal(_fact_d, ontology)}, ontology.constants);
  _addFact(problem.worldState, _fact_b + "(p1)=aVal", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_b + "(p2)=valGoal", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_b + "(p3)=anotherVal", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_c + "(pc1)=v1", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_c + "(pc2)=valGoal", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_c + "(pc3)=v3", problem.goalStack, ontology, setOfEventsMap, _now);

  EXPECT_EQ(action1 + "(?p -> p2)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ(action2 + "(?pc -> pc2)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
}



void _assignAFactThenCheckExistWithAnotherFact()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity\n"
                                             "param");
  ontology.constants = ogp::SetOfEntities::fromPddl("aVal anotherVal valGoal v1 v2 v3 - entity\n"
                                                    "p1 p2 p3 pc1 pc2 pc3 - param", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + " - entity\n" +
                                                      _fact_b + "(?p - param) - entity\n" +
                                                      _fact_c + "(?p - param) - entity\n" +
                                                      _fact_d, ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> action1Parameters{_parameter("?p - param", ontology)};
  ogp::Action action1Obj({}, _worldStateModification_fromStr("assign(" + _fact_a + ", " + _fact_b + "(?p))", ontology, action1Parameters));
  action1Obj.parameters = std::move(action1Parameters);
  actions.emplace(action1, action1Obj);
  ogp::Action action2Obj(_condition_fromStr("exists(?pc - param, =(" + _fact_a + ", " + _fact_c + "(?pc)))", ontology),
                         _worldStateModification_fromStr(_fact_d, ontology));
  actions.emplace(action2, action2Obj);

  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal(_fact_d, ontology)}, ontology.constants);
  _addFact(problem.worldState, _fact_b + "(p1)=aVal", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_b + "(p2)=valGoal", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_b + "(p3)=anotherVal", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_c + "(pc1)=v1", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_c + "(pc2)=valGoal", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_c + "(pc3)=v3", problem.goalStack, ontology, setOfEventsMap, _now);

  EXPECT_EQ(action1 + "(?p -> p2)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ(action2, _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
}


void _existWithEqualityInEvent()
{
  const std::string action1 = "action1";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity\n"
                                             "param");
  ontology.constants = ogp::SetOfEntities::fromPddl("aVal anotherVal valGoal v1 v2 v3 - entity\n"
                                                    "p1 p2 p3 pc1 pc2 pc3 - param", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + "(?p - param) - entity\n" +
                                                      _fact_b + "(?p - param)\n" +
                                                      _fact_c + "(?p - param) - entity\n" +
                                                      _fact_d + "(?p - param)", ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> action1Parameters{_parameter("?o - param", ontology)};
  ogp::Action action1Obj(_condition_fromStr(_fact_b + "(?o)", ontology, action1Parameters),
                         _worldStateModification_fromStr(_fact_d + "(?o)", ontology, action1Parameters));
  action1Obj.parameters = std::move(action1Parameters);
  actions.emplace(action1, action1Obj);

  ogp::SetOfEvents setOfEvents;
  std::vector<ogp::Parameter> derPred1Parameters{_parameter("?o - param", ontology)};
  ogp::Axiom derivedPredicate1(_condition_fromStr("exists(?pc - param, =(" + _fact_c + "(?pc), " + _fact_a + "(?o)))", ontology, derPred1Parameters),
                               _fact(_fact_b + "(?o)", ontology, derPred1Parameters), derPred1Parameters);
  for (auto& currEvent : derivedPredicate1.toEvents(ontology, {}))
    setOfEvents.add(currEvent);


  ogp::Domain domain(std::move(actions), ontology, std::move(setOfEvents));
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal(_fact_d + "(p2)", ontology)}, ontology.constants);
  _addFact(problem.worldState, _fact_a + "(p1)=aVal", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_a + "(p2)=valGoal", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_a + "(p3)=anotherVal", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_c + "(pc1)=v1", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_c + "(pc2)=valGoal", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_c + "(pc3)=v3", problem.goalStack, ontology, setOfEventsMap, _now);

  EXPECT_EQ(action1 + "(?o -> p2)", _lookForAnActionToDoConst(problem, domain, _now).actionInvocation.toStr());
  _removeFact(problem.worldState, _fact_c + "(pc1)=v1", problem.goalStack, ontology, setOfEventsMap, _now);
  EXPECT_EQ(action1 + "(?o -> p2)", _lookForAnActionToDoConst(problem, domain, _now).actionInvocation.toStr());
  _removeFact(problem.worldState, _fact_c + "(pc2)=valGoal", problem.goalStack, ontology, setOfEventsMap, _now);
  EXPECT_EQ("", _lookForAnActionToDoConst(problem, domain, _now).actionInvocation.toStr());
  _addFact(problem.worldState, _fact_c + "(pc1)=aVal", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_c + "(pc2)=valGoal", problem.goalStack, ontology, setOfEventsMap, _now);
  EXPECT_EQ(action1 + "(?o -> p2)", _lookForAnActionToDoConst(problem, domain, _now).actionInvocation.toStr());
  _setGoalsForAPriority(problem, {_goal(_fact_d + "(p1)", ontology)}, ontology.constants);
  EXPECT_EQ(action1 + "(?o -> p1)", _lookForAnActionToDoConst(problem, domain, _now).actionInvocation.toStr());
}


void _existWithEqualityInEvent_withEqualityInverted()
{
  const std::string action1 = "action1";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity\n"
                                             "param");
  ontology.constants = ogp::SetOfEntities::fromPddl("aVal anotherVal valGoal v1 v2 v3 - entity\n"
                                                    "p1 p2 p3 pc1 pc2 pc3 - param", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + "(?p - param) - entity\n" +
                                                      _fact_b + "(?p - param)\n" +
                                                      _fact_c + "(?p - param) - entity\n" +
                                                      _fact_d + "(?p - param)", ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> action1Parameters{_parameter("?o - param", ontology)};
  ogp::Action action1Obj(_condition_fromStr(_fact_b + "(?o)", ontology, action1Parameters),
                         _worldStateModification_fromStr(_fact_d + "(?o)", ontology, action1Parameters));
  action1Obj.parameters = std::move(action1Parameters);
  actions.emplace(action1, action1Obj);

  ogp::SetOfEvents setOfEvents;
  std::vector<ogp::Parameter> derPred1Parameters{_parameter("?o - param", ontology)};
  ogp::Axiom derivedPredicate1(_condition_fromStr("exists(?pc - param, =(" + _fact_a + "(?o), " + _fact_c + "(?pc)))", ontology, derPred1Parameters),
                               _fact(_fact_b + "(?o)", ontology, derPred1Parameters), derPred1Parameters);
  for (auto& currEvent : derivedPredicate1.toEvents(ontology, {}))
    setOfEvents.add(currEvent);


  ogp::Domain domain(std::move(actions), ontology, std::move(setOfEvents));
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal(_fact_d + "(p2)", ontology)}, ontology.constants);
  _addFact(problem.worldState, _fact_a + "(p1)=aVal", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_a + "(p2)=valGoal", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_a + "(p3)=anotherVal", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_c + "(pc1)=v1", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_c + "(pc2)=valGoal", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_c + "(pc3)=v3", problem.goalStack, ontology, setOfEventsMap, _now);

  EXPECT_EQ(action1 + "(?o -> p2)", _lookForAnActionToDoConst(problem, domain, _now).actionInvocation.toStr());
  _removeFact(problem.worldState, _fact_c + "(pc1)=v1", problem.goalStack, ontology, setOfEventsMap, _now);
  EXPECT_EQ(action1 + "(?o -> p2)", _lookForAnActionToDoConst(problem, domain, _now).actionInvocation.toStr());
  _removeFact(problem.worldState, _fact_c + "(pc2)=valGoal", problem.goalStack, ontology, setOfEventsMap, _now);
  EXPECT_EQ("", _lookForAnActionToDoConst(problem, domain, _now).actionInvocation.toStr());
  _addFact(problem.worldState, _fact_c + "(pc1)=aVal", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_c + "(pc2)=valGoal", problem.goalStack, ontology, setOfEventsMap, _now);
  EXPECT_EQ(action1 + "(?o -> p2)", _lookForAnActionToDoConst(problem, domain, _now).actionInvocation.toStr());
  _setGoalsForAPriority(problem, {_goal(_fact_d + "(p1)", ontology)}, ontology.constants);
  EXPECT_EQ(action1 + "(?o -> p1)", _lookForAnActionToDoConst(problem, domain, _now).actionInvocation.toStr());
}



void _fixEventWithValueInParameter()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity\n"
                                             "param");
  ontology.constants = ogp::SetOfEntities::fromPddl("aVal anotherVal valGoal v1 v2 v3 - entity\n"
                                                    "p1 p2 p3 pc1 pc2 pc3 titi - param", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + "(?p - param) - entity\n" +
                                                      _fact_b + "(?p - param) - entity\n" +
                                                      _fact_c + "(?p - param) - entity\n" +
                                                      _fact_d + "\n" +
                                                      _fact_e + "(?p - param) - entity", ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> action1Parameters{_parameter("?p - param", ontology)};
  ogp::Action action1Obj({}, _worldStateModification_fromStr("assign(" + _fact_a + "(titi), " + _fact_b + "(?p))", ontology, action1Parameters));
  action1Obj.parameters = std::move(action1Parameters);
  actions.emplace(action1, action1Obj);
  std::vector<ogp::Parameter> action2Parameters{_parameter("?pc - param", ontology)};
  ogp::Action action2Obj(_condition_fromStr("=(" + _fact_e + "(titi), " + _fact_c + "(?pc))", ontology, action2Parameters),
                         _worldStateModification_fromStr(_fact_d, ontology, action2Parameters));
  action2Obj.parameters = std::move(action2Parameters);
  actions.emplace(action2, action2Obj);

  ogp::SetOfEvents setOfEvents;
  std::vector<ogp::Parameter> derPred1Parameters{_parameter("?a - param", ontology), _parameter("?v - entity", ontology)};
  ogp::Axiom derivedPredicate1(_condition_fromStr(_fact_a + "(?a)=?v", ontology, derPred1Parameters),
                               _fact(_fact_e + "(?a)=?v", ontology, derPred1Parameters), derPred1Parameters);
  for (auto& currEvent : derivedPredicate1.toEvents(ontology, {}))
    setOfEvents.add(currEvent);

  ogp::Domain domain(std::move(actions), ontology, std::move(setOfEvents));

  EXPECT_EQ("action: action1\n"
            "----------------------------------\n"
            "\n"
            "fact: fact_a(titi)=*\n"
            "event: soe_from_constructor|event\n"
            "event: soe_from_constructor|event_2\n"
            "\n"
            "not action: action1\n"
            "\n"
            "\n"
            "action: action2\n"
            "----------------------------------\n"
            "\n"
            "not action: action2\n"
            "\n"
            "\n"
            "event: soe_from_constructor|event\n"
            "----------------------------------\n"
            "\n"
            "fact: fact_e(?a)=?v\n"
            "action: action2\n", domain.printSuccessionCache());
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal(_fact_d, ontology)}, ontology.constants);
  _addFact(problem.worldState, _fact_b + "(p2)=valGoal", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_c + "(pc2)=valGoal", problem.goalStack, ontology, setOfEventsMap, _now);

  EXPECT_EQ(action1 + "(?p -> p2)", _lookForAnActionToDo(problem, domain, _now).actionInvocation.toStr());
}


void _derivedPredicates()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity\n"
                                             "param");
  ontology.constants = ogp::SetOfEntities::fromPddl("aVal anotherVal valGoal v1 v2 v3 - entity\n"
                                                    "p1 p2 p3 pc1 pc2 pc3 titi - param", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + "(?p - param) - entity\n" +
                                                      _fact_b + "(?p - param) - entity\n" +
                                                      _fact_c + "(?p - param) - entity\n" +
                                                      _fact_d, ontology.types);
  ontology.derivedPredicates.addDerivedPredicate(
        ogp::DerivedPredicate(ogp::Predicate(_fact_e + "(?a - param) - entity", false, ontology.types),
                              _fact_a + "(?a)=?entity", ontology));

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> action1Parameters{_parameter("?p - param", ontology)};
  ogp::Action action1Obj({}, _worldStateModification_fromStr("assign(" + _fact_a + "(titi), " + _fact_b + "(?p))", ontology, action1Parameters));
  action1Obj.parameters = std::move(action1Parameters);
  actions.emplace(action1, action1Obj);
  std::vector<ogp::Parameter> action2Parameters{_parameter("?pc - param", ontology)};
  ogp::Action action2Obj(_condition_fromStr("=(" + _fact_e + "(titi), " + _fact_c + "(?pc))", ontology, action2Parameters),
                         _worldStateModification_fromStr(_fact_d, ontology, action2Parameters));
  action2Obj.parameters = std::move(action2Parameters);
  actions.emplace(action2, action2Obj);

  ogp::SetOfEvents setOfEvents;

  ogp::Domain domain(std::move(actions), ontology, std::move(setOfEvents));
  EXPECT_EQ("action: action1\n"
            "----------------------------------\n"
            "\n"
            "fact: fact_a(titi)=*\n"
            "action: action2\n"
            "\n"
            "not action: action1\n"
            "\n"
            "\n"
            "action: action2\n"
            "----------------------------------\n"
            "\n"
            "not action: action2\n", domain.printSuccessionCache());

  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal(_fact_d, ontology)}, ontology.constants);
  _addFact(problem.worldState, _fact_b + "(p2)=valGoal", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_c + "(pc2)=valGoal", problem.goalStack, ontology, setOfEventsMap, _now);

  EXPECT_EQ(action1 + "(?p -> p2)", _lookForAnActionToDo(problem, domain, _now).actionInvocation.toStr());
}


void _assignAFactTwoTimesInTheSamePlan()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity\n"
                                             "param");
  ontology.constants = ogp::SetOfEntities::fromPddl("aVal anotherVal valGoal v1 v2 v3 kitchen - entity\n"
                                                    "p1 p2 p3 pc1 pc2 pc3 - param", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + " - entity\n" +
                                                      _fact_b + "(?p - param) - entity\n" +
                                                      _fact_c + "(?p - param) - entity\n" +
                                                      _fact_d + "(?p - param) - entity", ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> action1Parameters{_parameter("?p - param", ontology)};
  ogp::Action action1Obj({}, _worldStateModification_fromStr("assign(" + _fact_a + ", " + _fact_b + "(?p))", ontology, action1Parameters));
  action1Obj.parameters = std::move(action1Parameters);
  actions.emplace(action1, action1Obj);

  std::vector<ogp::Parameter> action2Parameters{_parameter("?pc - param", ontology)};
  ogp::Action action2Obj(_condition_fromStr("=(" + _fact_a + ", " + _fact_c + "(?pc))", ontology, action2Parameters),
                         _worldStateModification_fromStr("assign(" + _fact_a + ", " + _fact_d + "(?pc))", ontology, action2Parameters));
  action2Obj.parameters = std::move(action2Parameters);
  actions.emplace(action2, action2Obj);

  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal(_fact_a + "=kitchen", ontology)}, ontology.constants);
  _addFact(problem.worldState, _fact_b + "(p1)=aVal", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_b + "(p2)=valGoal", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_b + "(p3)=anotherVal", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_c + "(pc1)=v1", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_c + "(pc2)=valGoal", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_c + "(pc3)=v3", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_d + "(pc2)=kitchen", problem.goalStack, ontology, setOfEventsMap, _now);

  EXPECT_EQ(action1 + "(?p -> p2)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ(action2 + "(?pc -> pc2)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
}


void _assignAFactTwoTimesInTheSamePlan2()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  const std::string action3 = "action3";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity\n"
                                             "param");
  ontology.constants = ogp::SetOfEntities::fromPddl("aVal anotherVal valGoal v1 v2 v3 kitchen - entity\n"
                                                    "p1 p2 p3 pc1 pc2 pc3 - param", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + " - entity\n" +
                                                      _fact_b + "(?p - param) - entity\n" +
                                                      _fact_c + "(?p - param) - entity\n" +
                                                      _fact_d + "(?p - param) - entity\n" +
                                                      _fact_e, ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> action1Parameters{_parameter("?p - param", ontology)};
  ogp::Action action1Obj({}, _worldStateModification_fromStr("assign(" + _fact_a + ", " + _fact_b + "(?p))", ontology, action1Parameters));
  action1Obj.parameters = std::move(action1Parameters);
  actions.emplace(action1, action1Obj);

  std::vector<ogp::Parameter> action2Parameters{_parameter("?pc - param", ontology)};
  ogp::Action action2Obj(_condition_fromStr("=(" + _fact_a + ", " + _fact_c + "(?pc))", ontology, action2Parameters),
                         _worldStateModification_fromStr("assign(" + _fact_a + ", " + _fact_d + "(?pc))", ontology, action2Parameters));
  action2Obj.parameters = std::move(action2Parameters);
  actions.emplace(action2, action2Obj);

  ogp::Action action3Obj(_condition_fromStr(_fact_a + "=kitchen", ontology, action2Parameters),
                         _worldStateModification_fromStr(_fact_e, ontology, action2Parameters));
  actions.emplace(action3, action3Obj);

  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal(_fact_e, ontology)}, ontology.constants);
  _addFact(problem.worldState, _fact_b + "(p1)=aVal", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_b + "(p2)=valGoal", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_b + "(p3)=anotherVal", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_c + "(pc1)=v1", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_c + "(pc2)=valGoal", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_c + "(pc3)=v3", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_d + "(pc2)=kitchen", problem.goalStack, ontology, setOfEventsMap, _now);

  EXPECT_EQ(action1 + "(?p -> p2)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ(action2 + "(?pc -> pc2)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ(action3, _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
}

void _checkTwoTimesTheEqualityOfAFact()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  const std::string action3 = "action3";

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity\n"
                                             "param");
  ontology.constants = ogp::SetOfEntities::fromPddl("aVal anotherVal valGoal v1 v2 v3 bedroom kitchen - entity\n"
                                                    "p1 p2 p3 pc1 pc2 pc3 - param", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr(_fact_a + " - entity\n" +
                                                      _fact_b + "(?p - param) - entity\n" +
                                                      _fact_c + "(?p - param) - entity\n" +
                                                      _fact_d + "(?p - param) - entity\n" +
                                                      _fact_e + "(?p - entity) - entity\n" +
                                                      _fact_f + " - entity", ontology.types);

  std::map<std::string, ogp::Action> actions;
  std::vector<ogp::Parameter> action1Parameters{_parameter("?p - param", ontology)};
  ogp::Action action1Obj({}, _worldStateModification_fromStr("assign(" + _fact_a + ", " + _fact_b + "(?p))", ontology, action1Parameters));
  action1Obj.parameters = std::move(action1Parameters);
  actions.emplace(action1, action1Obj);

  std::vector<ogp::Parameter> action2Parameters{_parameter("?pc - param", ontology)};
  ogp::Action action2Obj(_condition_fromStr("=(" + _fact_a + ", " + _fact_c + "(?pc))", ontology, action2Parameters),
                         _worldStateModification_fromStr("assign(" + _fact_a + ", " + _fact_d + "(?pc))", ontology, action2Parameters));
  action2Obj.parameters = std::move(action2Parameters);
  actions.emplace(action2, action2Obj);

  std::vector<ogp::Parameter> action3Parameters{_parameter("?tt - entity", ontology)};
  ogp::Action action3Obj(_condition_fromStr("=(" + _fact_a + ", " + _fact_e + "(?tt))", ontology, action3Parameters),
                         _worldStateModification_fromStr("assign(" + _fact_f + ", ?tt)", ontology, action3Parameters));
  action3Obj.parameters = std::move(action3Parameters);
  actions.emplace(action3, action3Obj);

  ogp::Domain domain(std::move(actions), ontology);
  auto& setOfEventsMap = domain.getSetOfEvents();
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_goal(_fact_f + "=bedroom", ontology)}, ontology.constants);
  _addFact(problem.worldState, _fact_b + "(p1)=aVal", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_b + "(p2)=valGoal", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_b + "(p3)=anotherVal", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_c + "(pc1)=v1", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_c + "(pc2)=valGoal", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_c + "(pc3)=v3", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_d + "(pc2)=kitchen", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, _fact_e + "(bedroom)=kitchen", problem.goalStack, ontology, setOfEventsMap, _now);

  EXPECT_EQ(action1 + "(?p -> p2)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ(action2 + "(?pc -> pc2)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ(action3 + "(?tt -> bedroom)", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
}



void _eventToRemoveAFactWithoutValue()
{
  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("loc_type\n"
                                             "robot");
  ontology.constants = ogp::SetOfEntities::fromPddl("me - robot\n"
                                                    "house1 house2 city anotherCity - loc_type", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("locationOfRobot(?r - robot) - loc_type\n"
                                                      "robotAt(?r - robot, ?l - loc_type)\n"
                                                      "within(?l1 - loc_type) - loc_type", ontology.types);

  std::map<std::string, ogp::Action> actions;
  ogp::SetOfEvents setOfEvents;
  std::vector<ogp::Parameter> inf1Parameters{_parameter("?l - loc_type", ontology)};
  ogp::Event event1(_condition_fromStr("locationOfRobot(me)=?l", ontology, inf1Parameters),
                    _worldStateModification_fromStr("robotAt(me, ?l)", ontology, inf1Parameters));
  event1.parameters = std::move(inf1Parameters);
  setOfEvents.add(event1);

  std::vector<ogp::Parameter> inf2Parameters{_parameter("?l - loc_type", ontology)};
  ogp::Event event2(_condition_fromStr("exists(?loc - loc_type, locationOfRobot(me)=?loc & within(?loc)=?l)", ontology, inf2Parameters),
                    _worldStateModification_fromStr("robotAt(me, ?l)", ontology, inf2Parameters));
  event2.parameters = std::move(inf2Parameters);
  setOfEvents.add(event2);

  std::vector<ogp::Parameter> inf3Parameters{_parameter("?l - loc_type", ontology)};
  ogp::Event event3(_condition_fromStr("!locationOfRobot(me)=?l", ontology, inf3Parameters),
                    _worldStateModification_fromStr("forall(?ll - loc_type, robotAt(me, ?ll), !robotAt(me, ?ll))", ontology, inf3Parameters));
  event3.parameters = std::move(inf3Parameters);
  setOfEvents.add(event3);
  ogp::Domain domain(std::move(actions), ontology, std::move(setOfEvents));

  auto& setOfEventsMap = domain.getSetOfEvents();

  ogp::Problem problem;
  _addFact(problem.worldState, "within(house1)=city", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, "within(house2)=city", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFact(problem.worldState, "locationOfRobot(me)=house1", problem.goalStack, ontology, setOfEventsMap, _now);
  EXPECT_TRUE(_hasFact(problem.worldState, "robotAt(me, house1)", ontology));
  EXPECT_TRUE(_hasFact(problem.worldState, "robotAt(me, city)", ontology));
  _removeFact(problem.worldState, "locationOfRobot(me)=house1", problem.goalStack, ontology, setOfEventsMap, _now);
  EXPECT_FALSE(_hasFact(problem.worldState, "robotAt(me, house1)", ontology));
  EXPECT_FALSE(_hasFact(problem.worldState, "robotAt(me, city)", ontology));
  _addFact(problem.worldState, "locationOfRobot(me)=house1", problem.goalStack, ontology, setOfEventsMap, _now);
  EXPECT_TRUE(_hasFact(problem.worldState, "robotAt(me, house1)", ontology));
  EXPECT_TRUE(_hasFact(problem.worldState, "robotAt(me, city)", ontology));
  _addFact(problem.worldState, "locationOfRobot(me)=city", problem.goalStack, ontology, setOfEventsMap, _now);
  EXPECT_FALSE(_hasFact(problem.worldState, "robotAt(me, house1)", ontology));
  EXPECT_TRUE(_hasFact(problem.worldState, "robotAt(me, city)", ontology));
  _addFact(problem.worldState, "locationOfRobot(me)=anotherCity", problem.goalStack, ontology, setOfEventsMap, _now);
  EXPECT_FALSE(_hasFact(problem.worldState, "robotAt(me, house1)", ontology));
  EXPECT_FALSE(_hasFact(problem.worldState, "robotAt(me, city)", ontology));
}


}



TEST(Planner, test_planWithSingleType)
{
  _test_createEmptyGoal();
  _test_goalToStr();
  _test_factToStr();
  _test_conditionParameters();
  _test_wsModificationToStr();
  _test_invertCondition();
  _test_checkCondition();
  _automaticallyRemoveGoalsWithAMaxTimeToKeepInactiveEqualTo0();
  _maxTimeToKeepInactiveEqualTo0UnderAnAlreadySatisfiedGoal();
  _noPreconditionGoalImmediatlyReached();
  _removeGoalWhenItIsSatisfiedByAnAction();
  _removeAnAction();
  _removeSomeGoals();
  _notifyGoalRemovedWhenItIsImmediatlyRemoved();
  _handlePreconditionWithNegatedFacts();
  _testWithNegatedAccessibleFacts();
  _noPlanWithALengthOf2();
  _noPlanWithALengthOf3();
  _2preconditions();
  _2Goals();
  _2UnrelatedGoals();
  _impossibleGoal();
  _privigelizeTheActionsThatHaveManyPreferedInContext();
  _preconditionThatCannotBeSolved();
  _preferInContext();
  _preferWhenPreconditionAreCloserToTheRealFacts();
  _avoidToDo2TimesTheSameActionIfPossble();
  _takeHistoricalIntoAccount();
  _goDoTheActionThatHaveTheMostPreferInContextValidated();
  _checkNotInAPrecondition();
  _checkClearGoalsWhenItsAlreadySatisfied();
  _checkActionHasAFact();
  _precoditionEqualEffect();
  _addGoalEvenForEmptyAction();
  _circularDependencies();
  _triggerActionThatRemoveAFact();
  _actionWithConstantValue();
  _actionWithParameterizedValue();
  _actionWithParameterizedParameter();
  _actionWithParametersInPreconditionsAndEffectsWithoutSolution();
  _actionWithParametersInsideThePath();
  _testPersistGoal();
  _testPersistImplyGoal();
  _testImplyGoal();
  _checkPreviousBugAboutSelectingAnInappropriateAction();
  _dontLinkActionWithPreferredInContext();
  _checkPriorities();
  _stackablePropertyOfGoals();
  _doNotRemoveAGoalWithMaxTimeToKeepInactiveEqual0BelowAGoalWithACondotionNotSatisfied();
  _checkMaxTimeToKeepInactiveForGoals();
  _changePriorityOfGoal();
  _factChangedNotification();
  _checkEvents();
  _checkEventsWithImply();
  _checkEventWithPunctualCondition();
  _checkEventAtEndOfAPlan();
  _checkEventInsideAPlan();
  _checkEventThatAddAGoal();
  _testGetNotSatisfiedGoals();
  _testGoalUnderPersist();
  _checkLinkedEvents();
  _oneStepTowards();
  _infrenceLinksFromManyEventsSets();
  _factValueModification();
  _removeGoaWhenAnActionFinishesByAddingNewGoals();
  _setWsModification();
  _forAllWsModification();
  _actionNavigationAndGrabObjectWithParameters();
  _actionNavigationAndGrabObjectWithParameters2();
  _moveObject();
  _moveAndUngrabObject();
  _failToMoveAnUnknownObject();
  _completeMovingObjectScenario();
  _moveAnObject();
  _matchSkillWithObjectsWithASubType();
  _eventWithANegatedFactWithParameter();
  _actionWithANegatedFactNotTriggeredIfNotNecessary();
  _useTwoTimesAnEvent();
  _linkWithAnyValueInCondition();
  _removeAFactWithAnyValue();
  _notDeducePathIfTheParametersOfAFactAreDifferents();
  _checkPreferInContext();
  _checkPreferHighImportanceOfNotRepeatingIt();
  _actionWithFactWithANegatedFact();
  _negatedFactValueInWorldState();
  _problemThatUseADomainThatChangedSinceLastUsage();
  _checkFilterFactInCondition();
  _checkFilterFactInConditionAndThenPropagate();
  _checkOutputValueOfLookForAnActionToDo();
  _hardProblemThatNeedsToBeSmart();
  _hardProblemThatNeedsToBeSmartWithAnEvent();
  _checkOverallEffectDuringParallelisation();
  _orInCondition();
  _axioms();
  _assignAnotherValueToSatisfyNotGoal();
  _assignUndefined();
  _assignAFact();
  _assignAFactToAction();
  _assignAFactThenCheckEqualityWithAnotherFact();
  _assignAFactThenCheckExistWithAnotherFact();
  _existWithEqualityInEvent();
  _existWithEqualityInEvent_withEqualityInverted();
  _fixEventWithValueInParameter();
  _derivedPredicates();
  _assignAFactTwoTimesInTheSamePlan();
  _assignAFactTwoTimesInTheSamePlan2();
  _checkTwoTimesTheEqualityOfAFact();
  _eventToRemoveAFactWithoutValue();
}
