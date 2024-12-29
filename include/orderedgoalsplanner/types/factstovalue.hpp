#ifndef INCLUDE_ORDEREDGOALSPLANNER_FACTSTOVALUE_HPP
#define INCLUDE_ORDEREDGOALSPLANNER_FACTSTOVALUE_HPP

#include "../util/api.hpp"
#include <list>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>
#include <sstream>
#include <orderedgoalsplanner/types/fact.hpp>

namespace ogp
{

struct ORDEREDGOALSPLANNER_API FactWithId
{
  FactWithId(const Fact& pFact,
             const std::string& pId)
    : fact(pFact),
      id(pId)
  {}

  Fact fact;
  std::string id;
};


struct ORDEREDGOALSPLANNER_API FactsToValue
{
  FactsToValue();

  void add(const Fact& pFact,
           const std::string& pId,
           bool pIgnoreValue = false);

  bool empty() const;

  struct FactWithValuePtr
  {
    FactWithValuePtr(const Fact* pFactPtr,
                     const std::string* pValuePtr)
     : factPtr(pFactPtr),
       valuePtr(pValuePtr)
    {
    }
    bool operator==(const FactWithValuePtr& pOther) const {
      return factPtr == pOther.factPtr && valuePtr == pOther.valuePtr;
    }
    const Fact* factPtr;
    const std::string* valuePtr;
  };
  class ConstMapOfFactIterator {
     public:
         // Constructor accepting reference to std::list<Toto*>
         ConstMapOfFactIterator(const std::list<std::string>* listPtr)
           : _listPtr(listPtr),
             _list()
         {}

         ConstMapOfFactIterator(std::list<std::string>&& list)
           : _listPtr(nullptr),
             _list(std::move(list))
         {}

         // Custom iterator class for non-const access
         class Iterator {
             typename std::list<std::string>::const_iterator iter;

         public:
             Iterator(typename std::list<std::string>::const_iterator it) : iter(it) {}

             // Overload the dereference operator to return Toto& instead of Toto*
             const std::string& operator*() const { return *iter; }

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

         std::string toStr() const {
           std::stringstream ss;
           ss << "[";
           bool firstElt = true;
           for (const auto& currElt : *this)
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

     private:
         const std::list<std::string>* _listPtr;
         std::list<std::string> _list;
  };

  ConstMapOfFactIterator find(const Fact& pFact,
                              bool pIgnoreValue = false) const;


private:
  std::optional<std::map<std::string, std::list<std::string>>> _exactCallToListsOpt;
  std::optional<std::map<std::string, std::list<std::string>>> _exactCallWithoutValueToListsOpt;
  struct ParameterToValues
  {
    ParameterToValues(std::size_t pNbOfArgs)
     : all(),
       argIdToArgValueToValues(pNbOfArgs),
       fluentValueToValues()
    {
    }
    std::list<std::string> all;
    std::vector<std::map<std::string, std::list<std::string>>> argIdToArgValueToValues;
    std::map<std::string, std::list<std::string>> fluentValueToValues;
  };
  std::map<std::string, ParameterToValues> _signatureToLists;

  // TODO: can be static
  void _removeAValueForList(std::list<std::string>& pList,
                            const std::string& pValue) const;

  const std::list<std::string>* _findAnExactCall(const std::optional<std::map<std::string, std::list<std::string>>>& pExactCalls,
                                                 const std::string& pExactCall) const;

};

} // !ogp


#endif // INCLUDE_ORDEREDGOALSPLANNER_FACTSTOVALUE_HPP
