#include <orderedgoalsplanner/util/extactminmaxvalueforfacts.hpp>
#include <orderedgoalsplanner/types/domain.hpp>
#include <orderedgoalsplanner/types/problem.hpp>


namespace ogp
{

std::map<std::string, MinMaxValues> extractMinMaxValuesForFacts(const Problem& pProblem,
                                                                const Domain& pDomain)
{
  std::map<std::string, MinMaxValues> res;
  for (const auto& currPrioToGoals : pProblem.goalStack.goals())
    for (const auto& currGoal : currPrioToGoals.second)
      currGoal.objective().extractMinMaxValuesForFacts(res);

  const auto& ontology = pDomain.getOntology();
  for (const auto& currDpPair : ontology.derivedPredicates.nameToDerivedPredicate())
  {
    const DerivedPredicate& currDp = currDpPair.second;
    if (currDp.condition)
      currDp.condition->extractMinMaxValuesForFacts(res);
  }

  for (const auto& currActionPair : pDomain.actions())
  {
    const Action& currAction = currActionPair.second;
    if (currAction.precondition)
      currAction.precondition->extractMinMaxValuesForFacts(res);
  }

  return res;
}


}
