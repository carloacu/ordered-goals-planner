#ifndef INCLUDE_ORDEREDGOALSPLANNER_TYPES_WORLDSTATE_HPP
#define INCLUDE_ORDEREDGOALSPLANNER_TYPES_WORLDSTATE_HPP

#include <chrono>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <orderedgoalsplanner/types/fact.hpp>
#include <orderedgoalsplanner/types/factstovalue.hpp>
#include <orderedgoalsplanner/types/setoffacts.hpp>
#include <orderedgoalsplanner/util/alias.hpp>
#include <orderedgoalsplanner/util/observableunsafe.hpp>
#include "../util/api.hpp"


namespace ogp
{
struct ConditionToCallback;
struct Domain;
struct Goal;
struct GoalStack;
struct Event;
struct ActionInvocationWithGoal;
struct SetOfCallbacks;
struct SetOfEvents;
struct WorldStateModification;

/**
 * @brief Current state of the world.<br/>
 * It is composed of a set of facts.<br/>
 * It also has accessors to optimize algorithms that will use this world state.
 */
struct ORDEREDGOALSPLANNER_API WorldState
{
  /// Construct a world state.
  WorldState(const SetOfFacts* pFactsPtr = nullptr);
  /// Construct a world state from another world state.
  WorldState(const WorldState& pOther);

  ~WorldState();

  void operator=(const WorldState& pOther);

  bool modifyFactsFromPddl(const std::string& pStr,
                           std::size_t& pPos,
                           GoalStack& pGoalStack,
                           const std::map<SetOfEventsId, SetOfEvents>& pSetOfEvents,
                           const SetOfCallbacks& pCallbacks,
                           const Ontology& pOntology,
                           const SetOfEntities& pEntities,
                           const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                           bool pCanFactsBeRemoved = true);

  /**
   * @brief Notify that an action has been done.
   * @param[in] pParameters Effect parameters.
   * @param[in] pEffect Effect of the done action.
   * @param[out] pGoalChanged Set to true if the goal stack changed.
   * @param[out] pGoalStack Goal stacks that need to be refreshed.<br/>
   * For example the current goal of the stack can be satisfied now and so maybe it should be removed from the goal stack.
   * @param[in] pSetOfEvents events to apply indirect modifications according to the events.
   * @param[in] pNow Current time.
   */
  void applyEffect(const std::map<Parameter, Entity>& pParameters,
                   const std::unique_ptr<WorldStateModification>& pEffect,
                   bool& pGoalChanged,
                   GoalStack& pGoalStack,
                   const std::map<SetOfEventsId, SetOfEvents>& pSetOfEvents,
                   const SetOfCallbacks& pCallbacks,
                   const Ontology& pOntology,
                   const SetOfEntities& pEntities,
                   const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow);


  /// Be notified when facts changed.
  ogpstd::observable::ObservableUnsafe<void (const std::map<Fact, bool>&)> onFactsChanged;
  /// Be notified when punctual facts changed.
  ogpstd::observable::ObservableUnsafe<void (const std::set<Fact>&)> onPunctualFacts;
  /// Be notified when facts are added.
  ogpstd::observable::ObservableUnsafe<void (const std::set<Fact>&)> onFactsAdded;
  /// Be notified when facts are removed.
  ogpstd::observable::ObservableUnsafe<void (const std::set<Fact>&)> onFactsRemoved;

  /**
   * @brief Add a fact.
   * @param[in] pFact Fact to add.
   * @param[out] pGoalStack Goal stacks that need to be refreshed.<br/>
   * For example the current goal of the stack can be satisfied now and so maybe it should be removed from the goal stack.
   * @param[in] pSetOfEvents events to apply indirect modifications according to the events.
   * @param[in] pNow Current time.
   */
  bool addFact(const Fact& pFact,
               GoalStack& pGoalStack,
               const std::map<SetOfEventsId, SetOfEvents>& pSetOfEvents,
               const SetOfCallbacks& pCallbacks,
               const Ontology& pOntology,
               const SetOfEntities& pEntities,
               const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
               bool pCanFactsBeRemoved = true);

