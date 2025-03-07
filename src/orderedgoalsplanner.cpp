#include <orderedgoalsplanner/orderedgoalsplanner.hpp>
#include <algorithm>
#include <iomanip>
#include <optional>
#include <orderedgoalsplanner/types/entitieswithparamconstraints.hpp>
#include <orderedgoalsplanner/types/parallelplan.hpp>
#include <orderedgoalsplanner/types/setofcallbacks.hpp>
#include <orderedgoalsplanner/types/setofevents.hpp>
#include <orderedgoalsplanner/util/util.hpp>
#include "types/factsalreadychecked.hpp"
#include "types/treeofalreadydonepaths.hpp"
#include "algo/actiondataforparallelisation.hpp"
#include "algo/converttoparallelplan.hpp"
#include "algo/notifyactiondone.hpp"

namespace ogp
{

namespace
{
static const ParameterValuesWithConstraints _emptyParameters;

enum class PossibleEffect
{
  SATISFIED,
  SATISFIED_BUT_DOES_NOT_MODIFY_THE_WORLD,
  NOT_SATISFIED
};

PossibleEffect _merge(PossibleEffect pEff1,
                      PossibleEffect pEff2)
{
  if (pEff1 == PossibleEffect::SATISFIED ||
      pEff2 == PossibleEffect::SATISFIED)
    return PossibleEffect::SATISFIED;
  if (pEff1 == PossibleEffect::SATISFIED_BUT_DOES_NOT_MODIFY_THE_WORLD ||
      pEff2 == PossibleEffect::SATISFIED_BUT_DOES_NOT_MODIFY_THE_WORLD)
    return PossibleEffect::SATISFIED_BUT_DOES_NOT_MODIFY_THE_WORLD;
  return PossibleEffect::NOT_SATISFIED;
}

struct PlanCost
{
  bool success = true;
  std::size_t nbOfGoalsNotSatisfied = 0;
  std::size_t nbOfGoalsSatisfied = 0;
  Number totalCost = 0;
  Number costForFirstGoal = 0;

  bool isBetterThan(const PlanCost& pOther) const
  {
    if (success != pOther.success)
      return success;
    if (nbOfGoalsNotSatisfied != pOther.nbOfGoalsNotSatisfied)
      return nbOfGoalsNotSatisfied > pOther.nbOfGoalsNotSatisfied;
    if (nbOfGoalsSatisfied != pOther.nbOfGoalsSatisfied)
      return nbOfGoalsSatisfied > pOther.nbOfGoalsSatisfied;
    if (costForFirstGoal != pOther.costForFirstGoal)
      return costForFirstGoal < pOther.costForFirstGoal;
    return totalCost < pOther.totalCost;
  }
};

struct PotentialNextActionComparisonCache
{
  PlanCost currentCost;
  std::list<const ProblemModification*> effectsWithWorseCosts;
};


struct ActionPtrWithGoal
{
  ActionPtrWithGoal(const Action* pActionPtr,
                    const ogp::Goal& pGoal)
   : actionPtr(pActionPtr),
     goal(pGoal)
  {
  }

  const Action* actionPtr;
  const ogp::Goal& goal;
};


struct DataRelatedToOptimisation
{
  bool tryToDoMoreOptimalSolution = false;
  ParameterValuesWithConstraints parameterToEntitiesFromEvent;
};


struct ActionInvocationWithPtr
{
  ActionInvocationWithPtr(
      ActionInvocation pActionInvocation,
      const Action* pActionPtr)
    : actionInvocation(pActionInvocation),
      actionPtr(pActionPtr)
  {
  }

  bool nextStepIsAnEvent(const ParameterValuesWithConstraints& pParameterToEntitiesFromEvent) const
  {
    for (auto& currParam : actionInvocation.parameters)
    {
      auto it = pParameterToEntitiesFromEvent.find(currParam.first);
      if (it != pParameterToEntitiesFromEvent.end())
        if (it->second.count(currParam.second) > 0)
          return true;
    }
    return false;
  }

  bool isMoreImportantThan(const ActionInvocationWithPtr& pOther,
                           const Problem& pProblem,
                           const Historical* pGlobalHistorical) const;

  ActionInvocation actionInvocation;
  const Action* actionPtr;
};


struct PotentialNextAction
{
  PotentialNextAction()
    : actionId(""),
      actionPtr(nullptr),
      parameters()
  {
  }
  PotentialNextAction(const ActionId& pActionId,
                      const Action& pAction);

  ActionId actionId;
  const Action* actionPtr;
  ParameterValuesWithConstraints parameters;

  std::list<ActionInvocationWithPtr> toActionInvocations(const SetOfEntities& pConstants,
                                                         const SetOfEntities& pObjects);
};



PotentialNextAction::PotentialNextAction(const ActionId& pActionId,
                                         const Action& pAction)
  : actionId(pActionId),
    actionPtr(&pAction),
    parameters()
{
  for (const auto& currParam : pAction.parameters)
    parameters[currParam];
}


struct ResearchContext
{
  ResearchContext(const Goal& pGoal,
                  const Problem& pProblem,
                  const Domain& pDomain,
                  const std::set<ActionId>& pActionIds,
                  const std::set<FullEventId>& pFullEventIds)
    : goal(pGoal),
      problem(pProblem),
      domain(pDomain),
      actionIds(pActionIds),
      fullEventIds(pFullEventIds)
  {
  }

