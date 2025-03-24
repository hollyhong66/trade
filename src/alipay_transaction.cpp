#include "alipay_transaction.h"
#include <sstream>
#include <chrono>
#include <iomanip>
#include <random>

AlipayTransaction::AlipayTransaction() : conn(nullptr) {}

AlipayTransaction::~AlipayTransaction() {
    if (conn) {
        mysql_close(conn);
    }
}

bool AlipayTransaction::connectDB(const char* host, const char* user, 
                                const char* password, const char* db) {
    conn = mysql_init(nullptr);
    if (!conn) return false;
    
    if (!mysql_real_connect(conn, host, user, password, db, 0, nullptr, 0)) {
        return false;
    }
    
    mysql_set_character_set(conn, "utf8mb4");
    return true;
}

bool AlipayTransaction::beginTransaction(const std::string& xid) {
    if (!conn) return false;
    
    current_xid_ = xid;
    std::string start_query = "XA START '" + current_xid_ + "'";
    return mysql_query(conn, start_query.c_str()) == 0;
}

bool AlipayTransaction::prepareTransaction() {
    if (!conn || current_xid_.empty()) return false;
    
    std::string prepare_query = "XA END '" + current_xid_ + "'";
    if (mysql_query(conn, prepare_query.c_str()) != 0) return false;
    
    prepare_query = "XA PREPARE '" + current_xid_ + "'";
    return mysql_query(conn, prepare_query.c_str()) == 0;
}

bool AlipayTransaction::commitTransaction() {
    if (!conn || current_xid_.empty()) return false;
    
    std::string commit_query = "XA COMMIT '" + current_xid_ + "'";
    bool result = mysql_query(conn, commit_query.c_str()) == 0;
    current_xid_.clear();
    return result;
}

bool AlipayTransaction::rollbackTransaction() {
    if (!conn || current_xid_.empty()) return false;
    
    std::string rollback_query = "XA ROLLBACK '" + current_xid_ + "'";
    bool result = mysql_query(conn, rollback_query.c_str()) == 0;
    current_xid_.clear();
    return result;
}

std::string AlipayTransaction::generateXID(const std::string& prefix) {
    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);
    
    std::stringstream ss;
    ss << prefix << "_" << now_ms << "_" << dis(gen);
    return ss.str();
} 