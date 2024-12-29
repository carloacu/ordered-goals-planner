#ifndef INCLUDE_ORDEREDGOALSPLANNER_CONDITIONSTOVALUE_HPP
#define INCLUDE_ORDEREDGOALSPLANNER_CONDITIONSTOVALUE_HPP

#include "../util/api.hpp"
#include <orderedgoalsplanner/types/factstovalue.hpp>

namespace ogp
{
struct Condition;
struct Fact;


struct ORDEREDGOALSPLANNER_API ConditionsToValue
{
public:
  bool add(const Condition& pCondition,
           const std::string& pValue);

  const FactsToValue& factsToValue() const { return _factsToValue; }
  const FactsToValue& notFactsToValue() const { return _notFactsToValue; }


private:
  /// Map of facts from condition to value.
  FactsToValue _factsToValue;
  /// Map of negationed facts from condition to value.
  FactsToValue _notFactsToValue;
};

} // !ogp


#endif // INCLUDE_ORDEREDGOALSPLANNER_CONDITIONSTOVALUE_HPP
