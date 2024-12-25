#include <orderedgoalsplanner/types/factoptional.hpp>


namespace ogp
{

FactOptional::FactOptional(const Fact& pFact,
                           bool pIsFactNegated)
  : isFactNegated(pIsFactNegated),
    fact(pFact)
{
}

FactOptional::FactOptional(bool pIsFactNegated,
                           const std::string& pName,
                           const std::vector<std::string>& pArgumentStrs,
                           const std::string& pValueStr,
                           bool pIsValueNegated,
                           const Ontology& pOntology,
                           const SetOfEntities& pObjects,
                           const std::vector<Parameter>& pParameters,
                           bool pIsOkIfValueIsMissing,
                           const std::map<std::string, Entity>* pParameterNamesToEntityPtr)
  : isFactNegated(pIsFactNegated),
    fact(pName, pArgumentStrs, pValueStr, pIsValueNegated, pOntology, pObjects, pParameters, pIsOkIfValueIsMissing, pParameterNamesToEntityPtr)
{
  _simplify();
}


FactOptional::FactOptional(const FactOptional& pOther,
                           const std::map<Parameter, Entity>* pParametersPtr)
  : isFactNegated(pOther.isFactNegated),
    fact(pOther.fact)
{
  if (pParametersPtr != nullptr)
    fact.replaceArguments(*pParametersPtr);
}

FactOptional::FactOptional(const std::string& pStr,
                           const Ontology& pOntology,
                           const SetOfEntities& pObjects,
                           const std::vector<Parameter>& pParameters,
                           std::size_t pBeginPos,
                           std::size_t* pResPos)
  : isFactNegated(false),
    fact(pStr, false, pOntology, pObjects, pParameters, &isFactNegated, pBeginPos, pResPos)
{
  _simplify();
}

bool FactOptional::operator<(const FactOptional& pOther) const
{
  if (isFactNegated != pOther.isFactNegated)
    return isFactNegated < pOther.isFactNegated;
  return fact < pOther.fact;
}

void FactOptional::operator=(const FactOptional& pOther)
{
  isFactNegated = pOther.isFactNegated;
  fact = pOther.fact;
}


bool FactOptional::operator==(const FactOptional& pOther) const
{
  return isFactNegated == pOther.isFactNegated &&
      fact == pOther.fact;
}

std::string FactOptional::toStr(const std::function<std::string (const Fact&)>* pFactWriterPtr,
                                bool pPrintAnyValue) const
{
  auto polarityStr = isFactNegated ? "!" : "";
  if (pFactWriterPtr)
    return polarityStr + (*pFactWriterPtr)(fact);
  return polarityStr + fact.toStr(pPrintAnyValue);
}

std::string FactOptional::toPddl(bool pInEffectContext,
                                 bool pPrintAnyValue) const
{
  auto res = fact.toPddl(pInEffectContext, pPrintAnyValue);
  if (isFactNegated)
  {
    if (fact.value() && fact.value()->isAnyEntity())
      return (pInEffectContext ? "(assign " : "(= ") + res + " undefined)";
    return "(not " + res + ")";
  }
  return res;
}

bool FactOptional::doesFactEffectOfSuccessorGiveAnInterestForSuccessor(const FactOptional& pOptFact) const
{
  if (isFactNegated != pOptFact.isFactNegated)
    return true;
  return fact.doesFactEffectOfSuccessorGiveAnInterestForSuccessor(pOptFact.fact);
}


bool FactOptional::hasAContradictionWith(const std::set<FactOptional>& pFactsOpt,
                                         std::list<Parameter>* pParametersPtr,
                                         bool pIsWrappingExpressionNegated) const
{
  for (auto& currFactOpt : pFactsOpt)
  {
    if (fact.areEqualWithoutArgsAndValueConsideration(currFactOpt.fact, pParametersPtr))
    {
      if (!pIsWrappingExpressionNegated)
      {
        if (!(fact == currFactOpt.fact && isFactNegated != currFactOpt.isFactNegated))
          return true;
      }
      else
      {
        if (!(fact == currFactOpt.fact && !isFactNegated != currFactOpt.isFactNegated))
          return true;
      }
    }
  }
  return false;
}


void FactOptional::_simplify()
{
  if (isFactNegated && fact.isValueNegated())
  {
    isFactNegated = false;
    fact.setValueNegated(false);
  }
}



} // !ogp
