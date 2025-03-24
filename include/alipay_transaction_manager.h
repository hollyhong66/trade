#pragma once

#include "alipay_transaction.h"
#include <unordered_map>
#include <mutex>
#include <memory>
#include <vector>

// 事务状态
enum class TransactionStatus {
    INIT,           // 初始状态
    STARTED,        // 已开始
    PREPARED,       // 已准备
    COMMITTED,      // 已提交
    ROLLED_BACK,    // 已回滚
    FAILED          // 失败
};

// 事务记录
struct TransactionRecord {
    std::string xid;
    TransactionStatus status;
    uint64_t create_time;
    uint64_t update_time;
    std::string order_no;
    std::vector<std::string> participants; // 参与者列表
};

class AlipayTransactionManager {
public:
    static AlipayTransactionManager& getInstance();

    // 事务操作
    bool startTransaction(const std::string& orderNo, 
                        std::shared_ptr<AlipayTransaction>& transaction);
    bool prepareTransaction(const std::string& xid);
    bool commitTransaction(const std::string& xid);
    bool rollbackTransaction(const std::string& xid);

    // 事务恢复
    void recoverTransactions();
    void cleanupTransactions(uint64_t beforeTime);

    // 事务状态查询
    TransactionStatus getTransactionStatus(const std::string& xid);
    std::vector<TransactionRecord> getPendingTransactions();

private:
    AlipayTransactionManager();
    ~AlipayTransactionManager();

    // 事务表操作
    bool createTransactionTable();
    bool saveTransactionRecord(const TransactionRecord& record);
    bool updateTransactionStatus(const std::string& xid, TransactionStatus status);

    std::mutex mutex_;
    std::unordered_map<std::string, TransactionRecord> active_transactions_;
    MYSQL* conn_;
}; 