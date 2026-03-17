#ifndef INCLUDE_ORDEREDGOALSPLANNER_EXTRACTMINMAXVALUEFORFACTS_HPP
#define INCLUDE_ORDEREDGOALSPLANNER_EXTRACTMINMAXVALUEFORFACTS_HPP

#include <orderedgoalsplanner/types/minmaxvalues.hpp>
#include <map>
#include <string>
#include "api.hpp"

namespace ogp
{
struct Domain;
struct Problem;


ORDEREDGOALSPLANNER_API
std::map<std::string, MinMaxValues> extractMinMaxValuesForFacts(const Problem& pProblem,
                                                                const Domain& pDomain);


}

#endif // INCLUDE_ORDEREDGOALSPLANNER_EXTRACTMINMAXVALUEFORFACTS_HPP
