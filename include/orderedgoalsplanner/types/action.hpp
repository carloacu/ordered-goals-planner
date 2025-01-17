#ifndef INCLUDE_ORDEREDGOALSPLANNER_TYPES_ACTION_HPP
#define INCLUDE_ORDEREDGOALSPLANNER_TYPES_ACTION_HPP

#include <vector>
#include "../util/api.hpp"
#include <orderedgoalsplanner/types/condition.hpp>
#include <orderedgoalsplanner/types/problemmodification.hpp>


namespace ogp
{
struct SetOfDerivedPredicates;


/// Thing that the bot can do
struct ORDEREDGOALSPLANNER_API Action
{
  /**
   * @brief Construct an Action.
   * @param pPreconditions[in] Condition that should be satisfied in the world to be able to do this action.
   * @param pEffect[in] How the problem will change when this action will finish.
   * @param pPreferInContext[in] More this condition contains parts that are satisfied in the world more the priority of this action will be.<br/>
   * More this condition contains parts that are not satisfied in the world less the priority of this action will be.
   */
  Action(std::unique_ptr<Condition> pPrecondition,
         const ProblemModification& pEffect,
         std::unique_ptr<Condition> pPreferInContext = {})
    : parameters(),
      precondition(pPrecondition ? std::move(pPrecondition) : std::unique_ptr<Condition>()),
      overAllCondition(),
      preferInContext(pPreferInContext ? std::move(pPreferInContext) : std::unique_ptr<Condition>()),
      effect(pEffect),
      highImportanceOfNotRepeatingIt(false),
      canThisActionBeUsedByThePlanner(true),
      actionsSuccessionsWithoutInterestCache(),
      actionsPredecessorsCache(),
      eventsPredecessorsCache(),
      duration(1)
  {
  }

  /// Move constructor of an action.
  Action(std::unique_ptr<Condition>&& pPrecondition,
         ProblemModification&& pEffect,
         std::unique_ptr<Condition>&& pPreferInContext)
    : parameters(),
      precondition(pPrecondition ? std::move(pPrecondition) : std::unique_ptr<Condition>()),
      overAllCondition(),
      preferInContext(pPreferInContext ? std::move(pPreferInContext) : std::unique_ptr<Condition>()),
      effect(std::move(pEffect)),
      highImportanceOfNotRepeatingIt(false),
      canThisActionBeUsedByThePlanner(true),
      actionsSuccessionsWithoutInterestCache(),
      actionsPredecessorsCache(),
      eventsPredecessorsCache(),
      duration(1)
  {
  }

  /// Copy constructor.
  Action(const Action& pAction)
    : parameters(pAction.parameters),
      precondition(pAction.precondition ? pAction.precondition->clone() : std::unique_ptr<Condition>()),
      overAllCondition(pAction.overAllCondition ? pAction.overAllCondition->clone() : std::unique_ptr<Condition>()),
      preferInContext(pAction.preferInContext ? pAction.preferInContext->clone() : std::unique_ptr<Condition>()),
      effect(pAction.effect),
      highImportanceOfNotRepeatingIt(pAction.highImportanceOfNotRepeatingIt),
      canThisActionBeUsedByThePlanner(pAction.canThisActionBeUsedByThePlanner),
      actionsSuccessionsWithoutInterestCache(pAction.actionsSuccessionsWithoutInterestCache),
      actionsPredecessorsCache(pAction.actionsPredecessorsCache),
      eventsPredecessorsCache(pAction.eventsPredecessorsCache),
      duration(pAction.duration)
  {
  }

  /// Copy operator.
  void operator=(const Action& pAction)
  {
    parameters = pAction.parameters;
    precondition = pAction.precondition ? pAction.precondition->clone() : std::unique_ptr<Condition>();
    overAllCondition = pAction.overAllCondition ? pAction.overAllCondition->clone() : std::unique_ptr<Condition>();
    preferInContext = pAction.preferInContext ? pAction.preferInContext->clone() : std::unique_ptr<Condition>();
    effect = pAction.effect;
    highImportanceOfNotRepeatingIt = pAction.highImportanceOfNotRepeatingIt;
    canThisActionBeUsedByThePlanner = pAction.canThisActionBeUsedByThePlanner;
    actionsSuccessionsWithoutInterestCache = pAction.actionsSuccessionsWithoutInterestCache;
    actionsPredecessorsCache = pAction.actionsPredecessorsCache;
    eventsPredecessorsCache = pAction.eventsPredecessorsCache;
    duration = pAction.duration;
  }

  /// Check equality with another action.
  bool operator==(const Action& pOther) const;
  bool operator!=(const Action& pOther) const { return !operator==(pOther); }

  Action clone(const SetOfDerivedPredicates& pDerivedPredicates) const;

  /// Check if this action contains a fact or the negation of the fact.
  bool hasFact(const ogp::Fact& pFact) const;

  /**
   * @brief Replace a fact by another inside this action.
   * @param pOldFact[in] Current fact to replace.
   * @param pNewFact[in] New fact to set.
   */
  void replaceArgument(const Entity& pOld,
                       const Entity& pNew);

  void updateSuccessionCache(const Domain& pDomain,
                             const ActionId& pIdOfThisAction,
                             const std::set<FactOptional>& pFactsFromCondition);
  void removePossibleSuccessionCache(const ActionId& pActionIdToRemove);
  std::string printSuccessionCache() const;

  // TODO: remove that function?
  void throwIfNotValid(const SetOfFacts& pSetOfFact);

  /// Print the precondition in string.
  std::string precondition_str() const { return precondition ? precondition->toStr() : ""; }

  /// Print preferInContext in string.
  std::string preferInContext_str() const { return preferInContext ? preferInContext->toStr() : ""; }

  /// Parameters of this action.
  std::vector<Parameter> parameters;
  /// Condition that should be satisfied in the world to be able to do this action.
  std::unique_ptr<Condition> precondition;
  std::unique_ptr<Condition> overAllCondition;
  /// More this condition matches the world higher the priority of this action will be.
  std::unique_ptr<Condition> preferInContext;
  /// Modification to apply to the problem when this action will finish.
  ProblemModification effect;
  /// If it is important to not repeat this action.
  bool highImportanceOfNotRepeatingIt = false;

  bool canThisActionBeUsedByThePlanner;
  std::set<ActionId> actionsSuccessionsWithoutInterestCache;
  std::set<ActionId> actionsPredecessorsCache;
  std::set<FullEventId> eventsPredecessorsCache;

  /// Duration of the action.
  Number duration;

private:
  void _throwIfNotValidForACondition(const std::unique_ptr<Condition>& pPrecondition);
  void _throwIfNotValidForAnWordStateModif(const std::unique_ptr<WorldStateModification>& pWs,
                                           const SetOfFacts& pSetOfFact);
  void _throwIfNotValidForAFact(const Fact& pFact);
};


} // !ogp


#endif // INCLUDE_ORDEREDGOALSPLANNER_TYPES_ACTION_HPP
