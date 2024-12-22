#ifndef INCLUDE_ORDEREDGOALSPLANNER_TYPES_SETOFPREDICATES_HPP
#define INCLUDE_ORDEREDGOALSPLANNER_TYPES_SETOFPREDICATES_HPP

#include "../util/api.hpp"
#include <list>
#include <map>
#include <string>
#include "predicate.hpp"

namespace ogp
{
struct SetOfTypes;

enum class PredicatePddlType
{
  PDDL_PREDICATE,
  PDDL_FUNCTION
};

struct ORDEREDGOALSPLANNER_API SetOfPredicates
{
  SetOfPredicates();

  static SetOfPredicates fromStr(const std::string& pStr,
                                 const SetOfTypes& pSetOfTypes);

  static SetOfPredicates fromPddl(const std::string& pStr,
                                  std::size_t& pPos,
                                  const SetOfTypes& pSetOfTypes,
                                  const std::shared_ptr<Type>& pDefaultValue = {});

  void addAll(const SetOfPredicates& pOther);
  void addPredicate(const Predicate& pPredicate);

  const Predicate* nameToPredicatePtr(const std::string& pName) const;
  Predicate nameToPredicate(const std::string& pName) const;

  std::string toPddl(PredicatePddlType pTypeFilter, std::size_t pIdentation = 0) const;
  std::string toStr() const;

  bool empty() const { return _nameToPredicate.empty(); }
  bool hasPredicateOfPddlType(PredicatePddlType pTypeFilter) const;

private:
  std::map<std::string, Predicate> _nameToPredicate;
};

} // namespace ogp

#endif // INCLUDE_ORDEREDGOALSPLANNER_TYPES_SETOFPREDICATES_HPP
