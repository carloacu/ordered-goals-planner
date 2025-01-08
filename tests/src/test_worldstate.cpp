#include <gtest/gtest.h>
#include <orderedgoalsplanner/types/goalstack.hpp>
#include <orderedgoalsplanner/types/ontology.hpp>
#include <orderedgoalsplanner/types/setofcallbacks.hpp>
#include <orderedgoalsplanner/types/setofconstfacts.hpp>
#include <orderedgoalsplanner/types/setofevents.hpp>
#include <orderedgoalsplanner/types/setofpredicates.hpp>
#include <orderedgoalsplanner/types/worldstate.hpp>

using namespace ogp;

namespace
{

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
}


TEST(Tool, test_wordstate)
{
  ogp::WorldState worldstate;

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("type1 type2 - entity");
  ontology.constants = ogp::SetOfEntities::fromPddl("ent_a ent_b - entity", ontology.types);

  {
    std::size_t pos = 0;
    ontology.predicates = ogp::SetOfPredicates::fromPddl("(pred_a ?e - entity)\n"
                                                         "pred_b\n"
                                                         "(pred_c ?t1 - type1)\n"
                                                         "(pred_d ?t2 - type2) - number\n"
                                                         "(pred_e ?e - entity) - type1", pos, ontology.types);
  }

  auto objects = ogp::SetOfEntities::fromPddl("toto - type1\n"
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
    EXPECT_TRUE(worldstate.removeFactsHoldingAnEntity({"toto"}, goalStack, {}, {}, ontology, objects, {}));
  }
  EXPECT_EQ("(pred_a ent_a)\n(pred_a ent_b)\n(pred_a ent_c)\n(pred_b)\n(pred_c toto2)", worldstate.factsMapping().toPddl(0, true));
  {
    GoalStack goalStack;
    EXPECT_TRUE(worldstate.removeFactsHoldingAnEntity({"toto", "toto2", "ent_a"}, goalStack, {}, {}, ontology, objects, {}));
  }
  EXPECT_EQ("(pred_a ent_b)\n(pred_a ent_c)\n(pred_b)", worldstate.factsMapping().toPddl(0, true));
  {
    GoalStack goalStack;
    EXPECT_FALSE(worldstate.removeFactsHoldingAnEntity({"ent_c"}, goalStack, {}, {}, ontology, objects, {}));
  }
  EXPECT_EQ("(pred_a ent_b)\n(pred_a ent_c)\n(pred_b)", worldstate.factsMapping().toPddl(0, true));
}
