/**
 * @file emergency_stop_demo.cpp
 * @brief 四足机器人急停控制Demo (Windows版)
 *
 * 编译: 使用 CMake 或 Visual Studio
 * 运行: emergency_stop_demo.exe
 *
 * 流程:
 *   1. 启动2Hz心跳（每500ms发送一次）
 *   2. 发送站立命令
 *   3. 等待2秒
 *   4. 发送急停命令（机器人会立即停止所有运动并趴下）
 *   5. 等待1秒后退出
 *
 * 急停说明:
 *   - 急停命令会使机器人立即停止所有运动
 *   - 机器人会进入安全趴下状态
 *   - 适用于紧急情况下快速停止机器人
 *
 * 注意:
 *   - 急停后需要重新发送站立命令才能恢复运动
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
constexpr uint32_t CMD_HEARTBEAT     = 0x21040001;
constexpr uint32_t CMD_STAND_UP      = 0x21010202;
constexpr uint32_t CMD_EMERGENCY_STOP = 0x21010C0E;  // 急停命令

// ============ UDP命令结构 ============
#pragma pack(push, 1)
struct UDPCommand {
    uint32_t code;
    uint32_t parameters_size;
    uint32_t type;

    UDPCommand(uint32_t cmd) : code(cmd), parameters_size(0), type(0) {}
};
#pragma pack(pop)

// ============ UDP发送函数 ============
void sendCommand(const char* ip, int port, uint32_t cmd_code) {
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) return;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    UDPCommand cmd(cmd_code);
    sendto(sock, reinterpret_cast<const char*>(&cmd), sizeof(cmd), 0, 
           (struct sockaddr*)&addr, sizeof(addr));

    closesocket(sock);
}

// ============ 心跳线程 ============
std::atomic<bool> heartbeat_running(true);

void heartbeatThread() {
    while (heartbeat_running) {
        sendCommand(ROBOT_IP, ROBOT_PORT, CMD_HEARTBEAT);
        Sleep(500);  // 500ms = 2Hz
    }
}

// ============ 急停函数 ============
void emergencyStop() {
    std::cout << "[WARNING] 发送急停命令!" << std::endl;
    sendCommand(ROBOT_IP, ROBOT_PORT, CMD_EMERGENCY_STOP);
}

// ============ 主函数 ============
int main() {
    // 初始化 Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "[ERROR] Winsock 初始化失败" << std::endl;
        return -1;
    }

    std::cout << "========================================" << std::endl;
    std::cout << "  四足机器人急停控制Demo (Windows)" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "目标机器人: " << ROBOT_IP << ":" << ROBOT_PORT << std::endl;
    std::cout << std::endl;

    // 启动心跳线程
    std::thread hb_thread(heartbeatThread);
    std::cout << "[INFO] 心跳线程已启动 (2Hz)" << std::endl;

    // 等待1s确保心跳已启动
    Sleep(1000);

    // 站立
    std::cout << "[INFO] 发送站立命令..." << std::endl;
    sendCommand(ROBOT_IP, ROBOT_PORT, CMD_STAND_UP);
    std::cout << "[INFO] 等待5秒..." << std::endl;
    Sleep(5000);

    // 急停
    emergencyStop();
    std::cout << "[INFO] 机器人已急停" << std::endl;
    Sleep(1000);

    // 停止心跳线程
    heartbeat_running = false;
    hb_thread.join();

    // 清理 Winsock
    WSACleanup();

    std::cout << "[INFO] Demo结束" << std::endl;
    return 0;
}

