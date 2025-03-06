#include <gtest/gtest.h>
#include <orderedgoalsplanner/types/action.hpp>
#include <orderedgoalsplanner/types/fact.hpp>
#include <orderedgoalsplanner/types/goalstack.hpp>
#include <orderedgoalsplanner/types/ontology.hpp>
#include <orderedgoalsplanner/types/setofcallbacks.hpp>
#include <orderedgoalsplanner/types/setofevents.hpp>
#include <orderedgoalsplanner/types/worldstate.hpp>
#include <orderedgoalsplanner/util/serializer/deserializefrompddl.hpp>

namespace
{
const ogp::SetOfCallbacks _emptyCallbacks;

ogp::ParameterValuesWithConstraints _toParameterMap(const std::vector<ogp::Parameter>& pParameters)
{
  ogp::ParameterValuesWithConstraints res;
  for (auto& currParam : pParameters)
    res[currParam];
  return res;
}

bool _isTrue(const std::string& pConditionPddlStr,
             const ogp::Ontology& pOntology,
             const ogp::WorldState& pWorldState,
             const ogp::SetOfEntities& pObjects)
{
  std::size_t pos = 0;
  auto condtionPtr = ogp::pddlToCondition(pConditionPddlStr, pos, pOntology, pObjects, {});
  return condtionPtr->isTrue(pWorldState, pOntology.constants, pObjects);
}



void _test_checkConditionWithOntology()
{
  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("my_type my_type2 my_type3 - entity\n"
                                             "sub_my_type3 - my_type3");
  ontology.constants = ogp::SetOfEntities::fromPddl("toto toto2 - my_type\n"
                                                    "titi - my_type2\n", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("pred_name(?e - entity)\n"
                                                      "fun1(?e - my_type3) - entity\n"
                                                      "fun2(?e - my_type2) - entity\n"
                                                      "nb - number", ontology.types);

  ogp::WorldState worldState;
  ogp::GoalStack goalStack;
  ogp::SetOfEntities objects;
  std::map<ogp::SetOfEventsId, ogp::SetOfEvents> setOfEvents;
  worldState.addFact(ogp::Fact::fromStr("pred_name(toto)", ontology, objects, {}), goalStack, setOfEvents, _emptyCallbacks, ontology, objects, {});

  EXPECT_FALSE(_isTrue("(pred_name titi)", ontology, worldState, objects));
  EXPECT_TRUE(_isTrue("(pred_name toto)", ontology, worldState, objects));
  EXPECT_FALSE(_isTrue("(not (pred_name toto))", ontology, worldState, objects));
  EXPECT_TRUE(_isTrue("(not (pred_name titi))", ontology, worldState, objects));
  EXPECT_TRUE(_isTrue("(and (pred_name toto) (pred_name toto))", ontology, worldState, objects));
  EXPECT_FALSE(_isTrue("(not (and (pred_name toto) (pred_name toto)))", ontology, worldState, objects));
  EXPECT_FALSE(_isTrue("(and (pred_name toto) (pred_name titi))", ontology, worldState, objects));
  EXPECT_TRUE(_isTrue("(not (and (pred_name toto) (pred_name titi)))", ontology, worldState, objects));

  EXPECT_TRUE(_isTrue("(imply (pred_name toto) (pred_name toto))", ontology, worldState, objects));
  EXPECT_FALSE(_isTrue("(imply (pred_name toto) (pred_name titi))", ontology, worldState, objects));
  EXPECT_TRUE(_isTrue("(imply (pred_name titi) (pred_name toto))", ontology, worldState, objects));
  EXPECT_TRUE(_isTrue("(imply (pred_name titi) (pred_name titi))", ontology, worldState, objects));

  EXPECT_FALSE(_isTrue("(not (imply (pred_name toto) (pred_name toto)))", ontology, worldState, objects));
  EXPECT_TRUE(_isTrue("(not (imply (pred_name toto) (pred_name titi)))", ontology, worldState, objects));
  EXPECT_FALSE(_isTrue("(not (imply (pred_name titi) (pred_name toto)))", ontology, worldState, objects));
  EXPECT_FALSE(_isTrue("(not (imply (pred_name titi) (pred_name titi)))", ontology, worldState, objects));

  worldState.addFact(ogp::Fact::fromStr("nb=10", ontology, objects, {}), goalStack, setOfEvents, _emptyCallbacks, ontology, objects, {});
  EXPECT_TRUE(_isTrue("(< nb 11)", ontology, worldState, objects));
  EXPECT_FALSE(_isTrue("(not (< nb 11))", ontology, worldState, objects));


  {
    std::vector<ogp::Parameter> parameters(1, ogp::Parameter::fromStr("?p - my_type", ontology.types));
    auto parametersMap = _toParameterMap(parameters);
    EXPECT_TRUE(ogp::strToCondition("pred_name(?p)", ontology, {}, parameters)->isTrue(worldState, ontology.constants, objects, {}, {}, {}, &parametersMap));
  }

  {
    std::vector<ogp::Parameter> parameters(1, ogp::Parameter::fromStr("?p - my_type2", ontology.types));
    auto parametersMap = _toParameterMap(parameters);
    EXPECT_FALSE(ogp::strToCondition("pred_name(?p)", ontology, {}, parameters)->isTrue(worldState, ontology.constants, objects, {}, {}, {}, &parametersMap));
  }

  EXPECT_FALSE(_isTrue("(exists (?p - my_type3) (= (fun1 ?p) undefined))", ontology, worldState, objects));
  EXPECT_TRUE(_isTrue("(exists (?p - my_type2) (= (fun2 ?p) undefined))", ontology, worldState, objects));
  EXPECT_TRUE(_isTrue("(forall (?p - my_type2) (= (fun2 ?p) undefined))", ontology, worldState, objects));

  objects.add(ogp::Entity::fromDeclaration("sub3a - sub_my_type3", ontology.types));

  EXPECT_TRUE(_isTrue("(exists (?p - my_type3) (= (fun1 ?p) undefined))", ontology, worldState, objects));
  EXPECT_TRUE(_isTrue("(forall (?p - my_type3) (= (fun1 ?p) undefined))", ontology, worldState, objects));

  objects.add(ogp::Entity::fromDeclaration("sub3b - sub_my_type3", ontology.types));

  EXPECT_TRUE(_isTrue("(forall (?p - my_type3) (= (fun1 ?p) undefined))", ontology, worldState, objects));
  EXPECT_TRUE(_isTrue("(exists (?p - my_type3) (= (fun1 ?p) undefined))", ontology, worldState, objects));

  worldState.addFact(ogp::Fact::fromStr("fun1(sub3a)=toto", ontology, objects, {}), goalStack, setOfEvents, _emptyCallbacks, ontology, objects, {});

  EXPECT_TRUE(_isTrue("(exists (?p - my_type3) (= (fun1 ?p) undefined))", ontology, worldState, objects));
  EXPECT_FALSE(_isTrue("(forall (?p - my_type3) (= (fun1 ?p) undefined))", ontology, worldState, objects));
  EXPECT_TRUE(_isTrue("(exists (?p - my_type3) (= (fun1 ?p) toto))", ontology, worldState, objects));
  EXPECT_FALSE(_isTrue("(forall (?p - my_type3) (= (fun1 ?p) toto))", ontology, worldState, objects));

  worldState.addFact(ogp::Fact::fromStr("fun1(sub3b)=toto", ontology, objects, {}), goalStack, setOfEvents, _emptyCallbacks, ontology, objects, {});

  EXPECT_FALSE(_isTrue("(exists (?p - my_type3) (= (fun1 ?p) undefined))", ontology, worldState, objects));
  EXPECT_FALSE(_isTrue("(forall (?p - my_type3) (= (fun1 ?p) undefined))", ontology, worldState, objects));
  EXPECT_TRUE(_isTrue("(exists (?p - my_type3) (= (fun1 ?p) toto))", ontology, worldState, objects));
  EXPECT_TRUE(_isTrue("(forall (?p - my_type3) (= (fun1 ?p) toto))", ontology, worldState, objects));

  worldState.addFact(ogp::Fact::fromStr("fun1(sub3b)=toto2", ontology, objects, {}), goalStack, setOfEvents, _emptyCallbacks, ontology, objects, {});
  EXPECT_TRUE(_isTrue("(exists (?p - my_type3) (= (fun1 ?p) toto))", ontology, worldState, objects));
  EXPECT_FALSE(_isTrue("(forall (?p - my_type3) (= (fun1 ?p) toto))", ontology, worldState, objects));
}

void _test_fluent_value_equality_with_sub_types()
{
  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("location\n"
                                             "robot checkpoint - physical_object\n"
                                             "charging_zone - checkpoint");
  ontology.constants = ogp::SetOfEntities::fromPddl("self - robot", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("declared_location(?po - physical_object) - location", ontology.types);

  ogp::WorldState worldState;
  ogp::GoalStack goalStack;
  ogp::SetOfEntities objects;
  std::map<ogp::SetOfEventsId, ogp::SetOfEvents> setOfEvents;
  objects.add(ogp::Entity::fromDeclaration("charging_zone_loc - location", ontology.types));
  objects.add(ogp::Entity::fromDeclaration("a_checkpoint_loc - location", ontology.types));
  objects.add(ogp::Entity::fromDeclaration("a_checkpoint - checkpoint", ontology.types));
  objects.add(ogp::Entity::fromDeclaration("pod - charging_zone", ontology.types));

  worldState.addFact(ogp::Fact::fromPddl("(= (declared_location a_checkpoint) a_checkpoint_loc)", ontology, objects, {}), goalStack, setOfEvents, _emptyCallbacks, ontology, objects, {});
  worldState.addFact(ogp::Fact::fromPddl("(= (declared_location pod) charging_zone_loc)", ontology, objects, {}), goalStack, setOfEvents, _emptyCallbacks, ontology, objects, {});
  worldState.addFact(ogp::Fact::fromPddl("(= (declared_location self) a_checkpoint_loc)", ontology, objects, {}), goalStack, setOfEvents, _emptyCallbacks, ontology, objects, {});

  EXPECT_FALSE(_isTrue("(exists (?cz - charging_zone) (= (declared_location self) (declared_location ?cz))", ontology, worldState, objects));
}


void _test_exists_with_and_list_inside()
{
  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("my_type my_type2 my_type3 - entity\n"
                                             "sub_my_type3 - my_type3");
  ontology.constants = ogp::SetOfEntities::fromPddl("toto toto2 - my_type\n"
                                                    "titi - my_type2\n"
                                                    "v3 - my_type3", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("pred_name(?e - entity)\n"
                                                      "fun1(?e - my_type3) - entity\n"
                                                      "fun2(?e - my_type2) - entity\n"
                                                      "fun3(?e - my_type3) - entity\n"
                                                      "nb - number", ontology.types);

  ogp::WorldState worldState;
  ogp::GoalStack goalStack;
  ogp::SetOfEntities objects;
  std::map<ogp::SetOfEventsId, ogp::SetOfEvents> setOfEvents;


  EXPECT_FALSE(_isTrue("(exists (?p - entity) (= (fun1 ?p) toto))", ontology, worldState, objects));
  worldState.addFact(ogp::Fact::fromStr("fun1(v3)=toto", ontology, objects, {}), goalStack, setOfEvents, _emptyCallbacks, ontology, objects, {});
  EXPECT_TRUE(_isTrue("(exists (?p - entity) (= (fun1 ?p) toto))", ontology, worldState, objects));
  EXPECT_FALSE(_isTrue("(exists (?p - entity) (and (= (fun1 ?p) toto) (= (fun2 ?p) toto2)))", ontology, worldState, objects));
  worldState.addFact(ogp::Fact::fromStr("fun2(titi)=toto", ontology, objects, {}), goalStack, setOfEvents, _emptyCallbacks, ontology, objects, {});
  EXPECT_FALSE(_isTrue("(exists (?p - entity) (and (= (fun1 ?p) toto) (= (fun2 ?p) toto2)))", ontology, worldState, objects));
  worldState.addFact(ogp::Fact::fromStr("fun2(titi)=toto2", ontology, objects, {}), goalStack, setOfEvents, _emptyCallbacks, ontology, objects, {});
  EXPECT_FALSE(_isTrue("(exists (?p - entity) (and (= (fun1 ?p) toto) (= (fun2 ?p) toto2)))", ontology, worldState, objects));
  worldState.addFact(ogp::Fact::fromStr("fun3(v3)=toto2", ontology, objects, {}), goalStack, setOfEvents, _emptyCallbacks, ontology, objects, {});
  EXPECT_TRUE(_isTrue("(exists (?p - entity) (and (= (fun1 ?p) toto) (= (fun3 ?p) toto2)))", ontology, worldState, objects));
  EXPECT_TRUE(_isTrue("(exists (?p - entity) (and (= (fun1 ?p) toto) (not (pred_name ?p))))", ontology, worldState, objects));
  worldState.addFact(ogp::Fact::fromStr("pred_name(v3)", ontology, objects, {}), goalStack, setOfEvents, _emptyCallbacks, ontology, objects, {});
  EXPECT_FALSE(_isTrue("(exists (?p - entity) (and (= (fun1 ?p) toto) (not (pred_name ?p))))", ontology, worldState, objects));
  EXPECT_TRUE(_isTrue("(exists (?p - entity) (and (= (fun1 ?p) toto) (pred_name ?p)))", ontology, worldState, objects));
}


void _test_exists_with_imply_inside()
{
  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("my_type my_type2 my_type3 - entity\n"
                                             "sub_my_type3 - my_type3");
  ontology.constants = ogp::SetOfEntities::fromPddl("toto - my_type\n"
                                                    "titi - my_type2\n"
                                                    "v3 - my_type3", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("pred_name(?e - entity)\n"
                                                      "fun1(?e - my_type3) - entity", ontology.types);

  ogp::WorldState worldState;
  ogp::GoalStack goalStack;
  ogp::SetOfEntities objects;
  std::map<ogp::SetOfEventsId, ogp::SetOfEvents> setOfEvents;

  EXPECT_TRUE(_isTrue("(exists (?p - my_type3) (imply (pred_name ?p) (= (fun1 ?p) toto)))", ontology, worldState, objects));
  worldState.addFact(ogp::Fact::fromStr("pred_name(v3)", ontology, objects, {}), goalStack, setOfEvents, _emptyCallbacks, ontology, objects, {});
  EXPECT_FALSE(_isTrue("(exists (?p - my_type3) (imply (pred_name ?p) (= (fun1 ?p) toto)))", ontology, worldState, objects));
  worldState.addFact(ogp::Fact::fromStr("fun1(v3)=titi", ontology, objects, {}), goalStack, setOfEvents, _emptyCallbacks, ontology, objects, {});
  EXPECT_FALSE(_isTrue("(exists (?p - my_type3) (imply (pred_name ?p) (= (fun1 ?p) toto)))", ontology, worldState, objects));
  worldState.addFact(ogp::Fact::fromStr("fun1(v3)=toto", ontology, objects, {}), goalStack, setOfEvents, _emptyCallbacks, ontology, objects, {});
  EXPECT_TRUE(_isTrue("(exists (?p - my_type3) (imply (pred_name ?p) (= (fun1 ?p) toto)))", ontology, worldState, objects));
  objects.add(ogp::Entity::fromDeclaration("v3b - my_type3", ontology.types));
  EXPECT_TRUE(_isTrue("(exists (?p - my_type3) (imply (pred_name ?p) (= (fun1 ?p) toto)))", ontology, worldState, objects));
  worldState.addFact(ogp::Fact::fromStr("fun1(v3)=titi", ontology, objects, {}), goalStack, setOfEvents, _emptyCallbacks, ontology, objects, {});
  EXPECT_TRUE(_isTrue("(exists (?p - my_type3) (imply (pred_name ?p) (= (fun1 ?p) toto)))", ontology, worldState, objects));
  worldState.addFact(ogp::Fact::fromStr("pred_name(v3b)", ontology, objects, {}), goalStack, setOfEvents, _emptyCallbacks, ontology, objects, {});
  EXPECT_FALSE(_isTrue("(exists (?p - my_type3) (imply (pred_name ?p) (= (fun1 ?p) toto)))", ontology, worldState, objects));
}


void _test_forall_with_imply_inside()
{
  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("my_type my_type2 my_type3 - entity\n"
                                             "sub_my_type3 - my_type3");
  ontology.constants = ogp::SetOfEntities::fromPddl("toto - my_type\n"
                                                    "titi - my_type2\n"
                                                    "v3 - my_type3", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("pred_name(?e - entity)\n"
                                                      "fun1(?e - my_type3) - entity", ontology.types);

  ogp::WorldState worldState;
  ogp::GoalStack goalStack;
  ogp::SetOfEntities objects;
  std::map<ogp::SetOfEventsId, ogp::SetOfEvents> setOfEvents;

  EXPECT_TRUE(_isTrue("(forall (?p - my_type3) (imply (pred_name ?p) (= (fun1 ?p) toto)))", ontology, worldState, objects));
  worldState.addFact(ogp::Fact::fromStr("pred_name(v3)", ontology, objects, {}), goalStack, setOfEvents, _emptyCallbacks, ontology, objects, {});
  EXPECT_FALSE(_isTrue("(forall (?p - my_type3) (imply (pred_name ?p) (= (fun1 ?p) toto)))", ontology, worldState, objects));
  worldState.addFact(ogp::Fact::fromStr("fun1(v3)=titi", ontology, objects, {}), goalStack, setOfEvents, _emptyCallbacks, ontology, objects, {});
  EXPECT_FALSE(_isTrue("(forall (?p - my_type3) (imply (pred_name ?p) (= (fun1 ?p) toto)))", ontology, worldState, objects));
  worldState.addFact(ogp::Fact::fromStr("fun1(v3)=toto", ontology, objects, {}), goalStack, setOfEvents, _emptyCallbacks, ontology, objects, {});
  EXPECT_TRUE(_isTrue("(forall (?p - my_type3) (imply (pred_name ?p) (= (fun1 ?p) toto)))", ontology, worldState, objects));
  objects.add(ogp::Entity::fromDeclaration("v3b - my_type3", ontology.types));
  EXPECT_TRUE(_isTrue("(forall (?p - my_type3) (imply (pred_name ?p) (= (fun1 ?p) toto)))", ontology, worldState, objects));
  worldState.addFact(ogp::Fact::fromStr("fun1(v3)=titi", ontology, objects, {}), goalStack, setOfEvents, _emptyCallbacks, ontology, objects, {});
  EXPECT_FALSE(_isTrue("(forall (?p - my_type3) (imply (pred_name ?p) (= (fun1 ?p) toto)))", ontology, worldState, objects));
  worldState.addFact(ogp::Fact::fromStr("pred_name(v3b)", ontology, objects, {}), goalStack, setOfEvents, _emptyCallbacks, ontology, objects, {});
  EXPECT_FALSE(_isTrue("(forall (?p - my_type3) (imply (pred_name ?p) (= (fun1 ?p) toto)))", ontology, worldState, objects));
  worldState.addFact(ogp::Fact::fromStr("fun1(v3)=toto", ontology, objects, {}), goalStack, setOfEvents, _emptyCallbacks, ontology, objects, {});
  EXPECT_FALSE(_isTrue("(forall (?p - my_type3) (imply (pred_name ?p) (= (fun1 ?p) toto)))", ontology, worldState, objects));
}


void _test_forall_with_imply_inside2()
{
  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity return_type");
  ontology.predicates = ogp::SetOfPredicates::fromStr("fact_1(?p - entity) - return_type\n"
                                                      "fact_2(?p - entity) - return_type\n"
                                                      "fact_3(?p - entity) - return_type\n"
                                                      "false", ontology.types);
  ontology.constants = ogp::SetOfEntities::fromPddl("r1 r2 r3 r4 r5 - return_type", ontology.types);

  ogp::WorldState worldState;
  ogp::GoalStack goalStack;
  ogp::SetOfEntities objects = ogp::SetOfEntities::fromPddl("v1 v2 v3 v4 - entity", ontology.types);
  std::map<ogp::SetOfEventsId, ogp::SetOfEvents> setOfEvents;

  worldState.addFact(ogp::Fact::fromStr("fact_3(v1)=r2", ontology, objects, {}), goalStack, setOfEvents, _emptyCallbacks, ontology, objects, {});
  worldState.addFact(ogp::Fact::fromStr("fact_3(v3)=r2", ontology, objects, {}), goalStack, setOfEvents, _emptyCallbacks, ontology, objects, {});
  worldState.addFact(ogp::Fact::fromStr("fact_2(v1)=r3", ontology, objects, {}), goalStack, setOfEvents, _emptyCallbacks, ontology, objects, {});
  worldState.addFact(ogp::Fact::fromStr("fact_2(v3)=r4", ontology, objects, {}), goalStack, setOfEvents, _emptyCallbacks, ontology, objects, {});
  worldState.addFact(ogp::Fact::fromStr("fact_1(v1)=r3", ontology, objects, {}), goalStack, setOfEvents, _emptyCallbacks, ontology, objects, {});

  EXPECT_FALSE(_isTrue("(forall (?e - entity) (imply (= (fact_3 ?e) r2) (= (fact_1 ?e) (fact_2 ?e))))", ontology, worldState, objects));
  worldState.addFact(ogp::Fact::fromStr("fact_1(v3)=r3", ontology, objects, {}), goalStack, setOfEvents, _emptyCallbacks, ontology, objects, {});
  EXPECT_TRUE(_isTrue("(forall (?e - entity) (imply (= (fact_3 ?e) r2) (= (fact_1 ?e) (fact_2 ?e))))", ontology, worldState, objects));
}



void _test_exists_with_and_list()
{
  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("location physical_object - entity\n"
                                             "user rune - physical_object");
  ontology.predicates = ogp::SetOfPredicates::fromStr("location_of(?o - physical_object) - location\n"
                                                      "wanted_location_of(?o - physical_object) - location", ontology.types);
  ontology.constants = ogp::SetOfEntities::fromPddl("unknown_human - user", ontology.types);

  ogp::WorldState worldState;
  ogp::GoalStack goalStack;
  ogp::SetOfEntities objects = ogp::SetOfEntities::fromPddl("rune1 - rune\n"
                                                            "location1 - location", ontology.types);
  std::map<ogp::SetOfEventsId, ogp::SetOfEvents> setOfEvents;

  worldState.addFact(ogp::Fact::fromStr("location_of(rune1)=location1", ontology, objects, {}), goalStack, setOfEvents, _emptyCallbacks, ontology, objects, {});
  worldState.addFact(ogp::Fact::fromStr("wanted_location_of(unknown_human)=location1", ontology, objects, {}), goalStack, setOfEvents, _emptyCallbacks, ontology, objects, {});
  worldState.addFact(ogp::Fact::fromStr("location_of(unknown_human)=location1", ontology, objects, {}), goalStack, setOfEvents, _emptyCallbacks, ontology, objects, {});

  EXPECT_TRUE(_isTrue("(and\n"
                      "  (not (= (wanted_location_of unknown_human) undefined))\n"
                      "       (= (location_of unknown_human) (wanted_location_of unknown_human))\n"
                      ")", ontology, worldState, objects));

  EXPECT_TRUE(_isTrue("(exists (?user_param - user)\n"
                      "   (and\n"
                      "       (not (= (wanted_location_of ?user_param) undefined))\n"
                      "       (= (location_of ?user_param) (wanted_location_of ?user_param))\n"
                      "   )\n"
                      ")", ontology, worldState, objects));
}


void _test_forall_with_imply_and_then_true()
{
  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity return_type");
  ontology.predicates = ogp::SetOfPredicates::fromStr("fact_1(?p - entity) - return_type\n"
                                                      "goal", ontology.types);
  ontology.constants = ogp::SetOfEntities::fromPddl("r1 r2 r3 r4 r5 - return_type", ontology.types);

  ogp::WorldState worldState;
  ogp::GoalStack goalStack;
  ogp::SetOfEntities objects = ogp::SetOfEntities::fromPddl("v1 v2 v3 v4 - entity", ontology.types);
  std::map<ogp::SetOfEventsId, ogp::SetOfEvents> setOfEvents;

  worldState.addFact(ogp::Fact::fromStr("fact_1(v1)=r2", ontology, objects, {}), goalStack, setOfEvents, _emptyCallbacks, ontology, objects, {});
  worldState.addFact(ogp::Fact::fromStr("fact_1(v3)=r2", ontology, objects, {}), goalStack, setOfEvents, _emptyCallbacks, ontology, objects, {});
  worldState.addFact(ogp::Fact::fromStr("goal", ontology, objects, {}), goalStack, setOfEvents, _emptyCallbacks, ontology, objects, {});

  EXPECT_TRUE(_isTrue("(forall (?e - entity) (imply (= (fact_1 ?e) r2) (goal)))", ontology, worldState, objects));
}



void _test_exists_with_imply_and_then_true()
{
  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity return_type");
  ontology.predicates = ogp::SetOfPredicates::fromStr("fact_1(?p - entity) - return_type\n"
                                                      "goal", ontology.types);
  ontology.constants = ogp::SetOfEntities::fromPddl("r1 r2 r3 r4 r5 - return_type", ontology.types);

  ogp::WorldState worldState;
  ogp::GoalStack goalStack;
  ogp::SetOfEntities objects = ogp::SetOfEntities::fromPddl("v1 v2 - entity", ontology.types);
  std::map<ogp::SetOfEventsId, ogp::SetOfEvents> setOfEvents;

  worldState.addFact(ogp::Fact::fromStr("fact_1(v1)=r2", ontology, objects, {}), goalStack, setOfEvents, _emptyCallbacks, ontology, objects, {});
  worldState.addFact(ogp::Fact::fromStr("fact_1(v2)=r2", ontology, objects, {}), goalStack, setOfEvents, _emptyCallbacks, ontology, objects, {});
  worldState.addFact(ogp::Fact::fromStr("goal", ontology, objects, {}), goalStack, setOfEvents, _emptyCallbacks, ontology, objects, {});

  EXPECT_FALSE(_isTrue("(exists (?e - entity) (imply (= (fact_1 ?e) r2) (not (goal))))", ontology, worldState, objects));
}

}



TEST(Tool, test_condition)
{
  _test_checkConditionWithOntology();
  _test_fluent_value_equality_with_sub_types();
  _test_exists_with_and_list_inside();
  _test_exists_with_imply_inside();
  _test_forall_with_imply_inside();
  _test_forall_with_imply_inside2();
  _test_exists_with_and_list();
  _test_forall_with_imply_and_then_true();
  _test_exists_with_imply_and_then_true();

}