  const Goal& goal;
  const Problem& problem;
  const Domain& domain;
  const std::set<ActionId>& actionIds;
  const std::set<FullEventId>& fullEventIds;
};



std::list<ActionInvocationWithGoal> _planForMoreImportantGoalPossible(Problem& pProblem,
                                                                      const Domain& pDomain,
                                                                      const SetOfCallbacks& pCallbacks,
                                                                      bool pTryToDoMoreOptimalSolution,
                                                                      const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                                                                      const Historical* pGlobalHistorical,
                                                                      LookForAnActionOutputInfos* pLookForAnActionOutputInfosPtr,
                                                                      const ActionPtrWithGoal* pPreviousActionPtr);

void _getPreferInContextStatistics(std::size_t& nbOfPreconditionsSatisfied,
                                   std::size_t& nbOfPreconditionsNotSatisfied,
                                   const Action& pAction,
                                   const std::map<Fact, bool>& pFacts)
{
  auto onFact = [&](const FactOptional& pFactOptional,
                    bool) -> ContinueOrBreak
  {
    if (pFactOptional.isFactNegated)
    {
      if (pFacts.count(pFactOptional.fact) == 0)
        ++nbOfPreconditionsSatisfied;
      else
        ++nbOfPreconditionsNotSatisfied;
    }
    else
    {
      if (pFacts.count(pFactOptional.fact) > 0)
        ++nbOfPreconditionsSatisfied;
      else
        ++nbOfPreconditionsNotSatisfied;
    }
    return ContinueOrBreak::CONTINUE;
  };

  if (pAction.preferInContext)
    pAction.preferInContext->forAll(onFact);
}


std::list<ActionInvocationWithPtr> PotentialNextAction::toActionInvocations(const SetOfEntities& pConstants,
                                                                            const SetOfEntities& pObjects)
{
  std::list<ActionInvocationWithPtr> res;
  if (parameters.empty())
  {
    res.emplace_back(ActionInvocation(actionId, std::map<Parameter, Entity>()), actionPtr);
    return res;
  }

  // remove any entities
  for (auto& currParam : parameters)
  {
    if (currParam.second.empty())
    {
      if (currParam.first.type)
        currParam.second = typeToEntities(*currParam.first.type, pConstants, pObjects);
    }
    else
    {
      for (auto itEntity = currParam.second.begin(); itEntity != currParam.second.end(); )
      {
        auto& currEntity = *itEntity;
        if (currEntity.first.isAnyEntity() && currEntity.first.type)
        {
          auto possibleValues = typeToEntities(*currEntity.first.type, pConstants, pObjects);
          itEntity = currParam.second.erase(itEntity);
          currParam.second.insert(possibleValues.begin(), possibleValues.end());
          continue;
        }
        ++itEntity;
      }
    }
  }

  std::list<std::map<Parameter, Entity>> parameterPossibilities;
  unfoldMapWithSet(parameterPossibilities, parameters);

  for (auto& currParams : parameterPossibilities)
    res.emplace_back(ActionInvocation(actionId, std::move(currParams)), actionPtr);
  return res;
}


bool ActionInvocationWithPtr::isMoreImportantThan(const ActionInvocationWithPtr& pOther,
                                                  const Problem& pProblem,
                                                  const Historical* pGlobalHistorical) const
{
  if (actionPtr == nullptr)
    return false;
  auto& action = *actionPtr;
  if (pOther.actionPtr == nullptr)
    return true;
  auto& otherAction = *pOther.actionPtr;

  auto nbOfTimesAlreadyDone = pProblem.historical.getNbOfTimeAnActionHasAlreadyBeenDone(actionInvocation.actionId);
  auto otherNbOfTimesAlreadyDone = pProblem.historical.getNbOfTimeAnActionHasAlreadyBeenDone(pOther.actionInvocation.actionId);

  if (action.highImportanceOfNotRepeatingIt)
  {
    if (otherAction.highImportanceOfNotRepeatingIt)
    {
      if (nbOfTimesAlreadyDone != otherNbOfTimesAlreadyDone)
        return nbOfTimesAlreadyDone < otherNbOfTimesAlreadyDone;
    }
    else if (nbOfTimesAlreadyDone > 0)
    {
      return false;
    }
  }
  else if (otherAction.highImportanceOfNotRepeatingIt && otherNbOfTimesAlreadyDone > 0)
  {
    return true;
  }

  // Compare according to prefer in context
  std::size_t nbOfPreferInContextSatisfied = 0;
  std::size_t nbOfPreferInContextNotSatisfied = 0;
  _getPreferInContextStatistics(nbOfPreferInContextSatisfied, nbOfPreferInContextNotSatisfied, action, pProblem.worldState.facts());
  std::size_t otherNbOfPreconditionsSatisfied = 0;
  std::size_t otherNbOfPreconditionsNotSatisfied = 0;
  _getPreferInContextStatistics(otherNbOfPreconditionsSatisfied, otherNbOfPreconditionsNotSatisfied, otherAction, pProblem.worldState.facts());
  if (nbOfPreferInContextSatisfied != otherNbOfPreconditionsSatisfied)
    return nbOfPreferInContextSatisfied > otherNbOfPreconditionsSatisfied;
  if (nbOfPreferInContextNotSatisfied != otherNbOfPreconditionsNotSatisfied)
    return nbOfPreferInContextNotSatisfied < otherNbOfPreconditionsNotSatisfied;

  if (nbOfTimesAlreadyDone != otherNbOfTimesAlreadyDone)
    return nbOfTimesAlreadyDone < otherNbOfTimesAlreadyDone;

  if (pGlobalHistorical != nullptr)
  {
    nbOfTimesAlreadyDone = pGlobalHistorical->getNbOfTimeAnActionHasAlreadyBeenDone(actionInvocation.actionId);
    otherNbOfTimesAlreadyDone = pGlobalHistorical->getNbOfTimeAnActionHasAlreadyBeenDone(pOther.actionInvocation.actionId);
    if (nbOfTimesAlreadyDone != otherNbOfTimesAlreadyDone)
      return nbOfTimesAlreadyDone < otherNbOfTimesAlreadyDone;
  }
  return actionInvocation.actionId < pOther.actionInvocation.actionId;
}


bool _lookForAPossibleEffect(ParameterValuesWithConstraints& pParametersWithTmpData,
                             DataRelatedToOptimisation& pDataRelatedToOptimisation,
                             TreeOfAlreadyDonePath& pTreeOfAlreadyDonePath,
                             const std::unique_ptr<ogp::WorldStateModification>& pWorldStateModificationPtr1,
                             const std::unique_ptr<ogp::WorldStateModification>& pWorldStateModificationPtr2,
                             const ResearchContext& pContext,
                             FactsAlreadyChecked& pFactsAlreadychecked,
                             const std::string& pFromDeductionId);


bool _fillParameter(const Parameter& pParameter,
                    EntitiesWithParamConstaints& pParameterValues,
                    ParameterValuesWithConstraints& pNewParentParameters,
                    const ParameterValuesWithConstraints& pHoldingActionParameters,
                    const Condition& pCondition,
                    const Fact& pFact,
                    ParameterValuesWithConstraints& pParentParameters,
                    ParameterValuesWithConstraints* pTmpParentParametersPtr,
                    const ResearchContext& pContext)
{
  if (pParameterValues.empty() &&
      pFact.hasParameterOrValue(pParameter))
  {
    const auto& ontology = pContext.domain.getOntology();
    auto& newParamValues = pNewParentParameters[pParameter];

    std::shared_ptr<Type> parameterType = pParameter.type;
    bool foundSomethingThatMatched = false;
    pCondition.findConditionCandidateFromFactFromEffect(
          [&](const FactOptional& pConditionFactOptional,
              const Fact* pOtherPatternPtr, const Fact* pOtherInstancePtr)
    {
      ParamToEntityValues constraints;
      auto parentParamValue = pFact.tryToExtractArgumentFromExample(pParameter, pConditionFactOptional.fact);
      if (!parentParamValue)
        return false;
      foundSomethingThatMatched = true;
      if (pOtherPatternPtr != nullptr && pOtherInstancePtr != nullptr && parentParamValue->isAParameterToFill())
      {
        auto& otherInstance = *pOtherInstancePtr;
        auto newPotentialParentParamValue = pOtherPatternPtr->tryToExtractArgumentFromExample(parentParamValue->toParameter(), otherInstance);
        if (newPotentialParentParamValue)
        {
          parentParamValue = newPotentialParentParamValue;
          if (pFact.value() && pFact.value()->isAParameterToFill() && otherInstance.value() && !otherInstance.value()->isAParameterToFill())
            constraints[pFact.value()->toParameter()].insert(*otherInstance.value());
        }
      }

      // Maybe the extracted parameter is also a parameter so we replace by it's value
      auto itParam = pHoldingActionParameters.find(parentParamValue->toParameter());
      if (itParam != pHoldingActionParameters.end())
        newParamValues = itParam->second;
      else if (!parentParamValue->isAParameterToFill() || parentParamValue->isAnyEntity())
        newParamValues.emplace(*parentParamValue, std::move(constraints));
      else if (parentParamValue->type && parameterType && parentParamValue->type->isA(*parameterType))
        parameterType = parentParamValue->type;
      return !newParamValues.empty();
    }, pContext.problem.worldState, ontology.constants, pContext.problem.objects, pFact, pParentParameters, pTmpParentParametersPtr, pHoldingActionParameters);

    if (foundSomethingThatMatched && newParamValues.empty())
    {
      if (parameterType)
      {
        // find all the possible occurence in the entities
        newParamValues = typeToEntities(*parameterType, ontology.constants, pContext.problem.objects);

        // remove the ones that are already in the world state for this fact
        std::set<Entity> parameterValues;
        pContext.problem.worldState.factsMapping().extractPotentialArgumentsOfAFactParameter(parameterValues, pFact, pParameter.name, {}, nullptr);
        for (const auto& elem : parameterValues)
          newParamValues.erase(elem);
      }
      return !newParamValues.empty();
    }

  }
  return true;
}


PossibleEffect _checkConditionAndFillParameters(const Condition& pCondition,
                                                const FactOptional& pFactOptional,
                                                ParameterValuesWithConstraints& pParentParameters,
                                                ParameterValuesWithConstraints* pTmpParentParametersPtr,
                                                const ResearchContext& pContext,
                                                const ParameterValuesWithConstraints& pHoldingActionParameters)
{
  // fill parent parameters
  ParameterValuesWithConstraints newParentParameters;
  for (auto& currParentParam : pParentParameters)
    if (!_fillParameter(currParentParam.first, currParentParam.second, newParentParameters,
                        pHoldingActionParameters, pCondition, pFactOptional.fact,
                        pParentParameters, pTmpParentParametersPtr, pContext))
      return PossibleEffect::NOT_SATISFIED;

  if (pTmpParentParametersPtr != nullptr)
  {
    ParameterValuesWithConstraints newTmpParentParameters;
    for (auto& currParentParam : *pTmpParentParametersPtr)
      if (!_fillParameter(currParentParam.first, currParentParam.second, newTmpParentParameters,
                          pHoldingActionParameters, pCondition, pFactOptional.fact,
                          pParentParameters, pTmpParentParametersPtr, pContext))
        return PossibleEffect::NOT_SATISFIED;
    applyNewParams(*pTmpParentParametersPtr, newTmpParentParameters);
  }
  applyNewParams(pParentParameters, newParentParameters);

  // Check that the new fact pattern is not already satisfied
  if (pContext.problem.worldState.canBeModifiedBy(pFactOptional, pParentParameters))
    return PossibleEffect::SATISFIED;
  return PossibleEffect::SATISFIED_BUT_DOES_NOT_MODIFY_THE_WORLD;
}


PossibleEffect _lookForAPossibleDeduction(TreeOfAlreadyDonePath& pTreeOfAlreadyDonePath,
                                          const std::vector<Parameter>& pParameters,
                                          const Condition& pCondition,
                                          const std::unique_ptr<ogp::WorldStateModification>& pWorldStateModificationPtr1,
                                          const std::unique_ptr<ogp::WorldStateModification>& pWorldStateModificationPtr2,
                                          const FactOptional& pFactOptional,
                                          ParameterValuesWithConstraints& pParentParameters,
                                          ParameterValuesWithConstraints* pTmpParentParametersPtr,
                                          const ResearchContext& pContext,
                                          FactsAlreadyChecked& pFactsAlreadychecked,
                                          const std::string& pFromDeductionId)
{
  ParameterValuesWithConstraints parametersWithData;
  for (const auto& currParam : pParameters)
    parametersWithData[currParam];

  DataRelatedToOptimisation dataRelatedToOptimisation;
  if (_lookForAPossibleEffect(parametersWithData, dataRelatedToOptimisation, pTreeOfAlreadyDonePath,
                              pWorldStateModificationPtr1, pWorldStateModificationPtr2,
                              pContext, pFactsAlreadychecked, pFromDeductionId))
    return _checkConditionAndFillParameters(pCondition, pFactOptional, pParentParameters, pTmpParentParametersPtr,
                                            pContext, parametersWithData);

  return PossibleEffect::NOT_SATISFIED;
}


bool _updatePossibleParameters(
    ParameterValuesWithConstraints& pNewPossibleParentParameters,
    ParameterValuesWithConstraints& pNewPossibleTmpParentParameters,
    ParameterValuesWithConstraints& pParentParameters,
    ParameterValuesWithConstraints& pCpParentParameters,
    ParameterValuesWithConstraints* pTmpParentParametersPtr,
    DataRelatedToOptimisation& pDataRelatedToOptimisation,
    ParameterValuesWithConstraints& pCpTmpParameters,
    bool pFromEvent)
{
  if (pCpParentParameters.empty() && pCpTmpParameters.empty())
    return true;

  if (!pDataRelatedToOptimisation.tryToDoMoreOptimalSolution)
  {
    pParentParameters = std::move(pCpParentParameters);
    if (pTmpParentParametersPtr != nullptr)
      *pTmpParentParametersPtr = std::move(pCpTmpParameters);
    return true;
  }

  if (pFromEvent && pDataRelatedToOptimisation.tryToDoMoreOptimalSolution)
  {
    for (auto& currParam : pCpParentParameters)
    {
      auto& currentEntities = pNewPossibleParentParameters[currParam.first];
      for (auto& currEntity : currParam.second)
      {
        if (currentEntities.emplace(currEntity).second)
          pDataRelatedToOptimisation.parameterToEntitiesFromEvent[currParam.first].insert(currEntity);
      }
    }
  }
  else
  {
    for (auto& currParam : pCpParentParameters)
      pNewPossibleParentParameters[currParam.first].insert(currParam.second.begin(), currParam.second.end());
  }

  if (pTmpParentParametersPtr != nullptr)
    for (auto& currParam : pCpTmpParameters)
      pNewPossibleTmpParentParameters[currParam.first].insert(currParam.second.begin(), currParam.second.end());
  return false;
}


void _lookForAPossibleExistingOrNotFactFromActionsAndEvents(
    PossibleEffect& res,
    ParameterValuesWithConstraints& newPossibleParentParameters,
    ParameterValuesWithConstraints& newPossibleTmpParentParameters,
    const std::set<ActionId>& pActionSuccessions,
    const std::map<SetOfEventsId, std::set<EventId>>& pEventSuccessions,
    const FactOptional& pFactOptional,
    ParameterValuesWithConstraints& pParentParameters,
    ParameterValuesWithConstraints* pTmpParentParametersPtr,
    DataRelatedToOptimisation& pDataRelatedToOptimisation,
    TreeOfAlreadyDonePath& pTreeOfAlreadyDonePath,
    const std::map<SetOfEventsId, SetOfEvents>& pEvents,
    const ResearchContext& pContext,
    FactsAlreadyChecked& pFactsAlreadychecked)
{
  auto& actions = pContext.domain.actions();
  for (const auto& currActionId : pActionSuccessions)
  {
    if (pContext.actionIds.count(currActionId) == 0)
      continue;

    auto itAction = actions.find(currActionId);
    if (itAction != actions.end())
    {
      auto cpParentParameters = pParentParameters;
      ParameterValuesWithConstraints cpTmpParameters;
      if (pTmpParentParametersPtr != nullptr)
        cpTmpParameters = *pTmpParentParametersPtr;

      auto& action = itAction->second;
      auto* newTreePtr = pTreeOfAlreadyDonePath.getNextActionTreeIfNotAnExistingLeaf(currActionId);
      auto newRes = PossibleEffect::NOT_SATISFIED;
      if (newTreePtr != nullptr && action.precondition)
      {
        const auto& precondition = *action.precondition;
        newRes = _lookForAPossibleDeduction(*newTreePtr, action.parameters, precondition,
                                            action.effect.worldStateModification,
                                            action.effect.potentialWorldStateModification,
                                            pFactOptional, cpParentParameters, &cpTmpParameters,
                                            pContext, pFactsAlreadychecked, currActionId);
        res = _merge(newRes, res);
      }

      if (newRes == PossibleEffect::SATISFIED &&
          _updatePossibleParameters(newPossibleParentParameters, newPossibleTmpParentParameters,
                                    pParentParameters, cpParentParameters, pTmpParentParametersPtr,
                                    pDataRelatedToOptimisation, cpTmpParameters, false))
        return;
    }
  }

  for (const auto& currSetOfEventsSucc : pEventSuccessions)
  {
    auto itSetOfEvents = pEvents.find(currSetOfEventsSucc.first);
    if (itSetOfEvents != pEvents.end())
    {
      for (const auto& currEventIdSucc : currSetOfEventsSucc.second)
      {
        const auto& currInfrences = itSetOfEvents->second.events();
        auto itEvent = currInfrences.find(currEventIdSucc);
        if (itEvent != currInfrences.end())
        {
          auto& event = itEvent->second;
          if (event.factsToModify)
          {
            auto fullEventId = generateFullEventId(currSetOfEventsSucc.first, currEventIdSucc);
            if (pContext.fullEventIds.count(fullEventId) == 0)
              continue;

            auto cpParentParameters = pParentParameters;
            ParameterValuesWithConstraints cpTmpParameters;
            if (pTmpParentParametersPtr != nullptr)
              cpTmpParameters = *pTmpParentParametersPtr;

            auto* newTreePtr = pTreeOfAlreadyDonePath.getNextInflectionTreeIfNotAnExistingLeaf(currEventIdSucc);
            auto newRes = PossibleEffect::NOT_SATISFIED;
            if (newTreePtr != nullptr && event.precondition)
            {
              const auto& precondition = *event.precondition;
              newRes = _lookForAPossibleDeduction(*newTreePtr, event.parameters, precondition,
                                                  event.factsToModify, {}, pFactOptional,
                                                  cpParentParameters, &cpTmpParameters,
                                                  pContext, pFactsAlreadychecked, fullEventId);
              res = _merge(newRes, res);
            }

            if (newRes == PossibleEffect::SATISFIED)
            {
              if (_updatePossibleParameters(newPossibleParentParameters, newPossibleTmpParentParameters,
                                            pParentParameters, cpParentParameters, pTmpParentParametersPtr,
                                            pDataRelatedToOptimisation, cpTmpParameters, true))
                return;
            }
          }
        }
      }
    }
  }
}



bool _doesConditionMatchAnOptionalFact(const ParameterValuesWithConstraints& pParameters,
                                       const FactOptional& pFactOptional,
                                       const ParameterValuesWithConstraints* pParametersToModifyInPlacePtr,
                                       const ResearchContext& pContext)
{
  const auto& ontology = pContext.domain.getOntology();
  const auto& objective = pContext.goal.objective();
  return objective.findConditionCandidateFromFactFromEffect(
        [&](const FactOptional& pConditionFactOptional, const Fact*, const Fact*)
  {
    if (!pConditionFactOptional.fact.hasAParameter() && pContext.problem.worldState.isOptionalFactSatisfied(pConditionFactOptional))
      return false;

    bool res = pConditionFactOptional.fact.areEqualExceptAnyEntities(pFactOptional.fact, &pParameters, pParametersToModifyInPlacePtr);
    if (pConditionFactOptional.isFactNegated != pFactOptional.isFactNegated)
      res = !res;
    return res;
  }, pContext.problem.worldState, ontology.constants, pContext.problem.objects, pFactOptional.fact, pParameters, pParametersToModifyInPlacePtr, {});
}


bool _doesStatisfyTheGoal(ParameterValuesWithConstraints& pParameters,
                          const std::unique_ptr<ogp::WorldStateModification>& pWorldStateModificationPtr1,
                          const std::unique_ptr<ogp::WorldStateModification>& pWorldStateModificationPtr2,
                          const ResearchContext& pContext,
                          const std::string& pFromDeductionId)
{
  auto checkObjectiveCallback = [&](const FactOptional& pFactOptional,
      ParameterValuesWithConstraints* pParametersToModifyInPlacePtr,
      const std::function<bool (const ParameterValuesWithConstraints&)>& pCheckValidity) -> bool
  {
    if (_doesConditionMatchAnOptionalFact(pParameters, pFactOptional, pParametersToModifyInPlacePtr, pContext))
    {
      if (pParameters.empty() && pParametersToModifyInPlacePtr == nullptr)
        return true;

      const auto& objective = pContext.goal.objective();
      if (_checkConditionAndFillParameters(objective, pFactOptional, pParameters, pParametersToModifyInPlacePtr,
                                           pContext, _emptyParameters) == PossibleEffect::SATISFIED)
      {
        if (pParametersToModifyInPlacePtr != nullptr &&  !pCheckValidity(*pParametersToModifyInPlacePtr))
          return false;
        return true;
      }
      return false;
    }
    return false;
  };

  const auto& constants = pContext.domain.getOntology().constants;
  const auto& objects = pContext.problem.objects;
  if (pWorldStateModificationPtr1 &&
      pWorldStateModificationPtr1->canSatisfyObjective(checkObjectiveCallback, pParameters, pContext.problem.worldState, pFromDeductionId, constants, objects))
    return true;
  if (pWorldStateModificationPtr2 &&
      pWorldStateModificationPtr2->canSatisfyObjective(checkObjectiveCallback, pParameters, pContext.problem.worldState, pFromDeductionId, constants, objects))
    return true;
  return false;
}


bool _lookForAPossibleEffect(ParameterValuesWithConstraints& pParametersWithTmpData,
                             DataRelatedToOptimisation& pDataRelatedToOptimisation,
                             TreeOfAlreadyDonePath& pTreeOfAlreadyDonePath,
                             const std::unique_ptr<ogp::WorldStateModification>& pWorldStateModificationPtr1,
                             const std::unique_ptr<ogp::WorldStateModification>& pWorldStateModificationPtr2,
                             const ResearchContext& pContext,
                             FactsAlreadyChecked& pFactsAlreadychecked,
                             const std::string& pFromDeductionId)
{
  bool canSatisfyThisGoal = pContext.goal.canDeductionSatisfyThisGoal(pFromDeductionId);
  if (canSatisfyThisGoal &&
      pContext.goal.isASimpleFactObjective())
  {
    if (_doesStatisfyTheGoal(pParametersWithTmpData, pWorldStateModificationPtr1, pWorldStateModificationPtr2,
                             pContext, pFromDeductionId))
      return true;
    canSatisfyThisGoal = false;
  }

  // Iterate on possible successions
  auto& setOfEvents = pContext.domain.getSetOfEvents();
  auto successionsCallback = [&](const Successions& pSuccessions,
                                 const ogp::FactOptional& pFactOptional,
                                 ParameterValuesWithConstraints* pParametersToModifyInPlacePtr,
                                 const std::function<bool (const ParameterValuesWithConstraints&)>& pCheckValidity) {
    auto possibleEffect = PossibleEffect::NOT_SATISFIED;
    ParameterValuesWithConstraints newPossibleParentParameters;
    ParameterValuesWithConstraints newPossibleTmpParentParameters;
    bool checkActionAndEvents = true;

    if (canSatisfyThisGoal &&
        _doesConditionMatchAnOptionalFact(pParametersWithTmpData, pFactOptional, pParametersToModifyInPlacePtr, pContext))
    {
      if (pParametersWithTmpData.empty() && pParametersToModifyInPlacePtr == nullptr)
        return true;

      auto cpParentParameters = pParametersWithTmpData;
      ParameterValuesWithConstraints cpTmpParameters;
      if (pParametersToModifyInPlacePtr != nullptr)
        cpTmpParameters = *pParametersToModifyInPlacePtr;

      const auto& objective = pContext.goal.objective();
      possibleEffect = _checkConditionAndFillParameters(objective, pFactOptional, cpParentParameters, &cpTmpParameters,
                                                        pContext, _emptyParameters);
      if (possibleEffect == PossibleEffect::SATISFIED &&
          _updatePossibleParameters(newPossibleParentParameters, newPossibleTmpParentParameters,
                                    pParametersWithTmpData, cpParentParameters, pParametersToModifyInPlacePtr,
                                    pDataRelatedToOptimisation, cpTmpParameters, false))
        checkActionAndEvents = false;
    }

    if (checkActionAndEvents &&
        (!pSuccessions.actions.empty() || !pSuccessions.events.empty()) &&
        ((!pFactOptional.isFactNegated && pFactsAlreadychecked.factsToAdd.count(pFactOptional.fact) == 0) ||
         (pFactOptional.isFactNegated && pFactsAlreadychecked.factsToRemove.count(pFactOptional.fact) == 0)))
    {
      FactsAlreadyChecked subFactsAlreadychecked = pFactsAlreadychecked;
      if (!pFactOptional.isFactNegated)
        subFactsAlreadychecked.factsToAdd.insert(pFactOptional.fact);
      else
        subFactsAlreadychecked.factsToRemove.insert(pFactOptional.fact);

      _lookForAPossibleExistingOrNotFactFromActionsAndEvents(
            possibleEffect, newPossibleParentParameters, newPossibleTmpParentParameters,
            pSuccessions.actions, pSuccessions.events, pFactOptional, pParametersWithTmpData, pParametersToModifyInPlacePtr,
            pDataRelatedToOptimisation, pTreeOfAlreadyDonePath,
            setOfEvents, pContext, subFactsAlreadychecked);

      if (possibleEffect != PossibleEffect::SATISFIED_BUT_DOES_NOT_MODIFY_THE_WORLD)
        pFactsAlreadychecked.swap(subFactsAlreadychecked);
    }

    if (!newPossibleParentParameters.empty())
    {
      pParametersWithTmpData = std::move(newPossibleParentParameters);
      if (pParametersToModifyInPlacePtr != nullptr)
        *pParametersToModifyInPlacePtr = std::move(newPossibleTmpParentParameters);
    }

    if (possibleEffect == PossibleEffect::SATISFIED && pParametersToModifyInPlacePtr != nullptr && !pCheckValidity(*pParametersToModifyInPlacePtr))
      possibleEffect = PossibleEffect::NOT_SATISFIED;
    return possibleEffect == PossibleEffect::SATISFIED;
    };

  const auto& constants = pContext.domain.getOntology().constants;
  const auto& objects = pContext.problem.objects;
  if (pWorldStateModificationPtr1)
    if (pWorldStateModificationPtr1->iterateOnSuccessions(successionsCallback, pParametersWithTmpData, pContext.problem.worldState, canSatisfyThisGoal, pFromDeductionId, constants, objects))
      return true;
  if (pWorldStateModificationPtr2)
    if (pWorldStateModificationPtr2->iterateOnSuccessions(successionsCallback, pParametersWithTmpData, pContext.problem.worldState, canSatisfyThisGoal, pFromDeductionId, constants, objects))
      return true;
  return false;
}


PlanCost _extractPlanCost(
    Problem& pProblem,
    const Domain& pDomain,
    const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
    Historical* pGlobalHistorical,
    LookForAnActionOutputInfos& pLookForAnActionOutputInfos,
    const ActionPtrWithGoal* pPreviousActionPtr)
{
  PlanCost res;
  if (pPreviousActionPtr != nullptr && pPreviousActionPtr->actionPtr != nullptr)
  {
    res.costForFirstGoal += pPreviousActionPtr->actionPtr->duration;
    res.totalCost += pPreviousActionPtr->actionPtr->duration;
  }

  std::set<std::string> actionAlreadyInPlan;
  bool shouldBreak = false;
  while (!pProblem.goalStack.goals().empty())
  {
    if (shouldBreak)
    {
      res.success = false;
      break;
    }

    const SetOfCallbacks callbacks;
    auto subPlan = _planForMoreImportantGoalPossible(pProblem, pDomain, callbacks, false,
                                                     pNow, pGlobalHistorical, &pLookForAnActionOutputInfos, pPreviousActionPtr);
    if (subPlan.empty())
      break;
    const auto& actions = pDomain.getActions();
    for (const auto& currActionInSubPlan : subPlan)
    {
      auto itAction = actions.find(currActionInSubPlan.actionInvocation.actionId);
      if (itAction != actions.end())
      {
        const auto& actionCost = itAction->second.duration;
        if (pLookForAnActionOutputInfos.nbOfNotSatisfiedGoals() == 0 && pLookForAnActionOutputInfos.nbOfSatisfiedGoals() == 0)
          res.costForFirstGoal += actionCost;
        res.totalCost += actionCost;
      }

      const auto& actionToDoStr = currActionInSubPlan.actionInvocation.toStr();
      if (actionAlreadyInPlan.count(actionToDoStr) > 0)
        shouldBreak = true;
      actionAlreadyInPlan.insert(actionToDoStr);
      bool goalChanged = false;
      updateProblemForNextPotentialPlannerResult(pProblem, goalChanged, currActionInSubPlan, pDomain, pNow, pGlobalHistorical,
                                                 &pLookForAnActionOutputInfos);
      if (goalChanged)
        break;
    }
  }

  res.success = pLookForAnActionOutputInfos.isFirstGoalInSuccess();
  res.nbOfGoalsNotSatisfied = pLookForAnActionOutputInfos.nbOfNotSatisfiedGoals();
  res.nbOfGoalsSatisfied = pLookForAnActionOutputInfos.nbOfSatisfiedGoals();
  return res;
}


bool _isMoreOptimalNextAction(
    std::optional<PotentialNextActionComparisonCache>& pPotentialNextActionComparisonCacheOpt,
    bool& pNextInPlanCanBeAnEvent,
    const ActionInvocationWithPtr& pNewPotentialNextAction,
    const std::optional<ActionInvocationWithPtr>& pCurrentNextAction,
    const Problem& pProblem,
    const Domain& pDomain,
    const DataRelatedToOptimisation& pDataRelatedToOptimisation,
    std::size_t pLength,
    const Goal& pCurrentGoal,
    const Historical* pGlobalHistorical)
{
  if (pDataRelatedToOptimisation.tryToDoMoreOptimalSolution &&
      pLength == 0 &&
      pCurrentNextAction &&
      pCurrentNextAction->actionPtr != nullptr &&
      pNewPotentialNextAction.actionPtr != nullptr &&
      (pNewPotentialNextAction.actionPtr->effect != pCurrentNextAction->actionPtr->effect ||
       pNewPotentialNextAction.actionInvocation.parameters != pCurrentNextAction->actionInvocation.parameters))
  {
    auto& currentNextAction = *pCurrentNextAction;
    ActionInvocationWithGoal oneStepOfPlannerResult1(pNewPotentialNextAction.actionInvocation.actionId, pNewPotentialNextAction.actionInvocation.parameters, {}, 0);
    ActionInvocationWithGoal oneStepOfPlannerResult2(currentNextAction.actionInvocation.actionId, currentNextAction.actionInvocation.parameters, {}, 0);
    std::unique_ptr<std::chrono::steady_clock::time_point> now;

    PlanCost newCost;
    bool nextStepIsAnEvent = pNewPotentialNextAction.nextStepIsAnEvent(pDataRelatedToOptimisation.parameterToEntitiesFromEvent);
    {
      auto localProblem1 = pProblem;
      bool goalChanged = false;
      LookForAnActionOutputInfos lookForAnActionOutputInfos;
      updateProblemForNextPotentialPlannerResult(localProblem1, goalChanged, oneStepOfPlannerResult1, pDomain, now, nullptr, &lookForAnActionOutputInfos);
      ActionPtrWithGoal actionPtrWithGoal(pNewPotentialNextAction.actionPtr, pCurrentGoal);
      auto* actionPtrWithGoalPtr = nextStepIsAnEvent ? nullptr : &actionPtrWithGoal;
      newCost = _extractPlanCost(localProblem1, pDomain, now, nullptr, lookForAnActionOutputInfos, actionPtrWithGoalPtr);
    }

    if (!pPotentialNextActionComparisonCacheOpt)
    {
      auto localProblem2 = pProblem;
      bool goalChanged = false;
      LookForAnActionOutputInfos lookForAnActionOutputInfos;
      updateProblemForNextPotentialPlannerResult(localProblem2, goalChanged, oneStepOfPlannerResult2, pDomain, now, nullptr, &lookForAnActionOutputInfos);
      pPotentialNextActionComparisonCacheOpt = PotentialNextActionComparisonCache();
      ActionPtrWithGoal actionPtrWithGoal(currentNextAction.actionPtr, pCurrentGoal);
      bool nextStepIsAnEventForCurrentAction = currentNextAction.nextStepIsAnEvent(pDataRelatedToOptimisation.parameterToEntitiesFromEvent);
      auto* actionPtrWithGoalPtr = nextStepIsAnEventForCurrentAction ? nullptr : &actionPtrWithGoal;
      pPotentialNextActionComparisonCacheOpt->currentCost = _extractPlanCost(localProblem2, pDomain, now, nullptr, lookForAnActionOutputInfos, actionPtrWithGoalPtr);
    }

    if (newCost.isBetterThan(pPotentialNextActionComparisonCacheOpt->currentCost))
    {
      pPotentialNextActionComparisonCacheOpt->currentCost = newCost;
      pPotentialNextActionComparisonCacheOpt->effectsWithWorseCosts.push_back(&currentNextAction.actionPtr->effect);
      pNextInPlanCanBeAnEvent = nextStepIsAnEvent;
      return true;
    }
    if (pPotentialNextActionComparisonCacheOpt->currentCost.isBetterThan(newCost))
    {
      pPotentialNextActionComparisonCacheOpt->effectsWithWorseCosts.push_back(&pNewPotentialNextAction.actionPtr->effect);
      return false;
    }
  }

  bool res = true;
  if (pCurrentNextAction)
    res = pNewPotentialNextAction.isMoreImportantThan(*pCurrentNextAction, pProblem, pGlobalHistorical);
  if (res)
  {
    pNextInPlanCanBeAnEvent = pNewPotentialNextAction.nextStepIsAnEvent(pDataRelatedToOptimisation.parameterToEntitiesFromEvent);
    return true;
  }
  return false;
}


ActionId _findFirstActionForAGoal(
    std::map<Parameter, Entity>& pParameters,
    bool& pNextInPlanCanBeAnEvent,
    TreeOfAlreadyDonePath& pTreeOfAlreadyDonePath,
    const Goal& pGoal,
    const Problem& pProblem,
    const Domain& pDomain,
    bool pTryToDoMoreOptimalSolution,
    std::size_t pLength,
    const Historical* pGlobalHistorical,
    const ActionPtrWithGoal* pPreviousActionPtr)
{
  std::optional<ActionInvocationWithPtr> res;
  std::set<ActionId> actionIdsToSkip;
  if (pPreviousActionPtr != nullptr &&
      pPreviousActionPtr->goal.objective() == pGoal.objective() &&
      pPreviousActionPtr->actionPtr != nullptr)
    actionIdsToSkip = pPreviousActionPtr->actionPtr->actionsSuccessionsWithoutInterestCache;
  std::optional<PotentialNextActionComparisonCache> potentialNextActionComparisonCacheOpt;

  ResearchContext context(pGoal, pProblem, pDomain,
                          pGoal.getActionsPredecessors(), pGoal.getEventsPredecessors());

  auto& ontology = pDomain.getOntology();
  auto& domainActions = pDomain.actions();
  for (const ActionId& currActionId : context.actionIds)
  {
    if (actionIdsToSkip.count(currActionId) > 0)
      continue;

    auto itAction = domainActions.find(currActionId);
    if (itAction != domainActions.end())
    {
      const Action& action = itAction->second;
      if (!action.canThisActionBeUsedByThePlanner)
        continue;
      auto* newTreePtr = pTreeOfAlreadyDonePath.getNextActionTreeIfNotAnExistingLeaf(currActionId);
      if (newTreePtr != nullptr) // To skip leaf of already seen path
      {
        FactsAlreadyChecked factsAlreadyChecked;
        auto newPotRes = PotentialNextAction(currActionId, action);
        DataRelatedToOptimisation dataRelatedToOptimisation;
        dataRelatedToOptimisation.tryToDoMoreOptimalSolution = pTryToDoMoreOptimalSolution;
        if (_lookForAPossibleEffect(newPotRes.parameters, dataRelatedToOptimisation, *newTreePtr,
                                    action.effect.worldStateModification, action.effect.potentialWorldStateModification,
                                    context, factsAlreadyChecked, currActionId) &&
            (!action.precondition || action.precondition->isTrue(pProblem.worldState, ontology.constants, pProblem.objects, {}, {}, {}, &newPotRes.parameters)))
        {
          const auto& constants = pDomain.getOntology().constants;
          auto actionInvocations = newPotRes.toActionInvocations(constants, pProblem.objects);
          for (auto& currActionInvocation : actionInvocations)
          {
            if (_isMoreOptimalNextAction(potentialNextActionComparisonCacheOpt, pNextInPlanCanBeAnEvent,
                                         currActionInvocation, res, pProblem, pDomain,
                                         dataRelatedToOptimisation, pLength, pGoal, pGlobalHistorical))
              res.emplace(currActionInvocation);
          }
        }
      }
    }
  }
  if (res)
  {
    pParameters = std::move(res->actionInvocation.parameters);
    return res->actionInvocation.actionId;
  }
  return "";
}


bool _goalToPlanRec(
    std::list<ActionInvocationWithGoal>& pActionInvocations,
    Problem& pProblem,
    std::map<std::string, std::size_t>& pActionAlreadyInPlan,
    const Domain& pDomain,
    bool pTryToDoMoreOptimalSolution,
    const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
    const Historical* pGlobalHistorical,
    const Goal& pGoal,
    int pPriority,
    const ActionPtrWithGoal* pPreviousActionPtr)
{
  TreeOfAlreadyDonePath treeOfAlreadyDonePath;

  std::unique_ptr<ActionInvocationWithGoal> potentialRes;
  bool nextInPlanCanBeAnEvent = false;
  {
    std::map<Parameter, Entity> parameters;
    auto actionId =
        _findFirstActionForAGoal(parameters, nextInPlanCanBeAnEvent, treeOfAlreadyDonePath, pGoal, pProblem,
                                 pDomain, pTryToDoMoreOptimalSolution, 0,
                                 pGlobalHistorical, pPreviousActionPtr);
    if (!actionId.empty())
      potentialRes = std::make_unique<ActionInvocationWithGoal>(actionId, parameters, pGoal.clone(), pPriority);
  }

  if (potentialRes && potentialRes->fromGoal)
  {
    const auto& actionToDoStr = potentialRes->actionInvocation.toStr();
    auto itAlreadyFoundAction = pActionAlreadyInPlan.find(actionToDoStr);
    if (itAlreadyFoundAction == pActionAlreadyInPlan.end())
    {
      pActionAlreadyInPlan[actionToDoStr] = 1;
    }
    else
    {
      if (itAlreadyFoundAction->second > 1)
        return false;
      ++itAlreadyFoundAction->second;
    }

    auto problemForPlanCost = pProblem;
    bool goalChanged = false;

    auto* potActionPtr = pDomain.getActionPtr(potentialRes->actionInvocation.actionId);
    if (potActionPtr != nullptr)
    {
      const auto& ontology = pDomain.getOntology();
      updateProblemForNextPotentialPlannerResultWithAction(problemForPlanCost, goalChanged,
                                                           *potentialRes, *potActionPtr,
                                                           pDomain, pNow, nullptr, nullptr);
      ActionPtrWithGoal previousAction(potActionPtr, pGoal);
      auto* previousActionPtr = nextInPlanCanBeAnEvent ? nullptr : &previousAction;
      if (problemForPlanCost.worldState.isGoalSatisfied(pGoal, ontology.constants, problemForPlanCost.objects) ||
          _goalToPlanRec(pActionInvocations, problemForPlanCost, pActionAlreadyInPlan,
                         pDomain, pTryToDoMoreOptimalSolution, pNow, nullptr, pGoal, pPriority, previousActionPtr))
      {
        potentialRes->fromGoal->notifyActivity();
        pActionInvocations.emplace_front(std::move(*potentialRes));
        return true;
      }
    }
  }
  else
  {
    return false; // Fail to find an next action to do
  }
  return false;
}

std::list<ActionInvocationWithGoal> _planForMoreImportantGoalPossible(Problem& pProblem,
                                                                     const Domain& pDomain,
                                                                     const SetOfCallbacks& pCallbacks,
                                                                     bool pTryToDoMoreOptimalSolution,
                                                                     const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                                                                     const Historical* pGlobalHistorical,
                                                                     LookForAnActionOutputInfos* pLookForAnActionOutputInfosPtr,
                                                                     const ActionPtrWithGoal* pPreviousActionPtr)
{
  const auto& ontology = pDomain.getOntology();
  std::list<ActionInvocationWithGoal> res;
  pProblem.goalStack.refreshIfNeeded(pDomain);
  pProblem.goalStack.iterateOnGoalsAndRemoveNonPersistent(
        [&](const Goal& pGoal, int pPriority){

            if (pProblem.goalStack.effectBeweenGoals)
            {
              bool goalChanged = false;
              pProblem.worldState.applyEffect({}, pProblem.goalStack.effectBeweenGoals, goalChanged, pProblem.goalStack,
                                              pDomain.getSetOfEvents(), pCallbacks, pDomain.getOntology(), pProblem.objects, pNow);
            }

            std::map<std::string, std::size_t> actionAlreadyInPlan;
            return _goalToPlanRec(res, pProblem, actionAlreadyInPlan,
                                  pDomain, pTryToDoMoreOptimalSolution, pNow, pGlobalHistorical, pGoal, pPriority,
                                  pPreviousActionPtr);
          },
        pProblem.worldState, ontology.constants, pProblem.objects, pNow,
        pLookForAnActionOutputInfosPtr);
  return res;
}

std::list<std::string> _goalsToPddls(const std::list<Goal>& pGoals)
{
  std::list<std::string> res;
  for (const auto& currGoal : pGoals)
    res.emplace_back(currGoal.toPddl(0));
  return res;
}

}


std::list<ActionInvocationWithGoal> planForMoreImportantGoalPossible(Problem& pProblem,
                                                                     const Domain& pDomain,
                                                                     const SetOfCallbacks& pCallbacks,
                                                                     bool pTryToDoMoreOptimalSolution,
                                                                     const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                                                                     const Historical* pGlobalHistorical,
                                                                     LookForAnActionOutputInfos* pLookForAnActionOutputInfosPtr)
{
  return _planForMoreImportantGoalPossible(pProblem, pDomain, pCallbacks, pTryToDoMoreOptimalSolution, pNow,
                                           pGlobalHistorical, pLookForAnActionOutputInfosPtr, nullptr);
}


ActionsToDoInParallel actionsToDoInParallelNow(
    Problem& pProblem,
    const Domain& pDomain,
    const SetOfCallbacks& pCallbacks,
    const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
    Historical* pGlobalHistorical)
{
  pProblem.goalStack.refreshIfNeeded(pDomain);
  std::list<Goal> goalsDone;
  auto problemForPlanResolution = pProblem;
  auto sequentialPlan = planForEveryGoals(problemForPlanResolution, pDomain, pCallbacks,
                                          pNow, pGlobalHistorical, &goalsDone);
  auto parallelPlan = toParallelPlan(sequentialPlan, true, pProblem, pDomain, goalsDone, pNow);
  if (!parallelPlan.actionsToDoInParallel.empty())
    return parallelPlan.actionsToDoInParallel.front();
  return {};
}


void notifyActionStarted(Problem& pProblem,
                         const Domain& pDomain,
                         const SetOfCallbacks& pCallbacks,
                         const ActionInvocationWithGoal& pActionInvocationWithGoal,
                         const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow)
{
  const auto& actions = pDomain.actions();
  auto itAction = actions.find(pActionInvocationWithGoal.actionInvocation.actionId);
  if (itAction != actions.end())
  {
    if (itAction->second.effect.worldStateModificationAtStart)
    {
      auto worldStateModificationAtStart = itAction->second.effect.worldStateModificationAtStart->clone(&pActionInvocationWithGoal.actionInvocation.parameters);
      auto& setOfEvents = pDomain.getSetOfEvents();
      const auto& ontology = pDomain.getOntology();
      if (worldStateModificationAtStart)
        pProblem.worldState.modify(&*worldStateModificationAtStart, pProblem.goalStack, setOfEvents,
                                   pCallbacks, ontology, pProblem.objects, pNow);
    }
  }
}


bool notifyActionDone(Problem& pProblem,
                      const Domain& pDomain,
                      const SetOfCallbacks& pCallbacks,
                      const ActionInvocationWithGoal& pOnStepOfPlannerResult,
                      const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                      LookForAnActionOutputInfos* pLookForAnActionOutputInfosPtr)
{
  const auto& actions = pDomain.actions();
  auto itAction = actions.find(pOnStepOfPlannerResult.actionInvocation.actionId);
  if (itAction != actions.end())
  {
    auto& setOfEvents = pDomain.getSetOfEvents();
    bool goalChanged = false;
    const auto& ontology = pDomain.getOntology();
    notifyActionInvocationDone(pProblem, goalChanged, setOfEvents, pCallbacks, pOnStepOfPlannerResult,
                               itAction->second.effect.worldStateModification, ontology, pNow,
                               &itAction->second.effect.goalsToAdd, &itAction->second.effect.goalsToAddInCurrentPriority,
                               pLookForAnActionOutputInfosPtr);
    return true;
  }
  return false;
}



std::list<ActionInvocationWithGoal> planForEveryGoals(
    Problem& pProblem,
    const Domain& pDomain,
    const SetOfCallbacks& pCallbacks,
    const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
    Historical* pGlobalHistorical,
    std::list<Goal>* pGoalsDonePtr)
{
  const bool tryToDoMoreOptimalSolution = true;
  std::map<std::string, std::size_t> actionAlreadyInPlan;
  std::list<ActionInvocationWithGoal> res;
  LookForAnActionOutputInfos lookForAnActionOutputInfos;
  while (!pProblem.goalStack.goals().empty())
  {
    auto subPlan = _planForMoreImportantGoalPossible(pProblem, pDomain, pCallbacks, tryToDoMoreOptimalSolution,
                                                     pNow, pGlobalHistorical, &lookForAnActionOutputInfos, nullptr);
    if (subPlan.empty())
      break;
    for (auto& currActionInSubPlan : subPlan)
    {
      const auto& actionToDoStr = currActionInSubPlan.actionInvocation.toStr();
      auto itAlreadyFoundAction = actionAlreadyInPlan.find(actionToDoStr);
      if (itAlreadyFoundAction == actionAlreadyInPlan.end())
      {
        actionAlreadyInPlan[actionToDoStr] = 1;
      }
      else
      {
        if (itAlreadyFoundAction->second > 10)
          break;
        ++itAlreadyFoundAction->second;
      }
      bool goalChanged = false;
      updateProblemForNextPotentialPlannerResult(pProblem, goalChanged, currActionInSubPlan, pDomain, pNow, pGlobalHistorical,
                                                 &lookForAnActionOutputInfos);
      res.emplace_back(std::move(currActionInSubPlan));
      if (goalChanged)
        break;
    }
  }
  if (pGoalsDonePtr != nullptr)
    lookForAnActionOutputInfos.moveGoalsDone(*pGoalsDonePtr);
  return res;
}


ParallelPan parallelPlanForEveryGoals(
    Problem& pProblem,
    const Domain& pDomain,
    const SetOfCallbacks& pCallbacks,
    const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
    Historical* pGlobalHistorical)
{
  pProblem.goalStack.refreshIfNeeded(pDomain);

  std::list<Goal> goalsDone;
  auto problemForPlanResolution = pProblem;
  auto sequentialPlan = planForEveryGoals(problemForPlanResolution, pDomain, pCallbacks, pNow, pGlobalHistorical, &goalsDone);
  return toParallelPlan(sequentialPlan, false, pProblem, pDomain, goalsDone, pNow);
}



std::string planToStr(const std::list<ActionInvocationWithGoal>& pPlan,
                      const std::string& pSep)
{
  std::string res;
  bool firstIteration = true;
  for (const auto& currAction : pPlan)
  {
    if (firstIteration)
      firstIteration = false;
    else
      res += pSep;
    res += currAction.actionInvocation.toStr();
  }
  return res;
}


std::string parallelPlanToStr(const ParallelPan& pPlan)
{
  std::string res;
  for (const auto& currAcctionsInParallel : pPlan.actionsToDoInParallel)
  {
    if (!res.empty())
      res += "\n";
    bool firstIteration = true;
    for (const auto& currAction : currAcctionsInParallel.actions)
    {
      if (firstIteration)
        firstIteration = false;
      else
        res += ", ";
      res += currAction.actionInvocation.toStr();
    }
  }
  return res;
}

std::string planToPddl(const std::list<ActionInvocationWithGoal>& pPlan,
                       const Domain& pDomain)
{
  std::size_t step = 0;
  std::stringstream ss;
  for (const auto& currActionInvocationWithGoal : pPlan)
  {
    ss << std::setw(2) << std::setfill('0') << step << ": ";
    ++step;
    ss << currActionInvocationWithGoal.actionInvocation.toPddl(pDomain) << "\n";
  }
  return ss.str();
}


std::string parallelPlanToPddl(const ParallelPan& pPlan,
                               const Domain& pDomain)
{
  std::size_t step = 0;
  std::string res;
  for (const auto& currActionsToDoInParallel : pPlan.actionsToDoInParallel)
  {
    std::stringstream ssBeginOfStep;
    ssBeginOfStep << std::setw(2) << std::setfill('0') << step << ": ";
    ++step;
    for (const auto& currActionInvocationWithGoal : currActionsToDoInParallel.actions)
      res += ssBeginOfStep.str() + currActionInvocationWithGoal.actionInvocation.toPddl(pDomain) + "\n";
  }
  return res;
}



std::string goalsToStr(const std::list<Goal>& pGoals,
                       const std::string& pSep)
{
  auto size = pGoals.size();
  if (size == 1)
    return pGoals.front().toStr();
  std::string res;
  bool firstIteration = true;
  for (const auto& currGoal : pGoals)
  {
    if (firstIteration)
      firstIteration = false;
    else
      res += pSep;
    res += currGoal.toStr();
  }
  return res;
}


bool evaluate
(ParallelPan& pPlan,
 Problem& pProblem,
 const Domain& pDomain)
{
  std::list<std::list<ActionDataForParallelisation>> planWithCache;
  const auto& ontology = pDomain.getOntology();
  const auto& actions = pDomain.actions();

  for (auto& currActionsInParallel : pPlan.actionsToDoInParallel)
  {
    std::list<ActionDataForParallelisation> actionInASubList;
    for (auto& currAction : currActionsInParallel.actions)
    {
      auto itAction = actions.find(currAction.actionInvocation.actionId);
      if (itAction == actions.end())
        throw std::runtime_error("ActionId \"" + currAction.actionInvocation.actionId + "\" not found in algorithm to manaage parralelisation");
      actionInASubList.emplace_back(itAction->second, std::move(currAction));
    }
    if (!actionInASubList.empty())
      planWithCache.emplace_back(std::move(actionInASubList));
  }

  auto expectedGoalsSatisfied = pPlan.extractSatisiedGoals();

  std::unique_ptr<std::chrono::steady_clock::time_point> now;
  pProblem.goalStack.refreshIfNeeded(pDomain);
  pProblem.goalStack.removeFirstGoalsThatAreAlreadySatisfied(pProblem.worldState, ontology.constants, pProblem.objects, now);
  auto itBegin = planWithCache.begin();
  auto goals = extractSatisfiedGoals(pProblem, pDomain, itBegin, planWithCache, nullptr, now);
  auto goalsPddls = _goalsToPddls(goals);
  return goalsPddls == expectedGoalsSatisfied;
}


} // !ogp
