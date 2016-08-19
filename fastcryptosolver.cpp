#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_set>
#include <chrono>
#include <iomanip>


constexpr const char* wordlistName="../wordlist/english_huge.txt";

void LoadWordListIntoMap(const std::string filename, std::unordered_set<std::string>& outMap)
{
    std::cout << "Start loading wordlist from file: " << std::endl << filename << std::endl;
    auto tpBegin = std::chrono::system_clock::now();
    std::ifstream wordlistFile(filename);
    outMap.reserve(2000000);
    if(wordlistFile.is_open())
    {
        std::string line;
        // get length of file:
        wordlistFile.seekg (0, wordlistFile.end);
        int fileSize = wordlistFile.tellg();
        wordlistFile.seekg (0, wordlistFile.beg);

        while(std::getline(wordlistFile, line))
        {
            outMap.insert(std::move(line));
        }
        auto tpEnd = std::chrono::system_clock::now();
        auto millisecondsElapsed =  std::chrono::duration_cast<std::chrono::milliseconds>(tpEnd - tpBegin).count();

        double secondsElapsed = millisecondsElapsed / 1000.0;

        std::cout << "Wordlist loaded in " << std::setprecision(3) << std::fixed << secondsElapsed << " s" <<std::endl;
        std::cout << "Word count: " << outMap.size() << " words" << std::endl;
        std::cout << "Speed: " << std::setprecision(3) << fileSize / secondsElapsed / 1024 / 1024 << " MB/s" << std::endl;

    }
    else
    {
        std::cout << "Error opening file" << std::endl;
    }
}


int main()
{
    std::string cryptogramText;
    std::cout << "Enter cryptogram:" << std::endl;
    std::cin >> cryptogramText;

    std::unordered_set<std::string> wordList;
    LoadWordListIntoMap(wordlistName, wordList);

}

