#include <gtest/gtest.h>
#include <orderedgoalsplanner/types/fact.hpp>
#include <orderedgoalsplanner/types/setoffacts.hpp>
#include <orderedgoalsplanner/types/ontology.hpp>
#include <orderedgoalsplanner/util/alias.hpp>

using namespace ogp;


TEST(Tool, test_setOfFacts)
{
  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity\n"
                                             "my_type my_type2 - entity\n"
                                             "sub_my_type2 - my_type2");
  ontology.constants = ogp::SetOfEntities::fromPddl("toto toto2 - my_type\n"
                                                    "titi titi_const - my_type2", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("pred_name(?e - entity)\n"
                                                      "pred_name2(?e - entity)\n"
                                                      "pred_name3(?p1 - my_type, ?p2 - my_type2)\n"
                                                      "pred_name4(?p1 - my_type, ?p2 - my_type2)\n"
                                                      "pred_name5(?p1 - my_type) - my_type2",
                                                      ontology.types);

  auto entities = ogp::SetOfEntities::fromPddl("toto3 - my_type\n"
                                               "titi2 - my_type2\n"
                                               "titi3 - sub_my_type2", ontology.types);

  SetOfFacts factToFacts;

  auto fact1 = ogp::Fact::fromStr("pred_name(toto)", ontology, entities, {});
  factToFacts.add(fact1);

  EXPECT_EQ("[pred_name(toto)]", factToFacts.find(fact1).toStr());
  EXPECT_TRUE(factToFacts.erase(fact1));
  EXPECT_EQ("[]", factToFacts.find(fact1).toStr());


  // tests with pred_name5

  auto fact2 = ogp::Fact::fromStr("pred_name5(toto2)=titi", ontology, entities, {});
  EXPECT_TRUE(factToFacts.add(fact2, false));
  EXPECT_FALSE(factToFacts.erase(fact2));
  EXPECT_EQ("[]", factToFacts.find(fact1).toStr());

  auto fact3 = ogp::Fact::fromStr("pred_name5(toto)=titi", ontology, entities, {});
  EXPECT_TRUE(factToFacts.add(fact3));
  EXPECT_FALSE(factToFacts.add(fact3));

  EXPECT_FALSE(factToFacts.add(ogp::Fact::fromStr("pred_name5(toto2)=titi", ontology, entities, {})));
  EXPECT_TRUE(factToFacts.add(ogp::Fact::fromStr("pred_name5(toto)=titi_const", ontology, entities, {})));

  {
    auto factWithParam = ogp::Fact::fromStr("pred_name5(toto)=titi", ontology, entities, {});
    EXPECT_EQ("[pred_name5(toto)=titi]", factToFacts.find(factWithParam).toStr());
    EXPECT_EQ("[pred_name5(toto)=titi, pred_name5(toto)=titi_const]", factToFacts.find(factWithParam, true).toStr());
  }

  {
    std::vector<ogp::Parameter> parameters(1, ogp::Parameter::fromStr("?p1 - my_type", ontology.types));
    auto factWithParam = ogp::Fact::fromStr("pred_name5(?p1)=titi_const", ontology, entities, parameters);
    EXPECT_EQ("[pred_name5(toto)=titi_const]", factToFacts.find(factWithParam).toStr());
    EXPECT_EQ("[pred_name5(toto2)=titi, pred_name5(toto)=titi, pred_name5(toto)=titi_const]", factToFacts.find(factWithParam, true).toStr());
  }

  auto factCopied = fact3;
  factToFacts.erase(factCopied);

  {
    auto factWithParam = ogp::Fact::fromStr("pred_name5(toto)=titi", ontology, entities, {});
    EXPECT_EQ("[]", factToFacts.find(factWithParam).toStr());
    EXPECT_EQ("[pred_name5(toto)=titi_const]", factToFacts.find(factWithParam, true).toStr());
  }

  auto fact4WithAnyValue = ogp::Fact::fromStr("pred_name5(toto)=*", ontology, entities, {});
  factToFacts.erase(fact4WithAnyValue);

  {
    auto factWithParam = ogp::Fact::fromStr("pred_name5(toto)=titi", ontology, entities, {});
    EXPECT_EQ("[]", factToFacts.find(factWithParam).toStr());
    EXPECT_EQ("[]", factToFacts.find(factWithParam, true).toStr());
  }
}
