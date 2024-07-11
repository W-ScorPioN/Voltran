#ifndef _LEARNING_DATA_H
#define _LEARNING_DATA_H

#include <set>
#include <vector>
#include <map>
#include <string>

#include "request_data_header.h"

using namespace std;

struct intComp {
	bool operator() (const int& lhs, const int& rhs) const{
		return lhs < rhs;
	}
};

struct fileNameComp {
    bool operator() (const feature_info_t& lFileName, const feature_info_t& rFileName) const {
        if(lFileName.round != rFileName.round) 
        {
            return lFileName.round < rFileName.round;
        }
        if(lFileName.featureSerialNumber != rFileName.featureSerialNumber) 
        {
            return lFileName.featureSerialNumber < rFileName.featureSerialNumber;
        }
		return lFileName.clientNumber < rFileName.clientNumber;
	}
};

class learning_data {
public:
    learning_data();
    ~learning_data();
private:
    int splitFileName(string fileName, std::vector<int>& vec);
    string getFilePath(feature_info_t featureInfo);
    void readAllFileName(int clientID, int round);
    int getRound(string fileName);
    int getClientNumber(string fileName);
    int getFeatureSerial(string fileName);
    int parseRoundFileName(int round, std::set<int> &keys);
    int parseFileContent(feature_info_t featureInfo);

    int computeOneFeatureBodySize(int featureSerial);
    int computeOneFeatureDataSize(int featureSerial);
    int computeFeatureDataSize(int featureSerial);
    int computeAllFeatureDataSize(std::set<int> &keys);

    int readFileData(const char* filePath, std::vector<double>& vecFileData);
    void resetEnv();
public:
    void setDirPath(string dirPath);
    void parseAllFileName(int clientID, int round);
    void getAllRound(std::set<int, intComp>& allRound);
    request_data_header_t * getRoundAndKeyData(int round, std::string strClientID, std::set<int> &keys);
private:
    string dirPath;
    int round;
    int feature;
    std::set<int, intComp> setRound;
    std::set<feature_info_t, fileNameComp> allFeatureInfos;
    std::set<feature_info_t, fileNameComp> roundFileNameInfos;
    std::set<int, intComp> featureSerials;
    std::map<int, int> featureElementCount;     // 每个客户端的特征数组中，元素个数map：特征序列号 -- 元素个数
    std::map<int, int> featureDataCount;        // 每个特征有多少个客户端生成训练数据

public:
    static const string FILENAME_DELIMITER;             //文件名中，分隔符："_"
    static const int FILENAME_SPLITS_NUMBER;            //文件名中，按分隔符分割后的数量，文件名格式：Roun_client-number_feature-number
    static const int FILENAME_ROUND_INDEX;
    static const int FILENAME_CLIENT_NUMBER_INDEX;
    static const int FILENAME_FEATURE_SERIAL_INDEX;

    static const string FILE_PATH_SPLITTER;             //文件名中，路径的分割符，linux下为："/"

    static const int ERROR_ROUND;
    
};


#endif