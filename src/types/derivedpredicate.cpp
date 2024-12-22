#include <orderedgoalsplanner/types/derivedpredicate.hpp>
#include <orderedgoalsplanner/types/setofentities.hpp>
#include <orderedgoalsplanner/util/serializer/deserializefrompddl.hpp>

namespace ogp
{

DerivedPredicate::DerivedPredicate(const Predicate& pPredicate,
                                   const std::string& pConditionStr,
                                   const ogp::Ontology& pOntology)
  : predicate(pPredicate),
    condition()
{
  std::vector<ogp::Parameter> parameters;
  for (const auto& currParam : pPredicate.parameters)
    if (currParam.isAParameterToFill())
      parameters.emplace_back(currParam);
  if (pPredicate.value)
    parameters.emplace_back(Parameter::fromType(pPredicate.value));

  condition = strToCondition(pConditionStr, pOntology, {}, parameters);
}


DerivedPredicate::DerivedPredicate(const DerivedPredicate& pDerivedPredicate)
  : predicate(pDerivedPredicate.predicate),
    condition(pDerivedPredicate.condition->clone())
{
}

void DerivedPredicate::operator=(const DerivedPredicate& pDerivedPredicate)
{
  predicate = pDerivedPredicate.predicate;
  condition = pDerivedPredicate.condition->clone();
}


} // !ogp
