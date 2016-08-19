#include <iostream>
#include <vector>
#include <string>
#include <unordered_set>


constexpr const char* wordlistName="english_big.txt";

std::unordered_set<std::string> LoadWordList(std::string filename)
{
    std::unordered_set<std::string> retVal;

    return retVal;
}


int main()
{
    std::string cryptogramText;
    std::cin >> cryptogramText;

    std::unordered_set<std::string> wordList = LoadWordList(wordlistName);


}

