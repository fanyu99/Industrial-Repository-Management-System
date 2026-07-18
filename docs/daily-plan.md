# WMS 每日实施计划

> 目的：把短期实施计划放在项目内，避免后续 Codex 上下文过长时遗漏当前进度。
>
> 范围：只维护“今天 + 后 2-3 天”的滚动计划。每天完成后更新一次。

## 当前滚动计划

### Day 2：DatabaseTypes、元类型、错误码、Executor 状态机

**目标：** 在继续实现数据库执行器行为之前，先把数据库模块对外使用的公共协议定义清楚。

**知识点：**

- `DatabaseConfig`：数据库连接配置，以及配置有效性的校验规则。
- `DatabaseStatement`：SQL 文本、语句类型、绑定参数。
- `DatabaseTask`：单语句任务与事务任务的区别。
- `DatabaseResult`：成功标记、语句结果、失败语句下标、错误信息。
- `DatabaseErrorCode`：稳定的、给程序判断用的错误分类。
- `Q_DECLARE_METATYPE` 与 `qRegisterMetaType`：为什么自定义类型跨线程信号传递前需要注册。
- `DatabaseExecutorState`：`Starting`、`Ready`、`Failed`、`ShuttingDown`、`Stopped` 等生命周期状态。

**实施任务：**

- 复查 `DatabaseTypes.h`，确保每个类型只有一个清晰职责。
- 检查非法配置、非法语句、非法任务是否能被稳定拒绝。
- 画出 Executor 状态机。
- 写出状态迁移表。
- 增加类型层面的测试：配置校验、任务校验、元类型可用性。

**验收标准：**

- 只看 `DatabaseTypes.h`，就能理解数据库模块对外暴露的协议。
- 所有跨线程信号会用到的自定义类型都已声明并注册。
- Executor 状态机没有含糊不清的迁移。
- 类型校验测试通过。

### Day 3：Worker 初始化、ODBC 配置、健康检查、连接关闭

**目标：** 让 `DatabaseWorker` 完整拥有数据库连接，并确保连接只在 Worker 线程内创建、打开、查询和关闭。

**知识点：**

- Qt SQL 连接的线程归属。
- `QSqlDatabase::addDatabase(driver, connectionName)` 的使用方式。
- 唯一连接名与 `QSqlDatabase::removeDatabase`。
- ODBC 连接字符串的构造。
- 登录超时与基础连接诊断。
- 使用 `SELECT 1` 做健康检查。
- 定时器、查询对象、数据库连接、Worker 线程的安全关闭顺序。

**实施任务：**

- 确认 `DatabaseWorker` 只在 Worker 线程中创建、打开、查询和关闭 `QSqlDatabase`。
- 校验 ODBC 配置失败路径。
- 增加健康检查逻辑。
- shutdown 时关闭并移除具名数据库连接。
- 增加连接成功和连接失败测试。

**验收标准：**

- UI/main 线程不创建、不使用 `QSqlDatabase`。
- Worker 初始化时能发出成功信号或结构化失败信号。
- 连接失败能返回有用的 `DatabaseError`。
- Worker 关闭后，数据库连接已关闭并从 Qt 连接池中移除。

### Day 4：队列容量、requestId、队列满、无效任务、taskFinished 映射

**目标：** 让 `DatabaseExecutor` 在正常提交、非法提交和队列溢出时都有确定行为。

**知识点：**

- 有界队列设计。
- `QUuid` 请求 ID 的生成与结果关联。
- 在任务派发前拒绝非法任务。
- 将 Worker 完成结果映射为 `taskFinished`。
- “派发前被拒绝”和“Worker 执行失败”的区别。

**实施任务：**

- 确保每个提交的任务都有非空 `requestId`。
- 实现并校验队列容量限制。
- 队列溢出时返回 `QueueFull`。
- 任务结构非法时返回 `InvalidTask`。
- 确保 `taskFinished` 总是携带原始请求 ID。
- 增加队列边界测试。

**验收标准：**

- 队列满行为稳定、可测试。
- 无效任务不会进入 Worker。
- 每个完成结果都能通过 requestId 对应到原始请求。
- 被拒绝的任务不会破坏 Executor 后续运行。

### Day 5：事务任务、affectedRows 守卫、排队取消、软超时、异步 shutdown

**目标：** 为后续真实库存事务做准备，尤其是入库确认、出库扣减和库存一致性。

**知识点：**

- 单 SQL 任务与多语句事务任务。
- 事务的 begin、commit、rollback。
- `affectedRows` 作为数据库层业务守卫。
- 为什么不能强杀正在执行的 `QSqlQuery::exec()`。
- 排队取消、软超时、关闭排空超时的区别。
- 异步 shutdown 与待处理任务清理。

**实施任务：**

- 如果事务任务尚未完整实现，补齐事务执行逻辑。
- 为需要守卫的语句增加 affected-rows 期望。
- shutdown 或排空超时时取消待派发任务。
- 明确定义调用方看到的软超时行为。
- 增加事务回滚和 shutdown drain 测试。

**验收标准：**

- 事务中任意语句失败时，前面已执行语句全部回滚。
- 带守卫的 UPDATE 在 affected row 数不符合预期时会让事务失败。
- 待派发任务可以取消，但不强杀正在执行的 SQL。
- shutdown 行为确定，并能发出完成信号。

## 滚动更新规则

每天完成后：

- 在本文件中标记当天完成。
- 追加下一天计划，保持 3-4 天窗口。
- 每天内容保持简洁，方便编码前快速扫描。
