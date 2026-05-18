# XR Generic Action 使用说明

## 1. 功能目的

`Generic Action` 用来触发放在 `MainMap` 持久关卡中的通用蓝图逻辑。

它适合这两类目标：

* 持久关卡里的 Actor 蓝图
* `MainMap` 的 Level Blueprint

它不依赖关卡序列自动触发，也不要求在插件里写死特效控制逻辑。你可以继续把实际效果写在：

* Actor 自己的蓝图里
* `MainMap` 的 Level Blueprint 里
* Sequencer 事件里转调蓝图逻辑

插件只负责根据整数触发把 `ActionName` 分发给正确的蓝图目标。

## 2. 当前实现规则

### 2.1 统一入口

所有 Generic Action 都通过同一个 Blueprint Interface 接口接收：

* 接口：`XRGenericActionReceiver`
* 函数：`HandleXRGenericAction(ActionName)`

`ActionName` 是当前版本唯一传入参数。

### 2.2 支持的目标类型

数据资产里的 `GenericActions` 目前支持两种 `TargetType`：

* `Actor`
* `Persistent Level Blueprint`

### 2.3 TriggerId 的触发关系

一个 `TriggerId` 可以同时触发：

* 一个背景关卡
* 多个特效关卡
* 多个 Generic Action

也就是说，一个整数可以同时触发多个通用蓝图事件。

## 3. 数据资产怎么配

在 `XRSceneTriggerConfig` 数据资产里，当前和 Generic Action 相关的部分有两块：

### 3.1 GenericActions

这是通用动作定义表，每一条定义一个“可被触发的动作”。

字段说明：

* `Generic Action Id`
  * 这条动作的唯一名字
  * 给 `TriggerActions` 引用
* `TargetType`
  * 目标类型
  * `Actor` 表示调用持久关卡里的 Actor 蓝图
  * `Persistent Level Blueprint` 表示调用 `MainMap` 的 Level Blueprint
* `Target Actor Tag`
  * 仅在 `TargetType = Actor` 时填写
  * 控制器会在持久关卡里查找带这个 Tag 的 Actor
* `Trigger Action Name`
  * 真正传给蓝图的 `ActionName`
* `Stop Action Name`
  * 在背景切换或 `StopAllAndReset()` 时，如果你希望该动作能被“停止”，就在这里填写一个停止动作名
  * 不需要停止逻辑就留空
* `AllowedBackgroundIds`
  * 可选
  * 留空表示所有背景都允许触发
  * 填了以后，只有当前背景在列表内时，这条 Generic Action 才会执行

### 3.2 TriggerActions

这里还是整数触发映射表。

和 Generic Action 相关的字段是：

* `Generic Actions To Trigger`

这里填的不是蓝图事件名，而是 `GenericActions` 里面定义好的 `Generic Action Id`。

也就是说关系是：

`TriggerId -> Generic Action Id -> ActionName -> 蓝图接口`

## 4. Actor 蓝图接法

如果你要让持久关卡里的某个 Actor 响应 Generic Action，按下面做：

1. 打开这个 Actor 蓝图
2. 在 Class Settings 里添加接口 `XRGenericActionReceiver`
3. 实现函数 `HandleXRGenericAction`
4. 在函数里根据 `ActionName` 分支处理

推荐写法：

* `Switch on Name`
* 或多个 `Branch` 判断 `ActionName`

例如：

* `ActionName = FX_Start`
  * 播放 Niagara
  * 打开材质参数动画
  * 播放 Timeline
* `ActionName = FX_Stop`
  * 停止 Niagara
  * 关闭灯光
  * 停止 Timeline

### 4.1 Actor 数据资产示例

假设场景里有一个 Actor：

* Actor Tag：`SmokeWall`

那么 `GenericActions` 可以这样配一条：

* `Generic Action Id = SmokeWall_Start`
* `TargetType = Actor`
* `Target Actor Tag = SmokeWall`
* `Trigger Action Name = FX_Start`
* `Stop Action Name = FX_Stop`

然后在 `TriggerActions` 里：

* `Generic Actions To Trigger` 添加 `SmokeWall_Start`

## 5. MainMap Level Blueprint 接法

