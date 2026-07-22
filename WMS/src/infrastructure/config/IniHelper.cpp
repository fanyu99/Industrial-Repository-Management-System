// 配置助手 IniHelper
#include "IniHelper.h"
#include <QFile>

// 构造
IniHelper::IniHelper(QString filePath)
    : filePath_(filePath)
    , settings_(filePath, QSettings::IniFormat)
{
}

// 判断是否有效
bool IniHelper::isValid() const
{
    return settings_.status() == QSettings::NoError && QFile::exists(filePath_); // 检查配置文件是否存在/是否有效
}

// 获取配置文件路径
QString IniHelper::filePath() const
{
    return filePath_;
}

// 判断是否存在指定的key
bool IniHelper::contains(const QString& key) const
{
    return settings_.contains(key);
}

// 读取key对应的字符串值
QString IniHelper::readString(const QString& key, const QString& defaultValue) const
{
    return settings_.value(key, defaultValue).toString();
}

// 读取key对应的整数值
int IniHelper::readInt(const QString& key, int defaultValue) const
{
    return settings_.value(key, defaultValue).toInt();
}

// 读取key对应的整数值, 无默认值
int IniHelper::readInt(const QString& key) const
{
    return settings_.value(key).toInt();
}
// 读取key对应的布尔值
bool IniHelper::readBool(const QString& key, bool defaultValue) const
{
    return settings_.value(key, defaultValue).toBool();
}
// 读取key对应的布尔值, 无默认值
bool IniHelper::readBool(const QString& key) const
{
    return settings_.value(key).toBool();
}

ConfigValueResult IniHelper::readValue(const QString& key, ConfigValueType valueType) const
{
    ConfigValueResult result;
    if (!this->contains(key)) {
        result.error = ConfigError::KeyNotFound;
        result.errorMessage = QStringLiteral("键不存在");
        result.value = QVariant();
        result.success = false;
        return result;
    } else {
        switch (valueType) {
        case ConfigValueType::String: {
            result.value = settings_.value(key);
            if ((result.value.canConvert<QString>() || result.value.canConvert<std::string>()) && result.value.isValid()) {
                result.success = true;
            } else {
                result.error = ConfigError::InvalidValue;
                result.errorMessage = QStringLiteral("无效值/非字符串");
            }
            break;
        }
        case ConfigValueType::Int: {
            bool ok = false;
            const int value = settings_.value(key).toInt(&ok);
            if (ok) {
                result.value = value;
                result.success = true;
            } else {
                result.error = ConfigError::InvalidValue;
                result.errorMessage = QStringLiteral("无效值/非整数");
            }
            break;
        }
        case ConfigValueType::Bool: {
            result.value = settings_.value(key);
            if (result.value.canConvert<bool>() && result.value.isValid()) {
                result.success = true;
            } else {
                result.error = ConfigError::InvalidValue;
                result.errorMessage = QStringLiteral("无效值/非布尔值");
            }
            break;
        }
        default:
            result.error = ConfigError::InvalidValue;
            result.errorMessage = QStringLiteral("未知值类型");
            break;
        }
    }
    return result;
}