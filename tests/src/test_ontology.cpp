#include <gtest/gtest.h>
#include <orderedgoalsplanner/types/action.hpp>
#include <orderedgoalsplanner/types/goalstack.hpp>
#include <orderedgoalsplanner/types/ontology.hpp>
#include <orderedgoalsplanner/types/setofcallbacks.hpp>
#include <orderedgoalsplanner/types/setofevents.hpp>
#include <orderedgoalsplanner/types/worldstate.hpp>
#include <orderedgoalsplanner/util/serializer/deserializefrompddl.hpp>

namespace
{
const ogp::SetOfCallbacks _emptyCallbacks;


void _test_setOfTypes()
{
  ogp::SetOfTypes setOfTypes;
  EXPECT_EQ("", setOfTypes.toStr());
  setOfTypes.addType("object");
  setOfTypes.addType("voiture", "object");
  setOfTypes.addType("maison", "object");
  setOfTypes.addType("citroen", "voiture");
  setOfTypes.addType("ferrari", "voiture");
  setOfTypes.addType("peugeot", "voiture");
  setOfTypes.addType("c3", "citroen");
  setOfTypes.addType("location");

  EXPECT_EQ("voiture maison - object\n"
                         "citroen ferrari peugeot - voiture\n"
                         "c3 - citroen\n"
                         "location",
                         setOfTypes.toStr());
}


void _test_setOfTypes_fromStr()
{
  std::string typesStr = "voiture maison - object\n"
                         "citroen ferrari peugeot - voiture\n"
                         "c3 - citroen\n"
                         "location";
  auto setOfTypes = ogp::SetOfTypes::fromPddl(typesStr + " ");
  EXPECT_EQ(typesStr, setOfTypes.toStr());
  EXPECT_EQ("voiture", setOfTypes.nameToType("citroen")->parent->name);
}


void _test_predicateToStr()
{
  auto setOfTypes = ogp::SetOfTypes::fromPddl("my_type my_type2 return_type");
  EXPECT_EQ("pred_name(?v - my_type)", ogp::Predicate("pred_name(?v - my_type)", false, setOfTypes).toStr());
  EXPECT_EQ("pred_name(?v - my_type, ?o - my_type2)", ogp::Predicate("pred_name(?v - my_type, ?o - my_type2)", false, setOfTypes).toStr());
  EXPECT_EQ("pred_name(?v - my_type, ?o - my_type2) - return_type", ogp::Predicate("pred_name(?v - my_type, ?o - my_type2) - return_type", false, setOfTypes).toStr());

  auto otherSetOfTypes = ogp::SetOfTypes::fromPddl("a b");
  try {
    ogp::Predicate("pred_name(?v - my_type)", false, otherSetOfTypes);
    EXPECT_TRUE(false);
  } catch (const std::exception& e) {
    EXPECT_EQ("\"my_type\" is not a valid type name", std::string(e.what()));
  }
  try {
    ogp::Predicate("pred_name(?v - a, ?o - b) - return_type", false, otherSetOfTypes);
    EXPECT_TRUE(false);
  } catch (const std::exception& e) {
    EXPECT_EQ("\"return_type\" is not a valid type name", std::string(e.what()));
  }
}


void _test_setOfPredicates_fromStr()
{
  auto setOfTypes = ogp::SetOfTypes::fromPddl("my_type my_type2 return_type");
  std::string predicatesStr = "pred_name(?v - my_type)\n"
                              "pred_name2(?v - my_type, ?o - my_type2)";
  auto setOfPredicates = ogp::SetOfPredicates::fromStr(predicatesStr, setOfTypes);
  EXPECT_EQ(predicatesStr, setOfPredicates.toStr());
}


void _test_setOfEntities_fromStr()
{
  auto setOfTypes = ogp::SetOfTypes::fromPddl("my_type my_type2 return_type");
  std::string entitiesStr = "toto - my_type\n"
                            "titi tutu - my_type2";
  auto setOfEntities = ogp::SetOfEntities::fromPddl(entitiesStr, setOfTypes);
  EXPECT_EQ(entitiesStr, setOfEntities.toStr());
  EXPECT_EQ("my_type2", setOfEntities.valueToEntity("titi")->type->name);
}

void _test_fact_initialization()
{
  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("my_type my_type2 return_type\n"
                                           "sub_my_type - my_type");
  ontology.constants = ogp::SetOfEntities::fromPddl("toto - my_type\n"
                                                  "sub_toto - sub_my_type\n"
                                                  "titi tutu - my_type2\n"
                                                  "res - return_type", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("pred_name(?v - my_type)\n"
                                                     "pred_name2(?v - my_type, ?o - my_type2) - return_type", ontology.types);

  ogp::Fact("pred_name(toto)", false, ontology, {}, {});
  ogp::Fact("pred_name(sub_toto)", false, ontology, {}, {});

  try
  {
    ogp::Fact("pred_that_does_not_exist(titi)", false, ontology, {}, {});
    EXPECT_TRUE(false);
  }
  catch(const std::exception& e) {
    EXPECT_EQ("\"pred_that_does_not_exist\" is not a predicate name. The exception was thrown while parsing fact: \"pred_that_does_not_exist(titi)\"", std::string(e.what()));
  }
  try
  {
    ogp::Fact("pred_name(unknown_value)", false, ontology, {}, {});
    EXPECT_TRUE(false);
  }
  catch(const std::exception& e) {
    EXPECT_EQ("\"unknown_value\" is not an entity value. The exception was thrown while parsing fact: \"pred_name(unknown_value)\"", std::string(e.what()));
  }
  try
  {
    ogp::Fact("pred_name", false, ontology, {}, {});
    EXPECT_TRUE(false);
  }
  catch(const std::exception& e) {
    EXPECT_EQ("The fact \"(pred_name)\" does not have the same number of parameters than the associated predicate \"(pred_name ?v - my_type)\". The exception was thrown while parsing fact: \"pred_name\"", std::string(e.what()));
  }
  try
  {
    ogp::Fact("pred_name(titi)", false, ontology, {}, {});
    EXPECT_TRUE(false);
  }
  catch(const std::exception& e) {
    EXPECT_EQ("\"titi - my_type2\" is not a \"my_type\" for predicate: \"(pred_name ?v - my_type)\". The exception was thrown while parsing fact: \"pred_name(titi)\"", std::string(e.what()));
  }
  try
  {
    ogp::Fact("pred_name(toto)=val", false, ontology, {}, {});
    EXPECT_TRUE(false);
  }
  catch(const std::exception& e) {
    EXPECT_EQ("\"val\" is not an entity value. The exception was thrown while parsing fact: \"pred_name(toto)=val\"", std::string(e.what()));
  }

  ogp::Fact("pred_name2(toto, titi)=res", false, ontology, {}, {});
  {
    std::vector<ogp::Parameter> parameters(1, ogp::Parameter::fromStr("?v - my_type2", ontology.types));
    ogp::Fact("pred_name2(toto, ?v)=res", false, ontology, {}, parameters);
  }
  {
    std::vector<ogp::Parameter> parameters(1, ogp::Parameter::fromStr("?v - my_type2", ontology.types));
    parameters.push_back( ogp::Parameter::fromStr("?r - return_type", ontology.types));
    ogp::Fact("pred_name2(toto, ?v)=?r", false, ontology, {}, parameters);
  }

  try
  {
    ogp::Fact("pred_name2(toto, titi)=unknown_val", false, ontology, {}, {});
    EXPECT_TRUE(false);
  }
  catch(const std::exception& e) {
    EXPECT_EQ("\"unknown_val\" is not an entity value. The exception was thrown while parsing fact: \"pred_name2(toto, titi)=unknown_val\"", std::string(e.what()));
  }
}


void _test_action_initialization()
{
  ogp::SetOfFacts setOfFacts;
  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("my_type my_type2 return_type\n"
                                           "sub_my_type - my_type");
  ontology.constants = ogp::SetOfEntities::fromPddl("toto - my_type\n"
                                                  "sub_toto - sub_my_type\n"
                                                  "titi tutu - my_type2\n"
                                                  "res - return_type", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("pred_name(?v - my_type)\n"
                                                     "pred_name2(?v - my_type, ?o - my_type2) - return_type\n"
                                                     "pred_name3(?o - my_type2) - return_type", ontology.types);

  ogp::SetOfEntities entities;

  ogp::Action action(ogp::strToCondition("pred_name(toto)", ontology, entities, {}),
                    ogp::strToWsModification("pred_name2(toto, titi)=res", ontology, entities, {}));
  action.throwIfNotValid(setOfFacts);

  {
    std::vector<ogp::Parameter> parameters(1, ogp::Parameter::fromStr("?p - my_type", ontology.types));
    ogp::Action action2(ogp::strToCondition("pred_name(?p)", ontology, entities, parameters),
                      ogp::strToWsModification("pred_name2(toto, titi)=res", ontology, entities, parameters));
    action2.parameters = std::move(parameters);
    action2.throwIfNotValid(setOfFacts);
  }

  {
    std::vector<ogp::Parameter> parameters(1, ogp::Parameter::fromStr("?p - sub_my_type", ontology.types));
    ogp::Action action3(ogp::strToCondition("pred_name(?p)", ontology, entities, parameters),
                      ogp::strToWsModification("pred_name2(toto, titi)=res", ontology, entities, parameters));
    action3.parameters = std::move(parameters);
    action3.throwIfNotValid(setOfFacts);
  }

  try
  {
    ogp::Action action4(ogp::strToCondition("pred_name(?p)", ontology, entities, {}),
                       ogp::strToWsModification("pred_name2(toto, titi)=res", ontology, entities, {}));
    action4.throwIfNotValid(setOfFacts);
    EXPECT_TRUE(false);
  }
  catch (const std::exception& e)
  {
    EXPECT_EQ("The parameter \"?p\" is unknown", std::string(e.what()));
  }

  {
    std::vector<ogp::Parameter> parameters(1, ogp::Parameter::fromStr("?p - my_type", ontology.types));
    try
    {
      ogp::Action action5(ogp::strToCondition("pred_name(?p)", ontology, entities, parameters),
                         ogp::strToWsModification("pred_name2(toto, titi)=?r", ontology, entities, parameters));
      action5.parameters = std::move(parameters);
      action5.throwIfNotValid(setOfFacts);
      EXPECT_TRUE(false);
    }
    catch (const std::exception& e)
    {
      EXPECT_EQ("The parameter \"?r\" is unknown", std::string(e.what()));
    }
  }

  ogp::strToCondition("exists(?obj - my_type2, pred_name2(toto, ?obj)=res)", ontology, entities, {});
  ogp::strToCondition("exists(?obj - my_type2, =(pred_name3(?obj), pred_name3(tutu)))", ontology, entities, {});
  ogp::strToCondition("=(pred_name3(tutu), undefined)", ontology, entities, {});
  ogp::strToCondition("=(pred_name3(tutu), res)", ontology, entities, {});
  ogp::strToWsModification("forall(?obj - my_type2, pred_name2(toto, ?obj)=res, set(pred_name3(?obj), pred_name3(tutu)))", ontology, entities, {});
  ogp::strToWsModification("assign(pred_name3(tutu), res)", ontology, entities, {});
  std::vector<ogp::Parameter> returnParameter(1, ogp::Parameter::fromStr("?r - return_type", ontology.types));
  ogp::strToWsModification("assign(pred_name3(tutu), ?r)", ontology, entities, returnParameter);
}


}



TEST(Tool, test_ontology)
{
  _test_setOfTypes();
  _test_setOfTypes_fromStr();
  _test_predicateToStr();
  _test_setOfPredicates_fromStr();
  _test_setOfEntities_fromStr();
  _test_fact_initialization();
  _test_action_initialization();
}
