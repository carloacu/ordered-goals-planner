#include <orderedgoalsplanner/types/factoptionalstoid.hpp>
#include <memory>
#include <sstream>
#include <orderedgoalsplanner/types/condition.hpp>
#include <orderedgoalsplanner/types/factoptional.hpp>

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

struct FactWithId
{
  FactWithId(const FactOptional& pFactOptional,
             const std::string& pId)
    : factOptional(pFactOptional),
      id(pId)
  {}

  FactOptional factOptional;
  std::string id;

  bool operator==(const FactWithId& pOther) const {
    return factOptional == pOther.factOptional && id == pOther.id;
  }
};


struct FactsToId
{
  FactsToId();

  void add(const FactOptional& pFactOptional,
           const std::string& pId,
           bool pIgnoreValue = false);

  bool empty() const;

  class ConstMapOfFactIterator {
     public:
         // Constructor accepting reference to std::list<Toto*>
         ConstMapOfFactIterator(const std::list<FactWithId>* listPtr)
           : _listPtr(listPtr),
             _list()
         {}

         ConstMapOfFactIterator(std::list<FactWithId>&& list)
           : _listPtr(nullptr),
             _list(std::move(list))
         {}

         // Custom iterator class for non-const access
         class Iterator {
             typename std::list<FactWithId>::const_iterator iter;

         public:
             Iterator(typename std::list<FactWithId>::const_iterator it) : iter(it) {}

             // Overload the dereference operator to return Toto& instead of Toto*
             const FactWithId& operator*() const { return *iter; }

             // Pre-increment operator
             Iterator& operator++() {
                 ++iter;
                 return *this;
             }

             bool operator==(const Iterator& other) const { return iter == other.iter; }
             bool operator!=(const Iterator& other) const { return iter != other.iter; }
         };

         // Begin and end methods to return the custom iterator
         Iterator begin() const { return Iterator(_listPtr != nullptr ? _listPtr->begin() : _list.begin()); }
         Iterator end() const { return Iterator(_listPtr != nullptr ? _listPtr->end() : _list.end()); }
         bool empty() const { return begin() == end(); }
         std::size_t size() const {
           std::size_t res = 0;
           for (const auto& _ : *this)
             ++res;
           return res;
         }

     private:
         const std::list<FactWithId>* _listPtr;
         std::list<FactWithId> _list;
  };

  ConstMapOfFactIterator find(const Fact& pFact,
                              bool pIgnoreValue = false) const;


private:
  std::optional<std::map<std::string, std::list<FactWithId>>> _exactCallToListsOpt;
  std::optional<std::map<std::string, std::list<FactWithId>>> _exactCallWithoutValueToListsOpt;
  struct ParameterToValues
  {
    ParameterToValues(std::size_t pNbOfArgs)
     : all(),
       argIdToArgValueToValues(pNbOfArgs),
       fluentValueToValues()
    {
    }
    std::list<FactWithId> all;
    std::vector<std::map<std::string, std::list<FactWithId>>> argIdToArgValueToValues;
    std::map<std::string, std::list<FactWithId>> fluentValueToValues;
  };
  std::map<std::string, ParameterToValues> _signatureToLists;

  const std::list<FactWithId>* _findAnExactCall(const std::optional<std::map<std::string, std::list<FactWithId>>>& pExactCalls,
                                                const std::string& pExactCall) const;

};





FactsToId::FactsToId()
 : _exactCallToListsOpt(),
   _exactCallWithoutValueToListsOpt(),
   _signatureToLists()
{
}


void FactsToId::add(const FactOptional& pFactOptional,
                       const std::string& pId,
                       bool pIgnoreValue)
{
  auto factWithId = FactWithId(pFactOptional, pId);

  if (!pFactOptional.fact.hasAParameter())
  {
    auto exactCallStr = _getExactCall(pFactOptional.fact);
    if (!_exactCallWithoutValueToListsOpt)
      _exactCallWithoutValueToListsOpt.emplace();
    (*_exactCallWithoutValueToListsOpt)[exactCallStr].emplace_back(factWithId);

    if (!pIgnoreValue && pFactOptional.fact.value())
    {
      _addValueToExactCall(exactCallStr, pFactOptional.fact);
      if (!_exactCallToListsOpt)
        _exactCallToListsOpt.emplace();
      (*_exactCallToListsOpt)[exactCallStr].emplace_back(factWithId);
    }
  }

  pFactOptional.fact.generateSignaturesWithRelatedTypes([&](const std::string& pSignature) {
    auto& factArguments = pFactOptional.fact.arguments();
    auto insertionRes = _signatureToLists.try_emplace(pSignature, factArguments.size());
    ParameterToValues& parameterToValues = insertionRes.first->second;

    parameterToValues.all.emplace_back(factWithId);
    for (std::size_t i = 0; i < factArguments.size(); ++i)
    {
      if (!factArguments[i].isAParameterToFill())
        parameterToValues.argIdToArgValueToValues[i][factArguments[i].value].emplace_back(factWithId);
      else
        parameterToValues.argIdToArgValueToValues[i][""].emplace_back(factWithId);
    }
    if (pIgnoreValue || pFactOptional.fact.value())
    {
      if (!pIgnoreValue && !pFactOptional.fact.value()->isAParameterToFill() && !pFactOptional.fact.isValueNegated())
        parameterToValues.fluentValueToValues[pFactOptional.fact.value()->value].emplace_back(factWithId);
      else
        parameterToValues.fluentValueToValues[""].emplace_back(factWithId);
    }
  }, true, true);
}


