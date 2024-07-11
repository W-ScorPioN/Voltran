#ifndef _REQUEST_DATA_HEADER_H
#define _REQUEST_DATA_HEADER_H

#include <stdint.h>

#define CLIENT_ID_SIZE         32

typedef struct _feature_info_t
{
    // uint32_t taskID;             // 任务id
    uint32_t round;                 // 第几轮
    uint32_t clientNumber;          // 在读取客户端读取文件数据时，客户端编号
    uint32_t clientCount;           // sgx中聚合时，参与聚合的客户端数量
    uint32_t elementNumber;         // 该特征的元素个数
    uint32_t featureSerialNumber;   // 特征编号
} feature_info_t;

typedef struct _feature_data_t
{
    feature_info_t featureInfo;
    uint32_t elementNumber;     // body中包含的元素个数
    uint32_t size;
    uint8_t body[];
} feature_data_t;

typedef struct _feature_data_info_t
{
    uint32_t featureSerialNumber;   // 特征编号
    uint32_t elementCount;          // 元素个数
} feature_data_info_t;

typedef struct _request_data_header_t
{
    uint32_t currRound;                     // 当前轮
    uint32_t featureTotalCount;             // body中包含feature_data_t的个数
    uint8_t  clientID[CLIENT_ID_SIZE];      // 客户id 或者 sgxID
    uint32_t size;
    uint8_t  body[];
} request_data_header_t;

typedef struct _aggregating_w_t_1_header_t
{
    uint32_t currRound;             // 当前轮
    uint32_t featureTotalCount;     // 特征数量
    uint32_t size;
    uint8_t  body[];      
}aggregating_w_t_1_header_t;


#endif