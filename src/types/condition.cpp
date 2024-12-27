#include <orderedgoalsplanner/types/condition.hpp>
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

bool _forEachValueUntil(const std::function<bool (const Entity&)>& pValueCallback,
                        bool pUntilValue,
                        const Condition& pCondition,
                        const WorldState& pWorldState,
                        const std::map<Parameter, std::set<Entity>>* pParametersPtr)
{
  const auto& setOfFacts = pWorldState.factsMapping();
  if (pParametersPtr == nullptr || pParametersPtr->empty())
  {
    auto valueOpt = pCondition.getValue(setOfFacts);
    if (valueOpt)
      return pValueCallback(*valueOpt);
  }

  std::list<std::map<Parameter, Entity>> paramPossibilities;
  unfoldMapWithSet(paramPossibilities, *pParametersPtr);
  for (auto& currParamPoss : paramPossibilities)
  {
    auto condToExtractValue = pCondition.clone(&currParamPoss);
    auto valueOpt = condToExtractValue->getValue(setOfFacts);
    if (valueOpt && pValueCallback(*valueOpt) == pUntilValue)
      return pUntilValue;
  }

  auto* factCondPtr = pCondition.fcFactPtr();
  if (factCondPtr != nullptr &&
      (!factCondPtr->factOptional.fact.value() || factCondPtr->factOptional.fact.value()->isAnyEntity()))
  {
    pWorldState.iterateOnMatchingFactsWithoutValueConsideration(
          [&pValueCallback](const Fact& pFact) {
      if (pFact.value())
        pValueCallback(*pFact.value());
      return false;
    },
    factCondPtr->factOptional.fact, // pFact
    *pParametersPtr); // Parameters to consider as any value of set is empty, otherwise it is a filter
  }

  return !pUntilValue;
}


void _forEach(const std::function<void (const Entity&, const Fact*)>& pValueCallback,
              const Condition& pCondition,
              const WorldState& pWorldState,
              const std::map<Parameter, std::set<Entity>>* pParametersPtr)
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


