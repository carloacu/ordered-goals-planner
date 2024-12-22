#include <orderedgoalsplanner/types/factstovalue.hpp>
#include <stdexcept>
#include <orderedgoalsplanner/types/fact.hpp>
#include <orderedgoalsplanner/util/alias.hpp>
#include <orderedgoalsplanner/util/util.hpp>

namespace ogp
{
namespace
{
const std::string _emptyString = "";

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



FactsToValue::FactsToValue()
 : _values(),
   _valueToFacts(),
   _exactCallToListsOpt(),
   _exactCallWithoutValueToListsOpt(),
   _signatureToLists(),
   _valuesWithoutFact()
{
}


void FactsToValue::add(const Fact& pFact,
                       const std::string& pValue,
                       bool pIgnoreValue)
{
  auto insertionResult = _values.insert(pValue);
  _valueToFacts[pValue].emplace_back(pFact);

  if (!pFact.hasAParameter())
  {
    auto exactCallStr = _getExactCall(pFact);
    if (!_exactCallWithoutValueToListsOpt)
      _exactCallWithoutValueToListsOpt.emplace();
    (*_exactCallWithoutValueToListsOpt)[exactCallStr].emplace_back(pValue);

    if (!pIgnoreValue && pFact.value())
    {
      _addValueToExactCall(exactCallStr, pFact);
      if (!_exactCallToListsOpt)
        _exactCallToListsOpt.emplace();
      (*_exactCallToListsOpt)[exactCallStr].emplace_back(pValue);
    }
  }

  std::list<std::string> factSignatures;
  pFact.generateSignatureForSubAndUpperTypes(factSignatures);
  for (auto& currSignature : factSignatures)
  {
    auto& factArguments = pFact.arguments();
    auto insertionRes = _signatureToLists.emplace(currSignature, factArguments.size());
    ParameterToValues& parameterToValues = insertionRes.first->second;

    parameterToValues.all.emplace_back(pValue);
    for (std::size_t i = 0; i < factArguments.size(); ++i)
    {
      if (!factArguments[i].isAParameterToFill())
        parameterToValues.argIdToArgValueToValues[i][factArguments[i].value].emplace_back(pValue);
      else
        parameterToValues.argIdToArgValueToValues[i][""].emplace_back(pValue);
    }
    if (pIgnoreValue || pFact.value())
    {
      if (!pIgnoreValue && !pFact.value()->isAParameterToFill() && !pFact.isValueNegated())
        parameterToValues.fluentValueToValues[pFact.value()->value].emplace_back(pValue);
      else
        parameterToValues.fluentValueToValues[""].emplace_back(pValue);
    }
  }
}


void FactsToValue::addValueWithoutFact(const std::string& pValue)
{
  auto insertionResult = _values.insert(pValue);
  // pValue well added and did not already exists
  if (insertionResult.second)
  {
    _valuesWithoutFact.emplace_back(pValue);
  }
}


void FactsToValue::_erase(const Fact& pFact,
                          const std::string& pValue)
{
  auto it = _values.find(pValue);
  if (it != _values.end())
  {
    if (!pFact.hasAParameter())
    {
      auto exactCallStr = _getExactCall(pFact);
      if (_exactCallWithoutValueToListsOpt)
      {
        std::list<std::string>& listOfTypes = (*_exactCallWithoutValueToListsOpt)[exactCallStr];
        _removeAValueForList(listOfTypes, pValue);
        if (listOfTypes.empty())
          _exactCallWithoutValueToListsOpt->erase(exactCallStr);
      }

      if (_exactCallToListsOpt && pFact.value())
      {
        _addValueToExactCall(exactCallStr, pFact);
        std::list<std::string>& listWithValueOfTypes = (*_exactCallToListsOpt)[exactCallStr];
        _removeAValueForList(listWithValueOfTypes, pValue);
        if (listWithValueOfTypes.empty())
          _exactCallToListsOpt->erase(exactCallStr);
      }
    }

    std::list<std::string> factSignatures;
    pFact.generateSignatureForSubAndUpperTypes(factSignatures);
    for (auto& currSignature : factSignatures)
    {
      auto& factArguments = pFact.arguments();
      auto itParameterToValues = _signatureToLists.find(currSignature);
      if (itParameterToValues != _signatureToLists.end())
      {
        ParameterToValues& parameterToValues = itParameterToValues->second;

        _removeAValueForList(parameterToValues.all, pValue);
        if (parameterToValues.all.empty())
        {
          _signatureToLists.erase(currSignature);
        }
        else
        {
          for (std::size_t i = 0; i < factArguments.size(); ++i)
          {
            const std::string& argKey = !factArguments[i].isAParameterToFill() ? factArguments[i].value : _emptyString;
            std::list<std::string>& listOfValues = parameterToValues.argIdToArgValueToValues[i][argKey];
            _removeAValueForList(listOfValues, pValue);
            if (listOfValues.empty())
               parameterToValues.argIdToArgValueToValues[i].erase(argKey);
          }
          if (pFact.value())
          {
            const std::string& fluentKey = !pFact.value()->isAParameterToFill() ? pFact.value()->value : _emptyString;
            std::list<std::string>& listOfValues = parameterToValues.fluentValueToValues[fluentKey];
            _removeAValueForList(listOfValues, pValue);
            if (listOfValues.empty())
               parameterToValues.fluentValueToValues.erase(fluentKey);
          }
        }
      }
      else
      {
        throw std::runtime_error("Errur while deleteing a fact link");
      }
    }
  }
}


void FactsToValue::erase(const std::string& pValue)
{
  auto it = _valueToFacts.find(pValue);
  if (it != _valueToFacts.end())
  {
    for (auto& currFact : it->second)
    {
      _erase(currFact, pValue);
    }
    _valueToFacts.erase(it);

    auto itVal = _values.find(pValue);
    if (itVal != _values.end())
    {
      _removeAValueForList(_valuesWithoutFact, pValue);
      _values.erase(itVal);
    }
  }
}


void FactsToValue::clear()
{
  _values.clear();
  _valueToFacts.clear();
  if (_exactCallToListsOpt)
    _exactCallToListsOpt.reset();
  if (_exactCallWithoutValueToListsOpt)
    _exactCallWithoutValueToListsOpt.reset();
  _signatureToLists.clear();
}


bool FactsToValue::empty() const
{
  return _values.empty();
}


typename FactsToValue::ConstMapOfFactIterator FactsToValue::find(const Fact& pFact,
                                                                 bool pIgnoreValue) const
{
  const std::list<std::string>* exactMatchPtr = nullptr;

  if (!pFact.hasAParameter(pIgnoreValue) && !pFact.isValueNegated())
  {
    auto exactCallStr = _getExactCall(pFact);
    if (!pIgnoreValue && pFact.value())
    {
      _addValueToExactCall(exactCallStr, pFact);
      exactMatchPtr = _findAnExactCall(_exactCallToListsOpt, exactCallStr);
    }
    else
    {
      exactMatchPtr = _findAnExactCall(_exactCallWithoutValueToListsOpt, exactCallStr);
    }
  }

  const std::list<std::string>* resPtr = nullptr;
  auto _matchArg = [&](const std::map<std::string, std::list<std::string>>& pArgValueToValues,
                       const std::string& pArgValue) -> std::optional<typename FactsToValue::ConstMapOfFactIterator> {
    auto itForThisValue = pArgValueToValues.find(pArgValue);
    if (itForThisValue != pArgValueToValues.end())
    {
      auto itForAnyValue = pArgValueToValues.find("");
      if (itForAnyValue != pArgValueToValues.end())
      {
        if (exactMatchPtr != nullptr)
          return ConstMapOfFactIterator(mergeTwoListsWithNoDoubleEltCheck(*exactMatchPtr, itForAnyValue->second));
        return ConstMapOfFactIterator(mergeTwoLists(itForThisValue->second, itForAnyValue->second));
      }
      if (exactMatchPtr == nullptr)
      {
        if (resPtr != nullptr)
          return ConstMapOfFactIterator(intersectTwoLists(*resPtr, itForThisValue->second));
        resPtr = &itForThisValue->second;
      }
    }
    else
    {
      auto itForAnyValue = pArgValueToValues.find("");
      if (itForAnyValue != pArgValueToValues.end())
      {
        if (resPtr != nullptr)
          return ConstMapOfFactIterator(intersectTwoLists(*resPtr, itForAnyValue->second));
        resPtr = &itForAnyValue->second;
      }
      else
      {
        return ConstMapOfFactIterator(nullptr);
      }
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
      return ConstMapOfFactIterator(&parameterToValues.all);
  }

  if (resPtr != nullptr)
    return ConstMapOfFactIterator(resPtr);
  return ConstMapOfFactIterator(exactMatchPtr);
}

typename FactsToValue::ConstMapOfFactIterator FactsToValue::valuesWithoutFact() const
{
  return ConstMapOfFactIterator(&_valuesWithoutFact);
}



void FactsToValue::_removeAValueForList(std::list<std::string>& pList,
                                            const std::string& pValue) const
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


const std::list<std::string>* FactsToValue::_findAnExactCall(
    const std::optional<std::map<std::string, std::list<std::string>>>& pExactCalls,
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

