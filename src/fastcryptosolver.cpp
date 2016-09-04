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
#include <future>
#include "MurmurHash3.h"
#include <assert.h>


constexpr const char* wordlistName = "../wordlist/google-10000-english-usa.txt";
constexpr unsigned int ALPHABET_LETTERS_NUM = 26;
constexpr unsigned int GOOD_SOLUTION_NUM = 1000;
constexpr double SOLUTION_QUALITY = 0.7;
constexpr unsigned int maxTextLength = 70;
constexpr int mutateGoodLetterFactor = 20;
constexpr size_t maxWords = 20;
constexpr unsigned int keyTryLimit = 80000000u;

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

    inline const char * c_str() const
    {
        return array;
    }

    inline const char& at(size_t offset) const
    {
        assert(offset >= 0);
        assert(offset < maxLen);
        return array[offset];
    }

    inline char& at(size_t offset)
    {
        return array[offset];
    }

    inline size_t size() const
    {
        return currentLen;
    }

    struct hasher
    {
        size_t operator()( const FixedString<maxLen>& objToHash ) const
        {
            size_t retVal;
            MurmurHash3_x86_32(&objToHash.array, sizeof(FixedString<maxLen>::array), 0xDEADBEEF, &retVal);
            return retVal;
        }
    };

    inline bool operator==(const FixedString<maxLen>& other) const
    {
        return (memcmp(array, other.array, currentLen) == 0);
    }

    inline void operator=(const FixedString<maxLen> &other )
    {
        memcpy(array, other.array, arrSize);
        currentLen = other.currentLen;
    }

    inline size_t find_first_of(const char val, size_t offset = 0) const
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

    inline void push_back(char chr)
    {
        array[currentLen] = chr;
        currentLen++;
    }

    inline FixedString<maxLen> substr(size_t begin, size_t len) const
    {
        FixedString<maxLen> retVal;
        assert(begin + len < arrSize);
        memset(retVal.array, 0, arrSize);
        memcpy(retVal.array, &array[begin], len);
        retVal.currentLen = len;
        return retVal;
    }

    inline size_t count(const char val) 
    {
        size_t retVal = 0;
        for (size_t index = 0; index < currentLen; ++index) 
        {
            if (array[index] == val) 
            {
                ++retVal;
            }
        }
        return retVal;
    }

    static const size_t npos = static_cast<size_t>(-1);

private:
    size_t currentLen;
    alignas(16) char array[arrSize];
};



//using Word = std::string;
//using WordList = std::unordered_set<std::string>;
using Word = FixedString<maxTextLength>;
using WordList = std::unordered_set<Word, Word::hasher>;
using WordPatternMap = std::unordered_map<Word, WordList, Word::hasher>;
using WordArray = std::array<Word, maxWords>;
using CryptoText = FixedString<maxTextLength>;
using CryptoKey = FixedString<ALPHABET_LETTERS_NUM>;
using CryptoKeyList = std::vector<CryptoKey>;
using CryptoKeySet = std::unordered_set<CryptoKey, CryptoKey::hasher>;
using CryptoKeyData = std::pair<CryptoKey, unsigned int>;
using Solution = std::pair<CryptoKeyData, const CryptoText>;
using SolutionMap = std::multimap<double, Solution>;
using LetterFrequencyMap = std::map<char, unsigned int>;
using Combination = std::vector<int>;
using CombinationList = std::vector<Combination>;


Word getWordPattern(const Word& word)
{

    Word retVal;
    Word listOfUsedLetters;
    char code = 'A';
    for(size_t index = 0; index < word.size(); ++index)
    {
        const char& chr = word.at(index);
        size_t pos = listOfUsedLetters.find_first_of(chr);
        if(pos == Word::npos)
        {
            retVal.push_back(code);
            listOfUsedLetters.push_back(chr);
            code++;
        }
        else
        {
            retVal.push_back(retVal.at(pos));
        }
    }
    return retVal;
}

WordPatternMap createPatternMap(WordList& list)
{
    WordPatternMap retVal;
    for(const Word& word : list)
    {
        Word pattern = getWordPattern(word);
        WordList& wordListOfCurrentPattern = retVal[pattern];
        wordListOfCurrentPattern.emplace(word);
    }
    return retVal;
}

