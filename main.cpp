//
//  main.cpp
//  FinalLab
//
//  Created by Mher Torjyan on 6/6/18.
//  Copyright © 2018 Mher Torjyan. All rights reserved.
//

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <regex>
#include <future>
#include <mutex>
#include <atomic>
#include <queue>
#include <deque>
#include <list>
#include <stack>
#include <algorithm>
#include <iterator>
#include <functional>
#include <cmath>
#include <queue>
#include <numeric>
#include <map>
#include <locale>
#include <thread>

#include <functional>
#include <initializer_list>

using namespace std;

mutex global_mutex;

bool CustomBinaryMapSearch(map<string, double> mapToSearch, string toFind){
    auto it = mapToSearch.find(toFind);
    if (it != mapToSearch.end()){
        return true;
    }
    return false;
}


string Parse(string text, string split){
    vector<string> words;
    sregex_token_iterator end;
    regex pattern(split);
    for (sregex_token_iterator iter(text.begin(), text.end(), pattern); iter != end; ++iter){
        if ((*iter).length() > 0){
            if ((static_cast<string>(*iter))[0] != 0x20){
                return *iter;
            }
        }
    }
    return "";
}

class WordStorage{
    
protected:
    map<string, int> freqOfWordsInArticle;
    map<string, double> weightOfWordsInArticle;
    int numWordInFile;
public:
    
    virtual void genererateWeightListForFile() = 0;
    int getNumWordInFile(){return numWordInFile;}
    void setNumWordsInFile(int num){numWordInFile = num;}
    
};

class CatagoryDictionary : public WordStorage{
private:
    string catagory;
    map<string, double> weightOfWordsInAllFiles;

public:
    CatagoryDictionary(string nameOfCatagory){
        this->catagory = nameOfCatagory;
    }
    
    void generateFrequencyTableForFile(vector<pair<vector<string>, int>> filesIn);
    void genererateWeightListForFile();
    double getWeightForWord(string toFind);
    void deleteWord(string in){
        weightOfWordsInAllFiles.erase(in);
    }
    string getCatagory(){
        return catagory;
    }
    map<string, double>* getWeightTable(){
        return &weightOfWordsInAllFiles;
    }
    
    
};

double CatagoryDictionary::getWeightForWord(string toFind){
    double weight = -1;
    if(CustomBinaryMapSearch(weightOfWordsInAllFiles, toFind)){
        weight = (double)weightOfWordsInAllFiles.lower_bound(toFind)->second;
    }
    return weight;
    
}

void CatagoryDictionary::generateFrequencyTableForFile(vector<pair<vector<string>, int>> filesIn){
    global_mutex.lock();
    for(int i = 0 ; i < filesIn.size(); i++){
        for (string s : filesIn[i].first){
            s = Parse(s, "[a-zA-Z0-9]*");
            int& number = freqOfWordsInArticle[s];
            ++number;
        }
        setNumWordsInFile(filesIn[i].second);
        genererateWeightListForFile();
    }
    global_mutex.unlock();
}

void CatagoryDictionary::genererateWeightListForFile(){
    string temp;
    for_each(freqOfWordsInArticle.begin(), freqOfWordsInArticle.end(), [&](const pair<string, int>& pair){
        temp = pair.first;
        for (int i=0; temp[i]; i++) {
            temp[i] = tolower(temp[i]);
        }
        if(CustomBinaryMapSearch(weightOfWordsInAllFiles, temp)){
            weightOfWordsInAllFiles[temp]+=(double)pair.second/getNumWordInFile();
        }else{
            weightOfWordsInAllFiles[temp]=(double)pair.second/getNumWordInFile();
        }
    });
}

class DictionaryHandler{
private:
    vector<CatagoryDictionary> dictionaryCatagory;
    
public:
    vector<CatagoryDictionary> getDictionaries();
    void insertNewDictionary(CatagoryDictionary dicIn);
    void truncateDuplicates();
};

void DictionaryHandler::truncateDuplicates(){
    map<string, double> weightTable1 = *dictionaryCatagory[0].getWeightTable();
    map<string, double> weightTable2 = *dictionaryCatagory[1].getWeightTable();
    map<string, double> weightTable3 = *dictionaryCatagory[2].getWeightTable();
    for_each(weightTable1.begin(), weightTable1.end(), [&](const pair<string, int>& pair){
        if(CustomBinaryMapSearch(weightTable2, pair.first) && CustomBinaryMapSearch(weightTable3, pair.first)){
            dictionaryCatagory[0].deleteWord(pair.first);
            dictionaryCatagory[1].deleteWord(pair.first);
            dictionaryCatagory[2].deleteWord(pair.first);
        }
    });
    for_each(weightTable2.begin(), weightTable2.end(), [&](const pair<string, int>& pair){
        if(CustomBinaryMapSearch(weightTable1, pair.first) && CustomBinaryMapSearch(weightTable3, pair.first)){
            dictionaryCatagory[0].deleteWord(pair.first);
            dictionaryCatagory[1].deleteWord(pair.first);
            dictionaryCatagory[2].deleteWord(pair.first);
        }
    });
    for_each(weightTable3.begin(), weightTable3.end(), [&](const pair<string, int>& pair){
        if(CustomBinaryMapSearch(weightTable1, pair.first) && CustomBinaryMapSearch(weightTable2, pair.first)){
            dictionaryCatagory[0].deleteWord(pair.first);
            dictionaryCatagory[1].deleteWord(pair.first);
            dictionaryCatagory[2].deleteWord(pair.first);
        }
    });
}

