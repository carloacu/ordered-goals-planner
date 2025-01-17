#include <gtest/gtest.h>
#include <orderedgoalsplanner/orderedgoalsplanner.hpp>
#include <orderedgoalsplanner/types/setofcallbacks.hpp>
#include <orderedgoalsplanner/types/setofevents.hpp>
#include <orderedgoalsplanner/util/serializer/deserializefrompddl.hpp>

namespace
{
const std::unique_ptr<std::chrono::steady_clock::time_point> _now = {};
const ogp::SetOfCallbacks _emptyCallbacks;


static const std::vector<ogp::Parameter> _emptyParameters;


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


std::string _solveStr(ogp::Problem& pProblem,
                      const ogp::Domain& pDomain,
                      const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow = {})
{
  return ogp::planToStr(ogp::planForEveryGoals(pProblem, pDomain, pNow), ", ");
}



void _veryTestInvolvingDuration()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  const std::string action3 = "action3";

  ogp::Ontology ontology;
  ontology.predicates = ogp::SetOfPredicates::fromStr("fact_a\n"
                                                      "fact_b", ontology.types);

  std::map<std::string, ogp::Action> actions;
  ogp::Action actionObj1({}, _worldStateModification_fromPddl("(fact_a)", ontology));
  actionObj1.duration = 1;
  actions.emplace(action1, actionObj1);

  ogp::Action actionObj2(_condition_fromPddl("(fact_a)", ontology),
                         _worldStateModification_fromPddl("(fact_b)", ontology));
  actionObj2.duration = 3;
  actions.emplace(action2, actionObj2);

  ogp::Action actionObj3({},
                         _worldStateModification_fromPddl("(fact_b)", ontology));
  actionObj3.duration = 10;
  actions.emplace(action3, actionObj3);

  ogp::Domain domain(std::move(actions), ontology);
  ogp::Problem problem;
  _setGoalsForAPriority(problem, {_pddlGoal("(fact_b)", ontology)}, ontology.constants);
  EXPECT_EQ(action1, _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ(action2, _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
  EXPECT_EQ("", _lookForAnActionToDoThenNotify(problem, domain, _now).actionInvocation.toStr());
}


void _pathWothSmalldurationsInsteadOfOnrBigDuration()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  const std::string action3 = "action3";
  const std::string action4 = "action4";

  std::vector<std::pair<ogp::Number, std::string>> act2DurationToExpectedPlan{
    {1, "action2"},
    {2, "action2"},
    {4, "action1, action3, action4"}
  };

  for (const auto& currTestParams : act2DurationToExpectedPlan)
  {
    const auto& act2Duration = currTestParams.first;
    const auto& expectedPlan = currTestParams.second;

    ogp::Ontology ontology;
    ontology.predicates = ogp::SetOfPredicates::fromStr("fact_a\n"
                                                        "fact_b\n"
                                                        "fact_c", ontology.types);

    std::map<std::string, ogp::Action> actions;
    ogp::Action actionObj1({}, _worldStateModification_fromPddl("(fact_a)", ontology));
    actionObj1.duration = 1;
    actions.emplace(action1, actionObj1);

    ogp::Action actionObj2({},
                           _worldStateModification_fromPddl("(and (fact_a) (fact_b) (fact_c))", ontology));
    actionObj2.duration = act2Duration;
    actions.emplace(action2, actionObj2);

    ogp::Action actionObj3({},
                           _worldStateModification_fromPddl("(fact_b)", ontology));
    actionObj3.duration = 1;
    actions.emplace(action3, actionObj3);

    ogp::Action actionObj4({},
                           _worldStateModification_fromPddl("(fact_c)", ontology));
    actionObj4.duration = 1;
    actions.emplace(action4, actionObj4);

    ogp::Domain domain(std::move(actions), ontology);
    ogp::Problem problem;
    _setGoalsForAPriority(problem, {_pddlGoal("(and (fact_a) (fact_b) (fact_c))", ontology)}, ontology.constants);

    EXPECT_EQ(expectedPlan, _solveStr(problem, domain)) << "for action 2 duration: " + ogp::numberToString(act2Duration);
  }
}


}



TEST(Planner, test_duration)
{
  _veryTestInvolvingDuration();
  _pathWothSmalldurationsInsteadOfOnrBigDuration();
}
