#include <orderedgoalsplanner/types/parallelplan.hpp>

namespace ogp
{


std::list<std::string> ParallelPan::extractSatisiedGoals() const
{
   std::list<std::string> res;
   for (auto& currActionsInParallel : actionsToDoInParallel)
   {
     for (auto& currAction : currActionsInParallel.actions)
     {
       if (currAction.fromGoal)
       {
         auto goalStr = currAction.fromGoal->toPddl(0);
         if (currAction.fromGoal &&
             (res.empty() || res.back() != goalStr))
           res.push_back(goalStr);
       }
     }
   }
   return res;
}


std::size_t ParallelPan::cost() const
{
  return actionsToDoInParallel.size();
}


} // !ogp
