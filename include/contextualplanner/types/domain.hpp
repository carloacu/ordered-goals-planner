#ifndef INCLUDE_CONTEXTUALPLANNER_TYPES_DOMAIN_HPP
#define INCLUDE_CONTEXTUALPLANNER_TYPES_DOMAIN_HPP

#include <map>
#include <set>
#include "../util/api.hpp"
#include <contextualplanner/util/alias.hpp>
#include <contextualplanner/types/action.hpp>


namespace cp
{

/// Set of all the actions that the bot can do.
struct CONTEXTUALPLANNER_API Domain
{
  /**
   * @brief Construct a domain.
   * @param[in] pActions Map of action identifiers to action.
   */
  Domain(const std::map<ActionId, Action>& pActions);

  /**
   * @brief Add an action.
   * @param pActionId Identifier of the action to add.
   * @param pAction Action to add.
   *
   * If the action identifier is already user in the domain, the addition will not be done.
   */
  void addAction(const ActionId& pActionId,
                 const Action& pAction);

  /**
   * @brief Remove an action.
   * @param pActionId Identifier of the action to remove.
   *
   * If the action identifier is not found in the domain, this function will have no effect.
   * No exception will be raised.
   */
  void removeAction(ActionId pActionId);

  /// All action identifiers to action of this domain.
  const std::map<ActionId, Action>& actions() const { return _actions; }
  /// All preconditions to action idntifiers of this domain.
  const std::map<std::string, std::set<ActionId>>& preconditionToActions() const { return _preconditionToActions; }
  /// All preconditions in expression to action idntifiers of this domain.
  const std::map<std::string, std::set<ActionId>>& preconditionToActionsExps() const { return _preconditionToActionsExps; }
  /// All preconditions negationed to action idntifiers of this domain.
  const std::map<std::string, std::set<ActionId>>& notPreconditionToActions() const { return _notPreconditionToActions; }
  /// All action identifiers that does not have any precondition in this domain.
  const std::set<ActionId>& actionsWithoutPrecondition() const { return _actionsWithoutPrecondition; }


private:
  /// Map of action identifiers to action.
  std::map<ActionId, Action> _actions;
  /// Map of preconditions to action idntifiers.
  std::map<std::string, std::set<ActionId>> _preconditionToActions;
  /// Map of preconditions in expression to action idntifiers.
  std::map<std::string, std::set<ActionId>> _preconditionToActionsExps;
  /// Map of preconditions negationed to action idntifiers.
  std::map<std::string, std::set<ActionId>> _notPreconditionToActions;
  /// Set of action identifiers that does not have any precondition.
  std::set<ActionId> _actionsWithoutPrecondition;
};

} // !cp


#endif // INCLUDE_CONTEXTUALPLANNER_TYPES_DOMAIN_HPP
