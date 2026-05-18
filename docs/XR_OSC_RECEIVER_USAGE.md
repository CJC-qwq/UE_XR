# XR OSC 接收使用说明

## 1. 当前目标

这一版先完成 `OSC 接收`。

当前实现的作用是：

* Unreal 运行时启动 `OSC Server`
* 监听一个可配置端口
* 只响应一个可配置的固定 OSC 地址
* 从收到的消息里读取第一个整数
* 直接调用 `TriggerInteger`

也就是说，外部设备只要往指定地址发送一个整数，就会触发 XR 场景调度。

## 2. 当前配置入口

OSC 接收配置在 `AXRSceneTriggerController` 上。

在控制器细节面板里，主要看这几个字段：

* `Enable OSC Receiver`
  * 是否启用 OSC 接收
* `OSC Receive Port`
  * 监听端口
  * 例如 `7000`
* `OSC Server Name`
  * OSC Server 在 Unreal 内部使用的名称
  * 主要用于和旧项目配置习惯保持一致，以及调试识别
* `OSC Address Path`
  * 固定匹配的 OSC 地址
  * 例如 `/xr/trigger`
* `Log OSC Messages`
  * 是否在 Output Log 打印接收日志
* `Resolved Local IP Address`
  * 运行时自动解析出的当前本机 IP
  * 这是只读显示，用来给现场确认

## 3. 推荐配置

第一版建议直接这样配：

* `Enable OSC Receiver = true`
* `OSC Receive Port = 7000`
* `OSC Server Name = XRSceneTriggerOSCServer`
* `OSC Address Path = /xr/trigger`
* `Log OSC Messages = true`

## 3.1 和旧项目蓝图对齐时怎么填

如果你们旧项目也是用 Unreal 自带 OSC 插件，并且旧蓝图里在 `CreateOSCServer` 上填了一个 `ServerName`，那么现场兼容时要特别注意：

* 当前 XR 插件真正拿来做消息匹配的是 `OSC Address Path`
* 当前 XR 插件里的 `OSC Server Name` 只是创建 server 时使用的内部名字，不参与消息路径匹配

但根据本项目在 2026-05-18 的现场回归验证：

* 旧项目蓝图 `CreateOSCServer` 里的 `ServerName`
* 实际上可以直接对应到外部发送端发来的 OSC 地址路径

因此，如果你是从旧项目迁移配置，推荐先这样试：

* 旧蓝图里的 `Port` -> 填到当前插件的 `OSC Receive Port`
* 旧蓝图里的 `ServerName` -> 直接填到当前插件的 `OSC Address Path`

例如旧蓝图如果是：

* `Port = 2302`
* `ServerName = /Level_cut`

那么当前 XR 插件可优先这样配：

* `OSC Receive Port = 2302`
* `OSC Address Path = /Level_cut`

如果这样能收到并触发，就说明旧项目现场约定里，那个 `ServerName` 实际承担的就是“消息路径标识”的作用。

## 4. 消息格式

当前版本只认这种格式：

* OSC 地址：必须匹配你配置的 `OSC Address Path`
* 第 1 个参数：必须是 `int32`

补充说明：

* 这里的“OSC 地址”指的是消息自己的地址路径，例如 `/xr/trigger`、`/Level_cut`
* 不是回调里显示的发送端 `IPAddress`
* 也不是当前插件配置项里的 `OSC Server Name`

### 4.1 为什么 OSC Address Path 是 `/xr/trigger` 这种形式

因为 OSC 协议里的消息地址本来就是“路径”写法。

它和网页 URL、文件夹路径有点像，习惯上都写成：

* `/something/action`
* `/project/scene/trigger`
* `/xr/trigger`

含义可以理解成：

* `/xr`
  * XR 相关消息分类
* `/trigger`
  * 这个分类下的触发动作

所以 `/xr/trigger` 不是 Unreal 特殊规定，而是 OSC 地址本身的常见标准写法。

你完全可以改成你们现场更习惯的形式，例如：

* `/ue/trigger`
* `/main/scene`
* `/trigger/int`

只要发送端和 Unreal 这边配置保持完全一致就行。

例如：

* 地址：`/xr/trigger`
* 参数：`3`

那么 Unreal 会执行：

* `TriggerInteger(3)`

## 5. 本机 IP 的行为

考虑到现场本机地址可能变化，当前逻辑是：

* 运行时启动时读取一次当前本机 IP
* 保存到 `Resolved Local IP Address`
* `OSC Server` 按当前解析出来的地址和端口启动

如果解析失败，会回退到：

* `127.0.0.1`

## 6. 测试方法

建议按这个顺序测试：

1. 在控制器上启用 `OSC Receiver`
2. 设置端口和地址
3. 运行项目
4. 打开 `Output Log`
5. 用外部 OSC 工具发送消息

例如发送：

* 地址：`/xr/trigger`
* 参数：`3`

如果成功，你会看到类似日志：

* `OSC receiver listening on ...`
* `Received OSC trigger 3 from ...`

随后现有的 `TriggerInteger(3)` 逻辑会继续执行。

## 7. 常见错误

### 7.1 没有任何反应

先检查：

* `Enable OSC Receiver` 是否开启
* 项目是否在运行
* 端口是否和发送端一致
* `OSC Address Path` 是否完全一致
* 第一个参数是不是整数
* 如果你是照旧项目蓝图迁移，优先试一下“把旧蓝图 `CreateOSCServer` 的 `ServerName` 直接填到当前插件的 `OSC Address Path`”

### 7.2 收到了消息但没触发

检查：

* 发送的整数对应的 `TriggerId` 是否存在
* `TriggerConfig` 是否挂到了正确的数据资产

### 7.3 本机地址不对

检查：

* 当前机器网络环境是否切换
* 运行后显示的 `Resolved Local IP Address` 是否为你当前网卡地址

## 8. 当前版本边界

这版先只做接收，不做输出。

当前边界是：

* 只支持一个固定地址配置
* 只取第一个 `int32` 参数
* 只在运行时读取一次本机 IP
* 不处理更复杂的消息路由

## 9. 下一步可扩展

后续可以继续扩展：

* OSC 输出
* 多地址路由
* 多参数解析
* 图形化 OSC 配置界面