vector<CatagoryDictionary> DictionaryHandler::getDictionaries(){
    return dictionaryCatagory;
}
void DictionaryHandler::insertNewDictionary(CatagoryDictionary dicIn){
    dictionaryCatagory.push_back(dicIn);
}

class DetermineArticleCatagory : public WordStorage{
private:
    vector<string> wordsInArticle;
    map<string, double> weightOfWordsInArticle;
    DictionaryHandler localDicHandler;
    
public:
    DetermineArticleCatagory(DictionaryHandler dicsIn){
        this->localDicHandler = dicsIn;
    }
    
    void setArticle(vector<string> articleIn){
        wordsInArticle.clear();
        freqOfWordsInArticle.clear();
        weightOfWordsInArticle.clear();
        wordsInArticle = articleIn;
        generateFreqTable();
    }
    void generateFreqTable();
    void genererateWeightListForFile();
    string determineCatagory();
};

void DetermineArticleCatagory::generateFreqTable(){
    int words = 0;
    for (string s : wordsInArticle){
        s = Parse(s, "[a-zA-Z0-9]*");
        int& number = freqOfWordsInArticle[s];
        ++number;
        words++;
    }
    setNumWordsInFile(words);
    genererateWeightListForFile();
    
}
void DetermineArticleCatagory::genererateWeightListForFile(){
    string temp;
    for_each(freqOfWordsInArticle.begin(), freqOfWordsInArticle.end(), [&](const pair<string, int>& pair){
        temp = pair.first;
        for (int i=0; temp[i]; i++) {
            temp[i] = tolower(temp[i]);
        }
        weightOfWordsInArticle[temp]=(double)pair.second/getNumWordInFile();
    });
}

string DetermineArticleCatagory::determineCatagory(){
    string foundCatagory = "Not Found";
    vector<double> totals;
    vector<CatagoryDictionary> cats = localDicHandler.getDictionaries();
    string wordToFind;
    int count=0;
    double totalForCat = 0;
    double weightOfWord;

    for_each(cats.begin(), cats.end(), [&](CatagoryDictionary dicIn){
        count = 0;
        totalForCat = 0;
        auto freqIter = freqOfWordsInArticle.begin();
        for(auto iter = weightOfWordsInArticle.begin(); iter != weightOfWordsInArticle.end(); iter++){
            wordToFind = iter->first;
            weightOfWord = dicIn.getWeightForWord(wordToFind);
            if(weightOfWord!=-1){
                count++;
                totalForCat += (double)((weightOfWord*iter->second)*freqIter->second);
            }
            freqIter++;
        }
        totals.push_back(totalForCat);
    });
    
    double curMax = totals[0];
    double sum = curMax;
    int curIndex = 0;
    for(int i = 1; i < totals.size(); i++){
        sum += totals[i];
        if(totals[i]>curMax){
            curMax = totals[i];
            curIndex = i;
        }
    }
    cout << "Politics: " << ((double)(totals[0]/sum)*100) << "% Sports: " << ((double)(totals[1]/sum)*100) << "% Tech: " << ((double)(totals[2]/sum)*100) << "%" << endl;
    
    foundCatagory = cats[curIndex].getCatagory();
    return foundCatagory;
}

class FileHandler{
private:
    ifstream inFile;
    int numWords;
public:
    
    
    vector<string> getWordsInFile(string filePath);
    void setNumWords(int num);
    int getNumWords();
    
};

vector<string> FileHandler::getWordsInFile(string filePath){
    vector<string> words;
    ifstream infile(filePath);
    int counter = 0;
    while(!infile.eof()){
        string buffer;
        getline(infile, buffer , ' ');
        if(buffer != " "){
            words.push_back(buffer);
        }
        counter ++;
    }
    setNumWords(counter);
    return words;
}

void FileHandler::setNumWords(int num){
    this->numWords = num;
}

int FileHandler::getNumWords(){
    return numWords;
}


