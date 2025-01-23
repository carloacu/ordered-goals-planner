#include <orderedgoalsplanner/types/goalstack.hpp>
#include <map>
#include <sstream>
#include <orderedgoalsplanner/types/domain.hpp>
#include <orderedgoalsplanner/types/actioninvocationwithgoal.hpp>
#include <orderedgoalsplanner/types/setofevents.hpp>
#include <orderedgoalsplanner/util/util.hpp>
#include <orderedgoalsplanner/types/lookforanactionoutputinfos.hpp>


namespace ogp
{

GoalStack::GoalStack(const GoalStack& pOther)
  : onGoalsChanged(),
    _goals(pOther._goals),
    _currentGoalPtr(nullptr)
{
}

GoalStack& GoalStack::operator=(const GoalStack& pOther)
{
  _goals = pOther._goals;
  _currentGoalPtr = nullptr;
  return *this;
}


bool GoalStack::notifyActionDone(const ActionInvocationWithGoal& pOnStepOfPlannerResult,
                                 const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                                 const std::map<int, std::vector<Goal>>* pGoalsToAdd,
                                 const std::vector<Goal>* pGoalsToAddInCurrentPriority,
                                 const WorldState& pWorldState,
                                 const SetOfEntities& pConstants,
                                 const SetOfEntities& pObjects,
                                 LookForAnActionOutputInfos* pLookForAnActionOutputInfosPtr)
{
  bool goalChanged = false;

  // Remove current goal if it was one step towards
  int currentPriority = _getCurrentPriority(pWorldState, pConstants, pObjects);
  if (pOnStepOfPlannerResult.fromGoal && pOnStepOfPlannerResult.fromGoal->isOneStepTowards())
  {
    auto isNotGoalThatHasDoneOneStepForward = [&](const Goal& pGoal, int){ return pGoal != *pOnStepOfPlannerResult.fromGoal; };
    goalChanged = _iterateOnGoalsAndRemoveNonPersistent(isNotGoalThatHasDoneOneStepForward, pWorldState, pConstants, pObjects, pNow, pLookForAnActionOutputInfosPtr);
  }
  else // Else remove only the first goals already satisfied
  {
    goalChanged = _removeFirstGoalsThatAreAlreadySatisfied(pWorldState, pConstants, pObjects, pNow, pLookForAnActionOutputInfosPtr);
  }

  if (pGoalsToAdd != nullptr && !pGoalsToAdd->empty())
    goalChanged = _addGoals(*pGoalsToAdd, pWorldState, pConstants, pObjects, pNow) || goalChanged;
  if (pGoalsToAddInCurrentPriority != nullptr && !pGoalsToAddInCurrentPriority->empty())
    goalChanged = _addGoals(std::map<int, std::vector<ogp::Goal>>{{currentPriority, *pGoalsToAddInCurrentPriority}}, pWorldState, pConstants, pObjects, pNow) || goalChanged;

  if (goalChanged)
  {
    onGoalsChanged(_goals);
    return true;
  }
  return false;
}


bool GoalStack::_removeFirstGoalsThatAreAlreadySatisfied(const WorldState& pWorldState,
                                                         const SetOfEntities& pConstants,
                                                         const SetOfEntities& pObjects,
                                                         const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                                                         LookForAnActionOutputInfos* pLookForAnActionOutputInfosPtr)
{
  auto alwaysTrue = [&](const Goal&, int){ return true; };
  return _iterateOnGoalsAndRemoveNonPersistent(alwaysTrue, pWorldState, pConstants, pObjects, pNow, pLookForAnActionOutputInfosPtr);
}


bool GoalStack::_iterateOnGoalsAndRemoveNonPersistent(
    const std::function<bool(Goal&, int)>& pManageGoal,
    const WorldState& pWorldState,
    const SetOfEntities& pConstants,
    const SetOfEntities& pObjects,
    const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
    LookForAnActionOutputInfos* pLookForAnActionOutputInfosPtr)
{
  bool res = false;
  if (pLookForAnActionOutputInfosPtr != nullptr)
    pLookForAnActionOutputInfosPtr->setType(PlannerStepType::FINISHED_ON_SUCCESS);

  bool isCurrentlyActiveGoal = true;
  for (auto itGoalsGroup = _goals.end(); itGoalsGroup != _goals.begin(); )
  {
    --itGoalsGroup;
    for (auto itGoal = itGoalsGroup->second.begin(); itGoal != itGoalsGroup->second.end(); )
    {
      // If the goal was inactive for too long we remove it
      if (!isCurrentlyActiveGoal && itGoal->isInactiveForTooLong(pNow))
      {
        itGoal = itGoalsGroup->second.erase(itGoal);
        res = true;
        continue;
      }

      if (!pWorldState.isGoalSatisfied(*itGoal, pConstants, pObjects))
      {
        _currentGoalPtr = &*itGoal;
        // Check if we are still in this goal
        if (pManageGoal(*itGoal, itGoalsGroup->first))
        {
          if (pLookForAnActionOutputInfosPtr != nullptr)
            pLookForAnActionOutputInfosPtr->setType(PlannerStepType::IN_PROGRESS);
          return res;
        }
        isCurrentlyActiveGoal = false;

        if (pLookForAnActionOutputInfosPtr != nullptr)
          pLookForAnActionOutputInfosPtr->setType(PlannerStepType::FINISEHD_ON_FAILURE);

        if (pLookForAnActionOutputInfosPtr != nullptr)
          pLookForAnActionOutputInfosPtr->notifyNotSatisfiedGoal(*itGoal);
      }
      else
      {
        if (pLookForAnActionOutputInfosPtr != nullptr && pLookForAnActionOutputInfosPtr->getType() != PlannerStepType::FINISEHD_ON_FAILURE)
          pLookForAnActionOutputInfosPtr->setType(PlannerStepType::FINISHED_ON_SUCCESS);

        if (pLookForAnActionOutputInfosPtr != nullptr)
          pLookForAnActionOutputInfosPtr->notifySatisfiedGoal(*itGoal);
        if (_currentGoalPtr == &*itGoal)
          isCurrentlyActiveGoal = false;
      }


      if (itGoal->isPersistent())
      {
        itGoal->setInactiveSinceIfNotAlreadySet(pNow);
        ++itGoal;
      }
      else
      {
        itGoal = itGoalsGroup->second.erase(itGoal);
        res = true;
      }
    }

    if (itGoalsGroup->second.empty())
      itGoalsGroup = _goals.erase(itGoalsGroup);
  }

  // If a goal was activated, but it is not anymore, and we did not find any other active goal
  // then we do not consider anymore the previously activited goal as an activated goal
  if (!isCurrentlyActiveGoal)
    _currentGoalPtr = nullptr;
  return res;
}


int GoalStack::_getCurrentPriority(const WorldState& pWorldState,
                                   const SetOfEntities& pConstants,
                                   const SetOfEntities& pObjects) const
{
  for (auto itGoalsGroup = _goals.end(); itGoalsGroup != _goals.begin(); )
  {
    --itGoalsGroup;
    for (const auto& currGoal : itGoalsGroup->second)
    {
      if (!currGoal.isPersistent())
        return itGoalsGroup->first;

      if (!pWorldState.isGoalSatisfied(currGoal, pConstants, pObjects))
        return itGoalsGroup->first;
    }
  }
  return 0;
}




std::string GoalStack::getCurrentGoalStr() const
{
  for (auto itGoalGroup = _goals.rbegin(); itGoalGroup != _goals.rend(); ++itGoalGroup)
    if (!itGoalGroup->second.empty())
      return itGoalGroup->second.front().toStr();
  return "";
}

const Goal* GoalStack::getCurrentGoalPtr() const
{
  for (auto itGoalGroup = _goals.rbegin(); itGoalGroup != _goals.rend(); ++itGoalGroup)
    if (!itGoalGroup->second.empty())
      return &itGoalGroup->second.front();
  return nullptr;
}



void GoalStack::iterateOnGoalsAndRemoveNonPersistent(
    const std::function<bool(Goal&, int)>& pManageGoal,
    const WorldState& pWorldState,
    const SetOfEntities& pConstants,
    const SetOfEntities& pObjects,
    const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
    LookForAnActionOutputInfos* pLookForAnActionOutputInfosPtr)
{
  if (_iterateOnGoalsAndRemoveNonPersistent(pManageGoal, pWorldState, pConstants, pObjects,
                                            pNow, pLookForAnActionOutputInfosPtr))
    onGoalsChanged(_goals);
}


void GoalStack::setGoals(const std::map<int, std::vector<Goal>>& pGoals,
                         const WorldState& pWorldState,
                         const SetOfEntities& pConstants,
                         const SetOfEntities& pObjects,
                         const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow)
{
  if (_goals != pGoals)
  {
    _currentGoalPtr = nullptr;
    {
      _goals = pGoals;
      onGoalsChanged(_goals);
    }
    if (_removeNoStackableGoals(pWorldState, pConstants, pObjects, pNow))
      onGoalsChanged(_goals);
  }
}

void GoalStack::setGoals(const std::vector<Goal>& pGoals,
                         const WorldState& pWorldState,
                         const SetOfEntities& pConstants,
                         const SetOfEntities& pObjects,
                         const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                         int pPriority)
{
  setGoals(std::map<int, std::vector<Goal>>{{pPriority, pGoals}}, pWorldState, pConstants, pObjects, pNow);
}

bool GoalStack::addGoals(const std::map<int, std::vector<Goal>>& pGoals,
                         const WorldState& pWorldState,
                         const SetOfEntities& pConstants,
                         const SetOfEntities& pObjects,
                         const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow)
{
  if (_addGoals(pGoals, pWorldState, pConstants, pObjects, pNow))
  {
    onGoalsChanged(_goals);
    return true;
  }
  return false;
}


bool GoalStack::addGoals(const std::vector<Goal>& pGoals,
                         const WorldState& pWorldState,
                         const SetOfEntities& pConstants,
                         const SetOfEntities& pObjects,
                         const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                         int pPriority)
{
  return addGoals(std::map<int, std::vector<Goal>>{{pPriority, pGoals}}, pWorldState, pConstants, pObjects, pNow);
}


bool GoalStack::_addGoals(const std::map<int, std::vector<Goal>>& pGoals,
                          const WorldState& pWorldState,
                          const SetOfEntities& pConstants,
                          const SetOfEntities& pObjects,
                          const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow)
{
  if (pGoals.empty())
    return false;
  // Sub goalChanged object to notify about removed goals
  bool goalChanged = false;
  for (auto& currGoals : pGoals)
  {
    auto& existingGoals = _goals[currGoals.first];
    existingGoals.insert(existingGoals.begin(), currGoals.second.begin(), currGoals.second.end());
    goalChanged = true;
  }
  if (goalChanged)
    onGoalsChanged(_goals);
  return _removeNoStackableGoals(pWorldState, pConstants, pObjects, pNow) || goalChanged;
}


void GoalStack::pushFrontGoal(const Goal& pGoal,
                              const WorldState& pWorldState,
                              const SetOfEntities& pConstants,
                              const SetOfEntities& pObjects,
                              const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                              int pPriority)
{
  {
    auto& existingGoals = _goals[pPriority];
    existingGoals.insert(existingGoals.begin(), pGoal);
    onGoalsChanged(_goals);
  }
  if (_removeNoStackableGoals(pWorldState, pConstants, pObjects, pNow))
    onGoalsChanged(_goals);
}

void GoalStack::pushBackGoal(const Goal& pGoal,
                             const WorldState& pWorldState,
                             const SetOfEntities& pConstants,
                             const SetOfEntities& pObjects,
                             const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                             int pPriority)
{
  {
    auto& existingGoals = _goals[pPriority];
    existingGoals.push_back(pGoal);
    onGoalsChanged(_goals);
  }
  if (_removeNoStackableGoals(pWorldState, pConstants, pObjects, pNow))
    onGoalsChanged(_goals);
}


void GoalStack::changeGoalPriority(const std::string& pGoalStr,
                                   int pPriority,
                                   bool pPushFrontOrBottomInCaseOfConflictWithAnotherGoal,
                                   const WorldState& pWorldState,
                                   const SetOfEntities& pConstants,
                                   const SetOfEntities& pObjects,
                                   const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow)
{
  std::unique_ptr<Goal> goalToMove;
  bool goalChanged = false;
  for (auto itGroup = _goals.begin(); itGroup != _goals.end(); )
  {
    for (auto it = itGroup->second.begin(); it != itGroup->second.end(); )
    {
      if (it->toStr() == pGoalStr)
      {
        goalToMove = std::make_unique<Goal>(std::move(*it));
        itGroup->second.erase(it);
        goalChanged = true;
        break;
      }
      ++it;
    }

    if (itGroup->second.empty())
    {
      itGroup = _goals.erase(itGroup);
      goalChanged = true;
    }
    else
    {
      ++itGroup;
    }

    if (goalToMove)
    {
      auto& goalsForThePriority = _goals[pPriority];
      if (pPushFrontOrBottomInCaseOfConflictWithAnotherGoal)
        goalsForThePriority.insert(goalsForThePriority.begin(), *goalToMove);
      else
        goalsForThePriority.push_back(*goalToMove);
      break;
    }
  }
  goalChanged = _removeNoStackableGoals(pWorldState, pConstants, pObjects, pNow) || goalChanged;
  if (goalChanged)
    onGoalsChanged(_goals);
}


void GoalStack::clearGoals(const WorldState& pWorldState,
                           const SetOfEntities& pConstants,
                           const SetOfEntities& pObjects,
                           const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow)
{
  if (_goals.empty())
    return;

  _goals.clear();
  _removeNoStackableGoals(pWorldState, pConstants, pObjects, pNow);
  onGoalsChanged(_goals);
}


bool GoalStack::removeGoals(const std::string& pGoalGroupId,
                            const WorldState& pWorldState,
                            const SetOfEntities& pConstants,
                            const SetOfEntities& pObjects,
                            const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow)
{
  bool goalChanged = false;
  for (auto itGroup = _goals.begin(); itGroup != _goals.end(); )
  {
    for (auto it = itGroup->second.begin(); it != itGroup->second.end(); )
    {
      if (it->getGoalGroupId() == pGoalGroupId)
      {
        it = itGroup->second.erase(it);
        goalChanged = true;
      }
      else
      {
        ++it;
      }
    }

    if (itGroup->second.empty())
      itGroup = _goals.erase(itGroup);
    else
      ++itGroup;
  }
  if (goalChanged)
  {
    _removeNoStackableGoals(pWorldState, pConstants, pObjects, pNow);
    onGoalsChanged(_goals);
    return true;
  }
  return false;
}


void GoalStack::removeFirstGoalsThatAreAlreadySatisfied(const WorldState& pWorldState,
                                                        const SetOfEntities& pConstants,
                                                        const SetOfEntities& pObjects,
                                                        const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow)
{
  if (_removeFirstGoalsThatAreAlreadySatisfied(pWorldState, pConstants, pObjects, pNow, nullptr))
    onGoalsChanged(_goals);
}



std::map<int, std::vector<Goal>> GoalStack::getNotSatisfiedGoals(const WorldState& pWorldState,
                                                                 const SetOfEntities& pConstants,
                                                                 const SetOfEntities& pObjects) const
{
  std::map<int, std::vector<Goal>> res;
  for (auto& currGoalWithPriority : _goals)
    for (auto& currGoal : currGoalWithPriority.second)
      if (!pWorldState.isGoalSatisfied(currGoal, pConstants, pObjects))
        res[currGoalWithPriority.first].push_back(currGoal);
  return res;
}

void GoalStack::refreshIfNeeded(const Domain& pDomain)
{
  for (auto& currGoalsGroup : _goals)
    for (Goal& currGoal : currGoalsGroup.second)
      currGoal.refreshIfNeeded(pDomain);
}

std::string GoalStack::printGoalsCache() const
{
  std::string res;

  for (const auto& currGoalsGroup : _goals)
  {
    for (const Goal& currGoal : currGoalsGroup.second)
    {
      auto subRes = currGoal.printActionsThatCanSatisfyThisGoal();
      if (subRes != "")
      {
        if (res != "")
          res += "\n\n\n";
        res += "goal: " + currGoal.toStr();
        res += "\n---------------------------\n";
        res += subRes;
      }
    }
  }
  return res;
}


std::set<ActionId> GoalStack::getActionsPredecessors() const
{
  std::set<ActionId> res;
  for (const auto& currGoalsGroup : _goals)
  {
    for (const Goal& currGoal : currGoalsGroup.second)
    {
      auto subRes = currGoal.getActionsPredecessors();
      res.insert(subRes.begin(), subRes.end());
    }
  }
  return res;
}

std::set<ActionId> GoalStack::getEventsPredecessors() const
{
  std::set<ActionId> res;
  for (const auto& currGoalsGroup : _goals)
  {
    for (const Goal& currGoal : currGoalsGroup.second)
    {
      auto subRes = currGoal.getEventsPredecessors();
      res.insert(subRes.begin(), subRes.end());
    }
  }
  return res;
}


void GoalStack::_removeNoStackableGoalsAndNotifyGoalsChanged(
    const WorldState& pWorldState,
    const SetOfEntities& pConstants,
    const SetOfEntities& pObjects,
    const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow)
{
  if (_removeNoStackableGoals(pWorldState, pConstants, pObjects, pNow))
    onGoalsChanged(_goals);
}


bool GoalStack::_removeNoStackableGoals(const WorldState& pWorldState,
                                        const SetOfEntities& pConstants,
                                        const SetOfEntities& pObjects,
                                        const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow)
{
  bool res = false;
  bool firstGoal = true;
  for (auto itGoalsGroup = _goals.end(); itGoalsGroup != _goals.begin(); )
  {
    --itGoalsGroup;
    for (auto itGoal = itGoalsGroup->second.begin(); itGoal != itGoalsGroup->second.end(); )
    {
      if (pWorldState.isGoalSatisfied(*itGoal, pConstants, pObjects))
      {
        ++itGoal;
        continue;
      }

      if (firstGoal)
      {
        firstGoal = false;
        ++itGoal;
        continue;
      }

      if (!itGoal->isInactiveForTooLong(pNow))
      {
        itGoal->setInactiveSinceIfNotAlreadySet(pNow);
        ++itGoal;
      }
      else
      {
        itGoal = itGoalsGroup->second.erase(itGoal);
        res = true;
      }
    }

    if (itGoalsGroup->second.empty())
      itGoalsGroup = _goals.erase(itGoalsGroup);
  }

  return res;
}



} // !ogp
