#include "plannerusingexternaldata.hpp"
#include <orderedgoalsplanner/util/util.hpp>

std::string PlannerUsingExternalData::dataPath = "";

// Arguments to set: --dataPath ../../doc/examples/


int main(int argc, char **argv)
{
  for (int i = 0; i < argc; ++i)
  {
    if ((i + 1) < argc)
    {
      const std::string currAgrv = argv[i];
      if (currAgrv == "--dataPath")
        PlannerUsingExternalData::dataPath = argv[i + 1];
    }
  }

  if (PlannerUsingExternalData::dataPath.empty())
    throw std::runtime_error("--dataPath option is missing. (For ex do: --dataPath ../../doc/examples/)");

  ogp::ORDEREDGOALSPLANNER_DEBUG_FOR_TESTS = true;
  ::testing::InitGoogleTest(&argc, argv);
  int res = RUN_ALL_TESTS();
  return res;
}
