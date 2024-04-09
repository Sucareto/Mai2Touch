# Mai2LED 数据分析
分析代码来源：Assembly-CSharp.dll  
LED 处理函数：Comio.BD15070_4.BoardCtrl15070_4._execThread()  
打印 LED 控制数据：Comio.Packet.dump()  
串口数据包编解码 Comio.JvsSyncProtect  
启用 DummyLED 的情况下，Comio.BoardCtrlBase.SendForceCommand 仍会发送灯光数据

以下数据包全部都是 16 进制数值，数据包以 `E0` 开始，`D0` 是转义符，最后一位是校验值  
如果原始数据中包含 `E0` 或 `D0`，则需要发送转义符，将原始数据 -1 再发送  

下面是数据包示例，`>>>` 是主机发送的内容，`<<<` 是设备回复的内容  
```
>>> e0 11 01 01 10 23
<<< e0 01 11 03 01 10 01 27
```
包含转义的数据包示例：  
```
>>> e0 11 01 04 39 d0 cf d0 cf ff ee
<<< e0 01 11 03 01 39 01 50
```
原始数据是：`e0 11 01 04 39 d0 d0 ff ee`

### 主机发送数据 req 格式
`sync dstNodeID srcNodeID length command payload sum`  
- `sync`：包头，固定为 E0
- `dstNodeID`：未知
- `srcNodeID`：未知
- `length`：从 `command` 开始到 `sum` 前的长度，不计算转义符
- `command`：命令值，根据这个判断需要执行的操作，可以参考 command 定义表
- `payload`：各个命令的 payload
- `sum`：转义前的数据校验值，从 `dstNodeID` 开始到包体末尾每个值相加，值最大 255，溢出部分不算

### 设备回复数据 ack 格式
`sync dstNodeID srcNodeID length status command report payload sum`
- `sync`：包头，固定为 E0
- `dstNodeID`：对应 req 的 srcNodeID
- `srcNodeID`：对应 req 的 dstNodeID
- `length`：从 `command` 开始到 `sum` 前的长度，不计算转义符
- `status`：命令解析状态，可能的值可以参考 status 定义表
- `command`：命令值，对应 req 的 command
- `report`：命令执行状态，可能的值可以参考 report 定义表
- `payload`：各个命令的 payload
- `sum`：转义前的数据校验值，从 `dstNodeID` 开始到包体末尾每个值相加，值最大 255，溢出部分不算

### 设备默认回复格式
如果解析和执行无异常，且不需要回报数据，默认使用这个格式回复，command 需要和对应的 req 一致  
下方命令解析如无特别说明，则默认使用这个格式
```
<<< sync dstNodeID srcNodeID length status command report sum
<<< e0 01 11 03 01 31 01 48
```

## 命令的具体解释

### setLedGs8Bit
设置单个灯颜色，需要收到 `SetLedGsUpdate` 才生效
```
>>> sync dstNodeID srcNodeID length command index r g b sum
>>> e0 11 01 05 31 03 ff ff ff 48
```

- `index`：灯序号
- `r g b`：3 个值，代表 RGB 的亮度，0-255

### setLedGs8BitMulti
设置多个灯颜色，需要收到 `SetLedGsUpdate` 才生效
```
>>> sync dstNodeID srcNodeID length command start end skip r g b speed sum
>>> e0 11 01 08 32 00 08 00 fa 96 00 00 e4
```
- `start`：灯起始序号
- `end`：灯的个数，如果值是 32，则代表全部灯
- `skip`：未知，未使用
- `r g b`：3 个值，代表 RGB 的亮度，0-255
- `speed`：未知，未使用

