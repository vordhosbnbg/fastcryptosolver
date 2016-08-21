#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <thread>
#include <map>
#include <future>
#include <random>
#include <sstream>


using Solution = std::pair<const std::string, const std::string>;
using SolutionMap = std::multimap<double, Solution>;

constexpr const char* wordlistName = "../wordlist/google-10000-english-usa.txt";
constexpr int ALPHABET_LETTERS_NUM = 26;
constexpr int GOOD_SOLUTION_NUM = 1000;
constexpr double SOLUTION_QUALITY = 0.7;

std::random_device rd;
std::default_random_engine e1(rd());

template<int NumLetters>
std::string transformText(const std::string& sourceText, const std::string& substitutionKey)
{
    std::string retVal;
    retVal = sourceText;
    if (substitutionKey.size() == NumLetters)
    {
        for (int index = 0; index < sourceText.size(); ++index)
        {
            const char& chr = sourceText.at(index);
            char& chrOut = retVal.at(index);
            if (chr >= 'A' && chr <= 'Z')
            {
                chrOut = substitutionKey.c_str()[chr - 'A'];
            }
        }
    }
    return retVal;
}

void loadWordListIntoSet(const std::string& filename, std::unordered_set<std::string>& outSet)
{
    std::cout << "Start loading wordlist from file: " << std::endl << filename << std::endl;
    auto tpBegin = std::chrono::system_clock::now();
    std::ifstream wordlistFile(filename);
    outSet.reserve(2000000);
    if (wordlistFile.is_open())
    {
        std::string line;
        // get length of file:
        wordlistFile.seekg(0, wordlistFile.end);
        std::streamoff fileSize = wordlistFile.tellg();
        wordlistFile.seekg(0, wordlistFile.beg);

        while (std::getline(wordlistFile, line))
        {
            std::transform(line.begin(), line.end(), line.begin(), ::toupper);
            outSet.insert(std::move(line));
        }
        auto tpEnd = std::chrono::system_clock::now();
        auto millisecondsElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(tpEnd - tpBegin).count();

        double secondsElapsed = millisecondsElapsed / 1000.0;

        std::cout << "Wordlist loaded in " << std::setprecision(3) << std::fixed << secondsElapsed << " s" << std::endl;
        std::cout << "Word count: " << outSet.size() << " words" << std::endl;
        std::cout << "Speed: " << std::setprecision(3) << fileSize / secondsElapsed / 1024 / 1024 << " MB/s" << std::endl;

    }
    else
    {
        std::cout << "Error opening file" << std::endl;
    }
}

template<int NumLetters>
std::string MutateKey(const std::string& sourceKey)
{
    std::string retVal = sourceKey;
    std::uniform_int_distribution<unsigned int> dist(0, NumLetters - 1);
    unsigned int rnd = dist(e1);
    unsigned int rndNew = dist(e1);
    char& chrToMutate = retVal.at(rnd);
    char newVal = 'A' + rndNew;
    retVal.at(retVal.find(newVal)) = chrToMutate;
    chrToMutate = newVal;
    return retVal;
}

std::vector<std::string> splitLineToWords(const std::string& line) 
{
    std::vector<std::string> retVal;
    //std::stringstream ss(line);
    size_t strPos = 0;
    size_t oldPos = 0;
    while (strPos != std::string::npos)
    {
        strPos = line.find_first_of(' ', oldPos);
        if (strPos != std::string::npos)
        {
            retVal.emplace_back(line.substr(oldPos, strPos - oldPos));
            oldPos = strPos + 1;
        }
    }
    //handle last word
    retVal.emplace_back(line.substr(oldPos, line.size() - oldPos));

    return retVal;
}

double calcTextQuality(const std::string& text, std::unordered_set<std::string>& wordList)
{
    double retVal;
    auto vecWords = splitLineToWords(text);
    size_t numWords = vecWords.size();
    int good = 0;
    for (auto word : vecWords)
    {
        if (wordList.find(word) != wordList.end())
        {
            ++good;
            //std::cout << "Found word: " << word << std::endl;
        }
    }
    retVal = (double)good / (double)vecWords.size();
    return retVal;
}

