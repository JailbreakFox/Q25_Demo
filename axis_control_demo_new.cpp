// ====================================================================
//          Created:    2025/12/25/ 15:33
//	         Author:	xuyanghe
//	        Company:
// ====================================================================

/**
 * @file axis_control_demo_new.cpp
 * @brief 四足机器人轴控制Demo - 使用0x21010140复杂指令 (Windows版)
 *
 * 编译: 使用 CMake 或 Visual Studio
 * 运行: axis_control_demo_new.exe
 *
 * 流程:
 *   1. 启动2Hz心跳线程（每500ms发送一次）
 *   2. 发送站立指令
 *   3. 等待10秒确保站立完成
 *   4. 前进1秒
 *   5. 后退1秒
 *   6. 左转2秒
 *   7. 右转2秒
 *   8. 停止并退出
 */

#include <cstring>
#include <cstdint>
#include <thread>
#include <atomic>
#include <iostream>

// Windows 特定头文件
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

// ============ 配置 ============
const char* ROBOT_IP = "192.168.3.20";
const int ROBOT_PORT = 43893;

// ============ 命令码 ============
constexpr uint32_t CMD_HEARTBEAT    = 0x21040001;
constexpr uint32_t CMD_STAND_UP     = 0x21010202;
constexpr uint32_t CMD_LIE_DOWN   = 0x21010222;
constexpr uint32_t CMD_AXIS_CONTROL = 0x21010140;  // 轴控制指令码（复杂指令）
constexpr uint32_t EXTENDED_CMD     = 1;           // 扩展指令类型

// ============ 轴值定义 ============
// 轴值区间: [-1000, 1000]，无死区
constexpr int32_t AXIS_FORWARD   = 500;   // 前进
constexpr int32_t AXIS_BACKWARD  = -500;  // 后退

constexpr int32_t AXIS_MOVE_LEFT  = -500;  // 左移
constexpr int32_t AXIS_MOVE_RIGHT = 500;   // 右移

constexpr int32_t AXIS_TURN_LEFT  = -500;  // 左转
constexpr int32_t AXIS_TURN_RIGHT = 500;   // 右转

constexpr int32_t AXIS_STOP = 0;  // 停止（所有轴）

// ============ 指令头结构体（复杂指令） ============
#pragma pack(push, 1)
struct CommandHead {
    uint32_t command_type;   // 1 = 扩展指令
    uint32_t command_code;   // 指令码
    uint32_t parameter_size; // 数据体字节数
};
#pragma pack(pop)

// ============ 轴指令数据体结构 ============
#pragma pack(push, 1)
struct AxisCommand {
    uint32_t left_x;   // 平移摇杆X轴值
    uint32_t left_y;   // 平移摇杆Y轴值
    uint32_t right_x;  // 转向摇杆X轴值
    uint32_t right_y;  // 转向摇杆Y轴值
};
#pragma pack(pop)

// ============ 完整轴控制消息结构 ============
#pragma pack(push, 1)
struct AxisControlMessage {
    CommandHead head;
    uint8_t      data[64];  // 存储 AxisCommand
};
#pragma pack(pop)

// ============ UDP命令结构（简单指令） ============
#pragma pack(push, 1)
struct UDPCommand {
    uint32_t code;
    uint32_t parameters_size;
    uint32_t type;

    UDPCommand(uint32_t cmd, int32_t param = 0)
        : code(cmd)
        , parameters_size(static_cast<uint32_t>(param))
        , type(0) {}
};
#pragma pack(pop)

// ============ UDP发送函数（简单指令） ============
void sendCommand(const char* ip, int port, uint32_t cmd_code, int32_t param = 0) {
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "[ERROR] Failed to create socket" << std::endl;
        return;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    UDPCommand cmd(cmd_code, param);
    sendto(sock, reinterpret_cast<const char*>(&cmd), sizeof(cmd), 0,
           (struct sockaddr*)&addr, sizeof(addr));

    closesocket(sock);
}

