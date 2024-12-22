#ifndef INCLUDE_ORDEREDGOALSPLANNER_GOAL_HPP
#define INCLUDE_ORDEREDGOALSPLANNER_GOAL_HPP

#include <memory>
#include <string>
#include <chrono>
#include "condition.hpp"
#include "factoptional.hpp"
#include "../util/api.hpp"
#include <orderedgoalsplanner/util/alias.hpp>

namespace ogp
{
struct Domain;


// A characteristic that the world should have. It is the motivation of the bot for doing actions to respect this characteristic of the world.
struct ORDEREDGOALSPLANNER_API Goal
{
  Goal(std::unique_ptr<Condition> pObjective,
       bool pIsPersistentIfSkipped = false,
       bool pOneStepTowards = false,
       int pMaxTimeToKeepInactive = -1,
       const std::string& pGoalGroupId = "");

  /// Construct a goal from another goal.
  Goal(const Goal& pOther,
       const std::map<Parameter, Entity>* pParametersPtr = nullptr,
       const std::string* pGoalGroupIdPtr = nullptr);

  static Goal fromStr(const std::string& pStr,
                      const Ontology& pOntology,
                      const SetOfEntities& pEntities,
                      int pMaxTimeToKeepInactive = -1,
                      const std::string& pGoalGroupId = "");

  /// Set content from another goal.
  void operator=(const Goal& pOther);

  /// Check equality with another goal.
  bool operator==(const Goal& pOther) const;
  /// Check not equality with another goal.
  bool operator!=(const Goal& pOther) const { return !operator==(pOther); }

  /// Clone the goal.
  std::unique_ptr<Goal> clone() const;

  /// Set start of inactive time, if not already set. (inactive = not in top of the goals stack)
  void setInactiveSinceIfNotAlreadySet(const std::unique_ptr<std::chrono::steady_clock::time_point>& pInactiveSince);

  // TODO: Optimize by having to lazy current time value!
  /**
   * @brief Check if the goal is inactive for too long.
   * @param pNow Current time.
   * @return True if the fact is inactive for too long, false otherwise.
   */
  bool isInactiveForTooLong(const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow) const;

  /// Notify that the gaol is active. (so in top of the goals stack)
  void notifyActivity();

  /// Get the time when this goal became incative. It is a null pointer if the goal is active.
  const std::unique_ptr<std::chrono::steady_clock::time_point>& getInactiveSince() const { return _inactiveSince; }

  /// Convert this goal to a string.
  std::string toStr() const;

  /// Convert this goal to a PDDL string.
  std::string toPddl(std::size_t pIdentation) const;

  /// Know if the goal will be kept in the goals stack, when we succeded or failed to satisfy it.
  bool isPersistent() const { return _isPersistentIfSkipped; }

  /// Know if the goal will be removed from the goals stack after the first time we try to satisfy it.
  bool isOneStepTowards() const { return _oneStepTowards; }

  /// Get a const reference of the optional fact contained in this goal.
  const Condition& objective() const { return *_objective; }

  /// Get a reference of the optional fact contained in this goal.
  Condition& objective() { return *_objective; }

  /// Get the group identifier of this goal. It can be empty if the goal does not belong to a group.
  const std::string& getGoalGroupId() const { return _goalGroupId; }

  /**
   * Get the maximum time that we allow for this goal to be inactive.<b/>
   * A negative value means that the time is infinite.
   */
  int getMaxTimeToKeepInactive() const { return _maxTimeToKeepInactive; }

  bool isASimpleFactObjective() const;

  void refreshIfNeeded(const Domain& pDomain);

  std::string printActionsThatCanSatisfyThisGoal() const;

  bool canActionSatisfyThisGoal(const ActionId& pActionId) const;

  bool canEventSatisfyThisGoal(const ActionId& pFullEventId) const;

  bool canDeductionSatisfyThisGoal(const ActionId& pDeductionId) const;

  const std::set<ActionId>& getActionsPredecessors() const { return _cacheOfActionsPredecessors; }
  const std::set<FullEventId>& getEventsPredecessors() const { return _cacheOfEventsPredecessors; }

  /// Persist function name.
  static const std::string& getPersistFunctionName();
  /// OneStepTowards function name.
  static const std::string& getOneStepTowardsFunctionName();

private:
  /// Condition that the world should satify in the world.
  std::unique_ptr<Condition> _objective;
  /**
   * The maximum time that we allow for this goal to be inactive in second.<br/>
   * A negative value means that the time is infinite.<br/>
   * A goal is inactive if the there is a not satisfied goal upper in the stack of goals of a problem.
   */
  int _maxTimeToKeepInactive;
  /// Time when this goal became inactive. It is a null pointer if the goal is active.
  std::unique_ptr<std::chrono::steady_clock::time_point> _inactiveSince;
  /// Know if the goal will be kept in the goals stack, when we succeded or failed to satisfy it.
  bool _isPersistentIfSkipped;
  /// Know if the goal will be removed from the goals stack after the first time we try to satisfy it.
  bool _oneStepTowards;
  /// Group identifier of this goal. It can be empty if the goal does not belong to a group.
  std::string _goalGroupId;
  std::string _uuidOfLastDomainUsedForCache;
  std::set<ActionId> _cacheOfActionsThatCanSatisfyThisGoal;
  std::set<std::string> _cacheOfEventsIdThatCanSatisfyThisGoal;
  std::set<ActionId> _cacheOfActionsPredecessors;
  std::set<FullEventId> _cacheOfEventsPredecessors;
};

} // !ogp


#endif // INCLUDE_ORDEREDGOALSPLANNER_GOAL_HPP
