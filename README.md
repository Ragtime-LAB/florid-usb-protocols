# florid-usb-protocols

Ragtime 机械臂 USB 传输协议层 — C++20 头文件库。

提供运行在主机（SDK/core）与 STM32H7 固件之间、基于 USB CDC / HySerial 的协议栈。

## 头文件

| 文件 | 用途 |
|---|---|
| `ProtocolStack.hpp` | 顶层协议栈 — 串联 RPL 解析器、反序列化器、可靠会话、分发器 |
| `ReliableSession.hpp` | ACK/重传会话（窗口=1，500ms 超时，最多 3 次重试） |
| `ProtocolDispatch.hpp` | 线程本地分发桥接（ISR → 主线程） |
| `ISRRingBuffer.hpp` | 无锁 SPSC 环形缓冲区（ISR 安全） |
| `IdRingBuffer.hpp` | 定长 ID 去重，用于响应过滤 |
| `ArmCommand.hpp` | 所有请求/响应包结构体（关节指令、电机控制、归零等） |
| `ArmStatus.hpp` | 遥测包结构体 — `ArmStatus`、`MotorFeedbackArray`、`GripperStatus` |

## 包区域

所有包基于 [RPL](https://github.com/C-One-Studio/RPL) 序列化，魔数字节 `0xA5`。

| 区域 | 类型 | 可靠 | 示例 |
|---|---|---|---|
| `0x6000+` | 遥测（固件 → 主机） | 否 | `ArmStatus (0x6001)`, `MotorFeedbackArray (0x6002)`, `GripperStatus (0x6003)` |
| `0x6100+` | 请求/响应 | 是（ACK + 重试） | `SetMotorControlMode (0x6103)`, `HomeAll (0x6109)`, `ClearFaults (0x610D)` |
| `0x6300+` | 即发即忘（主机 → 固件） | 否 | `JointCommandPacket (0x6301)`, `EmergencyStopPacket (0x6302)` |

## 依赖

- [rpl](https://github.com/C-One-Studio/RPL) — 内置于 `3rdparty/rpl`（纯头文件，`rpl::rpl` 目标）
- [unordered_dense](https://github.com/martinus/unordered_dense) — 若未找到则通过 CMake `FetchContent` 获取

## 集成

```cmake
add_subdirectory(path/to/florid-usb-protocols)

target_link_libraries(my_target PUBLIC usb_protocol)
```

`usb_protocol` INTERFACE 目标提供 include 路径及其所有传递依赖（`rpl::rpl`, `unordered_dense::unordered_dense`）。

### 与 florid-usb-sdk 配合

```cmake
add_subdirectory(protocols)
add_subdirectory(src)  # 定义 florid_usb 目标，链接 usb_protocol
```

## 使用示例

```cpp
#include <ProtocolStack.hpp>
#include <ArmCommand.hpp>
#include <ArmStatus.hpp>

using Stack = ProtocolStack<std::chrono::steady_clock,
    JointCommandPacket, EmergencyStopPacket, ArmStatus,
    HomeAllRequestPacket, HomeAllResponsePacket,
    ClearFaultsRequestPacket, ClearFaultsResponsePacket,
    SetMotorControlModeRequestPacket, SetMotorControlModeResponsePacket,
    UsbSessionStartRequestPacket, UsbSessionStartResponsePacket,
    UsbSessionStopRequestPacket, UsbSessionStopResponsePacket,
    GetMotorFeedbackRequestPacket, GetMotorFeedbackResponsePacket,
    MotorFeedbackArray, GripperStatus>;

Stack::Config cfg{};
Stack stack(cfg);
stack.set_send_frame([](const uint8_t* frame, size_t len) -> bool {
    return serial_send(frame, len);
});

// 注册遥测处理器
stack.register_handler(RPL::Meta::PacketTraits<ArmStatus>::cmd,
    [](uint16_t, TransactionId, const uint8_t* data, size_t size) {
        ArmStatus s{};
        std::memcpy(&s, data, sizeof(s));
        // 使用 s.mode, s.status.q, s.gripper, ...
    });

// 可靠请求
stack.send<UsbSessionStartRequestPacket, UsbSessionStartResponsePacket>(
    UsbSessionStartCommand{.dummy = 0},
    [](const uint8_t*, size_t) { /* 响应成功 */ },
    [](SessionError e) { /* 错误处理 */ },
    std::chrono::milliseconds(500), 3);

// 即发即忘
RPL::Serializer<JointCommandPacket> ser;
uint8_t buf[512];
auto result = ser.serialize(buf, sizeof(buf), joint_cmd);
serial_send(buf, *result);
```

## 构建

```bash
cmake -B build
cmake --build build
```
