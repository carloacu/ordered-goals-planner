#include "worldstatemodificationprivate.hpp"
#include <orderedgoalsplanner/types/domain.hpp>
#include <orderedgoalsplanner/types/ontology.hpp>
#include <orderedgoalsplanner/types/worldstate.hpp>
#include "expressionParsed.hpp"
#include <orderedgoalsplanner/util/util.hpp>

namespace ogp
{
namespace
{
bool _areEqual(
    const std::unique_ptr<WorldStateModification>& pCond1,
    const std::unique_ptr<WorldStateModification>& pCond2)
{
  if (!pCond1 && !pCond2)
    return true;
  if (pCond1 && pCond2)
    return *pCond1 == *pCond2;
  return false;
}


bool _isOkWithLocalParameters(const ParameterValuesWithConstraints& pLocalParameterToFind,
                              ParameterValuesWithConstraints& pParametersToFill,
                              const WorldStateModification& pWModif,
                              const WorldState& pWorldState,
                              ParameterValuesWithConstraints& pParametersToModifyInPlace)
{
  if (!pParametersToFill.empty() &&
      !pParametersToFill.begin()->second.empty())
  {
    bool res = false;
    const auto* wSMFPtr = dynamic_cast<const WorldStateModificationFact*>(&pWModif);
    if (wSMFPtr != nullptr)
    {
      auto& parameterPossibilities = pParametersToFill.begin()->second;

      ParameterValuesWithConstraints newParameters;
      while (!parameterPossibilities.empty())
      {
        auto factWithValueToAssign = wSMFPtr->factOptional.fact;
        factWithValueToAssign.replaceArguments(pLocalParameterToFind);
        auto itBeginOfParamPoss = parameterPossibilities.begin();
        factWithValueToAssign.setValue(itBeginOfParamPoss->first);

        const auto& factAccessorsToFacts = pWorldState.factsMapping();

        if (factWithValueToAssign.isInOtherFactsMap(factAccessorsToFacts, &newParameters, &pParametersToModifyInPlace))
          res = true;
        parameterPossibilities.erase(itBeginOfParamPoss);
      }

      if (res)
        applyNewParams(pParametersToModifyInPlace, newParameters);
    }

    return res;
  }
  return true;
}

}


std::string WorldStateModificationNode::toStr(bool pPrintAnyValue) const
{
  bool printAnyValue = pPrintAnyValue && printAnyValueFor(nodeType);

  std::string leftOperandStr;
  if (leftOperand)
    leftOperandStr = leftOperand->toStr(printAnyValue);
  std::string rightOperandStr;
  bool isRightOperandAFactWithoutParameter = false;
  if (rightOperand)
  {
    const auto* rightOperandFactPtr = toWmFact(*rightOperand);
    if (rightOperandFactPtr != nullptr && rightOperandFactPtr->factOptional.fact.arguments().empty() &&
        !rightOperandFactPtr->factOptional.fact.value())
      isRightOperandAFactWithoutParameter = true;
    rightOperandStr = rightOperand->toStr(printAnyValue);
  }

  switch (nodeType)
  {
  case WorldStateModificationNodeType::AND:
    return leftOperandStr + " & " + rightOperandStr;
  case WorldStateModificationNodeType::ASSIGN:
  {
    if (isRightOperandAFactWithoutParameter)
      rightOperandStr += "()"; // To significate it is a fact
    return "assign(" + leftOperandStr + ", " + rightOperandStr + ")";
  }
  case WorldStateModificationNodeType::FOR_ALL:
    if (!parameterOpt)
      throw std::runtime_error("for all statement without a parameter detected");
    if (leftOperandStr.empty())
      return "forall(" + parameterOpt->toStr() + ", " + rightOperandStr + ")";
    return "forall(" + parameterOpt->toStr() + ", when(" + leftOperandStr + ", " + rightOperandStr + "))";
  case WorldStateModificationNodeType::INCREASE:
    return "increase(" + leftOperandStr + ", " + rightOperandStr + ")";
  case WorldStateModificationNodeType::DECREASE:
    return "decrease(" + leftOperandStr + ", " + rightOperandStr + ")";
  case WorldStateModificationNodeType::MULTIPLY:
    return leftOperandStr + " * " + rightOperandStr;
  case WorldStateModificationNodeType::PLUS:
    return leftOperandStr + " + " + rightOperandStr;
  case WorldStateModificationNodeType::MINUS:
    return leftOperandStr + " - " + rightOperandStr;
  case WorldStateModificationNodeType::WHEN:
    return "when(" + leftOperandStr + ", " + rightOperandStr + ")";
  }
  return "";
}


void WorldStateModificationNode::forAll(const std::function<void (const FactOptional&)>& pFactCallback,
                                        const SetOfFacts& pSetOfFact) const
{
  if (nodeType == WorldStateModificationNodeType::AND)
  {
    if (leftOperand)
      leftOperand->forAll(pFactCallback, pSetOfFact);
    if (rightOperand)
      rightOperand->forAll(pFactCallback, pSetOfFact);
  }
  else if (nodeType == WorldStateModificationNodeType::ASSIGN && leftOperand && rightOperand)
  {
    auto* leftFactPtr = toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional;
      factToCheck.fact.setValue(rightOperand->getValue(pSetOfFact));
      return pFactCallback(factToCheck);
    }
  }
  else if (nodeType == WorldStateModificationNodeType::FOR_ALL)
  {
    ParameterValuesWithConstraints parameters;
    _forAllInstruction(
          [&](const WorldStateModification& pWsModification)
    {
      pWsModification.forAll(pFactCallback, pSetOfFact);
    }, pSetOfFact, parameters);
  }
  else if (nodeType == WorldStateModificationNodeType::INCREASE && leftOperand && rightOperand)
  {
    auto* leftFactPtr = toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional;
      factToCheck.fact.setValue(plusIntOrStr(leftOperand->getValue(pSetOfFact), rightOperand->getValue(pSetOfFact)));
      return pFactCallback(factToCheck);
    }
  }
  else if (nodeType == WorldStateModificationNodeType::DECREASE && leftOperand && rightOperand)
  {
    auto* leftFactPtr = toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional;
      factToCheck.fact.setValue(minusIntOrStr(leftOperand->getValue(pSetOfFact), rightOperand->getValue(pSetOfFact)));
      return pFactCallback(factToCheck);
    }
  }
  else if (nodeType == WorldStateModificationNodeType::MULTIPLY && leftOperand && rightOperand)
  {
    auto* leftFactPtr = toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional;
      factToCheck.fact.setValue(multiplyNbOrStr(leftOperand->getValue(pSetOfFact), rightOperand->getValue(pSetOfFact)));
      return pFactCallback(factToCheck);
    }
  }
  else if (nodeType == WorldStateModificationNodeType::WHEN && leftOperand && rightOperand)
  {
    auto* leftFactPtr = toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional;
      if (pSetOfFact.hasFact(factToCheck.fact))
        rightOperand->forAll(pFactCallback, pSetOfFact);
    }
  }
}


