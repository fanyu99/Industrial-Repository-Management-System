# C++ Qt + MySQL 企业仓库管理系统 — 完整学习与实现计划

> **目标：** 在 1-2 个月内从零构建一个能写入简历的生产级 C++ Qt 桌面应用  
> **技术栈：** C++17/23 · Qt 6.11 · MySQL 8.0 · CMake  
> **项目名称：** WMS (Warehouse Management System)  
> **工作目录：** `D:\Desktop\WMS\`

---

## 目录

- [1. 项目概述](#1-项目概述)
- [2. 环境准备 (Day 0)](#2-环境准备-day-0)
- [3. 分阶段学习计划](#3-分阶段学习计划)
- [4. 项目架构设计](#4-项目架构设计)
- [5. 数据库 Schema 设计](#5-数据库-schema-设计)
- [6. 分步实现计划 (约 27 天)](#6-分步实现计划-约-27-天)
- [7. 关键设计决策](#7-关键设计决策)
- [8. 常见陷阱与解决方案](#8-常见陷阱与解决方案)
- [9. 面试准备要点](#9-面试准备要点)

---

## 1. 项目概述

### 1.1 项目定位

构建一个**企业级物资进销存管理系统**，支持多角色权限管理、物资出入库、实时库存追踪、低库存预警、数据仪表盘和报表导出。

### 1.2 核心功能清单

| 模块 | 功能 | 涉及技术点 |
|------|------|-----------|
| 🔐 用户认证 | 登录/登出、SHA-256 加盐哈希、角色权限（Admin/Manager/Operator） | `QCryptographicHash`, Session 管理 |
| 📦 物资管理 | 物资 CRUD、分类/单位管理、搜索分页 | 自定义 `QAbstractTableModel` |
| 📥 入库管理 | 入库单创建+确认、批次管理、库存自动更新 | 数据库事务 |
| 📤 出库管理 | 出库单创建+确认、库存校验、库存扣减 | 事务+行级锁 |
| 📊 库存管理 | 实时库存查询、多条件筛选、低库存/超量预警 | 聚合查询 |
| 📈 数据仪表盘 | 4 个图表(柱状/折线/饼图)、概览统计卡片 | `Qt Charts` |
| 📋 操作日志 | 全操作审计追踪、日志查询 | 软删除、外键关联 |
| 📑 报表导出 | CSV/Excel 导出 | 文件 I/O、编码处理 |
| 🎨 UI | 深色主题、QStackedWidget 多页导航、QSS 样式 | `Qt Style Sheets` |

### 1.3 为什么这个项目能写入简历？

| 简历亮点 | 说明 |
|----------|------|
| **分层架构** | UI → Service → DAO → DB，清晰解耦，面试必问 |
| **数据库连接池** | 手写单例连接池，复用你的线程池设计经验，展示并发编程能力 |
| **事务处理** | 入库/出库确认涉及多表操作，用事务保证数据一致性 |
| **自定义 Model/View** | 不依赖 Qt 内置 SQL Model，手写 `QAbstractTableModel` 子类 |
| **安全实践** | SHA-256 + 随机盐、Prepared Statements 防注入、软删除 |
| **CMake 构建** | 多目录 CMake + Qt6 模块化管理，而非 `.pro` 文件 |
| **现代 C++** | 智能指针、RAII、模板基类、lambda、`std::optional` |

---

## 2. 环境准备 (Day 0)

> ⏱ 预计耗时：3-4 小时

### 2.1 当前环境检查

| 组件 | 状态 | 路径/版本 |
|------|------|-----------|
| Qt 6.11.1 | ✅ 已安装 | `D:\QT\6.11.1\mingw_64\` (MinGW 64-bit) |
| CMake 3.30.5 | ✅ 已安装 | `D:\QT\Tools\CMake_64\cmake.exe` |
| MySQL 8.0 | ✅ 已安装 | `C:\Program Files\MySQL\MySQL Server 8.0\` |
| MySQL ODBC Connector | ❌ 需要安装 | [下载地址](https://dev.mysql.com/downloads/connector/odbc/) |
| Qt MySQL 原生驱动 | ❌ 不存在 | `qsqlmysql.dll` 未编译，走 ODBC 路线 |

### 2.2 安装步骤

#### Step 1: 安装 MySQL ODBC Connector

1. 访问 [MySQL Connector/ODBC 下载页](https://dev.mysql.com/downloads/connector/odbc/)
2. 选择 **Windows (x86, 64-bit), MSI Installer** 版本
3. 安装时选择 **"Unicode"** 驱动（不要选 ANSI）
4. 验证安装：打开"ODBC 数据源管理器 (64-bit)" → "驱动程序"标签 → 应看到 `MySQL ODBC 8.0 Unicode Driver`

#### Step 2: 启动 MySQL 并创建数据库

```bash
# 以管理员身份启动 MySQL 服务
net start MySQL80

# 登录 MySQL
mysql -u root -p

# 创建项目数据库
CREATE DATABASE wms DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
```

#### Step 3: 验证 Qt ODBC 连接

创建一个简单的测试程序，验证 `QODBC` 驱动能否连接 MySQL：

```cpp
// 测试代码 — 放在 D:\Desktop\WMS\tests\db_test\main.cpp
#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    // 查看可用驱动
    qDebug() << "Available drivers:" << QSqlDatabase::drivers();

    QSqlDatabase db = QSqlDatabase::addDatabase("QODBC");
    db.setDatabaseName("DRIVER={MySQL ODBC 8.0 Unicode Driver};"
                       "SERVER=127.0.0.1;"
                       "PORT=3306;"
                       "DATABASE=wms;"
                       "UID=root;"
                       "PWD=your_password;");

    if (db.open()) {
        qDebug() << "✅ Database connected!";

        QSqlQuery query(db);
        if (query.exec("SELECT 1")) {
            qDebug() << "✅ Query test passed!";
        }
    } else {
        qDebug() << "❌ Connection failed:" << db.lastError().text();
    }

    return 0;
}
```

**知识点：**
- Qt SQL 模块架构 (`QSqlDatabase`, `QSqlQuery`, `QSqlError`)
- ODBC 连接字符串格式
- 可用驱动列表 (`QSqlDatabase::drivers()`)

**学习资源：**
- 📖 [Qt SQL Database Drivers 官方文档](https://doc.qt.io/qt-6/sql-driver.html)
- 📖 [Qt QSqlDatabase 使用指南](https://doc.qt.io/qt-6/qsqldatabase.html)

---

## 3. 分阶段学习计划

### 阶段 1: Qt 核心技术补齐 (Day 1-4)

> ⏱ 预计耗时：12 小时  
> 🎯 目标：掌握 Model/View 架构、Qt Charts 图表、QSS 样式、多窗口导航

---

#### 1.1 QAbstractTableModel 深入 (Day 1, ~3h)

**为什么重要：** 这是整个项目的 UI 数据绑定基础。你的每个表格页面都用它。

**知识点清单：**
- [ ] `QAbstractTableModel` 必须重写的纯虚函数：`rowCount()`, `columnCount()`, `data()`
- [ ] 可选重写：`headerData()`, `flags()`, `sort()`, `roleNames()`
- [ ] `Qt::ItemDataRole` 的理解（`DisplayRole`, `ForegroundRole`, `BackgroundRole`, `UserRole`）
- [ ] 通知视图刷新：`beginResetModel()` / `endResetModel()`
- [ ] 增量通知：`beginInsertRows()` / `endInsertRows()`, `beginRemoveRows()` / `endRemoveRows()`
- [ ] `QTableView` 的基本配置：交替行颜色、列宽模式、选择行为

**练习任务：** 手写一个 `PersonTableModel`，用 `QVector<Person>` 作为数据源，绑定到 `QTableView`。支持排序和自定义前景色。

**学习资源：**
- 📖 [Qt Model/View Programming 官方文档](https://doc.qt.io/qt-6/model-view-programming.html) — **必读**，理解 Model/View/Delegate 三角关系
- 📖 [QAbstractTableModel 类文档](https://doc.qt.io/qt-6/qabstracttablemodel.html)
- 🎬 [B站: Qt Model/View 框架详解 (搜索关键词)](https://search.bilibili.com/all?keyword=Qt%20Model%20View%20%E6%A1%86%E6%9E%B6)

---

#### 1.2 QSqlTableModel 对比学习 (Day 2 上午, ~2h)

**为什么重要：** 了解 Qt 内置方案的长处与局限，才能理解为什么本项目选择自定义 Model。

**知识点清单：**
- [ ] `QSqlTableModel` 自动 CRUD：`select()`, `insertRow()`, `submitAll()`, `revertAll()`
- [ ] `QSqlRelationalTableModel` 外键关联 + `QSqlRelationalDelegate`
- [ ] `setFilter()` 条件过滤
- [ ] **局限：** 不支持分页 (`LIMIT/OFFSET` 需手动)、不支持复杂 JOIN、自定义显示逻辑受限

**学习资源：**
- 📖 [Qt QSqlTableModel 文档](https://doc.qt.io/qt-6/qsqltablemodel.html)
- 📖 [CSDN: Qt SQL Model 使用总结](https://blog.csdn.net/qq_41453285/article/details/100534650)

---

#### 1.3 Qt Charts 图表 (Day 2 下午, ~2h)

**为什么重要：** 仪表盘的数据可视化核心，面试时展示效果好。

**知识点清单：**
- [ ] 图表组件关系：`QChartView` → `QChart` → `QAbstractSeries` → 数据点
- [ ] 柱状图 (`QBarSeries`, `QBarSet`, `QBarCategoryAxis`)
- [ ] 折线图 (`QLineSeries`, `QDateTimeAxis`, `QValueAxis`)
- [ ] 饼图 (`QPieSeries`, `QPieSlice`)
- [ ] 图表布局：`setTitle()`, `setAnimationOptions()`, `legend()`
- [ ] 暗色主题适配：`setBackgroundBrush()`, `setTitleBrush()`, 轴线颜色

**练习任务：** 用硬编码数据画 3 个 demo 图表（柱状图、折线图、饼图），每个图表都用深色背景。

**学习资源：**
- 📖 [Qt Charts Overview 官方文档](https://doc.qt.io/qt-6/qtcharts-overview.html)
- 🎬 [B站: Qt Charts 数据可视化教程](https://search.bilibili.com/all?keyword=Qt%20Charts%20%E5%9B%BE%E8%A1%A8)
- 📖 [Qt Charts Examples](https://doc.qt.io/qt-6/qtcharts-examples.html) — 官方示例代码

---

#### 1.4 QSS 样式表 (Day 3 上午, ~3h)

**为什么重要：** 一个好看的 UI 是面试第一印象。深色主题是当前行业趋势。

**知识点清单：**
- [ ] QSS 语法基础：选择器 (`QPushButton`, `#myButton`, `.warning`)、属性、伪状态 (`:hover`, `:pressed`, `:checked`)
- [ ] 盒模型：`margin`, `border`, `padding`, `content`
- [ ] 常用控件样式：`QTableView`, `QPushButton`, `QLineEdit`, `QComboBox`, `QTabWidget`, `QScrollBar`
- [ ] 全局样式 vs 局部样式加载
- [ ] 色板设计：深色主题常用色板 (参考 Catppuccin Mocha / Dracula)

