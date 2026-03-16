#ifndef INCLUDE_ORDEREDGOALSPLANNER_DERIVEDPREDICATE_HPP
#define INCLUDE_ORDEREDGOALSPLANNER_DERIVEDPREDICATE_HPP

#include "../util/api.hpp"
#include <orderedgoalsplanner/types/predicate.hpp>
#include <orderedgoalsplanner/types/condition.hpp>


namespace ogp
{

struct ORDEREDGOALSPLANNER_API DerivedPredicate
{
  DerivedPredicate(const Predicate& pPredicate,
                   const std::string& pConditionStr,
                   const ogp::Ontology& pOntology);
  DerivedPredicate(const Predicate& pPredicate,
                   std::unique_ptr<Condition> pCondition);

  /// Copy constructor.
  DerivedPredicate(const DerivedPredicate& pDerivedPredicate);
  /// Copy operator.
  void operator=(const DerivedPredicate& pDerivedPredicate);

  std::string toPddl(std::size_t pIdentation = 0) const;

  DerivedPredicate createImmutableCopy() const;

  /// Predicate.
  Predicate predicate;
  /// Condition to put when this is used predicate.
  std::unique_ptr<Condition> condition;
};

} // !ogp


#endif // INCLUDE_ORDEREDGOALSPLANNER_DERIVEDPREDICATE_HPP
