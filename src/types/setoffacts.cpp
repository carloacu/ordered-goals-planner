#include <orderedgoalsplanner/types/setoffacts.hpp>
#include <stdexcept>
#include <orderedgoalsplanner/types/fact.hpp>
#include <orderedgoalsplanner/util/alias.hpp>
#include <orderedgoalsplanner/util/util.hpp>
#include "expressionParsed.hpp"

namespace ogp
{
namespace
{
std::string _getExactCall(const Fact& pFact)
{
  auto res = pFact.name();
  res += "(";
  bool firstArg = true;
  for (const auto& currArg : pFact.arguments())
  {
    if (currArg.type)
    {
      if (firstArg)
        firstArg = false;
      else
        res += ", ";
      res += currArg.value;
    }
  }
  res += ")";
  return res;
}


void _addValueToExactCall(std::string& pRes,
                           const Fact& pFact)
{
  if (pFact.value())
  {
    pRes += "=";
    if (pFact.isValueNegated())
      pRes = "!";
    pRes += pFact.value()->value;
  }
}
}



SetOfFacts::SetOfFacts()
 : _facts(),
   _exactCallToListsOpt(),
   _exactCallWithoutValueToListsOpt(),
   _signatureToLists()
{
}


std::string SetOfFacts::Delta::toPddl() const
{
  std::string res;
  for (const auto& currFact : addedFacts)
  {
    if (res != "")
      res += "\n";
    res += "+" + currFact.toPddl(false);
  }
  for (const auto& currFact : removedFacts)
  {
    if (res != "")
      res += "\n";
    res += "-" + currFact.toPddl(false);
  }
  return res;
}


SetOfFacts::Delta SetOfFacts::deltaFrom(const SetOfFacts& pOldSetOfFacts) const
{
  Delta res;
  for (const auto& currFact : _facts)
  {
    auto it = pOldSetOfFacts._facts.find(currFact.first);
    if (it == pOldSetOfFacts._facts.end())
      res.addedFacts.insert(currFact.first);
  }

  for (const auto& currFact : pOldSetOfFacts._facts)
  {
    auto it = _facts.find(currFact.first);
    if (it == _facts.end())
    {
      bool canBeRemoved = true;
      for (const auto& addedFact : res.addedFacts)
      {
        if (addedFact.areEqualWithoutValueConsideration(currFact.first))
        {
          canBeRemoved = false;
          break;
        }
      }
      if (canBeRemoved)
        res.removedFacts.insert(currFact.first);
    }
  }
  return res;
}


SetOfFacts SetOfFacts::fromPddl(const std::string& pStr,
                                std::size_t& pPos,
                                const Ontology& pOntology,
                                const SetOfEntities& pObjects,
                                bool pCanFactsBeRemoved)
{
  SetOfFacts res;
  auto strSize = pStr.size();
  ExpressionParsed::skipSpaces(pStr, pPos);

  while (pPos < strSize && pStr[pPos] != ')')
  {
    bool isFactNegated = false;
    Fact fact(pStr, true, pOntology, pObjects, {}, &isFactNegated, pPos, &pPos);
    if (isFactNegated)
      res.erase(fact);
    else
      res.add(fact, pCanFactsBeRemoved);
  }
  return res;
}


std::string SetOfFacts::toPddl(std::size_t pIdentation, bool pPrintTimeLessFactsToo) const
{
  std::string res;
  bool firstIteration = true;
  for (auto& currFact : _facts)
  {
    if (!pPrintTimeLessFactsToo && !currFact.second)
      continue;
    if (firstIteration)
      firstIteration = false;
    else
      res += "\n";
    res += std::string(pIdentation, ' ') + currFact.first.toPddl(false, true);
  }
  return res;
}



bool SetOfFacts::add(const Fact& pFact,
                     bool pCanBeRemoved)
{
  if (!_facts.try_emplace(pFact, pCanBeRemoved).second)
    return false;

  if (!pFact.hasAParameter())
  {
    auto exactCallStr = _getExactCall(pFact);
    if (!_exactCallWithoutValueToListsOpt)
      _exactCallWithoutValueToListsOpt.emplace();
    (*_exactCallWithoutValueToListsOpt)[exactCallStr].emplace_back(pFact);

    if (pFact.value())
    {
      _addValueToExactCall(exactCallStr, pFact);
      if (!_exactCallToListsOpt)
        _exactCallToListsOpt.emplace();
      (*_exactCallToListsOpt)[exactCallStr].emplace_back(pFact);
    }
  }


  pFact.generateSignaturesWithRelatedTypes([&](const std::string& pSignature) {
    auto& factArguments = pFact.arguments();
    auto insertionRes = _signatureToLists.emplace(pSignature, factArguments.size());
    ParameterToValues& parameterToValues = insertionRes.first->second;

    parameterToValues.all.emplace_back(pFact);
    for (std::size_t i = 0; i < factArguments.size(); ++i)
    {
      if (!factArguments[i].isAParameterToFill())
        parameterToValues.argIdToArgValueToValues[i][factArguments[i].value].emplace_back(pFact);
      else
        parameterToValues.argIdToArgValueToValues[i][""].emplace_back(pFact);
    }
    if (pFact.value())
    {
      if (!pFact.value()->isAParameterToFill() && !pFact.isValueNegated())
        parameterToValues.fluentValueToValues[pFact.value()->value].emplace_back(pFact);
      else
        parameterToValues.fluentValueToValues[""].emplace_back(pFact);
    }
  }, false, true);
  return true;
}


bool SetOfFacts::erase(const Fact& pFact)
{
  if (_erase(pFact))
    return true;

  auto factIt = find(pFact);
  for (const auto& currFact : factIt) {
    auto copiedFact = currFact; // Copy to prevent usage after memory liberation
    return _erase(copiedFact);
  }
  return false;
}


std::map<Fact, bool>::iterator SetOfFacts::eraseFactIt(std::map<Fact, bool>::iterator pFactIt)
{
  // Do not allow to remove if this fact is marked as cannot be removed
  if (!pFactIt->second)
    throw std::runtime_error("Cannot erase this fact iterator because it is constant");
  const Fact& fact = pFactIt->first;

  if (!fact.hasAParameter())
  {
    auto exactCallStr = _getExactCall(fact);
    if (_exactCallWithoutValueToListsOpt)
    {
      std::list<Fact>& listOfTypes = (*_exactCallWithoutValueToListsOpt)[exactCallStr];
      _removeAValueForList(listOfTypes, fact);
      if (listOfTypes.empty())
        _exactCallWithoutValueToListsOpt->erase(exactCallStr);
    }

    if (_exactCallToListsOpt && fact.value())
    {
      _addValueToExactCall(exactCallStr, fact);
      std::list<Fact>& listWithValueOfTypes = (*_exactCallToListsOpt)[exactCallStr];
      _removeAValueForList(listWithValueOfTypes, fact);
      if (listWithValueOfTypes.empty())
        _exactCallToListsOpt->erase(exactCallStr);
    }
  }

  fact.generateSignaturesWithRelatedTypes([&](const std::string& pSignature) {
    auto& factArguments = fact.arguments();
    auto itParameterToValues = _signatureToLists.find(pSignature);
    if (itParameterToValues != _signatureToLists.end())
    {
      ParameterToValues& parameterToValues = itParameterToValues->second;

      _removeAValueForList(parameterToValues.all, fact);
      if (parameterToValues.all.empty())
      {
        _signatureToLists.erase(pSignature);
      }
      else
      {
        for (std::size_t i = 0; i < factArguments.size(); ++i)
        {
          const std::string argKey = !factArguments[i].isAParameterToFill() ? factArguments[i].value : "";
          std::list<Fact>& listOfValues = parameterToValues.argIdToArgValueToValues[i][argKey];
          _removeAValueForList(listOfValues, fact);
          if (listOfValues.empty())
             parameterToValues.argIdToArgValueToValues[i].erase(argKey);
        }
        if (fact.value())
        {
          const std::string valueKey = !fact.value()->isAParameterToFill() ? fact.value()->value : "";
          std::list<Fact>& listOfValues = parameterToValues.fluentValueToValues[valueKey];
          _removeAValueForList(listOfValues, fact);
          if (listOfValues.empty())
             parameterToValues.fluentValueToValues.erase(valueKey);
        }
      }
    }
    else
    {
      throw std::runtime_error("Errur while deleteing a fact link");
    }
  }, false, true);

  return _facts.erase(pFactIt);
}


bool SetOfFacts::_erase(const Fact& pFact)
{
  auto it = _facts.find(pFact);
  if (it != _facts.end())
  {
    if (!it->second)
      return false;
    eraseFactIt(it);
    return true;
  }
  return false;
}


std::string SetOfFacts::SetOfFactIterator::toStr() const
{
  std::stringstream ss;
  ss << "[";
  bool firstElt = true;
  for (const Fact& currElt : *this)
  {
    if (firstElt)
      firstElt = false;
    else
      ss << ", ";
    ss << currElt;
  }
  ss << "]";
  return ss.str();
}


void SetOfFacts::clear()
{
  _facts.clear();
  if (_exactCallToListsOpt)
    _exactCallToListsOpt.reset();
  if (_exactCallWithoutValueToListsOpt)
    _exactCallWithoutValueToListsOpt.reset();
  _signatureToLists.clear();
}


typename SetOfFacts::SetOfFactIterator SetOfFacts::find(const Fact& pFact,
                                                        bool pIgnoreValue) const
{
  if (!pFact.hasAParameter(pIgnoreValue) && !pFact.isValueNegated())
  {
    auto exactCallStr = _getExactCall(pFact);
    if (!pIgnoreValue && pFact.value())
    {
      _addValueToExactCall(exactCallStr, pFact);
      return SetOfFactIterator(_findAnExactCall(_exactCallToListsOpt, exactCallStr));
    }
    return SetOfFactIterator(_findAnExactCall(_exactCallWithoutValueToListsOpt, exactCallStr));
  }

  const std::list<Fact>* resPtr = nullptr;
  auto _matchArg = [&](const std::map<std::string, std::list<Fact>>& pArgValueToValues,
                       const std::string& pArgValue) -> std::optional<typename SetOfFacts::SetOfFactIterator> {
    auto itForThisValue = pArgValueToValues.find(pArgValue);
    if (itForThisValue != pArgValueToValues.end())
    {
      if (resPtr != nullptr)
        return SetOfFactIterator(intersectTwoLists(*resPtr, itForThisValue->second));
      resPtr = &itForThisValue->second;
    }
    return {};
  };

  auto itParameterToValues = _signatureToLists.find(pFact.factSignature());
  if (itParameterToValues != _signatureToLists.end())
  {
    const ParameterToValues& parameterToValues = itParameterToValues->second;

    bool hasOnlyParameters = true;
    auto& factArguments = pFact.arguments();
    for (std::size_t i = 0; i < factArguments.size(); ++i)
    {
      if (!factArguments[i].isAParameterToFill())
      {
        hasOnlyParameters = false;
        auto subRes = _matchArg(parameterToValues.argIdToArgValueToValues[i], factArguments[i].value);
        if (subRes)
          return *subRes;
      }
    }

    const auto& fluentValue = pFact.value();
    if (!pIgnoreValue && fluentValue)
    {
      if (!fluentValue->isAParameterToFill() && !pFact.isValueNegated())
      {
        hasOnlyParameters = false;
        auto subRes = _matchArg(parameterToValues.fluentValueToValues, fluentValue->value);
        if (subRes)
          return *subRes;
      }
    }

    if (hasOnlyParameters)
      return SetOfFactIterator(&parameterToValues.all);
  }

  if (resPtr != nullptr)
    return SetOfFactIterator(resPtr);
  return SetOfFactIterator(nullptr);
}

std::optional<Entity> SetOfFacts::getFluentValue(const ogp::Fact& pFact) const
{
  auto factMatchingInWs = find(pFact, true);
  for (const auto& currFact : factMatchingInWs)
    if (currFact.arguments() == pFact.arguments())
      return currFact.value();
  return {};
}


void SetOfFacts::extractPotentialArgumentsOfAFactParameter(
    std::set<Entity>& pPotentialArgumentsOfTheParameter,
    const Fact& pFact,
    const std::string& pParameter,
    const ParameterValuesWithConstraints& pParameters,
    ParameterValuesWithConstraints* pPotentialNewParametersPtr) const
{
  auto factMatchingInWs = find(pFact);
  for (const auto& currFact : factMatchingInWs)
  {
    if (currFact.arguments().size() == pFact.arguments().size())
    {
      std::set<Entity> potentialNewValues;

      auto onEntity = [&](const Entity& pFactEntity, const Entity& pCurrFactEntity)
      {
        if (pFactEntity.value == pParameter)
        {
          potentialNewValues.insert(pCurrFactEntity);
          return true;
        }
        if (pFactEntity == pCurrFactEntity)
          return true;

        bool res = false;
        for (const auto& currParam : pParameters)
        {
          if (pFactEntity.value == currParam.first.name)
          {
            if (pPotentialNewParametersPtr != nullptr)
              (*pPotentialNewParametersPtr)[currParam.first][pCurrFactEntity];
            res = true;
            break;
          }
        }
        return res;
      };

      bool doesItMatch = true;
      for (auto i = 0; i < pFact.arguments().size(); ++i)
      {
        if (!onEntity(pFact.arguments()[i], currFact.arguments()[i]))
        {
          doesItMatch = false;
          break;
        }
      }

      if (pFact.value() && currFact.value() && !onEntity(*pFact.value(), *currFact.value()))
        doesItMatch = false;

      if (doesItMatch)
      {
        if (pPotentialArgumentsOfTheParameter.empty())
          pPotentialArgumentsOfTheParameter = std::move(potentialNewValues);
        else
          pPotentialArgumentsOfTheParameter.insert(potentialNewValues.begin(), potentialNewValues.end());
      }
    }
  }
}


bool SetOfFacts::hasFact(const Fact& pFact) const
{
  auto factMatchingInWs = find(pFact);
  for (const auto& currFact : factMatchingInWs)
  {
    if (currFact.arguments().size() == pFact.arguments().size())
    {
      bool doesItMatch = true;
      for (auto i = 0; i < pFact.arguments().size(); ++i)
      {
        if (pFact.arguments()[i] == currFact.arguments()[i])
          continue;
        doesItMatch = false;
        break;
      }
      if (doesItMatch)
        return true;
    }
  }
  return false;
}


void SetOfFacts::_removeAValueForList(std::list<Fact>& pList,
                                     const Fact& pValue) const
{
  for (auto it = pList.begin(); it != pList.end(); ++it)
  {
    if (*it == pValue)
    {
      pList.erase(it);
      return;
    }
  }
}


const std::list<Fact>* SetOfFacts::_findAnExactCall(
    const std::optional<std::map<std::string, std::list<Fact>>>& pExactCalls,
    const std::string& pExactCall) const
{
  if (pExactCalls)
  {
    auto it = pExactCalls->find(pExactCall);
    if (it != pExactCalls->end())
      return &it->second;
  }
  return nullptr;
}



} // !ogp

