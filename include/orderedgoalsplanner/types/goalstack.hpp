#ifndef INCLUDE_ORDEREDGOALSPLANNER_GOALSTACK_HPP
#define INCLUDE_ORDEREDGOALSPLANNER_GOALSTACK_HPP

#include <set>
#include <map>
#include <memory>
#include "../util/api.hpp"
#include <orderedgoalsplanner/types/fact.hpp>
#include <orderedgoalsplanner/types/goal.hpp>
#include <orderedgoalsplanner/types/event.hpp>
#include <orderedgoalsplanner/types/worldstate.hpp>
#include <orderedgoalsplanner/util/observableunsafe.hpp>


namespace ogp
{
struct Domain;
struct ProblemModification;
struct SetOfEvents;
struct ActionInvocationWithGoal;
struct LookForAnActionOutputInfos;


/// Current world, goal for the world and historical of actions done.
struct ORDEREDGOALSPLANNER_API GoalStack
{
  /// Construct a problem.
  GoalStack() = default;
  /// Construct a problem from another problem.
  GoalStack(const GoalStack& pOther);
  GoalStack& operator=(const GoalStack& pOther);

  /**
   * @brief Notify that an action has been done.
   * @param[in] pOnStepOfPlannerResult Planner result step that motivated this action.
   * @param pNow Current time.
   * @param pGoalsToAdd Priorities to goals to add.
   * @param pGoalsToAddInCurrentPriority Goals to add in current priority.
   * @param[in] pWorldState World state to consider.
   * @param[out] pLookForAnActionOutputInfosPtr Output to know informations (is the goal satified, does the goal resolution failed, how many goals was solved, ...)
   * @return True, if the goal stack has changed.
   */
  bool notifyActionDone(const ActionInvocationWithGoal& pOneStepOfPlannerResult,
                        const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                        const std::map<int, std::vector<Goal>>* pGoalsToAdd,
                        const std::vector<Goal>* pGoalsToAddInCurrentPriority,
                        const WorldState& pWorldState,
                        const SetOfEntities& pConstants,
                        const SetOfEntities& pObjects,
                        LookForAnActionOutputInfos* pLookForAnActionOutputInfosPtr);

  /// Be notified when goals changed.
  ogpstd::observable::ObservableUnsafe<void (const std::map<int, std::vector<Goal>>&)> onGoalsChanged{};


  // Goal stack
  // ----------

  /// Get the goal serialized in string in top of the stack of goals, an empty string is returned if there is no goal.
  std::string getCurrentGoalStr() const;

  /// Get the goal in top of the stack of goals, nullptr is returned if there is no goal.
  const Goal* getCurrentGoalPtr() const;

  /**
   * @brief Iterate on goals and remove non persistent goals.
   * @param pManageGoal Callback to manage the goal. If the callback returns true we stop the iteration.
   * @param[in] pWorldState World state to consider.
   * @param pNow Current time.
   * @param[out] pLookForAnActionOutputInfosPtr Output to know informations (is the goal satified, does the goal resolution failed, how many goals was solved, ...)
   */
  void iterateOnGoalsAndRemoveNonPersistent(const std::function<bool (Goal&, int)>& pManageGoal,
                                            const WorldState& pWorldState,
                                            const SetOfEntities& pConstants,
                                            const SetOfEntities& pObjects,
                                            const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                                            LookForAnActionOutputInfos* pLookForAnActionOutputInfosPtr);

  /// Default priority.
  static int getDefaultPriority() { return 10; }

  /**
   * @brief Set the goals.
   * @param pGoals Map of priority to the associated goals.
   * @param[in] pWorldState World state to consider.
   * @param pNow Current time.
   */
  void setGoals(const std::map<int, std::vector<Goal>>& pGoals,
                const WorldState& pWorldState,
                const SetOfEntities& pConstants,
                const SetOfEntities& pObjects,
                const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow);

