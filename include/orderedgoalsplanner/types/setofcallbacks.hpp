#ifndef INCLUDE_ORDEREDGOALSPLANNER_TYPES_SETOFCALLBACKS_HPP
#define INCLUDE_ORDEREDGOALSPLANNER_TYPES_SETOFCALLBACKS_HPP

#include <map>
#include "../util/api.hpp"
#include <orderedgoalsplanner/types/conditiontocallback.hpp>
#include <orderedgoalsplanner/types/factoptionalstoid.hpp>
#include <orderedgoalsplanner/util/alias.hpp>

namespace ogp
{


/// Container of a set of conditions to callback.
struct ORDEREDGOALSPLANNER_API SetOfCallbacks
{
  SetOfCallbacks(const std::map<CallbackId, ConditionToCallback>& pCallbacks = {});



  bool empty() const { return _callbacks.empty(); }
  const std::map<CallbackId, ConditionToCallback>& callbacks() const { return _callbacks; }
  std::map<CallbackId, ConditionToCallback>& callbacks() { return _callbacks; }
  const FactOptionalsToId& conditionsToIds() const { return _conditionsToIds; }


private:
  std::map<CallbackId, ConditionToCallback> _callbacks{};
  FactOptionalsToId _conditionsToIds;
};



/// Container of a set of conditions to callback, with this possibility to add or remove callbacks.
struct ORDEREDGOALSPLANNER_API MutableSetOfCallbacks
{
  MutableSetOfCallbacks() = default;

  CallbackId add(const ConditionToCallback& pConditionToCallback,
                 const CallbackId& pCallbackId = "callback");

  void remove(const CallbackId& pCallbackId);


  bool empty() const { return _callbacks.empty(); }
  const std::map<CallbackId, ConditionToCallback>& callbacks() const { return _callbacks; }
  std::map<CallbackId, ConditionToCallback>& callbacks() { return _callbacks; }


private:
  std::map<CallbackId, ConditionToCallback> _callbacks{};
  std::optional<SetOfCallbacks> _setOfCallbacks{};
};


} // !ogp


#endif // INCLUDE_ORDEREDGOALSPLANNER_TYPES_SETOFCALLBACKS_HPP
