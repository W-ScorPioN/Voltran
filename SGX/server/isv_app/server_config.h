#ifndef _APP_SERVER_CONFIG_H
#define _APP_SERVER_CONFIG_H

#include <string>

class server_config
{
private:
    server_config(/* args */);
    ~server_config();

	static server_config instance;
public:
	static server_config* getInstance();
public:
    bool readConfig(std::string appCfgFileName);
    std::string getSgxID();
    int getPort();
    int getParallelAggNumber();
    int getTcsNum();
    std::string getChainServerUrl();

private:
    std::string m_ipAddr;
    int m_port;

    int m_parallelAggNumber;  // 并行聚合线程拷贝数据到enclave数目
    int m_tcsNum;             // 编译enclave时，TCSNum配置

    std::string m_chainServerUrl;
};



#endif