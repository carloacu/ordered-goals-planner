#include <gtest/gtest.h>
#include <orderedgoalsplanner/orderedgoalsplanner.hpp>
#include <orderedgoalsplanner/types/predicate.hpp>
#include <orderedgoalsplanner/types/setofcallbacks.hpp>
#include <orderedgoalsplanner/types/setofevents.hpp>
#include <orderedgoalsplanner/util/serializer/deserializefrompddl.hpp>

namespace
{
const std::unique_ptr<std::chrono::steady_clock::time_point> _now = {};
const ogp::SetOfCallbacks _emptyCallbacks;


ogp::Parameter _parameter(const std::string& pStr,
                          const ogp::Ontology& pOntology)
{
  return ogp::Parameter::fromStr(pStr, pOntology.types);
}

ogp::Goal _goal_fromPddl(const std::string& pStr,
                         const ogp::Ontology& pOntology)
{
  std::size_t pos = 0;
  return *ogp::pddlToGoal(pStr, pos, pOntology, {});
}

std::unique_ptr<ogp::Condition> _condition_fromPddl(const std::string& pConditionStr,
                                                    const ogp::Ontology& pOntology,
                                                    const std::vector<ogp::Parameter>& pParameters = {})
{
  std::size_t pos = 0;
  return ogp::pddlToCondition(pConditionStr, pos, pOntology, {}, pParameters);
}

std::unique_ptr<ogp::WorldStateModification> _worldStateModification_fromPddl(
    const std::string& pStr,
    const ogp::Ontology& pOntology,
    const std::vector<ogp::Parameter>& pParameters = {})
{
  std::size_t pos = 0;
  return ogp::pddlToWsModification(pStr, pos, pOntology, {}, pParameters);
}

void _addFactFromPddl(ogp::WorldState& pWorldState,
                      const std::string& pFactStr,
                      ogp::GoalStack& pGoalStack,
                      const ogp::Ontology& pOntology,
                      const std::map<ogp::SetOfEventsId, ogp::SetOfEvents>& pSetOfEvents = {},
                      const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {})
{
  std::size_t pos = 0;
  pWorldState.addFact(ogp::Fact::fromPddl(pFactStr, pOntology, {}, {}, pos, &pos),
                      pGoalStack, pSetOfEvents, _emptyCallbacks, pOntology, ogp::SetOfEntities(), pNow);
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

}


TEST(Planner, test_eventWithForallGoalAddsAnUnnecessaryMoveAtBeginning)
{
  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("location physical_object\n"
                                             "physical_agent - physical_object\n"
                                             "user robot - physical_agent");
  ontology.constants = ogp::SetOfEntities::fromPddl("me - robot\n"
                                                    "unknown_human - user\n"
                                                    "front_of_gifts_1_loc front_of_reception_desk_loc - location",
                                                    ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("is_detected(?object - physical_object)\n"
                                                      "holds(?agent - physical_agent, ?object - physical_object)\n"
                                                      "location_of(?object - physical_object) - location\n"
                                                      "wanted_location_of(?object - physical_object) - location",
                                                      ontology.types);

  std::map<std::string, ogp::Action> actions;

  std::vector<ogp::Parameter> goToLocationParameters{_parameter("?location - location", ontology)};
  ogp::Action goToLocation({}, _worldStateModification_fromPddl("(assign (location_of me) ?location)",
                                                                ontology, goToLocationParameters));
  goToLocation.parameters = std::move(goToLocationParameters);
  actions.emplace("go_to_location", goToLocation);

  std::vector<ogp::Parameter> guideUserToParameters{_parameter("?user - user", ontology),
                                                    _parameter("?destination - location", ontology)};
  ogp::Action guideUserTo(_condition_fromPddl("(is_detected ?user)", ontology, guideUserToParameters),
                          _worldStateModification_fromPddl(
                              "(and (assign (location_of me) ?destination)"
                              "     (assign (location_of ?user) ?destination))",
                              ontology, guideUserToParameters));
  guideUserTo.parameters = std::move(guideUserToParameters);
  actions.emplace("guide_user_to", guideUserTo);

  ogp::Action lookAroundUntilHumanIsSeen({}, _worldStateModification_fromPddl("(is_detected unknown_human)", ontology));
  actions.emplace("look_around_until_human_is_seen", lookAroundUntilHumanIsSeen);

  std::vector<ogp::Parameter> eventParameters{_parameter("?object - physical_object", ontology),
                                              _parameter("?location - location", ontology)};
  ogp::SetOfEvents setOfEvents;
  setOfEvents.add(ogp::Event(_condition_fromPddl("(and (holds me ?object) (= (location_of me) ?location))", ontology, eventParameters),
                             _worldStateModification_fromPddl("(assign (location_of ?object) ?location)", ontology, eventParameters),
                             eventParameters));

  ogp::Domain domain(std::move(actions), ontology, std::move(setOfEvents));
  auto& setOfEventsMap = domain.getSetOfEvents();

  ogp::Problem problem;
  _addFactFromPddl(problem.worldState, "(= (location_of me) front_of_reception_desk_loc)", problem.goalStack, ontology, setOfEventsMap, _now);
  _addFactFromPddl(problem.worldState, "(= (wanted_location_of unknown_human) front_of_gifts_1_loc)", problem.goalStack, ontology, setOfEventsMap, _now);
  _setGoalsForAPriority(problem,
                        {_goal_fromPddl("(forall (?user_param - user)"
                                        "  (= (location_of ?user_param) (wanted_location_of ?user_param)))",
                                        ontology)},
                        ontology.constants);

  EXPECT_EQ("look_around_until_human_is_seen",
            _lookForAnActionToDoThenNotify(problem, domain, _emptyCallbacks, _now).actionInvocation.toStr());
  EXPECT_EQ("guide_user_to(?destination -> front_of_gifts_1_loc, ?user -> unknown_human)",
            _lookForAnActionToDoThenNotify(problem, domain, _emptyCallbacks, _now).actionInvocation.toStr());
  EXPECT_EQ("",
            _lookForAnActionToDoThenNotify(problem, domain, _emptyCallbacks, _now).actionInvocation.toStr());
}
