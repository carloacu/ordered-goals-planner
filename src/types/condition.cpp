#include <orderedgoalsplanner/types/condition.hpp>
#include <algorithm>
#include <optional>
#include <orderedgoalsplanner/types/ontology.hpp>
#include <orderedgoalsplanner/types/setofderivedpredicates.hpp>
#include <orderedgoalsplanner/types/worldstate.hpp>
#include <orderedgoalsplanner/util/util.hpp>
#include "expressionParsed.hpp"

namespace ogp
{
namespace
{

bool _forEachValueUntil(const std::function<bool (const Entity&, const Fact*, const Fact*)>& pValueCallback,
                        bool pUntilValue,
                        const Condition& pCondition,
                        const WorldState& pWorldState,
                        const ParameterValuesWithConstraints& pParameters)
{
  const auto& setOfFacts = pWorldState.factsMapping();
  if (pParameters.empty())
  {
    auto valueOpt = pCondition.getValue(setOfFacts);
    if (valueOpt)
      return pValueCallback(*valueOpt, nullptr, nullptr);
  }

  std::list<std::map<Parameter, Entity>> paramPossibilities;
  unfoldMapWithSet(paramPossibilities, pParameters);
  for (auto& currParamPoss : paramPossibilities)
  {
    auto condToExtractValue = pCondition.clone(&currParamPoss);
    auto valueOpt = condToExtractValue->getValue(setOfFacts);
    if (valueOpt && pValueCallback(*valueOpt, nullptr, nullptr) == pUntilValue)
      return pUntilValue;
  }

  bool res = !pUntilValue;
  auto* factCondPtr = pCondition.fcFactPtr();
  const auto& factCond = factCondPtr->factOptional.fact;
  pWorldState.iterateOnMatchingFactsWithoutValueConsideration(
        [&](const Fact& pFact) {
    if (pFact.value() && pValueCallback(*pFact.value(), &factCond, &pFact) == pUntilValue)
      res = true;
    return false;
  },
  factCond,
  pParameters); // Parameters to consider as any value of set is empty, otherwise it is a filter

  return res;
}


void _forEach(const std::function<void (const Entity&, const Fact*)>& pValueCallback,
              const Condition& pCondition,
              const WorldState& pWorldState,
              const ParameterValuesWithConstraints* pParametersPtr)
{
  const auto& setOfFacts = pWorldState.factsMapping();
  if (pParametersPtr == nullptr || pParametersPtr->empty())
  {
    auto valueOpt = pCondition.getValue(setOfFacts);
    if (valueOpt)
      pValueCallback(*valueOpt, nullptr);
    return;
  }

  std::list<std::map<Parameter, Entity>> paramPossibilities;
  unfoldMapWithSet(paramPossibilities, *pParametersPtr);
  for (auto& currParamPoss : paramPossibilities)
  {
    auto factToExtractValue = pCondition.clone(&currParamPoss);
    auto valueOpt = factToExtractValue->getValue(setOfFacts);
    if (valueOpt)
      pValueCallback(*valueOpt, nullptr);
  }

  auto* factCondPtr = pCondition.fcFactPtr();
  if (factCondPtr != nullptr &&
      factCondPtr->factOptional.fact.value() &&
      factCondPtr->factOptional.fact.value()->isAnyEntity())
  {
    pWorldState.iterateOnMatchingFactsWithoutValueConsideration(
          [&pValueCallback](const Fact& pFact) {
      if (pFact.value())
        pValueCallback(*pFact.value(), &pFact);
      return false;
    },
    factCondPtr->factOptional.fact, // pFact
    *pParametersPtr); // Parameters to consider as any value of set is empty, otherwise it is a filter
  }
}

bool _areEqual(
    const std::unique_ptr<Condition>& pCond1,
    const std::unique_ptr<Condition>& pCond2)
{
  if (!pCond1 && !pCond2)
    return true;
  if (pCond1 && pCond2)
    return *pCond1 == *pCond2;
  return false;
}


struct ParamToEntities
{
  ParameterValuesWithConstraints paramToEntities1;
  ParameterValuesWithConstraints paramToEntities2;
};

bool _isTrueRecEquality(ParameterValuesWithConstraints& pLocalParamToValue,
                        ParameterValuesWithConstraints* pConditionParametersToPossibleArguments,
                        bool pMustBeTrueForAllParameters,
                        const Fact& pFact1,
                        const Fact& pFact2,
                        const WorldState& pWorldState,
                        const EntitiesWithParamConstaints& pAllEntitiesForParam)
{
  std::map<Entity, ParamToEntities> valueToParamToEntities;

  std::map<Entity, ParameterValuesWithConstraints> leftOpPossibleValuesToParams;
  pWorldState.iterateOnMatchingFactsWithoutParametersAndValueConsideration([&](const Fact& pFact){
    if (pFact.value())
    {
      auto& value = *pFact.value();

      auto& paramToEntities = valueToParamToEntities[value];
      for (auto& currArg : pLocalParamToValue)
      {
        auto argValue = pFact1.tryToExtractArgumentFromExampleWithoutValueConsideration(currArg.first, pFact);
        if (argValue)
          paramToEntities.paramToEntities1[currArg.first][*argValue];
      }

      auto& newParams = leftOpPossibleValuesToParams[value];
      if (pConditionParametersToPossibleArguments != nullptr)
      {
        for (auto& currArg : *pConditionParametersToPossibleArguments)
        {
          auto argValue = pFact1.tryToExtractArgumentFromExampleWithoutValueConsideration(currArg.first, pFact);
          if (argValue)
            newParams[currArg.first][*argValue];
        }
      }
    }
    return false;
  }, pFact1);

  bool res = false;
  ParameterValuesWithConstraints newParameters;
  pWorldState.iterateOnMatchingFactsWithoutParametersAndValueConsideration([&](const Fact& pFact){
    if (pFact.value())
    {
      auto& value = *pFact.value();

      auto& paramToEntities = valueToParamToEntities[value];
      for (auto& currArg : pLocalParamToValue)
      {
        auto argValue = pFact1.tryToExtractArgumentFromExampleWithoutValueConsideration(currArg.first, pFact);
        if (argValue)
          paramToEntities.paramToEntities2[currArg.first][*argValue];
      }

      auto itToLeftPoss = leftOpPossibleValuesToParams.find(value);
      if (itToLeftPoss != leftOpPossibleValuesToParams.end())
      {
        if (pConditionParametersToPossibleArguments != nullptr)
        {
          if (!itToLeftPoss->second.empty())
          {
            for (auto& currArg : itToLeftPoss->second)
              newParameters[currArg.first].insert(currArg.second.begin(), currArg.second.end());
          }
          else
          {
            for (auto& currArg : *pConditionParametersToPossibleArguments)
            {
              auto argValue = pFact2.tryToExtractArgumentFromExampleWithoutValueConsideration(currArg.first, pFact);
              if (argValue)
                newParameters[currArg.first][*argValue];
            }
          }
        }
        res = true;
      }
    }
    return res && pConditionParametersToPossibleArguments == nullptr;
  }, pFact2);

  if (pConditionParametersToPossibleArguments != nullptr)
    applyNewParams(*pConditionParametersToPossibleArguments, newParameters);

  ParameterValuesWithConstraints newLocalParamToValue;
  for (const auto& currElt : valueToParamToEntities)
  {
    for (const auto& currParam : currElt.second.paramToEntities1)
    {
      auto& resSet = newLocalParamToValue[currParam.first];
      auto itParam2 = currElt.second.paramToEntities2.find(currParam.first);
      if (itParam2 == currElt.second.paramToEntities2.end())
      {
        resSet.insert(currParam.second.begin(), currParam.second.end());
      }
      else
      {
        std::set_intersection(currParam.second.begin(), currParam.second.end(),
                              itParam2->second.begin(), itParam2->second.end(),
                              std::inserter(resSet, resSet.begin()));
      }
    }
  }

  if (pMustBeTrueForAllParameters)
  {
    if (res)
      for (const auto& currLocalParamToValue : newLocalParamToValue)
        if (currLocalParamToValue.second != pAllEntitiesForParam)
          return false;
  }

  return res;
}


bool _isTrueRec(ParameterValuesWithConstraints& pLocalParamToValue,
                ParameterValuesWithConstraints* pConditionParametersToPossibleArguments,
                bool pMustBeTrueForAllParameters,
                const Condition& pCondition,
                const WorldState& pWorldState,
                const std::set<Fact>& pPunctualFacts,
                const std::set<Fact>& pRemovedFacts,
                EntitiesWithParamConstaints& pAllEntitiesForParam)
{
  auto* factOfConditionPtr = pCondition.fcFactPtr();
  if (factOfConditionPtr != nullptr)
  {
    const auto& condFactOptional = factOfConditionPtr->factOptional;
    bool res = pWorldState.isOptionalFactSatisfiedInASpecificContext(condFactOptional, pPunctualFacts, pRemovedFacts,
                                                                     pConditionParametersToPossibleArguments, &pLocalParamToValue);
    if (pMustBeTrueForAllParameters && !condFactOptional.isFactNegated)
    {
      if (res)
        for (const auto& currLocalParamToValue : pLocalParamToValue)
          if (condFactOptional.fact.hasParameterOrValue(currLocalParamToValue.first) &&
              currLocalParamToValue.second != pAllEntitiesForParam)
            return false;
    }
    if (!pMustBeTrueForAllParameters && condFactOptional.isFactNegated)
    {
      if (!res)
        for (const auto& currLocalParamToValue : pLocalParamToValue)
          if (condFactOptional.fact.hasParameterOrValue(currLocalParamToValue.first) &&
              currLocalParamToValue.second != pAllEntitiesForParam)
            return true;
    }
    return res;
  }

  auto* nodeOfConditionPtr = pCondition.fcNodePtr();
  if (nodeOfConditionPtr != nullptr &&
      nodeOfConditionPtr->leftOperand && nodeOfConditionPtr->rightOperand)
  {
    auto updateAllEntitiesForParam = [&](const ParameterValuesWithConstraints& pParamToValue) {
      if (!pParamToValue.empty())
        pAllEntitiesForParam = pParamToValue.begin()->second;
    };

    if (nodeOfConditionPtr->nodeType == ConditionNodeType::AND)
    {
      if (!_isTrueRec(pLocalParamToValue, pConditionParametersToPossibleArguments, pMustBeTrueForAllParameters,
                     *nodeOfConditionPtr->leftOperand, pWorldState, pPunctualFacts, pRemovedFacts, pAllEntitiesForParam))
        return false;
      updateAllEntitiesForParam(pLocalParamToValue);
      return  _isTrueRec(pLocalParamToValue, pConditionParametersToPossibleArguments, pMustBeTrueForAllParameters,
                     *nodeOfConditionPtr->rightOperand, pWorldState, pPunctualFacts, pRemovedFacts, pAllEntitiesForParam);
    }
    if (nodeOfConditionPtr->nodeType == ConditionNodeType::OR)
    {
      if (_isTrueRec(pLocalParamToValue, pConditionParametersToPossibleArguments, pMustBeTrueForAllParameters,
                              *nodeOfConditionPtr->leftOperand, pWorldState, pPunctualFacts, pRemovedFacts, pAllEntitiesForParam))
        return true;
      updateAllEntitiesForParam(pLocalParamToValue);
      return _isTrueRec(pLocalParamToValue, pConditionParametersToPossibleArguments, pMustBeTrueForAllParameters,
                        *nodeOfConditionPtr->rightOperand, pWorldState, pPunctualFacts, pRemovedFacts, pAllEntitiesForParam);
    }

    if (nodeOfConditionPtr->nodeType == ConditionNodeType::IMPLY)
    {
      auto implyLocalParamToValue = pLocalParamToValue;
      if (_isTrueRec(implyLocalParamToValue, pConditionParametersToPossibleArguments, false,
                     *nodeOfConditionPtr->leftOperand, pWorldState, pPunctualFacts, pRemovedFacts, pAllEntitiesForParam))
      {
        // For exists (!pMustBeTrueForAllParameters) if one parameter makes that imply not satisfied it is ok
        if (!pMustBeTrueForAllParameters && !implyLocalParamToValue.empty() && pAllEntitiesForParam != implyLocalParamToValue.begin()->second)
          return true;
        updateAllEntitiesForParam(implyLocalParamToValue);
        return _isTrueRec(pLocalParamToValue, pConditionParametersToPossibleArguments, pMustBeTrueForAllParameters,
                          *nodeOfConditionPtr->rightOperand, pWorldState, pPunctualFacts, pRemovedFacts, pAllEntitiesForParam);
      }
      return true;
    }

    if (nodeOfConditionPtr->nodeType == ConditionNodeType::EQUALITY)
    {
      auto* leftOpFactPtr = nodeOfConditionPtr->leftOperand->fcFactPtr();
      if (leftOpFactPtr != nullptr)
      {
        auto& leftOpFact = *leftOpFactPtr;
        auto* rightOpFactPtr = nodeOfConditionPtr->rightOperand->fcFactPtr();
        if (rightOpFactPtr != nullptr)
        {
          auto& rightOpFact = *rightOpFactPtr;
          return _isTrueRecEquality(pLocalParamToValue, pConditionParametersToPossibleArguments,
                                    pMustBeTrueForAllParameters,
                                    leftOpFact.factOptional.fact, rightOpFact.factOptional.fact,
                                    pWorldState, pAllEntitiesForParam);
        }
      }
      return false;
    }
  }
  return false;
}


void _extractParamForNotLoopRec(ParameterValuesWithConstraints& pLocalParamToValue,
                           const ParameterValuesWithConstraints& pConditionParametersToPossibleArguments,
                           const Condition& pCondition,
                           const SetOfFacts& pFacts,
                           const Fact& pFactFromEffect,
                           const Parameter& pParameter)
{
  auto* factOfConditionPtr = pCondition.fcFactPtr();
  if (factOfConditionPtr != nullptr)
  {
    const auto& factOfConditionOpt = factOfConditionPtr->factOptional;
    if (!factOfConditionOpt.isFactNegated ||
        !factOfConditionOpt.fact.areEqualWithoutAnArgConsideration(pFactFromEffect, pParameter.name))
    {
      ParameterValuesWithConstraints newParameters;
      factOfConditionOpt.fact.isInOtherFactsMap(pFacts, &newParameters, &pConditionParametersToPossibleArguments, &pLocalParamToValue);
    }
    return;
  }

  auto* nodeOfConditionPtr = pCondition.fcNodePtr();
  if (nodeOfConditionPtr != nullptr &&
      nodeOfConditionPtr->leftOperand && nodeOfConditionPtr->rightOperand &&
      (nodeOfConditionPtr->nodeType == ConditionNodeType::AND ||
       nodeOfConditionPtr->nodeType == ConditionNodeType::OR ||
       nodeOfConditionPtr->nodeType == ConditionNodeType::IMPLY))
  {
    _extractParamForNotLoopRec(pLocalParamToValue, pConditionParametersToPossibleArguments, *nodeOfConditionPtr->leftOperand, pFacts, pFactFromEffect, pParameter);
    _extractParamForNotLoopRec(pLocalParamToValue, pConditionParametersToPossibleArguments, *nodeOfConditionPtr->rightOperand, pFacts, pFactFromEffect, pParameter);
  }
}

}


bool Condition::isOptFactMandatory(const FactOptional& pFactOptional,
                                   bool pIgnoreValue) const
{
  bool res = false;
  forAll([&](const FactOptional& pFactOptionalFromCond, bool pIgnoreValueFromCond) {
    if (pFactOptional.isFactNegated == pFactOptionalFromCond.isFactNegated)
    {
      if (pIgnoreValue || pIgnoreValueFromCond)
        return ContinueOrBreak::CONTINUE;
      if (pFactOptional.fact == pFactOptionalFromCond.fact)
      {
        res = true;
        return ContinueOrBreak::BREAK;
      }
    }
    return ContinueOrBreak::CONTINUE;
  }, false, false, true);
  return res;
}


std::set<FactOptional> Condition::getAllOptFacts() const
{
  std::set<FactOptional> res;
  forAll([&](const FactOptional& pFactOptional, bool) {
      res.insert(pFactOptional);
      return ContinueOrBreak::CONTINUE;
  });
  return res;
}


std::string ConditionNode::toStr(const std::function<std::string (const Fact&)>* pFactWriterPtr,
                                 bool pPrintAnyValue) const
{
  bool printAnyValue = pPrintAnyValue && nodeType != ConditionNodeType::EQUALITY &&
      nodeType != ConditionNodeType::SUPERIOR &&  nodeType != ConditionNodeType::SUPERIOR_OR_EQUAL &&
      nodeType != ConditionNodeType::INFERIOR &&  nodeType != ConditionNodeType::INFERIOR_OR_EQUAL;

  std::string leftOperandStr;
  if (leftOperand)
    leftOperandStr = leftOperand->toStr(pFactWriterPtr, printAnyValue);

  std::string rightOperandStr;
  if (rightOperand)
    rightOperandStr = rightOperand->toStr(pFactWriterPtr, printAnyValue);

  switch (nodeType)
  {
  case ConditionNodeType::AND:
    return leftOperandStr + " & " + rightOperandStr;
  case ConditionNodeType::OR:
    return leftOperandStr + " | " + rightOperandStr;
  case ConditionNodeType::IMPLY:
    return "imply(" + leftOperandStr + ", " + rightOperandStr + ")";
  case ConditionNodeType::EQUALITY:
    return "equals(" + leftOperandStr + ", " + rightOperandStr + ")";
  case ConditionNodeType::PLUS:
    return leftOperandStr + " + " + rightOperandStr;
  case ConditionNodeType::MINUS:
    return leftOperandStr + " - " + rightOperandStr;
  case ConditionNodeType::SUPERIOR:
    return leftOperandStr + ">" + rightOperandStr;
  case ConditionNodeType::SUPERIOR_OR_EQUAL:
    return leftOperandStr + ">=" + rightOperandStr;
  case ConditionNodeType::INFERIOR:
    return leftOperandStr + "<" + rightOperandStr;
  case ConditionNodeType::INFERIOR_OR_EQUAL:
    return leftOperandStr + "<=" + rightOperandStr;
  }
  return "";
}



ConditionNode::ConditionNode(ConditionNodeType pNodeType,
                             std::unique_ptr<Condition> pLeftOperand,
                             std::unique_ptr<Condition> pRightOperand)
  : Condition(),
    nodeType(pNodeType),
    leftOperand(std::move(pLeftOperand)),
    rightOperand(std::move(pRightOperand))
{
}

bool ConditionNode::hasFact(const Fact& pFact) const
{
  return (leftOperand && leftOperand->hasFact(pFact)) ||
      (rightOperand && rightOperand->hasFact(pFact));
}

bool ConditionNode::hasEntity(const std::string& pEntityId) const
{
  return (leftOperand && leftOperand->hasEntity(pEntityId)) ||
      (rightOperand && rightOperand->hasEntity(pEntityId));
}


ContinueOrBreak ConditionNode::forAll(const std::function<ContinueOrBreak (const FactOptional&, bool)>& pFactCallback,
                                      bool pIsWrappingExpressionNegated,
                                      bool pIgnoreValue,
                                      bool pOnlyMandatoryFacts) const
{
  if (pOnlyMandatoryFacts && nodeType == ConditionNodeType::OR)
    return ContinueOrBreak::CONTINUE;
  bool ignoreValue = pIgnoreValue || (nodeType != ConditionNodeType::AND &&
      nodeType != ConditionNodeType::OR && nodeType != ConditionNodeType::IMPLY);
  auto res = ContinueOrBreak::CONTINUE;
  if (leftOperand)
    res = leftOperand->forAll(pFactCallback, pIsWrappingExpressionNegated, ignoreValue, pOnlyMandatoryFacts);
  if (rightOperand && res == ContinueOrBreak::CONTINUE)
    res = rightOperand->forAll(pFactCallback, pIsWrappingExpressionNegated, ignoreValue, pOnlyMandatoryFacts);
  return res;
}


bool ConditionNode::findConditionCandidateFromFactFromEffect(
    const std::function<bool (const FactOptional&, const Fact*, const Fact*)>& pDoesConditionFactMatchFactFromEffect,
    const WorldState& pWorldState,
    const SetOfEntities& pConstants,
    const SetOfEntities& pObjects,
    const Fact& pFactFromEffect,
    const ParameterValuesWithConstraints& pFactFromEffectParameters,
    const ParameterValuesWithConstraints* pFactFromEffectTmpParametersPtr,
    const ParameterValuesWithConstraints& pConditionParametersToPossibleArguments,
    bool pIsWrappingExpressionNegated) const
{
  if (nodeType == ConditionNodeType::AND || nodeType == ConditionNodeType::OR)
  {
    if (leftOperand && leftOperand->findConditionCandidateFromFactFromEffect(pDoesConditionFactMatchFactFromEffect, pWorldState, pConstants, pObjects,
                                                                             pFactFromEffect, pFactFromEffectParameters, pFactFromEffectTmpParametersPtr,
                                                                             pConditionParametersToPossibleArguments, pIsWrappingExpressionNegated))
      return true;
    if (rightOperand && rightOperand->findConditionCandidateFromFactFromEffect(pDoesConditionFactMatchFactFromEffect, pWorldState, pConstants, pObjects,
                                                                               pFactFromEffect, pFactFromEffectParameters, pFactFromEffectTmpParametersPtr,
                                                                               pConditionParametersToPossibleArguments, pIsWrappingExpressionNegated))
      return true;
  }
  else if (nodeType == ConditionNodeType::IMPLY)
  {
    auto conditionParametersToPossibleArguments = pConditionParametersToPossibleArguments;
    if (leftOperand && leftOperand->isTrue(pWorldState, pConstants, pObjects, {}, {}, &conditionParametersToPossibleArguments) &&
        rightOperand && rightOperand->findConditionCandidateFromFactFromEffect(pDoesConditionFactMatchFactFromEffect, pWorldState, pConstants, pObjects,
                                                                               pFactFromEffect, pFactFromEffectParameters, pFactFromEffectTmpParametersPtr,
                                                                               pConditionParametersToPossibleArguments, pIsWrappingExpressionNegated))
      return true;
  }
  else if (leftOperand && rightOperand)
  {
    auto* leftFactPtr = leftOperand->fcFactPtr();
    if (leftFactPtr != nullptr)
    {
      const auto& leftFact = *leftFactPtr;
      if (nodeType == ConditionNodeType::EQUALITY)
      {
        if (leftFact.factOptional.fact.areEqualWithoutValueConsideration(pFactFromEffect, &pFactFromEffectParameters, pFactFromEffectTmpParametersPtr))
        {
          bool potRes = _forEachValueUntil(
                [&](const Entity& pValue, const Fact* pOtherPatternPtr, const Fact* pOtherInstancePtr)
          {
            auto factToCheck = leftFact.factOptional.fact;
            factToCheck.setValue(pValue);
            return pDoesConditionFactMatchFactFromEffect(FactOptional(factToCheck), pOtherPatternPtr, pOtherInstancePtr);
          }, true, *rightOperand, pWorldState, pConditionParametersToPossibleArguments);
          if (potRes)
            return true;
        }

        auto* rightFactPtr = rightOperand->fcFactPtr();
        if (rightFactPtr != nullptr)
        {
          const auto& rightFact = *rightFactPtr;
          if (rightFact.factOptional.fact.areEqualWithoutValueConsideration(pFactFromEffect, &pFactFromEffectParameters, pFactFromEffectTmpParametersPtr))
          {
            return _forEachValueUntil(
                  [&](const Entity& pValue, const Fact* pOtherPatternPtr, const Fact* pOtherInstancePtr)
            {
              auto factToCheck = rightFact.factOptional.fact;
              factToCheck.setValue(pValue);
              return pDoesConditionFactMatchFactFromEffect(FactOptional(factToCheck), pOtherPatternPtr, pOtherInstancePtr);
            }, true, *leftOperand, pWorldState, pConditionParametersToPossibleArguments);
          }
        }
      }
      else if (nodeType == ConditionNodeType::SUPERIOR || nodeType == ConditionNodeType::SUPERIOR_OR_EQUAL ||
               nodeType == ConditionNodeType::INFERIOR || nodeType == ConditionNodeType::INFERIOR_OR_EQUAL)
      {
         return pDoesConditionFactMatchFactFromEffect(leftFact.factOptional, nullptr, nullptr);
      }
    }
  }
  return false;
}



bool ConditionNode::untilFalse(const std::function<bool (const FactOptional&)>& pFactCallback,
                               const SetOfFacts& pSetOfFact) const
{
  if (nodeType == ConditionNodeType::AND || nodeType == ConditionNodeType::OR|| nodeType == ConditionNodeType::IMPLY)
  {
    if (leftOperand && !leftOperand->untilFalse(pFactCallback, pSetOfFact))
      return false;
    if (rightOperand && !rightOperand->untilFalse(pFactCallback, pSetOfFact))
      return false;
  }
  else if (leftOperand && rightOperand)
  {
    auto* leftFactPtr = leftOperand->fcFactPtr();
    if (leftFactPtr != nullptr)
    {
      const auto& leftFact = *leftFactPtr;
      if (nodeType == ConditionNodeType::EQUALITY)
      {
        auto factToCheck = leftFact.factOptional.fact;
        factToCheck.setValue(rightOperand->getValue(pSetOfFact));
        return pFactCallback(FactOptional(factToCheck));
      }
      else if (nodeType == ConditionNodeType::SUPERIOR || nodeType == ConditionNodeType::SUPERIOR_OR_EQUAL ||
               nodeType == ConditionNodeType::INFERIOR || nodeType == ConditionNodeType::INFERIOR_OR_EQUAL)
      {
         return pFactCallback(leftFact.factOptional);
      }
    }
  }
  return true;
}


bool ConditionNode::isTrue(const WorldState& pWorldState,
                           const SetOfEntities& pConstants,
                           const SetOfEntities& pObjects,
                           const std::set<Fact>& pPunctualFacts,
                           const std::set<Fact>& pRemovedFacts,
                           ParameterValuesWithConstraints* pConditionParametersToPossibleArguments,
                           bool pIsWrappingExpressionNegated) const
{
  if (nodeType == ConditionNodeType::AND)
  {
    if (leftOperand && !leftOperand->isTrue(pWorldState, pConstants, pObjects, pPunctualFacts, pRemovedFacts, pConditionParametersToPossibleArguments, false))
      return pIsWrappingExpressionNegated;
    if (rightOperand && !rightOperand->isTrue(pWorldState, pConstants, pObjects, pPunctualFacts, pRemovedFacts, pConditionParametersToPossibleArguments, false))
      return pIsWrappingExpressionNegated;
    return !pIsWrappingExpressionNegated;
  }

  if (nodeType == ConditionNodeType::OR)
  {
    if (leftOperand && leftOperand->isTrue(pWorldState, pConstants, pObjects, pPunctualFacts, pRemovedFacts, pConditionParametersToPossibleArguments, false))
      return !pIsWrappingExpressionNegated;
    if (rightOperand && rightOperand->isTrue(pWorldState, pConstants, pObjects, pPunctualFacts, pRemovedFacts, pConditionParametersToPossibleArguments, false))
      return !pIsWrappingExpressionNegated;
    return pIsWrappingExpressionNegated;
  }

  if (nodeType == ConditionNodeType::IMPLY)
  {
    if (leftOperand && leftOperand->isTrue(pWorldState, pConstants, pObjects, pPunctualFacts, pRemovedFacts, pConditionParametersToPossibleArguments, false))
    {
      if (rightOperand && !rightOperand->isTrue(pWorldState, pConstants, pObjects, pPunctualFacts, pRemovedFacts, pConditionParametersToPossibleArguments, false))
        return pIsWrappingExpressionNegated;
    }
    return !pIsWrappingExpressionNegated;
  }

  if (leftOperand && rightOperand)
  {
    auto* leftFactPtr = leftOperand->fcFactPtr();
    if (leftFactPtr != nullptr)
    {
      const auto& leftFact = leftFactPtr->factOptional.fact;

      if (nodeType == ConditionNodeType::EQUALITY)
      {
        const auto& factAccessorsToFacts = pWorldState.factsMapping();
        bool res = false;
        ParameterValuesWithConstraints newParameters;
        _forEach([&](const Entity& pValue, const Fact* pFromFactPtr)
        {
          auto factToCheck = leftFactPtr->factOptional.fact;
          factToCheck.setValue(pValue);
          bool subRes = false;
          if (factToCheck.isPunctual())
            subRes = pPunctualFacts.count(factToCheck) != 0;
          else
            subRes = factToCheck.isInOtherFactsMap(factAccessorsToFacts, &newParameters, pConditionParametersToPossibleArguments);

          // Try to resolve the parameters
          if (subRes && pFromFactPtr != nullptr &&
              pConditionParametersToPossibleArguments != nullptr &&
              !pConditionParametersToPossibleArguments->empty())
          {
            auto* rightFactPtr = rightOperand->fcFactPtr();
            if (rightFactPtr != nullptr)
            {
              for (auto& currArg : *pConditionParametersToPossibleArguments)
                if (currArg.second.empty())
                {
                  auto value = rightFactPtr->factOptional.fact.tryToExtractArgumentFromExampleWithoutValueConsideration(currArg.first, *pFromFactPtr);
                  if (value)
                    currArg.second[*value];
                }
            }
          }

          res = subRes || res;
        }, *rightOperand, pWorldState, pConditionParametersToPossibleArguments);

        if (pConditionParametersToPossibleArguments != nullptr)
          applyNewParams(*pConditionParametersToPossibleArguments, newParameters);
        if (!pIsWrappingExpressionNegated)
          return res;
        return !res;
      }
      else if (nodeType == ConditionNodeType::SUPERIOR || nodeType == ConditionNodeType::SUPERIOR_OR_EQUAL ||
               nodeType == ConditionNodeType::INFERIOR || nodeType == ConditionNodeType::INFERIOR_OR_EQUAL)
      {
        auto* rightNbPtr = rightOperand->fcNbPtr();
        if (rightNbPtr != nullptr)
        {
          const auto& factsMapping = pWorldState.factsMapping();
          auto leftFactMatchingInWs = factsMapping.find(leftFact);
          for (const auto& currWsFact : leftFactMatchingInWs)
          {
            if (currWsFact.value() &&
                leftFact.areEqualWithoutValueConsideration(currWsFact))
            {
              bool res = compIntNb(currWsFact.value()->value, rightNbPtr->nb,
                                   canBeSuperior(nodeType), canBeEqual(nodeType));
              if (!pIsWrappingExpressionNegated)
                return res;
              return !res;
            }
          }
        }
      }
    }
  }

  return !pIsWrappingExpressionNegated;
}

bool ConditionNode::operator==(const Condition& pOther) const
{
  auto* otherNodePtr = pOther.fcNodePtr();
  return otherNodePtr != nullptr &&
      nodeType == otherNodePtr->nodeType &&
      _areEqual(leftOperand, otherNodePtr->leftOperand) &&
      _areEqual(rightOperand, otherNodePtr->rightOperand);
}

std::optional<Entity> ConditionNode::getValue(const SetOfFacts& pSetOfFact) const
{
  if (nodeType == ConditionNodeType::PLUS)
  {
    auto leftValue = leftOperand->getValue(pSetOfFact);
    auto rightValue = rightOperand->getValue(pSetOfFact);
    return plusIntOrStr(leftValue, rightValue);
  }
  if (nodeType == ConditionNodeType::MINUS)
  {
    auto leftValue = leftOperand->getValue(pSetOfFact);
    auto rightValue = rightOperand->getValue(pSetOfFact);
    return minusIntOrStr(leftValue, rightValue);
  }
  return {};
}


std::unique_ptr<Condition> ConditionNode::clone(const std::map<Parameter, Entity>* pConditionParametersToArgumentPtr,
                                                bool pInvert,
                                                const SetOfDerivedPredicates* pDerivedPredicatesPtr) const
{
  if (!pInvert)
    return std::make_unique<ConditionNode>(
          nodeType,
          leftOperand ? leftOperand->clone(pConditionParametersToArgumentPtr, false, pDerivedPredicatesPtr) : std::unique_ptr<Condition>(),
          rightOperand ? rightOperand->clone(pConditionParametersToArgumentPtr, false, pDerivedPredicatesPtr) : std::unique_ptr<Condition>());

  std::optional<ConditionNodeType> invertedNodeOpt;
  switch (nodeType) {
  case ConditionNodeType::AND:
    invertedNodeOpt.emplace(ConditionNodeType::OR);
    break;
  case ConditionNodeType::OR:
    invertedNodeOpt.emplace(ConditionNodeType::AND);
    break;
  }
  if (invertedNodeOpt)
    return std::make_unique<ConditionNode>(
          *invertedNodeOpt,
          leftOperand ? leftOperand->clone(pConditionParametersToArgumentPtr, true, pDerivedPredicatesPtr) : std::unique_ptr<Condition>(),
          rightOperand ? rightOperand->clone(pConditionParametersToArgumentPtr, true, pDerivedPredicatesPtr) : std::unique_ptr<Condition>());

  return std::make_unique<ConditionNot>(
        std::make_unique<ConditionNode>(
          nodeType,
          leftOperand ? leftOperand->clone(pConditionParametersToArgumentPtr, false, pDerivedPredicatesPtr) : std::unique_ptr<Condition>(),
          rightOperand ? rightOperand->clone(pConditionParametersToArgumentPtr, false, pDerivedPredicatesPtr) : std::unique_ptr<Condition>()));
}


bool ConditionNode::hasAContradictionWith(const std::set<FactOptional>& pFactsOpt,
                                          bool pIsWrappingExpressionNegated,
                                          std::list<Parameter>* pParametersPtr) const
{
  bool and_or_imply = nodeType == ConditionNodeType::AND || nodeType == ConditionNodeType::IMPLY;
  if ((and_or_imply && !pIsWrappingExpressionNegated) ||
      (nodeType == ConditionNodeType::OR && pIsWrappingExpressionNegated))
  {
    if (leftOperand && leftOperand->hasAContradictionWith(pFactsOpt, pIsWrappingExpressionNegated, pParametersPtr))
      return true;
    return rightOperand && rightOperand->hasAContradictionWith(pFactsOpt, pIsWrappingExpressionNegated, pParametersPtr);
  }
  else if ((nodeType == ConditionNodeType::OR && !pIsWrappingExpressionNegated) ||
           (and_or_imply && pIsWrappingExpressionNegated))
  {
    return leftOperand && leftOperand->hasAContradictionWith(pFactsOpt, pIsWrappingExpressionNegated, pParametersPtr) &&
        rightOperand && rightOperand->hasAContradictionWith(pFactsOpt, pIsWrappingExpressionNegated, pParametersPtr);
  }
  else if (leftOperand && rightOperand)
  {
    auto* leftFactPtr = leftOperand->fcFactPtr();
    if (leftFactPtr != nullptr)
    {
      const auto& leftFact = leftFactPtr->factOptional.fact;
      if (nodeType == ConditionNodeType::EQUALITY ||
          nodeType == ConditionNodeType::SUPERIOR || nodeType == ConditionNodeType::SUPERIOR_OR_EQUAL ||
          nodeType == ConditionNodeType::INFERIOR || nodeType == ConditionNodeType::INFERIOR_OR_EQUAL)
      {
        for (const auto& currFactOpt : pFactsOpt)
          if (leftFact.areEqualWithoutValueConsideration(currFactOpt.fact))
            return true;
        return false;
      }
    }
  }
  throw std::runtime_error("Noce badly managed in hasAContradictionWith");
}




ConditionExists::ConditionExists(const Parameter& pParameter,
                                 std::unique_ptr<Condition> pCondition)
  : Condition(),
    parameter(pParameter),
    condition(std::move(pCondition))
{
}

std::string ConditionExists::toStr(const std::function<std::string (const Fact&)>* pFactWriterPtr,
                                   bool) const
{
  std::string conditionStr;
  if (condition)
    conditionStr = condition->toStr(pFactWriterPtr);
  return "exists(" + parameter.toStr() + ", " + conditionStr + ")";
}

bool ConditionExists::hasFact(const Fact& pFact) const
{
  return condition && condition->hasFact(pFact);
}

bool ConditionExists::hasEntity(const std::string& pEntityId) const
{
  return condition && condition->hasEntity(pEntityId);
}


ContinueOrBreak ConditionExists::forAll(const std::function<ContinueOrBreak (const FactOptional&, bool)>& pFactCallback,
                                        bool pIsWrappingExpressionNegated,
                                        bool pIgnoreValue,
                                        bool pOnlyMandatoryFacts) const
{

  if (condition)
    return condition->forAll(pFactCallback, pIsWrappingExpressionNegated, pIgnoreValue, pOnlyMandatoryFacts);
  return ContinueOrBreak::CONTINUE;
}


bool ConditionExists::findConditionCandidateFromFactFromEffect(
    const std::function<bool (const FactOptional&, const Fact*, const Fact*)>& pDoesConditionFactMatchFactFromEffect,
    const WorldState& pWorldState,
    const SetOfEntities& pConstants,
    const SetOfEntities& pObjects,
    const Fact& pFactFromEffect,
    const ParameterValuesWithConstraints& pFactFromEffectParameters,
    const ParameterValuesWithConstraints* pFactFromEffectTmpParametersPtr,
    const ParameterValuesWithConstraints& pConditionParametersToPossibleArguments,
    bool pIsWrappingExpressionNegated) const
{
  if (condition)
  {
    ParameterValuesWithConstraints localParamToValue{{parameter, {}}};
    if (pIsWrappingExpressionNegated)
    {
      const auto& factsMapping = pWorldState.factsMapping();
      _extractParamForNotLoopRec(localParamToValue, pConditionParametersToPossibleArguments, *condition, factsMapping, pFactFromEffect, parameter);
    }
    auto parameters = pConditionParametersToPossibleArguments;
    parameters[parameter];

    return condition->findConditionCandidateFromFactFromEffect([&](const FactOptional& pConditionFact, const Fact* pOtherPatternPtr, const Fact* pOtherInstancePtr) {
      auto factToConsider = pConditionFact;
      factToConsider.fact.replaceArguments(localParamToValue);
      return pDoesConditionFactMatchFactFromEffect(factToConsider, pOtherPatternPtr, pOtherInstancePtr) == !pIsWrappingExpressionNegated;
    }, pWorldState, pConstants, pObjects, pFactFromEffect, pFactFromEffectParameters, pFactFromEffectTmpParametersPtr,
    parameters, pIsWrappingExpressionNegated);
  }
  return pIsWrappingExpressionNegated;
}


bool ConditionExists::isTrue(const WorldState& pWorldState,
                             const SetOfEntities& pConstants,
                             const SetOfEntities& pObjects,
                             const std::set<Fact>& pPunctualFacts,
                             const std::set<Fact>& pRemovedFacts,
                             ParameterValuesWithConstraints* pConditionParametersToPossibleArguments,
                             bool pIsWrappingExpressionNegated) const
{
  EntitiesWithParamConstaints entities;
  if (parameter.type)
    entities = typeToEntities(*parameter.type, pConstants, pObjects);
  if (entities.empty())
      return pIsWrappingExpressionNegated;

  if (condition)
  {
    ParameterValuesWithConstraints localParamToValue{{parameter, {}}};
    return _isTrueRec(localParamToValue, pConditionParametersToPossibleArguments, false, *condition, pWorldState,
                      pPunctualFacts, pRemovedFacts, entities) == !pIsWrappingExpressionNegated;
  }
  return !pIsWrappingExpressionNegated;
}


bool ConditionExists::operator==(const Condition& pOther) const
{
  auto* otherExistsPtr = pOther.fcExistsPtr();
  return otherExistsPtr != nullptr &&
      parameter == otherExistsPtr->parameter &&
      _areEqual(condition, otherExistsPtr->condition);
}

std::unique_ptr<Condition> ConditionExists::clone(const std::map<Parameter, Entity>* pConditionParametersToArgumentPtr,
                                                  bool pInvert,
                                                  const SetOfDerivedPredicates* pDerivedPredicatesPtr) const
{
  auto res = std::make_unique<ConditionExists>(
        parameter,
        condition ? condition->clone(pConditionParametersToArgumentPtr, false, pDerivedPredicatesPtr) : std::unique_ptr<Condition>());
  if (pInvert)
    return std::make_unique<ConditionNot>(std::move(res));
  return res;
}


bool ConditionExists::hasAContradictionWith(const std::set<FactOptional>& pFactsOpt,
                                            bool pIsWrappingExpressionNegated,
                                            std::list<Parameter>* pParametersPtr) const
{
  if (!condition)
    return false;
  auto contextParameters = addParameter(pParametersPtr, parameter);

  auto* factOfConditionPtr = condition->fcFactPtr();
  if (factOfConditionPtr != nullptr)
  {
    return factOfConditionPtr->factOptional.hasAContradictionWith(pFactsOpt, &contextParameters, pIsWrappingExpressionNegated);
  }

  auto* nodeOfConditionPtr = condition->fcNodePtr();
  if (nodeOfConditionPtr != nullptr &&
      nodeOfConditionPtr->leftOperand && nodeOfConditionPtr->rightOperand)
  {
    if (nodeOfConditionPtr->nodeType == ConditionNodeType::EQUALITY ||
        nodeOfConditionPtr->nodeType == ConditionNodeType::SUPERIOR || nodeOfConditionPtr->nodeType == ConditionNodeType::SUPERIOR_OR_EQUAL ||
        nodeOfConditionPtr->nodeType == ConditionNodeType::INFERIOR || nodeOfConditionPtr->nodeType == ConditionNodeType::INFERIOR_OR_EQUAL)
    {
      for (auto& currFactOpt : pFactsOpt)
        if (factOfConditionPtr->factOptional.fact.areEqualWithoutArgsAndValueConsideration(currFactOpt.fact, &contextParameters))
          return true;
      return false;
    }

    if (nodeOfConditionPtr->nodeType == ConditionNodeType::AND ||
        nodeOfConditionPtr->nodeType == ConditionNodeType::OR ||
        nodeOfConditionPtr->nodeType == ConditionNodeType::IMPLY)
    {
      return condition->hasAContradictionWith(pFactsOpt, pIsWrappingExpressionNegated, &contextParameters);
    }
  }
  return true;
}





ConditionForall::ConditionForall(const Parameter& pParameter,
                                 std::unique_ptr<Condition> pCondition)
  : Condition(),
    parameter(pParameter),
    condition(std::move(pCondition))
{
}

std::string ConditionForall::toStr(const std::function<std::string (const Fact&)>* pFactWriterPtr,
                                   bool) const
{
  std::string conditionStr;
  if (condition)
    conditionStr = condition->toStr(pFactWriterPtr);
  return "forall(" + parameter.toStr() + ", " + conditionStr + ")";
}

bool ConditionForall::hasFact(const Fact& pFact) const
{
  return condition && condition->hasFact(pFact);
}

bool ConditionForall::hasEntity(const std::string& pEntityId) const
{
  return condition && condition->hasEntity(pEntityId);
}

ContinueOrBreak ConditionForall::forAll(const std::function<ContinueOrBreak (const FactOptional&, bool)>& pFactCallback,
                                        bool pIsWrappingExpressionNegated,
                                        bool pIgnoreValue,
                                        bool pOnlyMandatoryFacts) const
{

  if (condition)
    return condition->forAll(pFactCallback, pIsWrappingExpressionNegated, pIgnoreValue, pOnlyMandatoryFacts);
  return ContinueOrBreak::CONTINUE;
}


bool ConditionForall::findConditionCandidateFromFactFromEffect(
    const std::function<bool (const FactOptional&, const Fact*, const Fact*)>& pDoesConditionFactMatchFactFromEffect,
    const WorldState& pWorldState,
    const SetOfEntities& pConstants,
    const SetOfEntities& pObjects,
    const Fact& pFactFromEffect,
    const ParameterValuesWithConstraints& pFactFromEffectParameters,
    const ParameterValuesWithConstraints* pFactFromEffectTmpParametersPtr,
    const ParameterValuesWithConstraints& pConditionParametersToPossibleArguments,
    bool pIsWrappingExpressionNegated) const
{
  if (condition)
  {
    const auto& factAccessorsToFacts = pWorldState.factsMapping();
    ParameterValuesWithConstraints localParamToValue{{parameter, {}}};
    if (pIsWrappingExpressionNegated)
      _extractParamForNotLoopRec(localParamToValue, pConditionParametersToPossibleArguments, *condition, factAccessorsToFacts, pFactFromEffect, parameter);
    auto parameters = pConditionParametersToPossibleArguments;
    parameters[parameter];

    return condition->findConditionCandidateFromFactFromEffect([&](const FactOptional& pConditionFact, const Fact* pOtherPatternPtr, const Fact* pOtherInstancePtr) {
      bool res = false;
      auto& values = localParamToValue[parameter];

      while (true)
      {
        auto factToConsider = pConditionFact;
        factToConsider.fact.replaceArguments(localParamToValue);
        res = pDoesConditionFactMatchFactFromEffect(factToConsider, pOtherPatternPtr, pOtherInstancePtr) || res;
        if (values.empty())
          break;
        values.erase(values.begin());
      }

      return res == !pIsWrappingExpressionNegated;
    }, pWorldState, pConstants, pObjects, pFactFromEffect, pFactFromEffectParameters, pFactFromEffectTmpParametersPtr,
    parameters, pIsWrappingExpressionNegated);
  }
  return pIsWrappingExpressionNegated;
}


bool ConditionForall::isTrue(const WorldState& pWorldState,
                             const SetOfEntities& pConstants,
                             const SetOfEntities& pObjects,
                             const std::set<Fact>& pPunctualFacts,
                             const std::set<Fact>& pRemovedFacts,
                             ParameterValuesWithConstraints* pConditionParametersToPossibleArguments,
                             bool pIsWrappingExpressionNegated) const
{
  EntitiesWithParamConstaints entities;
  if (parameter.type)
    entities = typeToEntities(*parameter.type, pConstants, pObjects);
  if (entities.empty())
    return !pIsWrappingExpressionNegated;

  if (condition)
  {
    ParameterValuesWithConstraints localParamToValue{{parameter, {}}};
    return _isTrueRec(localParamToValue, pConditionParametersToPossibleArguments, true,
                      *condition, pWorldState, pPunctualFacts, pRemovedFacts,
                      entities) == !pIsWrappingExpressionNegated;
  }
  return !pIsWrappingExpressionNegated;
}



bool ConditionForall::operator==(const Condition& pOther) const
{
  auto* otherExistsPtr = pOther.fcExistsPtr();
  return otherExistsPtr != nullptr &&
      parameter == otherExistsPtr->parameter &&
      _areEqual(condition, otherExistsPtr->condition);
}

std::unique_ptr<Condition> ConditionForall::clone(const std::map<Parameter, Entity>* pConditionParametersToArgumentPtr,
                                                  bool pInvert,
                                                  const SetOfDerivedPredicates* pDerivedPredicatesPtr) const
{
  auto res = std::make_unique<ConditionForall>(
        parameter,
        condition ? condition->clone(pConditionParametersToArgumentPtr, false, pDerivedPredicatesPtr) : std::unique_ptr<Condition>());
  if (pInvert)
    return std::make_unique<ConditionNot>(std::move(res));
  return res;
}


bool ConditionForall::hasAContradictionWith(const std::set<FactOptional>& pFactsOpt,
                                            bool pIsWrappingExpressionNegated,
                                            std::list<Parameter>* pParametersPtr) const
{
  if (!condition)
    return false;
  auto contextParameters = addParameter(pParametersPtr, parameter);

  auto* factOfConditionPtr = condition->fcFactPtr();
  if (factOfConditionPtr != nullptr)
  {
    return factOfConditionPtr->factOptional.hasAContradictionWith(pFactsOpt, &contextParameters, pIsWrappingExpressionNegated);
  }

  auto* nodeOfConditionPtr = condition->fcNodePtr();
  if (nodeOfConditionPtr != nullptr &&
      nodeOfConditionPtr->leftOperand && nodeOfConditionPtr->rightOperand)
  {
    if (nodeOfConditionPtr->nodeType == ConditionNodeType::EQUALITY ||
        nodeOfConditionPtr->nodeType == ConditionNodeType::SUPERIOR || nodeOfConditionPtr->nodeType == ConditionNodeType::SUPERIOR_OR_EQUAL ||
        nodeOfConditionPtr->nodeType == ConditionNodeType::INFERIOR || nodeOfConditionPtr->nodeType == ConditionNodeType::INFERIOR_OR_EQUAL)
    {
      for (auto& currFactOpt : pFactsOpt)
        if (factOfConditionPtr->factOptional.fact.areEqualWithoutArgsAndValueConsideration(currFactOpt.fact, &contextParameters))
          return true;
      return false;
    }

    if (nodeOfConditionPtr->nodeType == ConditionNodeType::AND ||
        nodeOfConditionPtr->nodeType == ConditionNodeType::OR ||
        nodeOfConditionPtr->nodeType == ConditionNodeType::IMPLY)
    {
      return condition->hasAContradictionWith(pFactsOpt, pIsWrappingExpressionNegated, &contextParameters);
    }
  }
  return true;
}




std::string ConditionNot::toStr(const std::function<std::string (const Fact&)>* pFactWriterPtr,
                                bool pPrintAnyValue) const
{
  std::string conditionStr;
  if (condition)
    conditionStr = condition->toStr(pFactWriterPtr, pPrintAnyValue);
  if (condition->fcExistsPtr() != nullptr)
    return "!" + conditionStr;
  return "!(" + conditionStr + ")";
}

ConditionNot::ConditionNot(std::unique_ptr<Condition> pCondition)
  : Condition(),
    condition(std::move(pCondition))
{
}

bool ConditionNot::hasFact(const Fact& pFact) const
{
  return condition && condition->hasFact(pFact);
}

bool ConditionNot::hasEntity(const std::string& pEntityId) const
{
  return condition && condition->hasEntity(pEntityId);
}

ContinueOrBreak ConditionNot::forAll(const std::function<ContinueOrBreak (const FactOptional&, bool)>& pFactCallback,
                                     bool pIsWrappingExpressionNegated,
                                     bool pIgnoreValue,
                                     bool pOnlyMandatoryFacts) const
{

  if (condition)
    return condition->forAll(pFactCallback, !pIsWrappingExpressionNegated, pIgnoreValue, pOnlyMandatoryFacts);
  return ContinueOrBreak::CONTINUE;
}


bool ConditionNot::findConditionCandidateFromFactFromEffect(
    const std::function<bool (const FactOptional&, const Fact*, const Fact*)>& pDoesConditionFactMatchFactFromEffect,
    const WorldState& pWorldState,
    const SetOfEntities& pConstants,
    const SetOfEntities& pObjects,
    const Fact& pFactFromEffect,
    const ParameterValuesWithConstraints& pFactFromEffectParameters,
    const ParameterValuesWithConstraints* pFactFromEffectTmpParametersPtr,
    const ParameterValuesWithConstraints& pConditionParametersToPossibleArguments,
    bool pIsWrappingExpressionNegated) const
{
  if (condition)
    return condition->findConditionCandidateFromFactFromEffect(pDoesConditionFactMatchFactFromEffect, pWorldState, pConstants, pObjects,
                                                               pFactFromEffect, pFactFromEffectParameters, pFactFromEffectTmpParametersPtr,
                                                               pConditionParametersToPossibleArguments, !pIsWrappingExpressionNegated);
  return pIsWrappingExpressionNegated;
}


bool ConditionNot::isTrue(const WorldState& pWorldState,
                          const SetOfEntities& pConstants,
                          const SetOfEntities& pObjects,
                          const std::set<Fact>& pPunctualFacts,
                          const std::set<Fact>& pRemovedFacts,
                          ParameterValuesWithConstraints* pConditionParametersToPossibleArguments,
                          bool pIsWrappingExpressionNegated) const
{
  if (condition)
    return condition->isTrue(pWorldState, pConstants, pObjects, pPunctualFacts, pRemovedFacts, pConditionParametersToPossibleArguments, !pIsWrappingExpressionNegated);
  return !pIsWrappingExpressionNegated;
}


bool ConditionNot::operator==(const Condition& pOther) const
{
  auto* otherNotPtr = pOther.fcNotPtr();
  return otherNotPtr != nullptr &&
      _areEqual(condition, otherNotPtr->condition);
}

std::unique_ptr<Condition> ConditionNot::clone(const std::map<Parameter, Entity>* pConditionParametersToArgumentPtr,
                                               bool pInvert,
                                               const SetOfDerivedPredicates* pDerivedPredicatesPtr) const
{
  if (pInvert)
    return condition ? condition->clone(pConditionParametersToArgumentPtr, false, pDerivedPredicatesPtr) : std::unique_ptr<Condition>();
  return std::make_unique<ConditionNot>(condition ? condition->clone(pConditionParametersToArgumentPtr, pInvert, pDerivedPredicatesPtr) : std::unique_ptr<Condition>());
}


bool ConditionNot::hasAContradictionWith(const std::set<FactOptional>& pFactsOpt,
                                         bool pIsWrappingExpressionNegated,
                                         std::list<Parameter>* pParametersPtr) const
{
  return condition->hasAContradictionWith(pFactsOpt, !pIsWrappingExpressionNegated, pParametersPtr);
}


ConditionFact::ConditionFact(const FactOptional& pFactOptional)
  : Condition(),
    factOptional(pFactOptional)
{
}

bool ConditionFact::hasFact(const Fact& pFact) const
{
  return factOptional.fact == pFact;
}

bool ConditionFact::hasEntity(const std::string& pEntityId) const
{
  return factOptional.fact.hasEntity(pEntityId);
}


ContinueOrBreak ConditionFact::forAll(const std::function<ContinueOrBreak (const FactOptional&, bool)>& pFactCallback,
                                      bool pIsWrappingExpressionNegated,
                                      bool pIgnoreValue,
                                      bool) const
{
  if (!pIsWrappingExpressionNegated)
    return pFactCallback(factOptional, pIgnoreValue);

  auto factOptionalCopied = factOptional;
  factOptionalCopied.isFactNegated = !factOptionalCopied.isFactNegated;
  return pFactCallback(factOptionalCopied, pIgnoreValue);
}


bool ConditionFact::findConditionCandidateFromFactFromEffect(
    const std::function<bool (const FactOptional&, const Fact*, const Fact*)>& pDoesConditionFactMatchFactFromEffect,
    const WorldState&,
    const SetOfEntities&,
    const SetOfEntities&,
    const Fact&,
    const ParameterValuesWithConstraints&,
    const ParameterValuesWithConstraints*,
    const ParameterValuesWithConstraints&,
    bool pIsWrappingExpressionNegated) const
{
  bool res = pDoesConditionFactMatchFactFromEffect(factOptional, nullptr, nullptr);
  if (pIsWrappingExpressionNegated)
    return !res;
  return res;
}


bool ConditionFact::isTrue(const WorldState& pWorldState,
                           const SetOfEntities& pConstants,
                           const SetOfEntities& pObjects,
                           const std::set<Fact>& pPunctualFacts,
                           const std::set<Fact>& pRemovedFacts,
                           ParameterValuesWithConstraints* pConditionParametersToPossibleArguments,
                           bool pIsWrappingExpressionNegated) const
{
  bool res = pWorldState.isOptionalFactSatisfiedInASpecificContext(factOptional, pPunctualFacts, pRemovedFacts, pConditionParametersToPossibleArguments, nullptr);
  if (!pIsWrappingExpressionNegated)
    return res;
  return !res;
}

bool ConditionFact::operator==(const Condition& pOther) const
{
  auto* otherFactPtr = pOther.fcFactPtr();
  return otherFactPtr != nullptr &&
      factOptional == otherFactPtr->factOptional;
}

std::optional<Entity> ConditionFact::getValue(const SetOfFacts& pSetOfFact) const
{
  return pSetOfFact.getFluentValue(factOptional.fact);
}

std::unique_ptr<Condition> ConditionFact::clone(const std::map<Parameter, Entity>* pConditionParametersToArgumentPtr,
                                                bool pInvert,
                                                const SetOfDerivedPredicates* pDerivedPredicatesPtr) const
{
  if (pDerivedPredicatesPtr != nullptr)
  {
    auto condition = pDerivedPredicatesPtr->optFactToConditionPtr(factOptional);
    if (condition)
      return condition;
  }

  auto res = std::make_unique<ConditionFact>(factOptional);
  if (pConditionParametersToArgumentPtr != nullptr)
    res->factOptional.fact.replaceArguments(*pConditionParametersToArgumentPtr);
  if (pInvert)
    res->factOptional.isFactNegated = !res->factOptional.isFactNegated;
  return res;
}

bool ConditionFact::hasAContradictionWith(const std::set<FactOptional>& pFactsOpt,
                                          bool pIsWrappingExpressionNegated,
                                          std::list<Parameter>* pParametersPtr) const
{
  return factOptional.hasAContradictionWith(pFactsOpt, pParametersPtr, pIsWrappingExpressionNegated);
}


std::string ConditionNumber::toStr(const std::function<std::string (const Fact&)>*,
                                   bool) const
{
  return numberToString(nb);
}

ConditionNumber::ConditionNumber(const Number& pNb)
  : Condition(),
    nb(pNb)
{
}

bool ConditionNumber::operator==(const Condition& pOther) const
{
  auto* otherNbPtr = pOther.fcNbPtr();
  return otherNbPtr != nullptr &&
      nb == otherNbPtr->nb;
}

std::optional<Entity> ConditionNumber::getValue(const SetOfFacts&) const
{
  return Entity::createNumberEntity(toStr(nullptr, true));
}

std::unique_ptr<Condition> ConditionNumber::clone(const std::map<Parameter, Entity>*,
                                                  bool,
                                                  const SetOfDerivedPredicates*) const
{
  return std::make_unique<ConditionNumber>(nb);
}


} // !ogp
