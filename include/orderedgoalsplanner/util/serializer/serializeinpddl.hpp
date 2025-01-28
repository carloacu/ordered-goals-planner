#ifndef INCLUDE_ORDEREDGOALSPLANNER_UTIL_SERIALIZER_SERIALIZEINPDDL_HPP
#define INCLUDE_ORDEREDGOALSPLANNER_UTIL_SERIALIZER_SERIALIZEINPDDL_HPP

#include <list>
#include <string>

namespace ogp
{
struct Condition;
struct Domain;
struct Problem;
struct WorldStateModification;


std::string domainToPddl(const Domain& pDomain);

std::string problemToPddl(const Problem& pProblem,
                          const Domain& pDomain);


std::string conditionToPddl(const Condition& pCondition,
                            std::size_t pIdentation,
                            bool pPrintAnyValue = true);

std::string effectToPddl(const WorldStateModification& pWsModif,
                         std::size_t pIdentation,
                         bool pActuallyItIsACondition = false,
                         bool pPrintAnyValue = true);

} // End of namespace ogp



#endif // INCLUDE_ORDEREDGOALSPLANNER_UTIL_SERIALIZER_SERIALIZEINPDDL_HPP
