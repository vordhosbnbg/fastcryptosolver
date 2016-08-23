#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_set>
#include <set>
#include <unordered_map>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <thread>
#include <map>
#include <future>
#include <random>
#include <sstream>
#include <string.h>
#include "MurmurHash3.h"



constexpr const char* wordlistName = "../wordlist/google-10000-english-usa.txt";
constexpr unsigned int ALPHABET_LETTERS_NUM = 26;
constexpr unsigned int GOOD_SOLUTION_NUM = 1000;
constexpr double SOLUTION_QUALITY = 0.7;
constexpr unsigned int maxTextLength = 50;

std::random_device rd;
std::default_random_engine e1(rd());

template<int maxLen>
class FixedString
{
    static constexpr int arrSize = maxLen + 1;
public:
    FixedString() : currentLen(0)
    {
        memset(array, 0, arrSize);
    }

    FixedString(const std::string& original) : currentLen(original.size())
    {
        memset(array, 0, arrSize);
        memcpy(array, original.c_str(), original.size());
    }

    FixedString(const char * cStr)
    {
        memset(array, 0, arrSize);
        if(cStr)
        {
            currentLen = strnlen(cStr, maxLen);
            memcpy(array, cStr, currentLen);
        }
        else
        {
            currentLen = 0;
        }
    }
    ~FixedString(){}

    const char * c_str() const
    {
        return array;
    }

    const char& at(size_t offset) const
    {
        return array[offset];
    }

    char& at(size_t offset)
    {
        return array[offset];
    }

    size_t size() const
    {
        return currentLen;
    }

    struct hasher
    {
        size_t operator()( const FixedString<maxLen>& objToHash ) const
        {
            size_t retVal;
            MurmurHash3_x86_32(&objToHash, sizeof(FixedString<maxLen>), 0xDEADBEEF, &retVal);
            return retVal;
        }
    };

    bool operator==(const FixedString<maxLen>& other) const
    {
        return (memcmp(array, other.array, currentLen) == 0);
    }

    void operator=(const FixedString<maxLen> &other )
    {
        memcpy(array, other.array, arrSize);
        currentLen = other.currentLen;
    }

    size_t find_first_of(const char val, size_t offset = 0) const
    {
        size_t retVal = npos;
        if(offset <= currentLen)
        {
            for(size_t index = offset; index < currentLen; ++index)
            {
                if(val == array[index])
                {
                    retVal = index;
                    break;
                }
            }
        }
        return retVal;
    }

    FixedString<maxLen> substr(size_t begin, size_t end) const
    {
        FixedString<maxLen> retVal;
        memcpy(retVal.array, &array[begin], end - begin);
        return retVal;
    }

    static const size_t npos = static_cast<size_t>(-1);

private:
    size_t currentLen;
    char array[arrSize];
};



//using Word = std::string;
//using WordList = std::unordered_set<std::string>;
using Word = FixedString<maxTextLength>;
using WordList = std::unordered_set<Word, Word::hasher >;
using CryptoText = FixedString<maxTextLength>;
using CryptoKey = FixedString<ALPHABET_LETTERS_NUM>;
using CryptoKeySet = std::unordered_set<CryptoKey, CryptoKey::hasher>;
using Solution = std::pair<const CryptoKey, const CryptoText>;
using SolutionMap = std::multimap<double, Solution>;



