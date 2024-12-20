#include <orderedgoalsplanner/util/serializer/serializeinpddl.hpp>
#include <orderedgoalsplanner/types/domain.hpp>
#include <orderedgoalsplanner/types/problem.hpp>
#include "../../types/worldstatemodificationprivate.hpp"


namespace ogp
{
namespace
{
std::size_t _identationOffset = 4;

std::string _parametersToPddl(const std::vector<Parameter>& pParameters)
{
  std::string res = "(";
  bool firstIteraction = true;
  for (const auto& currParam : pParameters)
  {
    if (firstIteraction)
      firstIteraction = false;
    else
      res +=  " ";
    res += currParam.toStr();
  }
  return res + ")";
}


struct ConditionWithPartInfo
{
  ConditionWithPartInfo(const Condition& pCondition,
                        ConditionPart pConditionPart)
   : condition(pCondition),
     conditionPart(pConditionPart)
  {
  }

  const Condition& condition;
  ConditionPart conditionPart;
};


std::string _conditionsToPddl(
    const std::list<ConditionWithPartInfo>& pConditionWithPartInfos,
    std::size_t pIdentation)
{
  std::list<std::string> results;
  for (const auto& currElt : pConditionWithPartInfos)
  {
    std::list<std::string> subResults;

    const ConditionNode* condNodePtr = currElt.condition.fcNodePtr();
    if (condNodePtr != nullptr)
    {
      const auto& condNode = *condNodePtr;

      if (condNode.nodeType == ConditionNodeType::AND)
      {
        auto subIndentation = _identationOffset;
        std::string leftOperandStr;
        if (condNode.leftOperand)
          leftOperandStr = conditionToPddl(*condNode.leftOperand, subIndentation);
        subResults.emplace_back(leftOperandStr);
        auto* nodePtr = condNodePtr;

        while (true)
        {
          auto* newNodePtr = nodePtr->rightOperand->fcNodePtr();
          if (newNodePtr == nullptr || newNodePtr->nodeType != condNode.nodeType)
          {
            subResults.emplace_back(conditionToPddl(*nodePtr->rightOperand, subIndentation));
            break;
          }

          subResults.emplace_back(conditionToPddl(*newNodePtr->leftOperand, subIndentation));
          nodePtr = newNodePtr;
        }
      }
    }

    if (subResults.empty())
      subResults.emplace_back(conditionToPddl(currElt.condition, 0));

    for (const auto& currSubResult : subResults)
    {
      if (currElt.conditionPart == ConditionPart::AT_START)
        results.emplace_back("(at start " + currSubResult + ")");
      else
        results.emplace_back("(over all " + currSubResult + ")");
    }
  }

  if (results.size() == 1)
    return results.front();

  if (results.size() > 1)
  {
    std::string res = "(and\n";
    for (const auto& currPart : results)
      res += std::string(pIdentation + _identationOffset, ' ') + currPart + "\n";
    return res + std::string(pIdentation, ' ') + ")";
  }

  return "";
}


struct WorldStateModificationWithPartInfo
{
  WorldStateModificationWithPartInfo(const WorldStateModification& pWsModif,
                                     WsModificationPart pWsModificationPart)
    : wsModif(pWsModif),
      wsModificationPart(pWsModificationPart)
  {
  }

