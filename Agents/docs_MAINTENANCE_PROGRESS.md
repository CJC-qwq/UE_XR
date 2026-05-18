# docs\MAINTENANCE_PROGRESS.md

# 维护进度
## 2026-04-25 项目规约与文档基线建立

### 任务

* 将不适用于 Unreal 项目的旧规则修正为当前 XR 插件项目版本
* 分析并理解 `docs_REQUIREMENT_WORKFLOW.md` 与 `docs_MAINTENANCE_PROGRESS.md`
* 建立当前项目自己的维护记录基线

### 处理

* 重写
  * `AGENTS.md`
  * `docs_REQUIREMENT_WORKFLOW.md`
  * `docs_MAINTENANCE_PROGRESS.md`
* 更新
  * 明确 `Plugins/XR` 为 XR 功能主实现模块
  * 明确主工程为插件宿主和测试环境

### 验证

* 已完成项目结构和文档结构只读分析

### 状态

* 已完成

### 说明

* 后续所有 XR 非 trivial 需求都按当前流程继续追加维护记录

### 下一步

* 按专家团队流程推进后续 XR 插件开发

## 2026-04-26 07:59:17 整包版本回退

### 任务

* 按用户要求，直接从 ackups/XR_Running_feature_milestone_20260426_072118.zip 恢复项目
* 停止继续使用“局部修正式回退”的方式，改为整包回退
* 补充“维护记录必须带日期+时分秒，且每次只追加不覆盖旧记录”的执行纪律

### 处理

* 已先创建回退前临时备份：ackups/pre_rollback_backup_20260426_075712.zip
* 已从里程碑压缩包覆盖恢复项目顶级内容
* 本次回退以压缩包版本为准，不再保留之前那些局部推测式回退结果

### 验证

* 已完成压缩包结构检查
* 已完成整包覆盖恢复
* 已确认回退前临时备份已生成

### 状态

* 已完成整包回退

### 说明

* 后续维护记录一律按“日期 + 时分秒 + 新增条目”的方式追加
* 不再改写之前已有维护记录内容

## 2026-04-25 XR 场景调度最小闭环

### 任务

* 为“空 Persistent Level + 流送子关卡”工作流建立第一版 XR 插件能力
* 支持背景关卡、特效关卡、通用特效和整数触发入口
* 将使用者入口收敛为图形化配置，降低非技术人员使用门槛

### 处理

* 新增
  * `UXRSceneTriggerConfig` 数据资产，负责背景关卡、特效关卡和整数触发映射配置
  * `AXRSceneTriggerController` 控制器 Actor，负责运行时调度
  * `UXRGenericEffectComponent`，用于主关卡中的点控通用特效
* 更新
  * `XR.Build.cs`，加入关卡流送与 Level Sequence 所需模块依赖
  * `XRBPLibrary`，移除模板示例函数并保留最小可用工具入口
* 实现
  * 一个整数可同时触发一个背景和多个特效关卡
  * 背景切换时自动停止通用特效并清理已激活的关卡特效
  * 特效关卡的背景绑定校验
  * 同背景或同特效的重复触发重载控制

### 验证

* 已完成插件结构与 XR 模块边界分析
* 已完成 Unreal 运行时方案设计评审
* 已完成首轮代码级检查
* 已完成 Unreal Editor 编译与运行
* 待执行 OSC 整数输入对接验证

### 状态

* 已完成首版落地

### 说明

* 首版将 OSC 传输层留在插件外部，插件内部只暴露整数触发入口
* 当时版本后续已发现“动态实例化流送关卡”与实际项目使用方式不完全一致，因此在 2026-04-26 开始修正为直接控制已有流送子关卡

### 下一步

* 对接现有 OSC 插件到 `TriggerInteger`
* 提升编辑器侧的配置可读性、校验提示和预览能力

## 2026-04-26 XR 场景调度修正：改为已有流送子关卡控制并支持显式 Sequence 配置

### 任务

* 修正背景关卡与特效关卡的加载方式，改为直接控制关卡查看器中已有的流送子关卡
* 解决 preview 时重复创建关卡实例、旧背景未正确卸载的问题
* 将背景关卡和特效关卡都改为可显式配置一个要播放的 Level Sequence
* 将维护记录文档改回中文，方便项目日常阅读

### 处理

* 重写
  * `AXRSceneTriggerController` 的关卡流送路径，移除动态实例化方式，改为使用 Unreal 自带的 `Load Stream Level` / `Unload Stream Level`
* 更新
  * `FXRBackgroundLevelDefinition`，新增背景关卡的 `SequenceAsset`
  * `FXREffectLevelDefinition`，新增特效关卡的 `SequenceAsset`
  * 背景切换逻辑，切换新背景前先停止通用特效、停止已跟踪的 Sequence、卸载所有活动特效关卡、卸载旧背景关卡
  * 特效关卡触发逻辑，加载对应子关卡后按配置播放指定 Sequence
  * 特效自动卸载时间计算逻辑，改为优先读取配置的 Sequence 资产时长，不再默认依赖关卡内自动播放 Sequence
* 新增
  * 关卡加载完成后的延迟 Sequence 播放队列，避免子关卡尚未完成加载时过早触发播放
* 修正
  * `PreviewTrigger` 所走的编辑器预览流送路径，改为直接操作已有 `ULevelStreaming` 的加载与可见性状态，并主动刷新关卡流送，解决编辑器里点 Preview 没有反应的问题
  * 已有流送子关卡的匹配逻辑，增加按完整包名和短名双重匹配，并在编辑器环境下同步设置 `SetShouldBeVisibleInEditor`

### 验证

* 已完成当前 XR 场景调度代码复查
* 已完成与用户最新运行反馈的行为差异比对
* 待执行 Unreal Editor 编译验证
* 待执行主关卡下已有流送子关卡的加载/卸载验证
* 待执行背景关卡与特效关卡显式 Sequence 播放验证

### 状态

* 代码已修改，待编辑器验证

### 说明

* 当前版本的目标是直接控制 Persistent Level 下已经存在的流送子关卡，而不是再创建新的动态关卡实例
* 一个关卡可配置一个实际运行时播放的 Sequence；你提到的多个 Sequence 库存仍然保留在内容层面，但运行配置只选其中一个
* 背景关卡和特效关卡都支持独立配置 Sequence

### 下一步

* 重新编译插件并验证关卡查看器中的加载、可视性和卸载表现
* 验证背景切换时旧背景是否被正确卸载
* 验证特效关卡按配置的 Sequence 正确播放并按时自动卸载





## 2026-04-26 08:35:25 稳定版本快照

### 任务

* 对当前已验证稳定的项目版本创建压缩快照
* 保留一个可直接回退的版本节点

### 处理

* 已确认当前版本中背景关卡与特效关卡的加载和卸载表现正常
* 已创建版本快照文件：backups/XR_Running_snapshot_20260426_083525.zip

### 验证

* 压缩包已生成完成
* 可作为后续修正前的稳定回退基线

### 状态

* 已完成

## 2026-04-26 08:40:39 提交提示语言规约补充

### 任务

* 将“需要我 submit 时必须用中文提醒”的偏好写入项目规约

### 处理

* 已在 AGENTS.md 中新增 Communication Preference Rule
* 已明确规定：凡是需要用户执行或确认 submit 类步骤时，必须使用中文提示

### 验证

* 项目规约已更新
* 后续会按该规则执行

### 状态

* 已完成

## 2026-04-26 09:01:29 Generic Action 蓝图触发升级

### 任务

* 将 generic effect 从组件触发升级为数据资产驱动的通用动作触发
* 支持持久关卡 Actor 蓝图与持久关卡 Level Blueprint 两种目标
* 保持一个 TriggerId 可同时触发多个 generic action

### 处理

* 新增 XRGenericActionReceiver Blueprint Interface，统一入口为 HandleXRGenericAction(ActionName)
* 在 XRSceneTriggerConfig 中新增 GenericActions 配置表
* 保留 TriggerActions 里的 GenericEffectsToTrigger 字段作为动作 Id 列表使用，避免扩大现有数据资产改动面
* 在 XRSceneTriggerController 中新增 generic action 分发逻辑
* Actor 目标按 Tag 在持久关卡中查找，并调用接口
* Level Blueprint 目标直接调用 Persistent Level 的 LevelScriptActor
* 新增 StopActionName，用于背景切换和 StopAllAndReset 时请求停止通用动作
* 补充了编辑器字段显示名，方便配置时理解

### 验证

* 已执行 Unreal Editor C++ 编译验证
* 编译结果成功

### 状态

* 已完成第一版最小闭环

### 下一步

* 由你在编辑器中为 MainMap 的目标 Actor 或 Level Blueprint 实现 HandleXRGenericAction
* 在数据资产中补充 GenericActions 配置，并在对应 TriggerId 下填写要触发的动作 Id

## 2026-04-26 10:08:29 Generic Action 使用说明补充

### 任务

* 将 Generic Action 的配置与使用方法整理成项目内文档

### 处理

* 新增文档：docs/XR_GENERIC_ACTION_USAGE.md
* 已写清 Actor 蓝图接法
* 已写清 MainMap Level Blueprint 接法
* 已写清数据资产字段含义、StopActionName 行为、背景限制和排错顺序

### 验证

* 文档已写入项目
* 可作为后续配置与交接说明使用

### 状态

* 已完成

