#include <gtest/gtest.h>
#include <orderedgoalsplanner/types/goalstack.hpp>
#include <orderedgoalsplanner/types/ontology.hpp>
#include <orderedgoalsplanner/types/problem.hpp>
#include <orderedgoalsplanner/types/setofcallbacks.hpp>
#include <orderedgoalsplanner/types/setofconstfacts.hpp>
#include <orderedgoalsplanner/types/setofevents.hpp>
#include <orderedgoalsplanner/types/setofpredicates.hpp>
#include <orderedgoalsplanner/types/worldstate.hpp>
#include <orderedgoalsplanner/util/serializer/deserializefrompddl.hpp>
#include <orderedgoalsplanner/util/serializer/serializeinpddl.hpp>

using namespace ogp;

namespace
{

ogp::Parameter _parameter(const std::string& pStr,
                          const ogp::Ontology& pOntology) {
  return ogp::Parameter::fromStr(pStr, pOntology.types);
}

void _modifyFactsFromPddl(ogp::WorldState& pWorldstate,
                          const std::string& pStr,
                          const ogp::Ontology& pOntology,
                          const SetOfEntities& pObjects)
{
  std::size_t pos = 0;
  GoalStack goalStack;
  const std::map<SetOfEventsId, SetOfEvents> setOfEvents;
  const SetOfCallbacks callbacks;
  pWorldstate.modifyFactsFromPddl(pStr, pos, goalStack, setOfEvents, callbacks,
                                  pOntology, pObjects, {});
}


std::string _effectToDelta(const std::string& pEffect,
                           const ogp::Ontology& pOntology,
                           const SetOfEntities& pObjects,
                           const std::string& pWsPddl,
                           const std::map<SetOfEventsId, SetOfEvents>& pSetOfEvents)
{
  ogp::Problem problem;
  problem.objects = pObjects;
  {
    std::size_t pos = 0;
    problem.worldState.modifyFactsFromPddl(pWsPddl,
                                           pos, problem.goalStack,
                                           pSetOfEvents, {},
                                           pOntology, problem.objects, {});
  }

  std::size_t pos = 0;
  auto effect = ogp::pddlToWsModification(pEffect, pos, pOntology, pObjects, {});
  bool goalChanged = false;
  GoalStack goalStack;
  auto newWorldstate = problem.worldState;
  newWorldstate.applyEffect({}, effect, goalChanged, goalStack, pSetOfEvents, {}, pOntology, pObjects, {});
  return newWorldstate.factsMapping().deltaFrom(problem.worldState.factsMapping()).toPddl();
}


std::string _updateProblem(Problem& pProblem,
                           const ogp::Ontology& pOntology,
                           const SetOfEntities& pObjects,
                           const std::string& pWsPddl,
                           const std::map<SetOfEventsId, SetOfEvents>& pSetOfEvents)
{
  // Calculate the deltas since last update
  WorldState worldState;
  {
    std::size_t pos = 0;
    GoalStack goalStack;
    worldState.modifyFactsFromPddl(pWsPddl, pos, goalStack,
                                   {}, {},
                                   pOntology, pObjects, {});
  }
  auto objDelta = pObjects.deltaFrom(pProblem.objects);
  auto factDelta = worldState.factsMapping().deltaFrom(pProblem.worldState.factsMapping());

  // Check if fact modifications are needed because of some events
  auto newObjects = pProblem.objects;
  newObjects.addAllIfNotExist(pObjects);
  {
    auto worldStateCopied = pProblem.worldState;
    GoalStack goalStack;
    worldStateCopied.applyDelta(factDelta, goalStack, pSetOfEvents, {}, pOntology, newObjects, {});
    auto eventsFactDelta = worldStateCopied.factsMapping().deltaFrom(worldState.factsMapping());
    if (!eventsFactDelta.empty())
      return eventsFactDelta.toPddl();
  }

  // Update the problem
  pProblem.objects = newObjects;
  GoalStack goalStack;
  pProblem.worldState.applyDelta(factDelta, goalStack, pSetOfEvents, {}, pOntology, pProblem.objects, {});
  for (auto& currEntity : objDelta.removedEntities)
    pProblem.objects.remove(currEntity);
  return "";
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

}


TEST(Tool, test_wordstate)
{
  ogp::WorldState worldstate;

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("type1 type2 - entity");
  ontology.constants = ogp::SetOfEntities::fromPddl("ent_a ent_b - entity\n"
                                                    "v1 - type1\n", ontology.types);

  {
    std::size_t pos = 0;
    ontology.predicates = ogp::SetOfPredicates::fromPddl("(pred_a ?e - entity)\n"
                                                         "pred_b\n"
                                                         "(pred_c ?t1 - type1)\n"
                                                         "(pred_d ?t2 - type2) - number\n"
                                                         "(pred_e ?e - entity) - type1", pos, ontology.types);
  }

  auto objects = ogp::SetOfEntities::fromPddl("toto toto2 - type1\n"
                                              "titi - type2", ontology.types);

  _modifyFactsFromPddl(worldstate, "(pred_a toto)\n(pred_b)", ontology, objects);
  EXPECT_EQ("(pred_a toto)\n(pred_b)", worldstate.factsMapping().toPddl(0, true));
  _modifyFactsFromPddl(worldstate, "(= (pred_d titi) 4)\n(pred_a ent_a)", ontology, objects);
  EXPECT_EQ("(pred_a ent_a)\n(pred_a toto)\n(pred_b)\n(= (pred_d titi) 4)", worldstate.factsMapping().toPddl(0, true));
  _modifyFactsFromPddl(worldstate, "(= (pred_d titi) undefined)\n(pred_a titi)\n(not (pred_a toto))", ontology, objects);
  EXPECT_EQ("(pred_a ent_a)\n(pred_a titi)\n(pred_b)", worldstate.factsMapping().toPddl(0, true));
  _modifyFactsFromPddl(worldstate, "(not (pred_a titi))", ontology, objects);
  EXPECT_EQ("(pred_a ent_a)\n(pred_b)", worldstate.factsMapping().toPddl(0, true));
  _modifyFactsFromPddl(worldstate, "(= (pred_e ent_b) toto)\n(pred_b)", ontology, objects);
  EXPECT_EQ("(pred_a ent_a)\n(pred_b)\n(= (pred_e ent_b) toto)", worldstate.factsMapping().toPddl(0, true));
  _modifyFactsFromPddl(worldstate, "(= (pred_e ent_b) undefined)", ontology, objects);
  EXPECT_EQ("(pred_a ent_a)\n(pred_b)", worldstate.factsMapping().toPddl(0, true));
  _modifyFactsFromPddl(worldstate, "(= (pred_e ent_b) undefined)", ontology, objects);
  EXPECT_EQ("(pred_a ent_a)\n(pred_b)", worldstate.factsMapping().toPddl(0, true));
  _modifyFactsFromPddl(worldstate, "(= (pred_e toto) toto)", ontology, objects);
  EXPECT_EQ("(pred_a ent_a)\n(pred_b)\n(= (pred_e toto) toto)", worldstate.factsMapping().toPddl(0, true));
  _modifyFactsFromPddl(worldstate, "(= (pred_e toto2) v1)\n(not (pred_a ent_a))", ontology, objects);
  EXPECT_EQ("(pred_b)\n(= (pred_e toto) toto)\n(= (pred_e toto2) v1)", worldstate.factsMapping().toPddl(0, true));

  // test apply of forall without when
  {
    std::size_t pos = 0;
    auto effect = ogp::pddlToWsModification("(forall (?e - entity) (assign (pred_e ?e) undefined))", pos, ontology, objects, {});
    bool goalChanged = false;
    GoalStack goalStack;
    worldstate.applyEffect({}, effect, goalChanged, goalStack, {}, {}, ontology, objects, {});
  }
  EXPECT_EQ("(pred_b)", worldstate.factsMapping().toPddl(0, true));

  // test set with fluent with undefined value
  objects.addAllIfNotExist(ogp::SetOfEntities::fromPddl("new_val - type1", ontology.types));
  {
    std::size_t pos = 0;
    auto effect = ogp::pddlToWsModification("(assign (pred_e new_val) (pred_a new_val))", pos, ontology, objects, {});
    bool goalChanged = false;
    GoalStack goalStack;
    worldstate.applyEffect({}, effect, goalChanged, goalStack, {}, {}, ontology, objects, {});
  }
  // As (pred_a new_val) as not value (pred_e new_val) is not added
  EXPECT_EQ("(pred_b)", worldstate.factsMapping().toPddl(0, true));
}


TEST(Tool, test_remove_fact_with_an_entity)
{
  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("type1 type2 - entity");
  ontology.constants = ogp::SetOfEntities::fromPddl("ent_a ent_b ent_c - entity", ontology.types);

  {
    std::size_t pos = 0;
    ontology.predicates = ogp::SetOfPredicates::fromPddl("(pred_a ?e - entity)\n"
                                                         "pred_b\n"
                                                         "(pred_c ?t1 - type1)\n"
                                                         "(pred_d ?t2 - type2) - number\n"
                                                         "(pred_e ?e - entity) - type1", pos, ontology.types);
  }

  ogp::SetOfConstFacts constFacts;
  constFacts.add(ogp::Fact::fromPddl("(pred_a ent_c)", ontology, {}, {}));

  auto objects = ogp::SetOfEntities::fromPddl("toto toto2 - type1\n"
                                              "titi - type2", ontology.types);
  ogp::WorldState worldstate(&constFacts.setOfFacts());

  _modifyFactsFromPddl(worldstate, "(pred_a toto)\n(pred_b)\n(pred_c toto)\n(pred_c toto2)\n(pred_a ent_a)\n(pred_a ent_b)\n(= (pred_e ent_b) toto)", ontology, objects);
  EXPECT_EQ("(pred_a ent_a)\n(pred_a ent_b)\n(pred_a ent_c)\n(pred_a toto)\n(pred_b)\n(pred_c toto)\n(pred_c toto2)\n(= (pred_e ent_b) toto)",
            worldstate.factsMapping().toPddl(0, true));
  {
    GoalStack goalStack;
    EXPECT_TRUE(worldstate.removeFactsHoldingEntities({"toto"}, goalStack, {}, {}, ontology, objects, {}));
  }
  EXPECT_EQ("(pred_a ent_a)\n(pred_a ent_b)\n(pred_a ent_c)\n(pred_b)\n(pred_c toto2)", worldstate.factsMapping().toPddl(0, true));
  {
    GoalStack goalStack;
    EXPECT_TRUE(worldstate.removeFactsHoldingEntities({"toto", "toto2", "ent_a"}, goalStack, {}, {}, ontology, objects, {}));
  }
  EXPECT_EQ("(pred_a ent_b)\n(pred_a ent_c)\n(pred_b)", worldstate.factsMapping().toPddl(0, true));
  {
    GoalStack goalStack;
    EXPECT_FALSE(worldstate.removeFactsHoldingEntities({"ent_c"}, goalStack, {}, {}, ontology, objects, {}));
  }
  EXPECT_EQ("(pred_a ent_b)\n(pred_a ent_c)\n(pred_b)", worldstate.factsMapping().toPddl(0, true));
}



TEST(Tool, test_effect_to_delta)
{
  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("type1 type2 - entity");
  ontology.constants = ogp::SetOfEntities::fromPddl("ent_a ent_b ent_c - entity", ontology.types);

  {
    std::size_t pos = 0;
    ontology.predicates = ogp::SetOfPredicates::fromPddl("(pred_a ?e - entity)\n"
                                                         "pred_b\n"
                                                         "(pred_c ?t1 - type1)\n"
                                                         "(pred_d ?t2 - type2) - number\n"
                                                         "(pred_e ?e - entity) - type1\n"
                                                         "(pred_f ?e - entity) - type1", pos, ontology.types);
  }

  ogp::SetOfEvents setOfEvents;
  std::vector<ogp::Parameter> inf1Parameters{_parameter("?e - entity", ontology), _parameter("?t - type1", ontology)};
  ogp::Event event1(_condition_fromPddl("(= (pred_e ?e) ?t)", ontology, inf1Parameters),
                    _worldStateModification_fromPddl("(assign (pred_f ?e) undefined)", ontology, inf1Parameters));
  event1.parameters = std::move(inf1Parameters);
  setOfEvents.add(event1);
  std::map<SetOfEventsId, SetOfEvents> setOfEventsMap;
  setOfEventsMap.emplace("ev", std::move(setOfEvents));

  auto objects = ogp::SetOfEntities::fromPddl("toto toto2 - type1\n"
                                              "titi titi2 titi3 - type2", ontology.types);


  EXPECT_EQ("+(= (pred_e ent_b) toto2)\n"
            "-(pred_a toto)",
            _effectToDelta("(and (assign (pred_e ent_b) toto2) (not (pred_a toto)))",
                           ontology, objects,
                           "(pred_a ent_a)\n(pred_a ent_b)\n(pred_a toto)\n(pred_b)\n(pred_c toto)\n(pred_c toto2)\n(= (pred_e ent_b) toto)",
                           setOfEventsMap));

  EXPECT_EQ("+(= (pred_e ent_a) toto2)\n"
            "-(pred_c toto2)\n"
            "-(= (pred_f ent_a) toto2)",
            _effectToDelta("(and (assign (pred_e ent_a) toto2) (not (pred_c toto)) (not (pred_c toto2)))",
                           ontology, objects,
                           "(= (pred_f ent_a) toto2)\n(pred_c toto2)\n(= (pred_e ent_b) toto)",
                           setOfEventsMap));


  EXPECT_EQ("-(= (pred_d titi3) 12)\n"
            "-(= (pred_d titi) 34)",
            _effectToDelta("(forall (?e - entity) (assign (pred_d ?e) undefined))",
                           ontology, objects,
                           "(= (pred_f ent_a) toto2)\n(= (pred_d titi) 34)\n(= (pred_d titi3) 12)",
                           setOfEventsMap));
}



TEST(Tool, test_update_problem)
{
  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("type1 type2 - entity");
  ontology.constants = ogp::SetOfEntities::fromPddl("ent_a ent_b ent_c - entity", ontology.types);

  {
    std::size_t pos = 0;
    ontology.predicates = ogp::SetOfPredicates::fromPddl("(pred_a ?e - entity)\n"
                                                         "pred_b\n"
                                                         "(pred_c ?t1 - type1)\n"
                                                         "(pred_d ?t2 - type2) - number\n"
                                                         "(pred_e ?e - entity) - type1\n"
                                                         "(pred_f ?e - entity) - type1", pos, ontology.types);
  }

  ogp::SetOfEvents setOfEvents;
  std::vector<ogp::Parameter> inf1Parameters{_parameter("?e - entity", ontology), _parameter("?t - type1", ontology)};
  ogp::Event event1(_condition_fromPddl("(= (pred_e ?e) ?t)", ontology, inf1Parameters),
                    _worldStateModification_fromPddl("(assign (pred_f ?e) undefined)", ontology, inf1Parameters));
  event1.parameters = std::move(inf1Parameters);
  setOfEvents.add(event1);
  std::map<SetOfEventsId, SetOfEvents> setOfEventsMap;
  setOfEventsMap.emplace("ev", std::move(setOfEvents));

  ogp::Problem problem;
  problem.objects = ogp::SetOfEntities::fromPddl("toto toto2 - type1\n"
                                                 "titi titi2 titi3 - type2", ontology.types);


  std::vector<std::string> wss {
    "(pred_a ent_a)\n(pred_a ent_b)\n(pred_a toto)\n(pred_b)\n(pred_c toto)\n(pred_c toto2)\n(= (pred_e ent_b) toto)",
    "(pred_a ent_a)\n(pred_a ent_b)\n(pred_b)\n(pred_c toto)\n(pred_c toto2)\n(= (pred_e ent_b) toto2)",
    "(pred_a ent_a)\n(pred_a ent_b)\n(pred_b)\n(pred_c toto)\n(pred_c toto2)",
    "(pred_a ent_a)\n(pred_a ent_b)\n(pred_b)\n(pred_c toto)\n(pred_c toto2)",
    "(pred_a ent_a)\n(pred_a ent_b)\n(pred_b)\n(pred_c toto)\n(pred_c toto2)\n(= (pred_f ent_a) toto)",
    "(pred_a ent_a)\n(pred_a ent_b)\n(pred_b)\n(pred_c toto)\n(pred_c toto2)\n(= (pred_e ent_c) toto)\n(= (pred_f ent_a) toto)"
  };

  for (const auto& currWs : wss)
  {
    // No event modification so we update the problem right away
    EXPECT_EQ("",
              _updateProblem(problem, ontology,
                             problem.objects,
                             currWs, setOfEventsMap));
    EXPECT_EQ(currWs, problem.worldState.factsMapping().toPddl(0, true));
    EXPECT_EQ("toto toto2 - type1\n"
              "titi titi2 titi3 - type2", problem.objects.toStr());
  }

  // An event modification so we notify about the modification befire to update the problem
  EXPECT_EQ("-(= (pred_f ent_a) toto)",
            _updateProblem(problem, ontology,
                           problem.objects,
                           "(= (pred_e ent_c) toto)\n(= (pred_e ent_a) toto2)\n(= (pred_f ent_a) toto)", setOfEventsMap));
  EXPECT_EQ("(pred_a ent_a)\n(pred_a ent_b)\n(pred_b)\n(pred_c toto)\n(pred_c toto2)\n(= (pred_e ent_c) toto)\n(= (pred_f ent_a) toto)",
            problem.worldState.factsMapping().toPddl(0, true));
  EXPECT_EQ("",
            _updateProblem(problem, ontology,
                           problem.objects,
                           "(= (pred_e ent_c) toto)\n(= (pred_e ent_a) toto2)", setOfEventsMap));
  EXPECT_EQ("(= (pred_e ent_c) toto)\n(= (pred_e ent_a) toto2)",
            problem.worldState.factsMapping().toPddl(0, true));
}
