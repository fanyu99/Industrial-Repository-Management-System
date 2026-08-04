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
- Day 7：版本化数据库 Schema、`001_initial.sql`、回滚脚本与验证 SQL 已完成。
- Day 8：PasswordHasher、PBKDF2 单元测试、算法边界修正与管理员 seed SQL 已完成，单元测试已通过。
- Day 9：认证领域模型、`SessionManager`、角色权限边界、`AuthService` 与 Fake Repository 测试已完成。
- Day 10：Product Entity、DTO、`IProductRepository`、`ProductService`、`FakeProductRepository` 与 ProductService 单元测试已完成；延迟回调场景已验证 owner 销毁后不会触发调用方 callback。
- Day 11：`MySqlProductRepository` 已完成，包含物资分页查询、按编码查询、创建、更新、启用/停用状态、数据库结果映射和 owner 生命周期守卫；debug 构建已通过。
- Day 12：`ProductTableModel`、Product 列表查询页面、分页数据展示、页面状态管理与 Presentation 层基础测试已完成，相关测试已通过。
- Day 13：Product 创建、编辑、启用/停用、刷新 UI，`ProductEditDialog`，分类/单位选项加载，行选择处理、增强版 `FakeProductRepository` 异步测试与真实 MySQL 链路验证已完成，相关测试已通过。
- Day 14：入库单领域模型、创建请求 DTO、`IInboundRepository`、`InboundService`、`FakeInboundRepository` 与 InboundService 单元测试已完成；创建、确认、列表查询、权限、异常成功结果和 owner 生命周期场景已覆盖，测试通过。当前权限模型下创建权限拒绝测试跳过，不阻塞完成。

## 当前滚动计划

> 调整依据：Product 应用层和 MySQL Repository 已完成，当前应继续沿详细计划 C.7 “Repository 与 Application Service”向 Presentation 层推进，让物资主数据从服务层查询形成可显示、可测试的列表闭环。列表 Model 稳定后，再接入创建、编辑、启用/停用 UI 和后续入库事务主线。

### Day 12：ProductTableModel 与物资列表页面查询闭环（已完成）

**目标：** 建立 Product 列表的 Presentation 层基础，让 UI 通过 ProductService 查询分页数据，而不是直接接触 Repository 或 SQL。

**知识点：**

- `QAbstractTableModel` 的职责、`rowCount()`、`columnCount()`、`data()`、`headerData()`。
- `beginResetModel()/endResetModel()` 与视图刷新边界。
- Model 只保存 DTO，不保存 SQL 或数据库连接。
- 页面状态：Loading、Ready、Empty、Error。
- 查询按钮、筛选条件、分页控件和防重复提交。
- `QPointer`、latest-wins 与连续搜索只接受最新结果。

**实施任务：**

- 设计 `ProductTableModel` 的列枚举、数据角色和更新接口。
- 使用 `QAbstractItemModelTester` 或 QTest 验证 Model 基本契约。
- 设计 Product 页面查询流程：构造 filter/pageRequest，调用 Service，更新 Model。
- 处理空结果、错误提示、加载态和重复点击。
- 保持 Presentation 只依赖 application 层接口，不包含 SQL 和 `DatabaseExecutor`。

**验收标准：**

- Model 的行列数、表头、单元格数据和重置行为测试通过。
- 页面查询成功时能显示分页物资列表。
- 查询中页面有明确 Loading 状态，并避免重复请求造成错乱。
- 空数据和错误结果有独立状态，不与正常列表混淆。
- UI、Model 不直接认识 MySQL、`QSqlQuery` 或原始 `DatabaseResult`。

**完成状态：** 已完成。`ProductTableModel` 和 Product 列表页面相关测试已通过。

### Day 13：物资创建、编辑、启用/停用 UI 与端到端纵向切片（已完成）

**目标：** 在 Product 页面完成物资主数据的基础 CRUD 交互，形成“权限校验 → 业务校验 → Repository → 数据刷新”的可演示闭环。

**知识点：**

- Dialog/Form 的输入校验与 Service 校验的分工。
- 创建、编辑、启用/停用三类命令的 UI 状态管理。
- 权限控制：隐藏/禁用按钮只是体验，Service 才是安全边界。
- 乐观刷新与重新查询的取舍。
- 业务错误、数据库错误和权限错误的用户提示差异。

**实施任务：**