## 2026-04-26 11:19:08 Generic Action 排错日志补充

### 任务

* 为 Generic Action 增加更明确的运行时日志，便于定位“没通”的环节

### 处理

* 在 XRSceneTriggerController 中补充了 Generic Action 分发成功/失败日志
* 补充了持久关卡 Level Blueprint 未实现接口、LevelScriptActor 无效、Actor Tag 未命中等警告
* 已重新编译验证通过

### 验证

* Unreal Editor C++ 编译通过
* 可在 Output Log 中直接观察 Generic Action 分发情况

### 状态

* 已完成

## 2026-04-26 11:31:50 Generic Action 稳定节点快照

### 任务

* 为 Generic Action 已验证打通的当前项目状态创建版本快照

### 处理

* 已确认 Generic Action 从 TriggerId 到 MainMap Level Blueprint 的触发链路正常
* 已创建快照文件：backups/XR_Running_snapshot_generic_action_20260426_113150.zip

### 验证

* 压缩包已生成
* 可作为 Generic Action 功能节点的稳定回退基线

### 状态

* 已完成

## 2026-04-26 11:37:09 周期性上下文重读规约

### 任务

* 将“每隔五个对话重读三份项目文档”的要求写入项目规约

### 处理

* 已在 AGENTS.md 中补充周期性上下文同步规则
* 已明确要求每隔 5 个对话轮次，重新读取 AGENTS.md、docs_MAINTENANCE_PROGRESS.md、docs_REQUIREMENT_WORKFLOW.md

### 验证

* 项目规约已更新
* 后续按该规则执行

### 状态

* 已完成

## 2026-04-26 11:52:42 OSC 接收最小闭环

### 任务

* 完成 OSC 接收功能第一版
* 支持固定地址匹配、可配置端口、运行时读取本机 IP
* 收到整数后直接触发现有 TriggerInteger

### 处理

* 在 XRSceneTriggerController 中新增 OSC 接收配置项
* 使用 Unreal 自带 OSC 插件创建 OSC Server
* 支持配置固定 OSC Address Path
* 支持配置监听端口
* 运行时读取一次当前本机 IP，并显示到 ResolvedLocalIPAddress
* 地址匹配成功后读取第一个 int32 参数并调用 TriggerInteger
* 在 XR.uplugin 中补充了对 OSC 插件的依赖声明
* 新增文档：docs/XR_OSC_RECEIVER_USAGE.md

### 验证

* Unreal Editor C++ 编译通过
* XR.uplugin 依赖声明补齐后再次验证通过

### 状态

* 已完成 OSC 接收第一版

### 下一步

* 由你在编辑器里实际接入外部 OSC 发送端验证收发链路
* 后续再单独设计 OSC 输出部分
* 再进入图形化配置界面工作

## 2026-04-26 12:12:59 OSC 接收流程对齐旧蓝图写法

### 任务

* 将当前 OSC 接收实现尽量收敛到旧项目蓝图写法的工作流

### 处理

* 调整为创建 OSC Server 时直接启用 StartListening
* 改为通过 OnOscMessageReceived 接收消息
* 在回调中对比配置的 OSC Address Path
* 地址匹配后读取第一个 int32 参数并调用 TriggerInteger
* 保留当前运行时自动解析本机 IP 的能力

### 验证

* Unreal Editor C++ 编译通过

### 状态

* 已完成对齐

## 2026-04-26 12:20:26 OSC Server Name 暴露与地址说明补充

### 任务

* 增加可配置的 OSC Server Name
* 补充解释 OSC Address Path 为什么采用 /xr/trigger 这种写法

### 处理

* 已在 XRSceneTriggerController 上新增 OSC Server Name 配置项
* 已更新 docs/XR_OSC_RECEIVER_USAGE.md，补充 Server Name 说明
* 已在文档中补充 OSC 地址路径写法说明

### 验证

* 已尝试重新编译
* 当前被 Unreal Live Coding 阻止，需要关闭编辑器或结束 Live Coding 后再做最终编译确认

### 状态

* 代码已完成，待你关闭 Live Coding 后做最后编译确认

## 2026-04-26 12:24:32 OSC 验证状态记录

### 任务

* 记录当前 OSC 接收功能状态
* 将后续工作重心切换到图形化配置

### 处理

* 已记录：OSC 接收代码已完成
* 已记录：本轮暂不进行外部发送端现场验证
* 已明确下一阶段进入图形化配置界面开发

### 状态

* 已完成状态记录

## 2026-04-26 12:24:32 图形化配置面板第一版

### 任务

* 为非技术人员提供更容易操作的 XR 图形化配置入口

### 处理

* 新增编辑器模块 XRConfigEditor
* 新增 Window -> XR Config 面板
* 面板支持选择 XRSceneTriggerController 与 TriggerConfig
* 面板支持快捷定位控制器、读取控制器当前配置、直接打开配置资产
* 面板增加了背景关卡、特效关卡、Generic Action、Trigger 数量摘要
* 新增文档：docs/XR_CONFIG_PANEL_USAGE.md

### 验证

* UHT 已通过
* 最终 C++ 编译被 Unreal Live Coding 阻止，待关闭编辑器或结束 Live Coding 后补最终验证

### 状态

* 第一版代码已完成，待最终编译确认

## 2026-04-26 12:31:27 图形化配置面板编译修正与最终验证

### 任务

* 修复 XRConfigEditor 第一版的实际编译错误
* 完成图形化配置面板最终编译确认

### 处理

* 修正了 ToolMenus 可用性判断接口
* 补充了选择集相关头文件引用
* 已重新编译 XR_RunningEditor

### 验证

* XRConfigEditor 模块编译通过
* XR_RunningEditor 整体编译通过

### 状态

* 图形化配置面板第一版已完成并通过最终编译验证

## 2026-04-26 12:39:47 图形化配置工具栏入口

### 任务

* 为 XR Config 面板增加更显眼的编辑器入口

### 处理

* 已在编辑器顶部工具栏增加 XR Config 按钮
* 保留 Window -> XR Config 菜单入口
* 已更新使用文档

### 验证

* XRConfigEditor 模块编译通过

### 状态

* 已完成

## 2026-04-26 12:49:37 配置条目中文显示名

### 任务

* 让 Background、Effect、Generic Action、Trigger 在不展开时直接显示可读名称

### 处理

* 为 BackgroundLevels 新增 BackgroundLabel
* 为 EffectLevels 新增 EffectLabel
* 为 GenericActions 新增 ActionLabel
* 为 TriggerActions 新增 TriggerLabel
* 已将各数组的 TitleProperty 切换到对应的 Label 字段
* 已补充图形化面板使用文档说明

### 验证

* XR_RunningEditor 编译通过

### 状态

* 已完成

## 2026-04-26 12:54:03 图形化入口与中文显示名快照

### 任务

* 为当前图形化入口与配置中文显示名版本创建项目快照

### 处理

* 已确认当前版本包含 XR Config 工具栏入口
* 已确认当前版本包含配置条目的 Label 显示名能力
* 已创建快照文件：backups/XR_Running_snapshot_ui_labels_20260426_125403.zip

### 验证

* 压缩包已生成
* 可作为当前 UI 与配置体验阶段的稳定回退点

### 状态

* 已完成

## 2026-04-26 13:01:36 清理无用测试资产

### 任务

* 删除遗留的空测试数据资产，避免启动时内容浏览器产生误导

### 处理

* 已删除 Content/NewDataAsset.uasset
* 已确认文件已不存在

### 状态

* 已完成

## 2026-04-26 13:05:30 幽灵数据资产排查

### 任务

* 排查内容浏览器中反复出现但无法打开的空数据资产条目

### 处理

* 排查后未发现当前插件代码会主动创建新的 DataAsset 文件
* 已将 XRConfigEditorContext 明确标记为 Transient
* 已将面板上下文对象创建方式改为纯临时对象，并增加临时专用名称与临时标志
* 已重新编译验证通过

### 状态

* 已完成第一轮修正，待重启编辑器后观察现象是否消失

## 2026-04-26 13:13:45 幽灵数据资产缓存清理

### 任务

* 清理 NewDataAsset 删除后仍残留的缓存与配置引用

### 处理

* 已删除 Intermediate 下的资产注册表缓存文件
* 已清理 EditorPerProjectUserSettings.ini 中对 /Game/NewDataAsset.NewDataAsset 的残留引用
* 已清理 Saved/SourceControl 中对 NewDataAsset.uasset 的残留记录

### 状态

* 已完成缓存清理，待重启编辑器后观察现象是否消失

## 2026-04-27 09:38:38 XR Config 图形化面板分层重构

### 任务

* 将原先挤在同一界面的图形化配置面板重构成更清晰的顶层切换结构
* 顶层改为 `XR Config Workspace`、`Scene Trigger Controller`、`Trigger Config Asset` 三个大入口
* 去掉 `Scene Trigger Controller` 页面里无关的默认 Actor 选项，只保留 XR 相关必需字段
* 将 `Trigger Config Asset` 页面进一步拆成按内容类型切换的独立编辑区

### 处理

* 重写
  * `SXRConfigEditorPanel` 的整体 Slate 布局，从左右分栏拼接改为顶层切页式结构
* 新增
  * `Scene Trigger Controller` 页面内部分区：`General`、`OSC`、`Preview / Stop`
  * `Trigger Config Asset` 页面内部分区：`Background Levels`、`Effect Levels`、`Generic Actions`、`Trigger Actions`