CryptoKey getInitialKey()
{
    return CryptoKey("**************************");
}

CryptoKey getCommonKeyFromTwoWords(const Word& encryptedWord, const Word& decryptedWord)
{
    CryptoKey retVal = getInitialKey();
    assert(encryptedWord.size() == decryptedWord.size());
    for(size_t index = 0; index < encryptedWord.size(); ++index)
    {
        const char& chrEnc = encryptedWord.at(index);
        const char& chrDec = decryptedWord.at(index);
        retVal.at(chrEnc - 'A') = chrDec;
    }
    return retVal;
}

CryptoKeyList getMatchingKeys(const Word& encryptedWord, const WordList& possibleMatches)
{
    CryptoKeyList retVal;

    for(const Word& matchingWord : possibleMatches)
    {
        retVal.emplace_back(getCommonKeyFromTwoWords(encryptedWord, matchingWord));
    }

    return retVal;
}
CryptoKeyList combineTwoKeys(const CryptoKey& key1, const CryptoKey& key2)
{
    CryptoKeyList list;
    CryptoKey outKey(getInitialKey());
    bool success = true;
    for (size_t index = 0; index < key1.size(); ++index)
    {
        const char& chr1 = key1.at(index);
        const char& chr2 = key2.at(index);
        if ((chr1 != '*') && (chr2 != '*'))
        {
            success = false;
            break;
        }
        else if (chr1 != '*')
        {
            outKey.at(index) = chr1;
        }
        else if (chr2 != '*')
        {
            outKey.at(index) = chr2;
        }
    }
    if (success) 
    {
        list.emplace_back(outKey);
    }
    else 
    {
        list.emplace_back(key1);
        list.emplace_back(key2);
    }
    return list;
}

CryptoKeyList combineTwoKeyLists(const CryptoKeyList& keyList1, const CryptoKeyList& keyList2)
{
    CryptoKeyList retVal;
    CryptoKeyList::const_iterator it1;
    CryptoKeyList::const_iterator it2;

    for (it1 = keyList1.begin(); it1 != keyList1.end(); ++it1)
    {
        for (it2 = keyList2.begin(); it2 != keyList2.end(); ++it2) 
        {
            CryptoKeyList res = combineTwoKeys(*it1, *it2);
            retVal.insert(retVal.end(), res.begin(), res.end());
        }
    }
    return retVal;
}

CryptoKeyList combineKeys(const std::vector<CryptoKeyList>& keyList, const CombinationList& comboList)
{
    CryptoKeyList retVal;
    
    for (const Combination& combo : comboList)
    {
        bool finished = false;
        CryptoKeyList currentList;
        currentList = keyList[0];
        for (int index = 1; index < combo.size(); ++index) 
        {
            const CryptoKeyList& keyListOther = keyList[index];
            currentList = combineTwoKeyLists(currentList, keyListOther);
        }
        retVal.insert(retVal.end(), currentList.begin(), currentList.end());
    }
    return retVal;
}

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

CombinationList createAllPermutations(size_t numOfElements)
{
    CombinationList retVal;
    std::vector<int> initVec;
    initVec.resize(numOfElements);
    retVal.reserve(numOfElements*numOfElements);
    for(auto ind = 0; ind < numOfElements; ++ind)
    {
        initVec[ind] = ind;
    }

    do
    {
        retVal.emplace_back(initVec);
    } while (std::next_permutation(initVec.begin(), initVec.end()));

    return retVal;
}

void analyseWord(Word& word, LetterFrequencyMap& freqMap)
{
    for (auto index = 0; index < word.size(); ++index)
    {
        const char chr = word.at(index);
        if (freqMap.find(chr) == freqMap.end())
        {
            freqMap[chr] = 0;
        }
        freqMap[chr]++;
    }
}

