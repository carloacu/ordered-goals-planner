#include <orderedgoalsplanner/types/setofcallbacks.hpp>
#include <orderedgoalsplanner/util/util.hpp>

namespace ogp
{
namespace
{
void _addLinks(FactOptionalsToId& pLinks,
               const ConditionToCallback& pConditionToCallback,
               const CallbackId& pCallbackId)
{
  if (pConditionToCallback.condition)
  {
    pLinks.add(*pConditionToCallback.condition, pCallbackId);
    /*
    pConditionToCallback.condition->forAll(
          [&](const FactOptional& pFactOptional,
          bool pIgnoreValue)
    {
      if (pFactOptional.isFactNegated)
        pLinks.add(pFactOptional.fact, pCallbackId, pIgnoreValue);
      else
        pLinks.conditionToCallbacks.add(pFactOptional.fact, pCallbackId, pIgnoreValue);
      return ContinueOrBreak::CONTINUE;
    }
    );
    */
  }
}

}



CallbackId MutableSetOfCallbacks::add(const ConditionToCallback& pConditionToCallback,
                                      const CallbackId& pCallbackId)
{
  _setOfCallbacks.reset();

  auto isIdOkForInsertion = [this](const std::string& pId)
  {
    return _callbacks.count(pId) == 0;
  };
  auto newId = incrementLastNumberUntilAConditionIsSatisfied(pCallbackId, isIdOkForInsertion);
  _callbacks.try_emplace(newId, pConditionToCallback);
  return newId;
}


void MutableSetOfCallbacks::remove(const CallbackId& pCallbackId)
{
  auto it = _callbacks.find(pCallbackId);
  if (it != _callbacks.end())
  {
    _callbacks.erase(it);
    _setOfCallbacks.reset();
  }
}


SetOfCallbacks::SetOfCallbacks(const std::map<CallbackId, ConditionToCallback>& pCallbacks)
  : _callbacks(pCallbacks),
    _conditionsToIds()
{
  for (const auto& currCallback : _callbacks)
    _addLinks(_conditionsToIds, currCallback.second, currCallback.first);
}


} // !ogp