template<int NumLetters>
CryptoText transformText(const CryptoText& sourceText, const CryptoKey& substitutionKey)
{
    CryptoText retVal;
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

void loadWordListIntoSet(const std::string& filename, WordList& outSet)
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
            outSet.insert(line);
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
CryptoKey MutateKey(const CryptoKey& sourceKey, std::set<char>& goodPos)
{
    CryptoKey retVal = sourceKey;
    std::uniform_int_distribution<unsigned int> dist(0, NumLetters - 1);
    unsigned int rnd;
    do
    {
        rnd = dist(e1);
    } while ((goodPos.find(rnd) != goodPos.end()) && goodPos.size() != ALPHABET_LETTERS_NUM);
    unsigned int rndNew = dist(e1);
    char& chrToMutate = retVal.at(rnd);
    char newVal = 'A' + rndNew;
    retVal.at(retVal.find_first_of(newVal)) = chrToMutate;
    chrToMutate = newVal;
    return retVal;
}

std::vector<Word> splitLineToWords(const CryptoText& line)
{
    std::vector<Word> retVal;
    size_t strPos = 0;
    size_t oldPos = 0;
    while (strPos != CryptoText::npos)
    {
        strPos = line.find_first_of(' ', oldPos);
        if (strPos != CryptoText::npos)
        {
            retVal.emplace_back(line.substr(oldPos, strPos - oldPos));
            oldPos = strPos + 1;
        }
    }
    //handle last word
    retVal.emplace_back(line.substr(oldPos, line.size() - oldPos));

    return retVal;
}

double calcTextQuality(const CryptoText& text, const WordList& wordList, std::set<char>& goodPos)
{
    double retVal;
    auto vecWords = splitLineToWords(text);
    size_t numWords = vecWords.size();
    size_t allTextLength = 0;
    size_t goodTextLength = 0;
    int good = 0;
    for (const auto& word : vecWords)
    {
        allTextLength += word.size();
        if (wordList.find(word) != wordList.end())
        {
            goodTextLength += word.size();
            ++good;
            for(auto index = 0; index < word.size(); ++index)
            {
                char chr = word.at(index);
                goodPos.insert(chr);
            }
        }
    }
    retVal = (double)goodTextLength / (double)allTextLength;
    return retVal;
}

void addOneBetterSolution(SolutionMap& sMap, std::mutex& mapMutex, const CryptoKey& initialKey, const CryptoText& cryptoText, WordList& wordList)
{
    bool added = false;
    CryptoKey newKey = initialKey;
    CryptoText decryptedText = transformText<ALPHABET_LETTERS_NUM>(cryptoText, initialKey);
    std::set<char> goodPositions;
    const double minQuality = calcTextQuality(decryptedText, wordList, goodPositions);
    while (!added)
    {
        newKey = MutateKey<ALPHABET_LETTERS_NUM>(newKey, goodPositions);
        decryptedText = transformText<ALPHABET_LETTERS_NUM>(cryptoText, newKey);
        double quality = calcTextQuality(decryptedText, wordList, goodPositions);
        //std::cout << decryptedText << std::endl;
        if (quality > minQuality)
        {
            auto pair = std::make_pair(newKey, decryptedText);
            mapMutex.lock();
            sMap.insert(std::make_pair(quality, pair));
            mapMutex.unlock();
            std::cout << "New better solution (Q: " << std::setprecision(4) << std::fixed << quality << " ): " << decryptedText.c_str() << std::endl;
            added = true;
        }
    }
}

void removeRandomMember(CryptoKeySet& outSet)
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
void addRandomKey(CryptoKeySet& outSet)
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
CryptoKeySet getBestKeys(const SolutionMap& sMap, int numberOfKeys)
{
    CryptoKeySet retVal;

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



int main(int argc, char *argv[])
{
    WordList wordList;
    loadWordListIntoSet(wordlistName, wordList);
    std::string cryptogramText;
    std::cout << "Enter cryptogram:" << std::endl;
    std::getline(std::cin, cryptogramText);
    std::transform(cryptogramText.begin(), cryptogramText.end(), cryptogramText.begin(), ::toupper);
    CryptoText cryptogramTextFixed = cryptogramText;
    if (wordList.size() > 0)
    {
        unsigned maxThreads = std::thread::hardware_concurrency();
        SolutionMap solutionMap;
        std::uniform_int_distribution<unsigned int> dist(0, maxThreads - 1);


        while (solutionMap.size() < GOOD_SOLUTION_NUM)
        {
            std::vector<std::thread> vecThreads;
            CryptoKeySet bestKeys = getBestKeys<ALPHABET_LETTERS_NUM>(solutionMap, maxThreads);
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
                const CryptoKey& keyToPass = *itKeyToPass;
                vecThreads.push_back(std::thread(addOneBetterSolution, std::ref(solutionMap), std::ref(solutionMapLocker), std::ref(keyToPass), std::ref(cryptogramTextFixed), std::ref(wordList)));
                ++itKeyToPass;
            }

            for (auto& thr : vecThreads) 
            {
                thr.join();
            }
            
        }
    }
}

