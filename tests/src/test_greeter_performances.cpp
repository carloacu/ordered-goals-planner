#include "plannerusingexternaldata.hpp"
#include <gtest/gtest.h>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <vector>
#include <orderedgoalsplanner/orderedgoalsplanner.hpp>
#include <orderedgoalsplanner/types/domain.hpp>
#include <orderedgoalsplanner/types/parallelplan.hpp>
#include <orderedgoalsplanner/types/problem.hpp>
#include <orderedgoalsplanner/types/setofcallbacks.hpp>
#include <orderedgoalsplanner/util/serializer/deserializefrompddl.hpp>

// The Greeter demo's domain, as Harmony generates it, types its agents with `(either physical_agent
// social_agent)`, and what has a location with `(either physical_object social_agent)`. Without unions,
// these parameters could only be typed with their common ancestor, `entity`. These tests plan the
// Greeter's situations with both domains, to check that unions cost little.

namespace
{
const ogp::SetOfCallbacks _emptyCallbacks;
const std::string _agentUnion = "(either physical_agent social_agent)";
constexpr int _nbOfRepetitions = 5;
// Unions are expected to cost little: the bound is loose, so that a loaded machine does not fail the test.
constexpr double _maxDurationRatio = 2.0;
const std::chrono::microseconds _durationTolerance(2000);

std::string _readFile(const std::string& pPath)
{
  std::ifstream file(pPath);
  if (!file.is_open())
    throw std::runtime_error("Could not open file " + pPath);
  std::stringstream content;
  content << file.rdbuf();
  return content.str();
}

std::string _withoutUnions(const std::string& pDomainPddl)
{
  static const std::regex unionType(R"(\(either [^)]*\))");
  return std::regex_replace(pDomainPddl, unionType, "entity");
}

/// The visitor acts through the first human, every human is visible, the even ones are engaged and every
/// third one was already welcomed.
std::string _greeterProblem(int pNbOfHumans, const std::string& pGoal)
{
  std::string objects;
  std::string init = "(= (acts_through visitor) human_1)\n"
                     "(= (location_of me) hall)\n"
                     "(= (is_carrying_too_heavy_payload) false)\n";
  for (int i = 1; i <= pNbOfHumans; ++i)
  {
    const auto human = "human_" + std::to_string(i);
    objects += human + " - user\n";
    init += "(is_visible " + human + ")\n";
    init += "(is_detected " + human + ")\n";
    init += "(= (location_of " + human + ") hall)\n";
    if (i % 2 == 0)
      init += "(engages me " + human + ")\n";
    if (i % 3 == 0)
      init += "(has_told welcome me " + human + ")\n";
  }
  return "(define\n"
         "(problem greeter_problem)\n"
         "(:domain harmony)\n"
         "(:objects\n" + objects +
         "visitor - social_agent\n"
         "hall kitchen - location\n"
         "welcome dr_brown - string\n"
         ")\n"
         "(:init\n" + init + ")\n"
         "(:goal\n" + pGoal + "\n)\n"
         ")";
}

struct PlanningMeasure
{
  std::string plan;
  std::chrono::microseconds fastestDuration;
};

/// Parse the problem and plan for it, as Harmony does for each planning, and keep the fastest repetition.
PlanningMeasure _measure(const ogp::Domain& pDomain, const std::string& pProblemPddl)
{
  PlanningMeasure res{"", std::chrono::microseconds::max()};
  for (int i = 0; i < _nbOfRepetitions; ++i)
  {
    const auto start = std::chrono::steady_clock::now();
    auto domainAndProblemPtrs = ogp::pddlToProblem(pProblemPddl, pDomain);
    auto& problem = *domainAndProblemPtrs.problemPtr;
    auto parallelPlan = ogp::parallelPlanForEveryGoals(problem, pDomain, _emptyCallbacks, {}, nullptr);
    auto plan = ogp::parallelPlanToPddl(parallelPlan, pDomain);
    const auto end = std::chrono::steady_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    res.fastestDuration = std::min(res.fastestDuration, duration);
    res.plan = std::move(plan);
  }
  return res;
}

void _test_planAsFastWithUnions(const std::string& pDataPath,
                                const std::string& pSituation,
                                const std::string& pGoal,
                                const std::string& pExpectedPlanStart,
                                const std::vector<int>& pNbsOfHumans = {5, 50})
{
  const auto unionDomainPddl = _readFile(pDataPath + "/greeter/domain.pddl");
  ASSERT_NE(std::string::npos, unionDomainPddl.find(_agentUnion));
  const auto entityDomainPddl = _withoutUnions(unionDomainPddl);
  ASSERT_EQ(std::string::npos, entityDomainPddl.find("(either"));
  const auto unionDomain = ogp::pddlToDomain(unionDomainPddl, true, {});
  const auto entityDomain = ogp::pddlToDomain(entityDomainPddl, true, {});

  for (int nbOfHumans : pNbsOfHumans)
  {
    const auto problemPddl = _greeterProblem(nbOfHumans, pGoal);

    const auto withUnion = _measure(unionDomain, problemPddl);
    const auto withEntity = _measure(entityDomain, problemPddl);

    std::cout << "[ PERF     ] " << pSituation << " with " << nbOfHumans << " humans: "
              << withUnion.fastestDuration.count() << " us with unions, "
              << withEntity.fastestDuration.count() << " us with entity" << std::endl;
    EXPECT_EQ(0u, withUnion.plan.rfind(pExpectedPlanStart, 0)) << pSituation << ":\n" << withUnion.plan;
    EXPECT_EQ(withEntity.plan, withUnion.plan) << pSituation << " with " << nbOfHumans << " humans";
    EXPECT_LE(withUnion.fastestDuration.count(),
              _maxDurationRatio * withEntity.fastestDuration.count() + _durationTolerance.count())
        << pSituation << " with " << nbOfHumans << " humans";
  }
}

}


TEST_F(PlannerUsingExternalData, test_greeter_performances_guide_the_visitor)
{
  _test_planAsFastWithUnions(PlannerUsingExternalData::dataPath, "guide the visitor",
                             "(= (location_of visitor) kitchen)",
                             "00: (guide_social_agent_to visitor kitchen)");
}

TEST_F(PlannerUsingExternalData, test_greeter_performances_tell_the_visitor)
{
  _test_planAsFastWithUnions(PlannerUsingExternalData::dataPath, "tell the visitor",
                             "(has_told welcome me visitor)",
                             "00: (find_social_agent visitor)");
}

// Few humans: with or without unions, planning for a goal quantified over the humans takes exponentially
// longer with their number (seconds from 4 humans).
TEST_F(PlannerUsingExternalData, test_greeter_performances_welcome_every_visible_human)
{
  _test_planAsFastWithUnions(PlannerUsingExternalData::dataPath, "welcome every visible human",
                             "(forall (?u - user) (imply (is_visible ?u) (has_told welcome me ?u)))",
                             "00: (tell_with_reformulation human_1 welcome)", {2, 3});
}

TEST_F(PlannerUsingExternalData, test_greeter_performances_inform_about_the_visitor)
{
  _test_planAsFastWithUnions(PlannerUsingExternalData::dataPath, "inform about the visitor",
                             "(employee_informed_about_guest dr_brown visitor)",
                             "00: (inform_employee_about_guest_arrival dr_brown visitor)");
}