ContinueOrBreak WorldStateModificationNode::forAllThatCanBeModified(const std::function<ContinueOrBreak (const FactOptional&)>& pFactCallback) const
{
  ContinueOrBreak res = ContinueOrBreak::CONTINUE;
  if (nodeType == WorldStateModificationNodeType::AND)
  {
    if (leftOperand)
      res = leftOperand->forAllThatCanBeModified(pFactCallback);
    if (rightOperand && res == ContinueOrBreak::CONTINUE)
      res = rightOperand->forAllThatCanBeModified(pFactCallback);
  }
  else if (nodeType == WorldStateModificationNodeType::ASSIGN && leftOperand)
  {
    auto* leftFactPtr = toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
      return pFactCallback(leftFactPtr->factOptional);
  }
  else if (nodeType == WorldStateModificationNodeType::FOR_ALL && rightOperand)
  {
    return rightOperand->forAllThatCanBeModified(pFactCallback);
  }
  else if ((nodeType == WorldStateModificationNodeType::INCREASE ||
            nodeType == WorldStateModificationNodeType::DECREASE ||
            nodeType == WorldStateModificationNodeType::MULTIPLY) && leftOperand)
  {
    auto* leftFactPtr = toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
      return pFactCallback(leftFactPtr->factOptional);
  }
  else if (nodeType == WorldStateModificationNodeType::WHEN && rightOperand)
  {
    return rightOperand->forAllThatCanBeModified(pFactCallback);
  }
  return res;
}


