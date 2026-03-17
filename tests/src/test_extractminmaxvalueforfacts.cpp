#include <gtest/gtest.h>
#include <orderedgoalsplanner/types/domain.hpp>
#include <orderedgoalsplanner/types/problem.hpp>
#include <orderedgoalsplanner/util/extactminmaxvalueforfacts.hpp>
#include <orderedgoalsplanner/util/serializer/deserializefrompddl.hpp>

namespace
{

float _numberToFloat(const ogp::Number& pNumber)
{
  return std::visit([](auto pValue) {
    return static_cast<float>(pValue);
  }, pNumber);
}

void _test_extractMinMaxValuesForFacts_fromGoalsDerivedPredicatesAndActions()
{
  const std::string domainPddl = R"((define
    (domain minmax)
    (:requirements :strips :typing :derived-predicates)

    (:types
        robot - object
    )

    (:functions
        (battery ?r - robot) - number
        (load) - number
        (pressure) - number
        (speed) - number
        (temperature) - number
    )

    (:derived (battery-safe ?r - robot)
        (and
            (>= (battery ?r) 3)
            (<= (battery ?r) 8)
        )
    )

    (:action move
        :parameters
            (?r - robot)

        :precondition
            (and
                (<= (pressure) 12)
                (>= (speed) 2)
                (<= (speed) 5)
            )

        :effect
            (assign (speed) 0)
    )
))";

  const std::string problemPddl = R"((define
    (problem minmax-problem)
    (:domain minmax)

    (:objects
        r1 - robot
    )

    (:goal
        (and
            (>= (load) 7)
            (>= (temperature) 10)
            (<= (temperature) 20)
        )
    )
))";

  auto domain = ogp::pddlToDomain(domainPddl, false, {});
  auto domainAndProblemPtrs = ogp::pddlToProblem(problemPddl, domain);

  auto minMaxValues = ogp::extractMinMaxValuesForFacts(*domainAndProblemPtrs.problemPtr, domain);

  ASSERT_EQ(5u, minMaxValues.size());

  const auto& batteryValues = minMaxValues.at("battery");
  ASSERT_TRUE(batteryValues.min.has_value());
  ASSERT_TRUE(batteryValues.max.has_value());
  EXPECT_FLOAT_EQ(3.f, _numberToFloat(*batteryValues.min));
  EXPECT_FLOAT_EQ(8.f, _numberToFloat(*batteryValues.max));

  const auto& loadValues = minMaxValues.at("load");
  ASSERT_TRUE(loadValues.min.has_value());
  ASSERT_FALSE(loadValues.max.has_value());
  EXPECT_FLOAT_EQ(7.f, _numberToFloat(*loadValues.min));

  const auto& pressureValues = minMaxValues.at("pressure");
  ASSERT_FALSE(pressureValues.min.has_value());
  ASSERT_TRUE(pressureValues.max.has_value());
  EXPECT_FLOAT_EQ(12.f, _numberToFloat(*pressureValues.max));

  const auto& speedValues = minMaxValues.at("speed");
  ASSERT_TRUE(speedValues.min.has_value());
  ASSERT_TRUE(speedValues.max.has_value());
  EXPECT_FLOAT_EQ(2.f, _numberToFloat(*speedValues.min));
  EXPECT_FLOAT_EQ(5.f, _numberToFloat(*speedValues.max));

  const auto& temperatureValues = minMaxValues.at("temperature");
  ASSERT_TRUE(temperatureValues.min.has_value());
  ASSERT_TRUE(temperatureValues.max.has_value());
  EXPECT_FLOAT_EQ(10.f, _numberToFloat(*temperatureValues.min));
  EXPECT_FLOAT_EQ(20.f, _numberToFloat(*temperatureValues.max));
}

}

TEST(Tool, test_extractMinMaxValuesForFacts)
{
  _test_extractMinMaxValuesForFacts_fromGoalsDerivedPredicatesAndActions();
}
