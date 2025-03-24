#pragma once

#include <string>
#include <memory>
#include <vector>

// 商户类型基类
class MerchantType {
public:
    virtual ~MerchantType() = default;
    
    // 设置下一个处理者
    void setNext(std::shared_ptr<MerchantType> next) {
        next_ = next;
    }
    
    // 处理商户逻辑
    virtual bool process(const std::string& merchantId) = 0;
    
    // 获取商户类型
    virtual std::string getType() const = 0;
    
    // 获取费率范围
    virtual std::pair<double, double> getFeeRange() const = 0;
    
    // 获取结算周期
    virtual std::vector<std::string> getSettlementCycles() const = 0;
    
protected:
    std::shared_ptr<MerchantType> next_;
};

// 普通商户
class NormalMerchant : public MerchantType {
public:
    bool process(const std::string& merchantId) override;
    std::string getType() const override { return "NORMAL"; }
    std::pair<double, double> getFeeRange() const override { 
        return {0.006, 0.01}; // 0.6% - 1%
    }
    std::vector<std::string> getSettlementCycles() const override {
        return {"T+1", "T+2"};
    }
};

// 服务商
class ISVMerchant : public MerchantType {
public:
    bool process(const std::string& merchantId) override;
    std::string getType() const override { return "ISV"; }
    std::pair<double, double> getFeeRange() const override { 
        return {0.004, 0.008}; // 0.4% - 0.8%
    }
    std::vector<std::string> getSettlementCycles() const override {
        return {"T+1", "WEEKLY", "MONTHLY"};
    }
};

// 子商户
class SubMerchant : public MerchantType {
public:
    bool process(const std::string& merchantId) override;
    std::string getType() const override { return "SUB"; }
    std::pair<double, double> getFeeRange() const override { 
        return {0.005, 0.009}; // 0.5% - 0.9%
    }
    std::vector<std::string> getSettlementCycles() const override {
        return {"T+1"};
    }
    
    void setParentId(const std::string& parentId) {
        parent_id_ = parentId;
    }
    
    std::string getParentId() const {
        return parent_id_;
    }

private:
    std::string parent_id_; // 父商户ID
}; 