#include <orderedgoalsplanner/types/condtionstovalue.hpp>
#include <orderedgoalsplanner/types/condition.hpp>
#include <orderedgoalsplanner/types/factoptional.hpp>

namespace ogp
{


bool ConditionsToValue::add(const Condition& pCondition,
                            const std::string& pValue)
{
  bool hasAddedAFact = false;
  pCondition.forAll(
        [&](const FactOptional& pFactOptional,
        bool pIgnoreValue)
  {
    if (pFactOptional.isFactNegated)
    {
      _notFactsToValue.add(pFactOptional.fact, pValue, pIgnoreValue);
    }
    else
    {
      _factsToValue.add(pFactOptional.fact, pValue, pIgnoreValue);
      hasAddedAFact = true;
    }
    return ContinueOrBreak::CONTINUE;
  }
  );
  return hasAddedAFact;
}


void ConditionsToValue::erase(const std::string& pValue)
{
  _factsToValue.erase(pValue);
  _notFactsToValue.erase(pValue);
}



} // !ogp

