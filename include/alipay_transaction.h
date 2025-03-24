#pragma once

#include <string>
#include <mysql/mysql.h>

class AlipayTransaction {
public:
    AlipayTransaction();
    ~AlipayTransaction();

    // 数据库连接
    bool connectDB(const char* host, const char* user, 
                  const char* password, const char* db);

    // XA事务操作
    bool beginTransaction(const std::string& xid);
    bool prepareTransaction();
    bool commitTransaction();
    bool rollbackTransaction();

    // 获取数据库连接
    MYSQL* getConnection() const { return conn; }
    
    // 生成XID
    static std::string generateXID(const std::string& prefix);

private:
    MYSQL* conn;
    std::string current_xid_;
}; 