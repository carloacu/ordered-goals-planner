#ifndef INCLUDE_ORDEREDGOALSPLANNER_TYPES_ONTOLOGY_HPP
#define INCLUDE_ORDEREDGOALSPLANNER_TYPES_ONTOLOGY_HPP

#include "../util/api.hpp"
#include "setofderivedpredicates.hpp"
#include "setofentities.hpp"
#include "setofpredicates.hpp"
#include "setoftypes.hpp"


namespace ogp
{

struct ORDEREDGOALSPLANNER_API Ontology
{
  SetOfTypes types;
  SetOfPredicates predicates;
  SetOfEntities constants;
  SetOfDerivedPredicates derivedPredicates;


  const Predicate* nameToPredicatePtr(const std::string& pName) const
  {
    auto* predicatePtr = predicates.nameToPredicatePtr(pName);
    if (predicatePtr != nullptr)
      return predicatePtr;
    return derivedPredicates.nameToPredicatePtr(pName);
  }
};

} // namespace ogp

#endif // INCLUDE_ORDEREDGOALSPLANNER_TYPES_ONTOLOGY_HPP
