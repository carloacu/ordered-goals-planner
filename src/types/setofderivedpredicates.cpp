#include <orderedgoalsplanner/types/setofderivedpredicates.hpp>
#include <orderedgoalsplanner/types/derivedpredicate.hpp>


namespace ogp
{

SetOfDerivedPredicates::SetOfDerivedPredicates()
    : _nameToDerivedPredicate()
{
}

void SetOfDerivedPredicates::addAll(const SetOfDerivedPredicates& pOther)
{
  for (const auto& currNameToDerivedPredicate : pOther._nameToDerivedPredicate)
    addDerivedPredicate(currNameToDerivedPredicate.second);
}

void SetOfDerivedPredicates::addDerivedPredicate(const DerivedPredicate& pDerivedPredicate)
{
  _nameToDerivedPredicate.erase(pDerivedPredicate.predicate.name);
  _nameToDerivedPredicate.emplace(pDerivedPredicate.predicate.name, pDerivedPredicate);
}


const Predicate* SetOfDerivedPredicates::nameToPredicatePtr(const std::string& pPredicateName) const
{
  auto it = _nameToDerivedPredicate.find(pPredicateName);
  if (it != _nameToDerivedPredicate.end())
    return &it->second.predicate;
  return nullptr;
}

std::unique_ptr<Condition> SetOfDerivedPredicates::optFactToConditionPtr(const FactOptional& pFactOptional,
                                                                         bool pAutoAddImmutablePredicates) const
{
  auto it = _nameToDerivedPredicate.find(pFactOptional.fact.name());
  if (it != _nameToDerivedPredicate.end())
  {
    std::map<Parameter, Entity> conditionParametersToArgument = pFactOptional.fact.extratParameterToArguments();
    return it->second.condition->clone(&conditionParametersToArgument, pFactOptional.isFactNegated, nullptr, pAutoAddImmutablePredicates);
  }
  return {};
}

std::string SetOfDerivedPredicates::toPddl(std::size_t pIdentation) const
{
  std::string res;
  bool firstIteration = true;
  for (const auto& currNameToDerivedPredicate : _nameToDerivedPredicate)
  {
    if (firstIteration)
      firstIteration = false;
    else
      res += "\n\n";
    res += currNameToDerivedPredicate.second.toPddl(pIdentation);
  }
  return res;
}


void SetOfDerivedPredicates::updateImmutablePredicates()
{
  std::map<std::string, DerivedPredicate> nameToImmutableDerPredicate;
  for (auto& currNameToPredicate : _nameToDerivedPredicate) {
    if (!currNameToPredicate.second.predicate.isImmutable()) {
      auto newPredicate = currNameToPredicate.second.createImmutableCopy();
      nameToImmutableDerPredicate.emplace(currNameToPredicate.first, std::move(newPredicate));
    }
  }
  for (auto& currNameToImmutablePredicate : nameToImmutableDerPredicate)
    addDerivedPredicate(currNameToImmutablePredicate.second);
}


} // !ogp
