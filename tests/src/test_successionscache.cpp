#include <gtest/gtest.h>
#include <orderedgoalsplanner/types/action.hpp>
#include <orderedgoalsplanner/types/domain.hpp>
#include <orderedgoalsplanner/types/ontology.hpp>
#include <orderedgoalsplanner/types/setoffacts.hpp>
#include <orderedgoalsplanner/util/serializer/deserializefrompddl.hpp>


using namespace ogp;

namespace
{

void _test_actionSuccessions()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  const std::string action3 = "action3";
  const std::string action4 = "action4";
  const std::string action5 = "action5";
  const std::string action6 = "action6";

  std::map<std::string, ogp::Action> actions;

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("e1 e2 - entity\n"
                                            "resource");
  ontology.constants = ogp::SetOfEntities::fromPddl("a - entity\n"
                                                   "moves - resource", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("fact_a\n"
                                                     "fact_b(?e - entity)\n"
                                                     "fact_c\n"
                                                     "fact_d\n"
                                                     "fact_e\n"
                                                     "fact_f\n"
                                                     "fact_g() - entity\n"
                                                     "fact_h(?e - entity)\n"
                                                     "locked(?r - resource)",
                                                     ontology.types);

  {
    std::vector<ogp::Parameter> parameters1(1, ogp::Parameter::fromStr("?e - e1", ontology.types));
    ogp::Action actionObj1(ogp::strToCondition("not(locked(moves)) & fact_f", ontology, {}, parameters1),
                          ogp::strToWsModification("not(locked(moves)) & fact_f & not(fact_a) & fact_b(?e)", ontology, {}, parameters1));
    actionObj1.parameters = std::move(parameters1);
    actions.emplace(action1, actionObj1);
  }

  {
    std::vector<ogp::Parameter> parameters2(1, ogp::Parameter::fromStr("?e - e1", ontology.types));
    ogp::Action actionObj2(ogp::strToCondition("not(locked(moves)) & fact_b(?e) & fact_f", ontology, {}, parameters2),
                          ogp::strToWsModification("not(locked(moves)) & fact_c", ontology, {}, parameters2));
    actionObj2.parameters = std::move(parameters2);
    actions.emplace(action2, actionObj2);
  }

  {
    std::vector<ogp::Parameter> parameters3(1, ogp::Parameter::fromStr("?e - e2", ontology.types));
    ogp::Action actionObj3(ogp::strToCondition("not(locked(moves)) & fact_b(?e)", ontology, {}, parameters3),
                          ogp::strToWsModification("not(locked(moves)) & fact_d", ontology, {}, parameters3));
    actionObj3.parameters = std::move(parameters3);
    actions.emplace(action3, actionObj3);
  }

  {
    std::vector<ogp::Parameter> parameters4{ogp::Parameter::fromStr("?e1 - entity", ontology.types), ogp::Parameter::fromStr("?e2 - entity", ontology.types)};
    ogp::Action actionObj4(ogp::strToCondition("not(locked(moves)) & not(fact_a) & fact_h(?e1)", ontology, {}, parameters4),
                          ogp::strToWsModification("not(locked(moves)) & fact_e & fact_h(?e2)", ontology, {}, parameters4));
    actionObj4.parameters = std::move(parameters4);
    actions.emplace(action4, actionObj4);
  }

  {
    std::vector<ogp::Parameter> parameters5(1, ogp::Parameter::fromStr("?e - entity", ontology.types));
    ogp::Action actionObj5(ogp::strToCondition("not(locked(moves)) & fact_b(?e)", ontology, {}, parameters5),
                          ogp::strToWsModification("not(locked(moves)) & fact_d", ontology, {}, parameters5));
    actionObj5.parameters = std::move(parameters5);
    actions.emplace(action5, actionObj5);
  }

  {
    std::vector<ogp::Parameter> parameters6(1, ogp::Parameter::fromStr("?e - entity", ontology.types));
    ogp::Action actionObj6({}, ogp::strToWsModification("fact_g()=?e", ontology, {}, parameters6));
    actionObj6.parameters = std::move(parameters6);
    actions.emplace(action6, actionObj6);
  }

  Domain domain(actions, ontology);

  EXPECT_EQ("action: action1\n"
            "----------------------------------\n"
            "\n"
            "fact: !fact_a\n"
            "action: action4\n"
            "\n"
            "fact: fact_b(?e)\n"
            "action: action2\n"
            "action: action5\n"
            "\n"
            "\n"
            "action: action2\n"
            "----------------------------------\n"
            "\n"
            "not action: action2\n"
            "\n"
            "\n"
            "action: action3\n"
            "----------------------------------\n"
            "\n"
            "not action: action3\n"
            "not action: action5\n"
            "\n"
            "\n"
            "action: action5\n"
            "----------------------------------\n"
            "\n"
            "not action: action3\n"
            "not action: action5\n"
            "\n"
            "\n"
            "action: action6\n"
            "----------------------------------\n"
            "\n"
            "not action: action6\n", domain.printSuccessionCache());
}


void _test_notActionSuccessions()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";