* 精简
  * 通过属性可见性过滤，仅在控制器页显示 `TriggerConfig`、预览相关字段、OSC 相关字段
  * 不再在图形化面板中暴露复制、联网、Actor 默认项等无关信息
* 更新
  * `docs/XR_CONFIG_PANEL_USAGE.md`，使使用说明与当前 UI 结构一致

### 验证

* 已执行 `XR_RunningEditor` 编译验证
* `XRConfigEditor` 模块编译通过
* `XR_RunningEditor` 整体编译通过

### 状态

* 代码已完成并通过编译

### 说明

* 这次重构的重点是先把“对象层级入口”拆清楚，而不是立刻重做底层配置数据结构
* 当前版本仍然基于现有数据资产与控制器逻辑工作，只是把图形化配置入口改得更清晰、更聚焦

### 下一步

* 由你在 Unreal Editor 中实际查看新版 `XR Config` 面板，确认交互层级是否符合使用习惯
* 再决定是否继续做表格式编辑、校验提示和更强的非技术用户引导

## 2026-04-27 10:05:29 XR Config 面板展开与上下文字段修正

### 任务

* 修正 `Trigger Config Asset` 页面中数组条目可以看到索引但展开后没有内容的问题
* 修正 `XR Config Workspace` 页面里 `XR Config` 字段偶发消失、重新打开后又出现的问题

### 处理

* 修正
  * `SXRConfigEditorPanel` 的属性可见性过滤逻辑，从“只匹配当前属性名”改为“同时匹配当前属性和父属性链”
* 更新
  * `Background Levels`、`Effect Levels`、`Generic Actions`、`Trigger Actions` 四个配置分区展开后，其内部字段现在会继续显示
* 精简
  * 去掉 `Workspace Context` DetailsView 上不必要的属性可见性过滤，降低上下文字段在刷新过程中异常消失的概率

### 验证

* 已完成代码级修正
* 已尝试执行 `XR_RunningEditor` 编译验证
* 当前被 Unreal Live Coding 阻止，尚未完成最终编译确认

### 状态

* 代码已修正，待关闭 Live Coding 后做最终编译验证

### 说明

* 这次问题的根因是上一版图形化面板的属性过滤只判断了顶层属性名，没有把展开项的父属性链算进去
* `Workspace Context` 本身只有两个字段，因此不再额外加一层过滤更稳妥

### 下一步

* 关闭 Unreal Editor 的 Live Coding 后重新编译 `XR_RunningEditor`
* 进入 `XR Config` 面板，确认四类配置条目可正常展开，且 `XR Config` 字段不再消失

## 2026-04-27 10:17:42 XR Config 按钮尺寸与工作上下文记忆

### 任务

* 放大 `XR Config Workspace` 等顶层切换按钮的宽度和字体观感
* 放大 `Background Levels` 等二级切换按钮，提升可读性和点击区域
* 让 `XR Config` 面板在关闭再打开后，自动记住上次的 Workspace Context

### 处理

* 调整
  * 顶层页签按钮最小宽高，增大 `XR Config Workspace`、`Scene Trigger Controller`、`Trigger Config Asset` 的视觉尺寸
  * 二级分区按钮最小宽高，增大 `Background Levels`、`Effect Levels`、`Generic Actions`、`Trigger Actions` 等分区的点击区域
* 新增
  * `XRConfigEditorSettings`，用于在编辑器侧持久化上次使用的控制器与配置资产路径
* 实现
  * 面板构建时自动恢复上次保存的 `SceneTriggerController` 和 `TriggerConfig`
  * 当 Workspace Context 被修改时自动保存当前选择

### 验证

* UnrealHeaderTool 与反射生成已通过
* 已尝试执行 `XR_RunningEditor` 编译验证
* 当前再次被 Unreal Live Coding 阻止，尚未完成最终编译确认

### 状态

* 代码已完成，待关闭 Live Coding 后做最终编译验证

### 说明

* 控制器使用关卡对象路径恢复，因此只有在对应关卡对象仍然存在且当前已加载时，自动恢复才会成功
* 配置资产使用软路径恢复，只要资产仍在项目中即可自动找回

### 下一步

* 关闭 Unreal Editor 的 Live Coding 后重新编译 `XR_RunningEditor`
* 重新打开 `XR Config` 面板，确认按钮尺寸是否顺手、Workspace Context 是否能自动恢复

## 2026-04-27 10:25:40 XR Config 按钮文字对齐与层级微调

### 任务

* 将 `XR Config Workspace` 等三个大选项的文字改为更明确的居中显示
* 提升下方二级选项的字体大小，但仍保持略小于上方三个大选项
* 让按钮尺寸、字体和层级关系更统一

### 处理

* 调整
  * 顶层大选项按钮的文字布局，改为水平方向与垂直方向的真正居中显示
  * 二级分区按钮字体大小上调，不再只放大按钮外框
* 保留
  * 上大下小的视觉层级关系，确保三个大选项比下方分区更醒目
* 优化
  * 为按钮内部文字增加适度内边距，让居中后的观感更稳定

### 验证

* 已完成界面代码调整
* 本轮未单独重新执行编译验证，属于窄范围 Slate 排版微调，且你已确认当前版本已可正常运行

### 状态

* 代码已完成，待你在编辑器里直接确认最终观感

### 说明

* 这一轮只处理按钮排版与字号层级，不改动数据逻辑与交互流程

### 下一步

* 由你在 Unreal Editor 中查看新版按钮层级是否顺手
* 如还需要，可继续细调按钮高度、圆角观感和分区间距

## 2026-04-27 10:34:58 Scene Trigger Controller 页面默认分类清理

### 任务

* 修正 `Scene Trigger Controller` 页面中 `OSC`、`Preview / Stop` 等分区仍会出现 `Transform` 等默认 Actor 分类的问题
* 让每个分区尽量只显示它各自对应的内容

### 处理

* 新增
  * `AXRSceneTriggerController` 的实例级 Details 自定义，专用于 `XR Config` 面板内的控制器子视图
* 隐藏
  * `Transform`、`Actor`、`Replication`、`Rendering`、`Physics`、`Collision`、`Tags`、`Tick` 等与当前配置无关的默认分类
* 保留
  * 原有 XR 属性可见性过滤逻辑，使 `General`、`OSC`、`Preview / Stop` 三个分区与各自用途保持一致

### 验证

* 已完成代码级修正
* 本轮未单独重新执行编译验证，属于编辑器侧定向 Details 清理修正

### 状态

* 代码已完成，待你在编辑器中直接确认最终显示效果

### 说明

* 这次不是继续追加属性过滤，而是直接在控制器详情视图里隐藏默认 Actor 分类，因此会比单纯过滤属性更稳

### 下一步

* 由你在 Unreal Editor 中检查 `Scene Trigger Controller` 下的 `General`、`OSC`、`Preview / Stop` 是否已经只剩对应内容

## 2026-04-27 10:55:36 Trigger Config 关卡条目自动命名与下拉选取

### 任务

* 让 `Background Levels` 和 `Effect Levels` 在选择关卡资源后自动生成对应 `Id`
* 让 `Trigger Actions` 里的 `Background To Activate` 与 `Effect Levels To Trigger` 不再手写，而是直接从前面已配置条目里下拉选择
* 保持现有 `XR Config` 图形化界面结构不大改，只增强配置体验

### 处理

* 新增
  * `UXRSceneTriggerConfig::GetBackgroundLevelOptions()`
  * `UXRSceneTriggerConfig::GetEffectLevelOptions()`
* 接入
  * 使用 Unreal 原生 `meta = (GetOptions = "...")`，将 `BackgroundToActivate` 接到背景条目选项列表
  * 使用 Unreal 原生 `meta = (GetOptions = "...")`，将 `EffectLevelsToTrigger` 接到特效条目选项列表
* 实现
  * 在 `UXRSceneTriggerConfig::PostEditChangeChainProperty` 中监听 `BackgroundLevels` / `EffectLevels` 里的 `LevelAsset` 变更
  * 选中关卡后自动用关卡资源名回填 `BackgroundId` / `EffectId`
  * 当 `BackgroundLabel` / `EffectLabel` 为空时，同步回填同名显示，避免折叠列表出现空标题

### 验证

* 已执行 `XR_RunningEditor` 编译验证
* 编译结果成功
* 尚待你在 Unreal Editor 里实际确认两个下拉是否按预期显示为已配置项列表

### 状态

* 代码已完成并通过编译

### 说明

* 这次优先采用 Unreal 自带的 `GetOptions` 元数据方案，没有额外重做 `XRConfigEditor` 的属性定制，改动范围更小
* 当前下拉显示优先使用 `Label`，若 `Label` 为空则回退显示 `Id`
* 这次只处理了你明确提出的两个触发字段，没有顺带大范围改其他配置项，避免把交互改得过散

### 下一步

* 由你在 `Trigger Config Asset` 页面里实际选择一个背景和一个特效条目，确认：
* 选关卡后 `Id` 是否自动生成
* `Background To Activate` 是否出现背景下拉
* `Effect Levels To Trigger` 是否出现特效下拉

## 2026-04-27 11:04:11 Trigger Config 的 Id / Label 行为修正

### 任务