  /**
   * @brief Add several facts.
   * @param[in] pFacts Facts to add.
   * @param[out] pGoalStack Goal stacks that need to be refreshed.<br/>
   * For example the current goal of the stack can be satisfied now and so maybe it should be removed from the goal stack.
   * @param[in] pSetOfEvents events to apply indirect modifications according to the events.
   * @param[in] pNow Current time.
   */
  template<typename FACTS>
  bool addFacts(const FACTS& pFacts,
                GoalStack& pGoalStack,
                const std::map<SetOfEventsId, SetOfEvents>& pSetOfEvents,
                const SetOfCallbacks& pCallbacks,
                const Ontology& pOntology,
                const SetOfEntities& pEntities,
                const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                bool pCanFactsBeRemoved = true);

  /// Check if the world has a fact or the negation of the fact.
  bool hasFact(const Fact& pFact) const;

  /**
   * @brief Remove a fact.
   * @param[in] pFact Fact to remove.
   * @param[out] pGoalStack Goal stacks that need to be refreshed.<br/>
   * For example the current goal of the stack can be satisfied now and so maybe it should be removed from the goal stack.
   * @param[in] pSetOfEvents events to apply indirect modifications according to the events.
   * @param[in] pNow Current time.
  */
  bool removeFact(const Fact& pFact,
                  GoalStack& pGoalStack,
                  const std::map<SetOfEventsId, SetOfEvents>& pSetOfEvents,
                  const SetOfCallbacks& pCallbacks,
                  const Ontology& pOntology,
                  const SetOfEntities& pEntities,
                  const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow);

  /**
   * @brief Remove several facts.
   * @param[in] pFacts Facts to remove.
   * @param[out] pGoalStack Goal stacks that need to be refreshed.<br/>
   * For example the current goal of the stack can be satisfied now and so maybe it should be removed from the goal stack.
   * @param[in] pSetOfEvents events to apply indirect modifications according to the events.
   * @param[in] pNow Current time.
   */
  template<typename FACTS>
  bool removeFacts(const FACTS& pFacts,
                   GoalStack& pGoalStack,
                   const std::map<SetOfEventsId, SetOfEvents>& pSetOfEvents,
                   const SetOfCallbacks& pCallbacks,
                   const Ontology& pOntology,
                   const SetOfEntities& pEntities,
                   const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow);

  /**
   * @brief Modify the world state.
   * @param[in] pWsModifPtr Modification to do.
   * @param[out] pGoalStack Goal stacks that need to be refreshed.<br/>
   * For example the current goal of the stack can be satisfied now and so maybe it should be removed from the goal stack.
   * @param[in] pSetOfEvents events to apply indirect modifications according to the events.
   * @param[in] pNow Current time.
   */
  bool modify(const WorldStateModification* pWsModifPtr,
              GoalStack& pGoalStack,
              const std::map<SetOfEventsId, SetOfEvents>& pSetOfEvents,
              const SetOfCallbacks& pCallbacks,
              const Ontology& pOntology,
              const SetOfEntities& pEntities,
              const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
              bool pCanFactsBeRemoved = true);

  /**
   * @brief Set the facts of the world.
   * @param[in] pFacts New set of facts for the world.
   * @param[out] pGoalStack Goal stacks that need to be refreshed.<br/>
   * For example the current goal of the stack can be satisfied now and so maybe it should be removed from the goal stack.
   * @param[in] pSetOfEvents events to apply indirect modifications according to the events.
   * @param[in] pNow Current time.
   */
  void setFacts(const std::set<Fact>& pFacts,
                GoalStack& pGoalStack,
                const std::map<SetOfEventsId, SetOfEvents>& pSetOfEvents,
                const SetOfCallbacks& pCallbacks,
                const Ontology& pOntology,
                const SetOfEntities& pEntities,
                const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow);


  /// Facts of the world.
  const std::map<Fact, bool>& facts() const { return _factsMapping.facts(); }
  /// Fact names to facts in the world.
  const SetOfFacts& factsMapping() const { return _factsMapping; }


  /**
   * @brief Is an optional fact satisfied.
   * @param[in] pFactOptional Input optional fact.
   * @return True if the optional fact is satisfied, false otherwise.
   */
  bool isOptionalFactSatisfied(const FactOptional& pFactOptional) const;