  std::map<std::string, ogp::Action> actions;

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity\n"
                                            "location");
  ontology.constants = ogp::SetOfEntities::fromPddl("e1 e2 - entity", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("fact_a\n"
                                                     "fact_b(?e - entity)\n"
                                                     "fact_c(?e - entity) - location\n"
                                                     "fact_d - location",
                                                     ontology.types);

  {
    ogp::Action actionObj1({},
                          ogp::strToWsModification("fact_a & assign(fact_c(e1), fact_d())", ontology, {}, {}));
    actions.emplace(action1, actionObj1);
  }

  {
    std::vector<ogp::Parameter> parameters2{
      ogp::Parameter::fromStr("?e - entity", ontology.types),
          ogp::Parameter::fromStr("?l - location", ontology.types)};
    ogp::Action actionObj2(ogp::strToCondition("fact_a", ontology, {}, parameters2),
                          ogp::strToWsModification("fact_a & fact_b(?e) & fact_c(e1)=?l", ontology, {}, parameters2));
    actionObj2.parameters = std::move(parameters2);
    actions.emplace(action2, actionObj2);
  }

  Domain domain(actions, ontology);
  EXPECT_EQ("action: action1\n"
            "----------------------------------\n"
            "\n"
            "not action: action1\n"
            "not action: action2\n", domain.printSuccessionCache());
}

void _test_impossibleSuccessions()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  const std::string action3 = "action3";

  std::map<std::string, ogp::Action> actions;
  ogp::Ontology ontology;
  ogp::SetOfConstFacts timelessFacts;
  ontology.predicates = ogp::SetOfPredicates::fromStr("fact_a\n"
                                                     "fact_b\n"
                                                     "fact_c\n"
                                                     "fact_d",
                                                     ontology.types);
  timelessFacts.add(ogp::Fact("fact_d", false, ontology, {}, {}));

  {
    ogp::Action actionObj1({},
                          ogp::strToWsModification("fact_a & fact_b", ontology, {}, {}));
    actions.emplace(action1, actionObj1);
  }

  {
    ogp::Action actionObj2(ogp::strToCondition("fact_a & not(fact_b)", ontology, {}, {}),
                          ogp::strToWsModification("fact_c", ontology, {}, {}));
    actions.emplace(action2, actionObj2);
  }

  // Do not consider action that can never be true

  {
    ogp::Action actionObj3(ogp::strToCondition("fact_a & !fact_d", ontology, {}, {}),
                          ogp::strToWsModification("fact_c", ontology, {}, {}));
    actions.emplace(action3, actionObj3);
  }

  Domain domain(actions, ontology, {}, {}, timelessFacts);
  EXPECT_EQ("action: action1\n"
            "----------------------------------\n"
            "\n"
            "not action: action1\n"
            "not action: action2\n"
            "\n"
            "\n"
            "action: action2\n"
            "----------------------------------\n"
            "\n"
            "not action: action2\n", domain.printSuccessionCache());
}



