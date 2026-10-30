#include <gtest/gtest.h>
#include <orderedgoalsplanner/orderedgoalsplanner.hpp>
#include <orderedgoalsplanner/types/domain.hpp>
#include <orderedgoalsplanner/types/problem.hpp>
#include <orderedgoalsplanner/types/setofcallbacks.hpp>
#include <orderedgoalsplanner/types/setoftypes.hpp>
#include <orderedgoalsplanner/util/serializer/deserializefrompddl.hpp>
#include <orderedgoalsplanner/util/serializer/serializeinpddl.hpp>

namespace
{
const ogp::SetOfCallbacks _emptyCallbacks;

const std::string _types = "robot user - physical_agent\n"
                           "social_agent\n"
                           "location";

const std::string _domainPddl = R"((define
    (domain either_types)
    (:requirements :strips :typing :universal-preconditions)

    (:types
        robot user - physical_agent
        social_agent
        location
    )

    (:predicates
        (has_told ?to - (either user social_agent))
    )

    (:action tell
        :parameters
            (?to - (either user social_agent))

        :effect
            (has_told ?to)
    )
))";

std::string _problemPddl(const std::string& pGoal,
                         const std::string& pInit = "")
{
  return R"((define
    (problem either_types_problem)
    (:domain either_types)

    (:objects
        human_1 - user
        visitor - social_agent
        kitchen - location
    )

    (:init
        )" + pInit + R"(
    )

    (:goal
        )" + pGoal + R"(
    )
))";
}

std::string _plan(const std::string& pGoal)
{
  auto domain = ogp::pddlToDomain(_domainPddl, false, {});
  auto domainAndProblemPtrs = ogp::pddlToProblem(_problemPddl(pGoal), domain);
  auto& problem = *domainAndProblemPtrs.problemPtr;
  return ogp::planToPddl(ogp::planForEveryGoals(problem, domain, _emptyCallbacks, {}), domain);
}


void _test_eitherTypeIsTheSameWhateverTheOrderOfItsMembers()
{
  auto setOfTypes = ogp::SetOfTypes::fromPddl(_types);

  auto eitherType = setOfTypes.nameToType("(either user social_agent)");
  auto sameEitherType = setOfTypes.nameToType("(either  social_agent\tuser )");

  ASSERT_TRUE(eitherType);
  EXPECT_EQ(eitherType, sameEitherType);
  EXPECT_EQ("(either social_agent user)", eitherType->name);
  EXPECT_TRUE(eitherType->isEither);
  EXPECT_EQ(2u, eitherType->subTypes.size());
}


void _test_eitherTypeOfOneMemberIsThatMember()
{
  auto setOfTypes = ogp::SetOfTypes::fromPddl(_types);

  auto eitherType = setOfTypes.nameToType("(either user user)");

  EXPECT_EQ(setOfTypes.nameToType("user"), eitherType);
}


void _test_eitherTypeOnlyOfDeclaredTypes()
{
  auto setOfTypes = ogp::SetOfTypes::fromPddl(_types);

  EXPECT_THROW(setOfTypes.nameToType("(either user visitor)"), std::runtime_error);
  EXPECT_THROW(setOfTypes.nameToType("(either )"), std::runtime_error);
  EXPECT_THROW(setOfTypes.nameToType("(eitheruser)"), std::runtime_error);
}


void _test_eitherTypeIsA()
{
  auto setOfTypes = ogp::SetOfTypes::fromPddl(_types);
  auto user = setOfTypes.nameToType("user");
  auto robot = setOfTypes.nameToType("robot");
  auto physicalAgent = setOfTypes.nameToType("physical_agent");
  auto agent = setOfTypes.nameToType("(either user social_agent)");
  auto body = setOfTypes.nameToType("(either robot user)");

  EXPECT_TRUE(user->isA(*agent));
  EXPECT_FALSE(robot->isA(*agent));
  EXPECT_FALSE(physicalAgent->isA(*agent));
  EXPECT_TRUE(agent->isA(*agent));
  EXPECT_TRUE(body->isA(*physicalAgent));
  EXPECT_FALSE(agent->isA(*physicalAgent));
  EXPECT_FALSE(agent->isA(*user));
}


void _test_planForAMemberOfAnEitherParameter()
{
  EXPECT_EQ("00: (tell visitor) [1]\n", _plan("(has_told visitor)"));
  EXPECT_EQ("00: (tell human_1) [1]\n", _plan("(forall (?u - user) (has_told ?u))"));
}


void _test_conditionOverAnEitherParameter()
{
  auto domain = ogp::pddlToDomain(_domainPddl, false, {});
  auto domainAndProblemPtrs = ogp::pddlToProblem(_problemPddl("(has_told visitor)", "(has_told visitor)"), domain);
  auto& problem = *domainAndProblemPtrs.problemPtr;
  const auto& ontology = domain.getOntology();
  std::size_t pos = 0;

  auto someoneWasTold = ogp::pddlToCondition("(exists (?a - (either user social_agent)) (has_told ?a))",
                                             pos, ontology, problem.objects, {});

  ASSERT_TRUE(someoneWasTold);
  EXPECT_TRUE(someoneWasTold->isTrue(problem.worldState, ontology.constants, problem.objects));
}


void _test_refuseAnEntityOfAnotherType()
{
  EXPECT_THROW(_plan("(has_told kitchen)"), std::runtime_error);
}


void _test_eitherTypeSurvivesSerialization()
{
  auto domain = ogp::pddlToDomain(_domainPddl, false, {});

  auto domainPddl = ogp::domainToPddl(domain);
  auto domainParsedAgain = ogp::pddlToDomain(domainPddl, false, {});

  EXPECT_NE(std::string::npos, domainPddl.find("(has_told ?to - (either social_agent user))"));
  EXPECT_EQ(domainPddl, ogp::domainToPddl(domainParsedAgain));
}

}


TEST(Tool, test_either_types)
{
  _test_eitherTypeIsTheSameWhateverTheOrderOfItsMembers();
  _test_eitherTypeOfOneMemberIsThatMember();
  _test_eitherTypeOnlyOfDeclaredTypes();
  _test_eitherTypeIsA();
  _test_planForAMemberOfAnEitherParameter();
  _test_conditionOverAnEitherParameter();
  _test_refuseAnEntityOfAnotherType();
  _test_eitherTypeSurvivesSerialization();
}
