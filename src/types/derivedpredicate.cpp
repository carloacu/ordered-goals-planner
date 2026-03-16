#include <orderedgoalsplanner/types/derivedpredicate.hpp>
#include <orderedgoalsplanner/types/setofentities.hpp>
#include <orderedgoalsplanner/util/serializer/deserializefrompddl.hpp>
#include <orderedgoalsplanner/util/serializer/serializeinpddl.hpp>

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


DerivedPredicate::DerivedPredicate(const Predicate& pPredicate,
                                   std::unique_ptr<Condition> pCondition)
  : predicate(pPredicate),
    condition(std::move(pCondition))
{
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

std::string DerivedPredicate::toPddl(std::size_t pIdentation) const
{
  static const std::size_t indentationOffset = 4;
  std::string res = std::string(pIdentation, ' ') + "(:derived " + predicate.toPddl() + "\n";
  res += std::string(pIdentation + indentationOffset, ' ') +
      conditionToPddl(*condition, pIdentation + indentationOffset) + "\n";
  res += std::string(pIdentation, ' ') + ")";
  return res;
}


DerivedPredicate DerivedPredicate::createImmutableCopy() const
{
  return DerivedPredicate(predicate.createImmutableCopy(), condition->clone(nullptr, false, nullptr, true));
}

} // !ogp