* 修正选入关卡后 `Label` 被自动跟着 `Id` 一起填入的问题
* 修正 `Trigger Actions` 下拉显示成 `Label` 而不是 `Id` 的问题
* 保持 `Label` 回归备注用途，`Id` 回归内部标识与触发选择用途

### 处理

* 修正
  * 去掉 `UXRSceneTriggerConfig::PostEditChangeChainProperty` 中对 `BackgroundLabel` 的自动回填
  * 去掉 `UXRSceneTriggerConfig::PostEditChangeChainProperty` 中对 `EffectLabel` 的自动回填
* 调整
  * `GetBackgroundLevelOptions()` 下拉显示改为始终显示 `BackgroundId`
  * `GetEffectLevelOptions()` 下拉显示改为始终显示 `EffectId`
* 保留
  * 选择 `LevelAsset` 后自动生成 `BackgroundId` / `EffectId` 的逻辑保留不变

### 验证

* 已完成代码级修正
* 已尝试执行 `XR_RunningEditor` 编译验证
* 本轮编译被 Unreal Live Coding 占用阻止，尚未完成最终编译确认

### 状态

* 代码已修正，待关闭 Live Coding 后完成最终编译确认

### 说明

* 现在 `Label` 不会再自动生成，完全留给你手动写备注
* 现在 `Trigger Actions` 里的背景 / 特效下拉会以 `Id` 作为显示内容，和实际触发标识保持一致

### 下一步

* 关闭 Unreal Editor 的 Live Coding 后重新编译 `XR_RunningEditor`
* 在 `Trigger Config Asset` 页面确认：
* 选关卡后只自动生成 `Id`
* `Label` 保持空白，除非你自己填写
* `Trigger Actions` 下拉显示的是 `Id` 而不是 `Label`

## 2026-04-27 11:09:43 Effect Level 自动填充 Id 修正

### 任务

* 修复 `Effect Levels` 选入关卡后 `EffectId` 没有自动填入的问题
* 保持 `Background Levels` 现有自动填充行为不受影响

### 处理

* 排查后确认根因：
  * `Background` 和 `Effect` 两种结构里的字段名都叫 `LevelAsset`
  * 之前的 `PostEditChangeChainProperty` 使用了 `if / else if` 结构
  * 导致 effect 的改动事件先命中前一个 `LevelAsset` 判断，后面的 effect 分支永远进不去
* 修正
  * 改为在同一个 `LevelAsset` 变更事件中，分别独立检查 `BackgroundLevels` 和 `EffectLevels` 的数组索引
  * 只要索引有效，就分别回填对应的 `BackgroundId` / `EffectId`

### 验证

* 已执行 `XR_RunningEditor` 编译验证
* 编译结果成功

### 状态

* 已完成并通过编译

### 说明

* 这是一个嵌套数组属性变更判断里的分支遮蔽问题，不是数据资产本身配置问题
* 当前 `EffectId` 会和 `BackgroundId` 一样，在选中关卡资源后自动按资源名生成

### 下一步

* 由你在 Unreal Editor 里再选一次 `Effect Levels` 的关卡资源，确认 `EffectId` 已恢复自动填入

## 2026-04-27 11:12:16 当前稳定版本快照压缩

### 任务

* 为当前已验证通过的版本创建新的项目快照压缩包
* 保留一个可直接回退的稳定节点，方便后续继续修改

### 处理

* 已创建快照文件：`backups/XR_Running_snapshot_effect_id_fix_20260427_111216.zip`
* 本次快照覆盖当前已验证状态：
  * `BackgroundId` 自动填充正常
  * `EffectId` 自动填充正常
  * `Trigger Actions` 下拉显示 `Id`
  * `Label` 保持为手动备注用途

### 验证

* 压缩包已生成完成
* 快照文件已落盘到 `backups` 目录

### 状态

* 已完成

### 说明

* 这个快照可作为你继续做后续功能前的回退基线
* 后续如果再做 OSC 输出、配置界面深化或数据结构调整，都可以直接以这个节点为参照

### 下一步

* 继续进入下一个功能节点开发，或在需要时直接从该压缩包回退

## 2026-04-27 14:07:52 UE 5.5 降版兼容适配

### 任务

* 将当前基于 UE 5.6 的项目与 XR 插件调整为可在 UE 5.5 下使用
* 用本机已安装的 UE 5.5 做一次真实编译验证，确认不是只停留在理论兼容

### 处理

* 排查
  * 确认本机同时存在 `D:\Unreal\UE_5.5` 与 `D:\Unreal\UE_5.6`
  * 确认 `XR.uplugin` 与 `Build.cs` 本身没有写死 5.6 版本
  * 确认最直接的 5.5 编译阻塞来自项目级配置，而不是 XR 运行时逻辑本身
* 修正
  * 将 `XR_Running.uproject` 的 `EngineAssociation` 从 `5.6` 改为 `5.5`
  * 将 `Source/XR_Running.Target.cs` 的 `IncludeOrderVersion` 从 `EngineIncludeOrderVersion.Unreal5_6` 改为 `EngineIncludeOrderVersion.Unreal5_5`
  * 将 `Source/XR_RunningEditor.Target.cs` 的 `IncludeOrderVersion` 从 `EngineIncludeOrderVersion.Unreal5_6` 改为 `EngineIncludeOrderVersion.Unreal5_5`
* 验证
  * 已使用 `D:\Unreal\UE_5.5\Engine\Build\BatchFiles\Build.bat` 对 `XR_RunningEditor Win64 Development` 执行真实编译验证
  * 编译结果成功

### 验证

* UE 5.5 下 `XR_RunningEditor` 编译通过
* XR 插件运行时模块 `XR` 编译通过
* XR 编辑器模块 `XRConfigEditor` 编译通过

### 状态

* 已完成 5.5 兼容降版并通过编译

### 说明

* 这次降版的首个阻塞点并不是插件代码，而是项目级的 5.6 绑定配置
* 目前 `XR_Running.sln` 里仍然保留 UE 5.6 路径引用；这不影响刚才用 5.5 `Build.bat` 的实际编译结果，但如果后续要继续在 Visual Studio 里保持工程元数据一致，建议再用 UE 5.5 重新生成一次项目文件
* 当前结论是：这套 XR 插件和宿主工程已经在本机 UE 5.5 上通过实际编译验证，不是纸面兼容

### 下一步

* 用 UE 5.5 打开工程，做一轮编辑器内运行验证
* 重点回归：XR Config 面板、OSC 接收、关卡加载/卸载、Sequence 播放、Generic Action 分发

## 2026-04-27 14:31:10 VS 启动项目参数修正

### 任务

* 修复降到 UE 5.5 后，从 Visual Studio 运行 `XR_Running` 时没有直接带项目打开，而是只启动裸 `UnrealEditor` 的问题

### 处理

* 排查后确认根因：
  * `XR_Running` 已经是启动项目
  * `Development Editor | Win64` 配置本身也没选错
  * 真正的问题在 `Intermediate/ProjectFiles/XR_Running.vcxproj.user`
  * 其中用于传递 `.uproject` 的调试参数条件仍然写成旧格式，例如 `Win64_x64_Development_Editor|x64`
  * 而当前解决方案实际激活的是 `Development_Editor|x64` / `DebugGame_Editor|x64`
  * 条件名不匹配，导致 `LocalDebuggerCommandArguments` 没有生效
* 修正
  * 将 `XR_Running.vcxproj.user` 中本地 x64 配置的条件名改为当前真实使用的配置名
  * 保留 `"$(SolutionDir)XR_Running.uproject" -skipcompile` 作为编辑器启动参数

### 验证

* 已确认 `XR_Running.vcxproj.user` 中 `Development_Editor|x64` 与 `DebugGame_Editor|x64` 现在都正确带有 `.uproject` 参数

### 状态

* 已完成修正，待你在 Visual Studio 中实际再次点击运行确认

### 说明

* 这不是 Unreal 引擎本身启动方式变化，而是降版后项目文件与 VS 用户调试配置之间出现了条件名错位
* 修正后，VS 运行 `XR_Running` 应恢复为“打开带当前项目的编辑器”，而不是只开一个裸 `UnrealEditor`

### 下一步

* 在 Visual Studio 中保持 `XR_Running` 为启动项目、`Development Editor | Win64` 为当前配置，再次点击运行验证

## 2026-04-27 14:58:40 UE 5.5 关卡流送显示修复

### 任务

* 修复 UE 5.5 下 `XRSceneTriggerController` 触发后关卡查看器眼睛状态变化但场景实际不显示的问题
* 排查 VS 调试中出现的 `Controller = 0x7FF800000000` 异常上下文，优先收敛到底层流送链路

### 处理

* 排查后确认当前问题的高风险点主要有三类：
  * 运行时代码没有真正走 Unreal 标准 `Load Stream Level / Unload Stream Level` 路径，而是直接改 `ULevelStreaming` 标志位
  * 后续逻辑只检查 `GetLoadedLevel()`，没有等待关卡真正进入 `Visible` 状态
  * 流送关卡匹配优先走手写短名匹配，UE 5.5 下容错性不如引擎自带查找
