#include <orderedgoalsplanner/util/serializer/deserializefrompddl.hpp>
#include <memory>
#include <orderedgoalsplanner/types/axiom.hpp>
#include <orderedgoalsplanner/types/domain.hpp>
#include <orderedgoalsplanner/types/problem.hpp>
#include <orderedgoalsplanner/types/setofcallbacks.hpp>
#include "../../types/expressionParsed.hpp"
#include "../../types/worldstatemodificationprivate.hpp"

namespace ogp
{
namespace
{
const char* _equalsConditonFunctionName = "equals";
const char* _equalsCharConditonFunctionName = "=";
const char* _existsConditonFunctionName = "exists";
const char* _forallConditonFunctionName = "forall";
const char* _notConditonFunctionName = "not";
const char* _superiorConditionFunctionName = ">";
const char* _superiorOrEqualConditionFunctionName = ">=";
const char* _inferiorConditionFunctionName = "<";
const char* _inferiorOrEqualConditionFunctionName = "<=";
const char* _andConditonFunctionName = "and";
const char* _orConditonFunctionName = "or";
const char* _implyConditonFunctionName = "imply";

const char* _assignWsFunctionName = "assign";
const char* _setWsFunctionName = "set"; // deprecated
const char* _forAllWsFunctionName = "forall";
const char* _forAllOldWsFunctionName = "forAll";
const char* _addWsFunctionName = "add";
const char* _mutliplyWsFunctionName = "*";
const char* _increaseWsFunctionName = "increase";
const char* _decreaseWsFunctionName = "decrease";
const char* _andWsFunctionName = "and";
const char* _whenWsFunctionName = "when";
const char* _notWsFunctionName = "not";


std::vector<Parameter> _pddlArgumentsToParameters(
    const std::list<ExpressionParsed>& pArguments,
    const std::string& pParameterName,
    const SetOfTypes& pSetOfTypes)
{
  std::vector<Parameter> res;
  std::string parameterName = pParameterName;
  bool nextIsAParameterName = true;
  for (const auto& currArg : pArguments)
  {
    const auto& token = currArg.name;
    if (token == "-")
    {
      nextIsAParameterName = false;
    }
    else if (nextIsAParameterName)
    {
      if (parameterName != "")
        res.emplace_back(parameterName, pSetOfTypes.nameToType("number"));
      parameterName = token;
    }
    else
    {
      res.emplace_back(parameterName, pSetOfTypes.nameToType(token));
      parameterName = "";
      nextIsAParameterName = true;
    }
  }

  if (parameterName != "")
    res.emplace_back(parameterName, pSetOfTypes.nameToType("number"));
  return res;
}


std::vector<Parameter> _expressionParsedToParameters(
    const ExpressionParsed& pExpressionParsed,
    std::vector<Parameter>& pAllParameters,
    const SetOfTypes& pSetOfTypes)
{
  std::vector<Parameter> res;
  if (pExpressionParsed.followingExpression)
  {
    auto paramType = pSetOfTypes.nameToType(pExpressionParsed.followingExpression->name);
    res.emplace_back(pExpressionParsed.name, paramType);
  }
  else if (pExpressionParsed.arguments.size() >= 2)
  {
    res = _pddlArgumentsToParameters(pExpressionParsed.arguments, pExpressionParsed.name, pSetOfTypes);
  }

  for (const auto& currElt : res)
    pAllParameters.push_back(currElt);
  return res;
}

std::unique_ptr<Condition> _expressionParsedToCondition(const ExpressionParsed& pExpressionParsed,
                                                        const Ontology& pOntology,
                                                        const SetOfEntities& pEntities,
                                                        const std::vector<Parameter>& pParameters,
                                                        bool pIsOkIfFluentIsMissing)
{
  std::unique_ptr<Condition> res;

  auto nodeType = ConditionNodeType::AND;
  if (pExpressionParsed.followingExpression)
  {
    if (pExpressionParsed.separatorToFollowingExp == '+')
      nodeType = ConditionNodeType::PLUS;
    else if (pExpressionParsed.separatorToFollowingExp == '-')
      nodeType = ConditionNodeType::MINUS;
    else if (pExpressionParsed.separatorToFollowingExp == '>')
      nodeType = ConditionNodeType::SUPERIOR;
    else if (pExpressionParsed.separatorToFollowingExp == '<')
      nodeType = ConditionNodeType::INFERIOR;
    else if (pExpressionParsed.separatorToFollowingExp == '|')
      nodeType = ConditionNodeType::OR;
  }

  if ((pExpressionParsed.name == _equalsConditonFunctionName ||
       pExpressionParsed.name == _equalsCharConditonFunctionName) &&
      pExpressionParsed.arguments.size() == 2)
  {
    auto leftOperand = _expressionParsedToCondition(pExpressionParsed.arguments.front(), pOntology, pEntities, pParameters, true);
    auto* leftFactPtr = leftOperand->fcFactPtr();

    const auto& rightOperandExp = *(++pExpressionParsed.arguments.begin());
    if (leftFactPtr != nullptr && !leftFactPtr->factOptional.isFactNegated &&
        rightOperandExp.arguments.empty() &&
        !rightOperandExp.followingExpression && rightOperandExp.value == "")
    {
      if (rightOperandExp.name == Fact::getUndefinedValue().value)
      {
        leftFactPtr->factOptional.isFactNegated = true;
        leftFactPtr->factOptional.fact.setFluentValue(Entity::anyEntityValue());
        res = std::make_unique<ConditionFact>(std::move(*leftFactPtr));
      }
      else if (pExpressionParsed.name == _equalsCharConditonFunctionName && !rightOperandExp.isAFunction &&
               rightOperandExp.name != "")
      {
        leftFactPtr->factOptional.fact.setFluent(
              Entity::fromUsage(rightOperandExp.name, pOntology, pEntities, pParameters));
        res = std::make_unique<ConditionFact>(std::move(*leftFactPtr));
      }
    }

    if (!res)
    {
      auto rightOperand = _expressionParsedToCondition(rightOperandExp, pOntology, pEntities, pParameters, true);
      res = std::make_unique<ConditionNode>(ConditionNodeType::EQUALITY,
                                            std::move(leftOperand), std::move(rightOperand));
    }
  }
  else if (pExpressionParsed.name == _existsConditonFunctionName)
  {
    if (pExpressionParsed.arguments.size() != 2)
      throw std::runtime_error("Exists function badly formatted, it should have only 2 parameters");

    auto itArg = pExpressionParsed.arguments.begin();
    auto& firstArg = *itArg;
    auto newParameters = pParameters;
    auto existsParameters = _expressionParsedToParameters(firstArg, newParameters, pOntology.types);
    if (existsParameters.size() != 1)
      throw std::runtime_error("Only one parameter is handled for exists function (needs to be improved)");
    auto& existsParameter = existsParameters.front();

    ++itArg;
    res = std::make_unique<ConditionExists>(existsParameter,
                                            _expressionParsedToCondition(*itArg, pOntology, pEntities, newParameters, false));
  }
  else if (pExpressionParsed.name == _forallConditonFunctionName)
  {
    if (pExpressionParsed.arguments.size() != 2)
      throw std::runtime_error("Forall function badly formatted, it should have only 2 parameters");

    auto itArg = pExpressionParsed.arguments.begin();
    auto& firstArg = *itArg;
    auto newParameters = pParameters;
    auto existsParameters = _expressionParsedToParameters(firstArg, newParameters, pOntology.types);
    if (existsParameters.size() != 1)
      throw std::runtime_error("Only one parameter is handled for forall function (needs to be improved)");
    auto& existsParameter = existsParameters.front();

    ++itArg;
    res = std::make_unique<ConditionForall>(existsParameter,
                                            _expressionParsedToCondition(*itArg, pOntology, pEntities, newParameters, false));
  }
  else if (pExpressionParsed.name == _notConditonFunctionName &&
           pExpressionParsed.arguments.size() == 1)
  {
    auto& expNegationned = pExpressionParsed.arguments.front();

    res = _expressionParsedToCondition(expNegationned, pOntology, pEntities, pParameters, pIsOkIfFluentIsMissing);
    if (res)
    {
       auto* factPtr = res->fcFactPtr();
       if (factPtr != nullptr)
         factPtr->factOptional.isFactNegated = !factPtr->factOptional.isFactNegated;
       else
         res = std::make_unique<ConditionNot>(std::move(res));
    }
  }
  else if (pExpressionParsed.name == _superiorConditionFunctionName &&
           pExpressionParsed.arguments.size() == 2)
  {
    res = std::make_unique<ConditionNode>(ConditionNodeType::SUPERIOR,
                                          _expressionParsedToCondition(pExpressionParsed.arguments.front(), pOntology, pEntities, pParameters, true),
                                          _expressionParsedToCondition(*(++pExpressionParsed.arguments.begin()), pOntology, pEntities, pParameters, true));
  }
  else if (pExpressionParsed.name == _superiorOrEqualConditionFunctionName &&
           pExpressionParsed.arguments.size() == 2)
  {
    res = std::make_unique<ConditionNode>(ConditionNodeType::SUPERIOR_OR_EQUAL,
                                          _expressionParsedToCondition(pExpressionParsed.arguments.front(), pOntology, pEntities, pParameters, true),
                                          _expressionParsedToCondition(*(++pExpressionParsed.arguments.begin()), pOntology, pEntities, pParameters, true));
  }
  else if (pExpressionParsed.name == _inferiorConditionFunctionName &&
           pExpressionParsed.arguments.size() == 2)
  {
    res = std::make_unique<ConditionNode>(ConditionNodeType::INFERIOR,
                                          _expressionParsedToCondition(pExpressionParsed.arguments.front(), pOntology, pEntities, pParameters, true),
                                          _expressionParsedToCondition(*(++pExpressionParsed.arguments.begin()), pOntology, pEntities, pParameters, true));
  }
  else if (pExpressionParsed.name == _inferiorOrEqualConditionFunctionName &&
           pExpressionParsed.arguments.size() == 2)
  {
    res = std::make_unique<ConditionNode>(ConditionNodeType::INFERIOR_OR_EQUAL,
                                          _expressionParsedToCondition(pExpressionParsed.arguments.front(), pOntology, pEntities, pParameters, true),
                                          _expressionParsedToCondition(*(++pExpressionParsed.arguments.begin()), pOntology, pEntities, pParameters, true));
  }
  else if ((pExpressionParsed.name == _andConditonFunctionName ||
            pExpressionParsed.name == _orConditonFunctionName ||
            pExpressionParsed.name == _implyConditonFunctionName) &&
           pExpressionParsed.arguments.size() >= 2)
  {
    auto listNodeType = ConditionNodeType::AND;
    if (pExpressionParsed.name == _orConditonFunctionName)
      listNodeType = ConditionNodeType::OR;
    else if (pExpressionParsed.name == _implyConditonFunctionName)
      listNodeType = ConditionNodeType::IMPLY;
    std::list<std::unique_ptr<Condition>> elts;
    for (auto& currExp : pExpressionParsed.arguments)
      elts.emplace_back(_expressionParsedToCondition(currExp, pOntology, pEntities, pParameters, false));

    res = std::make_unique<ConditionNode>(listNodeType, std::move(*(--(--elts.end()))), std::move(elts.back()));
    elts.pop_back();
    elts.pop_back();

    while (!elts.empty())
    {
      res = std::make_unique<ConditionNode>(listNodeType, std::move(elts.back()), std::move(res));
      elts.pop_back();
    }
  }
  else
  {
    if (pExpressionParsed.arguments.empty() && pExpressionParsed.value == "")
    {
      try {
        res = std::make_unique<ConditionNumber>(stringToNumber(pExpressionParsed.name));
      }  catch (...) {}
    }

    if (!res)
    {
      bool isOkIfFluentIsMissing = pIsOkIfFluentIsMissing ||
          nodeType == ConditionNodeType::SUPERIOR || nodeType == ConditionNodeType::SUPERIOR_OR_EQUAL ||
          nodeType == ConditionNodeType::INFERIOR || nodeType == ConditionNodeType::INFERIOR_OR_EQUAL;
      res = std::make_unique<ConditionFact>(pExpressionParsed.toFact(pOntology, pEntities, pParameters, isOkIfFluentIsMissing));
    }
  }

  if (pExpressionParsed.followingExpression)
  {
    res = std::make_unique<ConditionNode>(nodeType,
                                          std::move(res),
                                          _expressionParsedToCondition(*pExpressionParsed.followingExpression, pOntology, pEntities, pParameters, false));
  }

  return res;
}


std::unique_ptr<Goal> _expressionParsedToGoal(ExpressionParsed& pExpressionParsed,
                                              const Ontology& pOntology,
                                              const SetOfEntities& pEntities,
                                              int pMaxTimeToKeepInactive,
                                              const std::string& pGoalGroupId)
{
  bool isPersistentIfSkipped = false;
  bool oneStepTowards = false;

  if (pExpressionParsed.name == "persist" && pExpressionParsed.arguments.size() == 1)
  {
    isPersistentIfSkipped = true;
    pExpressionParsed = pExpressionParsed.arguments.front().clone();
  }
  if (pExpressionParsed.name == "oneStepTowards" && pExpressionParsed.arguments.size() == 1)
  {
    oneStepTowards = true;
    pExpressionParsed = pExpressionParsed.arguments.front().clone();
  }

  auto objective = _expressionParsedToCondition(pExpressionParsed, pOntology, pEntities, {}, false);
  if (!objective)
    throw std::runtime_error("Failed to load the goal objective");
  return std::make_unique<Goal>(std::move(objective), isPersistentIfSkipped, oneStepTowards,
                                pMaxTimeToKeepInactive, pGoalGroupId);
}


std::unique_ptr<WorldStateModification> _expressionParsedToWsModification(const ExpressionParsed& pExpressionParsed,
                                                                          const Ontology& pOntology,
                                                                          const SetOfEntities& pEntities,
                                                                          const std::vector<Parameter>& pParameters,
                                                                          bool pIsOkIfFluentIsMissing)
{
  std::unique_ptr<WorldStateModification> res;

  if ((pExpressionParsed.name == _assignWsFunctionName ||
       pExpressionParsed.name == _setWsFunctionName) && // set is deprecated
      pExpressionParsed.arguments.size() == 2)
  {
    auto leftOperand = _expressionParsedToWsModification(pExpressionParsed.arguments.front(), pOntology, pEntities, pParameters, true);
    const auto& rightOperandExp = *(++pExpressionParsed.arguments.begin());
    auto* leftFactPtr = dynamic_cast<WorldStateModificationFact*>(&*leftOperand);
    if (leftFactPtr != nullptr && !leftFactPtr->factOptional.isFactNegated &&
        rightOperandExp.arguments.empty() &&
        !rightOperandExp.followingExpression && rightOperandExp.value == "")
    {
      if (rightOperandExp.name == Fact::getUndefinedValue().value)
      {
        leftFactPtr->factOptional.isFactNegated = true;
        leftFactPtr->factOptional.fact.setFluentValue(Entity::anyEntityValue());
        res = std::make_unique<WorldStateModificationFact>(std::move(*leftFactPtr));
      }
      else if (pExpressionParsed.name == _assignWsFunctionName && !rightOperandExp.isAFunction &&
               rightOperandExp.name != "")
      {
        leftFactPtr->factOptional.fact.setFluent(Entity::fromUsage(rightOperandExp.name, pOntology, pEntities, pParameters));
        res = std::make_unique<WorldStateModificationFact>(std::move(*leftFactPtr));
      }
    }

    if (!res)
    {
      auto rightOperand = _expressionParsedToWsModification(rightOperandExp, pOntology, pEntities, pParameters, true);
      res = std::make_unique<WorldStateModificationNode>(WorldStateModificationNodeType::ASSIGN,
                                                         std::move(leftOperand), std::move(rightOperand));
    }
  }
  else if (pExpressionParsed.name == _notWsFunctionName &&
           pExpressionParsed.arguments.size() == 1)
  {
    auto factNegationedWs = _expressionParsedToWsModification(pExpressionParsed.arguments.front(), pOntology, pEntities, pParameters, true);
    if (factNegationedWs)
    {
      auto* factNegationedPtr = dynamic_cast<WorldStateModificationFact*>(&*factNegationedWs);
      if (factNegationedPtr != nullptr)
      {
        factNegationedPtr->factOptional.isFactNegated = !factNegationedPtr->factOptional.isFactNegated;
        res = std::move(factNegationedWs);
      }
    }
    if (!res)
      throw std::runtime_error("Not a valid negated world state modification: \"" + pExpressionParsed.toStr() + "\"");

  }
  else if ((pExpressionParsed.name == _forAllWsFunctionName || pExpressionParsed.name == _forAllOldWsFunctionName) &&
           (pExpressionParsed.arguments.size() == 2 || pExpressionParsed.arguments.size() == 3))
  {
    auto itArg = pExpressionParsed.arguments.begin();
    auto& firstArg = *itArg;

    auto newParameters = pParameters;
    auto forAllParameters = _expressionParsedToParameters(firstArg, newParameters, pOntology.types);
    if (forAllParameters.size() != 1)
      throw std::runtime_error("Only one parameter is handled for forall function (needs to be improved)");
    auto& forAllParameter = forAllParameters.front();

    ++itArg;
    auto& secondArg = *itArg;
    if (pExpressionParsed.arguments.size() == 3)
    {
      ++itArg;
      auto& thridArg = *itArg;
      res = std::make_unique<WorldStateModificationNode>(WorldStateModificationNodeType::FOR_ALL,
                                                         std::make_unique<WorldStateModificationFact>(secondArg.toFact(pOntology, pEntities, newParameters, false)),
                                                         _expressionParsedToWsModification(thridArg, pOntology, pEntities, newParameters, false),
                                                         forAllParameter);
    }
    else if (secondArg.name == _whenWsFunctionName &&
             secondArg.arguments.size() == 2)
    {
      auto itWhenArg = secondArg.arguments.begin();
      auto& firstWhenArg = *itWhenArg;
      ++itWhenArg;
      auto& secondWhenArg = *itWhenArg;
      res = std::make_unique<WorldStateModificationNode>(WorldStateModificationNodeType::FOR_ALL,
                                                         std::make_unique<WorldStateModificationFact>(firstWhenArg.toFact(pOntology, pEntities, newParameters, false)),
                                                         _expressionParsedToWsModification(secondWhenArg, pOntology, pEntities, newParameters, false),
                                                         forAllParameter);
    }
  }
  else if (pExpressionParsed.name == _andWsFunctionName &&
           pExpressionParsed.arguments.size() >= 2)
  {
    std::list<std::unique_ptr<WorldStateModification>> elts;
    for (auto& currExp : pExpressionParsed.arguments)
      elts.emplace_back(_expressionParsedToWsModification(currExp, pOntology, pEntities, pParameters, false));

    res = std::make_unique<WorldStateModificationNode>(WorldStateModificationNodeType::AND, std::move(*(--(--elts.end()))), std::move(elts.back()));
    elts.pop_back();
    elts.pop_back();

    while (!elts.empty())
    {
      res = std::make_unique<WorldStateModificationNode>(WorldStateModificationNodeType::AND, std::move(elts.back()), std::move(res));
      elts.pop_back();
    }
  }
  else if ((pExpressionParsed.name == _increaseWsFunctionName || pExpressionParsed.name == _addWsFunctionName) &&
           pExpressionParsed.arguments.size() == 2)
  {
    auto itArg = pExpressionParsed.arguments.begin();
    auto& firstArg = *itArg;
    ++itArg;
    auto& secondArg = *itArg;
    std::unique_ptr<WorldStateModification> rightOpPtr;
    try {
      rightOpPtr = WorldStateModificationNumber::create(secondArg.name);
    }  catch (...) {}
    if (!rightOpPtr)
      rightOpPtr = _expressionParsedToWsModification(secondArg, pOntology, pEntities, pParameters, true);

    res = std::make_unique<WorldStateModificationNode>(WorldStateModificationNodeType::INCREASE,
                                                       _expressionParsedToWsModification(firstArg, pOntology, pEntities, pParameters, true),
                                                       std::move(rightOpPtr));
  }
  else if (pExpressionParsed.name == _decreaseWsFunctionName &&
           pExpressionParsed.arguments.size() == 2)
  {
    auto itArg = pExpressionParsed.arguments.begin();
    auto& firstArg = *itArg;
    ++itArg;
    auto& secondArg = *itArg;
    std::unique_ptr<WorldStateModification> rightOpPtr;
    try {
      rightOpPtr = WorldStateModificationNumber::create(secondArg.name);
    }  catch (...) {}
    if (!rightOpPtr)
      rightOpPtr = _expressionParsedToWsModification(secondArg, pOntology, pEntities, pParameters, true);

    res = std::make_unique<WorldStateModificationNode>(WorldStateModificationNodeType::DECREASE,
                                                       _expressionParsedToWsModification(firstArg, pOntology, pEntities, pParameters, true),
                                                       std::move(rightOpPtr));
  }
  else if (pExpressionParsed.name == _mutliplyWsFunctionName &&
           pExpressionParsed.arguments.size() == 2)
  {
    auto itArg = pExpressionParsed.arguments.begin();
    auto& firstArg = *itArg;
    bool leftIsNumber = false;
    std::unique_ptr<WorldStateModification> leftOpPtr;
    try {
      leftOpPtr = WorldStateModificationNumber::create(firstArg.name);
      leftIsNumber = true;
    }  catch (...) {}
    if (!leftOpPtr)
      leftOpPtr = _expressionParsedToWsModification(firstArg, pOntology, pEntities, pParameters, true);

    ++itArg;
    auto& secondArg = *itArg;
    bool rightIsNumber = false;
    std::unique_ptr<WorldStateModification> rightOpPtr;
    try {
      rightOpPtr = WorldStateModificationNumber::create(secondArg.name);
      rightIsNumber = true;
    }  catch (...) {}
    if (!rightOpPtr)
      rightOpPtr = _expressionParsedToWsModification(secondArg, pOntology, pEntities, pParameters, true);

    if (leftIsNumber && !rightIsNumber)
      res = std::make_unique<WorldStateModificationNode>(WorldStateModificationNodeType::MULTIPLY,
                                                         std::move(rightOpPtr),
                                                         std::move(leftOpPtr));
    else
      res = std::make_unique<WorldStateModificationNode>(WorldStateModificationNodeType::MULTIPLY,
                                                         std::move(leftOpPtr),
                                                         std::move(rightOpPtr));
  }
  else if (pExpressionParsed.name == _whenWsFunctionName &&
           pExpressionParsed.arguments.size() == 2)
  {
    auto itWhenArg = pExpressionParsed.arguments.begin();
    auto& firstWhenArg = *itWhenArg;
    ++itWhenArg;
    auto& secondWhenArg = *itWhenArg;
    res = std::make_unique<WorldStateModificationNode>(WorldStateModificationNodeType::WHEN,
                                                       std::make_unique<WorldStateModificationFact>(firstWhenArg.toFact(pOntology, pEntities, {}, false)),
                                                       _expressionParsedToWsModification(secondWhenArg, pOntology, pEntities, {}, false));
  }
  else
  {
    if (pExpressionParsed.arguments.empty() && pExpressionParsed.value == "")
    {
      try {
        res = WorldStateModificationNumber::create(pExpressionParsed.name);
      }  catch (...) {}
    }

    if (!res)
      res = std::make_unique<WorldStateModificationFact>(pExpressionParsed.toFact(pOntology, pEntities, pParameters, pIsOkIfFluentIsMissing));
  }

  if (pExpressionParsed.followingExpression)
  {
    auto nodeType = WorldStateModificationNodeType::AND;
    if (pExpressionParsed.separatorToFollowingExp == '+')
      nodeType = WorldStateModificationNodeType::PLUS;
    else if (pExpressionParsed.separatorToFollowingExp == '-')
      nodeType = WorldStateModificationNodeType::MINUS;
    res = std::make_unique<WorldStateModificationNode>(nodeType,
                                                       std::move(res),
                                                       _expressionParsedToWsModification(*pExpressionParsed.followingExpression,
                                                                                         pOntology, pEntities, pParameters, false));
  }

  return res;
}

std::vector<Parameter> _pddlToParameters(const std::string& pStr,
                                         std::size_t& pPos,
                                         const SetOfTypes& pSetOfTypes)
{
  std::vector<Parameter> res;
  auto strSize = pStr.size();
  ExpressionParsed::skipSpaces(pStr, pPos);
  if (pPos >= strSize)
    return res;

  if (pStr[pPos] != '(')
    throw std::runtime_error("Parameters does not start with '(' in: " + pStr.substr(pPos, strSize - pPos));

  ++pPos;
  ExpressionParsed::skipSpaces(pStr, pPos);
  std::size_t beginOfTokenPos = pPos;

  std::string parameterName;
  bool nextIsAParameterName = true;

  while (pPos < strSize)
  {
    if (pStr[pPos] == ')')
    {
      break;
    }
    else if (ExpressionParsed::isEndOfTokenSeparator(pStr[pPos]))
    {
      auto token = pStr.substr(beginOfTokenPos, pPos - beginOfTokenPos);
      if (token == "-")
      {
        nextIsAParameterName = false;
      }
      else if (nextIsAParameterName)
      {
        if (parameterName != "")
          res.emplace_back(parameterName, pSetOfTypes.nameToType("number"));
        parameterName = token;
      }
      else
      {
        res.emplace_back(parameterName, pSetOfTypes.nameToType(token));
        parameterName = "";
        nextIsAParameterName = true;
      }
      ++pPos;
      ExpressionParsed::skipSpaces(pStr, pPos);
      beginOfTokenPos = pPos;
      continue;
    }
    ++pPos;
  }

  // Manage last token
  auto lastToken = pStr.substr(beginOfTokenPos, pPos - beginOfTokenPos);
  if (lastToken != "")
  {
    if (nextIsAParameterName)
    {
      if (parameterName != "")
        res.emplace_back(parameterName, pSetOfTypes.nameToType("number"));
      res.emplace_back(lastToken, pSetOfTypes.nameToType("number"));
    }
    else
    {
      res.emplace_back(parameterName, pSetOfTypes.nameToType(lastToken));
    }
  }

  ++pPos;
  ExpressionParsed::skipSpaces(pStr, pPos);
  return res;
}



ExpressionParsed _extractConditionPart(const ExpressionParsed& pInput,
                                       ConditionPart pConditionPart)
{
  if (pInput.name == _andConditonFunctionName)
  {
    std::list<ExpressionParsed> newArguments;
    for (auto& currExp : pInput.arguments)
    {
      auto subRes = _extractConditionPart(currExp, pConditionPart);
      if (!subRes.empty())
        newArguments.push_back(std::move(subRes));
    }

    if (newArguments.size() == 1)
      return std::move(newArguments.front());
    if (newArguments.size() > 1)
    {
      auto res = pInput.clone();
      res.arguments.clear();
      for (const auto& arg : newArguments)
         res.arguments.emplace_back(arg.clone());
      return res;
    }
    return ExpressionParsed();
  }

  if (pInput.name == "at" &&
      pInput.arguments.size() == 2 &&
      pInput.arguments.front().name == "start")
  {
    if (pConditionPart == ConditionPart::AT_START)
        return (++pInput.arguments.begin())->clone();
    return ExpressionParsed();
  }

  if (pInput.name == "over" &&
      pInput.arguments.size() == 2 &&
      pInput.arguments.front().name == "all")
  {
    if (pConditionPart == ConditionPart::OVER_ALL)
        return (++pInput.arguments.begin())->clone();
    return ExpressionParsed();
  }

  throw std::runtime_error("Not a condition valid for a durative action: " + pInput.toStr());
}


ExpressionParsed _extractWsModificationPart(const ExpressionParsed& pInput,
                                            WsModificationPart pWsModificationPart)
{
  if (pInput.name == _andConditonFunctionName)
  {
    std::list<ExpressionParsed> newArguments;
    for (auto& currExp : pInput.arguments)
    {
      auto subRes = _extractWsModificationPart(currExp, pWsModificationPart);
      if (!subRes.empty())
        newArguments.push_back(std::move(subRes));
    }

    if (newArguments.size() == 1)
      return std::move(newArguments.front());
    if (newArguments.size() > 1)
    {
      auto res = pInput.clone();
      res.arguments.clear();
      for (const auto& arg : newArguments)
         res.arguments.emplace_back(arg.clone());
      return res;
    }
    return ExpressionParsed();
  }


  if (pInput.name == "at" &&
      pInput.arguments.size() == 2)
  {
    if (pInput.arguments.front().name == "start")
    {
      if (pWsModificationPart == WsModificationPart::AT_START)
          return (++pInput.arguments.begin())->clone();
      return ExpressionParsed();
    }

    if (pInput.arguments.front().name == "end")
    {
      if (pInput.tags.count("__POTENTIALLY") > 0)
      {
        if (pWsModificationPart == WsModificationPart::POTENTIALLY_AT_END)
          return (++pInput.arguments.begin())->clone();
      }
      else if (pWsModificationPart == WsModificationPart::AT_END)
          return (++pInput.arguments.begin())->clone();
      return ExpressionParsed();
    }
  }

  throw std::runtime_error("Not a condition valid for a durative action: " + pInput.toStr());
}


Axiom _pddlToAxiom(const std::string& pStr,
                   std::size_t& pPos,
                   const Ontology& pOntology)
{
  std::vector<Parameter> vars;
  std::unique_ptr<Condition> context;
  std::unique_ptr<Fact> impliesPtr;

  auto strSize = pStr.size();
  while (pPos < strSize && pStr[pPos] != ')')
  {
    auto beginPos = pPos;
    auto subToken = ExpressionParsed::parseTokenThatCanBeEmpty(pStr, pPos);
    if (subToken == "")
    {
      if (pPos > beginPos)
        continue;
      break;
    }

    if (subToken == ":vars")
      vars = _pddlToParameters(pStr, pPos, pOntology.types);
    else if (subToken == ":context")
      context = pddlToCondition(pStr, pPos, pOntology, {}, vars);
    else if (subToken == ":implies")
      impliesPtr = std::make_unique<Fact>(Fact::fromPddl(pStr, pOntology, {}, vars, pPos, &pPos));
    else
      throw std::runtime_error("Unknown axiom token \"" + subToken + "\" in: " + pStr.substr(beginPos, strSize - beginPos));
  }

  if (!impliesPtr)
    throw std::runtime_error("Missing implies for an axiom");
  return Axiom(std::move(context), std::move(*impliesPtr), std::move(vars));
}


Event _pddlToEvent(const std::string& pStr,
                   std::size_t& pPos,
                   const Ontology& pOntology)
{
  std::vector<Parameter> parameters;
  std::unique_ptr<Condition> precondition;
  std::unique_ptr<WorldStateModification> effect;

  auto strSize = pStr.size();
  while (pPos < strSize && pStr[pPos] != ')')
  {
    auto beginPos = pPos;
    auto subToken = ExpressionParsed::parseTokenThatCanBeEmpty(pStr, pPos);
    if (subToken == "")
    {
      if (pPos > beginPos)
        continue;
      break;
    }

    if (subToken == ":parameters")
      parameters = _pddlToParameters(pStr, pPos, pOntology.types);
    else if (subToken == ":precondition")
      precondition = pddlToCondition(pStr, pPos, pOntology, {}, parameters);
    else if (subToken == ":effect")
      effect = pddlToWsModification(pStr, pPos, pOntology, {}, parameters);
    else
      throw std::runtime_error("Unknown event token \"" + subToken + "\" in: " + pStr.substr(beginPos, strSize - beginPos));
  }

  if (!precondition)
    throw std::runtime_error("An event has no precondition");
  if (!effect)
    throw std::runtime_error("An event has no effect");
  return Event(std::move(precondition), std::move(effect), std::move(parameters));
}

Action _actionPddlToAction(const std::string& pStr,
                           std::size_t& pPos,
                           const Ontology& pOntology)
{
  std::vector<Parameter> parameters;
  std::unique_ptr<Condition> precondition;
  std::unique_ptr<WorldStateModification> effect;

  auto strSize = pStr.size();
  while (pPos < strSize && pStr[pPos] != ')')
  {
    auto beginPos = pPos;
    auto subToken = ExpressionParsed::parseTokenThatCanBeEmpty(pStr, pPos);
    if (subToken == "")
    {
      if (pPos > beginPos)
        continue;
      break;
    }

    if (subToken == ":parameters")
      parameters = _pddlToParameters(pStr, pPos, pOntology.types);
    else if (subToken == ":precondition")
      precondition = pddlToCondition(pStr, pPos, pOntology, {}, parameters);
    else if (subToken == ":effect")
      effect = pddlToWsModification(pStr, pPos, pOntology, {}, parameters);
    else
      throw std::runtime_error("Unknown axiom token \"" + subToken + "\" in: " + pStr.substr(beginPos, strSize - beginPos));
  }

  if (!effect)
    throw std::runtime_error("An action has no effect");
  auto res = Action(std::move(precondition), std::move(effect));
  res.parameters = std::move(parameters);
  return res;
}


Action _durativeActionPddlToAction(const std::string& pStr,
                                   std::size_t& pPos,
                                   const Ontology& pOntology)
{
  std::vector<Parameter> parameters;
  std::unique_ptr<Condition> precondition;
  std::unique_ptr<Condition> overAllCondition;
  std::unique_ptr<WorldStateModification> effectAtStart;
  std::unique_ptr<WorldStateModification> effectAtEnd;
  std::unique_ptr<WorldStateModification> effectAtEndPotentially;

  auto strSize = pStr.size();
  while (pPos < strSize && pStr[pPos] != ')')
  {
    auto beginPos = pPos;
    auto subToken = ExpressionParsed::parseTokenThatCanBeEmpty(pStr, pPos);
    if (subToken == "")
    {
      if (pPos > beginPos)
        continue;
      break;
    }

    if (subToken == ":parameters")
    {
      parameters = _pddlToParameters(pStr, pPos, pOntology.types);
    }
    else if (subToken == ":duration")
    {
      ExpressionParsed::moveUntilClosingParenthesis(pStr, pPos);
      ++pPos;
    }
    else if (subToken == ":condition")
    {
      auto expressionParsed = ExpressionParsed::fromPddl(pStr, pPos, false);

      auto atStartExpressionParsed = _extractConditionPart(expressionParsed, ConditionPart::AT_START);
      if (!atStartExpressionParsed.empty())
        precondition = _expressionParsedToCondition(atStartExpressionParsed, pOntology, {}, parameters, false);

      auto overAllExpressionParsed = _extractConditionPart(expressionParsed, ConditionPart::OVER_ALL);
      if (!overAllExpressionParsed.empty())
        overAllCondition = _expressionParsedToCondition(overAllExpressionParsed, pOntology, {}, parameters, false);
    }
    else if (subToken == ":effect")
    {
      auto expressionParsed = ExpressionParsed::fromPddl(pStr, pPos, false);

      auto atStartExpressionParsed = _extractWsModificationPart(expressionParsed, WsModificationPart::AT_START);
      if (!atStartExpressionParsed.empty())
        effectAtStart = _expressionParsedToWsModification(atStartExpressionParsed, pOntology, {}, parameters, false);

      auto atEndExpressionParsed = _extractWsModificationPart(expressionParsed, WsModificationPart::AT_END);
      if (!atEndExpressionParsed.empty())
        effectAtEnd = _expressionParsedToWsModification(atEndExpressionParsed, pOntology, {}, parameters, false);

      auto potentiallyAtEndExpressionParsed = _extractWsModificationPart(expressionParsed, WsModificationPart::POTENTIALLY_AT_END);
      if (!potentiallyAtEndExpressionParsed.empty())
        effectAtEndPotentially = _expressionParsedToWsModification(potentiallyAtEndExpressionParsed, pOntology, {}, parameters, false);
    }
    else
    {
      throw std::runtime_error("Unknown axiom token \"" + subToken + "\" in: " + pStr.substr(beginPos, strSize - beginPos));
    }
  }

  if (!effectAtStart && !effectAtEnd && !effectAtEndPotentially)
    throw std::runtime_error("A durative action has no effect");
  auto res = Action(std::move(precondition), std::move(effectAtEnd));
  if (overAllCondition)
    res.overAllCondition = std::move(overAllCondition);
  if (effectAtStart)
    res.effect.worldStateModificationAtStart = std::move(effectAtStart);
  if (effectAtEndPotentially)
    res.effect.potentialWorldStateModification = std::move(effectAtEndPotentially);
  res.parameters = std::move(parameters);
  return res;
}

}

Domain pddlToDomain(const std::string& pStr,
                    const std::map<std::string, Domain>& pPreviousDomains)
{
  std::string domainName = "";
  ogp::Ontology ontology;
  std::map<ActionId, Action> actions;
  std::map<SetOfEventsId, SetOfEvents> idToSetOfEvents;
  SetOfConstFacts timelessFacts;
  std::set<std::string> requirements;

  const std::string defineToken = "(define";
  std::size_t found = pStr.find(defineToken);
  if (found != std::string::npos)
  {
    auto strSize = pStr.size();
    std::size_t pos = found + defineToken.size();

    while (pos < strSize)
    {
      if (pStr[pos] == ';')
      {
        ExpressionParsed::moveUntilEndOfLine(pStr, pos);
        ++pos;
        continue;
      }

      if (pStr[pos] == '(')
      {
        ++pos;
        auto token = ExpressionParsed::parseToken(pStr, pos);

        if (token == "domain")
        {
          domainName = ExpressionParsed::parseToken(pStr, pos);
        }
        else if (token == ":extends")
        {
          while (pos < strSize && pStr[pos] != ')')
          {
            auto domainToExtend = ExpressionParsed::parseToken(pStr, pos);
            auto itDomain = pPreviousDomains.find(domainToExtend);
            if (itDomain == pPreviousDomains.end())
              throw std::runtime_error("Domain \"" + domainToExtend + "\" is unknown!");
            const Domain& domainToExtand = itDomain->second;
            ontology = domainToExtand.getOntology();
            actions = domainToExtand.getActions();
            idToSetOfEvents = domainToExtand.getSetOfEvents();
            timelessFacts = domainToExtand.getTimelessFacts();
          }
        }
        else if (token == ":requirements")
        {
          while (pos < strSize && pStr[pos] != ')')
            requirements.insert(ExpressionParsed::parseToken(pStr, pos));
        }
        else if (token == ":types")
        {
          std::size_t beginPos = pos;
          ExpressionParsed::moveUntilClosingParenthesis(pStr, pos);
          std::string typesStr = pStr.substr(beginPos, pos - beginPos);
          ontology.types.addTypesFromPddl(typesStr);
        }
        else if (token == ":constants")
        {
          std::size_t beginPos = pos;
          ExpressionParsed::moveUntilClosingParenthesis(pStr, pos);
          std::string constantsStr = pStr.substr(beginPos, pos - beginPos);
          ontology.constants.addAllFromPddl(constantsStr, ontology.types);
        }
        else if (token == ":predicates")
        {
          ontology.predicates.addAll(ogp::SetOfPredicates::fromPddl(pStr, pos, ontology.types));
        }
        else if (token == ":functions")
        {
          auto numberType = ontology.types.nameToType("number");
          ontology.predicates.addAll(ogp::SetOfPredicates::fromPddl(pStr, pos, ontology.types, numberType));
        }
        else if (token == ":timeless")
        {
          timelessFacts = ogp::SetOfConstFacts::fromPddl(pStr, pos, ontology, {});
        }
        else if (token == ":axiom")
        {
          auto axiom = _pddlToAxiom(pStr, pos, ontology);
          for (auto& currEvent : axiom.toEvents(ontology, {}))
            idToSetOfEvents[Domain::getSetOfEventsIdFromConstructor()].add(currEvent, "from_axiom");
        }
        else if (token == ":event")
        {
          auto eventName = ExpressionParsed::parseToken(pStr, pos);
          auto event = _pddlToEvent(pStr, pos, ontology);
          idToSetOfEvents[Domain::getSetOfEventsIdFromConstructor()].add(event, eventName);
        }
        else if (token == ":action")
        {
          auto actionName = ExpressionParsed::parseToken(pStr, pos);
          auto action = _actionPddlToAction(pStr, pos, ontology);
          actions.emplace(actionName, std::move(action));
        }
        else if (token == ":durative-action")
        {
          auto actionName = ExpressionParsed::parseToken(pStr, pos);
          auto action = _durativeActionPddlToAction(pStr, pos, ontology);
          actions.emplace(actionName, std::move(action));
        }
        else
        {
          throw std::runtime_error("Unknown domain PDDL token: \"" + token + "\"");
        }
      }

      ++pos;
    }

  } else {
    throw std::runtime_error("No '(define' found in domain file");
  }
  auto res = Domain(actions, ontology, {}, idToSetOfEvents, timelessFacts, domainName);
  for (auto& currRequirement : requirements)
    res.addRequirement(currRequirement);
  return res;
}


DomainAndProblemPtrs pddlToProblem(const std::string& pStr,
                                   const std::map<std::string, Domain>& pLoadedDomains)
{
  DomainAndProblemPtrs res;
  std::string problemName;

  const std::string defineToken = "(define";
  std::size_t found = pStr.find(defineToken);
  if (found != std::string::npos)
  {
    auto strSize = pStr.size();
    std::size_t pos = found + defineToken.size();

    while (pos < strSize)
    {
      if (pStr[pos] == ';')
      {
        ExpressionParsed::moveUntilEndOfLine(pStr, pos);
        ++pos;
        continue;
      }

      if (pStr[pos] == '(')
      {
        ++pos;
        auto token = ExpressionParsed::parseToken(pStr, pos);

        if (token == "problem")
        {
          problemName = ExpressionParsed::parseToken(pStr, pos);
        }
        else if (token == ":domain")
        {
          while (pos < strSize && pStr[pos] != ')')
          {
            auto domainToExtend = ExpressionParsed::parseToken(pStr, pos);
            auto itDomain = pLoadedDomains.find(domainToExtend);
            if (itDomain == pLoadedDomains.end())
              throw std::runtime_error("Domain \"" + domainToExtend + "\" is unknown!");
            res.domainPtr = std::make_unique<Domain>(itDomain->second);
            res.problemPtr = std::make_unique<Problem>(&res.domainPtr->getTimelessFacts().setOfFacts());
          }
        }
        else if (token == ":objects")
        {
          if (!res.domainPtr)
            throw std::runtime_error("problem objects are defined before the domain.");
          const auto& ontology = res.domainPtr->getOntology();
          std::size_t beginPos = pos;
          ExpressionParsed::moveUntilClosingParenthesis(pStr, pos);
          std::string entitiesStr = pStr.substr(beginPos, pos - beginPos);
          res.problemPtr->entities.addAllFromPddl(entitiesStr, ontology.types);
        }
        else if (token == ":init")
        {
          if (!res.domainPtr)
            throw std::runtime_error("problem init are defined before the domain.");
          const auto& ontology = res.domainPtr->getOntology();
          auto& setOfEventsMap = res.domainPtr->getSetOfEvents();
          const SetOfCallbacks callbacks;
          res.problemPtr->worldState.modifyFactsFromPddl(pStr, pos, res.problemPtr->goalStack,
                                                         setOfEventsMap, callbacks,
                                                         ontology, res.problemPtr->entities, {});
        }
        else if (token == ":goal")
        {
          if (!res.domainPtr)
            throw std::runtime_error("problem objects are defined before the domain.");
          if (!res.problemPtr)
            throw std::runtime_error("problem not initialized before to set the goals.");
          const auto& ontology = res.domainPtr->getOntology();
          std::vector<Goal> goals;
          const auto& worldState = res.problemPtr->worldState;
          const auto& entities = res.problemPtr->entities;

          auto expressionParsed = ExpressionParsed::fromPddl(pStr, pos, false);
          if (expressionParsed.name == "and" && expressionParsed.tags.count("__ORDERED") > 0)
          {
            for (auto& currGoalExpParsed : expressionParsed.arguments)
            {
              auto goalPtr = _expressionParsedToGoal(currGoalExpParsed, ontology, entities, -1, "");
              if (!goalPtr)
                throw std::runtime_error("Failed to parse a pddl goal");
              goals.emplace_back(std::move(*goalPtr));
            }
          }
          else
          {
            auto goalPtr = _expressionParsedToGoal(expressionParsed, ontology, entities, -1, "");
            if (!goalPtr)
              throw std::runtime_error("Failed to parse a pddl goal");
            goals.emplace_back(std::move(*goalPtr));
          }

          res.problemPtr->goalStack.addGoals(goals, worldState, ontology.constants, entities, {});
        }
        else if (token == ":constraints")
        {
          break;
        }
        else
        {
          throw std::runtime_error("Unknown domain PDDL token: \"" + token + "\"");
        }
      }

      ++pos;
    }

  } else {
    throw std::runtime_error("No '(define' found in domain file");
  }

  if (!res.problemPtr)
    throw std::runtime_error("problem not initialized");
  res.problemPtr->name = problemName;
  return res;
}


std::unique_ptr<Condition> pddlToCondition(const std::string& pStr,
                                           std::size_t& pPos,
                                           const Ontology& pOntology,
                                           const SetOfEntities& pEntities,
                                           const std::vector<Parameter>& pParameters)
{
  auto expressionParsed = ExpressionParsed::fromPddl(pStr, pPos, false);
  return _expressionParsedToCondition(expressionParsed, pOntology, pEntities, pParameters, false);
}


std::unique_ptr<Condition> strToCondition(const std::string& pStr,
                                          const Ontology& pOntology,
                                          const SetOfEntities& pEntities,
                                          const std::vector<Parameter>& pParameters)
{
  if (pStr.empty())
    return {};
  std::size_t pos = 0;
  auto expressionParsed = ExpressionParsed::fromStr(pStr, pos);
  return _expressionParsedToCondition(expressionParsed, pOntology, pEntities, pParameters, false);
}


std::unique_ptr<Goal> pddlToGoal(const std::string& pStr,
                                 std::size_t& pPos,
                                 const Ontology& pOntology,
                                 const SetOfEntities& pEntities,
                                 int pMaxTimeToKeepInactive,
                                 const std::string& pGoalGroupId)
{
  if (pStr.empty())
    return {};
  auto expressionParsed = ExpressionParsed::fromPddl(pStr, pPos, false);
  return _expressionParsedToGoal(expressionParsed, pOntology, pEntities,
                                 pMaxTimeToKeepInactive, pGoalGroupId);
}

std::unique_ptr<Goal> strToGoal(const std::string& pStr,
                                const Ontology& pOntology,
                                const SetOfEntities& pEntities,
                                int pMaxTimeToKeepInactive,
                                const std::string& pGoalGroupId)
{
  if (pStr.empty())
    return {};
  std::size_t pos = 0;
  auto expressionParsed = ExpressionParsed::fromStr(pStr, pos);
  return _expressionParsedToGoal(expressionParsed, pOntology, pEntities,
                                 pMaxTimeToKeepInactive, pGoalGroupId);
}

std::unique_ptr<WorldStateModification> pddlToWsModification(const std::string& pStr,
                                                             std::size_t& pPos,
                                                             const Ontology& pOntology,
                                                             const SetOfEntities& pEntities,
                                                             const std::vector<Parameter>& pParameters)
{
  auto expressionParsed = ExpressionParsed::fromPddl(pStr, pPos, false);
  return _expressionParsedToWsModification(expressionParsed, pOntology, pEntities, pParameters, false);
}


std::unique_ptr<WorldStateModification> strToWsModification(const std::string& pStr,
                                                             const Ontology& pOntology,
                                                             const SetOfEntities& pEntities,
                                                             const std::vector<Parameter>& pParameters)
{
  if (pStr.empty())
    return {};
  std::size_t pos = 0;
  auto expressionParsed = ExpressionParsed::fromStr(pStr, pos);
  return _expressionParsedToWsModification(expressionParsed, pOntology, pEntities, pParameters, false);
}


std::vector<Parameter> pddlToParameters(const std::string& pStr,
                                        const SetOfTypes& pSetOfTypes)
{
  std::size_t pos = 0;
  return _pddlToParameters(pStr, pos, pSetOfTypes);
}



}
