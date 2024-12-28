#include <gtest/gtest.h>
#include <orderedgoalsplanner/types/fact.hpp>
#include <orderedgoalsplanner/types/ontology.hpp>

namespace
{

std::string _printSignatures(const std::set<std::string>& pSignatures)
{
  std::string res;
  for (const auto& currSignature : pSignatures)
  {
    if (!res.empty())
      res += "\n";
    res += currSignature;
  }
  return res;
}


void _test_generateSignatureForSubAndUpperTypes()
{
  ogp::Ontology ontology;
  ontology.types = ogp::SetOfTypes::fromPddl("my_type my_type2 my_type3 - entity\n"
                                             "sub_my_type3 - my_type3");
  ontology.constants = ogp::SetOfEntities::fromPddl("toto - my_type\n"
                                                    "titi - my_type2\n", ontology.types);
  ontology.predicates = ogp::SetOfPredicates::fromStr("pred_name(?e - entity)\n"
                                                      "fun1(?e - my_type3) - entity\n"
                                                      "fun2(?e - my_type2) - entity", ontology.types);

  ogp::SetOfEntities objects;
  objects.add(ogp::Entity::fromDeclaration("sub3a - sub_my_type3", ontology.types));
  auto signatures = ogp::Fact::fromStr("fun1(sub3a)=toto", ontology, objects, {}).generateSignatureForSubAndUpperTypes2();
  EXPECT_EQ("fun1(entity)\n"
            "fun1(my_type3)\n"
            "fun1(sub_my_type3)",
            _printSignatures(signatures));
}

}



TEST(Tool, test_fact)
{
  _test_generateSignatureForSubAndUpperTypes();
}