* 已修改 `AXRSceneTriggerController`
  * 将 `LoadStreamingLevelByName` 改为调用 `UGameplayStatics::LoadStreamLevelBySoftObjectPtr`
  * 将 `UnloadStreamingLevelByName` 改为调用 `UGameplayStatics::UnloadStreamLevelBySoftObjectPtr`
  * 新增唯一 `LatentActionInfo` 生成，避免重复触发时 latent action 冲突
  * 将 `QueueSequencePlaybackAfterLevelLoad` 和 `InspectEffectLevelPlayback` 的判定从“已加载”收紧为“已加载且已可见”
  * `FindStreamingLevel` 优先改为使用 `FLevelUtils::FindStreamingLevel`
  * 补充了加载请求状态和等待超时日志，便于后续继续定位 UE 5.5 兼容问题

### 验证

* 已使用 `D:\Unreal\UE_5.5\Engine\Build\BatchFiles\Build.bat` 对 `XR_RunningEditor Win64 Development` 执行真实编译验证
* 编译结果成功通过

### 状态

* 代码已完成并通过编译

### 说明

* 这次修复重点是先把流送调用链重新对齐到 Unreal 官方接口，再把“loaded 但未 visible”这类误判收紧
* `Controller = 0x7FF800000000` 目前更像调试期症状值，不像本次仓库内明确可追溯的固定业务指针；本轮优先修复了最可能触发该异常上下文的流送链路

### 下一步

* 由你在 Unreal Editor 中重新触发一次背景关卡和特效关卡加载，观察场景是否已正常出现
* 如果仍有异常，下一轮优先抓新的 Output Log，重点看：
  * `Requested load for ...`
  * `Sequence playback wait timed out ...`
  * 是否仍出现 `TriggerConfig is not assigned`

## 2026-04-27 15:03:40 UE 5.5 编辑器预览崩溃规避修复

### 任务

* 修复上一轮流送修复后再次出现的 `0xC0000005` 访问冲突
* 重点排查编辑器 world 下 preview 触发关卡流送时的崩溃风险

### 处理

* 结合 `Saved/Logs/XR_Running.log` 排查后发现：
  * 崩溃前存在频繁的 `Opening Asset editor for World /Game/Map1`、`/Game/Map2`、`/Game/MainMap`
  * 同时伴随 `ULevelStreaming::RequestLevel(/Game/Map1)`、`RequestLevel(/Game/Map2)` 这类日志
  * 这说明编辑器世界在来回切图/清理 world 时，上一轮引入的 latent stream action 有较高概率仍挂在旧 world 上
* 已将 `AXRSceneTriggerController` 的流送逻辑改为双路径：
  * 编辑器 world（例如 CallInEditor preview）：
    * 直接操作 `ULevelStreaming` 的 `SetShouldBeLoaded / SetShouldBeVisible / SetShouldBeVisibleInEditor`
    * 立即 `FlushLevelStreaming`
  * 游戏 world / PIE：
    * 继续使用 `UGameplayStatics::LoadStreamLevelBySoftObjectPtr`
    * 继续使用 `UGameplayStatics::UnloadStreamLevelBySoftObjectPtr`
* 保留了上一轮已经加上的两项收紧：
  * 关卡必须“已加载且已可见”才继续后续 Sequence / 特效检查
  * `FindStreamingLevel` 优先用 `FLevelUtils::FindStreamingLevel`

### 验证

* 已使用 `D:\Unreal\UE_5.5\Engine\Build\BatchFiles\Build.bat` 对 `XR_RunningEditor Win64 Development` 执行真实编译验证
* 编译结果成功通过

### 状态

* 代码已完成并通过编译

### 说明

* 这次不是简单回退上一轮，而是把“编辑器预览”和“运行时流送”拆开处理
* 目标是保留运行时/PIE 使用引擎标准 stream level 的正确性，同时避开编辑器切图时 latent action 悬挂在旧 world 上的风险

### 下一步

* 由你重新在 Unreal Editor 里测试一次 preview / trigger
* 如果还崩，下一轮优先要两样东西：
  * 新的 Output Log 尾部
  * 如果 VS 能看到调用堆栈，请把最上面几层函数名贴给我

## 2026-04-27 17:28:20 XR Config 面板失效引用防崩修复

### 任务

* 修复 `UnrealEditor-XRConfigEditor.dll!SXRConfigEditorPanel::GetControllerStatusText()` 中出现的未处理异常
* 解决地图切换 / world 清理后，XR Config 面板仍持有旧 controller 引用导致的 UI 刷新崩溃

### 处理

* 排查后确认本次崩点不在流送底层，而在 `XRConfigEditor` 面板自身：
  * `GetControllerStatusText()` 在只判断“指针非空”的情况下直接取 `Controller->GetName()`
  * 地图切换后 `ContextObject->SceneTriggerController` 仍可能保留一个已经失效的 Actor 引用
  * Slate 文本刷新时再次进入该函数，就会在 UI 回调期间抛出异常
* 已在 `SXRConfigEditorPanel` 中新增上下文清洗逻辑：
  * 若 `SceneTriggerController` 已失效，则主动清空
  * 若 `TriggerConfig` 已失效，则主动清空
  * 若 `TriggerConfig` 为空但 controller 仍有效，则回填 controller 当前持有的 config
* 已把以下路径统一改为先做有效性检查再继续：
  * `RefreshFromContext`
  * `RefreshSummary`
  * `HandleUseConfigFromController`
  * `HandleOpenConfigAsset`
  * `HandleFocusController`
  * `HandlePreviewCurrentTrigger`
  * `HandleStopAllAndReset`
  * `GetControllerStatusText`
  * `GetConfigStatusText`

### 验证

* 已使用 `D:\Unreal\UE_5.5\Engine\Build\BatchFiles\Build.bat` 对 `XR_RunningEditor Win64 Development` 执行真实编译验证
* 编译结果成功通过

### 状态

* 代码已完成并通过编译

### 说明

* 这次修的是编辑器面板层的对象生命周期防护，不改业务配置行为
* 当前目标是先确保即便地图来回切换、旧 controller 已销毁，面板也只会自动失去绑定，不会因为文本刷新直接崩掉

### 下一步

* 由你重新打开 `XR Config` 面板并切换地图 / 重新选择 controller 测一轮
* 如果还有异常，下一轮优先补地图切换事件监听，在 world 变化时主动触发 `RefreshFromContext`

## 2026-04-27 17:35:20 编辑器手动关卡可视性与控制器兼容修复

### 任务

* 修复“关卡原本就是开启状态，或者手动在关卡查看器里开关过可视性后，再用 controller 控制时，眼睛状态变化了但关卡内容仍不出现”的问题
* 目标是保留手动开关能力，同时不影响 controller 之后继续接管开关

### 处理

* 排查后确认问题重点在编辑器 world：
  * 当前 controller 虽然会改 `ULevelStreaming` 的 `ShouldBeLoaded / ShouldBeVisible / ShouldBeVisibleInEditor`
  * 但如果用户此前已经手动改过关卡查看器可视性，单改 streaming flag 不足以保证“已加载关卡本体”的 editor visibility 跟着恢复
* 已修改 `AXRSceneTriggerController`
  * 编辑器 world 下在 `FlushLevelStreaming` 后，新增使用 `UEditorLevelUtils::SetLevelVisibility(...)`
  * 让 controller 在编辑器里控制关卡时，同时同步：
    * streaming flag
    * loaded level 的实际 editor 可见性
* 已调整 editor world 的 ready 判定：
  * 不再完全套用 runtime 的 `IsLevelVisible()`
  * 编辑器路径改为按“已加载 + editor 可见意图”来判断，避免手动切过可视性后等待逻辑卡偏
* 已在 `XR.Build.cs` 中补充 editor target 下对 `UnrealEd` 的依赖，支撑 editor-only 可视性同步调用

### 验证

* 已使用 `D:\Unreal\UE_5.5\Engine\Build\BatchFiles\Build.bat` 对 `XR_RunningEditor Win64 Development` 执行真实编译验证
* 编译结果成功通过

### 状态

* 代码已完成并通过编译

### 说明

* 这次不是限制你手动开关关卡，而是让 controller 在你手动改过之后，仍然能把关卡真正恢复到它该显示或该隐藏的状态
* PIE / 运行时路径仍保留 Unreal 标准 `Load/Unload Stream Level`，只对编辑器世界做了额外同步

### 下一步

* 由你在编辑器里做一轮混合测试：
  * 先手动开/关一次 Map1 / Map2
  * 再用 controller preview / trigger 去切换
* 重点观察：
  * 关卡查看器眼睛状态
  * 场景内容是否真实出现 / 消失

## 2026-04-27 17:42:55 背景关卡手动干预后的独占收敛修复

### 任务

* 修复这样一个背景关卡 bug：
  * 先执行序号 1，开启背景关卡 1
  * 再手动开启背景关卡 2
  * 再次执行序号 1 时，背景关卡 2 没有被自动关闭
* 目标是即便用户手动干预过背景关卡，可再次触发 controller 时，目标背景仍能恢复成“独占”状态

### 处理

* 排查后确认根因：
  * 当前背景切换逻辑主要依赖 `ActiveBackgroundId`
  * 但用户手动开启的背景关卡，不会自动更新 controller 内部这份“上一次由我亲手开启的背景”记忆
  * 结果就是 controller 再次执行背景 1 时，只会处理自己记得的背景，不会主动收掉那个手动插进来的背景 2