  /**
   * @brief Is an optional fact satisfied in a specific context.
   * @param[in] pFactOptional Input optional fact.
   * @param[in] pPunctualFacts Punctual fact of the context.
   * @param[in] pRemovedFacts Facts currently removed in the context.
   * @param[in, out] pParametersToPossibleArgumentsPtr Map of parameters to possible values to extract.
   * @param[in, out] pParametersToModifyInPlacePtr Another map of parameters to possible values to extract.
   * @param[out] pCanBecomeTruePtr If this optional fact can become true according to the facts that can become true.
   * @return True if the optional fact is satisfied, false otherwise.
   */
  bool isOptionalFactSatisfiedInASpecificContext(const FactOptional& pFactOptional,
                                                 const std::set<Fact>& pPunctualFacts,
                                                 const std::set<Fact>& pRemovedFacts,
                                                 bool pCheckAllPossibilities,
                                                 std::map<Parameter, std::set<Entity>>* pParametersToPossibleArgumentsPtr,
                                                 std::map<Parameter, std::set<Entity>>* pParametersToModifyInPlacePtr,
                                                 bool* pCanBecomeTruePtr) const;

  /**
   * @brief Check if a goal is satisfied.<br/>
   * A goal is satisfied if his internal objective condition is satisfied and if the goal is enabled.
   * @param[in] pGoal Goal to check.
   * @return True if the goal is satisfied.
   */
  bool isGoalSatisfied(const Goal& pGoal,
                       const SetOfEntities& pConstants,
                       const SetOfEntities& pObjects) const;

  /**
   * @brief Iterate over all matching facts without fluent consideration.
   * @param[in] pCallback Callback of each matching facts. If it returns true we break the iteration.
   * @param[in] pFact Fact to consider.
   * @param[in] pParametersToConsiderAsAnyValue Parameters to consider as "any value" if there possible values (set of string) is empty.
   * @param[in] pParametersToConsiderAsAnyValuePtr Other parameters to consider as "any value" if there possible values (set of string) is empty.
   */
  void iterateOnMatchingFactsWithoutFluentConsideration(const std::function<bool (const Fact&)>& pCallback,
                                                        const Fact& pFact,
                                                        const std::map<Parameter, std::set<Entity>>& pParametersToConsiderAsAnyValue,
                                                        const std::map<Parameter, std::set<Entity>>* pParametersToConsiderAsAnyValuePtr = nullptr) const;

  /**
   * @brief Iterate over all matching facts.
   * @param[in] pCallback Callback of each matching facts. If it returns true we break the iteration.
   * @param[in] pFact Fact to consider.
   * @param[in] pParametersToConsiderAsAnyValue Parameters to consider as "any value" if there possible values (set of string) is empty.
   * @param[in] pParametersToConsiderAsAnyValuePtr Other parameters to consider as "any value" if there possible values (set of string) is empty.
   */
  void iterateOnMatchingFacts(const std::function<bool (const Fact&)>& pValueCallback,
                              const Fact& pFact,
                              const std::map<Parameter, std::set<Entity>>& pParametersToConsiderAsAnyValue,
                              const std::map<Parameter, std::set<Entity>>* pParametersToConsiderAsAnyValuePtr = nullptr) const;



private:
  /// Facts of the world state.
  SetOfFacts _factsMapping;

  /// Stored what changed.
  struct WhatChanged
  {
    /// Punctual facts that are pinged.
    std::set<Fact> punctualFacts;
    /// Facts that we added in the world.
    std::set<Fact> addedFacts;
    /// Facts that we removed in the world.
    std::set<Fact> removedFacts;

    /// Check if something changed.
    bool somethingChanged() const { return !punctualFacts.empty() || !addedFacts.empty() || !removedFacts.empty(); }
    /// Has some facts to add or to remove.
    bool hasFactsToModifyInTheWorldForSure() const { return !addedFacts.empty() || !removedFacts.empty(); }
  };


  /**
   * @brief Add facts without event deduction and raising a notification.
   * @param[out] pWhatChanged Get what changed.
   * @param[in] pFacts Facts to add.
   * @param[out] pGoalStack Goal stacks that need to be refreshed.<br/>
   * For example the current goal of the stack can be satisfied now and so maybe it should be removed from the goal stack.
   * @param[in] pSetOfEvents events to apply indirect modifications according to the events.
   * @param[in] pNow Current time.
   * @return True if some facts were added, false otherwise.
   */
  template<typename FACTS>
  void _addFacts(WhatChanged& pWhatChanged,
                 const FACTS& pFacts,
                 GoalStack& pGoalStack,
                 const std::map<SetOfEventsId, SetOfEvents>& pSetOfEvents,
                 const SetOfCallbacks& pCallbacks,
                 const Ontology& pOntology,
                 const SetOfEntities& pEntities,
                 const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                 bool pCanFactsBeRemoved);