// ============ 站立函数 ============
void standUp(const char* ip, int port) {
    std::cout << "[INFO] Sending stand up command..." << std::endl;
    sendCommand(ip, port, CMD_STAND_UP);
}

// ============ UDP发送函数（复杂指令-轴控制） ============
void sendAxisControl(const char* ip, int port, const AxisCommand& axisCmd) {
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "[ERROR] Failed to create socket" << std::endl;
        return;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    // 构造复杂指令消息
    AxisControlMessage msg;
    msg.head.command_type = EXTENDED_CMD;           // 1 = 扩展指令
    msg.head.command_code = CMD_AXIS_CONTROL;       // 0x21010140
    msg.head.parameter_size = sizeof(AxisCommand);  // 16 字节

    // 将 AxisCommand 数据复制到 data 字段
    memcpy(msg.data, &axisCmd, sizeof(AxisCommand));

    // 发送大小 = sizeof(head) + parameter_size
    int sendLen = sendto(sock, reinterpret_cast<const char*>(&msg),
                         sizeof(CommandHead) + msg.head.parameter_size, 0,
                         (struct sockaddr*)&addr, sizeof(addr));

    if (sendLen < 0) {
        std::cerr << "[ERROR] Failed to send axis control command" << std::endl;
    } else {
        std::cout << "[INFO] Send axis control: left_x=" << axisCmd.left_x
                  << ", left_y=" << axisCmd.left_y
                  << ", right_x=" << axisCmd.right_x
                  << ", right_y=" << axisCmd.right_y << std::endl;
    }

    closesocket(sock);
}

// ============ 全局控制变量 ============
std::atomic<bool> heartbeat_running(true);

// ============ 心跳线程 ============
void heartbeatThread() {
    while (heartbeat_running) {
        sendCommand(ROBOT_IP, ROBOT_PORT, CMD_HEARTBEAT);
        Sleep(500);  // 500ms = 2Hz
    }
}

// ============ 运动控制函数 ============
// 50Hz = 20ms间隔

// 前进（左摇杆Y轴正向）
void moveForward(int duration_sec) {
    AxisCommand cmd;
    cmd.left_x = 0;
    cmd.left_y = AXIS_FORWARD;
    cmd.right_x = 0;
    cmd.right_y = 0;
    int total_ms = duration_sec * 1000;
    for (int i = 0; i < total_ms; i += 10) {
        sendAxisControl(ROBOT_IP, ROBOT_PORT, cmd);
        Sleep(10);  // 100Hz = 10ms
    }
    cmd.left_y = AXIS_STOP;
    sendAxisControl(ROBOT_IP, ROBOT_PORT, cmd);
}

// 后退（左摇杆Y轴负向）
void moveBackward(int duration_sec) {
    AxisCommand cmd;
    cmd.left_x = 0;
    cmd.left_y = AXIS_BACKWARD;
    cmd.right_x = 0;
    cmd.right_y = 0;
    int total_ms = duration_sec * 1000;
    for (int i = 0; i < total_ms; i += 10) {
        sendAxisControl(ROBOT_IP, ROBOT_PORT, cmd);
        Sleep(10);  // 100Hz = 10ms
    }
    cmd.left_y = AXIS_STOP;
    sendAxisControl(ROBOT_IP, ROBOT_PORT, cmd);
}

// 左转（右摇杆X轴负向）
void turnLeft(int duration_sec) {
    AxisCommand cmd;
    cmd.left_x = 0;
    cmd.left_y = 0;
    cmd.right_x = AXIS_TURN_LEFT;
    cmd.right_y = 0;
    int total_ms = duration_sec * 1000;
    for (int i = 0; i < total_ms; i += 10) {
        sendAxisControl(ROBOT_IP, ROBOT_PORT, cmd);
        Sleep(10);  // 100Hz = 10ms
    }
    cmd.right_x = AXIS_STOP;
    sendAxisControl(ROBOT_IP, ROBOT_PORT, cmd);
}