void _test_implySuccessions()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";
  const std::string action3 = "action3";
  const std::string action4 = "action4";
  const std::string action5 = "action5";

  std::map<std::string, ogp::Action> actions;
  ogp::Ontology ontology;
  ogp::SetOfConstFacts timelessFacts;
  ontology.predicates = ogp::SetOfPredicates::fromStr("fact_a\n"
                                                      "fact_b\n"
                                                      "fact_c\n"
                                                      "fact_d",
                                                      ontology.types);

  {
    ogp::Action actionObj1({},
                          ogp::strToWsModification("fact_a", ontology, {}, {}));
    actions.emplace(action1, actionObj1);
  }

  {
    ogp::Action actionObj2(ogp::strToCondition("imply(fact_a, fact_b)", ontology, {}, {}),
                          ogp::strToWsModification("fact_c", ontology, {}, {}));
    actions.emplace(action2, actionObj2);
  }

  {
    ogp::Action actionObj3(ogp::strToCondition("imply(fact_b, fact_a)", ontology, {}, {}),
                          ogp::strToWsModification("fact_c", ontology, {}, {}));
    actions.emplace(action3, actionObj3);
  }

  actions.emplace(action4, ogp::Action(ogp::strToCondition("fact_a", ontology, {}, {}),
                                       ogp::strToWsModification("fact_a", ontology, {}, {})));

  actions.emplace(action5, ogp::Action(ogp::strToCondition("fact_a", ontology, {}, {}),
                                       ogp::strToWsModification("fact_d", ontology, {}, {})));

  Domain domain(actions, ontology, {}, {}, timelessFacts);

  EXPECT_EQ("action: action1\n"
            "----------------------------------\n"
            "\n"
            "fact: fact_a\n"
            "action: action2\n"
            "action: action3\n"
            "action: action5\n"
            "\n"
            "not action: action1\n"
            "not action: action4\n"
            "\n"
            "\n"
            "action: action2\n"
            "----------------------------------\n"
            "\n"
            "not action: action2\n"
            "not action: action3\n"
            "\n"
            "\n"
            "action: action3\n"
            "----------------------------------\n"
            "\n"
            "not action: action2\n"
            "not action: action3\n"
            "\n"
            "\n"
            "action: action4\n"
            "----------------------------------\n"
            "\n"
            "not action: action1\n"
            "not action: action4\n"
            "\n"
            "\n"
            "action: action5\n"
            "----------------------------------\n"
            "\n"
            "not action: action5\n", domain.printSuccessionCache());
}


void _test_successionsWithUndefinedValueInPrecondition()
{
  const std::string action1 = "action1";
  const std::string action2 = "action2";

  std::map<std::string, ogp::Action> actions;

  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("entity");
  ontology.constants = ogp::SetOfEntities::fromPddl("e1 e2 - entity", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("fact_a - entity\n"
                                                     "fact_b",
                                                     ontology.types);

  {
    std::vector<ogp::Parameter> parameters1{
      ogp::Parameter::fromStr("?e - entity", ontology.types)};
    ogp::Action actionObj1({},
                          ogp::strToWsModification("assign(fact_a, ?e)", ontology, {}, parameters1));
    actionObj1.parameters = std::move(parameters1);
    actions.emplace(action1, actionObj1);
  }

  {
    std::size_t pos = 0;
    ogp::Action actionObj2(ogp::pddlToCondition("(= fact_a undefined)", pos, ontology, {}, {}),
                          ogp::strToWsModification("fact_b", ontology, {}, {}));
    actions.emplace(action2, actionObj2);
  }

  Domain domain(actions, ontology);
  EXPECT_EQ("action: action1\n"
            "----------------------------------\n"
            "\n"
            "not action: action1\n"
            "\n"
            "\n"
            "action: action2\n"
            "----------------------------------\n"
            "\n"
            "not action: action2\n", domain.printSuccessionCache());
}


}


TEST(Tool, test_successionsCache)
{
  _test_actionSuccessions();
  _test_notActionSuccessions();
  _test_impossibleSuccessions();
  _test_implySuccessions();
  _test_successionsWithUndefinedValueInPrecondition();
}