int main(int argc, const char * argv[]) {
    
    unique_ptr<FileHandler> fileHandler(new FileHandler());
    
    shared_ptr<DictionaryHandler> handle(new DictionaryHandler());
    
    vector<pair<vector<string>, int>> saveAllFilesPolitics;
    vector<pair<vector<string>, int>> saveAllFilesSports;
    vector<pair<vector<string>, int>> saveAllFilesTech;
    
    vector<string> words = fileHandler->getWordsInFile("/Users/mtorjyan/Projects/CS29/FinalLab/FinalLab/SampleText/Politics/Politics1.txt");
    pair<vector<string>, int> politics1 (words, fileHandler->getNumWords());
    saveAllFilesPolitics.push_back(politics1);
    
    words = fileHandler->getWordsInFile("/Users/mtorjyan/Projects/CS29/FinalLab/FinalLab/SampleText/Politics/Politics2.txt");
    pair<vector<string>, int> politics2 (words, fileHandler->getNumWords());
    saveAllFilesPolitics.push_back(politics2);
    
    words = fileHandler->getWordsInFile("/Users/mtorjyan/Projects/CS29/FinalLab/FinalLab/SampleText/Politics/Politics3.txt");
    pair<vector<string>, int> politics3 (words, fileHandler->getNumWords());
    saveAllFilesPolitics.push_back(politics3);
    shared_ptr<CatagoryDictionary> politicsDic(new CatagoryDictionary("Politics"));
    
    words = fileHandler->getWordsInFile("/Users/mtorjyan/Projects/CS29/FinalLab/FinalLab/SampleText/Sports/Sports1.txt");
    pair<vector<string>, int> sports1 (words, fileHandler->getNumWords());
    saveAllFilesSports.push_back(sports1);
    
    words = fileHandler->getWordsInFile("/Users/mtorjyan/Projects/CS29/FinalLab/FinalLab/SampleText/Sports/Sports2.txt");
    pair<vector<string>, int> sports2 (words, fileHandler->getNumWords());
    saveAllFilesSports.push_back(sports2);
    
    words = fileHandler->getWordsInFile("/Users/mtorjyan/Projects/CS29/FinalLab/FinalLab/SampleText/Sports/Sports3.txt");
    pair<vector<string>, int> sports3 (words, fileHandler->getNumWords());
    saveAllFilesSports.push_back(sports3);
    shared_ptr<CatagoryDictionary> sportsDic(new CatagoryDictionary("Sports"));

    words = fileHandler->getWordsInFile("/Users/mtorjyan/Projects/CS29/FinalLab/FinalLab/SampleText/Tech/Tech1.txt");
    pair<vector<string>, int> tech1 (words, fileHandler->getNumWords());
    saveAllFilesTech.push_back(tech1);
    
    words = fileHandler->getWordsInFile("/Users/mtorjyan/Projects/CS29/FinalLab/FinalLab/SampleText/Tech/Tech2.txt");
    pair<vector<string>, int> tech2 (words, fileHandler->getNumWords());
    saveAllFilesTech.push_back(tech2);
    
    words = fileHandler->getWordsInFile("/Users/mtorjyan/Projects/CS29/FinalLab/FinalLab/SampleText/Tech/Tech3.txt");
    pair<vector<string>, int> tech3 (words, fileHandler->getNumWords());
    saveAllFilesTech.push_back(tech3);
    shared_ptr<CatagoryDictionary> techDic(new CatagoryDictionary("Tech"));
    
    thread t1 (&CatagoryDictionary::generateFrequencyTableForFile, politicsDic, saveAllFilesPolitics);
    thread t2 (&CatagoryDictionary::generateFrequencyTableForFile, sportsDic, saveAllFilesSports);
    thread t3 (&CatagoryDictionary::generateFrequencyTableForFile, techDic, saveAllFilesTech);

    t1.join();
    t2.join();
    t3.join();
    
    
    handle->insertNewDictionary(*politicsDic);
    handle->insertNewDictionary(*sportsDic);
    handle->insertNewDictionary(*techDic);
    
    
    handle->truncateDuplicates();
    
    
    vector<string> toGuess = fileHandler->getWordsInFile("/Users/mtorjyan/Projects/CS29/FinalLab/FinalLab/SampleText/Tests/ToTestSports.txt");
    unique_ptr<DetermineArticleCatagory> determineCat(new DetermineArticleCatagory(*handle));
    determineCat->setArticle(toGuess);
    string guess = determineCat->determineCatagory();
    cout <<  "Guess: " << guess << endl;
    cout << "Actually: Sports" << endl;
    
    toGuess = fileHandler->getWordsInFile("/Users/mtorjyan/Projects/CS29/FinalLab/FinalLab/SampleText/Tests/ToTestPolitics.txt");
    determineCat->setArticle(toGuess);
    guess = determineCat->determineCatagory();
    cout <<  "Guess: " << guess << endl;
    cout << "Actually: Politics" << endl;
    
    toGuess = fileHandler->getWordsInFile("/Users/mtorjyan/Projects/CS29/FinalLab/FinalLab/SampleText/Tests/ToTestTech.txt");
    determineCat->setArticle(toGuess);
    guess = determineCat->determineCatagory();
    cout <<  "Guess: " << guess << endl;
    cout << "Actually: Tech" << endl;
    return 0;
}