void loadWordListIntoSet(const std::string& filename, WordList& outSet, LetterFrequencyMap& freqMap)
{
    std::cout << "Start loading wordlist from file: " << std::endl << filename << std::endl;
    auto tpBegin = std::chrono::system_clock::now();
    std::ifstream wordlistFile(filename);
    outSet.reserve(4000000);
    if (wordlistFile.is_open())
    {
        std::string line;
        // get length of file:
        wordlistFile.seekg(0, wordlistFile.end);
        std::streamoff fileSize = wordlistFile.tellg();
        wordlistFile.seekg(0, wordlistFile.beg);
        Word word;
        while (std::getline(wordlistFile, line))
        {
            std::transform(line.begin(), line.end(), line.begin(), ::toupper);
            word = line;
            //analyseWord(word, freqMap);
            outSet.insert(word);
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
CryptoKeyData MutateKey(const CryptoKeyData& sourceKeyData, std::set<char>& goodPos, LetterFrequencyMap& freqMap)
{
    CryptoKeyData retVal = sourceKeyData;
    CryptoKey& keyToMutate = retVal.first;
    std::uniform_int_distribution<unsigned int> mutateGoodChanceDist(0, 1000);
    std::uniform_int_distribution<unsigned int> letterDist(0, ALPHABET_LETTERS_NUM - 1);
    unsigned int rnd;
    unsigned int chance;
    unsigned int allLettersNum = 0;
    for (auto it = freqMap.begin(); it != freqMap.end(); ++it) 
    {
        allLettersNum += it->second;
    }
    std::uniform_int_distribution<unsigned int> freqDist(0, allLettersNum - 1);
    unsigned int rndNew = freqDist(e1);
    do
    {
        rnd = letterDist(e1);
        chance = mutateGoodChanceDist(e1);
    } while ((goodPos.find(rnd) != goodPos.end()) && (chance > mutateGoodLetterFactor));
    char& chrToMutate = keyToMutate.at(rnd);
    char newVal=0;
    unsigned int freqWalker = 0;

    for (auto it = freqMap.begin(); it != freqMap.end(); ++it) 
    {
        freqWalker += it->second;
        if (freqWalker >= allLettersNum - rndNew) 
        {
            newVal = it->first;
            break;
        }
    }

    keyToMutate.at(keyToMutate.find_first_of(newVal)) = chrToMutate;
    chrToMutate = newVal;
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

double calcTextQuality(const CryptoText& text, const WordList& wordList)
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
            }
        }
    }
    retVal = (double)goodTextLength / (double)allTextLength;
    return retVal;
}

void removeElementWithCryptoKey(SolutionMap& sMap, const CryptoKeyData& cKey, int iterations) 
{
    for (auto it = sMap.begin(); it != sMap.end(); ++it) 
    {
        const CryptoKey& elementKey = (*it).second.first.first;
        if (elementKey == cKey.first) 
        {
            std::cout << "Removing solution   (Q: " << it->first 
                << " K:" << std::setfill(' ') << std::setw(8) << iterations <<
                "): " << it->second.second.c_str() << std::endl;
            sMap.erase(it);
            break;
        }
    }
}

