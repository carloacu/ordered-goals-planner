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

void _entitiesToStr(std::string& pStr,
                    const std::vector<Entity>& pParameters)
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
                         const std::vector<Entity>& pParameters,
                         const std::string& pSeparator)
{
  bool firstIteration = true;
  for (auto& param : pParameters)
  {
    if (firstIteration)
      firstIteration = false;
    else
      pStr += pSeparator;
    pStr += param.value;
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

bool _isInside(const Entity& pEntity,
               const std::map<Parameter, std::set<Entity>>* pEltsPtr)
{
  if (pEltsPtr == nullptr)
    return false;
  auto it = pEltsPtr->find(pEntity.toParameter());
  return it != pEltsPtr->end() && it->second.empty();
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
    if (!pStrPddlFormated)
    {
      if (!expressionParsed.name.empty() && expressionParsed.name[0] == '!')
      {
        if (pIsFactNegatedPtr != nullptr)
           *pIsFactNegatedPtr = true;
        _name = expressionParsed.name.substr(1, expressionParsed.name.size() - 1);
      }
      else
      {
        _name = expressionParsed.name;
      }
    }
    else
    {
      if (expressionParsed.name == "not" && expressionParsed.arguments.size() == 1)
      {
        if (pIsFactNegatedPtr != nullptr)
           *pIsFactNegatedPtr = true;
        expressionParsed = expressionParsed.arguments.back().clone();
      }
      _name = expressionParsed.name;
    }

    _isValueNegated = expressionParsed.isValueNegated;
    auto* expressionParsedForArgumentsPtr = &expressionParsed;
    if (_name == "=" && expressionParsed.arguments.size() == 2)
    {
      auto valueStr = expressionParsed.arguments.back().name;
      if (valueStr == getUndefinedEntity().value)
      {
        if (pIsFactNegatedPtr != nullptr)
           *pIsFactNegatedPtr = true;
        _value.emplace(Entity::createAnyEntity());
      }
      else
      {
        _value.emplace(Entity::fromUsage(valueStr, pOntology, pObjects, pParameters));
      }
      expressionParsedForArgumentsPtr = &expressionParsed.arguments.front();
      _name = expressionParsedForArgumentsPtr->name;
    }
    else if (expressionParsed.value != "")
    {
      _value.emplace(Entity::fromUsage(expressionParsed.value, pOntology, pObjects, pParameters));
    }

    for (auto& currArgument : expressionParsedForArgumentsPtr->arguments)
      _arguments.push_back(Entity::fromUsage(currArgument.name, pOntology, pObjects, pParameters));


    predicate = pOntology.predicates.nameToPredicate(_name);
    _finalizeInisilizationAndValidityChecks(pIsOkIfValueIsMissing);
    _resetFactSignatureCache();
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
           const std::vector<std::string>& pArgumentStrs,
           const std::string& pValueStr,
           bool pIsValueNegated,
           const Ontology& pOntology,
           const SetOfEntities& pObjects,
           const std::vector<Parameter>& pParameters,
           bool pIsOkIfValueIsMissing,
           const std::map<std::string, Entity>* pParameterNamesToEntityPtr)
  : predicate("_not_set", true, pOntology.types),
    _name(pName),
    _arguments(),
    _value(),
    _isValueNegated(pIsValueNegated),
    _factSignature()
{
  auto* predicatePtr = pOntology.predicates.nameToPredicatePtr(_name);
  if (predicatePtr == nullptr)
    predicatePtr = pOntology.derivedPredicates.nameToPredicatePtr(_name);
  if (predicatePtr == nullptr)
    throw std::runtime_error("\"" + pName + "\" is not a predicate name or a derived predicate name");

  predicate = *predicatePtr;
  for (auto& currParam : pArgumentStrs)
    if (!currParam.empty())
      _arguments.push_back(Entity::fromUsage(currParam, pOntology, pObjects, pParameters, pParameterNamesToEntityPtr));
  if (!pValueStr.empty())
    _value = Entity::fromUsage(pValueStr, pOntology, pObjects, pParameters, pParameterNamesToEntityPtr);
  else if (pIsOkIfValueIsMissing && predicate.value)
    _value = Entity(Entity::anyEntityValue(), predicate.value);
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
                                              const std::map<Parameter, std::set<Entity>>* pOtherFactParametersToConsiderAsAnyValuePtr,
                                              const std::map<Parameter, std::set<Entity>>* pOtherFactParametersToConsiderAsAnyValuePtr2) const
{
  if (pFact._name != _name ||
      pFact._arguments.size() != _arguments.size())
    return false;

  auto itParam = _arguments.begin();
  auto itOtherParam = pFact._arguments.begin();
  while (itParam != _arguments.end())
  {
    if (*itParam != *itOtherParam && !itParam->isAnyEntity() && !itOtherParam->isAnyEntity() &&
        !(_isInside(*itOtherParam, pOtherFactParametersToConsiderAsAnyValuePtr) ||
          _isInside(*itOtherParam, pOtherFactParametersToConsiderAsAnyValuePtr2)))
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
        itParam->value != pArgToIgnore)
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
          if (currParam.name == itParam->value)
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
    if (*itParam != *itOtherParam && !itParam->isAParameterToFill() && !itOtherParam->isAParameterToFill())
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
                                     const std::map<Parameter, std::set<Entity>>* pOtherFactParametersToConsiderAsAnyValuePtr,
                                     const std::map<Parameter, std::set<Entity>>* pOtherFactParametersToConsiderAsAnyValuePtr2,
                                     const std::vector<Parameter>* pThisFactParametersToConsiderAsAnyValuePtr) const
{
  if (_name != pOther._name || _arguments.size() != pOther._arguments.size())
    return false;

  auto itParam = _arguments.begin();
  auto itOtherParam = pOther._arguments.begin();
  while (itParam != _arguments.end())
  {
    if (*itParam != *itOtherParam && !itParam->isAnyEntity() && !itOtherParam->isAnyEntity() &&
        !(_isInside(*itParam, pThisFactParametersToConsiderAsAnyValuePtr)) &&
        !(_isInside(*itOtherParam, pOtherFactParametersToConsiderAsAnyValuePtr) ||
          _isInside(*itOtherParam, pOtherFactParametersToConsiderAsAnyValuePtr2)))
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
                                             const std::map<Parameter, std::set<Entity>>* pOtherFactParametersToConsiderAsAnyValuePtr,
                                             const std::map<Parameter, std::set<Entity>>* pOtherFactParametersToConsiderAsAnyValuePtr2) const
{
  if (_name != pOther._name || _arguments.size() != pOther._arguments.size())
    return false;

  auto itParam = _arguments.begin();
  auto itOtherParam = pOther._arguments.begin();
  while (itParam != _arguments.end())
  {
    if (*itParam != *itOtherParam && !itParam->isAnyEntity() && !itOtherParam->isAnyEntity() &&
        !(_isInside(*itOtherParam, pOtherFactParametersToConsiderAsAnyValuePtr) ||
          _isInside(*itOtherParam, pOtherFactParametersToConsiderAsAnyValuePtr2)))
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
    if (itParam->match(pParameter))
      return true;
    ++itParam;
  }
  return false;
}


bool Fact::hasAParameter(bool pIgnoreValue) const
{
  for (const auto& currArg : _arguments)
    if (currArg.isAParameterToFill())
      return true;
  return !pIgnoreValue && _value && _value->isAParameterToFill();
}


std::optional<Entity> Fact::tryToExtractArgumentFromExample(const Parameter& pParameter,
                                                            const Fact& pExampleFact) const
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
    if (itParam->match(pParameter))
      res = *itOtherParam;
    ++itParam;
    ++itOtherParam;
  }
  return res;
}

std::optional<Entity> Fact::tryToExtractArgumentFromExampleWithoutValueConsideration(
    const Parameter& pParameter,
    const Fact& pExampleFact) const
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
    if (itArg->match(pParameter))
      res = *itOtherArg;
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
  {
    auto itValueParam = pCurrentArgumentsToNewArgument.find(currParam.toParameter());
    if (itValueParam != pCurrentArgumentsToNewArgument.end())
      currParam = itValueParam->second;
  }
  _resetFactSignatureCache();
}

