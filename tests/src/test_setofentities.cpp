#include <gtest/gtest.h>
#include <orderedgoalsplanner/orderedgoalsplanner.hpp>
#include <orderedgoalsplanner/types/parallelplan.hpp>
#include <orderedgoalsplanner/types/setofcallbacks.hpp>
#include <orderedgoalsplanner/util/serializer/deserializefrompddl.hpp>

namespace
{
const std::unique_ptr<std::chrono::steady_clock::time_point> _now = {};
const std::map<ogp::SetOfEventsId, ogp::SetOfEvents> _emptySetOfEvents;
const ogp::SetOfCallbacks _emptyCallbacks;


ogp::Fact _fact(const std::string& pStr,
                const ogp::Ontology& pOntology,
                const ogp::SetOfEntities& pObjects,
                const std::vector<ogp::Parameter>& pParameters = {}) {
  return ogp::Fact(pStr, true, pOntology, pObjects, pParameters);
}


ogp::Goal _pddlGoal(const std::string& pStr,
                    const ogp::Ontology& pOntology,
                    const ogp::SetOfEntities& pObjects,
                    int pMaxTimeToKeepInactive = -1,
                    const std::string& pGoalGroupId = "") {
  std::size_t pos = 0;
  return *ogp::pddlToGoal(pStr, pos, pOntology, pObjects, pMaxTimeToKeepInactive, pGoalGroupId);
}


void _testRemoveUnusedEntitiesOfTypes()
{
  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("t1 t2 t3 t4 - entity");
  ontology.predicates = ogp::SetOfPredicates::fromStr("fact_a(p - t1)\n"
                                                      "fact_b(p - t2)\n"
                                                      "fact_c(p - t3)\n"
                                                      "fact_d(p - t4)", ontology.types);
  auto t1Type = ontology.types.nameToType("t1");
  auto t2Type = ontology.types.nameToType("t2");
  auto t3Type = ontology.types.nameToType("t3");
  auto t4Type = ontology.types.nameToType("t4");

  ogp::WorldState worldState;
  ogp::SetOfEntities objects;
  std::vector<ogp::Goal> goals;

  objects = ogp::SetOfEntities::fromPddl("v1a v1b v1c - t1\n"
                                         "v2a v2b - t2\n"
                                         "v3a v3b - t3\n"
                                         "v4a v4b - t4\n", ontology.types);

  {
    ogp::GoalStack goalStack;
    worldState.addFact(_fact("(fact_a v1a)", ontology, objects), goalStack, _emptySetOfEvents, _emptyCallbacks, ontology, objects, _now);
    worldState.addFact(_fact("(fact_b v2b)", ontology, objects), goalStack, _emptySetOfEvents, _emptyCallbacks, ontology, objects, _now);
    worldState.addFact(_fact("(fact_c v3a)", ontology, objects), goalStack, _emptySetOfEvents, _emptyCallbacks, ontology, objects, _now);
    worldState.addFact(_fact("(fact_c v3b)", ontology, objects), goalStack, _emptySetOfEvents, _emptyCallbacks, ontology, objects, _now);
  }
  goals.emplace_back(_pddlGoal("(fact_a v1c)", ontology, objects));

  EXPECT_EQ("v1a v1b v1c - t1\n"
            "v2a v2b - t2\n"
            "v3a v3b - t3\n"
            "v4a v4b - t4", objects.toStr());
  objects.removeUnusedEntitiesOfTypes(worldState, goals, {t1Type, t2Type});
  EXPECT_EQ("v1a v1c - t1\n"
            "v2b - t2\n"
            "v3a v3b - t3\n"
            "v4a v4b - t4", objects.toStr());
  objects.removeUnusedEntitiesOfTypes(worldState, goals, {t3Type, t4Type});
  EXPECT_EQ("v1a v1c - t1\n"
            "v2b - t2\n"
            "v3a v3b - t3", objects.toStr());
}

}


TEST(Planner, test_setofentities)
{
  _testRemoveUnusedEntitiesOfTypes();
}