### setLedGs8BitMultiFade
设置多个灯颜色渐变，需要收到 `SetLedGsUpdate` 才开始执行，渐变的起始色是前一个 `setLedGs8BitMulti` 设置的
```
setLedGs8BitMulti
>>> sync dstNodeID srcNodeID length command start end skip r g b speed sum
>>> e0 11 01 08 32 00 08 00 fa 96 00 00 e4
<<< e0 01 11 03 01 32 01 49
setLedGs8BitMultiFade
>>> sync dstNodeID srcNodeID length command start end skip r g b speed sum
>>> e0 11 01 08 33 00 08 00 00 00 00 55 aa
<<<  e0 01 11 03 01 33 01 4a
SetLedGsUpdate
>>> sync dstNodeID srcNodeID length command sum
>>> e0 11 01 01 3c 4f
<<< e0 01 11 03 01 3c 01 53
```
- `start`：灯起始序号
- `end`：灯的个数
- `skip`：未知，未使用
- `r g b`：3 个值，代表 RGB 的亮度，0-255
- `speed`：渐变耗时，通过 (4095 / speed * 8) 算出毫秒
上面的例子，在 收到 `SetLedGsUpdate` 后的 885 ms 内，将 00 到 07 按键从 #960000 渐变到 #000000

### setLedFet
设置框体灯光，会连续多次发送实现闪烁，需要立刻生效
```
>>> sync dstNodeID srcNodeID length command BodyLed ExtLed SideLed sum
>>> e0 11 01 04 39 00 00 ff 4e
<<< e0 01 11 03 01 39 01 50
```
- `BodyLed ExtLed SideLed`：框体 3 个白灯的亮度值，0-255

### SetLedGsUpdate
刷新灯效
```
>>> sync dstNodeID srcNodeID length command sum
>>> e0 11 01 01 3c 4f
```

### SetEEPRom
设置 EEPRom 的值，只有 GetEEPRom 执行错误才会触发
```
>>> sync dstNodeID srcNodeID length command Set_adress writeData sum
```
- `Set_adress`：需要写入 EEPRom 的地址，0-7
- `writeData`：写入 EEPRom 的值

### GetEEPRom
读取 EEPRom 的值
```
>>> sync dstNodeID srcNodeID length command Get_adress sum
>>> e0 11 01 02 7c 00 90
<<< sync dstNodeID srcNodeID length status command report eepData sum
<<< e0 01 11 04 01 7c 01 00 94
```
- `Get_adress`：需要读取 EEPRom 的地址，0-7
- `eepData`：EEPRom 对应地址的值，主机程序内暂未发现对值的判断逻辑，回复 0 即可

### SetEnableResponse SetDisableResponse
无需回复

### getBoardInfo
获取设备固件号
```
>>> e0 11 01 01 f0 03
<<< sync dstNodeID srcNodeID length status command report boardNo[?] 0xFF firmRevision sum
<<< e0 01 11 0d 01 f0 01 31 35 30 37 30 2d 30 34 ff 90 2e
```
- `boardNo`：固定值 `15070-04`，需要接一个 0xFF 表示 boardNo 结束，后一位是 firmRevision
- `firmRevision`：固定值 144，不同的值会触发固件升级逻辑

### getBoardStatus
获取设备状态
```
>>> e0 11 01 01 f1 04
<<< sync dstNodeID srcNodeID length status command report timeoutStat timeoutSec pwmIo fetTimeout sum
<<< e0 01 11 07 01 f1 01 00 00 00 00 0c
```
- `timeoutStat timeoutSec pwmIo fetTimeout`：未知，主机程序内暂未发现对值的判断逻辑，回复 0 即可

### getFirmSum
应该是固件升级相关，正常运行过程中暂未发现该命令执行
```
<<< sync dstNodeID srcNodeID length status command report sum_upper sum_lower sum
```

### getProtocolVersion
未知
```
>>> e0 11 01 01 f3 06
<<< sync dstNodeID srcNodeID length status command report appliMode major minor sum
<<< e0 01 11 06 01 f3 01 01 00 00 0e
```
- `appliMode`：bool 值，如果是 0，会触发固件升级逻辑
- `major minor`：未知，主机程序内暂未发现对值的判断逻辑，回复 0 即可