void Fact::replaceArguments(const std::map<Parameter, std::set<Entity>>& pCurrentArgumentsToNewArgument)
{
  if (_value)
  {
    auto itValueParam = pCurrentArgumentsToNewArgument.find(_value->toParameter());
    if (itValueParam != pCurrentArgumentsToNewArgument.end() && !itValueParam->second.empty())
      _value = *itValueParam->second.begin();
  }

  for (auto& currParam : _arguments)
  {
    auto itValueParam = pCurrentArgumentsToNewArgument.find(currParam.toParameter());
    if (itValueParam != pCurrentArgumentsToNewArgument.end() && !itValueParam->second.empty())
      currParam = *itValueParam->second.begin();
  }
  _resetFactSignatureCache();
}

std::string Fact::toPddl(bool pInEffectContext,
                         bool pPrintAnyValue) const
{
  std::string res = "(" + _name;
  if (!_arguments.empty())
  {
    res += " ";
    _entitiesToValueStr(res, _arguments, " ");
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
    _entitiesToValueStr(res, _arguments, ", ");
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
                          std::map<Parameter, std::set<Entity>>* pNewParametersPtr,
                          bool pCheckAllPossibilities,
                          const std::map<Parameter, std::set<Entity>>* pParametersPtr,
                          std::map<Parameter, std::set<Entity>>* pParametersToModifyInPlacePtr,
                          bool* pTriedToModifyParametersPtr) const
{
  bool res = false;
  std::map<Parameter, std::set<Entity>> newPotentialParameters;
  std::map<Parameter, std::set<Entity>> newPotentialParametersInPlace;
  for (const auto& currOtherFact : pOtherFacts)
    if (isInOtherFact(currOtherFact, newPotentialParameters,
                      pParametersPtr, newPotentialParametersInPlace, pParametersToModifyInPlacePtr))
      res = true;

  if (!res)
    return false;
  return updateParameters(newPotentialParameters, newPotentialParametersInPlace, pNewParametersPtr, pCheckAllPossibilities,
                          pParametersPtr, pParametersToModifyInPlacePtr, pTriedToModifyParametersPtr);
}


bool Fact::isInOtherFactsMap(const SetOfFacts& pOtherFacts,
                             std::map<Parameter, std::set<Entity>>* pNewParametersPtr,
                             bool pCheckAllPossibilities,
                             const std::map<Parameter, std::set<Entity>>* pParametersPtr,
                             std::map<Parameter, std::set<Entity>>* pParametersToModifyInPlacePtr,
                             bool* pTriedToModifyParametersPtr) const
{
  bool res = false;
  auto otherFactsThatMatched = pOtherFacts.find(*this);
  std::map<Parameter, std::set<Entity>> newPotentialParameters;
  std::map<Parameter, std::set<Entity>> newPotentialParametersInPlace;
  if (pParametersToModifyInPlacePtr != nullptr)
    newPotentialParametersInPlace = *pParametersToModifyInPlacePtr;
  for (const auto& currOtherFact : otherFactsThatMatched)
    if (isInOtherFact(currOtherFact, newPotentialParameters, pParametersPtr,
                      newPotentialParametersInPlace, pParametersToModifyInPlacePtr))
      res = true;

  if (!res)
    return false;
  return updateParameters(newPotentialParameters, newPotentialParametersInPlace, pNewParametersPtr, pCheckAllPossibilities,
                          pParametersPtr, pParametersToModifyInPlacePtr, pTriedToModifyParametersPtr);
}


bool Fact::updateParameters(std::map<Parameter, std::set<Entity>>& pNewPotentialParameters,
                            std::map<Parameter, std::set<Entity>>& pNewPotentialParametersInPlace,
                            std::map<Parameter, std::set<Entity>>* pNewParametersPtr,
                            bool pCheckAllPossibilities,
                            const std::map<Parameter, std::set<Entity>>* pParametersPtr,
                            std::map<Parameter, std::set<Entity>>* pParametersToModifyInPlacePtr,
                            bool* pTriedToModifyParametersPtr) const
{
  if (pParametersToModifyInPlacePtr != nullptr)
    *pParametersToModifyInPlacePtr = std::move(pNewPotentialParametersInPlace);
  return _updateParameters(pNewParametersPtr, pNewPotentialParameters, pCheckAllPossibilities, pParametersPtr, pTriedToModifyParametersPtr);
}


bool Fact::_updateParameters(std::map<Parameter, std::set<Entity>>* pNewParametersPtr,
                             std::map<Parameter, std::set<Entity>>& pNewPotentialParameters,
                             bool pCheckAllPossibilities,
                             const std::map<Parameter, std::set<Entity>>* pParametersPtr,
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


bool Fact::isInOtherFact(const Fact& pOtherFact,
                         std::map<Parameter, std::set<Entity>>& pNewParameters,
                         const std::map<Parameter, std::set<Entity>>* pParametersPtr,
                         std::map<Parameter, std::set<Entity>>& pNewParametersInPlace,
                         const std::map<Parameter, std::set<Entity>>* pParametersToModifyInPlacePtr) const
{
  if (pOtherFact._name != _name ||
      pOtherFact._arguments.size() != _arguments.size())
    return false;

  std::map<Parameter, std::set<Entity>> newPotentialParameters;
  std::map<Parameter, std::set<Entity>> newParametersInPlace;
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

        newPotentialParameters[pFactValue.toParameter()].insert(pValueToLookFor);
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
        newParametersInPlace[pFactValue.toParameter()].insert(pValueToLookFor);
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
          !doesItMatch(*itLookForArguments, *itFactArguments))
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
    if (currParameter == pCurrent)
      currParameter = pNew;
}


std::map<Parameter, Entity> Fact::extratParameterToArguments() const
{
  if (_arguments.size() == predicate.parameters.size())
  {
    std::map<Parameter, Entity> res;
    for (auto i = 0; i < _arguments.size(); ++i)
      res.emplace(predicate.parameters[i], _arguments[i]);

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
    if (currArg.type)
    {
      if (firstArg)
        firstArg = false;
      else
        res += ", ";
      res += currArg.type->name;
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
    relatedTypes.insert(currArg.type);
    if (pIncludeSubTypes)
      currArg.type->getSubTypesRecursively(relatedTypes);
    if (pIncludeParentTypes)
       currArg.type->getParentTypesRecursively(relatedTypes);
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
  _arguments[pIndex].type = pType;
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
    if (!_arguments[i].type && !_arguments[i].isAnyEntity())
      throw std::runtime_error("\"" + _arguments[i].value + "\" does not have a type");
    if (_arguments[i].isAParameterToFill())
    {
      predicate.parameters[i].type = Type::getSmallerType(_arguments[i].type, predicate.parameters[i].type);
      _arguments[i].type = predicate.parameters[i].type;
      continue;
    }
    if (!_arguments[i].type->isA(*predicate.parameters[i].type))
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
