# Simuless 开发流程（跨 MCU / PC，BMAD 轻量版）

## 1. 总体目标

- 构建贯穿全生命周期的 MCU/PC 跨端工程。
- 第一阶段先打通并固化通信统一抽象。
- 第二阶段引入控制器/滤波器并支持代码生成。

生命周期主线：**统一通信抽象 → 控制/滤波模型化 → 代码生成 → 双端一致性验证**。

---

## 2. 阶段规划（Epic → Story → Sprint）

## Phase 1：通信统一抽象（当前）

### Epic 1.1 Transport 抽象固化
目标：统一 connection/channel 语义，面向 TCP/UDP/UART 扩展。
里程碑：
- M1：`transport` 纳入顶层 CMake 构建。
- M2：TCP client/server 语义一致，核心测试稳定。
- M3：UART/UDP 预留统一初始化与错误码语义。

DoD：
- API 头文件评审通过。
- 生命周期/错误码/边界测试通过。
- PC 侧最小链路可运行。

### Epic 1.2 Telemetry over Transport
目标：帧协议与传输层解耦并贯通端到端。
里程碑：
- M4：完成 `transport + frame_encode/frame_parser` 适配层。
- M5：分片、噪声、校验失败、断连恢复可验证。

### Epic 1.3 MCU/PC 双端落地
目标：完成 MCU 与 PC 双端最小可用实现（MVP）。

## Phase 2：控制器/滤波器 + 代码生成

### Epic 2.1 模型与 IR
目标：统一控制器/滤波器模型（PID、低通等）及 IR（JSON/YAML/DSL）。

### Epic 2.2 Codegen 引擎
目标：由模型生成 MCU C 与 PC 对照实现。

### Epic 2.3 一致性回归
目标：基于 golden vector 验证“模型行为 = 生成代码行为”。

---

## 3. 每个 Sprint 固定产物（BMAD 轻量）

1. Backlog：可交付 Story（建议 1~2 天粒度）
2. Architecture Note：接口/状态机/协议变更说明
3. Test Plan：正常流 + 边界 + 故障注入
4. Review Gate：评审结论 + 测试报告 + 可运行示例

---

## 4. 开发执行流程（Story 级）

1. 需求定义：输入/输出、时序、资源约束（RAM/CPU）
2. 接口先行：先定 `.h` 与状态机，再写实现
3. 测试先行：先补失败用例，再补实现
4. 最小可用：先跑通主链路，再做优化
5. 跨端验证：PC 仿真 + MCU 实机 + 回放比对
6. 回归冻结：版本标签、变更说明、示例同步

---

## 5. 代码风格约束（强制）

### 5.1 clang-format
- 必须使用仓库 `.clang-format`。
- 当前关键规则：
  - 4 空格缩进（`IndentWidth: 4`）
  - 行宽 100（`ColumnLimit: 100`）
  - Allman 大括号风格（`BreakBeforeBraces: Allman`）
  - 指针左对齐（`PointerAlignment: Left`）
- 提交前必须格式化；不通过不得合并。

### 5.2 clang-tidy
- 必须使用 `.clang-tidy.yaml`。
- 当前最低启用：`clang-analyzer-*`、`bugprone-*`。
- 合并前要求：
  - 无高风险静态分析告警
  - 无显著 bugprone 问题（空指针/UB/资源泄漏等）

### 5.3 C 工程规范
- 标准：C11。
- 编译警告按错误处理（`-Werror` / `/WX`）。
- 平台差异仅在 port 层隔离（禁止泄漏到业务层）。
- 接口层避免动态内存，保障 MCU 可控性。

---

## 6. Doxygen 注释约束（强制）

### 6.1 文件级（每个公共头文件必须）
- `@file`
- `@brief`
- 模块职责边界（负责/不负责）

### 6.2 对外符号（必须）
对外 `typedef/struct/enum/function` 必须包含：
- `@brief`
- `@param`
- `@return`
- 错误码语义与适用上下文（线程/中断，如有）

### 6.3 协议与状态机（必须）
- 状态定义与迁移条件
- 帧格式、字段、字节序、最大长度
- 异常恢复策略（重同步/超时/断连重连）

### 6.4 质量红线
- 注释不得与实现脱节。
- 接口变更必须同步更新 Doxygen。

---

## 7. 必要工程约束（跨 MCU/PC）

1. **边界约束**
   - `transport` 仅负责字节传输与连接语义。
   - `telemetry` 仅负责帧语义。
   - 控制器/滤波器不得直接耦合 socket/uart 细节。

2. **一致性约束**
   - MCU/PC 共用同一协议定义（字段、单位、量纲一致）。
   - 端序/对齐/打包必须显式定义并可测试。

3. **测试约束**
   - 每个 Epic 至少包含：1 主链路 + 1 边界 + 1 故障注入。
   - 必测回归：分片、噪声、校验失败、断连重连、多连接并发。

4. **合并门禁（Merge Gate）**
   - clang-format 通过
   - clang-tidy 通过
   - CTest 通过
   - 公共 API Doxygen 完整
   - 变更说明含影响范围与回归点

---

## 8. 近期执行建议（2~4 周）

### Sprint A：通信抽象固化
- `transport` 入顶层构建
- 修复抽象层接口一致性问题
- 完成生命周期/错误码单测

### Sprint B：端到端链路
- 完成 telemetry-over-transport 适配层
- 增加噪声/断连/分片测试
- 输出最小上位机交互 demo

### Sprint C：Phase 2 预埋
- 定义 controller/filter IR v0
- 完成 1 控制器 + 1 滤波器模板生成验证
- 建立 golden vector 回归框架

---

## 9. 生命周期验收总线

- L1：MCU ↔ PC 通信稳定（可断连恢复）
- L2：上位机可参数下发与状态回传
- L3：控制器/滤波器模型可生成代码
- L4：生成代码在 PC/MCU 行为一致且可回归

原则：优先达成 L1/L2，再扩大算法与代码生成范围。
