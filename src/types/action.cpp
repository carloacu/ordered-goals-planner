#include <orderedgoalsplanner/types/action.hpp>
#include <orderedgoalsplanner/util/util.hpp>

namespace ogp
{


bool Action::operator==(const Action& pOther) const
{
  return parameters == pOther.parameters &&
      areUPtrEqual(precondition, pOther.precondition) &&
      areUPtrEqual(overAllCondition, pOther.overAllCondition) &&
      areUPtrEqual(preferInContext, pOther.preferInContext) &&
      effect == pOther.effect &&
      highImportanceOfNotRepeatingIt == pOther.highImportanceOfNotRepeatingIt;
}


Action Action::clone(const SetOfDerivedPredicates& pDerivedPredicates) const
{
  Action res(precondition ? precondition->clone(nullptr, false, &pDerivedPredicates) : std::unique_ptr<Condition>(),
             effect,
             preferInContext ? preferInContext->clone(nullptr, false, &pDerivedPredicates) : std::unique_ptr<Condition>());
  if (overAllCondition)
    res.overAllCondition = overAllCondition->clone(nullptr, false, &pDerivedPredicates);
  res.parameters = parameters;
  res.highImportanceOfNotRepeatingIt = highImportanceOfNotRepeatingIt;
  return res;
}

bool Action::hasFact(const ogp::Fact& pFact) const
{
  return (precondition && precondition->hasFact(pFact)) ||
      (preferInContext && preferInContext->hasFact(pFact)) ||
      effect.hasFact(pFact);
}

void Action::replaceArgument(const Entity& pOld,
                             const Entity& pNew)
{
  effect.replaceArgument(pOld, pNew);
}


void Action::updateSuccessionCache(const Domain& pDomain,
                                   const ActionId& pIdOfThisAction,
                                   const std::set<FactOptional>& pFactsFromCondition)
{
  WorldStateModificationContainerId containerId;
  containerId.actionIdToExclude.emplace(pIdOfThisAction);

  if (effect.worldStateModification)
    effect.worldStateModification->updateSuccesions(pDomain, containerId, pFactsFromCondition);
  if (effect.potentialWorldStateModification)
    effect.potentialWorldStateModification->updateSuccesions(pDomain, containerId, pFactsFromCondition);
}

void Action::removePossibleSuccessionCache(const ActionId& pActionIdToRemove)
{
  if (effect.worldStateModification)
    effect.worldStateModification->removePossibleSuccession(pActionIdToRemove);
  if (effect.potentialWorldStateModification)
    effect.potentialWorldStateModification->removePossibleSuccession(pActionIdToRemove);
}


std::string Action::printSuccessionCache() const
{
  std::string res;
  if (effect.worldStateModification)
    effect.worldStateModification->printSuccesions(res);
  if (effect.potentialWorldStateModification)
    effect.potentialWorldStateModification->printSuccesions(res);

  std::string actionWithoutInterestStr = "";
  for (const auto& currActionId : actionsSuccessionsWithoutInterestCache)
    actionWithoutInterestStr += "not action: " + currActionId + "\n";
  if (!actionWithoutInterestStr.empty())
  {
    if (res != "")
      res += "\n";
    res += actionWithoutInterestStr;
  }
  return res;
}

void Action::throwIfNotValid(const SetOfFacts& pSetOfFact)
{
  _throwIfNotValidForACondition(precondition);
  _throwIfNotValidForACondition(preferInContext);
  _throwIfNotValidForAnWordStateModif(effect.worldStateModification, pSetOfFact);
  _throwIfNotValidForAnWordStateModif(effect.potentialWorldStateModification, pSetOfFact);
  _throwIfNotValidForAnWordStateModif(effect.worldStateModificationAtStart, pSetOfFact);
}


void Action::_throwIfNotValidForACondition(const std::unique_ptr<Condition>& pPrecondition)
{
  if (pPrecondition)
    pPrecondition->forAll([&](const FactOptional& pFactOptional, bool) {
      _throwIfNotValidForAFact(pFactOptional.fact);
      return ContinueOrBreak::CONTINUE;
    });
}


void Action::_throwIfNotValidForAnWordStateModif(const std::unique_ptr<WorldStateModification>& pWs,
                                                 const SetOfFacts& pSetOfFact)
{
  if (pWs)
    pWs->forAll([&](const FactOptional& pFactOptional) {
      _throwIfNotValidForAFact(pFactOptional.fact);
      return ContinueOrBreak::CONTINUE;
    }, pSetOfFact);
}


void Action::_throwIfNotValidForAFact(const Fact& pFact)
{
  for (auto& currArgument : pFact.arguments())
    if (currArgument.isAParameterToFill() && !currArgument.isValidParameterAccordingToPossiblities(parameters))
      throw std::runtime_error("\"" + currArgument.value + "\" is missing in action parameters");

  if (pFact.value() && pFact.value()->isAParameterToFill() && !pFact.value()->isValidParameterAccordingToPossiblities(parameters))
    throw std::runtime_error("\"" + pFact.value()->value + "\" value is missing in action parameters");
}



} // !ogp
