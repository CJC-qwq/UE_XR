# XR OSC 本地模拟发送

## 1. 目的

这个脚本用于在本机模拟“外部设备发送 OSC”，方便直接测试 `AXRSceneTriggerController` 的接收链路。

脚本路径：

* `tools/send_osc_test.py`

它只使用 Python 标准库，不依赖 `python-osc` 之类的第三方包。

## 2. 推荐前置设置

先确认控制器上这些字段已经配置好：

* `Enable OSC Receiver = true`
* `OSC Receive Port = 7000`
* `OSC Address Path = /xr/trigger`
* `Log OSC Messages = true`

## 3. 最小测试命令

在项目根目录打开 PowerShell 后执行：

```powershell
py .\tools\send_osc_test.py
```

如果你的系统里没有 `py` 或 `python` 命令，也可以直接使用 Unreal 自带的 Python：

```powershell
& 'D:\Unreal\UE_5.5\Engine\Binaries\ThirdParty\Python3\Win64\python.exe' .\tools\send_osc_test.py
```

默认行为等价于：

* Host: `127.0.0.1`
* Port: `7000`
* Address: `/xr/trigger`
* Argument Type: `int`
* Value: `3`

也就是会发送：

* `/xr/trigger`
* 第一个参数为 `int32 3`

这正对应当前 `XRSceneTriggerController` 的接收约定。

## 4. 常用示例

### 4.1 发送指定 TriggerId

```powershell
py .\tools\send_osc_test.py 5
```

表示发送：

* 地址 `/xr/trigger`
* 参数 `int32 5`

### 4.2 改端口

```powershell
py .\tools\send_osc_test.py 5 --port 8000
```

### 4.3 改 OSC 地址

```powershell
py .\tools\send_osc_test.py 5 --address /my/custom/path
```

### 4.4 连续发送

```powershell
py .\tools\send_osc_test.py 5 --repeat 10 --interval 0.5
```

表示每隔 0.5 秒发送一次，共 10 次。

## 5. 负向测试

如果你想验证“接收端是否正确拒绝错误类型”，可以故意发错类型：

```powershell
py .\tools\send_osc_test.py 5 --type float
py .\tools\send_osc_test.py hello --type string
```

按当前实现，这两种都不应该通过 `GetInt32(Message, 0, TriggerId)`。

## 6. 成功时建议观察

在 Unreal Editor 的 `Output Log` 里看：

* `OSC receiver listening on ...`
* `Received OSC trigger ...`

如果没反应，先检查：

* 是否真的启用了 `Enable OSC Receiver`
* 端口是否一致
* 地址路径是否完全一致
* 第一个参数是不是 `int32`
