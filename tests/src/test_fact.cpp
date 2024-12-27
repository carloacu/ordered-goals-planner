#include <gtest/gtest.h>
#include <orderedgoalsplanner/types/fact.hpp>
#include <orderedgoalsplanner/types/ontology.hpp>

namespace
{

std::string _printSignatures(const ogp::Fact& pFact,
                             bool pIncludeSubTypes,
                             bool pIncludeParentTypes)
{
  std::set<std::string> signatures;
  pFact.generateSignaturesWithRelatedTypes([&](const std::string& pSignature) {
    if (!signatures.insert(pSignature).second)
      throw std::runtime_error("\"" + pSignature + "\" signature found 2 times");
  }, pIncludeSubTypes, pIncludeParentTypes);

  std::string res;
  for (const auto& currSignature : signatures)
  {
    if (!res.empty())
      res += "\n";
    res += currSignature;
  }
  return res;
}


void _test_generateSignatureForUpperTypes()
{
  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("my_type my_type2 my_type3 - entity\n"
                                             "sub_my_type3 - my_type3");
  ontology.constants = ogp::SetOfEntities::fromPddl("e1 e2 - entity\n"
                                                    "toto - my_type\n"
                                                    "titi - my_type2\n", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("pred_name(?e - entity)\n"
                                                      "fun1(?e - my_type3) - entity\n"
                                                      "fun2(?e - my_type2, ?s - sub_my_type3)\n"
                                                      "fun3(?e1 - entity, ?e2 - entity)\n", ontology.types);

  ogp::SetOfEntities objects;
  objects.add(ogp::Entity::fromDeclaration("sub3a - sub_my_type3", ontology.types));
  objects.add(ogp::Entity::fromDeclaration("sub3b - sub_my_type3", ontology.types));
  EXPECT_EQ("fun1(entity)\n"
            "fun1(my_type3)\n"
            "fun1(sub_my_type3)",
            _printSignatures(ogp::Fact::fromStr("fun1(sub3a)=toto", ontology, objects, {}), false, true));

  EXPECT_EQ("fun2(entity, entity)\n"
            "fun2(entity, my_type3)\n"
            "fun2(entity, sub_my_type3)\n"
            "fun2(my_type2, entity)\n"
            "fun2(my_type2, my_type3)\n"
            "fun2(my_type2, sub_my_type3)",
            _printSignatures(ogp::Fact::fromStr("fun2(titi, sub3b)", ontology, objects, {}), false, true));

  EXPECT_EQ("fun3(entity, entity)\n"
            "fun3(entity, my_type3)\n"
            "fun3(entity, sub_my_type3)\n"
            "fun3(my_type2, entity)\n"
            "fun3(my_type2, my_type3)\n"
            "fun3(my_type2, sub_my_type3)",
            _printSignatures(ogp::Fact::fromStr("fun3(titi, sub3b)", ontology, objects, {}), false, true));

  EXPECT_EQ("fun3(entity, entity)",
            _printSignatures(ogp::Fact::fromStr("fun3(e1, e1)", ontology, objects, {}), false, true));
}

}



TEST(Tool, test_fact)
{
  _test_generateSignatureForUpperTypes();
}
