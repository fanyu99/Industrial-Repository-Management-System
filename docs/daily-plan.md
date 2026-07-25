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

## 当前滚动计划

> 调整依据：认证与 Product 应用层纵向切片已完成，当前应继续沿详细计划 C.7 “Repository 与 Application Service”推进，把 `IProductRepository` 的 MySQL 实现接到已完成的异步数据库执行器上。Product MySQL Repository 稳定后，再进入 Product 页面/Model 和入库事务主线。

### Day 11：MySQL ProductRepository 与数据库结果映射（当前任务）

**目标：** 实现 `IProductRepository` 的 MySQL 版本，让 ProductService 可以在不修改上层接口的情况下使用真实数据库查询和写入物资数据。

**知识点：**

- Repository Port 与 Infrastructure Adapter 的关系。
- Prepared Statement、命名参数与 SQL 注入防护。
- `DatabaseExecutor` 异步任务到 Repository callback 的结果映射。
- `DatabaseResult`、`QVariantMap/QVariantList` 到 `Product`/DTO 的字段校验。
- 分页查询中的 `LIMIT/OFFSET`、`COUNT(*)` 与排序稳定性。
- Repository 错误到 `AppError` 的分类：数据库失败、唯一键冲突、记录不存在。

**实施任务：**

- 在 infrastructure 层设计 `MySqlProductRepository` 的位置、CMake target 和依赖方向。
- 实现 `listProducts()`：筛选条件、分页、总数、字段映射和排序。
- 实现 `findByCode()`：用于 ProductService 的唯一编码判断。
- 实现 `createProduct()`、`updateProduct()`、`setProductActive()` 的参数化 SQL。
- 统一处理 owner 生命周期：Repository 内部也使用 `QPointer`，不向已销毁 owner 回调。
- 为真实 Repository 准备集成测试策略，优先覆盖 SQL 映射和错误映射。

**验收标准：**

- `ProductService` 不需要改接口即可切换到 MySQL Repository。
- 所有 SQL 均使用参数绑定，不拼接用户输入。
- 分页结果包含 `items/total/page/pageSize`，并且排序稳定。
- 数据库列缺失、类型错误、SQL 失败能映射为可理解的 `AppError`。
- 物资编码重复、产品不存在、停用失败有明确错误路径。
- owner 销毁后不会触发上层 callback。

### Day 12：ProductTableModel 与物资列表页面查询闭环

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

### Day 13：物资创建、编辑、启用/停用 UI 与端到端纵向切片

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

## 滚动更新规则

每天完成后：

- 先核对详细计划、代码、CMake 和测试结果。
- 在本文件中标记当天完成。
- 追加下一天计划，保持当前任务加后续 2-3 天窗口。
- 调整顺序时记录原计划位置和依赖理由。
- 无法确认计划依据时询问用户，不自行编造任务。
