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
 : _exactCallToListsOpt(),
   _exactCallWithoutValueToListsOpt(),
   _signatureToLists()
{
}


void FactsToValue::add(const Fact& pFact,
                       const std::string& pId,
                       bool pIgnoreValue)
{
  auto factWithId = FactWithId(pFact, pId);

  if (!pFact.hasAParameter())
  {
    auto exactCallStr = _getExactCall(pFact);
    if (!_exactCallWithoutValueToListsOpt)
      _exactCallWithoutValueToListsOpt.emplace();
    (*_exactCallWithoutValueToListsOpt)[exactCallStr].emplace_back(factWithId);

    if (!pIgnoreValue && pFact.value())
    {
      _addValueToExactCall(exactCallStr, pFact);
      if (!_exactCallToListsOpt)
        _exactCallToListsOpt.emplace();
      (*_exactCallToListsOpt)[exactCallStr].emplace_back(factWithId);
    }
  }

  auto factSignatures = pFact.generateSignatureForSubAndUpperTypes2();
  for (auto& currSignature : factSignatures)
  {
    auto& factArguments = pFact.arguments();
    auto insertionRes = _signatureToLists.try_emplace(currSignature, factArguments.size());
    ParameterToValues& parameterToValues = insertionRes.first->second;

    parameterToValues.all.emplace_back(factWithId);
    for (std::size_t i = 0; i < factArguments.size(); ++i)
    {
      if (!factArguments[i].isAParameterToFill())
        parameterToValues.argIdToArgValueToValues[i][factArguments[i].value].emplace_back(factWithId);
      else
        parameterToValues.argIdToArgValueToValues[i][""].emplace_back(factWithId);
    }
    if (pIgnoreValue || pFact.value())
    {
      if (!pIgnoreValue && !pFact.value()->isAParameterToFill() && !pFact.isValueNegated())
        parameterToValues.fluentValueToValues[pFact.value()->value].emplace_back(factWithId);
      else
        parameterToValues.fluentValueToValues[""].emplace_back(factWithId);
    }
  }
}


bool FactsToValue::empty() const
{
  return _signatureToLists.empty();
}


typename FactsToValue::ConstMapOfFactIterator FactsToValue::find(const Fact& pFact,
                                                                 bool pIgnoreValue) const
{
  const std::list<FactWithId>* exactMatchPtr = nullptr;

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

  const std::list<FactWithId>* resPtr = nullptr;
  auto _matchArg = [&](const std::map<std::string, std::list<FactWithId>>& pArgValueToValues,
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

  auto itParameterToValues = _signatureToLists.find(pFact.factSignature2());
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
    if (!pIgnoreValue && fluentValue && !fluentValue->isAParameterToFill() && !pFact.isValueNegated())
    {
      hasOnlyParameters = false;
      auto subRes = _matchArg(parameterToValues.fluentValueToValues, fluentValue->value);
      if (subRes)
        return *subRes;
    }

    if (hasOnlyParameters)
      return ConstMapOfFactIterator(&parameterToValues.all);
  }

  if (resPtr != nullptr)
    return ConstMapOfFactIterator(resPtr);
  return ConstMapOfFactIterator(exactMatchPtr);
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


const std::list<FactWithId>* FactsToValue::_findAnExactCall(
    const std::optional<std::map<std::string, std::list<FactWithId>>>& pExactCalls,
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