- 设计 Product 编辑表单的数据收集、默认值和错误展示。
- 接入 `createProduct()`、`updateProduct()`、`setProductActive()`。
- 根据当前用户权限控制按钮可见/可用状态。
- 操作成功后刷新当前分页或更新当前 Model 数据。
- 覆盖重复编码、无效分类/单位、无权限、停用不存在产品等场景。

**验收标准：**

- 管理员可以完成创建、编辑、启用/停用并刷新列表。
- 无权限用户不能通过 UI 或 Service 执行写操作。
- 重复编码和无效输入显示为业务校验错误。
- 操作过程中 owner 销毁不会出现悬空回调。
- Product 主数据形成一个可运行、可测试、可演示的纵向切片。

**完成状态：** 已完成。已实现并测试通过以下内容：

- `ProductPage` 创建、编辑、启用/停用和刷新按钮及其信号槽。
- `ProductEditDialog` 的创建/编辑表单、输入校验、产品数据回填和请求对象生成。
- `QTableView`、`QItemSelectionModel` 与 `ProductTableModel` 的选中行处理。
- ProductPage 页面状态：Loading、Ready、Empty、Error。
- 分类和单位选项通过 `loadProductDialogOptions()` 统一加载。
- 创建、编辑、设置状态成功后的列表刷新。
- 增强版 `FakeProductRepository` 支持延迟回调、owner 生命周期守卫和 list 多挂起请求。
- `ProductPage` 已覆盖 owner 销毁后不回调、latest-wins、Empty/Error 状态和按钮状态测试。
- `ProductTableModel` 列数、表头、数据展示和重置行为测试已通过。
- 真实 `MySqlProductRepository` 链路已完成验证，真实 MySQL 查询、编辑回显和页面刷新链路通过。
- `test_presentation` 中相关测试已通过。

**后续增强项：** 以下内容不再作为 Day13 阻塞项，后续进入对应模块或 UI 增强时处理：

- [ ] 接入真实的分类服务和单位服务，替换当前的默认分类/默认单位选项。
- [ ] 增加产品关键词、分类、状态等筛选控件，并与 `ProductFilter`、`reloadCurrentPage()` 联动。
- [ ] 增加分页操作控件，支持上一页、下一页和页码状态展示。
- [ ] 后续补充真实仓储的集成测试或固定手动联调记录。

**结论：** Day13 已完成。物资主数据已经形成从 Presentation 到 Application 再到真实 MySQL Repository 的可运行、可测试、可演示纵向切片。


### Day 14：入库单领域模型与入库应用服务设计（已完成）

**目标：** 在物资主数据纵向切片稳定后，进入库存业务主线，先建立入库单、入库明细和入库业务服务的应用层结构，为后续事务写入库存流水与库存余额做准备。

**知识点：**

- 入库单 Header/Line 的建模方式。
- 入库状态流转：草稿、已提交、已取消等状态边界。
- 应用服务如何组织跨 Repository 的业务用例。
- 入库数量、物资、仓库、操作人的业务校验。
- 后续事务一致性需求：入库单、明细、库存流水、库存余额必须作为一个业务整体考虑。

**实施任务：**

- 设计入库单领域模型和 DTO，明确字段与数据库表的映射关系。
- 定义 `IInboundRepository` 或等价 Port 接口，先描述应用层需要什么能力。
- 设计 `InboundService` 的创建入库单、提交入库单、查询入库单基础接口。
- 明确哪些校验属于 Service，哪些校验属于 Repository。
- 暂不直接写复杂事务，先把接口、模型和测试边界确定清楚。

**验收标准：**

- 入库应用层接口不依赖 MySQL、`QSqlQuery` 或 UI。
- 入库单与明细模型能表达后续事务所需的核心数据。
- 创建/提交入库单的业务校验边界清晰。
- 可以用 Fake Repository 编写应用服务单元测试。
- 后续 MySQL Repository 能基于现有 `DatabaseExecutor` 实现事务写入。

**完成状态：** 已完成。已完成入库单领域模型、请求/结果 DTO、Repository Port、InboundService、FakeInboundRepository 和应用服务单元测试。测试覆盖认证、权限、参数校验、创建、确认、重复确认、列表查询、Repository 异常成功结果和 owner 销毁后的回调保护。

### Day 15：MySqlInboundRepository 基础链路与确认事务设计（当前任务）

**计划依据：** 优化版详细计划 C.8“入库/出库条件事务”第 1 项练习，以及 F.2 第 3 周“库存流水与入库”阶段。当前代码已经完成 Fake Repository，但尚未建立真实 `MySqlInboundRepository`，因此先完成 Repository 到 `DatabaseExecutor` 的真实接入和事务边界设计。

