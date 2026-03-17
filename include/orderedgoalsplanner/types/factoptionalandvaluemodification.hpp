#ifndef INCLUDE_ORDEREDGOALSPLANNER_FACTOPTIONALANDVALUEMODIFICATION_HPP
#define INCLUDE_ORDEREDGOALSPLANNER_FACTOPTIONALANDVALUEMODIFICATION_HPP

#include "../util/api.hpp"
#include "factoptional.hpp"

namespace ogp
{

enum class ValueModification
{
  INCREASE,
  DECREASE,
  EXISTS
};

struct ORDEREDGOALSPLANNER_API FactOptionalAndValueModification
{
  FactOptionalAndValueModification(const FactOptional& pFactOpt,
                                   ValueModification pVm = ValueModification::EXISTS)
   : factOpt(pFactOpt),
     vm(pVm) {
  }

  bool operator<(const FactOptionalAndValueModification& pOther) const
  {
    if (factOpt != pOther.factOpt)
      return factOpt < pOther.factOpt;
    return vm < pOther.vm;
  }

  bool doesFactEffectOfSuccessorGiveAnInterestForSuccessor(const FactOptionalAndValueModification& pOptFact) const
  {
    if (factOpt.isFactNegated != pOptFact.factOpt.isFactNegated)
      return true;
    if (vm == ValueModification::INCREASE && pOptFact.vm != ValueModification::DECREASE)
      return true;
    if (vm == ValueModification::DECREASE && pOptFact.vm != ValueModification::INCREASE)
      return true;
    if (pOptFact.vm == ValueModification::INCREASE && vm != ValueModification::DECREASE)
      return true;
    if (pOptFact.vm == ValueModification::DECREASE && vm != ValueModification::INCREASE)
      return true;

    return factOpt.fact.doesFactEffectOfSuccessorGiveAnInterestForSuccessor(pOptFact.factOpt.fact);
  }

  const FactOptional& factOpt;
  ValueModification vm;
};

} // !ogp


#endif // INCLUDE_ORDEREDGOALSPLANNER_FACTOPTIONALANDVALUEMODIFICATION_HPP