// 右转（右摇杆X轴正向）
void turnRight(int duration_sec) {
    AxisCommand cmd;
    cmd.left_x = 0;
    cmd.left_y = 0;
    cmd.right_x = AXIS_TURN_RIGHT;
    cmd.right_y = 0;
    int total_ms = duration_sec * 1000;
    for (int i = 0; i < total_ms; i += 10) {
        sendAxisControl(ROBOT_IP, ROBOT_PORT, cmd);
        Sleep(10);  // 100Hz = 10ms
    }
    cmd.right_x = AXIS_STOP;
    sendAxisControl(ROBOT_IP, ROBOT_PORT, cmd);
}

// 左移（左摇杆X轴负向）
void moveLeft(int duration_sec) {
    AxisCommand cmd;
    cmd.left_x = AXIS_MOVE_LEFT;
    cmd.left_y = 0;
    cmd.right_x = 0;
    cmd.right_y = 0;
    int total_ms = duration_sec * 1000;
    for (int i = 0; i < total_ms; i += 10) {
        sendAxisControl(ROBOT_IP, ROBOT_PORT, cmd);
        Sleep(10);  // 100Hz = 10ms
    }
    cmd.left_x = AXIS_STOP;
    sendAxisControl(ROBOT_IP, ROBOT_PORT, cmd);
}

// 右移（左摇杆X轴正向）
void moveRight(int duration_sec) {
    AxisCommand cmd;
    cmd.left_x = AXIS_MOVE_RIGHT;
    cmd.left_y = 0;
    cmd.right_x = 0;
    cmd.right_y = 0;
    int total_ms = duration_sec * 1000;
    for (int i = 0; i < total_ms; i += 10) {
        sendAxisControl(ROBOT_IP, ROBOT_PORT, cmd);
        Sleep(10);  // 100Hz = 10ms
    }
    cmd.left_x = AXIS_STOP;
    sendAxisControl(ROBOT_IP, ROBOT_PORT, cmd);
}

// ============ 主函数 ============
int main() {

    // 初始化 Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "[ERROR] Winsock initialization failed" << std::endl;
        return -1;
    }

    std::cout << "========================================" << std::endl;
    std::cout << "  Quadruped Robot Axis Control Demo" << std::endl;
    std::cout << "  Using 0x21010140 Extended Command" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Target Robot: " << ROBOT_IP << ":" << ROBOT_PORT << std::endl;
    std::cout << "Axis Control Code: 0x" << std::hex << CMD_AXIS_CONTROL << std::dec << std::endl;
    std::cout << std::endl;

    // 启动心跳线程
    std::thread hb_thread(heartbeatThread);
    std::cout << "[INFO] Heartbeat thread started (2Hz)" << std::endl;

    // 等待1s确保心跳已启动
    Sleep(1000);

    // 站立
    standUp(ROBOT_IP, ROBOT_PORT);
    std::cout << "[INFO] Waiting 10 seconds for stand up..." << std::endl;
    Sleep(10000);

    // 前进1秒
    std::cout << "[INFO] Moving forward 2s..." << std::endl;
    moveForward(2);
    Sleep(1000);

    // 后退1秒
    std::cout << "[INFO] Moving backward 2s..." << std::endl;
    moveBackward(2);
    Sleep(1000);

    // 左转2秒
    std::cout << "[INFO] Turning left 2s..." << std::endl;
    turnLeft(2);
    Sleep(1000);

    // 右转2秒
    std::cout << "[INFO] Turning right 2s..." << std::endl;
    turnRight(2);
    Sleep(1000);

    // 左移1秒
    std::cout << "[INFO] Moving left 2s..." << std::endl;
    moveLeft(2);
    Sleep(1000);

    // 右移1秒
    std::cout << "[INFO] Moving right 2s..." << std::endl;
    moveRight(2);
    Sleep(1000);

    // 趴下
    std::cout << "[INFO] Sending lie down command..." << std::endl;
    sendCommand(ROBOT_IP, ROBOT_PORT, CMD_LIE_DOWN);
    Sleep(1000);

    // 停止心跳线程
    heartbeat_running = false;
    hb_thread.join();

    // 清理 Winsock
    WSACleanup();

    std::cout << "[INFO] Demo finished" << std::endl;
    return 0;
}