**目标：** 建立真实入库 Repository 的实现骨架，先完成草稿创建、按 ID/订单号查询、分页查询的数据库结果映射，并明确确认入库事务中订单状态、明细、库存余额、库存流水和审计日志的执行顺序。

**知识点：**

- Application Service、Repository Port、MySQL Repository 三者的职责边界。
- `DatabaseStatement`、`DatabaseTask` 与 Repository 结果 DTO 的映射。
- 订单 Header/Line 的多表查询与组装方式。
- 为什么 `confirmOrder` 必须由一个 Repository 事务任务完成，而不能由 Service 分步调用多个方法。
- 事务边界：状态更新、库存余额、库存流水和审计日志必须使用同一个 Worker 连接。
- `affectedRows` 守卫与数据库错误、业务状态错误的映射关系。

**实施任务：**

- 创建 `MySqlInboundRepository`，实现 `IInboundRepository` 的基础接口。
- 先实现 `createDraft()`：生成订单号、插入订单 Header、插入订单明细，并在成功后返回完整 `InboundOrder`。
- 实现 `findById()`、`findByOrderNo()` 和 `listOrders()` 的查询与 DTO/Entity 映射。
- 梳理 `confirmOrder()` 的事务步骤和 SQL 参数，不在 Service 中拼接 SQL。
- 明确确认事务的第一条状态守卫：`WHERE id = :id AND status = 'draft'`，并记录预期影响行数为 1。
- 为真实 Repository 保留 owner 生命周期保护，避免数据库异步完成后回调已销毁对象。

**测试任务：**

- 使用 Fake 或可控数据库测试验证 Repository 接收到的参数和返回对象映射。
- 增加创建订单号非空、订单明细正确保存、按 ID/订单号查询和分页查询测试。
- 增加确认事务设计测试或最小数据库联调，先验证草稿状态守卫和重复确认不会被当作成功。

**验收标准：**

- `MySqlInboundRepository` 不把 SQL 暴露给 Service 或 UI。
- 创建、查询和分页接口能够通过真实 `DatabaseExecutor` 执行并映射为领域对象/DTO。
- `confirmOrder()` 的事务步骤、连接归属和 affectedRows 守卫已经明确。
- Repository 异步回调具备 owner 生命周期保护。
- 代码能够编译，新增单元测试或真实链路测试通过。

### Day 16：入库确认事务实现与库存一致性（后续任务）

**目标：** 在 Day15 的 Repository 基础链路上，实现入库确认的单事务写入，保证订单状态、库存余额、库存流水和审计日志的一致性。

**重点任务：**

- 实现草稿订单到已确认订单的条件更新。
- 遍历入库明细，执行库存余额 UPSERT。
- 插入不可变库存流水，并使用来源单据、来源明细和 movement role 组成幂等键。
- 插入确认操作审计日志。
- 任一语句失败时回滚整个事务。

**验收标准：**

- 正常确认后订单状态、库存余额、库存流水和审计日志均存在。
- 重复确认不会重复增加库存或流水。
- 任一明细失败时，订单状态、余额、流水和日志全部回滚。

### Day 17：入库真实链路测试与异常注入（后续任务）

**目标：** 验证入库确认事务在真实 MySQL 链路中的回滚、幂等和错误映射行为。

**重点任务：**

- 注入重复确认、明细插入失败、库存余额更新失败和连接异常。
- 验证业务错误、数据库错误和事务回滚结果的映射。
- 对比确认前后的订单、库存余额、库存流水和审计日志。
- 补充 Repository 与 Service 的边界测试，确保 Service 不承担事务拼装职责。

**验收标准：**

- 事务失败注入测试能够证明全局回滚。
- 重复请求具备幂等结果，不产生重复库存变化。
- 真实 MySQL 链路测试通过，并留下可重复执行的验证方式。

> 说明：Day15-Day17 依据优化版详细计划 C.8 和 F.2 第 3 周展开。原始文件当前没有可检索到的“7.1 每日实施任务”小节，因此本滚动计划只采用其中明确写出的入库事务、库存流水、余额、审计和回滚要求，不自行扩展到出库或 UI 功能。
## 滚动更新规则

每天完成后：

- 先核对详细计划、代码、CMake 和测试结果。
- 在本文件中标记当天完成。
- 追加下一天计划，保持当前任务加后续 2-3 天窗口。
- 调整顺序时记录原计划位置和依赖理由。
- 无法确认计划依据时询问用户，不自行编造任务。