  const WorldStateModification& wsModif;
  WsModificationPart wsModificationPart;
};


std::string _effectToPddl(
    const WorldStateModification& pWsModif,
    std::size_t pIdentation)
{
  const auto* wsmNodePtr = toWmNode(pWsModif);
  if (wsmNodePtr != nullptr)
  {
    const auto& wsmNode = *wsmNodePtr;
    std::string leftOperandStr;
    if (wsmNode.leftOperand)
      leftOperandStr = _effectToPddl(*wsmNode.leftOperand, pIdentation);
    std::string rightOperandStr;
    bool isRightOperandAFactWithoutParameter = false;
    if (wsmNode.rightOperand)
    {
      const auto* rightOperandFactPtr = toWmFact(*wsmNode.rightOperand);
      if (rightOperandFactPtr != nullptr && rightOperandFactPtr->factOptional.fact.arguments().empty() &&
          !rightOperandFactPtr->factOptional.fact.fluent())
        isRightOperandAFactWithoutParameter = true;
      rightOperandStr = _effectToPddl(*wsmNode.rightOperand, pIdentation);
    }

    switch (wsmNode.nodeType)
    {
    case WorldStateModificationNodeType::AND:
      return "(and " + leftOperandStr + " " + rightOperandStr + ")";
    case WorldStateModificationNodeType::ASSIGN:
    {
      if (isRightOperandAFactWithoutParameter)
        rightOperandStr += "()"; // To significate it is a fact
      return "(assign " + leftOperandStr + " " + rightOperandStr + ")";
    }
    case WorldStateModificationNodeType::FOR_ALL:
      if (!wsmNode.parameterOpt)
        throw std::runtime_error("for all statement without a parameter detected");
      return "(forall (" + wsmNode.parameterOpt->toStr() + ") (when " +
          leftOperandStr + " " + rightOperandStr + "))";
    case WorldStateModificationNodeType::INCREASE:
      return "(increase " + leftOperandStr + " " + rightOperandStr + ")";
    case WorldStateModificationNodeType::DECREASE:
      return "(decrease " + leftOperandStr + " " + rightOperandStr + ")";
    case WorldStateModificationNodeType::MULTIPLY:
      return "(* " + leftOperandStr + " " + rightOperandStr + ")";
    case WorldStateModificationNodeType::PLUS:
      return "(+ " + leftOperandStr + " " + rightOperandStr + ")";
    case WorldStateModificationNodeType::MINUS:
      return "(- " + leftOperandStr + " " + rightOperandStr + ")";
    case WorldStateModificationNodeType::WHEN:
      return "(when " + leftOperandStr + " " + rightOperandStr + ")";
    }

    throw std::runtime_error("Unkown WorldStateModificationNodeType type");
  }

  const auto* wsmFactPtr = toWmFact(pWsModif);
  if (wsmFactPtr != nullptr)
    return wsmFactPtr->factOptional.toPddl(true, false);

  const auto* wsmNbPtr = toWmNumber(pWsModif);
  if (wsmNbPtr != nullptr)
    return numberToString(wsmNbPtr->getNb());

  throw std::runtime_error("Unknown WorldStateModification children struct");
}


std::string _effectsToPddl(
    const std::list<WorldStateModificationWithPartInfo>& pWorldStateModificationWithPartInfos,
    std::size_t pIdentation)
{
  std::list<std::string> results;
  for (const auto& currElt : pWorldStateModificationWithPartInfos)
  {
    std::list<std::string> subResults;

    const auto* wsmNodePtr = toWmNode(currElt.wsModif);
    if (wsmNodePtr != nullptr)
    {
      const auto& wsmNode = *wsmNodePtr;

      if (wsmNode.nodeType == WorldStateModificationNodeType::AND)
      {
        auto subIndentation = _identationOffset;
        std::string leftOperandStr;
        if (wsmNode.leftOperand)
          leftOperandStr = _effectToPddl(*wsmNode.leftOperand, subIndentation);

        subResults.emplace_back(leftOperandStr);
        auto* nodePtr = wsmNodePtr;

        while (true)
        {
          auto* newNodePtr = toWmNode(*nodePtr->rightOperand);
          if (newNodePtr == nullptr || newNodePtr->nodeType != wsmNode.nodeType)
          {
            subResults.emplace_back(_effectToPddl(*nodePtr->rightOperand, subIndentation));
            break;
          }

          subResults.emplace_back(_effectToPddl(*newNodePtr->leftOperand, subIndentation));
          nodePtr = newNodePtr;
        }
      }
    }

    if (subResults.empty())
      subResults.emplace_back(_effectToPddl(currElt.wsModif, 0));

    for (const auto& currSubResult : subResults)
    {
      if (currElt.wsModificationPart == WsModificationPart::AT_START)
        results.emplace_back("(at start " + currSubResult + ")");
      else if (currElt.wsModificationPart == WsModificationPart::AT_END)
        results.emplace_back("(at end " + currSubResult + ")");
      else
        results.emplace_back("(at end " + currSubResult + ") ;; __POTENTIALLY");
    }
  }

  if (results.size() == 1)
    return results.front();

  if (results.size() > 1)
  {
    std::string res = "(and\n";
    for (const auto& currPart : results)
      res += std::string(pIdentation + _identationOffset, ' ') + currPart + "\n";
    return res + std::string(pIdentation, ' ') + ")";
  }

  return "";
}

}


std::string domainToPddl(const Domain& pDomain)
{
  std::string res = "(define\n";

  std::size_t identation = _identationOffset;


  res += std::string(identation, ' ') + "(domain " + pDomain.getName() + ")\n";

  const auto& requirements = pDomain.requirements();
  if (!requirements.empty())
  {
    res += std::string(identation, ' ') + "(:requirements";
    for (const auto& currRequirement : requirements)
      res += " " + currRequirement;
    res += ")\n";
  }
  res += "\n";

  const auto& ontology = pDomain.getOntology();

  if (!ontology.types.empty())
  {
    res += std::string(identation, ' ') + "(:types\n";
    res += ontology.types.toStr(_identationOffset + identation);
    res += "\n" + std::string(identation, ' ') + ")\n\n";
  }

  if (!ontology.constants.empty())
  {
    res += std::string(identation, ' ') + "(:constants\n";
    res += ontology.constants.toStr(_identationOffset + identation);
    res += "\n" + std::string(identation, ' ') + ")\n\n";
  }

  if (ontology.predicates.hasPredicateOfPddlType(PredicatePddlType::PDDL_PREDICATE))
  {
    res += std::string(identation, ' ') + "(:predicates\n";
    res += ontology.predicates.toPddl(PredicatePddlType::PDDL_PREDICATE, _identationOffset + identation);
    res += "\n" + std::string(identation, ' ') + ")\n\n";
  }

  if (ontology.predicates.hasPredicateOfPddlType(PredicatePddlType::PDDL_FUNCTION))
  {
    res += std::string(identation, ' ') + "(:functions\n";
    res += ontology.predicates.toPddl(PredicatePddlType::PDDL_FUNCTION, _identationOffset + identation);
    res += "\n" + std::string(identation, ' ') + ")\n\n";
  }

  const auto& timelessFacts = pDomain.getTimelessFacts().setOfFacts();
  if (!timelessFacts.empty())
  {
    res += std::string(identation, ' ') + "(:timeless\n";
    res += timelessFacts.toPddl(_identationOffset + identation, true);
    res += "\n" + std::string(identation, ' ') + ")\n\n";
  }

  auto setOfEvents = pDomain.getSetOfEvents();
  if (!setOfEvents.empty())
  {
    for (const auto& currSetOfEvent : setOfEvents)
    {
      for (const auto& currEventIdToEvent : currSetOfEvent.second.events())
      {
        const Event& currEvent = currEventIdToEvent.second;
        res += std::string(identation, ' ') + "(:event ";
        if (setOfEvents.size() == 1)
          res += currEventIdToEvent.first + "\n";
        else
          res += currSetOfEvent.first + "-" + currEventIdToEvent.first + "\n";
        res += "\n";
        std::size_t subIdentation = identation + _identationOffset;
        std::size_t subSubIdentation = subIdentation + _identationOffset;

        std::string eventContent;
        if (!currEvent.parameters.empty())
        {
          eventContent += std::string(subIdentation, ' ') + ":parameters\n";
          eventContent += std::string(subSubIdentation, ' ') + _parametersToPddl(currEvent.parameters) + "\n";
        }

        if (currEvent.precondition)
        {
          if (!eventContent.empty())
            eventContent += "\n";
          eventContent += std::string(subIdentation, ' ') + ":precondition\n";
          eventContent += std::string(subSubIdentation, ' ') +
              conditionToPddl(*currEvent.precondition, subSubIdentation) + "\n";
        }

        if (currEvent.factsToModify)
        {
          if (!eventContent.empty())
            eventContent += "\n";
          eventContent += std::string(subIdentation, ' ') + ":effect\n";
          eventContent += std::string(subSubIdentation, ' ') +
              _effectToPddl(*currEvent.factsToModify, subSubIdentation);
        }

        res += eventContent;
        res += "\n" + std::string(identation, ' ') + ")\n\n";
      }
    }
  }

  for (const auto& currActionNameToAction : pDomain.actions())
  {
    res += std::string(identation, ' ') + "(:durative-action " +
        currActionNameToAction.first + "\n";
    std::size_t subIdentation = identation + _identationOffset;
    std::size_t subSubIdentation = subIdentation + _identationOffset;

    std::string actionContent;
    const Action& currAction = currActionNameToAction.second;
    if (!currAction.parameters.empty())
    {
      actionContent += std::string(subIdentation, ' ') + ":parameters\n";
      actionContent += std::string(subSubIdentation, ' ') + _parametersToPddl(currAction.parameters) + "\n";
    }

    {
      if (!actionContent.empty())
        actionContent += "\n";
      actionContent += std::string(subIdentation, ' ') + ":duration (= ?duration 1)\n";
    }

    if (currAction.precondition || currAction.overAllCondition)
    {
      actionContent += "\n";
      actionContent += std::string(subIdentation, ' ') + ":condition\n";
      std::list<ConditionWithPartInfo> conditionWithPartInfos;
      if (currAction.precondition)
        conditionWithPartInfos.emplace_back(*currAction.precondition, ConditionPart::AT_START);
      if (currAction.overAllCondition)
        conditionWithPartInfos.emplace_back(*currAction.overAllCondition, ConditionPart::OVER_ALL);
      actionContent += std::string(subSubIdentation, ' ') +
          _conditionsToPddl(conditionWithPartInfos, subSubIdentation) + "\n";
    }

    {
      actionContent += "\n";
      actionContent += std::string(subIdentation, ' ') + ":effect\n";
      std::list<WorldStateModificationWithPartInfo> worldStateModificationWithPartInfos;
      if (currAction.effect.worldStateModificationAtStart)
        worldStateModificationWithPartInfos.emplace_back(*currAction.effect.worldStateModificationAtStart, WsModificationPart::AT_START);
      if (currAction.effect.worldStateModification)
        worldStateModificationWithPartInfos.emplace_back(*currAction.effect.worldStateModification, WsModificationPart::AT_END);
      if (currAction.effect.potentialWorldStateModification)
        worldStateModificationWithPartInfos.emplace_back(*currAction.effect.potentialWorldStateModification, WsModificationPart::POTENTIALLY_AT_END);
      actionContent += std::string(subSubIdentation, ' ') +
          _effectsToPddl(worldStateModificationWithPartInfos, subSubIdentation);
    }


    res += actionContent;
    res += "\n" + std::string(identation, ' ') + ")\n\n";
  }

  return res + ")";
}


std::string problemToPddl(const Problem& pProblem,
                          const Domain& pDomain)
{
  std::string res = "(define\n";
  std::size_t identation = _identationOffset;

  res += std::string(identation, ' ') + "(problem " + pProblem.name + ")\n";
  res += std::string(identation, ' ') + "(:domain " + pDomain.getName() + ")\n\n";

  if (!pProblem.entities.empty())
  {
    res += std::string(identation, ' ') + "(:objects\n";
    res += pProblem.entities.toStr(_identationOffset + identation);
    res += "\n" + std::string(identation, ' ') + ")\n\n";
  }

  const SetOfFacts& facts = pProblem.worldState.factsMapping();
  if (!facts.empty())
  {
    res += std::string(identation, ' ') + "(:init\n";
    res += facts.toPddl(_identationOffset + identation, false);
    res += "\n" + std::string(identation, ' ') + ")\n\n";
  }


  std::list<std::string> pddlGoals;
  std::size_t subIdentation = identation + _identationOffset;
  std::size_t subSubIdentation = subIdentation + _identationOffset;

  const auto& goals = pProblem.goalStack.goals();
  if (!goals.empty())
  {
    res += std::string(identation, ' ') + "(:goal\n";
    if (goals.size() == 1 && goals.begin()->second.size() == 1)
    {
      const auto& currGoal = goals.begin()->second.front();
      auto pddlGoal = currGoal.toPddl(subSubIdentation);
      res += std::string(subSubIdentation, ' ') + pddlGoal + "\n";
      pddlGoals.emplace_back(pddlGoal);
    }
    else
    {
      res += std::string(subIdentation, ' ') + "(and ;; __ORDERED\n";
      for (auto itGoalsGroup = goals.end(); itGoalsGroup != goals.begin(); )
      {
        --itGoalsGroup;
        for (const Goal& currGoal : itGoalsGroup->second)
        {
          auto pddlGoal = currGoal.toPddl(subSubIdentation);
          res += std::string(subSubIdentation, ' ') + pddlGoal + "\n";
          pddlGoals.emplace_back(pddlGoal);
        }
      }
      res += std::string(subIdentation, ' ') + ")\n";
    }
    res += std::string(identation, ' ') + ")\n\n";
  }

  if (pddlGoals.size() > 1)
  {
    std::size_t subSubSubIdentation = subSubIdentation + _identationOffset;
    res += std::string(identation, ' ') + "(:constraints\n";
    res += std::string(subIdentation, ' ') + "(and ; These contraints are to specify the goals order\n";

    std::size_t preferenceIndex = 0;
    std::string previousGoal;
    for (const auto& currPddlGoal : pddlGoals)
    {
      if (previousGoal != "")
      {
        res += std::string(subSubIdentation, ' ') + "(preference p" + std::to_string(preferenceIndex) + "\n";
        ++preferenceIndex;
        res += std::string(subSubSubIdentation, ' ') + "(sometime-after " +
            previousGoal + " " + currPddlGoal + ")\n";
        res += std::string(subSubIdentation, ' ') + ")\n";
      }
      previousGoal = currPddlGoal;
    }

    res += std::string(subIdentation, ' ') + ")\n";
    res += std::string(identation, ' ') + ")\n\n";
  }

  return res + ")";
}




std::string conditionToPddl(const Condition& pCondition,
                            std::size_t pIdentation)
{
  const ConditionNode* condNodePtr = pCondition.fcNodePtr();
  if (condNodePtr != nullptr)
  {
    const auto& condNode = *condNodePtr;

    if (condNode.nodeType == ConditionNodeType::AND || condNode.nodeType == ConditionNodeType::OR || condNode.nodeType == ConditionNodeType::IMPLY)
    {
      std::string contentStr = "(";
      if (condNode.nodeType == ConditionNodeType::AND)
        contentStr += "and";
      else if (condNode.nodeType == ConditionNodeType::OR)
        contentStr += "or";
      else
        contentStr += "imply";
      auto indentation = pIdentation + _identationOffset;
      std::string leftOperandStr;
      if (condNode.leftOperand)
        leftOperandStr = conditionToPddl(*condNode.leftOperand, indentation);
      contentStr += "\n" + std::string(indentation, ' ') + leftOperandStr;
      auto* nodePtr = condNodePtr;

      while (true)
      {
        contentStr += "\n" + std::string(indentation, ' ');
        auto* newNodePtr = nodePtr->rightOperand->fcNodePtr();
        if (newNodePtr == nullptr || newNodePtr->nodeType != condNode.nodeType)
        {
          contentStr += conditionToPddl(*nodePtr->rightOperand, indentation);
          break;
        }

        contentStr += conditionToPddl(*newNodePtr->leftOperand, indentation);
        nodePtr = newNodePtr;
      }
      return contentStr + "\n" + std::string(pIdentation, ' ') + ")";
    }

    std::string leftOperandStr;
    if (condNode.leftOperand)
      leftOperandStr = conditionToPddl(*condNode.leftOperand, pIdentation);
    std::string rightOperandStr;
    if (condNode.rightOperand)
      rightOperandStr = conditionToPddl(*condNode.rightOperand, pIdentation);

    std::string res = "(";
    switch (condNode.nodeType)
    {
    case ConditionNodeType::EQUALITY:
      res += "=";
      break;
    case ConditionNodeType::PLUS:
      res += "+";
      break;
    case ConditionNodeType::MINUS:
      res += "-";
      break;
    case ConditionNodeType::SUPERIOR:
      res += ">";
      break;
    case ConditionNodeType::SUPERIOR_OR_EQUAL:
      res += ">=";
      break;
    case ConditionNodeType::INFERIOR:
      res += "<";
      break;
    case ConditionNodeType::INFERIOR_OR_EQUAL:
      res += "<=";
      break;
    case ConditionNodeType::AND:
    case ConditionNodeType::OR:
    case ConditionNodeType::IMPLY:
      break;
    }
    return res + " " + leftOperandStr + " " + rightOperandStr + ")";
  }

  const ConditionExists* condExistsPtr = pCondition.fcExistsPtr();
  if (condExistsPtr != nullptr)
  {
    const auto& condExists = *condExistsPtr;
    std::string conditionStr;
    if (condExists.condition)
      conditionStr = conditionToPddl(*condExists.condition, pIdentation);
    return "(exists (" + condExists.parameter.toStr() + ") " + conditionStr + ")";
  }

  const ConditionForall* condForallPtr = pCondition.fcForallPtr();
  if (condForallPtr != nullptr)
  {
    const auto& condForall = *condForallPtr;
    std::string conditionStr;
    if (condForall.condition)
      conditionStr = conditionToPddl(*condForall.condition, pIdentation);
    return "(forall (" + condForall.parameter.toStr() + ") " + conditionStr + ")";
  }

  const ConditionNot* condNotPtr = pCondition.fcNotPtr();
  if (condNotPtr != nullptr)
  {
    const auto& condNot = *condNotPtr;
    if (condNot.condition)
      return "(not " + conditionToPddl(*condNot.condition, pIdentation) + ")";
    return "";
  }

  const ConditionFact* condFactPtr = pCondition.fcFactPtr();
  if (condFactPtr != nullptr)
    return condFactPtr->factOptional.toPddl(false, false);

  const ConditionNumber* condNbPtr = pCondition.fcNbPtr();
  if (condNbPtr != nullptr)
    return numberToString(condNbPtr->nb);

  throw std::runtime_error("Unknown conditon type");
}

}
