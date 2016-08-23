#include <iostream>
#include <fstream>
#include <array>
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
#include <assert.h>


constexpr const char* wordlistName = "../wordlist/google-10000-english-usa.txt";
constexpr unsigned int ALPHABET_LETTERS_NUM = 26;
constexpr unsigned int GOOD_SOLUTION_NUM = 1000;
constexpr double SOLUTION_QUALITY = 0.7;
constexpr unsigned int maxTextLength = 70;
constexpr int mutateGoodLetterFactor = 20;
constexpr size_t maxWords = 20;
constexpr int keyTryLimit = 100000;

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

    FixedString<maxLen> substr(size_t begin, size_t len) const
    {
        FixedString<maxLen> retVal;
        assert(begin + len < arrSize);
        memset(retVal.array, 0, arrSize);
        memcpy(retVal.array, &array[begin], len);
        retVal.currentLen = len;
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
using WordArray = std::array<Word, maxWords>;
using CryptoText = FixedString<maxTextLength>;
using CryptoKey = FixedString<ALPHABET_LETTERS_NUM>;
using CryptoKeySet = std::unordered_set<CryptoKey, CryptoKey::hasher>;
using CryptoKeyData = std::pair<CryptoKey, unsigned int>;
using Solution = std::pair<CryptoKeyData, const CryptoText>;
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
CryptoKeyData MutateKey(const CryptoKeyData& sourceKeyData, std::set<char>& goodPos)
{
    CryptoKeyData retVal = sourceKeyData;
    CryptoKey& keyToMutate = retVal.first;
    std::uniform_int_distribution<unsigned int> dist(0, NumLetters - 1);
    std::uniform_int_distribution<unsigned int> mutateGoodChanceDist(0, 1000);
    unsigned int rnd;
    unsigned int chance;
    do
    {
        rnd = dist(e1);
        chance = mutateGoodChanceDist(e1);
    } while ((goodPos.find(rnd) != goodPos.end()) && (chance > mutateGoodLetterFactor));
    unsigned int rndNew = dist(e1);
    char& chrToMutate = keyToMutate.at(rnd);
    char newVal = 'A' + rndNew;
    keyToMutate.at(keyToMutate.find_first_of(newVal)) = chrToMutate;
    chrToMutate = newVal;
    retVal.second++;
    return retVal;
}

size_t splitLineToWords(const CryptoText& line, WordArray& outVec)
{
    size_t retVal = 0;
    size_t strPos = 0;
    size_t oldPos = 0;
    while (strPos != CryptoText::npos)
    {
        strPos = line.find_first_of(' ', oldPos);
        if (strPos != CryptoText::npos)
        {
            outVec.at(retVal) = line.substr(oldPos, strPos - oldPos);
            retVal++;
            oldPos = strPos + 1;
        }
    }
    //handle last word
    outVec.at(retVal) = line.substr(oldPos, line.size() - oldPos);
    retVal++;
    return retVal;
}

double calcTextQuality(const CryptoText& text, const WordList& wordList, std::set<char>& goodPos)
{
    double retVal;
    WordArray arrayWords;
    size_t numWords = splitLineToWords(text, arrayWords);
    size_t allTextLength = 0;
    size_t goodTextLength = 0;
    int good = 0;
    for (auto wordIndex = 0; wordIndex < numWords; ++wordIndex)
    {
        const Word& word = arrayWords.at(wordIndex);
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

void removeElementWithCryptoKey(SolutionMap& sMap, const CryptoKey& cKey) 
{
    for (auto it = sMap.begin(); it != sMap.end(); ++it) 
    {
        const CryptoKey& elementKey = (*it).second.first.first;
        if (elementKey == cKey) 
        {
            sMap.erase(it);
            std::cout << "Removing solution   (Q: " << it->first << "): " << it->second.second.c_str() << std::endl;
            break;
        }
    }
}

void addOneBetterSolution(SolutionMap& sMap, std::mutex& mapMutex, const CryptoKeyData& initialKey, const CryptoText& cryptoText, WordList& wordList)
{
    bool added = false;
    CryptoKeyData newKeyData = initialKey;
    CryptoText decryptedText = transformText<ALPHABET_LETTERS_NUM>(cryptoText, initialKey.first);
    std::set<char> goodPositions;
    const double minQuality = calcTextQuality(decryptedText, wordList, goodPositions);
    while (!added)
    {
        newKeyData = MutateKey<ALPHABET_LETTERS_NUM>(newKeyData, goodPositions);
        if (newKeyData.second > keyTryLimit)
        {
            mapMutex.lock();
            removeElementWithCryptoKey(sMap, initialKey.first);
            mapMutex.unlock();
        }
        decryptedText = transformText<ALPHABET_LETTERS_NUM>(cryptoText, newKeyData.first);
        double quality = calcTextQuality(decryptedText, wordList, goodPositions);
        //std::cout << decryptedText << std::endl;
        if (quality > minQuality)
        {
            auto pair = std::make_pair(newKeyData, decryptedText);
            mapMutex.lock();
            sMap.insert(std::make_pair(quality, pair));
            mapMutex.unlock();
            std::cout << "New better solution (Q: " << std::setprecision(4) << std::fixed << quality << "): " << decryptedText.c_str() << std::endl;
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
        retVal.insert(it->second.first.first);
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
    std::string cryptogramText;// = "AQQH TL AIF LMX XD SOG KPA TSUHF CDL QFTCS MPU HVNSTL";
    std::cout << "Enter cryptogram:" << std::endl;
    std::getline(std::cin, cryptogramText);
    std::transform(cryptogramText.begin(), cryptogramText.end(), cryptogramText.begin(), ::toupper);
    CryptoText cryptogramTextFixed = cryptogramText;
    if (wordList.size() > 0)
    {
        unsigned maxThreads = std::thread::hardware_concurrency();
        SolutionMap solutionMap;
        std::uniform_int_distribution<unsigned int> dist(0, maxThreads - 1);
        CryptoKeyData keyDataToPass;

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
                keyDataToPass.first = *itKeyToPass;
                vecThreads.push_back(std::thread(addOneBetterSolution, std::ref(solutionMap), std::ref(solutionMapLocker), std::ref(keyDataToPass), std::ref(cryptogramTextFixed), std::ref(wordList)));
                ++itKeyToPass;
            }

            for (auto& thr : vecThreads) 
            {
                thr.join();
            }
            
        }
    }
}