void addOneBetterSolution(SolutionMap& sMap, std::mutex& mapMutex, const std::string& initialKey, const std::string& cryptoText, std::unordered_set<std::string>& wordList)
{
    bool added = false;
    std::string newKey = initialKey;
    std::string decryptedText = transformText<ALPHABET_LETTERS_NUM>(cryptoText, initialKey);
    const double minQuality = calcTextQuality(decryptedText, wordList);
    while (!added)
    {
        newKey = MutateKey<ALPHABET_LETTERS_NUM>(newKey);
        decryptedText = transformText<ALPHABET_LETTERS_NUM>(cryptoText, newKey);
        double quality = calcTextQuality(decryptedText, wordList);
        //std::cout << decryptedText << std::endl;
        if (quality > minQuality)
        {
            auto pair = std::make_pair(newKey, decryptedText);
            mapMutex.lock();
            sMap.insert(std::make_pair(quality, pair));
            mapMutex.unlock();
            std::cout << "New better solution (Q: " << std::setprecision(4) << std::fixed << quality << " ): " << decryptedText << std::endl;
            added = true;
        }
    }
}

void removeRandomMember(std::unordered_set<std::string>& outSet) 
{
    std::uniform_int_distribution<size_t> dist(0, outSet.size() - 1);
    size_t rnd = dist(e1);

    size_t cntIter = 0;
    for (auto it = outSet.begin(); it != outSet.end(); ++it) 
    {
        if (cntIter == rnd) 
        {
            outSet.erase(it);
            break;
        }
        cntIter++;
    }
}
template<int NumLetters>
void addRandomKey(std::unordered_set<std::string>& outSet) 
{
    std::string newKey;
    std::uniform_int_distribution<int> randomLetter('A', 'Z');
    while (newKey.size() < NumLetters)
    {
        char chr = static_cast<char>(randomLetter(e1));
        if (newKey.find(chr) == std::string::npos)
        {
            newKey.push_back(chr);
        }
    }
    outSet.insert(std::move(newKey));
}

template<int NumLetters>
std::unordered_set<std::string> getBestKeys(const SolutionMap& sMap, int numberOfKeys)
{
    std::unordered_set<std::string> retVal;

    for (auto it = sMap.rbegin(); it != sMap.rend(); ++it)
    {
        retVal.insert(it->second.first);
        if (retVal.size() >= numberOfKeys) // we gathered numberOfKeys
        {
            auto itNext = it;
            ++itNext;
            if (itNext != sMap.rend()) // we are not at the last place (first)
            {
                if (itNext->first == it->first) // if the next Solution is with the same quality we add it too
                {
                    continue;
                }
            }
            break;
        }
    }

    while (retVal.size() > numberOfKeys)
    {
        removeRandomMember(retVal);
    }

    while (retVal.size() < numberOfKeys)
    {
        addRandomKey<NumLetters>(retVal);
    }

    return retVal;
}



int main()
{
    std::string cryptogramText;
    std::cout << "Enter cryptogram:" << std::endl;
    std::getline(std::cin, cryptogramText);
    std::transform(cryptogramText.begin(), cryptogramText.end(), cryptogramText.begin(), ::toupper);
    std::unordered_set<std::string> wordList;
    loadWordListIntoSet(wordlistName, wordList);
    if (wordList.size() > 0)
    {
        unsigned maxThreads = std::thread::hardware_concurrency();
        SolutionMap solutionMap;
        std::uniform_int_distribution<unsigned int> dist(0, maxThreads - 1);


        while (solutionMap.size() < GOOD_SOLUTION_NUM)
        {
            std::vector<std::thread> vecThreads;
            std::unordered_set<std::string> bestKeys = getBestKeys<ALPHABET_LETTERS_NUM>(solutionMap, maxThreads);
            std::unordered_set<std::string> usedKeys;
            auto itBestKeys = bestKeys.begin();
            auto itKeyToPass = bestKeys.begin();
            std::mutex solutionMapLocker;
            for (unsigned int thread = 0; thread < maxThreads; ++thread)
            {
                if (itBestKeys == bestKeys.end()) // exhausted the best keys
                {
                    unsigned int rnd = dist(e1);

                    itBestKeys = bestKeys.begin();
                    int cntWalk = 0;
                    for (itKeyToPass = bestKeys.begin(); itKeyToPass != bestKeys.end(); ++itKeyToPass) 
                    {
                        if (cntWalk == rnd)
                        {
                            break;
                        }
                        cntWalk++;
                    }
                }
                else 
                {
                    itKeyToPass = itBestKeys;
                }
                const std::string& keyToPass = *itKeyToPass;
                vecThreads.push_back(std::thread(addOneBetterSolution, solutionMap, std::ref(solutionMapLocker), keyToPass, cryptogramText, wordList));
                ++itKeyToPass;
            }

            for (auto& thr : vecThreads) 
            {
                thr.join();
            }
            
        }
    }
}

