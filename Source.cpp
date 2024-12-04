#include <iostream>
#include <regex>
#include <string>
#include <set>
#include <map>
#include <filesystem>
#include <fstream>
#include <clang-c/Index.h>

namespace fs = std::filesystem;

// Function to detect APIs from a code snippet
std::map<std::string, std::set<std::string>> detectAPIs(const std::string& codeSnippet) {
    std::regex functionCallPattern(R"(\b(\w+)\s*\()");
    std::regex includePattern(R"(#include\s*<(.*)>)");

    std::set<std::string> functionCalls;
    std::set<std::string> includes;

    // Find function calls
    auto functionCallStart = std::sregex_iterator(codeSnippet.begin(), codeSnippet.end(), functionCallPattern);
    auto functionCallEnd = std::sregex_iterator();
    for (auto it = functionCallStart; it != functionCallEnd; ++it) {
        functionCalls.insert((*it)[1].str());
    }

    // Find included libraries
    auto includeStart = std::sregex_iterator(codeSnippet.begin(), codeSnippet.end(), includePattern);
    auto includeEnd = std::sregex_iterator();
    for (auto it = includeStart; it != includeEnd; ++it) {
        includes.insert((*it)[1].str());
    }

    std::map<std::string, std::set<std::string>> detectedAPIs = {
        {"Standard Libraries", {}},
        {"OS-Specific APIs", {}}
    };

    // Classify includes
    for (const auto& include : includes) {
        if (include.find("llvm") == 0 || include == "clang-c" || include == "windows.h" || include == "pthread.h") {
            detectedAPIs["OS-Specific APIs"].insert(include);
        }
        else {
            detectedAPIs["Standard Libraries"].insert(include);
        }
    }

    // Classify function calls
    for (const auto& func : functionCalls) {
        if (func.find("LLVM") != std::string::npos ||
            func.find("clang") != std::string::npos ||
            func == "MessageBox" ||
            func == "pthread_create" ||
            func == "pthread_join") {
            detectedAPIs["OS-Specific APIs"].insert(func);
        }
        else {
            detectedAPIs["Standard Libraries"].insert(func);
        }
    }

    return detectedAPIs;
}

// Recursively traverse through all the text files and one file at a time
void processFile(const fs::path& filePath, std::ofstream& outputFile) {
    std::ifstream inputFile(filePath);
    if (!inputFile) {
        std::cerr << "Error: Could not open file " << filePath << std::endl;
        return;
    }

    std::string codeSnippet((std::istreambuf_iterator<char>(inputFile)),
        std::istreambuf_iterator<char>());
    inputFile.close();

    auto apis = detectAPIs(codeSnippet);

    outputFile << "File: " << filePath << std::endl;
    for (const auto& category : apis) {
        outputFile << category.first << ":" << std::endl;
        for (const auto& item : category.second) {
            outputFile << "  - " << item << std::endl;
        }
    }
    outputFile << std::endl;
}

// Main function
int main() {
    std::string folderPath;

    // Specify the path of the folder
    std::cout << "Enter the path to the folder containing .txt files with C/C++ code:" << std::endl;
    std::getline(std::cin, folderPath);

    // Open output file
    std::ofstream outputFile("detected_apis.txt");
    if (!outputFile) {
        std::cerr << "Error: Could not open file for writing." << std::endl;
        return 1;
    }

    // Check if all the files are .txt files
    try {
        for (const auto& entry : fs::recursive_directory_iterator(folderPath)) {
            if (entry.is_regular_file() && (entry.path().extension() == ".c"||entry.path()==".cpp"||entry.path()==".txt")) {
                processFile(entry.path(), outputFile);
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    outputFile.close();
    std::cout << "Output has been written into detected_apis.txt." << std::endl;

    // Use Clang to get and display the Clang version
    CXString version = clang_getClangVersion();
    std::cout << "Clang Version: " << clang_getCString(version) << std::endl;
    clang_disposeString(version);

    return 0;
}
