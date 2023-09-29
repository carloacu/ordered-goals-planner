#include <contextualplanner/types/goal.hpp>
#include <assert.h>


namespace cp
{
const std::string Goal::persistFunctionName = "persist";
const std::string Goal::implyFunctionName = "imply";
const std::string Goal::oneStepTowardsFunctionName = "oneStepTowards";
namespace
{
const std::string _persistPrefix = Goal::persistFunctionName + "(";
const std::size_t _persistPrefixSize = _persistPrefix.size();
}

Goal::Goal(const std::string& pStr,
           int pMaxTimeToKeepInactive,
           const std::string& pGoalGroupId)
  : _factCondition(),
    _maxTimeToKeepInactive(pMaxTimeToKeepInactive),
    _inactiveSince(),
    _isPersistentIfSkipped(false),
    _oneStepTowards(false),
    _conditionFactPtr(),
    _goalGroupId(pGoalGroupId)
{
  if (pStr.size() > _persistPrefixSize &&
      pStr.compare(0, _persistPrefixSize, _persistPrefix) == 0 &&
      pStr[pStr.size() - 1] == ')')
  {
    _isPersistentIfSkipped = true;
    auto subStr = pStr.substr(_persistPrefixSize, pStr.size() - _persistPrefixSize - 1);
    _factCondition = FactCondition::fromStr(subStr);
  }
  else
  {
    _factCondition = FactCondition::fromStr(pStr);
  }

  assert(_factCondition);
  if (!_factCondition)
    return;
  auto* factPtr = _factCondition->fcFactPtr();
  if (factPtr != nullptr)
  {
    if (factPtr->factOptional.fact.name == oneStepTowardsFunctionName &&
        factPtr->factOptional.fact.arguments.size() == 1 &&
        factPtr->factOptional.fact.value.empty())
    {
      _oneStepTowards = true;
      // Temporary variable factParameters is needed for Android compilation (to not have the same assignee and value)
      auto factFirstParameters = std::move(factPtr->factOptional.fact.arguments.front());
      factPtr->factOptional = std::move(factFirstParameters);
    }

    if (factPtr->factOptional.fact.name == implyFunctionName &&
        factPtr->factOptional.fact.arguments.size() == 2 &&
        factPtr->factOptional.fact.value.empty())
    {
      _conditionFactPtr = std::make_unique<FactOptional>(factPtr->factOptional.fact.arguments[0]);
      // Temporary variable factParameters is needed for Android compilation (to not have the same assignee and value)
      auto factSecondParameters = std::move(factPtr->factOptional.fact.arguments[1]);
      factPtr->factOptional = std::move(factSecondParameters);
    }

    assert(!factPtr->factOptional.fact.name.empty());
  }
}

Goal::Goal(const Goal& pOther,
           const std::map<std::string, std::string>* pParametersPtr,
           const std::string* pGoalGroupIdPtr)
  : _factCondition(pOther._factCondition->clone(pParametersPtr)),
    _maxTimeToKeepInactive(pOther._maxTimeToKeepInactive),
    _inactiveSince(pOther._inactiveSince ? std::make_unique<std::chrono::steady_clock::time_point>(*pOther._inactiveSince) : std::unique_ptr<std::chrono::steady_clock::time_point>()),
    _isPersistentIfSkipped(pOther._isPersistentIfSkipped),
    _oneStepTowards(pOther._oneStepTowards),
    _conditionFactPtr(pOther._conditionFactPtr ? std::make_unique<FactOptional>(*pOther._conditionFactPtr, pParametersPtr) : std::unique_ptr<FactOptional>()),
    _goalGroupId(pGoalGroupIdPtr != nullptr ? *pGoalGroupIdPtr : pOther._goalGroupId)
{
}

void Goal::operator=(const Goal& pOther)
{
  _factCondition = pOther._factCondition->clone();
  _maxTimeToKeepInactive = pOther._maxTimeToKeepInactive;
  if (pOther._inactiveSince)
    _inactiveSince = std::make_unique<std::chrono::steady_clock::time_point>(*pOther._inactiveSince);
  else
    _inactiveSince.reset();
  _isPersistentIfSkipped = pOther._isPersistentIfSkipped;
  _oneStepTowards = pOther._oneStepTowards;
  _conditionFactPtr = pOther._conditionFactPtr ? std::make_unique<FactOptional>(*pOther._conditionFactPtr) : std::unique_ptr<FactOptional>();
  _goalGroupId = pOther._goalGroupId;
}

bool Goal::operator==(const Goal& pOther) const
{
  return *_factCondition == *pOther._factCondition &&
      _maxTimeToKeepInactive == pOther._maxTimeToKeepInactive &&
      _isPersistentIfSkipped == pOther._isPersistentIfSkipped &&
      _oneStepTowards == pOther._oneStepTowards &&
      _goalGroupId == pOther._goalGroupId;
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


bool Goal::isInactiveForTooLong(const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow)
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
  auto res = _factCondition->toStr();
  if (_conditionFactPtr)
    res = implyFunctionName + "(" + _conditionFactPtr->toStr() + ", " + res + ")";
  if (_oneStepTowards)
    res = oneStepTowardsFunctionName + "(" + res + ")";
  if (_isPersistentIfSkipped)
    res = persistFunctionName + "(" + res + ")";
  return res;
}


} // !cp
