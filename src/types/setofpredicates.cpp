#include <orderedgoalsplanner/types/setofpredicates.hpp>
#include <vector>
#include <orderedgoalsplanner/types/setoftypes.hpp>
#include <orderedgoalsplanner/util/util.hpp>
#include "expressionParsed.hpp"

namespace ogp
{

SetOfPredicates::SetOfPredicates()
    : _nameToPredicate()
{
}


SetOfPredicates SetOfPredicates::fromStr(const std::string& pStr,
                                         const SetOfTypes& pSetOfTypes)
{
  SetOfPredicates res;
  std::vector<std::string> lineSplitted;
  ogp::split(lineSplitted, pStr, "\n");
  for (auto& currLine : lineSplitted)
    if (!currLine.empty())
      res.addPredicate(Predicate(currLine, false, pSetOfTypes));
  return res;
}


SetOfPredicates SetOfPredicates::fromPddl(const std::string& pStr,
                                          std::size_t& pPos,
                                          const SetOfTypes& pSetOfTypes,
                                          const std::shared_ptr<Type>& pDefaultValue)
{
  auto strSize = pStr.size();
  ExpressionParsed::skipSpaces(pStr, pPos);

  SetOfPredicates res;
  while (pPos < strSize && pStr[pPos] != ')')
  {
    Predicate predicate(pStr, true, pSetOfTypes, pPos, &pPos);
    if (pDefaultValue && !predicate.value)
      predicate.value = pDefaultValue;
    res.addPredicate(std::move(predicate));
  }
  return res;
}


void SetOfPredicates::addAll(const SetOfPredicates& pOther)
{
  for (const auto& currNameToPredicate : pOther._nameToPredicate)
    addPredicate(currNameToPredicate.second);
}

void SetOfPredicates::addPredicate(const Predicate& pPredicate)
{
  _nameToPredicate.erase(pPredicate.name);
  _nameToPredicate.emplace(pPredicate.name, pPredicate);
}

const Predicate* SetOfPredicates::nameToPredicatePtr(const std::string& pName) const
{
  auto it = _nameToPredicate.find(pName);
  if (it != _nameToPredicate.end())
    return &it->second;
  return nullptr;
}

Predicate SetOfPredicates::nameToPredicate(const std::string& pName) const
{
  auto it = _nameToPredicate.find(pName);
  if (it != _nameToPredicate.end())
    return it->second;

  throw std::runtime_error("\"" + pName + "\" is not a predicate name");
}


std::string SetOfPredicates::toPddl(PredicatePddlType pTypeFilter,
                                    std::size_t pIdentation) const
{
  std::string res;
  bool firstIteration = true;
  for (auto& currNameToPredicate : _nameToPredicate)
  {
    if (pTypeFilter == PredicatePddlType::PDDL_PREDICATE && currNameToPredicate.second.value)
      continue;
    if (pTypeFilter == PredicatePddlType::PDDL_FUNCTION && !currNameToPredicate.second.value)
      continue;

    if (firstIteration)
      firstIteration = false;
    else
      res += "\n";
    res += std::string(pIdentation, ' ') + currNameToPredicate.second.toPddl();
  }
  return res;
}

std::string SetOfPredicates::toStr() const
{
  std::string res;
  bool firstIteration = true;
  for (auto& currNameToPredicate : _nameToPredicate)
  {
    if (firstIteration)
      firstIteration = false;
    else
      res += "\n";
    res += currNameToPredicate.second.toStr();
  }
  return res;
}

bool SetOfPredicates::hasPredicateOfPddlType(PredicatePddlType pTypeFilter) const
{
  for (auto& currNameToPredicate : _nameToPredicate)
  {
    if (pTypeFilter == PredicatePddlType::PDDL_PREDICATE && currNameToPredicate.second.value)
      continue;
    if (pTypeFilter == PredicatePddlType::PDDL_FUNCTION && !currNameToPredicate.second.value)
      continue;
    return true;
  }
  return false;
}


} // !ogp