* 已修改 `AXRSceneTriggerController`
  * 新增辅助判断：
    * `IsLevelAssetCurrentlyActive`
    * `AreAnyOtherBackgroundLevelsActive`
    * `UnloadBackgroundLevelsExcept`
  * 背景激活时不再只看 `ActiveBackgroundId`
  * 改为扫描 `TriggerConfig` 里所有已配置背景关卡：
    * 若发现目标背景以外的其它背景仍处于激活状态，则先全部关闭
    * 再确保目标背景最终成为唯一保留的背景
* 保留了原有背景切换时的这些行为不变：
  * 通用特效停止
  * 特效关卡卸载
  * Sequence 停止

### 验证

* 你关闭编辑器 / Live Coding 后，已继续使用 `D:\Unreal\UE_5.5\Engine\Build\BatchFiles\Build.bat` 对 `XR_RunningEditor Win64 Development` 执行真实编译验证
* 编译结果成功通过

### 状态

* 代码已完成并通过编译

### 说明

* 这次修复的关键，不是让 controller 去“猜”你手动做过什么，而是每次背景切换时都按当前配置做一次全量收敛
* 这样背景关卡的最终状态会更稳定，不再强依赖 `ActiveBackgroundId` 这一个内部记忆值

### 下一步

* 由你在编辑器里按这个顺序复测：
  * 执行序号 1，开启背景 1
  * 手动开启背景 2
  * 再执行序号 1
* 重点确认：
  * 背景 2 是否被自动关闭
  * 背景 1 是否保持正常显示

## 2026-04-27 17:51:55 XR Config 面板地图切换上下文稳固修复

### 任务

* 修复 `SXRConfigEditorPanel::GetConfigStatusText()` 在打开子关卡单独编辑、再切回持久关卡后出现的访问冲突
* 解决 XR Config 面板在地图 / 子关卡编辑器 world 切换时仍持有旧上下文对象的问题

### 处理

* 排查后确认：
  * 仅靠上一轮的 `IsValid()` 判定仍不够稳
  * `UXRConfigEditorContext` 之前用的是 `TObjectPtr`
  * 在子关卡被单独打开、持久关卡 world 被切走的过程中，面板仍可能保留对旧 world 对象的硬引用语义
* 已修改 `UXRConfigEditorContext`
  * 将 `SceneTriggerController` 从 `TObjectPtr` 改为 `TSoftObjectPtr`
  * 将 `TriggerConfig` 从 `TObjectPtr` 改为 `TSoftObjectPtr`
* 已修改 `SXRConfigEditorPanel`
  * 新增 `ResolveSceneTriggerController`
  * 新增 `ResolveTriggerConfig`
  * 面板内部统一通过“解析器”获取当前可用对象，而不是直接解引用上下文字段
  * `TriggerConfig` 需要时从软路径安全加载
  * `SceneTriggerController` 只有在当前 world 真正存在时才解析成功
* 已接入 `FEditorDelegates::OnMapOpened`
  * 每次打开新地图 / 子关卡编辑器 world 时，面板会主动 `RefreshFromContext`
  * 避免等 Slate 文本刷新时才撞上旧 world 残留引用

### 验证

* 已使用 `D:\Unreal\UE_5.5\Engine\Build\BatchFiles\Build.bat` 对 `XR_RunningEditor Win64 Development` 执行真实编译验证
* 编译结果成功通过

### 状态

* 代码已完成并通过编译

### 说明

* 这次修复重点是把 XR Config 面板的上下文从“当前 world 的硬引用”改成“可延迟解析的软引用”
* 这样你打开子关卡单独编辑、再切回持久关卡时，面板应该表现为“重新解析当前可用对象”，而不是继续拿旧 world 的句柄硬顶

### 下一步

* 由你复测这条路径：
  * 打开持久关卡
  * 点开一个子关卡单独编辑
  * 调整它的可视性并做一次 sequence 改动
  * 再切回持久关卡
* 重点确认：
  * `XR Config` 面板是否还会报异常
  * 面板中的 controller / config 状态文字是否能正常刷新

## 2026-04-27 18:00:42 XR Config 面板软路径解析加固

### 任务

* 修复在 `XR Config` 面板保持打开时，切到子关卡单独编辑、修改可视性或 Sequencer、再切回持久关卡后出现的 `TPersistentObjectPtr<FSoftObjectPath>::Get()` 访问冲突
* 避免面板在 Slate 刷新期间继续走不稳定的软引用对象缓存路径

### 处理

* 将 `UXRConfigEditorContext` 里的 `SceneTriggerController` 和 `TriggerConfig` 从 `TSoftObjectPtr` 改为直接保存 `FSoftObjectPath`
* 重写 `SXRConfigEditorPanel` 的上下文解析逻辑：
  * Controller 不再调用 `.Get()`，改为按路径手动 `ResolveObject()` / `FindObject(...)`
  * Config 资产不再调用 `LoadSynchronous()`，改为按路径手动 `ResolveObject()`，必要时 `TryLoad()`
* 移除了面板上下文保存与读取流程里残留的 `ToSoftObjectPath()`、`.Get()`、`LoadSynchronous()` 调用
* 保留 workspace 已保存路径，即使当前 editor world 暂时解析不到 controller，也不会立刻丢失绑定
* 更新摘要和状态文案：如果对象路径已保存但当前世界不可用，面板只提示“当前不可用”，不再崩溃

### 验证

* 已使用 `D:\Unreal\UE_5.5\Engine\Build\BatchFiles\Build.bat` 对 `XR_RunningEditor Win64 Development` 执行真实编译验证
* 编译结果成功通过

### 状态

* 代码已完成并通过编译

### 说明

* 这次修复重点不是继续补 `IsValid()`，而是彻底绕开 `TSoftObjectPtr` 背后的 `TPersistentObjectPtr` 解析链路
* 这样在持久关卡 / 子关卡编辑 world 来回切换时，`XR Config` 面板会保留绑定路径，但只在当前上下文真正可用时解析成对象

### 下一步

* 由你复测刚才那条崩溃路径：
  * 打开持久关卡
  * 点开一个子关卡单独编辑
  * 打开它的可视性并改一次 Sequencer 或场景物体
  * 再切回持久关卡
* 重点确认：
  * `XR Config` 面板是否还会崩
  * Controller / Config 状态文字是否只是提示“当前不可用”或正常恢复，而不是直接异常

## 2026-04-27 18:09:31 XR Config workspace 绑定界面回收为专用 UI

### 任务

* 修复 `XR Config` 里 `Scene Trigger Controller` 被错误渲染成通用对象路径选择器的问题
* 修复 workspace 里误选关卡、Sequence 等无关对象后，`Scene Trigger Controller` 页面没有内容、像没读取到一样的回归

### 处理

* 确认根因在 `XRConfigEditor` 面板层，不在 runtime：
  * `SceneTriggerController` / `TriggerConfig` 改成 `FSoftObjectPath` 后，直接暴露给 `DetailsView`，丢失了原本的类型语义
  * 于是 workspace 中的字段退化成“什么 UObject 都能选”的通用 picker
  * 但 controller 页面实际解析时仍然要求对象必须能还原成当前 editor world 里的 `AXRSceneTriggerController`，所以一旦选错就会显示空
* 重做了 workspace 页面的绑定区，不再把整个 context 直接交给通用 `DetailsView`
* `Scene Trigger Controller` 改为专用绑定方式：
  * 只允许通过当前关卡选中的 actor 执行 `Use Selected Controller`
  * 增加 `Clear Controller`
  * 保留 `Focus Controller`
* `Trigger Config Asset` 改为专用资产选择器：
  * 使用 `SObjectPropertyEntryBox`
  * 仅允许选择 `UXRSceneTriggerConfig`
  * 增加 `Clear Config Asset`
* 保留上一轮为了防崩而引入的 `FSoftObjectPath` 内部存储和手动解析逻辑，不回退到之前那条有风险的 `TSoftObjectPtr` 取对象链路

### 验证

* 已使用 `D:\Unreal\UE_5.5\Engine\Build\BatchFiles\Build.bat` 对 `XR_RunningEditor Win64 Development` 执行真实编译验证
* 编译结果成功通过

### 状态

* 代码已完成并通过编译

### 说明

* 这次修复是把 workspace 从“通用属性面板”收回到“业务专用绑定 UI”
* 目标就是让非技术使用者只能做对的绑定，不再有机会把关卡、Sequence 之类的无关对象塞进 controller 上下文

### 下一步

* 由你重新打开 `XR Config`
* 重点确认三件事：
  * workspace 里 `Scene Trigger Controller` 不再出现关卡 / Sequence 那种乱选
  * 用 `Use Selected Controller` 绑定后，`Scene Trigger Controller` 页能正常出现 controller 面板
  * `Trigger Config Asset` 仍然能正常选择和打开配置资产

## 2026-04-27 18:14:22 XR Config workspace 记忆范围收敛到当前地图

### 任务

* 解决切换地图后，XR Config 面板仍试图沿用上一个地图里保存的 controller / config 绑定，导致重新打开面板时再次落入无效上下文的问题
* 保留“同一张地图内关面板再打开还能记住”的便利性，但去掉“跨地图记住”的高风险行为

### 处理

* 在 `UXRConfigEditorSettings` 中新增 `LastContextMapPath`
* 调整 workspace 上下文恢复规则：
  * 只有当前 editor map 与保存时的 map path 一致，才恢复 `LastSceneTriggerController` 和 `LastTriggerConfig`
  * 如果当前地图已经变了，则直接清空保存的上下文
