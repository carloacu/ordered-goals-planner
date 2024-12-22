#include <orderedgoalsplanner/types/setofcallbacks.hpp>
#include <orderedgoalsplanner/util/util.hpp>

namespace ogp
{

CallbackId SetOfCallbacks::add(const ConditionToCallback& pConditionToCallback,
                               const CallbackId& pCallbackId)
{
  auto isIdOkForInsertion = [this](const std::string& pId)
  {
    return _callbacks.count(pId) == 0;
  };
  auto newId = incrementLastNumberUntilAConditionIsSatisfied(pCallbackId, isIdOkForInsertion);

  _callbacks.emplace(newId, pConditionToCallback);

  if (pConditionToCallback.condition)
  {
    pConditionToCallback.condition->forAll(
          [&](const FactOptional& pFactOptional,
          bool pIgnoreValue)
    {
      if (pFactOptional.isFactNegated)
        _reachableCallbackLinks.notConditionToCallbacks.add(pFactOptional.fact, newId, pIgnoreValue);
      else
        _reachableCallbackLinks.conditionToCallbacks.add(pFactOptional.fact, newId, pIgnoreValue);
      return ContinueOrBreak::CONTINUE;
    }
    );
  }
  return newId;
}


void SetOfCallbacks::remove(const CallbackId& pCallbackId)
{
  auto it = _callbacks.find(pCallbackId);
  if (it == _callbacks.end())
    return;
  auto& callbackThatWillBeRemoved = it->second;

  if (callbackThatWillBeRemoved.condition)
  {
    _reachableCallbackLinks.notConditionToCallbacks.erase(pCallbackId);
    _reachableCallbackLinks.conditionToCallbacks.erase(pCallbackId);
  }
  _callbacks.erase(it);
}


} // !ogp
