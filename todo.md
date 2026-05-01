# OP 项目任务清单（含 Issue 链接）

## P0（本周必须完成）

- [ ] **T1 稳定 SetDisplayInput 内存模式**
  - 目标：修复 `mem:` 输入路径，确保 Win10/11 + Python/C# 均可用
  - 关联 Issue：
    - https://github.com/WallBreaker2/op/issues/175
    - https://github.com/WallBreaker2/op/issues/173
  - 验收标准：最小复现通过；新增回归用例；文档更新

- [ ] **T2 修复绑定后闪退与卡死**
  - 目标：解决绑定窗口闪退、`GetCmdStr` 卡死、线程重启异常
  - 关联 Issue：
    - https://github.com/WallBreaker2/op/issues/133
    - https://github.com/WallBreaker2/op/issues/139
    - https://github.com/WallBreaker2/op/issues/140
  - 验收标准：3 次绑定/解绑无崩溃；超时可控；线程可重复初始化

- [ ] **T3 修复 SendString 失效**
  - 目标：修复发送文字在部分场景不生效问题
  - 关联 Issue：
    - https://github.com/WallBreaker2/op/issues/73
  - 验收标准：前后台发送均通过；补齐输入法/布局说明；新增回归脚本

- [ ] **T4 统一后台键鼠事件链路**
  - 目标：统一 DX9/DX11/normal 模式下键鼠行为一致性
  - 关联 Issue：
    - https://github.com/WallBreaker2/op/issues/103
    - https://github.com/WallBreaker2/op/issues/114
    - https://github.com/WallBreaker2/op/issues/124
    - https://github.com/WallBreaker2/op/issues/136
    - https://github.com/WallBreaker2/op/issues/163
    - https://github.com/WallBreaker2/op/issues/176
  - 验收标准：点击/滚轮/组合键/按键状态回归全通过；模式差异文档明确

---

## P1（下周完成）

- [ ] **T5 修复 OCR 32/64 位可用性**
  - 目标：修复 `OcrEx/OcrAuto` 在 x86/x64 + tess_model 下异常
  - 关联 Issue：
    - https://github.com/WallBreaker2/op/issues/145
    - https://github.com/WallBreaker2/op/issues/153
  - 验收标准：标准样本集返回非空；准确率达到基线；新增回归用例

- [ ] **T6 整理 OCR 引擎优化提案**
  - 目标：形成 OCR 改造路线（性能/精度）
  - 关联 Issue：
    - https://github.com/WallBreaker2/op/issues/118
  - 验收标准：输出方案文档；明确阶段目标与指标

- [ ] **T7 补齐 D3D12 / 3D 绑定说明与兼容**
  - 目标：完善 D3D12 绑定、错误码、3D 窗口截图说明
  - 关联 Issue：
    - https://github.com/WallBreaker2/op/issues/96
    - https://github.com/WallBreaker2/op/issues/132
  - 验收标准：示例可运行；错误码说明完整；FAQ 补充

- [ ] **T8 发布雷电后台完整例程**
  - 目标：提供“绑定 -> 截图 -> 找色 -> 键鼠”全流程示例
  - 关联 Issue：
    - https://github.com/WallBreaker2/op/issues/131
    - https://github.com/WallBreaker2/op/issues/171
  - 验收标准：示例可跑通；README 完整；可复现

- [ ] **T9 修复 C# 调用兼容问题**
  - 目标：修复 C#4.7 / .NET6 调用异常，补齐免注册流程
  - 关联 Issue：
    - https://github.com/WallBreaker2/op/issues/106
    - https://github.com/WallBreaker2/op/issues/138
    - https://github.com/WallBreaker2/op/issues/149
  - 验收标准：两套框架示例通过；文档可执行

- [ ] **T10 补 Node.js 接入示例**
  - 目标：提供 Node.js 最小可运行示例（安装/加载/调用/错误处理）
  - 关联 Issue：
    - https://github.com/WallBreaker2/op/issues/146
  - 验收标准：示例可运行；步骤清晰

---

## P2（增强与生态）

- [ ] **T11 新增 lockinput 能力**
  - 目标：提供锁定/解锁外部输入 API
  - 关联 Issue：
    - https://github.com/WallBreaker2/op/issues/77
  - 验收标准：API 可用；异常恢复机制；风险提示文档

- [ ] **T12 新增 WaitKey(0,0) 当前按键值**
  - 目标：支持直接返回当前按键值
  - 关联 Issue：
    - https://github.com/WallBreaker2/op/issues/122
  - 验收标准：返回值规范；键码映射表发布；示例通过

- [ ] **T13 评估 Java 生态方案**
  - 目标：明确官方支持边界并给出可维护 Java 路径
  - 关联 Issue：
    - https://github.com/WallBreaker2/op/issues/174
  - 验收标准：方案结论清晰；示例链接可用；维护策略明确

- [ ] **T14 社区类 Issue 归档策略**
  - 目标：非缺陷类 issue 统一转 Discussion / 关闭模板
  - 关联 Issue：
    - https://github.com/WallBreaker2/op/issues/164
  - 验收标准：模板生效；历史案例归档；贡献指南更新
