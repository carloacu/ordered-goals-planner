#include <gtest/gtest.h>
#include <orderedgoalsplanner/types/entity.hpp>
#include <orderedgoalsplanner/types/parameter.hpp>
#include <orderedgoalsplanner/util/util.hpp>

using namespace ogp;

namespace
{

ogp::Parameter _parameter(const std::string& pStr) {
  return ogp::Parameter(pStr, {});
}

ogp::Entity _entity(const std::string& pStr) {
  return ogp::Entity(pStr, {});
}

std::string _toStr(const std::list<std::map<Parameter, Entity>>& pParams)
{
  std::string res;
  for (auto& currParams : pParams)
  {
    res += "(";
    bool firstIeration = true;
    for (const auto& currParam : currParams)
    {
      if (firstIeration)
        firstIeration = false;
      else
        res += ", ";
      res += currParam.first.name + " -> " + currParam.second.toStr();
    }
    res += ")";
  }
  return res;
}

std::string _unfoldMapWithSet(const std::map<Parameter, std::set<ogp::Entity>>& pInMap)
{
  ParameterValuesWithConstraints inMap;
  for (const auto& currElt : pInMap)
  {
    auto& inMapSecond = inMap[currElt.first];
    for (const auto& currEntity : currElt.second)
      inMapSecond[currEntity];
  }
  std::list<std::map<Parameter, ogp::Entity>> res;
  unfoldMapWithSet(res, inMap);
  return _toStr(res);
}

}

void test_unfoldMapWithSet()
{
  EXPECT_EQ("", _unfoldMapWithSet({}));
  EXPECT_EQ("(a -> b)", _unfoldMapWithSet({{_parameter("a"), {_entity("b")}}}));
  EXPECT_EQ("(a -> b)(a -> c)", _unfoldMapWithSet({{_parameter("a"), {_entity("b"), _entity("c")}}}));

  EXPECT_EQ("(a -> b, d -> e)",
            _unfoldMapWithSet({{_parameter("a"), {_entity("b")}}, {_parameter("d"), {_entity("e")}}}));
  EXPECT_EQ("(a -> b, d -> e)(a -> c, d -> e)",
            _unfoldMapWithSet({{_parameter("a"), {_entity("b"), _entity("c")}}, {_parameter("d"), {_entity("e")}}}));
  EXPECT_EQ("(a -> b, d -> e)(a -> b, d -> f)(a -> c, d -> e)(a -> c, d -> f)",
            _unfoldMapWithSet({{_parameter("a"), {_entity("b"), _entity("c")}}, {_parameter("d"), {_entity("e"), _entity("f")}}}));
}

void test_autoIncrementOfVersion()
{
  std::set<std::string> ids;
  auto isIdOkForInsertion = [&ids](const std::string& pId)
  {
    return ids.count(pId) == 0;
  };
  auto incrementAddIdAndReturnValue = [&](const std::string& pId)
  {
    auto newId = ogp::incrementLastNumberUntilAConditionIsSatisfied(pId, isIdOkForInsertion);
    ids.insert(newId);
    return newId;
  };

  EXPECT_EQ("", ogp::incrementLastNumberUntilAConditionIsSatisfied("", isIdOkForInsertion));
  EXPECT_EQ("dede", incrementAddIdAndReturnValue("dede"));
  EXPECT_EQ("dede_2", incrementAddIdAndReturnValue("dede"));
  EXPECT_EQ("dede_3", incrementAddIdAndReturnValue("dede"));
  EXPECT_EQ("dede_4", incrementAddIdAndReturnValue("dede_2"));
  EXPECT_EQ("dede_5", incrementAddIdAndReturnValue("dede_4"));
  EXPECT_EQ("dede_6", incrementAddIdAndReturnValue("dede_6"));
  EXPECT_EQ("didi", incrementAddIdAndReturnValue("didi"));
}


TEST(Tool, test_util)
{
  test_unfoldMapWithSet();
  test_autoIncrementOfVersion();
}