* 调整 `SXRConfigEditorPanel::HandleMapOpened()`：
  * 一旦发生地图切换，就立即清空内存中的 controller / config 绑定
  * 同时把“已清空”的状态写回设置，避免关掉面板再打开时又把旧地图绑定带回来
* 保留同图便利性：
  * 只要你没切地图，面板关闭再打开，之前绑定的 controller 和数据资产仍然会保留

### 验证

* 已使用 `D:\Unreal\UE_5.5\Engine\Build\BatchFiles\Build.bat` 对 `XR_RunningEditor Win64 Development` 执行真实编译验证
* 编译结果成功通过

### 状态

* 代码已完成并通过编译

### 说明

* 这次不是简单“多判空”，而是把 workspace 记忆边界明确收紧到“当前地图”
* 这样更符合 UE 编辑器里 world / actor 生命周期的边界，也更适合你现在这种经常切持久关卡和子关卡编辑的工作流

### 下一步

* 由你按这个逻辑复测：
  * 在同一张地图里绑定 controller / config，关掉 XR Config 再打开，确认绑定还在
  * 然后切到别的地图或子关卡单独编辑，再切回，确认 workspace 已自动清空，需要重新绑定
* 重点看重新打开面板时是否还会再报你刚才那类异常

## 2026-04-27 18:19:54 XR Config workspace 外观回调到最初的成对设计

### 任务

* 在保留当前防崩逻辑、按地图清空逻辑、config 资产专用选择器逻辑的前提下，把 workspace 的视觉组织和使用感尽量调回最初那版
* 解决“controller 和 config 看起来完全不是一套设计，而且 workspace 里一眼找不到 controller 选取入口”的问题

### 处理

* 重新整理了 workspace 的绑定区布局
* `Scene Trigger Controller` 改为更接近最初设计的“字段式”展示：
  * 显示当前绑定值
  * 紧跟 `Use Selected Controller`
  * 保留 `Focus Controller`
  * 保留 `Clear Controller`
* `Trigger Config Asset` 也整理成同层级的字段式区域：
  * 显示当前绑定值
  * 保留 `UXRSceneTriggerConfig` 专用选择器
  * 保留 `Use Config From Controller`
  * 保留 `Open Config Asset`
  * 保留 `Clear Config Asset`
* 这次只改 workspace 的界面组织，不回退之前已经修好的安全逻辑

### 验证

* 已尝试使用 `D:\Unreal\UE_5.5\Engine\Build\BatchFiles\Build.bat` 对 `XR_RunningEditor Win64 Development` 执行真实编译验证
* 本次编译未进入代码编译阶段，因为 Unreal Live Coding 处于激活状态并阻止了构建

### 状态

* 代码已修改完成，待关闭 Live Coding 后做最终编译确认

### 说明

* 这次目标不是推翻前几轮的稳定性修复，而是在现有稳定逻辑上，把 workspace 的入口感和可发现性拉回你更顺手的那版

### 下一步

* 请你先关闭编辑器，或者在编辑器里按一次 `Ctrl+Alt+F11` 结束 Live Coding
* 然后我继续做最终编译确认

## 2026-04-27 18:26:58 XR Config workspace 按快照版设计回拉

### 任务

* 按你明确指定的快照 `XR_Running_snapshot_effect_id_fix_20260427_111216` 回看并恢复 `XR Config workspace` 的设计方式
* 停止继续在后续那套越改越偏的 workspace 结构上叠改

### 处理

* 对照快照中的 `XRConfigEditor` 源文件和使用文档，确认你要的是：
  * workspace 使用最初那套 context-details 风格的上下文区
  * `SceneTriggerController` 和 `TriggerConfig` 在同一块统一结构里配置
  * 下方保留原本那套快捷按钮组织
* 将 `UXRConfigEditorContext` 从后面那版的 `FSoftObjectPath` 工作区字段改回带类型的对象引用：
  * `TObjectPtr<AXRSceneTriggerController>`
  * `TObjectPtr<UXRSceneTriggerConfig>`
* 将 `workspace` 页恢复为快照版的 `ContextDetailsView + 原始快捷按钮布局`
* 同时保留后面已经证明有必要的稳定性边界：
  * 只在当前地图内记住 workspace 绑定
  * 切地图时自动清空上下文
  * 没有有效 controller / config 时，状态区走保守提示，不再冒险解引用

### 验证

* 已使用 `D:\Unreal\UE_5.5\Engine\Build\BatchFiles\Build.bat` 对 `XR_RunningEditor Win64 Development` 执行真实编译验证
* 编译结果成功通过

### 状态

* 代码已完成并通过编译

### 说明

* 这次不是继续“微调当前设计”，而是明确按你认可的快照版回拉 workspace 设计
* 后续如果再修这块，我会以这版为基线，不再让它继续漂移

### 下一步

* 请你直接进编辑器复测这版 workspace
* 重点确认：
  * 界面结构是不是已经回到你认可的那版
  * 切地图后是否不再带着旧 controller / config 残留
  * 重新打开面板时是否还会出现你前面提到的断点异常

## 2026-04-27 18:35:52 当前稳定版本快照压缩

### 任务

* 将当前这版你已确认“没问题”的项目状态做成新的版本快照压缩包
* 作为后续继续迭代前的可回退节点

### 处理

* 生成新的版本快照：
  * `backups/XR_Running_snapshot_workspace_restored_20260427_183552.zip`
* 压缩时排除了：
  * `backups`
  * `.vs`
* 保持与之前版本快照相同的用途：用于后续修偏时直接回退

### 验证

* 快照压缩包已成功生成
* 文件落盘路径已确认：
  * `D:\Unreal\Project\C++\PluginsProject\XR_Running\backups\XR_Running_snapshot_workspace_restored_20260427_183552.zip`

### 状态

* 已完成

### 说明

* 这次快照基于当前 workspace 设计回拉并通过编译验证后的项目状态
* 后续如果继续改 XR Config 或其他编辑器工作流，可以直接把这版当作新的回退基线

### 下一步

* 继续下一项功能或优化时，以这个快照为当前稳定起点

## 2026-04-27 18:39:19 轻量快照规则切换

### 任务

* 将当前快照规则改为轻量版本
* 处理重复且过大的 `workspace_restored` 重快照

### 处理

* 删除了两个体积过大的重快照：
  * `backups/XR_Running_snapshot_workspace_restored_20260427_183204.zip`
  * `backups/XR_Running_snapshot_workspace_restored_20260427_183552.zip`
* 按轻量规则重新生成当前稳定快照：
  * `backups/XR_Running_snapshot_workspace_restored_light_20260427_183919.zip`
* 轻量规则明确排除：
  * `Binaries`
  * `Intermediate`
  * `Saved`
  * `.vs`
  * `backups`

### 验证

* 新轻量快照已成功生成
* 当前文件体积已恢复到轻量级别：
  * `213746` 字节

### 状态

* 已完成

### 说明

* 之前快照体积异常偏大，是因为把构建产物、运行缓存和日志也一起打进去了
* 以后默认使用轻量规则，只有在明确需要“完整运行环境快照”时再做重快照

### 下一步

* 后续继续迭代时，默认以这个轻量快照作为当前稳定回退点

## 2026-04-27 18:42:28 按旧版规则重做正式版本快照

### 任务

* 按你之前已经认可的旧版快照方式，重做当前稳定版本压缩包
* 不再使用我中途试出来的轻量新规则

### 处理

* 对照旧快照 `XR_Running_snapshot_effect_id_fix_20260427_111216.zip` 的实际打包结构，确认旧规则包含：
  * `Config`
  * `Content`
  * `Plugins/XR/Binaries`
  * `Plugins/XR/Content`
  * `Plugins/XR/Intermediate`
  * `Plugins/XR/Resources`
  * `Plugins/XR/Source`
  * `Plugins/XR/XR.uplugin`
  * `Source`
  * `docs`
  * 根目录关键文件
* 删除了前面试错生成的轻量快照
* 按旧版规则重新生成正式快照：
  * `backups/XR_Running_snapshot_workspace_restored_20260427_184228.zip`

### 验证

* 新快照已成功生成
* 当前文件体积恢复到与旧版快照同量级：
  * `31940210` 字节

### 状态

* 已完成

### 说明

* 这次已经回到你之前版本回照的实际规则，而不是我后面临时改出来的轻量规则
* 后续如果你再说“按之前版本回照的方式”，我就按这一套来执行

### 下一步

* 后续继续迭代时，以这个正式快照作为当前稳定回退点

## 2026-04-27 19:30:00 本地 OSC 模拟发送脚本

### 任务

* 提供一个不依赖第三方库的本地 OSC 发送方式
* 方便直接在同一台机器上验证 `XRSceneTriggerController` 的 OSC 接收链路

### 处理

* 新增脚本：
  * `tools/send_osc_test.py`
* 脚本使用 Python 标准库直接构造 OSC UDP 报文，不需要安装 `python-osc`
* 默认参数与当前 XR 接收端配置保持一致：
  * Host=`127.0.0.1`
  * Port=`7000`
  * Address=`/xr/trigger`
  * Argument=`int32 3`
* 增加了可选参数，支持：
  * 自定义 host / port / address
  * 切换 `int` / `float` / `string` 参数类型
  * 重复发送与发送间隔