bool _isTrueRec(std::map<Parameter, std::set<Entity>>& pLocalParamToValue,
                std::map<Parameter, std::set<Entity>>* pConditionParametersToPossibleArguments,
                bool pMustBeTrueForAllParameters,
                const Condition& pCondition,
                const WorldState& pWorldState,
                const std::set<Fact>& pPunctualFacts,
                const std::set<Fact>& pRemovedFacts,
                const std::set<Entity>& pAllEntitiesForParam)
{
  auto* factOfConditionPtr = pCondition.fcFactPtr();
  if (factOfConditionPtr != nullptr)
  {
    bool res = pWorldState.isOptionalFactSatisfiedInASpecificContext(factOfConditionPtr->factOptional, pPunctualFacts, pRemovedFacts, pMustBeTrueForAllParameters,
                                                                     pConditionParametersToPossibleArguments, &pLocalParamToValue);
    if (pMustBeTrueForAllParameters && !factOfConditionPtr->factOptional.isFactNegated)
    {
      if (res)
        for (const auto& currLocalParamToValue : pLocalParamToValue)
          if (currLocalParamToValue.second != pAllEntitiesForParam)
            return false;
    }
    if (!pMustBeTrueForAllParameters && factOfConditionPtr->factOptional.isFactNegated)
    {
      if (!res)
        for (const auto& currLocalParamToValue : pLocalParamToValue)
          if (currLocalParamToValue.second != pAllEntitiesForParam)
            return true;
    }
    return res;
  }

  auto* nodeOfConditionPtr = pCondition.fcNodePtr();
  if (nodeOfConditionPtr != nullptr &&
      nodeOfConditionPtr->leftOperand && nodeOfConditionPtr->rightOperand)
  {
    if (nodeOfConditionPtr->nodeType == ConditionNodeType::AND)
      return _isTrueRec(pLocalParamToValue, pConditionParametersToPossibleArguments, pMustBeTrueForAllParameters,
                        *nodeOfConditionPtr->leftOperand, pWorldState, pPunctualFacts, pRemovedFacts, pAllEntitiesForParam) &&
          _isTrueRec(pLocalParamToValue, pConditionParametersToPossibleArguments, pMustBeTrueForAllParameters,
                     *nodeOfConditionPtr->rightOperand, pWorldState, pPunctualFacts, pRemovedFacts, pAllEntitiesForParam);
    if (nodeOfConditionPtr->nodeType == ConditionNodeType::OR)
      return _isTrueRec(pLocalParamToValue, pConditionParametersToPossibleArguments, pMustBeTrueForAllParameters,
                        *nodeOfConditionPtr->leftOperand, pWorldState, pPunctualFacts, pRemovedFacts, pAllEntitiesForParam) ||
          _isTrueRec(pLocalParamToValue, pConditionParametersToPossibleArguments, pMustBeTrueForAllParameters,
                     *nodeOfConditionPtr->rightOperand, pWorldState, pPunctualFacts, pRemovedFacts, pAllEntitiesForParam);

    if (nodeOfConditionPtr->nodeType == ConditionNodeType::IMPLY)
    {
      if (_isTrueRec(pLocalParamToValue, pConditionParametersToPossibleArguments, pMustBeTrueForAllParameters,
                     *nodeOfConditionPtr->leftOperand, pWorldState, pPunctualFacts, pRemovedFacts, pAllEntitiesForParam))
        return _isTrueRec(pLocalParamToValue, pConditionParametersToPossibleArguments, pMustBeTrueForAllParameters,
                          *nodeOfConditionPtr->rightOperand, pWorldState, pPunctualFacts, pRemovedFacts, pAllEntitiesForParam);
      return true;
    }

    if (nodeOfConditionPtr->nodeType == ConditionNodeType::EQUALITY)
    {
      std::map<Entity, std::map<Parameter, std::set<Entity>>> leftOpPossibleValuesToParams;
      auto* leftOpFactPtr = nodeOfConditionPtr->leftOperand->fcFactPtr();
      if (leftOpFactPtr != nullptr)
      {
        auto& leftOpFact = *leftOpFactPtr;

        pWorldState.iterateOnMatchingFactsWithoutValueConsideration([&](const Fact& pFact){
          if (pFact.value())
          {
            auto& newParams = leftOpPossibleValuesToParams[*pFact.value()];
            if (pConditionParametersToPossibleArguments != nullptr)
            {
              for (auto& currArg : *pConditionParametersToPossibleArguments)
              {
                auto argValue = leftOpFact.factOptional.fact.tryToExtractArgumentFromExampleWithoutValueConsideration(currArg.first, pFact);
                if (argValue)
                  newParams[currArg.first].insert(*argValue);
              }
            }
          }
          return false;
        }, leftOpFact.factOptional.fact, pLocalParamToValue, pConditionParametersToPossibleArguments);
      }

      bool res = false;
      auto* rightOpFactPtr = nodeOfConditionPtr->rightOperand->fcFactPtr();
      if (rightOpFactPtr != nullptr)
      {
        auto& rightOpFact = *rightOpFactPtr;

        std::map<Parameter, std::set<Entity>> newParameters;
        pWorldState.iterateOnMatchingFactsWithoutValueConsideration([&](const Fact& pFact){
          if (pFact.value())
          {
            auto itToLeftPoss = leftOpPossibleValuesToParams.find(*pFact.value());
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
                    auto argValue = rightOpFact.factOptional.fact.tryToExtractArgumentFromExampleWithoutValueConsideration(currArg.first, pFact);
                    if (argValue)
                      newParameters[currArg.first].insert(*argValue);
                  }
                }
              }
              res = true;
            }
          }
          return res && pConditionParametersToPossibleArguments == nullptr;
        }, rightOpFact.factOptional.fact, pLocalParamToValue, pConditionParametersToPossibleArguments);

        if (pConditionParametersToPossibleArguments != nullptr)
          applyNewParams(*pConditionParametersToPossibleArguments, newParameters);
      }

      return res;
    }
  }
  return false;
}


