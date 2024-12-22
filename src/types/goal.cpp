#include <orderedgoalsplanner/types/goal.hpp>
#include <assert.h>
#include <orderedgoalsplanner/types/condtionstovalue.hpp>
#include <orderedgoalsplanner/types/domain.hpp>
#include <orderedgoalsplanner/util/serializer/deserializefrompddl.hpp>
#include <orderedgoalsplanner/util/serializer/serializeinpddl.hpp>

namespace ogp
{

Goal::Goal(std::unique_ptr<Condition> pObjective,
           bool pIsPersistentIfSkipped,
           bool pOneStepTowards,
           int pMaxTimeToKeepInactive,
           const std::string& pGoalGroupId)
  : _objective(std::move(pObjective)),
    _maxTimeToKeepInactive(pMaxTimeToKeepInactive),
    _inactiveSince(),
    _isPersistentIfSkipped(pIsPersistentIfSkipped),
    _oneStepTowards(pOneStepTowards),
    _goalGroupId(pGoalGroupId),
    _uuidOfLastDomainUsedForCache(),
    _cacheOfActionsThatCanSatisfyThisGoal(),
    _cacheOfEventsIdThatCanSatisfyThisGoal(),
    _cacheOfActionsPredecessors(),
    _cacheOfEventsPredecessors()
{
  assert(_objective);
}


Goal::Goal(const Goal& pOther,
           const std::map<Parameter, Entity>* pParametersPtr,
           const std::string* pGoalGroupIdPtr)
  : _objective(pOther._objective->clone(pParametersPtr)),
    _maxTimeToKeepInactive(pOther._maxTimeToKeepInactive),
    _inactiveSince(pOther._inactiveSince ? std::make_unique<std::chrono::steady_clock::time_point>(*pOther._inactiveSince) : std::unique_ptr<std::chrono::steady_clock::time_point>()),
    _isPersistentIfSkipped(pOther._isPersistentIfSkipped),
    _oneStepTowards(pOther._oneStepTowards),
    _goalGroupId(pGoalGroupIdPtr != nullptr ? *pGoalGroupIdPtr : pOther._goalGroupId),
    _uuidOfLastDomainUsedForCache(pOther._uuidOfLastDomainUsedForCache),
    _cacheOfActionsThatCanSatisfyThisGoal(pOther._cacheOfActionsThatCanSatisfyThisGoal),
    _cacheOfEventsIdThatCanSatisfyThisGoal(pOther._cacheOfEventsIdThatCanSatisfyThisGoal),
    _cacheOfActionsPredecessors(pOther._cacheOfActionsPredecessors),
    _cacheOfEventsPredecessors(pOther._cacheOfEventsPredecessors)
{
}


Goal Goal::fromStr(const std::string& pStr,
                   const Ontology& pOntology,
                   const SetOfEntities& pEntities,
                   int pMaxTimeToKeepInactive,
                   const std::string& pGoalGroupId)
{
  auto resPtr = strToGoal(pStr, pOntology, pEntities, pMaxTimeToKeepInactive, pGoalGroupId);
  if (!resPtr)
    throw std::runtime_error("Failed to load the goal: " + pStr);
  return *resPtr;
}


void Goal::operator=(const Goal& pOther)
{
  _objective = pOther._objective->clone();
  _maxTimeToKeepInactive = pOther._maxTimeToKeepInactive;
  if (pOther._inactiveSince)
    _inactiveSince = std::make_unique<std::chrono::steady_clock::time_point>(*pOther._inactiveSince);
  else
    _inactiveSince.reset();
  _isPersistentIfSkipped = pOther._isPersistentIfSkipped;
  _oneStepTowards = pOther._oneStepTowards;
  _goalGroupId = pOther._goalGroupId;
  _uuidOfLastDomainUsedForCache = pOther._uuidOfLastDomainUsedForCache;
  _cacheOfActionsThatCanSatisfyThisGoal = pOther._cacheOfActionsThatCanSatisfyThisGoal;
  _cacheOfEventsIdThatCanSatisfyThisGoal = pOther._cacheOfEventsIdThatCanSatisfyThisGoal;
  _cacheOfActionsPredecessors = pOther._cacheOfActionsPredecessors;
  _cacheOfEventsPredecessors = pOther._cacheOfEventsPredecessors;
}

bool Goal::operator==(const Goal& pOther) const
{
  return *_objective == *pOther._objective &&
      _maxTimeToKeepInactive == pOther._maxTimeToKeepInactive &&
      _isPersistentIfSkipped == pOther._isPersistentIfSkipped &&
      _oneStepTowards == pOther._oneStepTowards &&
      _goalGroupId == pOther._goalGroupId &&
      _uuidOfLastDomainUsedForCache == pOther._uuidOfLastDomainUsedForCache &&
      _cacheOfActionsThatCanSatisfyThisGoal == pOther._cacheOfActionsThatCanSatisfyThisGoal &&
      _cacheOfEventsIdThatCanSatisfyThisGoal == pOther._cacheOfEventsIdThatCanSatisfyThisGoal &&
      _cacheOfActionsPredecessors == pOther._cacheOfActionsPredecessors &&
      _cacheOfEventsPredecessors == pOther._cacheOfEventsPredecessors;
}

std::unique_ptr<Goal> Goal::clone() const
{
  return std::make_unique<Goal>(*this);
}

void Goal::setInactiveSinceIfNotAlreadySet(const std::unique_ptr<std::chrono::steady_clock::time_point>& pInactiveSince)
{
  if (!_inactiveSince && pInactiveSince)
    _inactiveSince = std::make_unique<std::chrono::steady_clock::time_point>(*pInactiveSince);
}


bool Goal::isInactiveForTooLong(const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow) const
{
  if (_maxTimeToKeepInactive < 0)
    return false;
  if (_maxTimeToKeepInactive == 0)
    return true;
  return _inactiveSince && pNow &&
      std::chrono::duration_cast<std::chrono::seconds>(*pNow - *_inactiveSince).count() > _maxTimeToKeepInactive;
}

void Goal::notifyActivity()
{
  _inactiveSince.reset();
}

std::string Goal::toStr() const
{
  auto res = _objective->toStr();
  if (_oneStepTowards)
    res = getOneStepTowardsFunctionName() + "(" + res + ")";
  if (_isPersistentIfSkipped)
    res = getPersistFunctionName() + "(" + res + ")";
  return res;
}

std::string Goal::toPddl(std::size_t pIdentation) const
{
  auto res = conditionToPddl(*_objective, pIdentation);
  if (_oneStepTowards)
    res += " ;; _ONE_STEP_TOWARDS";
  if (_isPersistentIfSkipped)
    res += " ;; __PERSIST";
  return res;
}


bool Goal::isASimpleFactObjective() const
{
  return _objective && _objective->fcFactPtr() != nullptr;
}


void Goal::refreshIfNeeded(const Domain& pDomain)
{
  if (_uuidOfLastDomainUsedForCache == pDomain.getUuid())
    return;
  _uuidOfLastDomainUsedForCache = pDomain.getUuid();

  ConditionsToValue conditionsToValue;
  conditionsToValue.add(*_objective, "goal");
  _cacheOfActionsPredecessors.clear();
  _cacheOfEventsPredecessors.clear();

  auto optFactIteration = [&](const FactOptional& pFactOptional,
                              const std::unique_ptr<Condition>& pPreCondition,
                              const std::unique_ptr<Condition>& pConditionOverAll) -> ContinueOrBreak {
    // Check that pFactOptional is not in the condtions
    if (pPreCondition && pPreCondition->isOptFactMandatory(pFactOptional))
      return ContinueOrBreak::CONTINUE;
    if (pConditionOverAll && pConditionOverAll->isOptFactMandatory(pFactOptional))
      return ContinueOrBreak::CONTINUE;

    const FactsToValue& factsToValue = pFactOptional.isFactNegated ?
          conditionsToValue.notFactsToValue() : conditionsToValue.factsToValue();
    if (!factsToValue.find(pFactOptional.fact).empty())
      return ContinueOrBreak::BREAK;

    if (pFactOptional.fact.value())
    {
      const FactsToValue& invertedFactsToValue = pFactOptional.isFactNegated ?
            conditionsToValue.factsToValue() : conditionsToValue.notFactsToValue();
      if (!invertedFactsToValue.find(pFactOptional.fact, true).empty())
        return ContinueOrBreak::BREAK;
    }

    return ContinueOrBreak::CONTINUE;
  };

  // Update actions cache
  _cacheOfActionsThatCanSatisfyThisGoal.clear();
  for (const auto& currIdToAction : pDomain.getActions())
  {
    auto search = ContinueOrBreak::CONTINUE;
    const Action& currAction = currIdToAction.second;
    const auto& currPrecondition = currAction.precondition;
    const auto& currConditionOverAll = currAction.overAllCondition;
    const ProblemModification& currEffect = currAction.effect;
    if (currEffect.worldStateModification)
    {
      search = currEffect.worldStateModification->forAllThatCanBeModified([&](const FactOptional& pFactOptional) {
        return optFactIteration(pFactOptional, currPrecondition, currConditionOverAll);
      });
    }
    if (currEffect.potentialWorldStateModification && search == ContinueOrBreak::CONTINUE)
    {
      search = currEffect.potentialWorldStateModification->forAllThatCanBeModified([&](const FactOptional& pFactOptional) {
        return optFactIteration(pFactOptional, currPrecondition, currConditionOverAll);
      });
    }
    if (search == ContinueOrBreak::BREAK)
    {
      _cacheOfActionsThatCanSatisfyThisGoal.insert(currIdToAction.first);
      _cacheOfActionsPredecessors.insert(currIdToAction.first);
      _cacheOfActionsPredecessors.insert(currAction.actionsPredecessorsCache.begin(),
                                         currAction.actionsPredecessorsCache.end());
      _cacheOfEventsPredecessors.insert(currAction.eventsPredecessorsCache.begin(),
                                        currAction.eventsPredecessorsCache.end());
    }
  }

  // Update events cache
  _cacheOfEventsIdThatCanSatisfyThisGoal.clear();
  for (const auto& currIdToSetOfEvents : pDomain.getSetOfEvents())
  {
    for (const auto& currSetOfEvents : currIdToSetOfEvents.second.events())
    {
      auto fullEventId = currIdToSetOfEvents.first + "|" + currSetOfEvents.first;
      const Event& currEvent = currSetOfEvents.second;
      if (currEvent.factsToModify)
      {
        auto search = currEvent.factsToModify->forAllThatCanBeModified([&](const FactOptional& pFactOptional) {
          return optFactIteration(pFactOptional, currEvent.precondition, {});
        });
        if (search == ContinueOrBreak::BREAK)
        {
          _cacheOfEventsIdThatCanSatisfyThisGoal.insert(fullEventId);
          _cacheOfEventsPredecessors.insert(fullEventId);
          _cacheOfActionsPredecessors.insert(currEvent.actionsPredecessorsCache.begin(),
                                             currEvent.actionsPredecessorsCache.end());
          _cacheOfEventsPredecessors.insert(currEvent.eventsPredecessorsCache.begin(),
                                            currEvent.eventsPredecessorsCache.end());
        }
      }
    }
  }
}


std::string Goal::printActionsThatCanSatisfyThisGoal() const
{
  std::string res;

  // Print actions cache
  bool firstIteration = true;
  for (const auto& currId : _cacheOfActionsThatCanSatisfyThisGoal)
  {
    if (firstIteration)
    {
      firstIteration = false;
      res += "actions:";
    }
    else
    {
      res += ",";
    }
    res += " " + currId;
  }

  // Print events cache
  firstIteration = true;
  for (const auto& currId : _cacheOfEventsIdThatCanSatisfyThisGoal)
  {
    if (firstIteration)
    {
      firstIteration = false;
      if (res != "")
        res += "\n";
      res += "events:";
    }
    else
    {
      res += ",";
    }
    res += " " + currId;
  }

  return res;
}


bool Goal::canActionSatisfyThisGoal(const ActionId& pActionId) const
{
  return _cacheOfActionsThatCanSatisfyThisGoal.count(pActionId) > 0;
}

bool Goal::canEventSatisfyThisGoal(const ActionId& pFullEventId) const
{
  return _cacheOfEventsIdThatCanSatisfyThisGoal.count(pFullEventId) > 0;
}

bool Goal::canDeductionSatisfyThisGoal(const ActionId& pDeductionId) const
{
  return canActionSatisfyThisGoal(pDeductionId) || canEventSatisfyThisGoal(pDeductionId);
}


const std::string& Goal::getPersistFunctionName()
{
  static const std::string persistFunctionName = "persist";
  return persistFunctionName;
}

const std::string& Goal::getOneStepTowardsFunctionName()
{
  static const std::string oneStepTowardsFunctionName = "oneStepTowards";
  return oneStepTowardsFunctionName;
}



} // !ogp
