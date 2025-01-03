#ifndef INCLUDE_ORDEREDGOALSPLANNER_UTIL_SERIALIZER_DESERIALIZEFROMPDDL_HPP
#define INCLUDE_ORDEREDGOALSPLANNER_UTIL_SERIALIZER_DESERIALIZEFROMPDDL_HPP

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace ogp
{
struct Condition;
struct Domain;
struct Entity;
struct FactOptional;
struct Goal;
struct Ontology;
struct Parameter;
struct SetOfEntities;
struct WorldStateModification;
struct Problem;
struct SetOfTypes;


struct DomainAndProblemPtrs
{
  std::unique_ptr<Domain> domainPtr;
  std::unique_ptr<Problem> problemPtr;
};

Domain pddlToDomain(const std::string& pStr,
                    const std::map<std::string, Domain>& pPreviousDomains);

DomainAndProblemPtrs pddlToProblem(const std::string& pStr,
                                   const std::map<std::string, Domain>& pPreviousDomains);


std::unique_ptr<Condition> pddlToCondition(const std::string& pStr,
                                           std::size_t& pPos,
                                           const Ontology& pOntology,
                                           const SetOfEntities& pObjects,
                                           const std::vector<Parameter>& pParameters,
                                           const std::map<std::string, Entity>* pParameterNamesToEntityPtr = nullptr);

std::unique_ptr<Condition> strToCondition(const std::string& pStr,
                                          const Ontology& pOntology,
                                          const SetOfEntities& pObjects,
                                          const std::vector<Parameter>& pParameters,
                                          const std::map<std::string, Entity>* pParameterNamesToEntityPtr = nullptr);

std::unique_ptr<Goal> pddlToGoal(const std::string& pStr,
                                 std::size_t& pPos,
                                 const Ontology& pOntology,
                                 const SetOfEntities& pObjects,
                                 int pMaxTimeToKeepInactive = -1,
                                 const std::string& pGoalGroupId = "",
                                 const std::map<std::string, Entity>* pParameterNamesToEntityPtr = nullptr);

std::unique_ptr<Goal> strToGoal(const std::string& pStr,
                                const Ontology& pOntology,
                                const SetOfEntities& pObjects,
                                int pMaxTimeToKeepInactive = -1,
                                const std::string& pGoalGroupId = "",
                                const std::map<std::string, Entity>* pParameterNamesToEntityPtr = nullptr);

std::unique_ptr<WorldStateModification> pddlToWsModification(const std::string& pStr,
                                                             std::size_t& pPos,
                                                             const Ontology& pOntology,
                                                             const SetOfEntities& pObjects,
                                                             const std::vector<Parameter>& pParameters);


std::unique_ptr<WorldStateModification> strToWsModification(const std::string& pStr,
                                                             const Ontology& pOntology,
                                                             const SetOfEntities& pObjects,
                                                             const std::vector<Parameter>& pParameters);


std::vector<Parameter> pddlToParameters(const std::string& pStr,
                                        const SetOfTypes& pSetOfTypes);


FactOptional pddlToFactOptional(const std::string& pStr,
                                const Ontology& pOntology,
                                const SetOfEntities& pObjects,
                                const std::vector<Parameter>& pParameters = {},
                                const std::map<std::string, Entity>* pParameterNamesToEntityPtr = nullptr);

} // End of namespace ogp



#endif // INCLUDE_ORDEREDGOALSPLANNER_UTIL_SERIALIZER_DESERIALIZEFROMPDDL_HPP
