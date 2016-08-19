#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_set>
#include <chrono>
#include <iomanip>


constexpr const char* wordlistName="../wordlist/english_big.txt";

std::unordered_set<std::string> LoadWordList(std::string filename)
{
    std::unordered_set<std::string> retVal;
    std::cout << "Start loading wordlist from file: " << std::endl << filename << std::endl;
    auto tpBegin = std::chrono::system_clock::now();
    std::ifstream wordlistFile(filename);
    if(wordlistFile.is_open())
    {
        std::string line;
        while(std::getline(wordlistFile, line))
        {
            retVal.insert(line);
        }
        auto tpEnd = std::chrono::system_clock::now();

        auto millisecondsElapsed =  std::chrono::duration_cast<std::chrono::milliseconds>(tpEnd - tpBegin).count();

        double secondsElapsed = millisecondsElapsed / 1000.0;

        std::cout << "Wordlist loaded in " << std::setprecision(3) << std::fixed << secondsElapsed << " s" <<std::endl;
    }
    else
    {
        std::cout << "Error opening file" << std::endl;
    }
    return retVal;
}


int main()
{
    std::string cryptogramText;
    std::cin >> cryptogramText;

    std::unordered_set<std::string> wordList = LoadWordList(wordlistName);
    std::cout << "Loaded " << wordList.size() << " words" << std::endl;

}

