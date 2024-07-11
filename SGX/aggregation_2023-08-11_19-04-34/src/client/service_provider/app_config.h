#ifndef _APP_CONFIG_H
#define _APP_CONFIG_H

#include <string>

class app_config
{
private:
    /* data */
public:
    app_config();
    ~app_config();

    static app_config instance;
public:
	static app_config* getInstance();
public:
    bool readConfig(std::string clientID);
    std::string getClientID();

private:
    std::string m_clientID;
};



#endif