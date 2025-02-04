#ifndef INCLUDE_ORDEREDGOALSPLANNER_ORDEREDGOALSPLANNER_HPP
#define INCLUDE_ORDEREDGOALSPLANNER_ORDEREDGOALSPLANNER_HPP

#include <list>
#include <map>
#include "util/api.hpp"
#include <orderedgoalsplanner/util/alias.hpp>
#include <orderedgoalsplanner/types/domain.hpp>
#include <orderedgoalsplanner/types/actioninvocationwithgoal.hpp>
#include <orderedgoalsplanner/types/actionstodoinparallel.hpp>
#include <orderedgoalsplanner/types/problem.hpp>
#include <orderedgoalsplanner/types/lookforanactionoutputinfos.hpp>

namespace ogp
{
struct ParallelPan;


/**
 * @brief Ask the planner to get the next action to do.
 * @param[in, out] pProblem Problem of the planner.
 * @param[in] pDomain Domain of the planner.
 * @param[in] pTryToDoMoreOptimalSolution True if we will try to find a result that bring the quicker to the goal.
 * @param[in] pNow Current time.
 * @param[in, opt] pGlobalHistorical A historical to give more priority to an action less frequently used.<br/>
 * Note that the problem already have a historical. The problem historical has more priority than this historical.<br/>
 * This historical can be useful if you have different problem instances and you want to favorise the action less freqently used at a global scope.
 * @param[out] pLookForAnActionOutputInfosPtr Output to know informations (is the goal satified, does the goal resolution failed, how many goals was solved, ...)
 * @return The next action to do, his parameters and information about the goal that motivated that action.
 */
ORDEREDGOALSPLANNER_API
std::list<ActionInvocationWithGoal> planForMoreImportantGoalPossible(Problem& pProblem,
                                                                     const Domain& pDomain, const SetOfCallbacks& pCallbacks,
                                                                     bool pTryToDoMoreOptimalSolution,
                                                                     const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                                                                     const Historical* pGlobalHistorical = nullptr,
                                                                     LookForAnActionOutputInfos* pLookForAnActionOutputInfosPtr = nullptr);



/**
 * @brief Ask the planner to get the next actions to do in parallel.
 * @param[in, out] pProblem Problem of the planner.
 * @param[in] pDomain Domain of the planner.
 * @param[in] pNow Current time.
 * @param[in, opt] pGlobalHistorical A historical to give more priority to an action less frequently used.<br/>
 * Note that the problem already have a historical. The problem historical has more priority than this historical.<br/>
 * @return The next actions to do in parallel.
 */
ORDEREDGOALSPLANNER_API
ActionsToDoInParallel actionsToDoInParallelNow(Problem& pProblem,
                                               const Domain& pDomain,
                                               const SetOfCallbacks& pCallbacks,
                                               const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                                               Historical* pGlobalHistorical = nullptr);


/**
 * @brief Notify that an action started. This function will update the world (contained in the problem) accordingly.
 * @param[in, out] pProblem Problem of the planner.
 * @param[in] pDomain Domain of the planner.
 * @param[in] pActionInvocationWithGoal Planner result step that motivated this action.
 * @param[in] pNow Current time.
 */
ORDEREDGOALSPLANNER_API
void notifyActionStarted(Problem& pProblem,
                         const Domain& pDomain,
                         const SetOfCallbacks& pCallbacks,
                         const ActionInvocationWithGoal& pActionInvocationWithGoal,
                         const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow);

/**
 * @brief Notify that an action finished. This function will update the world (contained in the problem) accordingly.
 * @param[in, out] pProblem Problem of the planner.
 * @param[in] pDomain Domain of the planner.
 * @param[in] pOnStepOfPlannerResult Planner result step that motivated this action.
 * @param[in] pNow Current time.
 * @return True if the action was found, False otherwise.
 */
ORDEREDGOALSPLANNER_API
bool notifyActionDone(Problem& pProblem,
                      const Domain& pDomain,
                      const SetOfCallbacks& pCallbacks,
                      const ActionInvocationWithGoal& pOnStepOfPlannerResult,
                      const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                      LookForAnActionOutputInfos* pLookForAnActionOutputInfosPtr = nullptr);


/**
 * @brief Ask the planner to get all the actions to do until:<br/>
 *  * The goals are satisfied.
 *  * We cannot satisfy more goals.
 * @param[in, out] pProblem Problem of the planner.
 * @param[in] pDomain Domain of the planner.
 * @param[in] pNow Current time.
 * @param[in, out] pGlobalHistorical Historical more global (and with a smaller priority) than the one contained in the problem.<br/>
 * The historical is used to add diversity in the actions to do. In other words, it is to always do the same action if another action is pertinent too.
 * @param[out] pGoalsDonePtr List of goals satisfied during the plannification.
 * @return List of all the actions to do with their parameters with values.
 */
ORDEREDGOALSPLANNER_API
std::list<ActionInvocationWithGoal> planForEveryGoals(Problem& pProblem,
                                                      const Domain& pDomain,
                                                      const SetOfCallbacks& pCallbacks,
                                                      const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                                                      Historical* pGlobalHistorical = nullptr,
                                                      std::list<Goal>* pGoalsDonePtr = nullptr);

ORDEREDGOALSPLANNER_API
ParallelPan parallelPlanForEveryGoals(Problem& pProblem,
                                      const Domain& pDomain,
                                      const SetOfCallbacks& pCallbacks,
                                      const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                                      Historical* pGlobalHistorical);

/**
 * @brief Convert a plan to a string.
 * @param[in] pPlan Plan to print.
 * @param[in] pSep Separator between each action of the plan to do sequentially.
 * @return The plan written in string.
 */
ORDEREDGOALSPLANNER_API
std::string planToStr(const std::list<ActionInvocationWithGoal>& pPlan,
                      const std::string& pSep = ", ");

ORDEREDGOALSPLANNER_API
std::string parallelPlanToStr(const ParallelPan& pPlan);

ORDEREDGOALSPLANNER_API
std::string planToPddl(const std::list<ActionInvocationWithGoal>& pPlan,
                       const Domain& pDomain);

ORDEREDGOALSPLANNER_API
std::string parallelPlanToPddl(const ParallelPan& pPlan,
                               const Domain& pDomain);
/**
 * @brief Convert a list of goals to a string.
 * @param[in] pGoals List of goals to print.
 * @param[in] pSep Separator between each action of the plan to do sequentially.
 * @return The list of goals written in string.
 */
ORDEREDGOALSPLANNER_API
std::string goalsToStr(const std::list<Goal>& pGoals,
                       const std::string& pSep = ", ");


/// Check that the plan is valid and no actions are done for nothing
ORDEREDGOALSPLANNER_API
bool evaluate
(ParallelPan& pPlan,
 Problem& pProblem,
 const Domain& pDomain);

} // !ogp


#endif // INCLUDE_ORDEREDGOALSPLANNER_ORDEREDGOALSPLANNER_HPP