如果你要让 `MainMap` 的 Level Blueprint 响应 Generic Action，按下面做：

1. 打开 `MainMap`
2. 打开 `MainMap` 的 Level Blueprint
3. 给 Level Blueprint 添加接口 `XRGenericActionReceiver`
4. 实现函数 `HandleXRGenericAction`
5. 在函数里根据 `ActionName` 分支执行总控逻辑

适合放在这里的逻辑一般是：

* MainMap 全局通用特效
* 多个持久 Actor 的统一调度
* 播放公共控制逻辑
* Sequencer 相关的前后置触发

### 5.1 Level Blueprint 数据资产示例

如果你希望触发 `MainMap` 的全局闪光逻辑：

`GenericActions` 可配置为：

* `Generic Action Id = Main_Flash_Start`
* `TargetType = Persistent Level Blueprint`
* `Trigger Action Name = Flash_Start`
* `Stop Action Name = Flash_Stop`

然后在 `MainMap` Level Blueprint 的 `HandleXRGenericAction` 里：

* `ActionName = Flash_Start` 时执行开始逻辑
* `ActionName = Flash_Stop` 时执行停止逻辑

## 6. StopActionName 什么时候会被调用

`Stop Action Name` 不是触发时自动执行的，它主要在这两种情况使用：

* 调用 `StopAllAndReset()`
* 切换背景关卡时，系统先停止当前通用动作，再加载新背景

如果某个通用动作只有“开始”，没有“停止”，就把 `Stop Action Name` 留空。

## 7. AllowedBackgroundIds 怎么理解

它是 Generic Action 的背景限制条件。

例如：

* 某个公共烟雾只能在 `BG_NightCity` 背景下播放

那就在这条 `GenericAction` 的 `AllowedBackgroundIds` 里填：

* `BG_NightCity`

这样如果当前背景不是它，触发器即使发到了这条动作，也不会执行。

## 8. 当前版本的使用建议

### 8.1 推荐这样用

* Actor 自己控制自己的局部效果
* `MainMap` Level Blueprint 负责总控型逻辑
* `ActionName` 尽量使用统一命名

例如：

* `FX_Start`
* `FX_Stop`
* `Flash_Start`
* `Flash_Stop`
* `Ripple_Start`
* `Ripple_Stop`

### 8.2 不推荐这样用

* 在数据资产里直接写很多零散、临时、无规则的动作名
* 一个 Tag 对应很多完全不同职责的 Actor
* 把所有逻辑都堆进 Level Blueprint

## 9. 当前版本的限制

第一版最小闭环目前有这些边界：

* 只传一个 `ActionName`
* Actor 目标通过 `Tag` 查找，不是直接软引用 Actor
* Level Blueprint 只支持持久关卡，也就是 `MainMap`
* `TriggerActions` 里字段名还是 `Generic Effects To Trigger`
  * 这是为了不扩大当前数据资产改动面
  * 实际语义现在已经是 “Generic Action Id 列表”

## 10. 推荐配置流程

实际使用时建议按这个顺序操作：

1. 先在蓝图里把 `HandleXRGenericAction` 写好
2. 确定好 `ActionName`
3. 再去数据资产里新增 `GenericActions`
4. 最后把 `ActionId` 填进对应的 `TriggerId`
5. 用控制器 `PreviewTrigger` 先验证

## 11. 出问题时先检查什么

如果触发没反应，优先检查：

1. `TriggerId` 是否映射到了正确的 `Generic Action Id`
2. `Generic Action Id` 是否存在于 `GenericActions`
3. `ActionName` 是否和蓝图里判断的名字一致
4. Actor 模式下，目标 Actor 是否真的在 `MainMap` 持久关卡里
5. Actor 模式下，目标 Actor 是否带了正确的 Tag
6. 目标蓝图或 Level Blueprint 是否实现了 `XRGenericActionReceiver`
7. `AllowedBackgroundIds` 是否把当前背景挡掉了

## 12. 后续可扩展方向

如果这版跑稳，后面可以继续扩展：

* 增加 `Int` / `Float` / `Name` 参数
* Actor 目标支持直接引用而不只靠 Tag
* 给 `TriggerActions` 里的 generic 字段正式改名
* 增加编辑器校验提示和配置检查