bool WorldStateModificationNode::canSatisfyObjective(const std::function<bool (const FactOptional&, ParameterValuesWithConstraints*, const std::function<bool (const ParameterValuesWithConstraints&)>&)>& pFactCallback,
                                                     ParameterValuesWithConstraints& pParameters,
                                                     const WorldState& pWorldState,
                                                     const std::string& pFromDeductionId) const
{
  const auto& setOfFacts = pWorldState.factsMapping();

  if (nodeType == WorldStateModificationNodeType::AND)
    return (leftOperand && leftOperand->canSatisfyObjective(pFactCallback, pParameters, pWorldState, pFromDeductionId)) ||
        (rightOperand && rightOperand->canSatisfyObjective(pFactCallback, pParameters, pWorldState, pFromDeductionId));

  if (nodeType == WorldStateModificationNodeType::ASSIGN && leftOperand && rightOperand)
  {
    auto* leftFactPtr = toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional;
      factToCheck.fact.setValue(rightOperand->getValue(setOfFacts));
      ParameterValuesWithConstraints localParameterToFind;

      if (!factToCheck.fact.value())
      {
        factToCheck.fact.setValue(Entity("??tmpValueFromSet_" + pFromDeductionId, factToCheck.fact.predicate.value));
        localParameterToFind[Parameter(factToCheck.fact.value()->value, factToCheck.fact.predicate.value)];
      }
      bool res = pFactCallback(factToCheck, &localParameterToFind, [&](const ParameterValuesWithConstraints& pLocalParameterToFind){
        return _isOkWithLocalParameters(pLocalParameterToFind, localParameterToFind, *rightOperand, pWorldState, pParameters);
      });
      return res;
    }
  }

  if (nodeType == WorldStateModificationNodeType::FOR_ALL)
  {
    bool res = false;
    _forAllInstruction(
          [&](const WorldStateModification& pWsModification)
    {
      if (!res)
        res = pWsModification.canSatisfyObjective(pFactCallback, pParameters, pWorldState, pFromDeductionId);
    }, setOfFacts, pParameters);
    return res;
  }

  if (nodeType == WorldStateModificationNodeType::INCREASE && leftOperand && rightOperand)
  {
    auto* leftFactPtr = toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional;
      factToCheck.fact.setValue(plusIntOrStr(leftOperand->getValue(setOfFacts), rightOperand->getValue(setOfFacts)));
      return pFactCallback(factToCheck, nullptr, [](const ParameterValuesWithConstraints&){ return true; });
    }
  }

  if (nodeType == WorldStateModificationNodeType::DECREASE && leftOperand && rightOperand)
  {
    auto* leftFactPtr = toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional;
      factToCheck.fact.setValue(minusIntOrStr(leftOperand->getValue(setOfFacts), rightOperand->getValue(setOfFacts)));
      return pFactCallback(factToCheck, nullptr, [](const ParameterValuesWithConstraints&){ return true; });
    }
  }

  if (nodeType == WorldStateModificationNodeType::MULTIPLY && leftOperand && rightOperand)
  {
    auto* leftFactPtr = toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional;
      factToCheck.fact.setValue(multiplyNbOrStr(leftOperand->getValue(setOfFacts), rightOperand->getValue(setOfFacts)));
      return pFactCallback(factToCheck, nullptr, [](const ParameterValuesWithConstraints&){ return true; });
    }
  }

  if (nodeType == WorldStateModificationNodeType::WHEN)
    return rightOperand && rightOperand->canSatisfyObjective(pFactCallback, pParameters, pWorldState, pFromDeductionId);

  return false;
}