  void _addAFact(WhatChanged& pWhatChanged,
                 const Fact& pFact,
                 GoalStack& pGoalStack,
                 const std::map<SetOfEventsId, SetOfEvents>& pSetOfEvents,
                 const SetOfCallbacks& pCallbacks,
                 const Ontology& pOntology,
                 const SetOfEntities& pEntities,
                 const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                 bool pCanFactsBeRemoved);

  /**
   * @brief Remove facts without event deduction and raising a notification.
   * @param[out] pWhatChanged Get what changed.
   * @param[in] pFacts Facts to remove.
   * @return True if some facts were removed, false otherwise.
   */
  template<typename FACTS>
  void _removeFacts(WhatChanged& pWhatChanged,
                    const FACTS& pFacts);

  void _removeAFact(WhatChanged& pWhatChanged,
                    const Fact& pFact);

  /**
   * @brief Modify the world state without event deduction and raising a notification.
   * @param[out] pWhatChanged Get what changed.
   * @param[in] pWsModifPtr Modification to do.
   * @param[out] pGoalStack Goal stacks that need to be refreshed.<br/>
   * For example the current goal of the stack can be satisfied now and so maybe it should be removed from the goal stack.
   * @param[in] pSetOfEvents events to apply indirect modifications according to the events.
   * @param[in] pNow Current time.
   */
  void _modify(WhatChanged& pWhatChanged,
               const WorldStateModification* pWsModifPtr,
               GoalStack& pGoalStack,
               const std::map<SetOfEventsId, SetOfEvents>& pSetOfEvents,
               const SetOfCallbacks& pCallbacks,
               const Ontology& pOntology,
               const SetOfEntities& pEntities,
               const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
               bool pCanFactsBeRemoved);

  /**
   * @brief Try to apply some events according to what changed in the world state.
   * @param[in, out] pEventsAlreadyApplied Cache of events that we already considered.
   * @param[in, out] pWhatChanged What changed in the world state.
   * @param[out] pGoalChanged Set to true if the goal stack changed.
   * @param[in] pEventIds events to consider.
   * @param[in] pEvents All the events.
   * @param[in] pNow Current time.
   * @return True if at least one event was applied.
   */
  bool _tryToApplyEvent(std::set<EventId>& pEventsAlreadyApplied,
                        WhatChanged& pWhatChanged,
                        bool& pGoalChanged,
                        GoalStack& pGoalStack,
                        const FactsToValue::ConstMapOfFactIterator& pEventIds,
                        const std::map<EventId, Event>& pEvents,
                        const std::map<SetOfEventsId, SetOfEvents>& pSetOfEvents,
                        const SetOfCallbacks& pCallbacks,
                        const Ontology& pOntology,
                        const SetOfEntities& pEntities,
                        const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow);

  void _tryToCallCallbacks(std::set<CallbackId>& pCallbackAlreadyCalled,
                           const WhatChanged& pWhatChanged,
                           const SetOfEntities& pConstants,
                           const SetOfEntities& pObjects,
                           const FactsToValue::ConstMapOfFactIterator& pCallbackIds,
                           const std::map<CallbackId, ConditionToCallback>& pCallbacks);

  /**
   * @brief Do events and raise the observables if some facts or goals changed.
   * @param[in, out] pWhatChanged Get what changed.
   * @param[out] pGoalChanged Set to true if the goal stack changed.
   * @param[out] pGoalStack Goal stacks that need to be refreshed.<br/>
   * For example the current goal of the stack can be satisfied now and so maybe it should be removed from the goal stack.
   * @param[in] pSetOfEvents events to apply indirect modifications according to the events.
   * @param[in] pNow Current time.
   */
  void _notifyWhatChanged(WhatChanged& pWhatChanged,
                          bool& pGoalChanged,
                          GoalStack& pGoalStack,
                          const std::map<SetOfEventsId, SetOfEvents>& pSetOfEvents,
                          const SetOfCallbacks& pCallbacks,
                          const Ontology& pOntology,
                          const SetOfEntities& pEntities,
                          const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow);


  friend struct ConditionNode;
};

} // !ogp


#endif // INCLUDE_ORDEREDGOALSPLANNER_TYPES_WORLDSTATE_HPP
