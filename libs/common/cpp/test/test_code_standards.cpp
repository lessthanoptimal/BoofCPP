#include "gtest/gtest.h"
#include "testing_utils.h"

#include <filesystem>
#include <fstream>
#include <regex>
#include <set>
#include <sstream>

namespace fs = std::filesystem;

// Checks the C++ tests of every library against CodeStandards.md. BOOFCPP_LIBS_DIR is set by CMake

class CodeStandardsTest : public boofcv::BoofCPPTest {};

/// Every test uses TEST_F with a fixture derived from BoofCPPTest, declared in the same file
TEST_F(CodeStandardsTest, tests_use_standard_fixture) {
    const std::regex plain_test(R"(^\s*TEST\s*\()");
    const std::regex test_f(R"(^\s*TEST_F\s*\(\s*(\w+)\s*,)");
    const std::regex fixture(R"(class\s+(\w+)\s*:\s*public\s+(boofcv::)?BoofCPPTest\b)");

    uint32_t files = 0;
    for (const auto& library : fs::directory_iterator(BOOFCPP_LIBS_DIR)) {
        fs::path test_dir = library.path() / "cpp" / "test";
        if (!fs::is_directory(test_dir))
            continue;
        for (const auto& entry : fs::directory_iterator(test_dir)) {
            if (entry.path().extension() != ".cpp")
                continue;
            files++;
            std::ifstream in(entry.path());
            std::stringstream text;
            text << in.rdbuf();
            const std::string source = text.str();

            std::set<std::string> fixtures;
            for (std::sregex_iterator it(source.begin(), source.end(), fixture), end; it != end; ++it)
                fixtures.insert((*it)[1]);

            std::istringstream lines(source);
            std::string line;
            for (uint32_t number = 1; std::getline(lines, line); number++) {
                std::smatch match;
                EXPECT_FALSE(std::regex_search(line, plain_test))
                    << entry.path() << ":" << number << " uses TEST instead of TEST_F";
                if (std::regex_search(line, match, test_f)) {
                    EXPECT_TRUE(fixtures.count(match[1]) == 1)
                        << entry.path() << ":" << number << " fixture " << match[1] << " doesn't extend BoofCPPTest";
                }
            }
        }
    }
    EXPECT_GT(files, 0u);
}