bool FactsToId::empty() const
{
  return _signatureToLists.empty();
}


typename FactsToId::ConstMapOfFactIterator FactsToId::find(const Fact& pFact,
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
                       const std::string& pArgValue) -> std::optional<typename FactsToId::ConstMapOfFactIterator> {
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


const std::list<FactWithId>* FactsToId::_findAnExactCall(
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




FactOptionalsToId::FactOptionalsToId()
  : _factsToValue(std::make_unique<FactsToId>()),
    _notFactsToValue(std::make_unique<FactsToId>())
{
}


FactOptionalsToId::FactOptionalsToId(const FactOptionalsToId& pOther)
  : _factsToValue(std::make_unique<FactsToId>(*pOther._factsToValue)),
    _notFactsToValue(std::make_unique<FactsToId>(*pOther._notFactsToValue))
{
}


void FactOptionalsToId::operator=(const FactOptionalsToId& pOther)
{
  _factsToValue = std::make_unique<FactsToId>(*pOther._factsToValue);
  _notFactsToValue = std::make_unique<FactsToId>(*pOther._notFactsToValue);
}


FactOptionalsToId::~FactOptionalsToId()
{
}



void FactOptionalsToId::add(const FactOptional& pFactOptional,
                            const std::string& pId)
{
  if (pFactOptional.isFactNegated)
    _notFactsToValue->add(pFactOptional, pId, false);
  else
    _factsToValue->add(pFactOptional, pId, false);
}

bool FactOptionalsToId::add(const Condition& pCondition,
                            const std::string& pId)
{
  bool hasAddedAFact = false;
  pCondition.forAll(
        [&](const FactOptional& pFactOptional,
        bool pIgnoreValue)
  {
    if (pFactOptional.isFactNegated)
    {
      _notFactsToValue->add(pFactOptional, pId, pIgnoreValue);
    }
    else
    {
      _factsToValue->add(pFactOptional, pId, pIgnoreValue);
      hasAddedAFact = true;
    }
    return ContinueOrBreak::CONTINUE;
  }
  );
  return hasAddedAFact;
}


ContinueOrBreak FactOptionalsToId::find(const std::function<ContinueOrBreak (const std::string&)>& pCallback,
                                        const FactOptional& pFactOptional,
                                        bool pIgnoreValue) const
{
  return findFact(pCallback, pFactOptional.fact, pFactOptional.isFactNegated, pIgnoreValue);
}


ContinueOrBreak FactOptionalsToId::findFact(const std::function<ContinueOrBreak (const std::string&)>& pCallback,
                                            const Fact& pFact,
                                            bool pIsFactNegated,
                                            bool pIgnoreValue,
                                            bool pIncludeMatchingWithOlderValue) const
{
  std::set<std::string> alreadyReturnedIds;

  {
    const FactsToId& factsToValue = pIsFactNegated ? *_notFactsToValue : *_factsToValue;
    for (const auto& currMatch : factsToValue.find(pFact, pIgnoreValue))
    {
      if (alreadyReturnedIds.insert(currMatch.id).second &&
          pFact.areEqualExceptAnyParameters(currMatch.factOptional.fact, pIgnoreValue) &&
          pCallback(currMatch.id) == ContinueOrBreak::BREAK)
        return ContinueOrBreak::BREAK;
    }
  }

  if (pIncludeMatchingWithOlderValue && !pIsFactNegated && pFact.value())
  {
    for (const auto& currMatch : _notFactsToValue->find(pFact, true))
    {
      if (!alreadyReturnedIds.insert(currMatch.id).second)
        continue;
      if (!pFact.areEqualExceptAnyParameters(currMatch.factOptional.fact, true))
        continue;
      if (!currMatch.factOptional.fact.value())
         throw std::runtime_error("currMatch should have a value");
      if (currMatch.factOptional.isFactNegated &&
          currMatch.factOptional.fact.value() && currMatch.factOptional.fact.value()->isAnyEntity())
        continue;
      if (pFact.value()->isAParameterToFill() ||
          currMatch.factOptional.fact.value()->isAParameterToFill() ||
          pFact.value()->value != currMatch.factOptional.fact.value()->value)
      {
        if (pCallback(currMatch.id) == ContinueOrBreak::BREAK)
           return ContinueOrBreak::BREAK;
      }
    }
  }

  return ContinueOrBreak::CONTINUE;
}


} // !ogp