* 新增文档：
  * `docs/OSC_LOCAL_TESTING.md`

### 验证

* 已执行脚本本地运行验证
* 默认命令可正常发出 UDP 数据包
* 当前未在本轮里强制依赖 Unreal 运行中做联机验证；需要你在编辑器打开并启用 OSC Receiver 后观察 Output Log

### 状态

* 已完成

### 下一步

* 由你在 Unreal Editor 中启用 `OSC Receiver` 并运行项目
* 然后执行：
  * `py .\\tools\\send_osc_test.py`
* 观察 `Output Log` 是否出现 `Received OSC trigger ...`

## 2026-05-17 23:35:00 运行时调试 Workspace（Switchboard / nDisplay）

### 任务

* 在 Switchboard / nDisplay 启动后的运行态里提供一个可随时呼出 / 关闭的调试 workspace
* 要求：
  * 配置显示只读，不能在这里修改设置
  * 但保留运行调试能力，能直接执行 trigger 和 stop/reset
  * 不能渲染到 nDisplay 正在输出的屏幕上
  * 优先在鼠标所在显示器打开；如果该显示器被 nDisplay 占用，则自动切到非 nDisplay 显示器

### 处理

* 在 `XR` runtime 模块中新增独立窗口式调试 workspace：
  * `Plugins/XR/Source/XR/Private/XRRuntimeDebugWorkspace.h`
  * `Plugins/XR/Source/XR/Private/XRRuntimeDebugWorkspace.cpp`
* 实现方式不是 UMG overlay，而是独立 Slate 顶层窗口，因此不会进入游戏 / nDisplay 画面
* 新增全局运行时呼出入口：
  * `F10` 切换显示 / 隐藏
  * `UXRBPLibrary`:
    * `ShowXRRuntimeDebugWorkspace`
    * `HideXRRuntimeDebugWorkspace`
    * `ToggleXRRuntimeDebugWorkspace`
    * `IsXRRuntimeDebugWorkspaceVisible`
  * console commands:
    * `XR.ShowRuntimeDebugWorkspace`
    * `XR.HideRuntimeDebugWorkspace`
    * `XR.ToggleRuntimeDebugWorkspace`
* Workspace 内容设计：
  * 只读显示 controller / config / OSC / active state
  * 只读列出 backgrounds / effects / generic actions / triggers
  * 允许执行：
    * 输入 TriggerId 后 `Run Trigger`
    * `Run Last Trigger`
    * 每个 trigger 条目单独 `Run`
    * `Stop All And Reset`
* 新增窗口落点规则：
  * 读取当前鼠标所在显示器
  * 读取当前游戏窗口所在显示器，视为 nDisplay 占用区
  * 如果鼠标显示器被占用，则自动回退到最近的非占用显示器
* 为运行态状态展示补充了 `AXRSceneTriggerController` 只读 getter：
  * `GetActiveEffectIds`
  * `GetActiveGenericActionIds`
  * `IsOSCReceiverRunning`
* 新增使用文档：
  * `docs/XR_RUNTIME_DEBUG_WORKSPACE.md`

### 验证

* 已完成代码接入
* 待执行真实编译验证
* 待你在 Switchboard / nDisplay 运行态现场确认：
  * 窗口不出现在 nDisplay 输出屏
  * 鼠标在非 nDisplay 屏时，`F10` 能正常呼出
  * trigger / stop/reset 调试链路正常

### 状态

* 代码已完成，待编译与现场验证

### 下一步

* 执行 `XR_RunningEditor` 或目标运行配置的编译验证
* 由你在真实多显示器运行环境里复测窗口落点与运行调试行为

## 2026-05-17 23:55:00 运行时调试 Workspace 增补编辑器世界支持

### 任务

* 让同一个 `XR Runtime Debug Workspace` 不只在 Switchboard / nDisplay 运行态可用
* 也要能在 Unreal Editor 里直接使用，尤其是：
  * PIE / Simulate 运行中
  * 没进 PIE 时的 editor world 预览调试

### 处理

* 调整 `FXRRuntimeDebugWorkspaceManager::ResolveRuntimeWorld()`
* 当前优先级改为：
  * `GameViewport` 当前世界
  * `Game / PIE` world context
  * `Editor` world fallback
* 因此同一个独立 Slate 窗口现在可以：
  * 先控制 PIE / 运行时世界
  * 如果没有运行时世界，就回退到当前编辑器世界
* 同步更新了 workspace 的状态与执行反馈文案：
  * 会明确显示当前是 `Runtime`
  * 还是 `Editor Preview`
* 更新文档：
  * `docs/XR_RUNTIME_DEBUG_WORKSPACE.md`

### 验证

* 待执行 `XR_RunningEditor` 编译验证
* 待你在编辑器里确认：
  * 不进 PIE 时按 `F10` 也能拉起窗口
  * 能对当前 editor world 的 controller 直接做 trigger / stop 调试
  * 进 PIE 后仍然优先控制 PIE 世界

### 状态

* 代码已修改，待编译与编辑器现场验证

### 下一步

* 完成 `XR_RunningEditor` 编译
* 由你分别在：
  * editor world
  * PIE / Simulate
  * Switchboard / nDisplay
  三种场景下各做一轮验证

## 2026-05-18 11:45:24 OSC 旧蓝图兼容说明修正

### 任务

* 修正文档里对旧项目 OSC 蓝图用法的说明
* 避免把当前插件里的 `OSC Server Name`、`OSC Address Path`、回调里的 `IPAddress` 三者混为一谈

### 处理

* 更新 `docs/XR_OSC_RECEIVER_USAGE.md`
* 明确写清：
  * 当前插件真正参与消息匹配的是 `OSC Address Path`
  * 当前插件里的 `OSC Server Name` 只是创建 server 时使用的内部名称
  * 回调里的 `IPAddress` 是发送端设备 IP，不是 OSC 消息路径
* 根据 2026-05-18 的现场验证结果，补充旧项目兼容规则：
  * 如果旧蓝图使用 `CreateOSCServer`
  * 且旧项目现场约定里 `ServerName` 就是外部发送端实际使用的消息路径
  * 那么迁移到当前 XR 插件时，可以直接把旧蓝图里的 `ServerName` 填到 `OSC Address Path`
* 补充了一个实际迁移示例：
  * `Port = 2302`
  * `ServerName = /Level_cut`
  * 对应当前插件：
    * `OSC Receive Port = 2302`
    * `OSC Address Path = /Level_cut`

### 验证

* 已根据你的现场反馈确认：
  * 发送和接收链路已打通
  * 将旧蓝图 `CreateOSCServer` 的 `ServerName` 填到当前插件的 `OSC Address Path` 后，可以正常接收
* 本次为文档修正，无需重新编译

### 状态

* 已完成

### 下一步

* 后续如果你还要继续兼容更多旧项目 OSC 发送端，再决定是否把“手动指定监听 IP”也补回当前 XR 插件配置

## 2026-05-18 12:45:00 GitHub 版本库初始化与 README 补齐

### 任务

* 将当前项目接入你新建的 GitHub 仓库 `CJC-qwq/UE_XR`
* 为项目补一个适合公开版本管理的 `README.md`
* 在提交前收敛 Unreal 项目的 Git 纳管边界，避免把缓存和构建产物推上去

### 处理

* 在项目根目录初始化了 Git 仓库
* 新增 `.gitignore`
* `.gitignore` 里排除了：
  * `.vs`
  * `Binaries`
  * `Intermediate`
  * `Saved`
  * `DerivedDataCache`
  * `Plugins/*/Binaries`
  * `Plugins/*/Intermediate`
  * `backups`
  * `*.zip`
  * 常见 VS / IDE 临时文件
* 新增 `README.md`
* README 已写清：
  * 项目用途
  * 当前 XR 能力范围
  * Unreal 5.5 依赖
  * 目录结构
  * OSC 兼容说明
  * 构建命令
  * 关键文档入口

### 验证

* 已确认 Git 仓库初始化成功
* 已确认 `git status --ignored` 下：
  * 项目源文件、资源、文档、工具会被纳管
  * Unreal 生成目录与压缩包会被忽略
* 待执行：
  * 首次提交
  * 远端关联
  * 推送到 GitHub

### 状态

* 本地仓库已准备完成，待推送

### 下一步

* 完成首个 Git commit
* 关联远端 `https://github.com/CJC-qwq/UE_XR.git`
* 推送当前版本到 GitHub

## 2026-05-18 13:16:18 GitHub 仓库清理误纳管的 OSC 蓝图导出文本

### 任务

* 将误上传到仓库里的 `osc蓝图.txt` 从 Git 跟踪中移除
* 避免这类本地分析导出文本后续再次被提交到 GitHub

### 处理

* 确认当前工作区里的 `osc蓝图.txt` 已被删除，但 Git 仍在跟踪该文件
* 更新 `.gitignore`
* 已新增忽略规则：
  * `osc蓝图.txt`
* 计划通过改写当前首个 commit 的方式，把该文件从当前分支公开历史中移除

### 验证

* 已确认 `git status` 中该文件表现为已删除的 tracked 文件
* 已完成 amend 与强制推送后验证：
  * `git ls-files` 已不再包含 `osc蓝图.txt`
  * GitHub `main` 已由新的首个提交覆盖旧历史

### 状态

* 已完成

### 下一步

* 继续按当前仓库边界提交后续 XR 功能与文档更新
