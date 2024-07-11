#include "learning_data.h"
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <fstream>  
#include <sstream>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

const string learning_data::FILE_PATH_SPLITTER = "/";

const string learning_data::FILENAME_DELIMITER = "_";
const int learning_data::FILENAME_SPLITS_NUMBER = 3;
const int learning_data::FILENAME_ROUND_INDEX = 0;
const int learning_data::FILENAME_CLIENT_NUMBER_INDEX = 1;
const int learning_data::FILENAME_FEATURE_SERIAL_INDEX = 2;

const int learning_data::ERROR_ROUND = -1;

learning_data::learning_data() 
{
    std::cout.precision(9);
}

learning_data::~learning_data() 
{

}

void learning_data::setDirPath(string dirPath) 
{
    this->dirPath = dirPath;
}

string learning_data::getFilePath(feature_info_t fileNameInfo)
{
    string strRound = std::to_string(fileNameInfo.round);
    string strClientNumber = std::to_string(fileNameInfo.clientNumber);
    string strFeatureSerial = std::to_string(fileNameInfo.featureSerialNumber);

    return dirPath + FILE_PATH_SPLITTER + strRound + FILENAME_DELIMITER 
           + strClientNumber + FILENAME_DELIMITER + strFeatureSerial + ".txt";
}

int learning_data::splitFileName(string strFileName, std::vector<int>& vec)
{
    std::string fileName = strFileName;
    std::string::size_type dotPos = strFileName.find(".");
    if(dotPos > 0)
    {
        fileName = strFileName.substr(0, dotPos);
    }
    std::string::size_type startPos = 0;
    std::string::size_type foundPos = fileName.find(FILENAME_DELIMITER);

    while (foundPos != std::string::npos){
        string tmpStr = fileName.substr(startPos, foundPos - startPos);
        vec.push_back(std::stoi(tmpStr));
        startPos = foundPos + FILENAME_DELIMITER.size();
        foundPos = fileName.find(FILENAME_DELIMITER, startPos);
    }
    string tmpStr = fileName.substr(startPos, foundPos - startPos);
    vec.push_back(std::stoi(tmpStr));

    return vec.size() == FILENAME_SPLITS_NUMBER;
}

int learning_data::getRound(string fileName)
{
    std::vector<int> vec;
    if(!splitFileName(fileName, vec))
    {
        return -1;
    }

    return vec[FILENAME_ROUND_INDEX];
}

int learning_data::getClientNumber(string fileName)
{
    std::vector<int> vec;
    if(!splitFileName(fileName, vec))
    {
        return -1;
    }

    return vec[FILENAME_CLIENT_NUMBER_INDEX];
}

int learning_data::getFeatureSerial(string fileName)
{
    std::vector<int> vec;
    if(!splitFileName(fileName, vec))
    {
        return -1;
    }
    return vec[FILENAME_FEATURE_SERIAL_INDEX];
}

void learning_data::readAllFileName(int clientID, int round) 
{
	struct dirent * filename;    // return value for readdir()
 	DIR * dir;                   // return value for opendir()
	dir = opendir(dirPath.c_str());
	if( NULL == dir )
	{
		cout<<"Can not open dir "<< dirPath <<endl;
		return;
	}

    char dot[3] = "."; 
    char dotdot[6] = "..";
	while( ( filename = readdir(dir) ) != NULL )
	{
		if( strcmp(filename->d_name, dot ) == 0 || strcmp( filename->d_name , dotdot) == 0)
        {
            continue;
        }

        if ( filename->d_type == DT_DIR )
        {
            continue;
        }

        std::vector<int> splits;
        if(!splitFileName(filename->d_name, splits))
        {
            continue;
        }
        if(splits[FILENAME_CLIENT_NUMBER_INDEX] != clientID)
        {
            continue;
        }
        if(splits[FILENAME_ROUND_INDEX] != round)
        {
            continue;
        }

        setRound.insert(splits[FILENAME_ROUND_INDEX]);
        feature_info_t t;                            // = {splits[FILENAME_ROUND_INDEX], splits[FILENAME_CLIENT_NUMBER_INDEX], splits[FILENAME_FEATURE_SERIAL_INDEX]};
        t.round = splits[FILENAME_ROUND_INDEX];
        t.clientNumber = splits[FILENAME_CLIENT_NUMBER_INDEX];
        t.featureSerialNumber = splits[FILENAME_FEATURE_SERIAL_INDEX];

		allFeatureInfos.insert(t);
	}

    closedir(dir);
}

void learning_data::parseAllFileName(int clientID, int round) 
{
    resetEnv();
    readAllFileName(clientID, round);
}

void learning_data::getAllRound(std::set<int, intComp>& allRound)
{
    for (std::set<int>::iterator it = setRound.begin(); it != setRound.end(); ++it)
    {
        allRound.insert(*it);
    }
}

void learning_data::resetEnv()
{
    setRound.clear();
    allFeatureInfos.clear();
    roundFileNameInfos.clear();
    featureSerials.clear();
    featureElementCount.clear();
    featureDataCount.clear();
}