void _existsExtractPossRec(std::map<Parameter, std::set<Entity>>& pLocalParamToValue,
                           const std::map<Parameter, std::set<Entity>>& pConditionParametersToPossibleArguments,
                           const Condition& pCondition,
                           const SetOfFacts& pFacts,
                           const Fact& pFactFromEffect,
                           const Parameter& pParameter,
                           bool pIsNegated)
{
  auto* factOfConditionPtr = pCondition.fcFactPtr();
  if (factOfConditionPtr != nullptr)
  {
    const auto& factOfConditionOpt = factOfConditionPtr->factOptional;
    if (factOfConditionOpt.isFactNegated != pIsNegated ||
        !factOfConditionOpt.fact.areEqualWithoutAnArgConsideration(pFactFromEffect, pParameter.name))
    {
      std::map<Parameter, std::set<Entity>> newParameters;
      factOfConditionOpt.fact.isInOtherFactsMap(pFacts, &newParameters, false, &pConditionParametersToPossibleArguments, &pLocalParamToValue);
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
    _existsExtractPossRec(pLocalParamToValue, pConditionParametersToPossibleArguments, *nodeOfConditionPtr->leftOperand, pFacts, pFactFromEffect, pParameter, pIsNegated);
    _existsExtractPossRec(pLocalParamToValue, pConditionParametersToPossibleArguments, *nodeOfConditionPtr->rightOperand, pFacts, pFactFromEffect, pParameter, pIsNegated);
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
    const std::function<bool (const FactOptional&)>& pDoesConditionFactMatchFactFromEffect,
    const WorldState& pWorldState,
    const SetOfEntities& pConstants,
    const SetOfEntities& pObjects,
    const Fact& pFactFromEffect,
    const std::map<Parameter, std::set<Entity>>& pFactFromEffectParameters,
    const std::map<Parameter, std::set<Entity>>* pFactFromEffectTmpParametersPtr,
    const std::map<Parameter, std::set<Entity>>& pConditionParametersToPossibleArguments,
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
                [&](const Entity& pValue)
          {
            auto factToCheck = leftFact.factOptional.fact;
            factToCheck.setValue(pValue);
            return pDoesConditionFactMatchFactFromEffect(FactOptional(factToCheck));
          }, true, *rightOperand, pWorldState, &pConditionParametersToPossibleArguments);
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
                  [&](const Entity& pValue)
            {
              auto factToCheck = rightFact.factOptional.fact;
              factToCheck.setValue(pValue);
              return pDoesConditionFactMatchFactFromEffect(FactOptional(factToCheck));
            }, true, *leftOperand, pWorldState, &pConditionParametersToPossibleArguments);
          }
        }
      }
      else if (nodeType == ConditionNodeType::SUPERIOR || nodeType == ConditionNodeType::SUPERIOR_OR_EQUAL ||
               nodeType == ConditionNodeType::INFERIOR || nodeType == ConditionNodeType::INFERIOR_OR_EQUAL)
      {
         return pDoesConditionFactMatchFactFromEffect(leftFact.factOptional);
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
                           std::map<Parameter, std::set<Entity>>* pConditionParametersToPossibleArguments,
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
        std::map<Parameter, std::set<Entity>> newParameters;
        _forEach([&](const Entity& pValue, const Fact* pFromFactPtr)
        {
          auto factToCheck = leftFactPtr->factOptional.fact;
          factToCheck.setValue(pValue);
          bool subRes = false;
          if (factToCheck.isPunctual())
            subRes = pPunctualFacts.count(factToCheck) != 0;
          else
            subRes = factToCheck.isInOtherFactsMap(factAccessorsToFacts, &newParameters, false, pConditionParametersToPossibleArguments);

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
                    currArg.second.insert(*value);
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
    const std::function<bool (const FactOptional&)>& pDoesConditionFactMatchFactFromEffect,
    const WorldState& pWorldState,
    const SetOfEntities& pConstants,
    const SetOfEntities& pObjects,
    const Fact& pFactFromEffect,
    const std::map<Parameter, std::set<Entity>>& pFactFromEffectParameters,
    const std::map<Parameter, std::set<Entity>>* pFactFromEffectTmpParametersPtr,
    const std::map<Parameter, std::set<Entity>>& pConditionParametersToPossibleArguments,
    bool pIsWrappingExpressionNegated) const
{
  if (condition)
  {
    const auto& factAccessorsToFacts = pWorldState.factsMapping();
    std::map<Parameter, std::set<Entity>> localParamToValue{{parameter, {}}};
    _existsExtractPossRec(localParamToValue, pConditionParametersToPossibleArguments, *condition, factAccessorsToFacts, pFactFromEffect, parameter, pIsWrappingExpressionNegated);
    auto parameters = pConditionParametersToPossibleArguments;
    parameters[parameter];

    return condition->findConditionCandidateFromFactFromEffect([&](const FactOptional& pConditionFact) {
      auto factToConsider = pConditionFact;
      factToConsider.fact.replaceArguments(localParamToValue);
      return pDoesConditionFactMatchFactFromEffect(factToConsider) == !pIsWrappingExpressionNegated;
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
                             std::map<Parameter, std::set<Entity>>* pConditionParametersToPossibleArguments,
                             bool pIsWrappingExpressionNegated) const
{
  std::set<Entity> entities;
  if (parameter.type)
    entities = typeToEntities(*parameter.type, pConstants, pObjects);
  if (entities.empty())
      return pIsWrappingExpressionNegated;

  if (condition)
  {
    std::map<Parameter, std::set<Entity>> localParamToValue{{parameter, {}}};
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
    const std::function<bool (const FactOptional&)>& pDoesConditionFactMatchFactFromEffect,
    const WorldState& pWorldState,
    const SetOfEntities& pConstants,
    const SetOfEntities& pObjects,
    const Fact& pFactFromEffect,
    const std::map<Parameter, std::set<Entity>>& pFactFromEffectParameters,
    const std::map<Parameter, std::set<Entity>>* pFactFromEffectTmpParametersPtr,
    const std::map<Parameter, std::set<Entity>>& pConditionParametersToPossibleArguments,
    bool pIsWrappingExpressionNegated) const
{
  if (condition)
  {
    const auto& factAccessorsToFacts = pWorldState.factsMapping();
    std::map<Parameter, std::set<Entity>> localParamToValue{{parameter, {}}};
    _existsExtractPossRec(localParamToValue, pConditionParametersToPossibleArguments, *condition, factAccessorsToFacts, pFactFromEffect, parameter, pIsWrappingExpressionNegated);
    auto parameters = pConditionParametersToPossibleArguments;
    parameters[parameter];

    return condition->findConditionCandidateFromFactFromEffect([&](const FactOptional& pConditionFact) {
      bool res = false;
      auto& values = localParamToValue[parameter];

      while (true)
      {
        auto factToConsider = pConditionFact;
        factToConsider.fact.replaceArguments(localParamToValue);
        res = pDoesConditionFactMatchFactFromEffect(factToConsider) || res;
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
                             std::map<Parameter, std::set<Entity>>* pConditionParametersToPossibleArguments,
                             bool pIsWrappingExpressionNegated) const
{
  std::set<Entity> entities;
  if (parameter.type)
    entities = typeToEntities(*parameter.type, pConstants, pObjects);
  if (entities.empty())
    return !pIsWrappingExpressionNegated;

  if (condition)
  {
    std::map<Parameter, std::set<Entity>> localParamToValue{{parameter, {}}};
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
    const std::function<bool (const FactOptional&)>& pDoesConditionFactMatchFactFromEffect,
    const WorldState& pWorldState,
    const SetOfEntities& pConstants,
    const SetOfEntities& pObjects,
    const Fact& pFactFromEffect,
    const std::map<Parameter, std::set<Entity>>& pFactFromEffectParameters,
    const std::map<Parameter, std::set<Entity>>* pFactFromEffectTmpParametersPtr,
    const std::map<Parameter, std::set<Entity>>& pConditionParametersToPossibleArguments,
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
                          std::map<Parameter, std::set<Entity>>* pConditionParametersToPossibleArguments,
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
    const std::function<bool (const FactOptional&)>& pDoesConditionFactMatchFactFromEffect,
    const WorldState&,
    const SetOfEntities&,
    const SetOfEntities&,
    const Fact&,
    const std::map<Parameter, std::set<Entity>>&,
    const std::map<Parameter, std::set<Entity>>*,
    const std::map<Parameter, std::set<Entity>>&,
    bool pIsWrappingExpressionNegated) const
{
  bool res = pDoesConditionFactMatchFactFromEffect(factOptional);
  if (pIsWrappingExpressionNegated)
    return !res;
  return res;
}


bool ConditionFact::isTrue(const WorldState& pWorldState,
                           const SetOfEntities& pConstants,
                           const SetOfEntities& pObjects,
                           const std::set<Fact>& pPunctualFacts,
                           const std::set<Fact>& pRemovedFacts,
                           std::map<Parameter, std::set<Entity>>* pConditionParametersToPossibleArguments,
                           bool pIsWrappingExpressionNegated) const
{
  bool res = pWorldState.isOptionalFactSatisfiedInASpecificContext(factOptional, pPunctualFacts, pRemovedFacts, false, pConditionParametersToPossibleArguments, nullptr);
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