**练习任务：** 为你之前的 NotePad 项目写一套深色主题 QSS。

**学习资源：**
- 📖 [Qt Style Sheets Reference 官方文档](https://doc.qt.io/qt-6/stylesheet-reference.html) — **最常用参考**
- 📖 [Qt Style Sheets Examples](https://doc.qt.io/qt-6/stylesheet-examples.html)
- 🎨 [Catppuccin 色板](https://catppuccin.com/palette) — 本项目深色主题参考色板

---

#### 1.5 多窗口导航架构 (Day 3 下午, ~2h)

**为什么重要：** 企业级应用不可能是单窗口。正确的导航架构决定代码的可维护性。

**知识点清单：**
- [ ] `QStackedWidget` 页面切换（比 `QTabWidget` 更灵活）
- [ ] 侧边菜单栏 `QListWidget` + `QStackedWidget` 联动
- [ ] 页面工厂模式：按菜单 key 创建/缓存页面
- [ ] 页面间通信：信号/槽跨页面通知 (如"修改了物资"→仪表盘刷新)

**练习任务：** 做一个 3 页的导航 Demo（首页/设置/关于），左侧 `QListWidget` 菜单点击切换右侧 `QStackedWidget`。

**学习资源：**
- 🎬 [B站: Qt QStackedWidget 多页面切换](https://search.bilibili.com/all?keyword=Qt%20QStackedWidget%20%E5%A4%9A%E9%A1%B5%E9%9D%A2)

---

### 阶段 2: 核心基础设施 (Day 4-6)

> ⏱ 预计耗时：10 小时  
> 🎯 目标：从线程池经验迁移到连接池，实现密码哈希、会话管理

---

#### 2.1 数据库连接池设计 (Day 4, ~4h) ⭐

**为什么重要：** 这是你面试时最大的技术亮点。将线程池的并发经验直接迁移到数据库层。

**知识点清单：**
- [ ] Qt 的线程-连接约束：**每个线程必须拥有自己的 `QSqlDatabase` 连接**
- [ ] 连接池核心数据结构：`QQueue<QString>` (存储连接名而非连接对象)
- [ ] `QMutex` + `QWaitCondition` 实现线程安全的 acquire/release
- [ ] RAII 包装器 `ScopedConnection`：构造时获取，析构时自动归还
- [ ] 连接健康检查：定时 `SELECT 1` ping
- [ ] 空闲连接超时回收
- [ ] 获取超时机制 (`QWaitCondition::wait(&mutex, timeoutMs)`)

**线程池 → 连接池 迁移对照：**

| 线程池概念 | 连接池对应 |
|-----------|-----------|
| `std::deque<std::function<void()>> tasks_` | `QQueue<QString> idleConnections_` |
| `cv.wait(lock, predicate)` 等任务 | `cv.wait(&mutex, timeout)` 等空闲连接 |
| `enqueue(task)` 提交任务 | `release(name)` 归还连接 |
| `std::atomic<int> busynum_` | `QAtomicInt activeCount_` |
| `~ThreadPool()` stop + join | `shutdown()` 关闭所有连接 |
| 启动时创建固定数量线程 | 初始化时创建最小连接数 |

**新增挑战（线程池没有的）：**
- 连接健康检查（MySQL `wait_timeout` 默认 8 小时断开）
- 获取超时处理（当所有连接都在用时）
- 动态扩容/缩容（按需创建，空闲回收）

**学习资源：**
- 📖 [Qt QSqlDatabase 线程安全说明](https://doc.qt.io/qt-6/threads-modules.html#threads-and-the-sql-module)
- 📖 [Qt QMutex 文档](https://doc.qt.io/qt-6/qmutex.html) + [QWaitCondition 文档](https://doc.qt.io/qt-6/qwaitcondition.html)
- 🎬 [B站: 数据库连接池设计模式](https://search.bilibili.com/all?keyword=%E6%95%B0%E6%8D%AE%E5%BA%93%E8%BF%9E%E6%8E%A5%E6%B1%A0%20C%2B%2B)

---

#### 2.2 密码安全 (Day 5 上午, ~2h)

**为什么重要：** 安全面试题高频考点。绝不能明文存密码。

**知识点清单：**
- [ ] SHA-256 哈希：`QCryptographicHash::hash(data, QCryptographicHash::Sha256)`
- [ ] 随机盐生成：`QRandomGenerator::global()->generate()` 生成 16 字节随机盐
- [ ] 盐+密码+胡椒(可选)拼接顺序：`salt + password` → SHA-256
- [ ] 验证流程：取用户记录中的盐 → 用同样的盐哈希输入密码 → 比对哈希值
- [ ] 密码强度校验（登录时，不是注册时强制）

**学习资源：**
- 📖 [QCryptographicHash 文档](https://doc.qt.io/qt-6/qcryptographichash.html)
- 📖 [OWASP Password Storage Cheat Sheet](https://cheatsheetseries.owasp.org/cheatsheets/Password_Storage_Cheat_Sheet.html) — 行业标准

---

#### 2.3 ConfigManager + SessionManager (Day 5 下午 + Day 6, ~4h)

**为什么重要：** 基础设施可复用组件，体现工程素养。

**ConfigManager 知识点：**
- [ ] 单例模式（你已掌握）
- [ ] INI/YAML 配置文件解析
- [ ] 默认值回退机制

**SessionManager 知识点：**
- [ ] 当前登录用户信息的存储和访问
- [ ] 角色枚举 (`enum class Role { Admin, Manager, Operator }`)
- [ ] 权限检查方法 `hasPermission(action)`

**学习资源：**
- 📖 [QSettings 文档](https://doc.qt.io/qt-6/qsettings.html) — 用于 INI 配置读写

---

### 阶段 3: 数据访问层 (Day 7-10)

> ⏱ 预计耗时：12 小时  
> 🎯 目标：建立 DAO 层设计模式，实现所有表的 CRUD

---

#### 3.1 DAO 层设计模式 (Day 7, ~3h)

**为什么重要：** 所有数据操作集中在这里，业务逻辑层的代码才能保持干净。

**知识点清单：**
- [ ] DAO (Data Access Object) 设计模式
- [ ] `BaseDAO` 公共基类：`getConnection()`, `querySingleValue()`, `querySingleRow()`
- [ ] Prepared Statements 统一使用：`QSqlQuery::prepare()` + `bindValue()`
- [ ] 分页查询模式：`SELECT ... WHERE ... LIMIT :limit OFFSET :offset`
- [ ] 搜索模式：`WHERE (name LIKE :kw OR code LIKE :kw)`
- [ ] 双版本方法设计：独立版（自己获取连接）+ 事务版（接受外部连接引用）

**双版本方法设计模式：**

```cpp
// 独立使用版本 — Service 层非事务调用
bool InventoryDAO::increaseStock(int productId, int qty) {
    auto conn = ConnectionPool::instance().acquire();
    return increaseStock(conn.db(), productId, qty);  // 委托给事务版本
}

// 事务内使用版本 — Service 层事务内调用，传入同一个 db
bool InventoryDAO::increaseStock(QSqlDatabase &db, int productId, int qty) {
    QSqlQuery query(db);
    query.prepare("UPDATE inventory SET quantity = quantity + ? WHERE product_id = ?");
    query.addBindValue(qty);
    query.addBindValue(productId);
    return query.exec();
}
```

**学习资源：**
- 📖 [QSqlQuery 文档](https://doc.qt.io/qt-6/qsqlquery.html) — Prepared Statements 部分
- 📖 [OWASP SQL Injection Prevention](https://cheatsheetseries.owasp.org/cheatsheets/SQL_Injection_Prevention_Cheat_Sheet.html)

---

#### 3.2 实现 7 个 DAO 类 (Day 8-10, ~9h)

每个 DAO 实现标准 CRUD 方法 + 特有业务方法。

| DAO | 标准方法 | 特殊方法 |
|-----|---------|---------|
| `UserDAO` | CRUD + findAll | `findByUsername(username)` |
| `CategoryDAO` | CRUD + findAll | — |
| `ProductDAO` | CRUD + findAll(分页+搜索) | `findByCode(code)` |
| `InboundDAO` | CRUD + findAll(分页) | `findDetailsByOrderId(orderId)`, `insertDetail(db, ...)` |
| `OutboundDAO` | CRUD + findAll(分页) | `findDetailsByOrderId(orderId)`, `insertDetail(db, ...)` |
| `InventoryDAO` | findAll(分页+联合查询) | `increaseStock(db, ...)`, `decreaseStock(db, ...)`, `getLowStockAlerts()` |
| `OperationLogDAO` | findAll(分页) | `insertWithConnection(db, log)` — 接受外部连接的插入 |

**学习资源：**
- 📖 [MySQL JOIN 语法](https://dev.mysql.com/doc/refman/8.0/en/join.html) — DAO 中的联合查询
- 📖 [Qt SQL Programming Guide](https://doc.qt.io/qt-6/sql-programming.html) — Qt SQL 完整编程指南

---

### 阶段 4: 业务逻辑层 (Day 11-13)

> ⏱ 预计耗时：10 小时  
> 🎯 目标：实现核心业务流程，特别是入库/出库确认的事务处理

---

#### 4.1 AuthService 认证服务 (Day 11 上午, ~2h)

**知识点清单：**
- [ ] 登录流程：查询用户 → 验证密码哈希 → 检查软删除 → 创建 Session → 记录操作日志
- [ ] 登出流程：清除 Session → 记录操作日志
- [ ] 权限检查辅助方法

---

#### 4.2 入库服务 (Day 11 下午 + Day 12 上午, ~4h) ⭐

**为什么重要：** 这是整个项目最复杂的业务逻辑，涉及多表事务，面试必问。

**入库确认事务流程：**

```
begin transaction
  1. 更新 inbound_orders SET status='confirmed', confirmed_at=NOW()
  2. FOR EACH 明细行:
       a. INSERT INTO inbound_details (...)
       b. UPDATE inventory SET quantity = quantity + inbound_qty
          WHERE product_id = ? AND warehouse_name = ?
          IF 不存在 → INSERT INTO inventory
  3. INSERT INTO operation_logs (记录: 谁+何时+确认了哪个入库单)
  4. INSERT INTO inventory_alerts (如入库后库存仍低于安全库存，记录预警)
commit (失败则 rollback)
```

**知识点清单：**
- [ ] `QSqlDatabase::transaction()` / `commit()` / `rollback()`
- [ ] 事务内所有操作使用**同一个** `QSqlDatabase` 连接
- [ ] 异常安全：用 `try-catch` 或 RAII 事务守卫确保 rollback
- [ ] UPSERT 模式（`INSERT ... ON DUPLICATE KEY UPDATE`）用于库存更新

**学习资源：**
- 📖 [MySQL 事务文档](https://dev.mysql.com/doc/refman/8.0/en/commit.html)
- 📖 [MySQL INSERT ... ON DUPLICATE KEY](https://dev.mysql.com/doc/refman/8.0/en/insert-on-duplicate.html)

---

#### 4.3 出库服务 (Day 12 下午, ~2h) ⭐

与入库类似，但多了一个**库存校验**步骤：

```
begin transaction
  1. FOR EACH 明细行:
       a. SELECT quantity FROM inventory WHERE product_id=? AND warehouse_name=? FOR UPDATE
       b. IF current_qty < outbound_qty → ROLLBACK, 返回库存不足错误
  2. 更新 outbound_orders SET status='confirmed'
  3. FOR EACH 明细行:
       a. INSERT INTO outbound_details
       b. UPDATE inventory SET quantity = quantity - outbound_qty
  4. INSERT INTO operation_logs
  5. IF 出库后库存 < 安全库存 → INSERT INTO inventory_alerts
commit
```

**新增知识点：**
- [ ] `SELECT ... FOR UPDATE` 行级锁定，防止并发出库导致超卖
- [ ] 业务校验 → 操作 → 日志 → 预警 的标准事务链路

---

#### 4.4 InventoryService + DashboardService (Day 13, ~2h)

**库存服务：**
- [ ] 分页查询（JOIN products, categories 获取可读信息）
- [ ] 低库存预警查询 (`WHERE quantity < safety_stock`)
- [ ] 库存手动调整（盘点差异处理，走事务）

**仪表盘服务：**
- [ ] 概览统计：物资总数、库存总值、本月入库/出库量、低库存项数
- [ ] 最近 7 天入库/出库趋势数据（按天 GROUP BY）
- [ ] 品类库存占比数据（按分类 GROUP BY）
- [ ] 入库 Top 5 产品

---

### 阶段 5: UI 展示层 (Day 14-25)

> ⏱ 预计耗时：35 小时  
> 🎯 目标：10 个完整页面 + 深色主题 + 可交互的完整系统

---

#### 5.1 MainWindow 导航框架 (Day 14, ~3h)

**知识点清单：**
- [ ] `QMainWindow` 布局：`QListWidget` (左侧菜单) + `QStackedWidget` (右侧内容)
- [ ] 菜单项 `QListWidgetItem` 设置 icon + 文字
- [ ] 角色路由：`admin` 显示全部菜单，`manager`/`operator` 部分菜单禁用或隐藏
- [ ] HeaderBar：显示当前用户 + 角色 + 登出按钮
- [ ] 页面缓存：已打开的页面保持在 `QStackedWidget` 中，切换回来不重建

**设计模式 — 菜单路由表：**

```cpp
struct PageRoute {
    QString key;        // 页面唯一标识
    QString title;      // 菜单显示文字
    QString icon;       // 图标路径
    Role minRole;       // 最低访问角色
};

const QVector<PageRoute> ROUTES = {
    {"dashboard", "仪表盘", "dashboard.png", Role::Operator},
    {"products", "物资管理", "box.png", Role::Operator},
    {"inbound", "入库管理", "in.png", Role::Operator},
    {"outbound", "出库管理", "out.png", Role::Operator},
    {"inventory", "库存管理", "warehouse.png", Role::Manager},
    {"alerts", "库存预警", "alert.png", Role::Manager},
    {"logs", "操作日志", "log.png", Role::Admin},
    {"users", "用户管理", "user.png", Role::Admin},
    {"reports", "报表导出", "file.png", Role::Manager},
};
```

---

#### 5.2 LoginWidget (Day 14 下午, ~2h)

**设计：**
- 居中 400×300 登录卡片，深色背景
- 用户名 + 密码输入框
- "登录"按钮 + 错误提示 Label
- 简单动画：登录失败时红框抖动

---

#### 5.3 DashboardWidget (Day 15-16, ~6h)

**布局设计：**
```
┌────────────────────────────────────────────────┐
│  ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐  │
│  │物资总数│ │库存总值│ │本月入库│ │本月出库│  │  4个概览卡片
│  │ 1,234  │ │¥52万   │ │ 89笔   │ │ 67笔   │  │
│  └────────┘ └────────┘ └────────┘ └────────┘  │
│  ┌──────────────────┐ ┌──────────────────┐     │
│  │ 最近7天入库趋势    │ │ 最近7天出库趋势    │     │  2个柱状图
│  │ (QBarSeries)     │ │ (QBarSeries)     │     │
│  └──────────────────┘ └──────────────────┘     │
│  ┌──────────────────┐ ┌──────────────────┐     │
│  │ 品类库存占比 (饼图) │ │ 入库Top5 (水平柱)  │     │  饼图 + 水平柱状图
│  └──────────────────┘ └──────────────────┘     │
└────────────────────────────────────────────────┘
```

**练习关键点：**
- 概览卡片用 `QFrame` 自定义样式（圆角、阴影效果）
- 图表与 MySQL 真实数据绑定
- 深色主题下图表颜色适配

---

#### 5.4 通用 CRUD 页面模式 (Day 17-22, ~15h)

以下 6 个页面遵循**相同的 UI 模式**，每页约 2-3 小时：

| 页面 | 功能 |
|------|------|
| `ProductManagementWidget` | 物资 CRUD + 搜索 + 分页 + 分类/单位下拉 |
| `InboundManagementWidget` | 入库单列表 + 新建入库单 Dialog (主表+明细表) |
| `OutboundManagementWidget` | 出库单列表 + 新建出库单 Dialog (库存不足自动提示) |
| `InventoryWidget` | 库存列表 + 多条件筛选 + 手动调整库存 |
| `AlertWidget` | 低库存预警列表 + 一键生成补货入库单 |
| `OperationLogWidget` | 日志列表 + 按用户/操作类型/时间筛选 |

**每个 CRUD 页面的标准组件：**

```
┌────────────────────────────────────────────┐
│ 🔍 [搜索框_______________] [搜索] [重置]   │  搜索栏
│ [+ 新增物资] [批量删除]                    │  操作栏
├────────────────────────────────────────────┤
│ ┌──────────────────────────────────────┐   │
│ │ ID │ 编码  │ 名称  │ 分类 │ 库存  │…│   │  QTableView
│ │────│───────│───────│──────│───────│─│   │  + 自定义 Model
│ │  1 │ MAT01 │ 钢材  │ 原料 │  200  │  │   │
│ │ ...│ ...   │ ...   │ ...  │ ...   │  │   │
│ └──────────────────────────────────────┘   │
│ < 上一页  [1] [2] [3]  下一页 > 共 58 条  │  分页栏
└────────────────────────────────────────────┘
```

**新增/编辑 Dialog 模式：**

```
┌──────────────────────────────────┐
│  新增物资                     ✕  │
│──────────────────────────────────│
│  物资编码: [____________]        │
│  物资名称: [____________]        │
│  分类:     [______ ▼]           │
│  单位:     [______ ▼]           │
│  规格型号: [____________]        │
│  安全库存: [____________]        │
│  备注:     [____________]        │
│──────────────────────────────────│
│              [取消]  [保存]      │
└──────────────────────────────────┘
```

---

#### 5.5 高级功能页面 (Day 23-24, ~6h)

| 页面 | 额外知识点 |
|------|-----------|
| `UserManagementWidget` | 仅 Admin 可见、密码重置（不可查看原密码）、角色分配 |
| `ReportWidget` | `QFileDialog` 选择保存路径、CSV/Excel 导出 |

**CSV 导出要点：**
- 写入 UTF-8 BOM (`\xEF\xBB\xBF`)，确保 Excel 正确识别中文
- 字段加双引号转义

**Excel 导出要点：**
- 生成 HTML table 格式的 `.xls` 文件（轻量方案，无需 libxlsxwriter）
- 或使用 [QtXlsxWriter](https://github.com/QtExcel/QXlsx) 库（更正式的方案）

---

#### 5.6 QSS 深色主题整合 (Day 24 下午 - Day 25, ~4h)

**色板参考 (Catppuccin Mocha):**

| 颜色名 | 用途 | 色值 |
|--------|------|------|
| Base | 页面背景 | `#1e1e2e` |
| Surface0 | 卡片/表格背景 | `#313244` |
| Surface2 | 输入框/下拉框背景 | `#45475a` |
| Text | 正文颜色 | `#cdd6f4` |
| Subtext0 | 次要文字 | `#a6adc8` |
| Blue | 强调色/选中 | `#89b4fa` |
| Green | 成功/入库 | `#a6e3a1` |
| Red | 错误/出库/预警 | `#f38ba8` |
| Yellow | 警告 | `#f9e2af` |

**QSS 文件组织：** `src/ui/resources/style.qss` (全局加载)

---

### 阶段 6: 导出、测试与收尾 (Day 26-27)

> ⏱ 预计耗时：8 小时

---

#### 6.1 报表导出 (Day 26 上午, ~3h)

- 实现 `ExportUtils` 工具类
- CSV 导出（UTF-8 BOM）
- Excel 导出（HTML table 格式或 QXlsx 库）
- 支持导出：库存明细、入库/出库记录、操作日志

---

#### 6.2 全面测试 (Day 26 下午, ~3h)

**功能测试清单 (15+ 测试场景)：**

| # | 测试场景 | 预期结果 |
|---|---------|---------|
| 1 | Admin 登录 | 看到全部 9 个菜单 |
| 2 | Operator 登录 | 只看到 4 个菜单 |
| 3 | 新增物资 → 在列表中出现 | 物资正确显示 |
| 4 | 搜索 "钢材" | 只显示名称含"钢材"的物资 |
| 5 | 确认入库单 | 库存增加相应的量 |
| 6 | 确认出库单 | 库存减少相应的量 |
| 7 | 出库数量超过库存 | 弹出错误提示，不扣减 |
| 8 | 库存低于安全库存 | 预警列表中出现此项 |
| 9 | 操作日志记录 | 操作出现在日志列表 |
| 10 | 仪表盘数据 | 与数据库数据一致 |
| 11 | CSV 导出 | 文件可被 Excel 正确打开 |
| 12 | 密码错误登录 | 提示"用户名或密码错误" |
| 13 | 分页功能 | 翻页数据正确 |
| 14 | 表格排序 | 点击表头排序 |
| 15 | 软删除 | 删除后数据不可见但数据库中存在 |

---

#### 6.3 README.md 编写 (Day 27, ~2h)

**README 应包含：**

1. **项目名称 + 一句话简介**
2. **技术栈 badges** (C++17, Qt6, MySQL, CMake)
3. **功能截图 (4-6 张)** — 仪表盘、入库、出库、库存、登录页
4. **系统架构图** (ASCII art 或 draw.io 导出的图片)
5. **功能特性列表**
6. **快速开始** — 环境依赖 + 构建步骤 + 配置说明
7. **项目结构** — 目录树
8. **数据库设计** — ER 图或关键表说明
9. **设计亮点** — 连接池、事务、权限、安全

---

## 4. 项目架构设计

### 4.1 分层架构

```
┌─────────────────────────────────────────────────────┐
│  UI Layer (展示层)                                   │
│  QMainWindow · QWidget · QTableView · QChartView     │
│  QSS Stylesheets · Qt Signal/Slot                    │
├─────────────────────────────────────────────────────┤
│  Service Layer (业务逻辑层)                           │
│  AuthService · ProductService · InboundService       │
│  OutboundService · InventoryService · DashboardService│
│  事务管理 · 权限检查 · 业务规则验证                    │
├─────────────────────────────────────────────────────┤
│  Model Layer (数据模型层)                             │
│  BaseTableModel<T> · ProductTableModel · ...         │
│  QAbstractTableModel 子类 · beginResetModel()        │
├─────────────────────────────────────────────────────┤
│  DAO Layer (数据访问层)                               │
│  BaseDAO · UserDAO · ProductDAO · InboundDAO · ...   │
│  Prepared Statements · 分页查询 · 双版本方法          │
├─────────────────────────────────────────────────────┤
│  Core Layer (基础设施层)                              │
│  ConnectionPool · ConfigManager · PasswordHasher     │
│  SessionManager · ScopedConnection                   │
├─────────────────────────────────────────────────────┤
│  MySQL 8.0 (持久层)                                   │
│  wms 数据库 · 11 张表 · 索引 · 外键                   │
└─────────────────────────────────────────────────────┘
```

### 4.2 分层通信规则 (严格单向依赖)

```
UI ──调用──> Service ──调用──> DAO ──调用──> Core ──调用──> MySQL
```

**禁止事项：**
- ❌ UI 层直接 `new ProductDAO()` — 应该调用 `ProductService`
- ❌ Service 层直接 `QSqlDatabase::addDatabase()` — 应该从 `ConnectionPool` 获取
- ❌ DAO 层包含业务逻辑判断 — 只做数据操作，业务规则在 Service 层
- ❌ Model 层跨层访问数据库 — Model 只持有数据，由 Service 喂给它

### 4.3 完整目录结构

```
D:\Desktop\WMS\
├── CMakeLists.txt                  # 根 CMake: project + find_package + add_subdirectory
├── config.ini                      # 数据库连接配置
├── README.md
├── sql/
│   ├── schema.sql                  # 建表 DDL (11张表)
│   └── seed.sql                    # 初始数据 (单位、分类、Admin账号)
├── src/
│   ├── CMakeLists.txt
│   ├── main.cpp                    # 入口: 配置→连接池→样式→MainWindow
│   ├── core/
│   │   ├── CMakeLists.txt
│   │   ├── ConfigManager.h/.cpp        # 单例配置管理器
│   │   ├── ConnectionPool.h/.cpp       # 数据库连接池 (单例+RAII)
│   │   ├── PasswordHasher.h/.cpp       # SHA-256 + 随机盐
│   │   └── SessionManager.h/.cpp       # 当前登录用户会话
│   ├── dao/
│   │   ├── CMakeLists.txt
│   │   ├── BaseDAO.h                   # DAO 基类
│   │   ├── UserDAO.h/.cpp
│   │   ├── ProductDAO.h/.cpp
│   │   ├── CategoryDAO.h/.cpp
│   │   ├── UnitDAO.h/.cpp
│   │   ├── InboundDAO.h/.cpp           # 入库单+明细
│   │   ├── OutboundDAO.h/.cpp          # 出库单+明细
│   │   ├── InventoryDAO.h/.cpp
│   │   └── OperationLogDAO.h/.cpp
│   ├── models/
│   │   ├── CMakeLists.txt
│   │   ├── BaseTableModel.h            # 模板基类
│   │   ├── ProductTableModel.h/.cpp
│   │   ├── InboundTableModel.h/.cpp
│   │   ├── OutboundTableModel.h/.cpp
│   │   ├── InventoryTableModel.h/.cpp
│   │   ├── AlertTableModel.h/.cpp
│   │   ├── OperationLogTableModel.h/.cpp
│   │   └── UserTableModel.h/.cpp
│   ├── services/
│   │   ├── CMakeLists.txt
│   │   ├── AuthService.h/.cpp
│   │   ├── ProductService.h/.cpp
│   │   ├── InboundService.h/.cpp       # 入库确认事务
│   │   ├── OutboundService.h/.cpp      # 出库确认事务
│   │   ├── InventoryService.h/.cpp
│   │   ├── DashboardService.h/.cpp
│   │   └── ReportService.h/.cpp
│   ├── ui/
│   │   ├── CMakeLists.txt
│   │   ├── MainWindow.h/.cpp           # QStackedWidget + 侧边菜单 + 角色路由
│   │   ├── LoginWidget.h/.cpp
│   │   ├── DashboardWidget.h/.cpp      # 4个卡片 + 4个图表
│   │   ├── ProductManagementWidget.h/.cpp
│   │   ├── InboundManagementWidget.h/.cpp
│   │   ├── OutboundManagementWidget.h/.cpp
│   │   ├── InventoryWidget.h/.cpp
│   │   ├── AlertWidget.h/.cpp
│   │   ├── OperationLogWidget.h/.cpp
│   │   ├── UserManagementWidget.h/.cpp # 仅Admin
│   │   ├── ReportWidget.h/.cpp
│   │   └── resources/
│   │       ├── style.qss               # 深色主题
│   │       └── resources.qrc
│   └── utils/
│       ├── CMakeLists.txt
│       ├── ExportUtils.h/.cpp          # CSV/Excel 导出
│       ├── DateUtils.h/.cpp
│       └── AppConstants.h              # 全局常量
└── tests/
    ├── CMakeLists.txt
    ├── test_db_connection.cpp
    ├── test_connection_pool.cpp
    ├── test_password_hasher.cpp
    └── test_inbound_service.cpp
```

---

## 5. 数据库 Schema 设计

### 5.1 ER 关系概览

```
users ──1:N── operation_logs
categories ──1:N── products
units ──1:N── products
products ──1:N── inbound_details
products ──1:N── outbound_details
products ──1:N── inventory
inbound_orders ──1:N── inbound_details
outbound_orders ──1:N── outbound_details
```

### 5.2 完整 DDL

```sql
-- 使用数据库
USE wms;

-- 1. 用户表
CREATE TABLE users (
    id INT AUTO_INCREMENT PRIMARY KEY,
    username VARCHAR(50) NOT NULL UNIQUE,
    password_hash CHAR(64) NOT NULL COMMENT 'SHA-256 哈希',
    salt CHAR(32) NOT NULL COMMENT '16字节随机盐的十六进制表示',
    real_name VARCHAR(50) NOT NULL,
    role ENUM('admin', 'manager', 'operator') NOT NULL DEFAULT 'operator',
    is_active TINYINT(1) NOT NULL DEFAULT 1,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    deleted_at DATETIME NULL,
    INDEX idx_deleted (deleted_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 2. 物资分类表
CREATE TABLE categories (
    id INT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(50) NOT NULL UNIQUE,
    description VARCHAR(200),
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    deleted_at DATETIME NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 3. 计量单位表
CREATE TABLE units (
    id INT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(20) NOT NULL UNIQUE COMMENT '个/箱/千克/米/升',
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    deleted_at DATETIME NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 4. 物资表
CREATE TABLE products (
    id INT AUTO_INCREMENT PRIMARY KEY,
    code VARCHAR(30) NOT NULL UNIQUE COMMENT '物资编码',
    name VARCHAR(100) NOT NULL,
    category_id INT NOT NULL,
    unit_id INT NOT NULL,
    specification VARCHAR(200) COMMENT '规格型号',
    safety_stock INT NOT NULL DEFAULT 0 COMMENT '安全库存量',
    remark TEXT,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    deleted_at DATETIME NULL,
    FOREIGN KEY (category_id) REFERENCES categories(id),
    FOREIGN KEY (unit_id) REFERENCES units(id),
    INDEX idx_category (category_id),
    INDEX idx_name (name),
    INDEX idx_code (code),
    INDEX idx_deleted (deleted_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 5. 入库单主表
CREATE TABLE inbound_orders (
    id INT AUTO_INCREMENT PRIMARY KEY,
    order_no VARCHAR(30) NOT NULL UNIQUE COMMENT '入库单号: IN-YYYYMMDD-XXX',
    supplier VARCHAR(100) COMMENT '供应商',
    status ENUM('draft', 'confirmed', 'cancelled') NOT NULL DEFAULT 'draft',
    total_amount DECIMAL(12, 2) NOT NULL DEFAULT 0.00,
    remark TEXT,
    operator_id INT NOT NULL COMMENT '操作员',
    confirmed_at DATETIME NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    deleted_at DATETIME NULL,
    FOREIGN KEY (operator_id) REFERENCES users(id),
    INDEX idx_order_no (order_no),
    INDEX idx_status (status),
    INDEX idx_created (created_at),
    INDEX idx_deleted (deleted_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 6. 入库单明细表
CREATE TABLE inbound_details (
    id INT AUTO_INCREMENT PRIMARY KEY,
    order_id INT NOT NULL,
    product_id INT NOT NULL,
    quantity INT NOT NULL CHECK (quantity > 0),
    unit_price DECIMAL(10, 2) NOT NULL DEFAULT 0.00,
    total_price DECIMAL(12, 2) GENERATED ALWAYS AS (quantity * unit_price) STORED,
    batch_no VARCHAR(50) COMMENT '批次号',
    remark VARCHAR(200),
    FOREIGN KEY (order_id) REFERENCES inbound_orders(id) ON DELETE CASCADE,
    FOREIGN KEY (product_id) REFERENCES products(id),
    INDEX idx_order (order_id),
    INDEX idx_product (product_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 7. 出库单主表
CREATE TABLE outbound_orders (
    id INT AUTO_INCREMENT PRIMARY KEY,
    order_no VARCHAR(30) NOT NULL UNIQUE COMMENT '出库单号: OUT-YYYYMMDD-XXX',
    recipient VARCHAR(100) COMMENT '领用人/领用部门',
    status ENUM('draft', 'confirmed', 'cancelled') NOT NULL DEFAULT 'draft',
    total_amount DECIMAL(12, 2) NOT NULL DEFAULT 0.00,
    remark TEXT,
    operator_id INT NOT NULL,
    confirmed_at DATETIME NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    deleted_at DATETIME NULL,
    FOREIGN KEY (operator_id) REFERENCES users(id),
    INDEX idx_order_no (order_no),
    INDEX idx_status (status),
    INDEX idx_created (created_at),
    INDEX idx_deleted (deleted_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 8. 出库单明细表
CREATE TABLE outbound_details (
    id INT AUTO_INCREMENT PRIMARY KEY,
    order_id INT NOT NULL,
    product_id INT NOT NULL,
    quantity INT NOT NULL CHECK (quantity > 0),
    unit_price DECIMAL(10, 2) NOT NULL DEFAULT 0.00 COMMENT '出库时的成本价',
    total_price DECIMAL(12, 2) GENERATED ALWAYS AS (quantity * unit_price) STORED,
    remark VARCHAR(200),
    FOREIGN KEY (order_id) REFERENCES outbound_orders(id) ON DELETE CASCADE,
    FOREIGN KEY (product_id) REFERENCES products(id),
    INDEX idx_order (order_id),
    INDEX idx_product (product_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 9. 库存表
CREATE TABLE inventory (
    id INT AUTO_INCREMENT PRIMARY KEY,
    product_id INT NOT NULL,
    warehouse_name VARCHAR(50) NOT NULL DEFAULT '默认仓库',
    quantity INT NOT NULL DEFAULT 0 CHECK (quantity >= 0),
    batch_no VARCHAR(50) DEFAULT NULL,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (product_id) REFERENCES products(id),
    UNIQUE KEY uk_stock (product_id, warehouse_name, COALESCE(batch_no, '')),
    INDEX idx_product (product_id),
    INDEX idx_quantity (quantity)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 10. 操作日志表
CREATE TABLE operation_logs (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    username VARCHAR(50) NOT NULL COMMENT '冗余存储，防止用户被删后无法追溯',
    operation_type VARCHAR(50) NOT NULL COMMENT 'LOGIN/LOGOUT/CREATE/UPDATE/DELETE/CONFIRM/CANCEL/EXPORT',
    target_type VARCHAR(50) COMMENT 'USER/PRODUCT/INBOUND/OUTBOUND/INVENTORY',
    target_id INT COMMENT '操作对象ID',
    detail TEXT COMMENT '操作详情 JSON',
    ip_address VARCHAR(45),
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_username (username),
    INDEX idx_type (operation_type),
    INDEX idx_created (created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 11. 库存预警表
CREATE TABLE inventory_alerts (
    id INT AUTO_INCREMENT PRIMARY KEY,
    product_id INT NOT NULL,
    warehouse_name VARCHAR(50) NOT NULL DEFAULT '默认仓库',
    alert_type ENUM('low_stock', 'overstock', 'out_of_stock') NOT NULL,
    current_quantity INT NOT NULL,
    safety_stock INT NOT NULL DEFAULT 0,
    is_resolved TINYINT(1) NOT NULL DEFAULT 0,
    resolved_at DATETIME NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (product_id) REFERENCES products(id),
    INDEX idx_resolved (is_resolved),
    INDEX idx_created (created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

### 5.3 初始数据

```sql
-- seed.sql
-- 默认 Admin 账号 (密码: admin123)
INSERT INTO users (username, password_hash, salt, real_name, role) VALUES
('admin', '<SHA-256 hash of salt+admin123>',
 '<16-byte-random-salt-as-hex>', '系统管理员', 'admin');

-- 示例分类
INSERT INTO categories (name, description) VALUES
('原材料', '生产所需的原始物料'),
('半成品', '生产过程中的中间产品'),
('成品', '可直接销售的最终产品'),
('辅料', '生产辅助材料'),
('包装材料', '产品包装用材料');

-- 示例单位
INSERT INTO units (name) VALUES
('个'), ('箱'), ('千克'), ('米'), ('升'), ('套'), ('卷');
```

---

## 6. 分步实现计划 (约 27 天)

### 总览时间线

```
Phase 1 ████░░░░░░░░░░░░░░░░░░░░░░ 基础设施 (3天)    Day 1-3
Phase 2 ████████░░░░░░░░░░░░░░░░░░ DAO层 (3天)        Day 4-6
Phase 3 ████████████░░░░░░░░░░░░░░ Service层 (3天)    Day 7-9
Phase 4 ████████████████░░░░░░░░░░ Model层 (2天)      Day 10-11
Phase 5 █████████████████████████░░ UI层 (12天)        Day 12-23
Phase 6 ███████████████████████████ 导出+收尾 (4天)    Day 24-27
```

### 详细日程

#### Day 0: 环境搭建
- [ ] 安装 MySQL ODBC Connector
- [ ] 验证 ODBC 连接测试程序通过
- [ ] 创建 `D:\Desktop\WMS\` 项目根目录
- [ ] `git init` 初始化版本控制

**交付物:** 能成功连接数据库的测试程序

---

#### Day 1: 项目骨架 + ConfigManager
- [ ] 根 `CMakeLists.txt` + `src/CMakeLists.txt`
- [ ] `main.cpp` 入口（最小可运行程序）
- [ ] `config.ini` 配置文件
- [ ] `ConfigManager` 单例实现

**交付物:** CMake 构建通过，ConfigManager 能读取 ini 配置

---

#### Day 2: ConnectionPool 连接池 ⭐
- [ ] 连接池核心类（单例 + `QMutex` + `QWaitCondition`）
- [ ] `acquire()` / `release()` 方法
- [ ] `ScopedConnection` RAII 包装器
- [ ] 健康检查 + 空闲超时回收
- [ ] 单元测试：并发获取连接，验证归还和复用

**交付物:** 连接池通过并发测试，10 个线程同时获取/归还连接

---

#### Day 3: PasswordHasher + SessionManager + 建表
- [ ] `PasswordHasher::hash(password)` + `PasswordHasher::verify(password, hash, salt)`
- [ ] `SessionManager` 会话管理
- [ ] 执行 `sql/schema.sql` 建表
- [ ] 执行 `sql/seed.sql` 插入初始数据

**交付物:** 密码哈希验证通过，数据库表创建完毕

---

#### Day 4: BaseDAO + UserDAO
- [ ] `BaseDAO` 基类：`getConnection()`, `querySingleValue()`
- [ ] `UserDAO` 完整 CRUD + `findByUsername()`

**交付物:** UserDAO 单元测试通过

---

#### Day 5: CategoryDAO + UnitDAO + ProductDAO
- [ ] `CategoryDAO` CRUD
- [ ] `UnitDAO` CRUD
- [ ] `ProductDAO` CRUD + 分页搜索

**交付物:** 三个 DAO 测试通过

---

#### Day 6: InboundDAO + OutboundDAO + InventoryDAO + OperationLogDAO
- [ ] `InboundDAO` 主表+明细双版本方法
- [ ] `OutboundDAO` 主表+明细双版本方法
- [ ] `InventoryDAO` 查询+增减库存(事务版)
- [ ] `OperationLogDAO` 写入日志

**交付物:** 所有 DAO 测试通过

---

#### Day 7: AuthService + 库存预置数据
- [ ] `AuthService::login()` 完整流程
- [ ] 生成 Admin 账号的正确 SHA-256 哈希
- [ ] 添加几个测试物资和初始库存

**交付物:** 能通过 AuthService 成功登录

---

#### Day 8: InboundService ⭐
- [ ] `InboundService::createOrder()` 创建草稿
- [ ] `InboundService::confirmOrder()` 事务确认
- [ ] 单元测试：创建入库单 → 确认 → 验证库存增加

**交付物:** 入库事务测试通过

---

#### Day 9: OutboundService + InventoryService + DashboardService
- [ ] `OutboundService` 出库确认事务 + 库存不足回滚
- [ ] `InventoryService` 查询 + 预警
- [ ] `DashboardService` 聚合数据

**交付物:** 出库事务测试通过（含库存不足场景）

---

#### Day 10: BaseTableModel + ProductTableModel + InventoryTableModel
- [ ] `BaseTableModel<T>` 模板基类
- [ ] `ProductTableModel` 具体实现
- [ ] `InventoryTableModel` 具体实现

**交付物:** 模型正确显示数据，排序和高亮正常

---

#### Day 11: 剩余 4 个 TableModel
- [ ] `InboundTableModel`
- [ ] `OutboundTableModel`
- [ ] `OperationLogTableModel`
- [ ] `UserTableModel`

**交付物:** 所有 Model 测试通过

---

#### Day 12: MainWindow 导航框架
- [ ] 侧边菜单 + `QStackedWidget`
- [ ] 角色路由（Admin 全菜单，Operator 受限）
- [ ] HeaderBar（用户名、角色、登出）

**交付物:** 导航框架可用，菜单切换页面正常

---

#### Day 13: LoginWidget
- [ ] 登录卡片 UI
- [ ] 连接 `AuthService`
- [ ] 登录成功后切换到 MainWindow
- [ ] 错误提示

**交付物:** 完整的登录 → 导航流程

---

#### Day 14-15: DashboardWidget
- [ ] 4 个概览统计卡片（自定义 `QFrame`）
- [ ] 连接 `DashboardService` 获取真实数据
- [ ] 最近 7 天入库趋势柱状图
- [ ] 最近 7 天出库趋势柱状图
- [ ] 品类库存占比饼图
- [ ] 入库 Top 5 水平柱状图

**交付物:** 仪表盘显示真实数据

---

#### Day 16-17: ProductManagementWidget
- [ ] 搜索栏 + 操作栏
- [ ] `QTableView` + `ProductTableModel`
- [ ] 分页控件
- [ ] 新增/编辑 Dialog

**交付物:** 物资管理完整 CRUD

---

#### Day 18-19: InboundManagementWidget + OutboundManagementWidget
- [ ] 入库单列表页
- [ ] 新建入库单 Dialog（主表 + 动态明细表）
- [ ] 出库单列表页
- [ ] 新建出库单 Dialog（库存不足提示）

**交付物:** 入库/出库完整流程

---

#### Day 20: InventoryWidget
- [ ] 库存列表 + 多条件筛选
- [ ] 低库存行红色高亮
- [ ] 手动调整库存 Dialog

**交付物:** 库存管理页面

---

#### Day 21: AlertWidget + OperationLogWidget
- [ ] 预警列表 + 一键补货按钮
- [ ] 日志列表 + 筛选

**交付物:** 预警和日志页面

---

#### Day 22: UserManagementWidget
- [ ] 用户列表（仅 Admin）
- [ ] 新增用户 Dialog（自动生成哈希密码）
- [ ] 角色分配 + 禁用/启用

**交付物:** 用户管理页面

---

#### Day 23: ReportWidget
- [ ] CSV 导出
- [ ] Excel 导出 (HTML table 格式)
- [ ] 选择导出内容（库存/入库/出库/日志）

**交付物:** 报表导出功能

---

#### Day 24-25: QSS 深色主题
- [ ] 全局 `style.qss` 编写
- [ ] 所有页面控件适配
- [ ] 图表深色主题适配

**交付物:** 深色主题 UI 完成

---

#### Day 26: 全面测试 + Bug 修复
- [ ] 执行 15+ 功能测试场景
- [ ] 修复发现的 Bug
- [ ] 边界情况测试（空数据、极限值、并发操作）

**交付物:** 所有测试场景通过

---

#### Day 27: README + 最终整理
- [ ] 编写 README.md
- [ ] 录制 2 分钟功能演示 GIF
- [ ] 截图（仪表盘、入库、出库、库存、登录）
- [ ] Git commit + 推送到 GitHub

**交付物:** 完整的开源项目

---

## 7. 关键设计决策

### 决策 1: ODBC vs 原生 MySQL 驱动

| 方案 | 优点 | 缺点 |
|------|------|------|
| **ODBC 桥接 (本项目选择)** | 5 分钟安装 Connector 即可用; 不依赖 Qt 源码编译 | 多一层桥接，理论性能开销（桌面应用可忽略） |
| 编译 `qsqlmysql.dll` | 原生连接，少一层桥接 | 需要 Qt 源码 + 重新编译插件，耗时且复杂 |

**结论：** 选 ODBC。你的环境没有 `qsqlmysql.dll` 原生驱动，也没有 Qt 源码。ODBC 桥接对桌面应用性能无影响。

### 决策 2: 自定义 Model vs QSqlTableModel

| 方案 | 优点 | 缺点 |
|------|------|------|
| **自定义 QAbstractTableModel (本项目选择)** | 完全可控（分页、自定义显示、高亮）；不依赖 SQL 缓存；与 Service 层解耦 | 需要手写更多代码 |
| `QSqlTableModel` | 开箱即用，自动 CRUD | 分页困难；不支持复杂 JOIN；自定义显示逻辑受限 |

**结论：** 选自定义 Model。你的能力足以手写，且项目需要分页、自定义高亮、JOIN 显示。

### 决策 3: DAO 双版本方法设计

```cpp
// 版本 1: 独立使用（日常查询）
Product ProductDAO::findById(int id) {
    auto conn = ConnectionPool::instance().acquire();
    QSqlQuery query(conn.db());
    // ...
}  // conn 析构自动归还

// 版本 2: 事务内使用（入库/出库确认）
bool InventoryDAO::increaseStock(QSqlDatabase &db, int productId, int qty) {
    QSqlQuery query(db);  // 使用外部传入的连接
    // ...
}
```

**为什么需要双版本：** 事务要求所有操作在同一个连接中进行。独立版方法自己获取连接；事务版方法接受外部连接引用。这是一线 C++ 项目的标准做法。

### 决策 4: 连接池的 RAII 归还

```cpp
class ScopedConnection {
    QString connName_;
public:
    ScopedConnection() : connName_(ConnectionPool::instance().acquire()) {}
    ~ScopedConnection() { if (!connName_.isEmpty()) ConnectionPool::instance().release(connName_); }
    QSqlDatabase db() { return QSqlDatabase::database(connName_); }
    // 禁止拷贝
    ScopedConnection(const ScopedConnection&) = delete;
    ScopedConnection& operator=(const ScopedConnection&) = delete;
};
```

**为什么不用 `std::unique_ptr`：** 这里需要的不是堆对象生命周期管理，而是自定义的 `release()` 调用。手写 RAII 类更清晰。

### 决策 5: 信号-槽跨页面同步

当用户在 `ProductManagementWidget` 修改了物资信息后，`DashboardWidget` 和 `InventoryWidget` 需要刷新。

```cpp
// MainWindow 作为信号中枢
connect(productWidget, &ProductManagementWidget::dataChanged,
        dashboardWidget, &DashboardWidget::refresh);
connect(productWidget, &ProductManagementWidget::dataChanged,
        inventoryWidget, &InventoryWidget::refresh);

// 各页面发出信号
void ProductManagementWidget::onSaveClicked() {
    // ... 保存逻辑
    emit dataChanged();  // 通知所有监听者刷新
}
```

---

## 8. 常见陷阱与解决方案

### 陷阱 1: ODBC 连接字符串不匹配

```
❌ DRIVER={MySQL ODBC 8.0 ANSI Driver}   // 32位 Qt 用这个
✅ DRIVER={MySQL ODBC 8.0 Unicode Driver} // 64位 Qt 用这个
```

**现象：** `db.lastError().text()` 返回 "Data source name not found"  
**解决：** 打开"ODBC 数据源管理器 (64-bit)"确认驱动名是否正确

---

### 陷阱 2: QAbstractTableModel 视图不刷新

**现象：** 数据变了但表格显示不变  
**原因：** 忘记调用 `beginResetModel()` / `endResetModel()`  
**解决：**

```cpp
void ProductTableModel::setRecords(QVector<Product> records) {
    beginResetModel();
    records_ = std::move(records);
    endResetModel();
}
```

---

### 陷阱 3: QChart 内存泄漏

**现象：** 页面切换时图表不释放  
**原因：** `QChartView` 被 `QStackedWidget` 切换隐藏但未删除，旧的 Chart 仍在内存  
**解决：** 在 `DashboardWidget` 析构函数中手动清理，或每次 show 时重建图表

```cpp
void DashboardWidget::clearCharts() {
    // 移除并删除所有旧的 QChartView
    for (auto *chartView : chartViews_) {
        chartView->setParent(nullptr);
        delete chartView;
    }
    chartViews_.clear();
}
```

---

### 陷阱 4: MySQL "server has gone away"

**现象：** 程序运行一段时间后数据库操作失败  
**原因：** MySQL `wait_timeout` 默认 28800 秒 (8小时)，空闲连接被服务器断开  
**解决：** 连接池定时健康检查

```cpp
void ConnectionPool::healthCheck() {
    QMutexLocker locker(&mutex_);
    for (auto it = idleConnections_.begin(); it != idleConnections_.end();) {
        QSqlDatabase db = QSqlDatabase::database(*it);
        QSqlQuery query("SELECT 1", db);
        if (!query.exec()) {
            // 连接已失效，移除并新建
            QSqlDatabase::removeDatabase(*it);
            it = idleConnections_.erase(it);
        } else {
            ++it;
        }
    }
}
```

---

### 陷阱 5: 事务连接不统一

**现象：** 事务内某个操作使用了不同的连接，导致数据不一致  
**原因：** 不小心在事务方法中又调用了 `ConnectionPool::acquire()`  
**解决：** 确保事务内所有 DAO 调用都传入同一个 `QSqlDatabase &db`

```cpp
// ✅ 正确
bool InboundService::confirmOrder(int orderId) {
    auto conn = ConnectionPool::instance().acquire();
    QSqlDatabase &db = conn.db();  // 同一个连接
    db.transaction();

    if (!inboundDao_->confirmOrder(db, orderId))      // 传 db
        { db.rollback(); return false; }
    if (!inventoryDao_->increaseStock(db, ...))       // 传 db
        { db.rollback(); return false; }
    if (!logDao_->insertWithConnection(db, ...))      // 传 db
        { db.rollback(); return false; }

    return db.commit();
}
```

---

### 陷阱 6: 软删除查询遗漏

**现象：** 删除的数据仍然显示  
**原因：** 某个 DAO 的 SQL 忘了加 `WHERE deleted_at IS NULL`  
**解决：** 在 `BaseDAO` 中封装查询辅助方法

```cpp
QString BaseDAO::activeWhere() { return "WHERE deleted_at IS NULL"; }
```

---

## 9. 面试准备要点

### 9.1 技术面试常见问题（结合本项目）

| 面试问题 | 如何用本项目回答 |
|----------|----------------|
| "介绍一下你的项目" | 先讲系统功能概览，再讲架构分层，重点提连接池和事务设计 |
| "为什么选择 ODBC 而不是 MySQL 原生驱动？" | Qt 6 默认不编译 MySQL 驱动; ODBC 安装 Connector 即可用; 桌面应用性能无差异 |
| "数据库连接池是怎么设计的？" | 单例 + QMutex + QWaitCondition + RAII ScopedConnection + 健康检查 |
| "如何处理并发入库/出库？" | 事务 + SELECT ... FOR UPDATE 行级锁; 每个线程独立 QSqlDatabase 连接 |
| "为什么要手写 Model 而不是用 QSqlTableModel？" | QSqlTableModel 不支持分页/复杂 JOIN/自定义高亮; 自定义 Model 实现更好的分层解耦 |
| "密码是怎么存储的？" | SHA-256 + 16字节随机盐; QCryptographicHash; 盐存在数据库单独列 |
| "如何防止 SQL 注入？" | 所有查询使用 Prepared Statements (`prepare() + bindValue()`)，绝不拼接字符串 |
| "软删除是怎么实现的？" | 所有表加 `deleted_at DATETIME NULL`，查询永远加 `WHERE deleted_at IS NULL` |
| "如果让你重做这个项目，你会怎么改进？" | 加入异步操作(QtConcurrent/QFuture); 增加单元测试覆盖率(QTestLib); REST API 后端分离 |

### 9.2 项目中的技术亮点（面试主动提及）

1. **线程池经验迁移到连接池** — "我之前写过 C++ 线程池，在设计数据库连接池时，我把线程池的核心模式（单例、条件变量等待、RAII 生命周期管理）直接复用了。增加的新挑战是连接健康检查和空闲超时回收。"

2. **双版本 DAO 方法** — "我给每个涉及数据库写操作的 DAO 方法提供了两个版本：独立版自己获取连接，事务版接受外部连接引用。这样 Service 层做事务时，可以确保所有操作在同一个连接中完成。"

3. **分层架构的严格单向依赖** — "UI → Service → DAO → Core，每层只依赖下一层。UI 层不知道数据库的存在，Service 层不知道 UI 如何渲染数据。这让每一层都可以独立测试和替换。"

4. **入库/出库的事务安全** — "入库确认是一个事务：写主表状态 → 逐一更新库存(UPSERT) → 写操作日志 → 检查是否需要库存预警。任何一步失败就整体回滚。出库还加了 SELECT FOR UPDATE 防止并发超卖。"

### 9.3 写在简历上的建议

**项目经验：**

> **企业仓库管理系统 (WMS)** | C++17, Qt 6, MySQL 8.0, CMake  
> *个人项目 — 2025年暑期*
> - 设计并实现了基于分层架构（UI-Service-DAO-Core）的物资进销存管理系统，支持多角色权限管理（Admin/Manager/Operator）
> - 手写数据库连接池（单例 + QMutex + QWaitCondition + RAII），支持连接复用、健康检查和空闲超时回收
> - 使用事务实现入库/出库确认流程，加入行级锁（SELECT FOR UPDATE）防止并发超卖
> - 自定义 QAbstractTableModel 实现分页数据绑定和解耦，替代 QSqlTableModel 的局限性
> - 基于 Qt Charts 实现数据仪表盘（柱状图/折线图/饼图），采用深色主题 QSS 样式
> - SHA-256 + 随机盐密码哈希，Prepared Statements 防 SQL 注入，软删除实现审计追踪
> - 使用 CMake 管理多目录项目构建，具备跨平台部署能力

---

## 附录: 推荐学习资源汇总

### 官方文档 (首选)

| 资源 | 链接 | 用途 |
|------|------|------|
| Qt SQL Module | https://doc.qt.io/qt-6/sql-programming.html | SQL 编程完整指南 |
| Qt Model/View | https://doc.qt.io/qt-6/model-view-programming.html | Model/View 框架必读 |
| Qt Charts | https://doc.qt.io/qt-6/qtcharts-overview.html | 图表模块概述 |
| Qt Style Sheets | https://doc.qt.io/qt-6/stylesheet-reference.html | QSS 属性速查 |
| Qt QSqlDatabase | https://doc.qt.io/qt-6/qsqldatabase.html | 数据库连接类文档 |
| Qt QSqlQuery | https://doc.qt.io/qt-6/qsqlquery.html | SQL 查询类文档 |
| MySQL ODBC Connector | https://dev.mysql.com/downloads/connector/odbc/ | ODBC 驱动下载 |

### 视频教程 (B站)

| 关键词搜索 | 内容 |
|-----------|------|
| "Qt Model View 框架" | Model/View/Delegate 三角关系详解 |
| "Qt Charts 数据可视化" | 柱状图/折线图/饼图实战 |
| "Qt QSS 样式表" | 深色主题 QSS 编写教程 |
| "Qt 数据库编程 MySQL" | Qt SQL 模块基础到实战 |
| "数据库连接池 C++" | 连接池设计模式讲解 |
| "Qt 多线程 SQL" | Qt 线程与数据库交互要点 |

### 书籍推荐

| 书名 | 说明 |
|------|------|
| 《Qt 5.9 C++开发指南》 | 国内最系统 Qt 教程，含数据库章节 |
| 《C++17 完全指南》 | 现代 C++ 特性速查 |
| 《MySQL 是怎样运行的》 | 深入理解 MySQL 内部机制 |
| 《C++ Concurrency in Action》 | C++ 并发编程（第2版），线程池参考 |

### GitHub 参考项目

| 项目 | 说明 |
|------|------|
| [QtExcel/QXlsx](https://github.com/QtExcel/QXlsx) | Excel 读写库 |
| [a-mo-xi-wei/userPrivilegeManagerSystem](https://github.com/a-mo-xi-wei/userPrivilegeManagerSystem) | Qt6+MySQL 权限管理系统参考 |

---

> 📌 **最后的建议：** 不要试图一次性做完美。先跑通一个最小闭环（登录 → 仪表盘 → 物资 CRUD），再逐步叠加入库/出库 / 预警/日志/导出。每天 commit 代码，让 Git 记录你的整个成长过程 — 这也是面试时可以展示的"工程习惯"。

**祝暑期项目顺利！🚀**
