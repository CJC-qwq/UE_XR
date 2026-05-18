# XR Config 图形化面板使用说明

## 1. 目标

`XR Config` 面板是给项目内非技术同事和现场配置人员使用的图形化入口。

这一版不再把多个 Details 面板硬挤在同一块区域里，而是改成先按对象层级切换，再进入对应对象的专属配置界面。

当前顶层分为 3 个主入口：

* `XR Config Workspace`
* `Scene Trigger Controller`
* `Trigger Config Asset`

## 2. 打开方式

编译并加载插件后，可以通过以下方式打开：

* `Window -> XR Config`
* 编辑器顶部工具栏中的 `XR Config` 按钮

## 3. 顶层结构

### XR Config Workspace

这是工作入口页，负责建立当前编辑上下文。

这里主要做：

* 选择当前要操作的 `XRSceneTriggerController`
* 选择当前要操作的 `Trigger Config Asset`
* 查看当前配置摘要
* 使用快捷按钮在控制器、配置资产、关卡场景之间跳转

这一页不负责深度编辑，只负责“选中谁、关联谁、当前是什么状态”。

### Scene Trigger Controller

这是控制器专属页，只保留真正常用的控制器配置，不再显示默认 Actor 上那些无关项。

当前内部又分为：

* `General`
* `OSC`
* `Preview / Stop`

其中：

* `General` 只保留与当前控制器直接相关的基础字段
* `OSC` 只保留 OSC 接收需要的字段
* `Preview / Stop` 只保留预览触发和强制停止相关内容

### Trigger Config Asset

这是配置资产专属页，用来编辑真正的调度数据。

当前内部又分为：

* `Background Levels`
* `Effect Levels`
* `Generic Actions`
* `Trigger Actions`

这样就不用一次面对所有数组配置，操作会清楚很多。

## 4. Workspace 页说明

### 上下文字段

Workspace 页里只保留两个核心对象：

* `Scene Trigger Controller`
* `Trigger Config`

你先把这两个对象选定，后面的 Controller 页和 Config 页才会有内容。

### 快捷按钮说明

#### Use Selected Controller

把当前关卡中选中的 `XRSceneTriggerController` 读进面板。

适合场景：

* 你已经在 Outliner 或关卡里选中了控制器
* 想把它快速带入 `XR Config` 面板继续配置

#### Use Config From Controller

读取当前控制器上已经绑定的 `TriggerConfig`，同步到当前 Workspace。

适合场景：

* 控制器已经挂好了配置资产
* 想直接继续编辑那份配置

#### Open Config Asset

直接打开当前选中的 `Trigger Config Asset`。

#### Focus Controller

在关卡中选中并定位当前控制器。

## 5. Scene Trigger Controller 页说明

### General

当前用于放控制器最常用、最直接的字段。

现阶段保留的重点是：

* `TriggerConfig`
* 运行预览相关字段

这里不显示复制、联网、Actor 默认变换等无关信息。

### OSC

这里只显示 OSC 接收相关字段，例如：

* 是否启用 OSC
* 接收端口
* Server Name
* Address Path
* 是否打印日志
* 当前解析出的本机 IP

这样现场同事只看这一页就能配置 OSC，不会被别的 Actor 选项干扰。

### Preview / Stop

这一页用于调试和临时控制。

主要内容：

* `PreviewTriggerId`
* `Preview Current Trigger`
* `Stop All And Reset`

## 6. Trigger Config Asset 页说明

### Background Levels

专门编辑背景关卡列表。

### Effect Levels

专门编辑特效关卡列表。

### Generic Actions

专门编辑通用动作列表。

### Trigger Actions

专门编辑整数触发映射列表。

这一页的设计重点不是“改掉底层数据结构”，而是把同类配置拆开，让你每次只专注一类内容。

## 7. 摘要区会显示什么

Workspace 顶部摘要会显示：

* 当前控制器名称
* 当前配置资产名称
* 背景关卡数量
* 特效关卡数量
* Generic Action 数量
* Trigger 数量

如果控制器当前绑定的 `TriggerConfig` 和 Workspace 里手动选中的配置资产不是同一份，也会显示提醒。

## 8. 推荐使用流程

推荐按这个顺序使用：

1. 在关卡中选中 `XRSceneTriggerController`
2. 打开 `XR Config`
3. 在 `XR Config Workspace` 页点击 `Use Selected Controller`
4. 点击 `Use Config From Controller`
5. 切到 `Scene Trigger Controller` 页配置控制器参数
6. 切到 `Trigger Config Asset` 页配置背景、特效、动作和触发映射

## 9. 当前版本的边界

这一版已经完成了“对象级切页 + 精简字段 + 分块编辑”的第一轮重构，但它还不是完整的专用表单系统。

当前已经做到：

* 顶层按 `Workspace / Controller / Config Asset` 分离
* Controller 页隐藏无关 Actor 默认项
* Config 页按数据类型分块查看
* 避免所有内容挤在同一屏里

当前还没有做到：

* 完整表格式批量编辑
* 配置错误高亮
* 拖拽式触发关系编辑
* 一键校验重复 Id、空引用和不合法绑定

## 10. 后续可继续增强

后续可以继续往这几个方向加强：

* `Trigger Actions` 表格式编辑
* 背景 / 特效 / 动作引用的快速搜索和筛选
* 配置校验提示
* 更明确的非技术用户引导式界面