void addOneBetterSolution(SolutionMap& sMap, std::mutex& mapMutex, const CryptoKeyData& initialKey, const CryptoText& cryptoText, WordList& wordList, LetterFrequencyMap& freqMap)
{
    bool added = false;
    CryptoKeyData newKeyData = initialKey;
    CryptoText decryptedText = transformText<ALPHABET_LETTERS_NUM>(cryptoText, initialKey.first);
    std::set<char> goodPositions;
    const double minQuality = calcTextQuality(decryptedText, wordList);
    while (!added)
    {
        newKeyData = MutateKey<ALPHABET_LETTERS_NUM>(newKeyData, goodPositions, freqMap);
        newKeyData.second++;
        if (newKeyData.second >= keyTryLimit)
        {
            mapMutex.lock();
            removeElementWithCryptoKey(sMap, initialKey, newKeyData.second);
            mapMutex.unlock();
            break;
        }
        decryptedText = transformText<ALPHABET_LETTERS_NUM>(cryptoText, newKeyData.first);
        double quality = calcTextQuality(decryptedText, wordList);
        //std::cout << decryptedText << std::endl;
        if (quality > minQuality)
        {
            auto pair = std::make_pair(newKeyData, decryptedText);
            mapMutex.lock();
            sMap.insert(std::make_pair(quality, pair));
            mapMutex.unlock();
            std::cout << "New better solution (Q: " << std::setprecision(4) << std::fixed << quality 
                << " K:" << std::setfill(' ') << std::setw(8) << newKeyData.second
                << "): " << decryptedText.c_str() << std::endl;
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
    CryptoKey newKey;
    std::uniform_int_distribution<int> randomLetter('A', 'Z');
    while (newKey.size() < NumLetters)
    {
        char chr = static_cast<char>(randomLetter(e1));
        if (newKey.find_first_of(chr) == CryptoKey::npos)
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


void printSolution(const CryptoText& text, const CryptoKey& key, const WordList& wordList)
{
    if (key.find_first_of('*') != CryptoKey::npos) 
    {
        CryptoText decrypted = transformText<ALPHABET_LETTERS_NUM>(text, key);
        double quality = calcTextQuality(decrypted, wordList);
        std::cout << "Key: " << key.c_str() << " Q(" << std::setprecision(4) << std::fixed << quality
            << ") Decrypted: " << decrypted.c_str() << std::endl;
    }
}

int main(int argc, char *argv[])
{
    WordList wordList;
    LetterFrequencyMap freqMap;
    loadWordListIntoSet(wordlistName, wordList, freqMap);
    WordPatternMap patternMap;
    patternMap = createPatternMap(wordList);
    std::string cryptogramText = "TUQS MGZI BHDDWA MGZSP ZI GUVT";
    //std::cout << "Enter cryptogram:" << std::endl;
    //std::getline(std::cin, cryptogramText);
    //std::transform(cryptogramText.begin(), cryptogramText.end(), cryptogramText.begin(), ::toupper);
    CryptoText cryptogramTextFixed = cryptogramText;
    if (wordList.size() > 0)
    {
        WordArray arrayWords;
        size_t numWords = splitLineToWords(cryptogramTextFixed, arrayWords);
        CryptoKeyList matchingKeys;
        std::vector<CryptoKeyList> keysPerWord;
        keysPerWord.resize(numWords);
        for(size_t index = 0; index < numWords; ++index)
        {
            Word& word = arrayWords[index];
            Word pattern = getWordPattern(word);
            WordPatternMap::const_iterator it = patternMap.find(pattern);
            if(it != patternMap.end())
            {
                const WordList& matchingWordList = it->second;

                keysPerWord[index] = getMatchingKeys(word, matchingWordList);
            }
        }
        
        CombinationList allCombinations = createAllPermutations(numWords);
        size_t maxThreads = std::thread::hardware_concurrency();
        std::vector<CombinationList> combinationsPerThread;
        maxThreads = std::min(maxThreads, allCombinations.size());
        combinationsPerThread.resize(maxThreads);
        size_t threadId = 0;
        for(const Combination& combo : allCombinations)
        {
            combinationsPerThread[threadId].emplace_back(combo);
            if (threadId < maxThreads - 1)
            {
                ++threadId;
            }
            else 
            {
                threadId = 0;
            }
        }

        std::vector<std::thread> vecThreads;
        std::vector<std::future<CryptoKeyList>> vecFutures;
        CryptoKeyList threadResults;
        for (size_t threadId = 0; threadId < maxThreads; ++threadId) 
        {
            std::packaged_task<CryptoKeyList(const std::vector<CryptoKeyList>&, const CombinationList&)> task(combineKeys);
            vecFutures.emplace_back(task.get_future());
            vecThreads.emplace_back(std::thread(std::move(task), keysPerWord, combinationsPerThread[threadId]));
        }
        for (size_t threadId = 0; threadId < maxThreads; ++threadId)
        {
            vecThreads[threadId].join();
            CryptoKeyList threadResult = vecFutures[threadId].get();
            threadResults.insert(threadResults.end(), threadResult.begin(), threadResult.end());
        }
        
        for(const CryptoKey& fullKey : matchingKeys)
        {
            printSolution(cryptogramTextFixed, fullKey, wordList);
        }
#if OLD_IMPLEMENTATION
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
                keyDataToPass.second = 0;
                vecThreads.push_back(
                    std::thread(addOneBetterSolution, 
                    std::ref(solutionMap), 
                    std::ref(solutionMapLocker), 
                    std::ref(keyDataToPass), 
                    std::ref(cryptogramTextFixed), 
                    std::ref(wordList),
                    std::ref(freqMap)));
                ++itKeyToPass;
            }

            for (auto& thr : vecThreads) 
            {
                thr.join();
            }
            
        }
#endif
    }
}