  /**
   * @brief Set the goals.
   * @param pGoals Set of goals.
   * @param[in] pWorldState World state to consider.
   * @param pNow Current time.
   * @param pPriority Priority of the goals.
   */
  void setGoals(const std::vector<Goal>& pGoals,
                const WorldState& pWorldState,
                const SetOfEntities& pConstants,
                const SetOfEntities& pObjects,
                const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                int pPriority = getDefaultPriority());

  /**
   * @brief Add some goals.
   * @param pGoals Map of priority to the associated goals.
   * @param[in] pWorldState World state to consider.
   * @param pNow Current time.
   * @return True, if the goal stack has changed.
   */
  bool addGoals(const std::map<int, std::vector<Goal>>& pGoals,
                const WorldState& pWorldState,
                const SetOfEntities& pConstants,
                const SetOfEntities& pObjects,
                const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow);

  /**
   * @brief Add some goals.
   * @param pGoals Set of goals to add.
   * @param[in] pWorldState World state to consider.
   * @param pNow Current time.
   * @param pPriority Priority of the goals to add.
   * @return True, if the goal stack has changed.
   */
  bool addGoals(const std::vector<Goal>& pGoals,
                const WorldState& pWorldState,
                const SetOfEntities& pConstants,
                const SetOfEntities& pObjects,
                const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                int pPriority = getDefaultPriority());

  /**
   * @brief Add a goal in front of the existing goals that have the same level of priority.
   * @param pGoal Goal to add.
   * @param[in] pWorldState World state to consider.
   * @param pNow Current time.
   * @param pPriorityPriority of the goal to add.
   */
  void pushFrontGoal(const Goal& pGoal,
                     const WorldState& pWorldState,
                     const SetOfEntities& pConstants,
                     const SetOfEntities& pObjects,
                     const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                     int pPriority = getDefaultPriority());

  /**
   * @brief Add a goal on bottom of the existing goals that have the same level of priority.
   * @param pGoal Goal to add.
   * @param[in] pWorldState World state to consider.
   * @param pNow Current time.
   * @param pPriorityPriority of the goal to add.
   */
  void pushBackGoal(const Goal& pGoal,
                    const WorldState& pWorldState,
                    const SetOfEntities& pConstants,
                    const SetOfEntities& pObjects,
                    const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                    int pPriority = getDefaultPriority());

  /**
   * @brief Change the priority of a goal.
   * @param pGoalStr Goal concerned.
   * @param pPriority New priority to set.
   * @param pPushFrontOrBottomInCaseOfConflictWithAnotherGoal Push in front or in bottom in case of conflict with another goal.
   * @param[in] pWorldState World state to consider.
   * @param pNow Current time.
   */
  void changeGoalPriority(const std::string& pGoalStr,
                          int pPriority,
                          bool pPushFrontOrBottomInCaseOfConflictWithAnotherGoal,
                          const WorldState& pWorldState,
                          const SetOfEntities& pConstants,
                          const SetOfEntities& pObjects,
                          const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow);

  /**
   * @brief Clear the goals.
   * @param[in] pWorldState World state to consider.
   * @param pNow Current time.
   */
  void clearGoals(const WorldState& pWorldState,
                  const SetOfEntities& pConstants,
                  const SetOfEntities& pObjects,
                  const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow);

  /**
   * @brief Remove some goals.
   * @param pGoalGroupId Group identifier of the goals to remove.
   * @param[in] pWorldState World state to consider.
   * @param pNow Current time.
   * @return True if at least one goal is removed, false otherwise.
   */
  bool removeGoals(const std::string& pGoalGroupId,
                   const WorldState& pWorldState,
                   const SetOfEntities& pConstants,
                   const SetOfEntities& pObjects,
                   const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow);

  /**
   * @brief Remove the first goals that are already satisfied.
   * @param[in] pWorldState World state to consider.
   * @param[in] pNow Current time.
   */
  void removeFirstGoalsThatAreAlreadySatisfied(const WorldState& pWorldState,
                                               const SetOfEntities& pConstants,
                                               const SetOfEntities& pObjects,
                                               const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow);

