#ifndef INCLUDE_ORDEREDGOALSPLANNER_WORLDSTATEMODIFICATION_HPP
#define INCLUDE_ORDEREDGOALSPLANNER_WORLDSTATEMODIFICATION_HPP

#include <functional>
#include <memory>
#include <optional>
#include "factoptional.hpp"
#include "../util/api.hpp"
#include <orderedgoalsplanner/util/alias.hpp>
#include <orderedgoalsplanner/util/continueorbreak.hpp>

namespace ogp
{
struct Domain;
struct WorldState;


struct ORDEREDGOALSPLANNER_API WorldStateModificationContainerId
{
  bool isAction(const ActionId& pActionId) const { return actionIdToExclude && *actionIdToExclude == pActionId; }
  bool isEvent(const SetOfEventsId& pSetOfEventsId,
                   const EventId& pEventId) const { return setOfEventsIdToExclude && *setOfEventsIdToExclude == pSetOfEventsId &&
        eventIdToExclude && *eventIdToExclude == pEventId; }

  std::optional<ActionId> actionIdToExclude;
  std::optional<SetOfEventsId> setOfEventsIdToExclude;
  std::optional<EventId> eventIdToExclude;
};



struct ORDEREDGOALSPLANNER_API Successions
{
  bool empty() const { return actions.empty() && events.empty(); }
  void clear() { actions.clear(); events.clear(); }

  void add(const Successions& pSuccessions);
  void addSuccesionsOptFact(const FactOptional& pFactOptional,
                            const Domain& pDomain,
                            const WorldStateModificationContainerId& pContainerId,
                            const std::set<FactOptional>& pOptionalFactsToIgnore);

  void print(std::string& pRes,
             const FactOptional& pFactOptional) const;

  std::set<ActionId> actions;

  std::map<SetOfEventsId, std::set<EventId>> events;
};



/// Modification of a world state represented in a tree structure to apply to a world state.
struct ORDEREDGOALSPLANNER_API WorldStateModification
{
  /**
   * @brief Create a world state modification by concatenating two existing world state modifications.
   * @param[in] pWsModif1 First world state modification to concatenate.
   * @param[in] pWsModif2 Second world state modification to concatenate.
   * @return New world state modification created.
   */
  static std::unique_ptr<WorldStateModification> createByConcatenation(const WorldStateModification& pWsModif1,
                                                                       const WorldStateModification& pWsModif2);

  /**
   * @brief Convert the world state modification to a string.
   * @return World state modification converted to a string.
   */
  virtual std::string toStr(bool pPrintAnyFluent = true) const = 0;

  /// Check if this object contains a fact or the negation of the fact.
  virtual bool hasFact(const ogp::Fact& pFact) const = 0;

  /// Check if this object contains an optional fact.
  virtual bool hasFactOptional(const ogp::FactOptional& FactOptional) const = 0;

  /**
   * @brief Replace a fact or the negation of the fact by another fact.
   * @param[in] pOldFact Existing fact to replace.
   * @param[in] pNewFact New fact to set instead.
   */
  virtual void replaceArgument(const Entity& pOld,
                               const Entity& pNew) = 0;

  /**
   * @brief Iterate over all the optional facts with fact value resolution according to the world state.
   * @param[in] pFactCallback Callback called for each optional fact of this object.
   * @param[in] pSetOfFact Facts to use to extract value of the facts.
   */
  virtual void forAll(const std::function<void (const FactOptional&)>& pFactCallback,
                      const SetOfFacts& pSetOfFact) const = 0;

  virtual ContinueOrBreak forAllThatCanBeModified(const std::function<ContinueOrBreak (const FactOptional&)>& pFactCallback) const = 0;

  /**
   * @brief Iterate over all the optional facts until one can satisfy the objective.
   * @param[in] pFactCallback Callback called for each optional fact of this object to check if it can satisfy the objective.
   * @param[in, out] pParameters Parameters of the holding action.
   * @param[in] pWorldState World state use to extract value of the facts.
   * @param[in] pFromDeductionId Identifier of the deduction holding the world state modification.
   */
  virtual bool canSatisfyObjective(const std::function<bool (const FactOptional&, std::map<Parameter, std::set<Entity>>*, const std::function<bool (const std::map<Parameter, std::set<Entity>>&)>&)>& pFactCallback,
                                   std::map<Parameter, std::set<Entity>>& pParameters,
                                   const WorldState& pWorldState,
                                   const std::string& pFromDeductionId) const = 0;

  virtual bool iterateOnSuccessions(const std::function<bool (const Successions&, const FactOptional&, std::map<Parameter, std::set<Entity>>*, const std::function<bool (const std::map<Parameter, std::set<Entity>>&)>&)>& pCallback,
                                    std::map<Parameter, std::set<Entity>>& pParameters,
                                    const WorldState& pWorldState,
                                    bool pCanSatisfyThisGoal,
                                    const std::string& pFromDeductionId) const = 0;

  virtual void updateSuccesions(const Domain& pDomain,
                                const WorldStateModificationContainerId& pContainerId,
                                const std::set<FactOptional>& pOptionalFactsToIgnore) = 0;
  virtual void removePossibleSuccession(const ActionId& pActionIdToRemove) = 0;
  virtual void getSuccesions(Successions& pSuccessions) const = 0;
  virtual void printSuccesions(std::string& pRes) const = 0;

  /// Equality operators.
  virtual bool operator==(const WorldStateModification& pOther) const = 0;
  virtual bool operator!=(const WorldStateModification& pOther) const { return !operator==(pOther); }

  /**
   * @brief Convert this world state modification to a value.
   * @param[in] pWorldState World state use to extract value of the facts.
   * @return The world state modification converted to a string value.
   */
  virtual std::optional<Entity> getFluent(const SetOfFacts& pSetOfFact) const = 0;

  /// Convert this world state modification to an optional fact if possible.
  virtual const FactOptional* getOptionalFact() const = 0;

  /**
   * @brief Create a copy of this object with arguments filling (or not if pParametersToArgumentPtr is nullptr).
   * @param pParametersToArgumentPtr Parameters to replace by their argument in the new object to create.
   * @return A copy of this object with arguments filling.
   */
  virtual std::unique_ptr<WorldStateModification> clone(const std::map<Parameter, Entity>* pParametersToArgumentPtr) const = 0;

  virtual bool hasAContradictionWith(const std::set<FactOptional>& pFactsOpt,
                                     std::list<Parameter>* pParametersPtr = nullptr) const = 0;
};


} // !ogp


#endif // INCLUDE_ORDEREDGOALSPLANNER_WORLDSTATEMODIFICATION_HPP
