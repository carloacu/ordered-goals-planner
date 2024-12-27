#include <gtest/gtest.h>
#include <orderedgoalsplanner/types/fact.hpp>
#include <orderedgoalsplanner/types/factoptionalstoid.hpp>
#include <orderedgoalsplanner/types/ontology.hpp>
#include <orderedgoalsplanner/util/alias.hpp>
#include <orderedgoalsplanner/util/serializer/deserializefrompddl.hpp>

using namespace ogp;

namespace
{

std::string _conditionsToValueFindToStr(const FactOptionalsToId& pFactOptionalsToId,
                                        const FactOptional& pFactOptional,
                                        bool pIgnoreValue = false)
{
  std::set<std::string> ids;
  pFactOptionalsToId.find([&](const std::string& pId) {
    if (!ids.insert(pId).second)
      throw std::runtime_error("\"" + pId + "\" id already returned");
    return ContinueOrBreak::CONTINUE;
  }, pFactOptional, pIgnoreValue);

  std::stringstream ss;
  ss << "[";
  bool firstElt = true;
  for (const auto& currId : ids)
  {
    if (firstElt)
      firstElt = false;
    else
      ss << ", ";
    ss << currId;
  }
  ss << "]";
  return ss.str();
}

}




TEST(Tool, test_factOptionalsToId)
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
                                                      "pred_name5(?p1 - my_type) - my_type2\n"
                                                      "pred_name6(?e - entity)\n"
                                                      "pred_name7(?p1 - my_type, ?p2 - my_type2, ?e - entity)",
                                                      ontology.types);

  auto objects = ogp::SetOfEntities::fromPddl("toto3 - my_type\n"
                                              "titi2 - my_type2\n"
                                              "titi3 - sub_my_type2", ontology.types);

  FactOptionalsToId factOptionalsToId;

  auto factOpt1 = ogp::pddlToFactOptional("(pred_name toto)", ontology, objects);
  factOptionalsToId.add(factOpt1, "action1");

  {
    EXPECT_EQ("[action1]", _conditionsToValueFindToStr(factOptionalsToId, factOpt1));
  }

  {
    std::vector<ogp::Parameter> parameters(1, ogp::Parameter::fromStr("?p - my_type", ontology.types));
    auto factWithParam = ogp::pddlToFactOptional("(pred_name ?p)", ontology, objects, parameters);
    EXPECT_EQ("[action1]", _conditionsToValueFindToStr(factOptionalsToId, factWithParam));
  }

  auto fact2 = ogp::pddlToFactOptional("(pred_name toto2)", ontology, objects, {});
  factOptionalsToId.add(fact2, "action2");

  auto fact2b = ogp::pddlToFactOptional("(pred_name6 toto)", ontology, objects, {});
  factOptionalsToId.add(fact2b, "action2");

  {
    auto factWithParam = ogp::pddlToFactOptional("(pred_name6 toto)", ontology, objects, {});
    EXPECT_EQ("[action2]", _conditionsToValueFindToStr(factOptionalsToId, factWithParam));
  }

  {
    std::vector<ogp::Parameter> parameters(1, ogp::Parameter::fromStr("?p - my_type", ontology.types));
    auto factWithParam = ogp::pddlToFactOptional("(pred_name ?p)", ontology, objects, parameters);
    EXPECT_EQ("[action1, action2]", _conditionsToValueFindToStr(factOptionalsToId, factWithParam));
  }

  {
    std::vector<ogp::Parameter> parameters(1, ogp::Parameter::fromStr("?p - my_type", ontology.types));
    auto factWithParam = ogp::pddlToFactOptional("(pred_name2 ?p)", ontology, objects, parameters);
    EXPECT_EQ("[]", _conditionsToValueFindToStr(factOptionalsToId, factWithParam));
  }

  {
    std::vector<ogp::Parameter> parameters(1, ogp::Parameter::fromStr("?p2 - my_type2", ontology.types));
    auto factWithParam = ogp::pddlToFactOptional("(pred_name3 toto ?p2)", ontology, objects, parameters);
    EXPECT_EQ("[]", _conditionsToValueFindToStr(factOptionalsToId, factWithParam));
  }

  auto fact3 = ogp::pddlToFactOptional("(pred_name3 toto titi)", ontology, objects, {});
  factOptionalsToId.add(fact3, "action3");

  {
    std::vector<ogp::Parameter> parameters(1, ogp::Parameter::fromStr("?p2 - my_type2", ontology.types));
    auto factWithParam = ogp::pddlToFactOptional("(pred_name3 toto ?p2)", ontology, objects, parameters);
    EXPECT_EQ("[action3]", _conditionsToValueFindToStr(factOptionalsToId, factWithParam));
  }

  std::vector<ogp::Parameter> fact4Parameters(1, ogp::Parameter::fromStr("?p - my_type2", ontology.types));
  auto fact4 = ogp::pddlToFactOptional("(pred_name3 toto ?p)", ontology, objects, fact4Parameters);
  factOptionalsToId.add(fact4, "action4");

  {
    std::vector<ogp::Parameter> parameters(1, ogp::Parameter::fromStr("?p2 - my_type2", ontology.types));
    auto factWithParam = ogp::pddlToFactOptional("(pred_name3 toto ?p2)", ontology, objects, parameters);
    EXPECT_EQ("[action3, action4]", _conditionsToValueFindToStr(factOptionalsToId, factWithParam));
  }

  {
    auto factWithParam = ogp::pddlToFactOptional("(pred_name3 toto titi)", ontology, objects, {});
    EXPECT_EQ("[action3, action4]", _conditionsToValueFindToStr(factOptionalsToId, factWithParam));
  }

  std::vector<ogp::Parameter> fact4bParameters(1, ogp::Parameter::fromStr("?p - my_type2", ontology.types));
  auto fact4b = ogp::pddlToFactOptional("(pred_name3 toto ?p)", ontology, objects, fact4bParameters);
  factOptionalsToId.add(fact4b, "action4b");

  {
    std::vector<ogp::Parameter> parameters(1, ogp::Parameter::fromStr("?p1 - my_type", ontology.types));
    auto factWithParam = ogp::pddlToFactOptional("(pred_name3 ?p1 titi)", ontology, objects, parameters);
    EXPECT_EQ("[action3, action4, action4b]", _conditionsToValueFindToStr(factOptionalsToId, factWithParam));
  }

  {
    std::vector<ogp::Parameter> parameters(1, ogp::Parameter::fromStr("?p1 - my_type", ontology.types));
    auto factWithParam = ogp::pddlToFactOptional("(pred_name3 ?p1 titi2)", ontology, objects, parameters);
    EXPECT_EQ("[action4, action4b]", _conditionsToValueFindToStr(factOptionalsToId, factWithParam));
  }

  {
    auto factWithParam = ogp::pddlToFactOptional("(pred_name3 toto3 titi2)", ontology, objects, {});
    EXPECT_EQ("[]", _conditionsToValueFindToStr(factOptionalsToId, factWithParam));
  }

  {
    std::vector<ogp::Parameter> parameters(1, ogp::Parameter::fromStr("?p1 - my_type", ontology.types));
    auto factWithParam = ogp::pddlToFactOptional("(pred_name3 ?p1 titi3)", ontology, objects, parameters);
    EXPECT_EQ("[action4, action4b]", _conditionsToValueFindToStr(factOptionalsToId, factWithParam));
  }

  auto fact5 = ogp::pddlToFactOptional("(pred_name3 toto titi_const)", ontology, objects, {});
  factOptionalsToId.add(fact5, "action5");

  auto fact6 = ogp::pddlToFactOptional("(pred_name3 toto2 titi)", ontology, objects, {});
  factOptionalsToId.add(fact6, "action6");

  {
    auto factWithParam = ogp::pddlToFactOptional("(pred_name3 toto titi)", ontology, objects, {});
    EXPECT_EQ("[action3, action4, action4b]", _conditionsToValueFindToStr(factOptionalsToId, factWithParam));
  }

  // tests with pred_name4

  auto fact7 = ogp::pddlToFactOptional("(pred_name4 toto titi_const)", ontology, objects, {});
  factOptionalsToId.add(fact7, "action7");

  std::vector<ogp::Parameter> fact8Parameters(1, ogp::Parameter::fromStr("?p2 - my_type2", ontology.types));
  auto fact8 = ogp::pddlToFactOptional("(pred_name4 toto ?p2)", ontology, objects, fact8Parameters);
  factOptionalsToId.add(fact8, "action8");

  std::vector<ogp::Parameter> fact9Parameters(1, ogp::Parameter::fromStr("?p2 - my_type2", ontology.types));
  auto fact9 = ogp::pddlToFactOptional("(pred_name4 toto2 ?p2)", ontology, objects, fact9Parameters);
  factOptionalsToId.add(fact9, "action9");

  {
    auto factWithParam = ogp::pddlToFactOptional("(pred_name4 toto titi)", ontology, objects, {});
    EXPECT_EQ("[action8]", _conditionsToValueFindToStr(factOptionalsToId, factWithParam));
  }


  // tests with pred_name5

  auto fact10 = ogp::pddlToFactOptional("(= (pred_name5 toto2) titi)", ontology, objects, {});
  factOptionalsToId.add(fact10, "action10");

  std::vector<ogp::Parameter> fact11Parameters(1, ogp::Parameter::fromStr("?p1 - my_type", ontology.types));
  auto fact11 = ogp::pddlToFactOptional("(= (pred_name5 ?p1) titi)", ontology, objects, fact11Parameters);
  factOptionalsToId.add(fact11, "action11");

  std::vector<ogp::Parameter> fact12Parameters(1, ogp::Parameter::fromStr("?p1 - my_type", ontology.types));
  auto fact12 = ogp::pddlToFactOptional("(= (pred_name5 ?p1) titi_const)", ontology, objects, fact12Parameters);
  factOptionalsToId.add(fact12, "action12");

  {
    auto factWithParam = ogp::pddlToFactOptional("(= (pred_name5 toto) titi)", ontology, objects, {});
    EXPECT_EQ("[action11]", _conditionsToValueFindToStr(factOptionalsToId, factWithParam));
    EXPECT_EQ("[action11, action12]", _conditionsToValueFindToStr(factOptionalsToId, factWithParam, true));
  }

  auto fact13 = ogp::Fact::fromStr("pred_name5(toto2)!=titi", ontology, objects, {});
  factOptionalsToId.add(ogp::FactOptional(fact13), "action13");

  {
    auto factWithParam = ogp::pddlToFactOptional("(= (pred_name5 toto) titi_const)", ontology, objects, {});
    EXPECT_EQ("[action12]", _conditionsToValueFindToStr(factOptionalsToId, factWithParam));
  }

  {
    auto factWithParam = ogp::pddlToFactOptional("(= (pred_name5 toto2) titi_const)", ontology, objects, {});
    EXPECT_EQ("[action12, action13]", _conditionsToValueFindToStr(factOptionalsToId, factWithParam));
  }

  // tests with pred_name7

  auto fact14 = ogp::Fact::fromStr("pred_name7(toto, titi, toto3)", ontology, objects, {});
  factOptionalsToId.add(ogp::FactOptional(fact14), "action14");

  {
    auto factWithParam = ogp::pddlToFactOptional("(pred_name7 toto titi toto)", ontology, objects, {});
    EXPECT_EQ("[]", _conditionsToValueFindToStr(factOptionalsToId, factWithParam));
  }

  {
    std::size_t pos = 0;
    auto parameters = ogp::pddlToParameters("(?e - entity)", ontology.types);
    std::unique_ptr<ogp::Condition> cond = ogp::pddlToCondition("(and (pred_name7 toto titi toto) (pred_name7 toto titi ?e))", pos, ontology, objects, parameters);
    if (!cond)
      FAIL();
    factOptionalsToId.add(*cond, "action14");
  }

  {
    auto factWithParam = ogp::pddlToFactOptional("(pred_name7 toto titi toto)", ontology, objects, {});
    EXPECT_EQ("[action14]", _conditionsToValueFindToStr(factOptionalsToId, factWithParam));
  }

}