  /// Goals to satisfy.
  const std::map<int, std::vector<Goal>>& goals() const { return _goals; }

  /**
   * @brief Get the not satisfied goals.<br/>
   * A goal is not satisfied if the condition is true (if it exist) and if the value of the goal is not true.
   * @param[in] pWorldState World state to consider.
   * @return Map of priority to not satisfied goals.
   */
  std::map<int, std::vector<Goal>> getNotSatisfiedGoals(const WorldState& pWorldState,
                                                        const SetOfEntities& pConstants,
                                                        const SetOfEntities& pObjects) const;

  void refreshIfNeeded(const Domain& pDomain);

  std::string printGoalsCache() const;

  std::set<ActionId> getActionsPredecessors() const;

  std::set<ActionId> getEventsPredecessors() const;


private:
  /// Map of priority to goals.
  std::map<int, std::vector<Goal>> _goals{};
  /// Current active goal.
  const Goal* _currentGoalPtr = nullptr;

  void _removeNoStackableGoalsAndNotifyGoalsChanged(const WorldState& pWorldState,
                                                    const SetOfEntities& pConstants,
                                                    const SetOfEntities& pObjects,
                                                    const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow);

  /**
   * @brief Remove the first goals that are already satisfied.
   * @param[in] pWorldState World state to consider.
   * @param[in] pNow Current time.
   * @param[out] pLookForAnActionOutputInfosPtr Output to know informations (is the goal satified, does the goal resolution failed, how many goals was solved, ...)
   * @return True, if the goal stack has changed.
   */
  bool _removeFirstGoalsThatAreAlreadySatisfied(const WorldState& pWorldState,
                                                const SetOfEntities& pConstants,
                                                const SetOfEntities& pObjects,
                                                const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                                                LookForAnActionOutputInfos* pLookForAnActionOutputInfosPtr);

  /**
   * @brief Iterate on goals and remove non persistent goals.
   * @param pManageGoal Callback to manage the goal. If the callback returns true we stop the iteration.
   * @param[in] pWorldState World state to consider.
   * @param[in] pNow Current time.
   * @param[out] pLookForAnActionOutputInfosPtr Output to know informations (is the goal satified, does the goal resolution failed, how many goals was solved, ...)
   * @return True, if the goal stack has changed.
   */
  bool _iterateOnGoalsAndRemoveNonPersistent(const std::function<bool (Goal&, int)>& pManageGoal,
                                             const WorldState& pWorldState,
                                             const SetOfEntities& pConstants,
                                             const SetOfEntities& pObjects,
                                             const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                                             LookForAnActionOutputInfos* pLookForAnActionOutputInfosPtr);

  /// Get the priority of the goal in top of the stack.
  int _getCurrentPriority(const WorldState& pWorldState,
                          const SetOfEntities& pConstants,
                          const SetOfEntities& pObjects) const;


  /**
   * @brief Remove no stackable goals.
   * @param[in] pWorldState World state to consider.
   * @param[in] pNow Current time.
   * @return True, if the goal stack has changed.
   */
  bool _removeNoStackableGoals(const WorldState& pWorldState,
                               const SetOfEntities& pConstants,
                               const SetOfEntities& pObjects,
                               const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow);


  /**
   * @brief Add some goals.
   * @param[in] pGoals Set of goals to add.
   * @param[in] pWorldState World state to consider.
   * @param[in] pNow Current time.
   * @return True, if the goal stack has changed.
   */
  bool _addGoals(const std::map<int, std::vector<Goal>>& pGoals,
                 const WorldState& pWorldState,
                 const SetOfEntities& pConstants,
                 const SetOfEntities& pObjects,
                 const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow);

  friend struct WorldState;
};

} // !ogp


#endif // INCLUDE_ORDEREDGOALSPLANNER_GOALSTACK_HPP
