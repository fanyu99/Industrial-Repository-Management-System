# WMS 每日实施计划

> 目的：把短期实施计划放在项目内，避免后续 Codex 上下文过长时遗漏当前进度。
>
> 规划原则：以优化版详细实施计划为总体方向，根据实际完成模块和依赖关系动态调整顺序，但不得脱离计划阶段目标。
>
> 范围：只维护“当前任务 + 后续 2-3 天”的滚动计划。每天完成后更新一次。

- [ ] TODO：普通任务的超时处理相关机制（暂缓，不阻塞当前纵向切片）

## 当前进度

- Day 1：项目结构、CMakePresets、基础构建流程已完成。
- Day 2：DatabaseTypes、元类型、错误码、Executor 状态机已完成。
- Day 3：DatabaseWorker 初始化、ODBC 配置、健康检查、连接关闭已完成，26 项测试已通过。
- Day 4：DatabaseExecutor 队列容量、requestId、队列满、无效任务、taskFinished 映射已完成。
- Day 5：事务执行、affectedRows 守卫、事务结果状态、回滚与异步 shutdown 已完成。
- Day 6：IniHelper、DatabaseConfigLoader、ConfigManager、CMake 集成与配置单元测试已完成，构建和测试均已通过。

## 当前滚动计划

> 调整依据：数据库执行器与配置模块已经完成。按照详细实施计划第 2 周“Schema、认证与物资纵向切片”的方向，先完成各后续模块共同依赖的版本化 Schema，再进入密码、认证会话和 Product 纵向切片。

### Day 7：版本化数据库 Schema 与 `001_initial.sql`（当前任务）

**目标：** 建立可追踪、可验证、可回滚的数据库迁移基础，并交付认证和物资纵向切片所需的首个 Schema 版本。

**知识点：**

- 版本化 migration 与手写“最终 DDL”的区别。
- migration 的版本号、前置条件、变更内容、回滚策略和验证 SQL。
- MySQL InnoDB、`utf8mb4`、主键、业务唯一约束、外键与索引。
- 主数据“停用而非删除”和业务单据状态约束。
- Schema 结构测试与业务代码单元测试的区别。

**实施任务：**

- 设计 migration 目录、文件命名和执行顺序。
- 按详细计划整理 `001_initial.sql`，覆盖用户、分类、单位、物资、入库单、出库单及明细。
- 为首个 migration 编写对应回滚脚本和验证 SQL。
- 明确 migration 是由命令行、部署脚本还是后续 MigrationRunner 执行。
- 在干净测试库执行迁移、验证约束并执行回滚测试。

**验收标准：**

- 空数据库可以按固定步骤升级到版本 1。
- 所有表使用 InnoDB 和统一的 `utf8mb4` 字符集策略。
- 用户名、物资编码和单号等业务唯一约束真实生效。
- 外键、必要索引、状态字段和非空约束符合详细计划。
- 验证 SQL 和回滚脚本可以重复执行验证。

### Day 8：完善 PasswordHasher、PBKDF2 测试与管理员种子数据

**目标：** 完成密码安全模块闭环，为后续登录认证和 `users` 表种子数据提供可靠能力。

**知识点：**

- PBKDF2-HMAC-SHA256、随机盐、迭代次数和派生密钥长度。
- 密码哈希、密码验证与 `needRehash` 的职责区别。
- 为什么同一密码每次生成的哈希应不同。
- 密码策略版本化和参数升级后的登录重哈希。
- 密码、哈希参数及敏感信息的日志边界。

**实施任务：**

- 审查并完善现有 `PasswordHasher` 接口和错误模型。
- 增加有效策略、无效策略、正确密码、错误密码和无效记录测试。
- 验证随机盐导致相同密码产生不同记录。
- 增加 `needRehash` 的策略升级测试。
- 生成管理员种子数据所需的 hash、salt、algorithm 和 iterations。

**验收标准：**

- 明文密码不写入数据库、配置文件或日志。
- 正确密码验证成功，错误密码和损坏记录能被区分。
- 相同密码的两次哈希结果具有不同随机盐。
- 策略参数变化时可以准确判断是否需要重哈希。
- PasswordHasher 单元测试和合理迭代次数下的基准测试通过。

### Day 9：认证领域模型、SessionManager 与权限边界

**目标：** 建立登录用例需要的用户、角色、会话和权限模型，明确 UI、Service 与 Repository 的授权职责。

**知识点：**

- Entity、DTO、Value Object 和持久化记录的区别。
- `Role`、`Permission` 与 `hasPermission(action)`。
- SessionManager 的生命周期和当前用户状态。
- 隐藏菜单为什么不是安全边界，Service 为什么必须再次鉴权。
- 登录失败时如何避免泄露用户名是否存在。

**实施任务：**

- 定义用户身份、角色和权限动作模型。
- 设计 SessionManager 的登录、登出、当前用户和权限查询接口。
- 设计认证 Repository Port，隐藏 SQL 和原始 `DatabaseResult`。
- 设计 AuthService 的认证流程与统一错误模型。
- 使用 Fake Repository 测试登录和权限规则，不依赖真实数据库。

**验收标准：**

- 未登录状态不能通过权限检查。
- 登录、登出和切换会话后的状态一致。
- Service 层会执行权限校验，不能只依赖 UI 菜单过滤。
- 认证测试可以使用 Fake Repository 独立运行。
- UI 和 Service 不直接拼接认证 SQL。

### Day 10：Product Entity、DTO 与 Repository 契约

**目标：** 开始详细计划中的物资纵向切片，先稳定领域模型、分页请求和 Repository 边界。

**知识点：**

- Product Entity、DTO、分页结果和筛选条件的职责。
- Repository Port 与 MySQL Repository 实现的依赖方向。
- 异步 Operation 的 owner、requestId、`QPointer` 和 latest-wins。
- 唯一编码、分类、单位和停用规则应位于哪一层。
- Repository 结果到 `AppError` 的映射。

**实施任务：**

- 定义 Product、ProductFilter、ProductPage 和相关 DTO。
- 设计 `IProductRepository` 的分页、查询和写入契约。
- 明确异步请求对象的所有权和回调生命周期。
- 设计 ProductService 的业务校验和权限边界。
- 编写 Repository 契约与 Fake Repository 测试方案。

**验收标准：**

- Presentation、Service 不依赖 SQL、`QSqlQuery` 或原始数据库行。
- 分页结果包含记录、总数、页码和页大小。
- 物资编码唯一、分类单位有效和停用规则有明确责任层。
- 异步请求不会向已经销毁的 owner 回调。
- 后续 MySQL Repository 可以在不修改上层接口的情况下接入。

## 滚动更新规则

每天完成后：

- 先核对详细计划、代码、CMake 和测试结果。
- 在本文件中标记当天完成。
- 追加下一天计划，保持当前任务加后续 2-3 天窗口。
- 调整顺序时记录原计划位置和依赖理由。
- 无法确认计划依据时询问用户，不自行编造任务。
