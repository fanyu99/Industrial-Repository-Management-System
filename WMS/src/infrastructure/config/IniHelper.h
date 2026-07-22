// 配置助手 IniHelper
#pragma once
#include <QSettings>
#include <optional>
#include <string>
// 配置 错误类型
enum class ConfigError {
    None, // 无配置错误
    KeyNotFound, // 键不存在
    InvalidValue, // 无效值
    UnknownError, // 未知错误
};
// 配置 值类型
enum class ConfigValueType {
    String, // 字符串值
    Int, // 整数值
    Bool, // 布尔值
    Else, // 其他值
};
// 配置 值结果
struct ConfigValueResult {
    bool success { false };
    ConfigError error { ConfigError::None };
    QString errorMessage;
    QVariant value;
};
// 配置助手 IniHelper
class IniHelper {
private:
    QString filePath_;
    QSettings settings_;

public:
    explicit IniHelper(QString filePath);
    ~IniHelper() = default;
    [[nodiscard]] QString readString(const QString& key, const QString& defaultValue = QString()) const; // 读取key对应的字符串值
    [[nodiscard]] int readInt(const QString& key, int defaultValue) const; // 读取key对应的整数值
    [[nodiscard]] int readInt(const QString& key) const; // 读取key对应的整数值, 无默认值
    [[nodiscard]] bool readBool(const QString& key, bool defaultValue) const; // 读取key对应的布尔值
    [[nodiscard]] bool readBool(const QString& key) const; // 读取key对应的布尔值, 无默认值
    [[nodiscard]] bool isValid() const; // 判断配置是否有效
    [[nodiscard]] QString filePath() const; // 获取配置文件路径
    [[nodiscard]] bool contains(const QString& key) const; // 判断是否存在指定的key
    [[nodiscard]] ConfigValueResult readValue(const QString& key, ConfigValueType valueType = ConfigValueType::String) const; // 读取key对应的值
};
