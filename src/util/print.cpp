#include <orderedgoalsplanner/util/print.hpp>
#include <sstream>
#include <iomanip>
#include <orderedgoalsplanner/orderedgoalsplanner.hpp>


namespace ogp
{

namespace
{

std::string _prettyPrintTime(int pNbOfSeconds)
{
  std::stringstream ss;
  if (pNbOfSeconds < 60)
  {
    ss << pNbOfSeconds << "s";
  }
  else
  {
    int nbOfMinutes = pNbOfSeconds / 60;
    ss << nbOfMinutes << "min";
    int nbOfSeconds = pNbOfSeconds % 60;
    if (nbOfSeconds > 0)
      ss << " " << nbOfSeconds << "s";
  }
  return ss.str();
}


void _limiteSizeStr(std::string& pStr, std::size_t pMaxSize)
{
  if (pStr.size() > pMaxSize && pMaxSize > 3)
  {
    pStr.resize(pMaxSize - 3);
    pStr += "...";
  }
  pStr.resize(pMaxSize, ' ');
}

}


std::string printGoals(const std::map<int, std::vector<Goal>>& pGoals)
{
  std::string res;
  for (auto itGoalsGroup = pGoals.end(); itGoalsGroup != pGoals.begin(); )
  {
    --itGoalsGroup;
    for (auto& currGoal : itGoalsGroup->second)
    {
      if (!res.empty())
        res += ", ";
      res += currGoal.toStr();
    }
  }
  return res;
}


std::string printGoalsTable(std::size_t pGoalNameMaxSize,
                            const std::map<int, std::vector<Goal>>& pGoals,
                            const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow)
{
  std::stringstream res;

  std::string nameLabel = "Name";
  _limiteSizeStr(nameLabel, pGoalNameMaxSize);
  res << nameLabel;

  std::size_t prioritySize = 12;
  res << std::setw(prioritySize) << std::setfill(' ') << "Priority";

  std::size_t stackTimeSize = 0;
  std::size_t maxStackTimeSize = 0;
  if (pNow)
  {
    stackTimeSize = 13;
    res << std::setw(stackTimeSize) << std::setfill(' ') << "Stack time";

    maxStackTimeSize = 17;
    res << std::setw(maxStackTimeSize) << std::setfill(' ') << "Max stack time";
  }
  res << "\n";

  std::string separator(pGoalNameMaxSize + prioritySize + stackTimeSize + maxStackTimeSize, '-');
  res << separator << "\n";

  for (auto itGoalsGroup = pGoals.end(); itGoalsGroup != pGoals.begin(); )
  {
    --itGoalsGroup;
    for (auto& currGoal : itGoalsGroup->second)
    {
      std::string goalName = currGoal.toStr();
      auto& goalGroupId = currGoal.getGoalGroupId();
      if (!goalGroupId.empty())
        goalName += " groupId: " + goalGroupId;
      _limiteSizeStr(goalName, pGoalNameMaxSize);

      res << goalName;
      res << std::setw(prioritySize) << std::setfill(' ') << itGoalsGroup->first;

      if (pNow)
      {
        if (currGoal.getInactiveSince())
          res << std::setw(stackTimeSize) << std::setfill(' ') << _prettyPrintTime(std::chrono::duration_cast<std::chrono::seconds>(*pNow - *currGoal.getInactiveSince()).count());
        else
          res << std::setw(stackTimeSize) << std::setfill(' ') << "-";

        if (currGoal.getMaxTimeToKeepInactive() > 0)
          res << std::setw(maxStackTimeSize) << std::setfill(' ') << _prettyPrintTime(currGoal.getMaxTimeToKeepInactive());
        else
          res << std::setw(maxStackTimeSize) << std::setfill(' ') << "-";
      }

      res << "\n";
    }
  }

  return res.str();
}


}
