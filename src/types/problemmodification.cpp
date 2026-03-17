#include <orderedgoalsplanner/types/problemmodification.hpp>
#include <orderedgoalsplanner/util/util.hpp>


namespace ogp
{

bool ProblemModification::operator==(const ProblemModification& pOther) const
{
  return areUPtrEqual(worldStateModification, pOther.worldStateModification) &&
      areUPtrEqual(potentialWorldStateModification, pOther.potentialWorldStateModification) &&
      areUPtrEqual(worldStateModificationAtStart, pOther.worldStateModificationAtStart) &&
      goalsToAdd == pOther.goalsToAdd &&
      goalsToAddInCurrentPriority == pOther.goalsToAddInCurrentPriority;
}


std::set<FactOptional> ProblemModification::getAllOptFactsThatCanBeModified() const
{
  std::set<FactOptional> res;
  if (worldStateModification)
  {
    worldStateModification->forAllThatCanBeModified([&](const FactOptionalAndValueModification& pFactOptional) {
        res.insert(pFactOptional.factOpt);
        return ContinueOrBreak::CONTINUE;
    });
  }
  if (potentialWorldStateModification)
  {
    potentialWorldStateModification->forAllThatCanBeModified([&](const FactOptionalAndValueModification& pFactOptional) {
        res.insert(pFactOptional.factOpt);
        return ContinueOrBreak::CONTINUE;
    });
  }
  return res;
}


std::set<FactOptionalAndValueModification> ProblemModification::getAllOptFactsThatCanBeModified2() const
{
  std::set<FactOptionalAndValueModification> res;
  if (worldStateModification)
  {
    worldStateModification->forAllThatCanBeModified([&](const FactOptionalAndValueModification& pFactOptional) {
        res.insert(pFactOptional);
        return ContinueOrBreak::CONTINUE;
    });
  }
  if (potentialWorldStateModification)
  {
    potentialWorldStateModification->forAllThatCanBeModified([&](const FactOptionalAndValueModification& pFactOptional) {
        res.insert(pFactOptional);
        return ContinueOrBreak::CONTINUE;
    });
  }
  return res;
}


} // !ogp