bool WorldStateModificationNode::iterateOnSuccessions(const std::function<bool (const Successions&, const FactOptional&, ParameterValuesWithConstraints*, const std::function<bool (const ParameterValuesWithConstraints&)>&)>& pCallback,
                                                      ParameterValuesWithConstraints& pParameters,
                                                      const WorldState& pWorldState,
                                                      bool pCanSatisfyThisGoal,
                                                      const std::string& pFromDeductionId) const
{
  const auto& setOfFacts = pWorldState.factsMapping();

  if (nodeType == WorldStateModificationNodeType::AND)
    return (leftOperand && leftOperand->iterateOnSuccessions(pCallback, pParameters, pWorldState, pCanSatisfyThisGoal, pFromDeductionId)) ||
        (rightOperand && rightOperand->iterateOnSuccessions(pCallback, pParameters, pWorldState, pCanSatisfyThisGoal, pFromDeductionId));

  if (nodeType == WorldStateModificationNodeType::ASSIGN && leftOperand && rightOperand && (pCanSatisfyThisGoal || !_successions.empty()))
  {
    auto* leftFactPtr = toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional;
      factToCheck.fact.setValue(rightOperand->getValue(setOfFacts));
      ParameterValuesWithConstraints localParameterToFind;

      if (!factToCheck.fact.value())
      {
        factToCheck.fact.setValue(Entity("??tmpValueFromSet_" + pFromDeductionId, factToCheck.fact.predicate.value));
        localParameterToFind[Parameter(factToCheck.fact.value()->value, factToCheck.fact.predicate.value)];
      }
      bool res = pCallback(_successions, factToCheck, &localParameterToFind, [&](const ParameterValuesWithConstraints& pLocalParameterToFind){
        return _isOkWithLocalParameters(pLocalParameterToFind, localParameterToFind, *rightOperand, pWorldState, pParameters);
      });
      return res;
    }
  }

  if (nodeType == WorldStateModificationNodeType::FOR_ALL)
  {
    bool res = false;
    _forAllInstruction(
          [&](const WorldStateModification& pWsModification)
    {
      if (!res)
        res = pWsModification.iterateOnSuccessions(pCallback, pParameters, pWorldState, pCanSatisfyThisGoal, pFromDeductionId);
    }, setOfFacts, pParameters);
    return res;
  }

  if (nodeType == WorldStateModificationNodeType::INCREASE && leftOperand && rightOperand && (pCanSatisfyThisGoal || !_successions.empty()))
  {
    auto* leftFactPtr = toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional;
      factToCheck.fact.setValue(plusIntOrStr(leftOperand->getValue(setOfFacts), rightOperand->getValue(setOfFacts)));
      return pCallback(_successions, factToCheck, nullptr, [](const ParameterValuesWithConstraints&){ return true; });
    }
  }

  if (nodeType == WorldStateModificationNodeType::DECREASE && leftOperand && rightOperand && (pCanSatisfyThisGoal || !_successions.empty()))
  {
    auto* leftFactPtr = toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional;
      factToCheck.fact.setValue(minusIntOrStr(leftOperand->getValue(setOfFacts), rightOperand->getValue(setOfFacts)));
      return pCallback(_successions, factToCheck, nullptr, [](const ParameterValuesWithConstraints&){ return true; });
    }
  }

  if (nodeType == WorldStateModificationNodeType::MULTIPLY && leftOperand && rightOperand && (pCanSatisfyThisGoal || !_successions.empty()))
  {
    auto* leftFactPtr = toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      auto factToCheck = leftFactPtr->factOptional;
      factToCheck.fact.setValue(multiplyNbOrStr(leftOperand->getValue(setOfFacts), rightOperand->getValue(setOfFacts)));
      return pCallback(_successions, factToCheck, nullptr, [](const ParameterValuesWithConstraints&){ return true; });
    }
  }

  if (nodeType == WorldStateModificationNodeType::WHEN)
  {
    return rightOperand && rightOperand->iterateOnSuccessions(pCallback, pParameters, pWorldState, pCanSatisfyThisGoal, pFromDeductionId);
  }

  return false;
}



void WorldStateModificationNode::updateSuccesions(const Domain& pDomain,
                                                  const WorldStateModificationContainerId& pContainerId,
                                                  const std::set<FactOptional>& pOptionalFactsToIgnore)
{
  _successions.clear();

  if (nodeType == WorldStateModificationNodeType::AND)
  {
    if (leftOperand)
      leftOperand->updateSuccesions(pDomain, pContainerId, pOptionalFactsToIgnore);
    if (rightOperand)
      rightOperand->updateSuccesions(pDomain, pContainerId, pOptionalFactsToIgnore);
  }
  else if (nodeType == WorldStateModificationNodeType::ASSIGN ||
           nodeType == WorldStateModificationNodeType::INCREASE ||
           nodeType == WorldStateModificationNodeType::DECREASE ||
           nodeType == WorldStateModificationNodeType::MULTIPLY)
  {
    if (leftOperand)
    {
      auto* leftFactPtr = toWmFact(*leftOperand);
      if (leftFactPtr != nullptr)
        _successions.addSuccesionsOptFact(leftFactPtr->factOptional, pDomain, pContainerId, pOptionalFactsToIgnore);
    }
  }
  else if (nodeType == WorldStateModificationNodeType::FOR_ALL ||
           nodeType == WorldStateModificationNodeType::WHEN)
  {
    if (rightOperand)
      rightOperand->updateSuccesions(pDomain, pContainerId, pOptionalFactsToIgnore);
  }
}

