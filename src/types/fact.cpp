#include <orderedgoalsplanner/types/fact.hpp>
#include <algorithm>
#include <memory>
#include <assert.h>
#include <optional>
#include <stdexcept>
#include <orderedgoalsplanner/types/factoptional.hpp>
#include <orderedgoalsplanner/types/ontology.hpp>
#include <orderedgoalsplanner/types/parameter.hpp>
#include <orderedgoalsplanner/types/setoffacts.hpp>
#include <orderedgoalsplanner/util/util.hpp>
#include "expressionParsed.hpp"

namespace ogp
{
namespace {

Fact _expressionParsedToFact(const ExpressionParsed& pExpressionParsed,
                             const Ontology& pOntology,
                             const SetOfEntities& pObjects,
                             const std::vector<Parameter>& pParameters,
                             bool& pIsValueNegated,
                             bool pIsOkIfValueIsMissing,
                             const std::map<std::string, Entity>* pParameterNamesToEntityPtr);

void _entitiesToStr(std::string& pStr,
                    const std::vector<FactArgument>& pParameters)
{
  bool firstIteration = true;
  for (auto& param : pParameters)
  {
    if (firstIteration)
      firstIteration = false;
    else
      pStr += ", ";
    pStr += param.toStr();
  }
}

void _entitiesToValueStr(std::string& pStr,
                         const std::vector<FactArgument>& pParameters,
                         const std::string& pSeparator)
{
  bool firstIteration = true;
  for (auto& param : pParameters)
  {
    if (firstIteration)
      firstIteration = false;
    else
      pStr += pSeparator;
    pStr += param.toStr();
  }
}

bool _isInside(const Entity& pEntity,
               const std::vector<Parameter>* pParametersPtr)
{
  if (pParametersPtr == nullptr)
    return false;
  for (auto& currParam : *pParametersPtr)
  {
    if (currParam.name == pEntity.value)
    {
      if (!pEntity.match(currParam))
        continue;
      return true;
    }
  }
  return false;
}

bool _isInside(const FactArgument& pArgument,
               const std::vector<Parameter>* pParametersPtr)
{
  return pArgument.isEntity() && _isInside(pArgument.entity(), pParametersPtr);
}

bool _isInside(const Entity& pEntity,
               const ParameterValuesWithConstraints* pEltsPtr)
{
  if (pEltsPtr == nullptr)
    return false;
  auto it = pEltsPtr->find(pEntity.toParameter());
  return it != pEltsPtr->end() && it->second.empty();
}

bool _isInside(const FactArgument& pArgument,
               const ParameterValuesWithConstraints* pEltsPtr)
{
  return pArgument.isEntity() && _isInside(pArgument.entity(), pEltsPtr);
}

bool _isInsideOrHasEntity(const Entity& pEntity,
                          const ParameterValuesWithConstraints* pEltsPtr,
                          const Entity& pEntityPossibility)
{
  if (pEltsPtr == nullptr)
    return false;
  auto it = pEltsPtr->find(pEntity.toParameter());
  if (it != pEltsPtr->end())
  {
    if (it->second.empty())
      return true;
    return it->second.count(pEntityPossibility) > 0;
  }
  return false;
}

bool _isInsideOrHasEntity(const FactArgument& pArgument,
                          const ParameterValuesWithConstraints* pEltsPtr,
                          const FactArgument& pEntityPossibility)
{
  return pArgument.isEntity() && pEntityPossibility.isEntity() &&
      _isInsideOrHasEntity(pArgument.entity(), pEltsPtr, pEntityPossibility.entity());
}

std::optional<Entity> _tryToResolveArgumentToEntity(const FactArgument& pArgument,
                                                    const SetOfFacts* pSetOfFactsPtr)
{
  if (pArgument.isEntity())
    return pArgument.entity();
  if (pSetOfFactsPtr != nullptr)
    return pArgument.tryToResolveToEntity(*pSetOfFactsPtr);
  return {};
}

bool _areEqualWithoutValueConsideration(const FactArgument& pArgument,
                                        const FactArgument& pOtherArgument,
                                        const ParameterValuesWithConstraints* pOtherFactParametersToConsiderAsAnyValuePtr,
                                        const ParameterValuesWithConstraints* pOtherFactParametersToConsiderAsAnyValuePtr2,
                                        const SetOfFacts* pSetOfFactsPtr)
{
  if (pArgument == pOtherArgument)
    return true;
  if (pArgument.isAnyEntity() || pOtherArgument.isAnyEntity())
    return true;

  auto resolvedArgument = _tryToResolveArgumentToEntity(pArgument, pSetOfFactsPtr);
  auto resolvedOtherArgument = _tryToResolveArgumentToEntity(pOtherArgument, pSetOfFactsPtr);
  if (resolvedArgument && resolvedOtherArgument)
  {
    return *resolvedArgument == *resolvedOtherArgument ||
        resolvedArgument->isAnyEntity() ||
        resolvedOtherArgument->isAnyEntity() ||
        _isInsideOrHasEntity(*resolvedOtherArgument, pOtherFactParametersToConsiderAsAnyValuePtr, *resolvedArgument) ||
        _isInsideOrHasEntity(*resolvedOtherArgument, pOtherFactParametersToConsiderAsAnyValuePtr2, *resolvedArgument);
  }

  return _isInsideOrHasEntity(pOtherArgument, pOtherFactParametersToConsiderAsAnyValuePtr, pArgument) ||
      _isInsideOrHasEntity(pOtherArgument, pOtherFactParametersToConsiderAsAnyValuePtr2, pArgument);
}

bool _areEqualExceptAnyEntities(const FactArgument& pArgument,
                                const FactArgument& pOtherArgument,
                                const ParameterValuesWithConstraints* pOtherFactParametersToConsiderAsAnyValuePtr,
                                const ParameterValuesWithConstraints* pOtherFactParametersToConsiderAsAnyValuePtr2,
                                const std::vector<Parameter>* pThisFactParametersToConsiderAsAnyValuePtr,
                                const SetOfFacts* pSetOfFactsPtr)
{
  if (pArgument == pOtherArgument)
    return true;
  if (pArgument.isAnyEntity() || pOtherArgument.isAnyEntity())
    return true;

  auto resolvedArgument = _tryToResolveArgumentToEntity(pArgument, pSetOfFactsPtr);
  auto resolvedOtherArgument = _tryToResolveArgumentToEntity(pOtherArgument, pSetOfFactsPtr);
  if (resolvedArgument && resolvedOtherArgument)
  {
    return *resolvedArgument == *resolvedOtherArgument ||
        resolvedArgument->isAnyEntity() ||
        resolvedOtherArgument->isAnyEntity() ||
        _isInside(*resolvedArgument, pThisFactParametersToConsiderAsAnyValuePtr) ||
        _isInside(*resolvedOtherArgument, pOtherFactParametersToConsiderAsAnyValuePtr) ||
        _isInside(*resolvedOtherArgument, pOtherFactParametersToConsiderAsAnyValuePtr2);
  }

  return _isInside(pArgument, pThisFactParametersToConsiderAsAnyValuePtr) ||
      _isInside(pOtherArgument, pOtherFactParametersToConsiderAsAnyValuePtr) ||
      _isInside(pOtherArgument, pOtherFactParametersToConsiderAsAnyValuePtr2);
}

std::optional<Entity> _tryToExtractArgumentFromExampleRec(const Parameter& pParameter,
                                                          const FactArgument& pArgument,
                                                          const FactArgument& pExampleArgument,
                                                          bool pIgnoreValue,
                                                          const SetOfFacts* pSetOfFactsPtr)
{
  if (pArgument.match(pParameter))
    return _tryToResolveArgumentToEntity(pExampleArgument, pSetOfFactsPtr);

  if (pArgument.fluent() != nullptr && pExampleArgument.fluent() != nullptr)
  {
    if (pIgnoreValue)
      return pArgument.fluent()->tryToExtractArgumentFromExampleWithoutValueConsideration(
            pParameter, *pExampleArgument.fluent(), pSetOfFactsPtr);
    return pArgument.fluent()->tryToExtractArgumentFromExample(
          pParameter, *pExampleArgument.fluent(), pSetOfFactsPtr);
  }

  return {};
}

bool _areTypesCompatible(const std::shared_ptr<Type>& pType1,
                         const std::shared_ptr<Type>& pType2)
{
  return !pType1 || !pType2 || pType1->isA(*pType2) || pType2->isA(*pType1);
}

bool _areEqualExceptAnyParameters(const FactArgument& pArgument,
                                  const FactArgument& pOtherArgument)
{
  if (pArgument == pOtherArgument)
    return true;
  if (pArgument.isAParameterToFill() || pOtherArgument.isAParameterToFill())
    return _areTypesCompatible(pArgument.type(), pOtherArgument.type());
  return false;
}


FactArgument _expressionParsedToFactArgument(const ExpressionParsed& pExpressionParsed,
                                             const Ontology& pOntology,
                                             const SetOfEntities& pObjects,
                                             const std::vector<Parameter>& pParameters,
                                             const std::map<std::string, Entity>* pParameterNamesToEntityPtr)
{
  if (pExpressionParsed.isAFunction)
  {
    bool isValueNegated = pExpressionParsed.isValueNegated;
    return FactArgument(_expressionParsedToFact(pExpressionParsed, pOntology, pObjects, pParameters,
                                                isValueNegated, true, pParameterNamesToEntityPtr));
  }
  return FactArgument(Entity::fromUsage(pExpressionParsed.name, pOntology, pObjects, pParameters, pParameterNamesToEntityPtr));
}


Fact _expressionParsedToFact(const ExpressionParsed& pExpressionParsed,
                             const Ontology& pOntology,
                             const SetOfEntities& pObjects,
                             const std::vector<Parameter>& pParameters,
                             bool& pIsValueNegated,
                             bool pIsOkIfValueIsMissing,
                             const std::map<std::string, Entity>* pParameterNamesToEntityPtr)
{
  std::string factName;
  std::vector<FactArgument> arguments;
  std::optional<Entity> value;
  const ExpressionParsed* expressionParsedForArgumentsPtr = &pExpressionParsed;

  if (pExpressionParsed.name.empty())
    throw std::runtime_error("Fact cannot have an empty name");

  if (pExpressionParsed.name == "=" && pExpressionParsed.arguments.size() == 2)
  {
    auto itArg = pExpressionParsed.arguments.begin();
    expressionParsedForArgumentsPtr = &*itArg;
    ++itArg;
    if (itArg->name == Fact::getUndefinedEntity().value)
    {
      value.emplace(Entity::createAnyEntity());
      pIsValueNegated = true;
    }
    else
    {
      value.emplace(Entity::fromUsage(itArg->name, pOntology, pObjects, pParameters, pParameterNamesToEntityPtr));
    }
    factName = expressionParsedForArgumentsPtr->name;
  }
  else
  {
    factName = pExpressionParsed.name;
    if (!pExpressionParsed.value.empty())
      value.emplace(Entity::fromUsage(pExpressionParsed.value, pOntology, pObjects, pParameters, pParameterNamesToEntityPtr));
  }

  for (const auto& currArgument : expressionParsedForArgumentsPtr->arguments)
    arguments.emplace_back(_expressionParsedToFactArgument(currArgument, pOntology, pObjects, pParameters, pParameterNamesToEntityPtr));

  return Fact(std::move(factName), std::move(arguments), std::move(value), pIsValueNegated, pOntology, pIsOkIfValueIsMissing);
}

}

Fact::Fact(const std::string& pStr,
           bool pStrPddlFormated,
           const Ontology& pOntology,
           const SetOfEntities& pObjects,
           const std::vector<Parameter>& pParameters,
           bool* pIsFactNegatedPtr,
           std::size_t pBeginPos,
           std::size_t* pResPos,
           bool pIsOkIfValueIsMissing)
  : predicate("_not_set", pStrPddlFormated, pOntology.types),
    _name(),
    _arguments(),
    _value(),
    _isValueNegated(false),
    _factSignature()
{
  std::size_t pos = pBeginPos;
  try
  {
    auto expressionParsed = pStrPddlFormated ?
        ExpressionParsed::fromPddl(pStr, pos, false) :
        ExpressionParsed::fromStr(pStr, pos);
    bool isFactNegated = expressionParsed.isValueNegated;
    if (!pStrPddlFormated && !expressionParsed.name.empty() && expressionParsed.name[0] == '!')
    {
      isFactNegated = true;
      expressionParsed.name = expressionParsed.name.substr(1, expressionParsed.name.size() - 1);
    }
    else if (pStrPddlFormated && expressionParsed.name == "not" && expressionParsed.arguments.size() == 1)
    {
      isFactNegated = true;
      expressionParsed = expressionParsed.arguments.back().clone();
    }

    *this = _expressionParsedToFact(expressionParsed, pOntology, pObjects, pParameters, isFactNegated, pIsOkIfValueIsMissing, nullptr);
    if (pIsFactNegatedPtr != nullptr)
        *pIsFactNegatedPtr = isFactNegated;
    if (pResPos != nullptr)
    {
      if (pos <= pBeginPos)
        throw std::runtime_error("Failed to parse a fact in str " + pStr.substr(pBeginPos, pStr.size() - pBeginPos));
      *pResPos = pos;
    }
  }
  catch (const std::exception& e)
  {
    throw std::runtime_error(std::string(e.what()) + ". The exception was thrown while parsing fact: \"" + pStr + "\"");
  }
}

Fact::Fact(const std::string& pName,
           const std::vector<FactArgument>& pArguments,
           const std::string& pValueStr,
           bool pIsValueNegated,
           const Ontology& pOntology,
           const SetOfEntities& pObjects,
           const std::vector<Parameter>& pParameters,
           bool pIsOkIfValueIsMissing,
           const std::map<std::string, Entity>* pParameterNamesToEntityPtr)
  : predicate("_not_set", true, pOntology.types),
    _name(pName),
    _arguments(pArguments),
    _value(),
    _isValueNegated(pIsValueNegated),
    _factSignature()
{
  auto* predicatePtr = pOntology.nameToPredicatePtr(_name);
  if (predicatePtr == nullptr)
    throw std::runtime_error("\"" + pName + "\" is not a predicate name or a derived predicate name");

  predicate = *predicatePtr;
  if (!pValueStr.empty())
    _value = Entity::fromUsage(pValueStr, pOntology, pObjects, pParameters, pParameterNamesToEntityPtr);
  else if (pIsOkIfValueIsMissing && predicate.value)
    _value = Entity(Entity::anyEntityValue(), predicate.value);
  _finalizeInisilizationAndValidityChecks(pIsOkIfValueIsMissing);
  _resetFactSignatureCache();
}

Fact::Fact(std::string pName,
           std::vector<FactArgument>&& pArguments,
           std::optional<Entity>&& pValue,
           bool pIsValueNegated,
           const Ontology& pOntology,
           bool pIsOkIfValueIsMissing)
  : predicate("_not_set", true, pOntology.types),
    _name(std::move(pName)),
    _arguments(std::move(pArguments)),
    _value(std::move(pValue)),
    _isValueNegated(pIsValueNegated),
    _factSignature()
{
  auto* predicatePtr = pOntology.nameToPredicatePtr(_name);
  if (predicatePtr == nullptr)
    throw std::runtime_error("\"" + _name + "\" is not a predicate name");

  predicate = *predicatePtr;
  _finalizeInisilizationAndValidityChecks(pIsOkIfValueIsMissing);
  _resetFactSignatureCache();
}


Fact::~Fact()
{
}

Fact::Fact(const Fact& pOther)
  : predicate(pOther.predicate),
    _name(pOther._name),
    _arguments(pOther._arguments),
    _value(pOther._value),
    _isValueNegated(pOther._isValueNegated),
    _factSignature(pOther._factSignature)
{
}

Fact::Fact(Fact&& pOther) noexcept
  : predicate(std::move(pOther.predicate)),
    _name(std::move(pOther._name)),
    _arguments(std::move(pOther._arguments)),
    _value(std::move(pOther._value)),
    _isValueNegated(std::move(pOther._isValueNegated)),
    _factSignature(std::move(pOther._factSignature))
{
}

Fact& Fact::operator=(const Fact& pOther) {
  predicate = pOther.predicate;
  _name = pOther._name;
  _arguments = pOther._arguments;
  _value = pOther._value;
  _isValueNegated = pOther._isValueNegated;
  _factSignature = pOther._factSignature;
  return *this;
}

Fact& Fact::operator=(Fact&& pOther) noexcept {
    predicate = std::move(pOther.predicate);
    _name = std::move(pOther._name);
    _arguments = std::move(pOther._arguments);
    _value = std::move(pOther._value);
    _isValueNegated = std::move(pOther._isValueNegated);
    _factSignature = std::move(pOther._factSignature);
    return *this;
}


bool Fact::operator<(const Fact& pOther) const
{
  if (_name != pOther._name)
    return _name < pOther._name;
  if (_value != pOther._value)
    return _value < pOther._value;
  if (_isValueNegated != pOther._isValueNegated)
    return _isValueNegated < pOther._isValueNegated;
  std::string paramStr;
  _entitiesToStr(paramStr, _arguments);
  std::string otherParamStr;
  _entitiesToStr(otherParamStr, pOther._arguments);
  return paramStr < otherParamStr;
}

bool Fact::operator==(const Fact& pOther) const
{
  return _name == pOther._name && _arguments == pOther._arguments &&
      _value == pOther._value && _isValueNegated == pOther._isValueNegated &&
      predicate == pOther.predicate;
}


std::ostream& operator<<(std::ostream& os, const Fact& pFact) {
    // Output the desired class members to the stream
    os << pFact.toStr();
    return os;  // Return the stream so it can be chained
}

bool Fact::areEqualWithoutValueConsideration(const Fact& pFact,
                                              const ParameterValuesWithConstraints* pOtherFactParametersToConsiderAsAnyValuePtr,
                                              const ParameterValuesWithConstraints* pOtherFactParametersToConsiderAsAnyValuePtr2,
                                              const SetOfFacts* pSetOfFactsPtr) const
{
  if (pFact._name != _name ||
      pFact._arguments.size() != _arguments.size())
    return false;

  auto itParam = _arguments.begin();
  auto itOtherParam = pFact._arguments.begin();
  while (itParam != _arguments.end())
  {
    if (!_areEqualWithoutValueConsideration(*itParam, *itOtherParam,
                                            pOtherFactParametersToConsiderAsAnyValuePtr,
                                            pOtherFactParametersToConsiderAsAnyValuePtr2,
                                            pSetOfFactsPtr))
      return false;
    ++itParam;
    ++itOtherParam;
  }

  return true;
}

bool Fact::areEqualWithoutAnArgConsideration(const Fact& pFact,
                                             const std::string& pArgToIgnore) const
{
  if (pFact._name != _name ||
      pFact._arguments.size() != _arguments.size() ||
      pFact._value != _value)
    return false;

  auto itParam = _arguments.begin();
  auto itOtherParam = pFact._arguments.begin();
  while (itParam != _arguments.end())
  {
    if (*itParam != *itOtherParam && !itParam->isAnyEntity() && !itOtherParam->isAnyEntity() &&
        itParam->valueStr() != pArgToIgnore)
      return false;
    ++itParam;
    ++itOtherParam;
  }

  return true;
}


bool Fact::areEqualWithoutArgsAndValueConsideration(const Fact& pFact,
                                                     const std::list<Parameter>* pParametersToIgnorePtr) const
{
  if (pFact._name != _name ||
      pFact._arguments.size() != _arguments.size())
    return false;

  auto itParam = _arguments.begin();
  auto itOtherParam = pFact._arguments.begin();
  while (itParam != _arguments.end())
  {
    if (*itParam != *itOtherParam && !itParam->isAnyEntity() && !itOtherParam->isAnyEntity())
    {
      if (pParametersToIgnorePtr != nullptr)
      {
        bool found = false;
        for (auto& currParam : *pParametersToIgnorePtr)
        {
          if (currParam.name == itParam->valueStr())
          {
            found = true;
            break;
          }
        }
        if (!found)
          return false;
      }
      else
      {
        return false;
      }
    }
    ++itParam;
    ++itOtherParam;
  }

  return true;
}


bool Fact::areEqualExceptAnyParameters(const Fact& pOther,
                                       bool pIgnoreValue) const
{
  if (_name != pOther._name || _arguments.size() != pOther._arguments.size())
    return false;

  auto itParam = _arguments.begin();
  auto itOtherParam = pOther._arguments.begin();
  while (itParam != _arguments.end())
  {
    if (!_areEqualExceptAnyParameters(*itParam, *itOtherParam))
      return false;
    ++itParam;
    ++itOtherParam;
  }

  if (pIgnoreValue)
    return true;
  if (!_value && !pOther._value)
    return _isValueNegated == pOther._isValueNegated;
  if (_value && _value->isAParameterToFill())
    return _isValueNegated == pOther._isValueNegated;
  if (pOther._value && pOther._value->isAParameterToFill())
    return _isValueNegated == pOther._isValueNegated;

  if ((_value && !pOther._value) || (!_value && pOther._value) || *_value != *pOther._value)
    return _isValueNegated != pOther._isValueNegated;

  return _isValueNegated == pOther._isValueNegated;
}


bool Fact::areEqualExceptAnyEntities(const Fact& pOther,
                                     const ParameterValuesWithConstraints* pOtherFactParametersToConsiderAsAnyValuePtr,
                                     const ParameterValuesWithConstraints* pOtherFactParametersToConsiderAsAnyValuePtr2,
                                     const std::vector<Parameter>* pThisFactParametersToConsiderAsAnyValuePtr,
                                     const SetOfFacts* pSetOfFactsPtr) const
{
  if (_name != pOther._name || _arguments.size() != pOther._arguments.size())
    return false;

  auto itParam = _arguments.begin();
  auto itOtherParam = pOther._arguments.begin();
  while (itParam != _arguments.end())
  {
    if (!_areEqualExceptAnyEntities(*itParam, *itOtherParam,
                                    pOtherFactParametersToConsiderAsAnyValuePtr,
                                    pOtherFactParametersToConsiderAsAnyValuePtr2,
                                    pThisFactParametersToConsiderAsAnyValuePtr,
                                    pSetOfFactsPtr))
      return false;
    ++itParam;
    ++itOtherParam;
  }

  if (!_value && !pOther._value)
    return _isValueNegated == pOther._isValueNegated;
  if (_value &&
      (_value->isAnyEntity() ||
       _isInside(*_value, pThisFactParametersToConsiderAsAnyValuePtr)))
    return _isValueNegated == pOther._isValueNegated;
  if (pOther._value &&
      (pOther._value->isAnyEntity() ||
       _isInside(*pOther._value, pOtherFactParametersToConsiderAsAnyValuePtr) ||
       _isInside(*pOther._value, pOtherFactParametersToConsiderAsAnyValuePtr2)))
    return _isValueNegated == pOther._isValueNegated;

  if ((_value && !pOther._value) || (!_value && pOther._value) || *_value != *pOther._value)
    return _isValueNegated != pOther._isValueNegated;

  return _isValueNegated == pOther._isValueNegated;
}


bool Fact::areEqualExceptAnyEntitiesAndValue(const Fact& pOther,
                                             const ParameterValuesWithConstraints* pOtherFactParametersToConsiderAsAnyValuePtr,
                                             const ParameterValuesWithConstraints* pOtherFactParametersToConsiderAsAnyValuePtr2,
                                             const SetOfFacts* pSetOfFactsPtr) const
{
  if (_name != pOther._name || _arguments.size() != pOther._arguments.size())
    return false;

  auto itParam = _arguments.begin();
  auto itOtherParam = pOther._arguments.begin();
  while (itParam != _arguments.end())
  {
    if (!_areEqualExceptAnyEntities(*itParam, *itOtherParam,
                                    pOtherFactParametersToConsiderAsAnyValuePtr,
                                    pOtherFactParametersToConsiderAsAnyValuePtr2,
                                    nullptr,
                                    pSetOfFactsPtr))
      return false;
    ++itParam;
    ++itOtherParam;
  }

  return _isValueNegated == pOther._isValueNegated;
}



bool Fact::areEqualExceptParametersAndValue(const Fact& pOther) const
{
  if (_name != pOther._name || _arguments.size() != pOther._arguments.size())
    return false;

  auto itParam = _arguments.begin();
  auto itOtherParam = pOther._arguments.begin();
  while (itParam != _arguments.end())
  {
    if (*itParam != *itOtherParam && !itParam->isAParameterToFill() && !itOtherParam->isAParameterToFill())
      return false;
    ++itParam;
    ++itOtherParam;
  }

  return _isValueNegated == pOther._isValueNegated;
}

bool Fact::doesFactEffectOfSuccessorGiveAnInterestForSuccessor(const Fact& pFact) const
{
  if (pFact._name != _name ||
      pFact._arguments.size() != _arguments.size() &&
      pFact._value.has_value() == _value.has_value())
    return true;

  auto itParam = _arguments.begin();
  auto itOtherParam = pFact._arguments.begin();
  while (itParam != _arguments.end())
  {
    if (!(itParam->isAnyEntity() && itOtherParam->isAnyEntity()) &&
        (itParam->isAParameterToFill() || itOtherParam->isAParameterToFill() || *itParam != *itOtherParam))
      return true;
    ++itParam;
    ++itOtherParam;
  }

  if (pFact._value.has_value() && _value.has_value())
    return *pFact._value != *_value && !(pFact._value->isAParameterToFill() && _value->isAParameterToFill());
  return false;
}

bool Fact::isPunctual() const
{
  const auto& punctualPrefix = getPunctualPrefix();
  return _name.compare(0, punctualPrefix.size(), punctualPrefix) == 0;
}


bool Fact::hasParameterOrValue(const Parameter& pParameter) const
{
  if (_value && _value->match(pParameter))
    return true;

  auto itParam = _arguments.begin();
  while (itParam != _arguments.end())
  {
    if (itParam->match(pParameter) || (itParam->fluent() && itParam->fluent()->hasParameterOrValue(pParameter)))
      return true;
    ++itParam;
  }
  return false;
}


bool Fact::hasAParameter(bool pIgnoreValue) const
{
  for (const auto& currArg : _arguments)
    if (currArg.hasParameter())
      return true;
  return !pIgnoreValue && _value && _value->isAParameterToFill();
}


bool Fact::hasAFluentArgument() const
{
  for (const auto& currArg : _arguments)
    if (currArg.isFluent())
      return true;
  return false;
}


bool Fact::hasEntity(const std::string& pEntityId) const
{
  for (const auto& currArg : _arguments)
    if (currArg.hasEntity(pEntityId))
      return true;
  return _value && _value->value == pEntityId && !_value->isAParameterToFill();
}


std::optional<Fact> Fact::tryToResolveFluentArguments(const SetOfFacts& pSetOfFacts) const
{
  if (!hasAFluentArgument())
    return *this;

  std::vector<FactArgument> resolvedArguments;
  resolvedArguments.reserve(_arguments.size());
  for (const auto& currArg : _arguments)
  {
    auto resolvedEntity = currArg.tryToResolveToEntity(pSetOfFacts);
    if (!resolvedEntity)
      return {};
    resolvedArguments.emplace_back(std::move(*resolvedEntity));
  }
  auto res = *this;
  res._arguments = std::move(resolvedArguments);
  res._resetFactSignatureCache();
  return res;
}


std::optional<Entity> Fact::tryToExtractArgumentFromExample(const Parameter& pParameter,
                                                            const Fact& pExampleFact,
                                                            const SetOfFacts* pSetOfFactsPtr) const
{
  if (_name != pExampleFact._name ||
      _isValueNegated != pExampleFact._isValueNegated ||
      _arguments.size() != pExampleFact._arguments.size())
    return {};

  std::optional<Entity> res;
  if (_value && pExampleFact._value && _value->match(pParameter))
    res = *pExampleFact._value;

  auto itParam = _arguments.begin();
  auto itOtherParam = pExampleFact._arguments.begin();
  while (itParam != _arguments.end())
  {
    auto argValue = _tryToExtractArgumentFromExampleRec(
          pParameter, *itParam, *itOtherParam, false, pSetOfFactsPtr);
    if (argValue)
      res = std::move(argValue);
    ++itParam;
    ++itOtherParam;
  }
  return res;
}

std::optional<Entity> Fact::tryToExtractArgumentFromExampleWithoutValueConsideration(
    const Parameter& pParameter,
    const Fact& pExampleFact,
    const SetOfFacts* pSetOfFactsPtr) const
{
  if (_name != pExampleFact._name ||
      _isValueNegated != pExampleFact._isValueNegated ||
      _arguments.size() != pExampleFact._arguments.size())
    return {};

  std::optional<Entity> res;
  auto itArg = _arguments.begin();
  auto itOtherArg = pExampleFact._arguments.begin();
  while (itArg != _arguments.end())
  {
    auto argValue = _tryToExtractArgumentFromExampleRec(
          pParameter, *itArg, *itOtherArg, true, pSetOfFactsPtr);
    if (argValue)
      res = std::move(argValue);
    ++itArg;
    ++itOtherArg;
  }
  return res;
}


void Fact::replaceArguments(const std::map<Parameter, Entity>& pCurrentArgumentsToNewArgument)
{
  if (_value)
  {
    auto itValueParam = pCurrentArgumentsToNewArgument.find(_value->toParameter());
    if (itValueParam != pCurrentArgumentsToNewArgument.end())
      _value = itValueParam->second;
  }

  for (auto& currParam : _arguments)
    currParam.replaceArguments(pCurrentArgumentsToNewArgument);
  _resetFactSignatureCache();
}

void Fact::replaceArguments(const ParameterValuesWithConstraints& pCurrentArgumentsToNewArgument)
{
  if (_value)
  {
    auto itValueParam = pCurrentArgumentsToNewArgument.find(_value->toParameter());
    if (itValueParam != pCurrentArgumentsToNewArgument.end() && !itValueParam->second.empty())
      _value = itValueParam->second.begin()->first;
  }

  for (auto& currParam : _arguments)
    currParam.replaceArguments(pCurrentArgumentsToNewArgument);
  _resetFactSignatureCache();
}

std::string Fact::toPddl(bool pInEffectContext,
                         bool pPrintAnyValue) const
{
  std::string res = "(" + _name;
  if (!_arguments.empty())
  {
    res += " ";
    bool firstIteration = true;
    for (const auto& currArg : _arguments)
    {
      if (firstIteration)
        firstIteration = false;
      else
        res += " ";
      if (currArg.isEntity())
        res += currArg.entity().value;
      else
        res += currArg.toPddl(pPrintAnyValue);
    }
  }
  res += ")";
  if (_value)
  {
    if (!pPrintAnyValue && _value->isAnyEntity())
      return res;
    res = (pInEffectContext ? "(assign " : "(= ") + res + " " + _value->value + ")";
    if (_isValueNegated)
    {
      if (pInEffectContext)
        throw std::runtime_error("Value should not be negated in effect: " + toStr(pPrintAnyValue));
      res += "(not " + res + ")";
    }
  }
  return res;
}

std::string Fact::toStr(bool pPrintAnyValue) const
{
  std::string res = _name;
  if (!_arguments.empty())
  {
    res += "(";
    bool firstIteration = true;
    for (const auto& currArg : _arguments)
    {
      if (firstIteration)
        firstIteration = false;
      else
        res += ", ";
      if (currArg.isEntity())
        res += currArg.entity().value;
      else
        res += currArg.toStr(pPrintAnyValue);
    }
    res += ")";
  }
  if (_value)
  {
    if (!pPrintAnyValue && _value->isAnyEntity())
      return res;
    if (_isValueNegated)
      res += "!=" + _value->value;
    else
      res += "=" + _value->value;
  }
  return res;
}


Fact Fact::fromStr(const std::string& pStr,
                   const Ontology& pOntology,
                   const SetOfEntities& pobjects,
                   const std::vector<Parameter>& pParameters,
                   bool* pIsFactNegatedPtr)
{
  return Fact(pStr, false, pOntology, pobjects, pParameters, pIsFactNegatedPtr);
}


Fact Fact::fromPddl(const std::string& pStr,
                    const Ontology& pOntology,
                    const SetOfEntities& pObjects,
                    const std::vector<Parameter>& pParameters,
                    std::size_t pBeginPos,
                    std::size_t* pResPos,
                    bool pIsOkIfValueIsMissing)
{
  return Fact(pStr, true, pOntology, pObjects, pParameters, nullptr, pBeginPos, pResPos, pIsOkIfValueIsMissing);
}


bool Fact::isInOtherFacts(const std::set<Fact>& pOtherFacts,
                          ParameterValuesWithConstraints* pNewParametersPtr,
                          const ParameterValuesWithConstraints* pParametersPtr,
                          ParameterValuesWithConstraints* pParametersToModifyInPlacePtr,
                          bool* pTriedToModifyParametersPtr) const
{
  bool res = false;
  ParameterValuesWithConstraints newPotentialParameters;
  ParameterValuesWithConstraints newPotentialParametersInPlace;
  for (const auto& currOtherFact : pOtherFacts)
    if (isInOtherFact(currOtherFact, newPotentialParameters,
                      pParametersPtr, newPotentialParametersInPlace, pParametersToModifyInPlacePtr))
      res = true;

  if (!res)
    return false;
  return updateParameters(newPotentialParameters, newPotentialParametersInPlace, pNewParametersPtr, false,
                          pParametersPtr, pParametersToModifyInPlacePtr, pTriedToModifyParametersPtr);
}


bool Fact::isInOtherFactsMap(const SetOfFacts& pOtherFacts,
                             ParameterValuesWithConstraints* pNewParametersPtr,
                             const ParameterValuesWithConstraints* pParametersPtr,
                             ParameterValuesWithConstraints* pParametersToModifyInPlacePtr,
                             bool* pTriedToModifyParametersPtr) const
{
  bool res = false;
  auto otherFactsThatMatched = pOtherFacts.find(*this);
  ParameterValuesWithConstraints newPotentialParameters;
  ParameterValuesWithConstraints newPotentialParametersInPlace;
  if (pParametersToModifyInPlacePtr != nullptr)
    newPotentialParametersInPlace = *pParametersToModifyInPlacePtr;
  for (const auto& currOtherFact : otherFactsThatMatched)
    if (isInOtherFact(currOtherFact, newPotentialParameters, pParametersPtr,
                      newPotentialParametersInPlace, pParametersToModifyInPlacePtr, &pOtherFacts))
      res = true;

  if (!res)
    return false;
  return updateParameters(newPotentialParameters, newPotentialParametersInPlace, pNewParametersPtr, false,
                          pParametersPtr, pParametersToModifyInPlacePtr, pTriedToModifyParametersPtr);
}


bool Fact::canModifySetOfFacts(const SetOfFacts& pOtherFacts,
                               ParameterValuesWithConstraints& pArgumentsToFilter) const
{
  bool res = true;
  auto otherFactsThatMatched = pOtherFacts.find(*this);
  ParameterValuesWithConstraints parametersAlreadyInSetOfFacts;
  for (const auto& currOtherFact : otherFactsThatMatched)
    if (filterPossibilities(currOtherFact, parametersAlreadyInSetOfFacts, pArgumentsToFilter, &pOtherFacts))
      res = false;
  return res || parametersAlreadyInSetOfFacts != pArgumentsToFilter;
}


bool Fact::updateParameters(ParameterValuesWithConstraints& pNewPotentialParameters,
                            ParameterValuesWithConstraints& pNewPotentialParametersInPlace,
                            ParameterValuesWithConstraints* pNewParametersPtr,
                            bool pCheckAllPossibilities,
                            const ParameterValuesWithConstraints* pParametersPtr,
                            ParameterValuesWithConstraints* pParametersToModifyInPlacePtr,
                            bool* pTriedToModifyParametersPtr) const
{
  if (pParametersToModifyInPlacePtr != nullptr)
    *pParametersToModifyInPlacePtr = std::move(pNewPotentialParametersInPlace);
  return _updateParameters(pNewParametersPtr, pNewPotentialParameters, pCheckAllPossibilities, pParametersPtr, pTriedToModifyParametersPtr);
}


bool Fact::_updateParameters(ParameterValuesWithConstraints* pNewParametersPtr,
                             ParameterValuesWithConstraints& pNewPotentialParameters,
                             bool pCheckAllPossibilities,
                             const ParameterValuesWithConstraints* pParametersPtr,
                             bool* pTriedToModifyParametersPtr) const
{
  if (pCheckAllPossibilities && pParametersPtr != nullptr && pNewPotentialParameters != *pParametersPtr)
    return false;

  if (!pNewPotentialParameters.empty())
  {
    if (pNewParametersPtr != nullptr)
    {
      if (pNewParametersPtr->empty())
      {
        *pNewParametersPtr = std::move(pNewPotentialParameters);
      }
      else
      {
        for (auto& currNewPotParam : pNewPotentialParameters)
          (*pNewParametersPtr)[currNewPotParam.first].insert(currNewPotParam.second.begin(), currNewPotParam.second.end());
      }
    }
    else if (pTriedToModifyParametersPtr != nullptr)
    {
      *pTriedToModifyParametersPtr = true;
    }
  }
  return true;
}


bool Fact::filterPossibilities(const Fact& pOtherFact,
                               ParameterValuesWithConstraints& pNewParameters,
                               const ParameterValuesWithConstraints& pParameters,
                               const SetOfFacts* pSetOfFactsPtr) const
{
  if (pOtherFact._name != _name ||
      pOtherFact._arguments.size() != _arguments.size())
    return false;

  ParameterValuesWithConstraints newPotentialParameters;
  auto doesItMatch = [&](const Entity& pFactValue, const Entity& pValueToLookFor) {
    if (pFactValue == pValueToLookFor ||
        pFactValue.isAnyEntity())
      return true;

    auto itParam = pParameters.find(pFactValue.toParameter());
    if (itParam != pParameters.end() &&
        (pValueToLookFor.type && itParam->first.type && pValueToLookFor.type->isA(*itParam->first.type)))
    {
      if (!itParam->second.empty() && itParam->second.count(pValueToLookFor) == 0)
        return false;

      newPotentialParameters[pFactValue.toParameter()][pValueToLookFor];
      return true;
    }

    return false;
  };

  {
    bool doesParametersMatches = true;
    auto itFactArguments = pOtherFact._arguments.begin();
    auto itLookForArguments = _arguments.begin();
    while (itFactArguments != pOtherFact._arguments.end())
    {
      if (*itFactArguments != *itLookForArguments &&
          ([&]() {
            auto lookForArgument = _tryToResolveArgumentToEntity(*itLookForArguments, pSetOfFactsPtr);
            auto factArgument = _tryToResolveArgumentToEntity(*itFactArguments, pSetOfFactsPtr);
            return !lookForArgument || !factArgument ||
                !doesItMatch(*lookForArgument, *factArgument);
          })())
        doesParametersMatches = false;
      ++itFactArguments;
      ++itLookForArguments;
    }
    if (!doesParametersMatches)
      return false;
  }

  std::optional<bool> resOpt;
  if (!_value && !pOtherFact._value)
  {
    resOpt.emplace(pOtherFact._isValueNegated == _isValueNegated);
  }
  else if (_value && pOtherFact._value)
  {
    if (doesItMatch(*_value, *pOtherFact._value))
      resOpt.emplace(pOtherFact._isValueNegated == _isValueNegated);
  }

  if (!resOpt)
    resOpt.emplace(pOtherFact._isValueNegated != _isValueNegated);

  if (*resOpt)
  {
    if (!newPotentialParameters.empty())
    {
      if (pNewParameters.empty())
      {
        pNewParameters = std::move(newPotentialParameters);
      }
      else
      {
        for (auto& currNewPotParam : newPotentialParameters)
          pNewParameters[currNewPotParam.first].insert(currNewPotParam.second.begin(), currNewPotParam.second.end());
      }
    }

    return true;
  }
  return false;
}


bool Fact::isInOtherFact(const Fact& pOtherFact,
                         ParameterValuesWithConstraints& pNewParameters,
                         const ParameterValuesWithConstraints* pParametersPtr,
                         ParameterValuesWithConstraints& pNewParametersInPlace,
                         const ParameterValuesWithConstraints* pParametersToModifyInPlacePtr,
                         const SetOfFacts* pSetOfFactsPtr) const
{
  if (pOtherFact._name != _name ||
      pOtherFact._arguments.size() != _arguments.size())
    return false;

  ParameterValuesWithConstraints newPotentialParameters;
  ParameterValuesWithConstraints newParametersInPlace;
  auto doesItMatch = [&](const Entity& pFactValue, const Entity& pValueToLookFor) {
    if (pFactValue == pValueToLookFor ||
        pFactValue.isAnyEntity())
      return true;

    if (pParametersPtr != nullptr)
    {
      auto itParam = pParametersPtr->find(pFactValue.toParameter());
      if (itParam != pParametersPtr->end() &&
          (!pValueToLookFor.type || !itParam->first.type || pValueToLookFor.type->isA(*itParam->first.type)))
      {
        if (!itParam->second.empty() && itParam->second.count(pValueToLookFor) == 0)
          return false;

        newPotentialParameters[pFactValue.toParameter()][pValueToLookFor];
        return true;
      }
    }

    if (pParametersToModifyInPlacePtr != nullptr)
    {
      auto itParam = pParametersToModifyInPlacePtr->find(pFactValue.toParameter());
      if (itParam != pParametersToModifyInPlacePtr->end() &&
          (!pValueToLookFor.type || !itParam->first.type || pValueToLookFor.type->isA(*itParam->first.type)))
      {
        if (!itParam->second.empty() && itParam->second.count(pValueToLookFor) == 0)
          return false;
        newParametersInPlace[pFactValue.toParameter()][pValueToLookFor];
        return true;
      }
    }

    return false;
  };

  {
    bool doesParametersMatches = true;
    auto itFactArguments = pOtherFact._arguments.begin();
    auto itLookForArguments = _arguments.begin();
    while (itFactArguments != pOtherFact._arguments.end())
    {
      if (*itFactArguments != *itLookForArguments &&
          ([&]() {
            auto lookForArgument = _tryToResolveArgumentToEntity(*itLookForArguments, pSetOfFactsPtr);
            auto factArgument = _tryToResolveArgumentToEntity(*itFactArguments, pSetOfFactsPtr);
            return !lookForArgument || !factArgument ||
                !doesItMatch(*lookForArgument, *factArgument);
          })())
        doesParametersMatches = false;
      ++itFactArguments;
      ++itLookForArguments;
    }
    if (!doesParametersMatches)
      return false;
  }

  std::optional<bool> resOpt;
  if (!_value && !pOtherFact._value)
  {
    resOpt.emplace(pOtherFact._isValueNegated == _isValueNegated);
  }
  else if (_value && pOtherFact._value)
  {
    if (doesItMatch(*_value, *pOtherFact._value))
      resOpt.emplace(pOtherFact._isValueNegated == _isValueNegated);
  }

  if (!resOpt)
    resOpt.emplace(pOtherFact._isValueNegated != _isValueNegated);

  if (*resOpt)
  {
    if (!newPotentialParameters.empty())
    {
      if (pNewParameters.empty())
      {
        pNewParameters = std::move(newPotentialParameters);
      }
      else
      {
        for (auto& currNewPotParam : newPotentialParameters)
          pNewParameters[currNewPotParam.first].insert(currNewPotParam.second.begin(), currNewPotParam.second.end());
      }
    }

    if (!newParametersInPlace.empty())
    {
      if (pNewParametersInPlace.empty())
      {
        pNewParametersInPlace = std::move(newParametersInPlace);
      }
      else
      {
        for (auto& currNewPotParam : newParametersInPlace)
          pNewParametersInPlace[currNewPotParam.first].insert(currNewPotParam.second.begin(), currNewPotParam.second.end());
      }
    }

    return true;
  }
  return false;
}


void Fact::replaceArgument(const Entity& pCurrent,
                           const Entity& pNew)
{
  for (auto& currParameter : _arguments)
    if (currParameter.isEntity() && currParameter.entity() == pCurrent)
      currParameter = FactArgument(pNew);
}


std::map<Parameter, Entity> Fact::extratParameterToArguments() const
{
  if (_arguments.size() == predicate.parameters.size())
  {
    std::map<Parameter, Entity> res;
    for (auto i = 0; i < _arguments.size(); ++i)
    {
      if (!_arguments[i].isEntity())
        throw std::runtime_error("Cannot extract fluent argument \"" + _arguments[i].toStr() + "\" as an entity from fact: " + toStr());
      res.emplace(predicate.parameters[i], _arguments[i].entity());
    }

    if (_value && predicate.value)
    {
      res.emplace(Parameter::fromType(predicate.value), *_value);
      return res;
    }
    if (!_value && !predicate.value)
      return res;
    throw std::runtime_error("Value difference between fact and predicate: " + toStr());
  }
  throw std::runtime_error("No same number of arguments vs predicate parameters: " + toStr());
}


std::string Fact::factSignature() const
{
  if (_factSignature != "")
    return _factSignature;

  if (ORDEREDGOALSPLANNER_DEBUG_FOR_TESTS)
    throw std::runtime_error("_factSignature2 is not set");
  return _generateFactSignature();
}

std::string Fact::_generateFactSignature() const
{
  auto res = _name;
  res += "(";
  bool firstArg = true;
  for (const auto& currArg : _arguments)
  {
    auto currArgType = currArg.type();
    if (currArgType)
    {
      if (firstArg)
        firstArg = false;
      else
        res += ", ";
      res += currArgType->name;
    }
  }
  res += ")";
  return res;
}


void Fact::generateSignaturesWithRelatedTypes(
    const std::function<void(const std::string&)>& pSignatureCallback,
    bool pIncludeSubTypes,
    bool pIncludeParentTypes) const
{
  // Gather all related types for each type in orderedTypes
  std::vector<std::set<std::shared_ptr<Type>>> allRelatedTypes;
  for (std::size_t i = 0; i < _arguments.size(); ++i)
  {
    const auto& currArg = _arguments[i];
    std::set<std::shared_ptr<Type>> relatedTypes;
    auto currArgType = currArg.type();
    relatedTypes.insert(currArgType);
    if (currArgType && pIncludeSubTypes)
      currArgType->getSubTypesRecursively(relatedTypes);
    if (currArgType && pIncludeParentTypes)
       currArgType->getParentTypesRecursively(relatedTypes);
    allRelatedTypes.push_back(std::move(relatedTypes));
  }

  // Generate all combinations
  std::vector<std::shared_ptr<Type>> currentCombination(allRelatedTypes.size());
  std::function<void(size_t)> backtrack = [&](size_t index) {
    if (index == allRelatedTypes.size()) {
      std::string newRes;
      for (const auto& currElt : currentCombination)
      {
        if (!newRes.empty())
          newRes += ", ";
        newRes += currElt->name;
      }
      pSignatureCallback(_name + "(" + newRes + ")");
      return;
    }

    for (const auto& type : allRelatedTypes[index]) {
      currentCombination[index] = type;
      backtrack(index + 1);
    }
  };
  backtrack(0);
}


void Fact::setArgumentType(std::size_t pIndex, const std::shared_ptr<Type>& pType)
{
  if (_arguments[pIndex].isEntity())
    _arguments[pIndex].entity().type = pType;
  _resetFactSignatureCache();
}

void Fact::setValueType(const std::shared_ptr<Type>& pType)
{
  if (_value)
  {
    _value->type = pType;
    _resetFactSignatureCache();
  }
}


void Fact::setValue(const std::optional<Entity>& pValue)
{
  _value = pValue;
  _resetFactSignatureCache();
}

void Fact::setValueFromStr(const std::string& pValueStr)
{
  if (_value)
    _value->value = pValueStr;
  else
    _value = Entity(pValueStr, predicate.value);
  _resetFactSignatureCache();
}

bool Fact::isCompleteWithAnyEntityValue() const
{
  if (_value && _value->isAnyEntity())
  {
    for (const auto& currArg : _arguments)
      if (currArg.isAParameterToFill())
        return false;
    return true;
  }
  return false;
}

void Fact::toImmutable()
{
  if (!predicate.isImmutable())
  {
    predicate = predicate.createImmutableCopy();
    _name = predicate.name;
    _resetFactSignatureCache();
  }
}


const Entity& Fact::getUndefinedEntity()
{
  static const auto undefinedValue = Entity("undefined", {});
  return undefinedValue;
}

const std::string& Fact::getPunctualPrefix()
{
  static const std::string punctualPrefix = "~punctual~";
  return punctualPrefix;
}


void Fact::_resetFactSignatureCache()
{
  _factSignature = _generateFactSignature();
}


void Fact::_finalizeInisilizationAndValidityChecks(bool pIsOkIfValueIsMissing)
{
  static const bool inEffectContext = false;
  if (predicate.parameters.size() != _arguments.size())
    throw std::runtime_error("The fact \"" + toPddl(inEffectContext) + "\" does not have the same number of parameters than the associated predicate \"" + predicate.toPddl() + "\"");
  for (auto i = 0; i < _arguments.size(); ++i)
  {
    if (!predicate.parameters[i].type)
      throw std::runtime_error("\"" + predicate.parameters[i].name + "\" does not have a type, in fact predicate \"" + predicate.toPddl() + "\"");
    auto currArgType = _arguments[i].type();
    if (!currArgType && !_arguments[i].isAnyEntity())
      throw std::runtime_error("\"" + _arguments[i].toStr() + "\" does not have a type");
    if (_arguments[i].isAParameterToFill())
    {
      predicate.parameters[i].type = Type::getSmallerType(currArgType, predicate.parameters[i].type);
      _arguments[i].entity().type = predicate.parameters[i].type;
      continue;
    }
    if (_arguments[i].isFluent() && !_arguments[i].fluent()->predicate.value)
      throw std::runtime_error("The fluent \"" + _arguments[i].toPddl() + "\" used as parameter of \"" + predicate.toPddl() + "\" does not return a value");
    if (!currArgType || !currArgType->isA(*predicate.parameters[i].type))
      throw std::runtime_error("\"" + _arguments[i].toStr() + "\" is not a \"" + predicate.parameters[i].type->name + "\" for predicate: \"" + predicate.toPddl() + "\"");
  }

  if (predicate.value)
  {
    if (_value)
    {
      if (!_value->type && !_value->isAnyEntity())
        throw std::runtime_error("\"" + _value->toStr() + "\" does not have type");

      if (_value->isAParameterToFill())
      {
        predicate.value = Type::getSmallerType(_value->type, predicate.value);
        _value->type = predicate.value;
      }
      else
      {
        if (!_value->type->isA(*predicate.value))
          throw std::runtime_error("\"" + _value->toStr() + "\" is not a \"" + predicate.value->name + "\" for predicate: \"" + predicate.toPddl() + "\"");
      }
    }
    else if (!pIsOkIfValueIsMissing)
    {
      throw std::runtime_error("The value of this fluent \"" + toPddl(inEffectContext) + "\" is missing. The associated function is \"" + predicate.toPddl() + "\"");
    }

  }
  else if (_value)
  {
    throw std::runtime_error("This fact \"" + toPddl(inEffectContext) + "\" should not have a value. The associated predicate is \"" + predicate.toPddl() + "\"");
  }
}



} // !ogp
