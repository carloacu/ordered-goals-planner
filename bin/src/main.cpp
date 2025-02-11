#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <orderedgoalsplanner/types/domain.hpp>
#include <orderedgoalsplanner/types/problem.hpp>
#include <orderedgoalsplanner/types/setofcallbacks.hpp>
#include <orderedgoalsplanner/util/serializer/deserializefrompddl.hpp>
#include <orderedgoalsplanner/util/serializer/serializeinpddl.hpp>
#include <orderedgoalsplanner/orderedgoalsplanner.hpp>

using namespace ogp;

namespace
{
std::string _getFileContent(const std::string& filePath)
{
  std::ifstream file(filePath);
  if (!file.is_open()) {
    throw std::runtime_error("Error: Could not open file " + filePath);
  }

  std::string res;
  std::string line;
  while (std::getline(file, line))
    res += line + "\n";

  file.close();
  return res;
}

void printUsage() {
  std::cout << "Usage: orderedgoalsplanner -d <domain_file> -p <problem_file> [--verbose] [-o <output_plan_file>]" << std::endl;
  std::cout << "or" << std::endl;
  std::cout << "Usage: orderedgoalsplanner <domain_file> <problem_file> [--verbose] [-o <output_plan_file>]" << std::endl;
  std::cout << "or" << std::endl;
  std::cout << "Usage: orderedgoalsplanner --dp <directory_with_domain_pddl_and_problem_pddl_files_in_it> [--verbose] [-o <output_plan_file>]" << std::endl;
}

}


int main(int argc, char* argv[])
{
  std::string domain_file;
  std::string problem_file;
  std::string domain_and_problem_directory;
  std::string output_plan_file;
  bool verbose = false;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    if (arg == "-d" && i + 1 < argc) {
      domain_file = argv[++i];
    } else if (arg == "-p" && i + 1 < argc) {
      problem_file = argv[++i];
    } else if (arg == "--dp" && i + 1 < argc) {
      domain_and_problem_directory = argv[++i];
    } else if (arg == "-o" && i + 1 < argc) {
      output_plan_file = argv[++i];
    } else if (arg == "--verbose") {
      verbose = true;
    } else if (domain_file == "") {
      domain_file = arg;
    } else if (problem_file == "") {
      problem_file = arg;
    } else {
      printUsage();
      return 1;
    }
  }

  if (!domain_and_problem_directory.empty()) {
    domain_file = domain_and_problem_directory + "/domain.pddl";
    problem_file = domain_and_problem_directory + "/problem.pddl";
  }
  else if (domain_file.empty()) {
    std::cerr << "Error: Missing domain file in arguments.\n";
    printUsage();
    return 1;
  }
  else if (problem_file.empty()) {
    std::cerr << "Error: Missing problem file in arguments.\n";
    printUsage();
    return 1;
  }

  auto domainContent = _getFileContent(domain_file);
  std::map<std::string, ogp::Domain> loadedDomains;
  auto domain = ogp::pddlToDomain(domainContent, loadedDomains);
  loadedDomains.emplace(domain.getName(), std::move(domain));
  if (verbose)
    std::cout << "Parsing domain file \"" << domain_file << "\" done successfully." << std::endl;

  auto problemContent = _getFileContent(problem_file);
  ogp::DomainAndProblemPtrs domainAndProblemPtrs = ogp::pddlToProblemFromDomains(problemContent, loadedDomains);
  auto& problem = *domainAndProblemPtrs.problemPtr;
  if (verbose)
    std::cout << "Parsing problem file \"" << problem_file << "\" done successfully." << std::endl;

  if (verbose)
  {
    std::cout << "\n" << std::endl;
    std::cout << "Successions" << std::endl;
    std::cout << "===========\n" << std::endl;
    std::cout << domain.printSuccessionCache() << std::endl;
    std::cout << "\n" << std::endl;
  }

  if (verbose)
    std::cout << "Searching for a plan..." << std::endl;
  std::string planStr = ogp::planToPddl(ogp::planForEveryGoals(problem, domain, {}, {}), domain);


  if (planStr == "")
  {
    std::cerr << "No plan found." << std::endl;
    return 1;
  }


  if (verbose)
  {
    std::cout << "Solution found.\n" << std::endl;

    std::cout << "Plan" << std::endl;
    std::cout << "====  " << std::endl;
    std::cout << planStr << std::endl;
  }

  if (!output_plan_file.empty())
  {

    std::ofstream file(output_plan_file);
    if (!file.is_open()) {
      std::cerr << "Error: Could not open the file " << output_plan_file << std::endl;
      return 1;
    }

    std::filesystem::path filePath = problem_file;
    std::string problemFileName = filePath.filename().string();
    file << "; Plan generated by orderedgoalsplanner.\n";
    file << "; Problem file: " << problemFileName << "\n\n";
    file << planStr;
    file.close();
    if (verbose)
      std::cout << "Output Plan wrote in " << output_plan_file << std::endl;
  }
  else if (!verbose)
  {
    std::cout << planStr << std::endl;
  }

  return 0;
}