void WorldStateModificationNode::removePossibleSuccession(const ActionId& pActionIdToRemove)
{
  _successions.actions.erase(pActionIdToRemove);

  if (nodeType == WorldStateModificationNodeType::AND)
  {
    if (leftOperand)
      leftOperand->removePossibleSuccession(pActionIdToRemove);
    if (rightOperand)
      rightOperand->removePossibleSuccession(pActionIdToRemove);
  }
  else if (nodeType == WorldStateModificationNodeType::ASSIGN ||
           nodeType == WorldStateModificationNodeType::INCREASE ||
           nodeType == WorldStateModificationNodeType::DECREASE ||
           nodeType == WorldStateModificationNodeType::MULTIPLY)
  {
    if (leftOperand)
      leftOperand->removePossibleSuccession(pActionIdToRemove);
  }
  else if (nodeType == WorldStateModificationNodeType::FOR_ALL ||
           nodeType == WorldStateModificationNodeType::WHEN)
  {
    if (rightOperand)
      rightOperand->removePossibleSuccession(pActionIdToRemove);
  }
}

void WorldStateModificationNode::getSuccesions(Successions& pSuccessions) const
{
  if (nodeType == WorldStateModificationNodeType::AND)
  {
    if (leftOperand)
      leftOperand->getSuccesions(pSuccessions);
    if (rightOperand)
      rightOperand->getSuccesions(pSuccessions);
  }
  else if (nodeType == WorldStateModificationNodeType::ASSIGN ||
           nodeType == WorldStateModificationNodeType::INCREASE ||
           nodeType == WorldStateModificationNodeType::DECREASE ||
           nodeType == WorldStateModificationNodeType::MULTIPLY)
  {
    pSuccessions.add(_successions);
  }
  else if (nodeType == WorldStateModificationNodeType::FOR_ALL ||
           nodeType == WorldStateModificationNodeType::WHEN)
  {
    if (rightOperand)
      rightOperand->getSuccesions(pSuccessions);
  }
}


void WorldStateModificationNode::printSuccesions(std::string& pRes) const
{
  if (nodeType == WorldStateModificationNodeType::AND)
  {
    if (leftOperand)
      leftOperand->printSuccesions(pRes);
    if (rightOperand)
      rightOperand->printSuccesions(pRes);
  }
  else if (nodeType == WorldStateModificationNodeType::ASSIGN ||
           nodeType == WorldStateModificationNodeType::INCREASE ||
           nodeType == WorldStateModificationNodeType::DECREASE ||
           nodeType == WorldStateModificationNodeType::MULTIPLY)
  {
    if (leftOperand)
    {
      auto* leftFactPtr = toWmFact(*leftOperand);
      if (leftFactPtr != nullptr)
        _successions.print(pRes, leftFactPtr->factOptional);
    }
  }
  else if (nodeType == WorldStateModificationNodeType::FOR_ALL ||
           nodeType == WorldStateModificationNodeType::WHEN)
  {
    if (rightOperand)
      rightOperand->printSuccesions(pRes);
  }
}


bool WorldStateModificationNode::operator==(const WorldStateModification& pOther) const
{
  auto* otherNodePtr = toWmNode(pOther);
  return otherNodePtr != nullptr &&
      nodeType == otherNodePtr->nodeType &&
      _areEqual(leftOperand, otherNodePtr->leftOperand) &&
      _areEqual(rightOperand, otherNodePtr->rightOperand) &&
      parameterOpt == otherNodePtr->parameterOpt;
}


std::optional<Entity> WorldStateModificationNode::getValue(const SetOfFacts& pSetOfFact) const
{
  if (nodeType == WorldStateModificationNodeType::PLUS)
  {
    auto leftValue = leftOperand->getValue(pSetOfFact);
    auto rightValue = rightOperand->getValue(pSetOfFact);
    return plusIntOrStr(leftValue, rightValue);
  }
  if (nodeType == WorldStateModificationNodeType::MINUS)
  {
    auto leftValue = leftOperand->getValue(pSetOfFact);
    auto rightValue = rightOperand->getValue(pSetOfFact);
    return minusIntOrStr(leftValue, rightValue);
  }
  return {};
}