int learning_data::parseRoundFileName(int round, std::set<int> &keys)
{
    // resetEnv();
    for (std::set<feature_info_t>::iterator it = allFeatureInfos.begin(); it != allFeatureInfos.end(); ++it)
    {
        if(it->round != round) 
        {
            continue;
        }

        std::set<int>::iterator itSet = keys.find(it->featureSerialNumber);
        if(itSet == keys.end())
        {
            continue;
        }

        roundFileNameInfos.insert(*it);
        featureSerials.insert(it->featureSerialNumber);
        parseFileContent(*it);
    }
}

int learning_data::parseFileContent(feature_info_t fileNameInfo)
{
    int dataCount = 1;
    if(featureDataCount.find(fileNameInfo.featureSerialNumber) != featureDataCount.end())
    {
        dataCount = featureDataCount[fileNameInfo.featureSerialNumber] + 1;
    }
    featureDataCount[fileNameInfo.featureSerialNumber] = dataCount;

    if(featureElementCount.find(fileNameInfo.featureSerialNumber) != featureElementCount.end())
    {
        return 0;
    }

    string filePath = getFilePath(fileNameInfo);

    std::cout << "[learning_data::parseFileContent]file patch:" << filePath << std::endl;

    std::vector<double> vecFileData;
    readFileData(filePath.c_str(), vecFileData);
    featureElementCount[fileNameInfo.featureSerialNumber] = vecFileData.size();

    return 0;
}

int learning_data::readFileData(const char* filePath, std::vector<double>& vecFileData)
{
    std::ifstream t(filePath);  
    std::stringstream buffer;  
    buffer << "[" << t.rdbuf()  << "]";
    t.close();

    std::string contents(buffer.str());
    std::vector<double> vecContent = json::parse(contents);
    vecFileData.swap(vecContent);

    return 0;
}

int learning_data::computeOneFeatureBodySize(int featureSerial)
{
    return featureElementCount[featureSerial] * sizeof(double);
}

int learning_data::computeOneFeatureDataSize(int featureSerial)
{
    return sizeof(feature_data_t) + computeOneFeatureBodySize(featureSerial);
}

int learning_data::computeFeatureDataSize(int featureSerial)
{
    return featureDataCount[featureSerial] * computeOneFeatureDataSize(featureSerial);
}

int learning_data::computeAllFeatureDataSize(std::set<int> &keys)
{
    int dataSize = 0;
    for (std::set<int>::iterator it = featureSerials.begin(); it != featureSerials.end(); ++it)
    {
        std::set<int>::iterator itSet = keys.find(*it);
        if(itSet == keys.end())
        {
            continue;
        }
        dataSize += computeFeatureDataSize(*it);
    }

    return dataSize;
}

request_data_header_t * learning_data::getRoundAndKeyData(int round, std::string strClientID, std::set<int> &keys)
{
    uint32_t clientID = std::atoll(strClientID.c_str());

    parseRoundFileName(round, keys);

    map<int, int>::iterator mapIter = featureElementCount.begin();
    while(mapIter != featureElementCount.end()) {
        cout << mapIter->first << " : " << mapIter->second << endl;
        mapIter++;
    }

    mapIter = featureDataCount.begin();
    while(mapIter != featureDataCount.end()) {
        cout << mapIter->first << " : " << mapIter->second << endl;
        mapIter++;
    }

    int allDataSize = computeAllFeatureDataSize(keys);

    request_data_header_t *p_req = (request_data_header_t *)malloc(sizeof(request_data_header_t) + allDataSize);
    memset(p_req, 0, sizeof(request_data_header_t) + allDataSize);

    int clientIDSize = strClientID.size();
    if(clientIDSize > sizeof(p_req->clientID))
    {
        clientIDSize = sizeof(p_req->clientID) - 1;
    }
    memcpy(p_req->clientID, strClientID.c_str(), clientIDSize);
    p_req->currRound = round;
    p_req->size = allDataSize;

    feature_data_t *p_featureData = (feature_data_t *)p_req->body;

    for (std::set<feature_info_t>::iterator it = allFeatureInfos.begin(); it != allFeatureInfos.end(); ++it)
    {

        if(it->round != round || it->clientNumber != clientID) 
        {
            continue;
        }

        std::set<int>::iterator itSet = keys.find(it->featureSerialNumber);
        if(itSet == keys.end())
        {
            continue;
        }

        string filePath = getFilePath(*it);
        std::vector<double> vecFileData;
        readFileData(filePath.c_str(), vecFileData);
        if(vecFileData.size() != featureElementCount[it->featureSerialNumber])  // 判断元素个数是否正确
        {
            std::cout << "error:" << filePath << std::endl;
            continue; 
        }

        p_req->featureTotalCount = p_req->featureTotalCount + 1;

        int bodySize = computeOneFeatureBodySize(it->featureSerialNumber);
        memcpy(p_featureData->body, &vecFileData[0], bodySize);

        p_featureData->featureInfo.round = it->round;
        p_featureData->featureInfo.clientNumber = it->clientNumber;
        p_featureData->featureInfo.clientCount = 1;
        p_featureData->featureInfo.elementNumber = featureElementCount[it->featureSerialNumber];
        p_featureData->featureInfo.featureSerialNumber = it->featureSerialNumber;
        p_featureData->elementNumber = featureElementCount[it->featureSerialNumber];
        p_featureData->size = bodySize;

        int dataSize = computeOneFeatureDataSize(it->featureSerialNumber);
        p_featureData = (feature_data_t *)((void*)p_featureData + dataSize);
    }

    return p_req;
}