void WorldStateModificationNode::_forAllInstruction(const std::function<void (const WorldStateModification &)>& pCallback,
                                                    const SetOfFacts& pSetOfFact,
                                                    ParameterValuesWithConstraints& pParameters) const
{
  if (leftOperand && rightOperand && parameterOpt)
  {
    auto* leftFactPtr = toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
    {
      std::set<Entity> parameterValues;
      ParameterValuesWithConstraints potentialNewParameters;
      pSetOfFact.extractPotentialArgumentsOfAFactParameter(parameterValues, leftFactPtr->factOptional.fact, parameterOpt->name,
                                                           pParameters, &potentialNewParameters);
      if (!parameterValues.empty())
      {
        for (const auto& paramValue : parameterValues)
        {
          auto newWsModif = rightOperand->clone(nullptr);
          newWsModif->replaceArgument(parameterOpt->toEntity(), paramValue);
          pCallback(*newWsModif);
        }

        for (auto& currParam : potentialNewParameters)
        {
          auto& entities = pParameters[currParam.first];
          if (entities.empty())
            entities = std::move(currParam.second);
          else
            entities.insert(currParam.second.begin(), currParam.second.end());
        }
      }
    }
  }
}


bool WorldStateModificationNode::hasAContradictionWith(const std::set<FactOptional>& pFactsOpt,
                                                       std::list<Parameter>* pParametersPtr) const
{
  if (nodeType == WorldStateModificationNodeType::AND)
  {
    if (leftOperand && leftOperand->hasAContradictionWith(pFactsOpt, pParametersPtr))
      return true;
    if (rightOperand && rightOperand->hasAContradictionWith(pFactsOpt, pParametersPtr))
      return true;
  }
  else if ((nodeType == WorldStateModificationNodeType::ASSIGN ||
            nodeType == WorldStateModificationNodeType::INCREASE ||
            nodeType == WorldStateModificationNodeType::DECREASE ||
            nodeType == WorldStateModificationNodeType::MULTIPLY)  && leftOperand)
  {
    auto* leftFactPtr = toWmFact(*leftOperand);
    if (leftFactPtr != nullptr)
      for (const auto& currFactOpt : pFactsOpt)
        if (leftFactPtr->factOptional.fact.areEqualWithoutValueConsideration(currFactOpt.fact))
          return true;
  }
  else if (nodeType == WorldStateModificationNodeType::FOR_ALL && rightOperand)
  {
    auto parameters = addParameter(pParametersPtr, parameterOpt);
    return rightOperand->hasAContradictionWith(pFactsOpt, &parameters);
  }
  else if (nodeType == WorldStateModificationNodeType::WHEN && rightOperand)
  {
    if (rightOperand && rightOperand->hasAContradictionWith(pFactsOpt, pParametersPtr))
      return true;
  }
  return false;
}

bool WorldStateModificationFact::operator==(const WorldStateModification& pOther) const
{
  auto* otherFactPtr = toWmFact(pOther);
  return otherFactPtr != nullptr &&
      factOptional == otherFactPtr->factOptional;
}

std::optional<Entity> WorldStateModificationFact::getValue(const SetOfFacts& pSetOfFact) const
{
  return pSetOfFact.getFluentValue(factOptional.fact);
}

bool WorldStateModificationFact::hasAContradictionWith(const std::set<FactOptional>& pFactsOpt,
                                                       std::list<Parameter>* pParametersPtr) const
{
  return factOptional.hasAContradictionWith(pFactsOpt, pParametersPtr, false);
}


std::unique_ptr<WorldStateModificationNumber> WorldStateModificationNumber::create(const std::string& pStr)
{
  return std::make_unique<WorldStateModificationNumber>(stringToNumber(pStr));
}

std::string WorldStateModificationNumber::toStr(bool) const
{
  return numberToString(_nb);
}


bool WorldStateModificationNumber::operator==(const WorldStateModification& pOther) const
{
  auto* otherNumberPtr = toWmNumber(pOther);
  return otherNumberPtr != nullptr && _nb == otherNumberPtr->_nb;
}



const WorldStateModificationNode* toWmNode(const WorldStateModification& pOther)
{
  return dynamic_cast<const WorldStateModificationNode*>(&pOther);
}

const WorldStateModificationFact* toWmFact(const WorldStateModification& pOther)
{
  return dynamic_cast<const WorldStateModificationFact*>(&pOther);
}

const WorldStateModificationNumber* toWmNumber(const WorldStateModification& pOther)
{
  return dynamic_cast<const WorldStateModificationNumber*>(&pOther);
}


} // !ogp
