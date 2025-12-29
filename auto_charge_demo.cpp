// ====================================================================
//          Created:    2025/12/25/ 15:33
//	         Author:	xuyanghe
//	        Company:
// ====================================================================

/**
 * @file auto_charge_demo.cpp
 * @brief 四足机器人自主充电Demo (Windows版)
 *
 * 编译: 使用 CMake 或 Visual Studio
 * 运行: auto_charge_demo.exe
 *
 * 流程:
 *   1. 启动2Hz心跳（每500ms发送一次）
 *   2. 发送启动自主充电任务命令
 *   3. 等待5秒（模拟充电任务执行中）
 *   4. 发送停止自主充电任务命令
 *   5. 等待1秒后退出
 *
 * 自主充电说明:
 *   - 机器人需要先处于导航模式
 *   - 启动自主充电后，机器人会自动导航到充电桩
 *   - 到达充电桩后自动对接并开始充电
 *   - 可以随时发送停止命令取消充电任务
 *
 * 参数说明:
 *   - 启动充电参数: 0 = 启动, 1 = 停止
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
constexpr uint32_t CMD_HEARTBEAT         = 0x21040001;
constexpr uint32_t CMD_AUTO_CHARGE_START = 0x91910250;  // 自主充电启动/停止

// ============ 充电任务参数 ============
constexpr int32_t CHARGE_START = 0;  // 启动充电任务
constexpr int32_t CHARGE_STOP  = 1;  // 停止充电任务

// ============ UDP命令结构 ============
#pragma pack(push, 1)
struct UDPCommand {
    uint32_t code;
    uint32_t parameters_size;  // 对于充电指令，这个字段存储参数
    uint32_t type;

    UDPCommand(uint32_t cmd, int32_t param = 0)
        : code(cmd)
        , parameters_size(static_cast<uint32_t>(param))
        , type(0) {}
};
#pragma pack(pop)

// ============ UDP发送函数 ============
void sendCommand(const char* ip, int port, uint32_t cmd_code, int32_t param = 0) {
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) return;

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

// ============ 心跳线程 ============
std::atomic<bool> heartbeat_running(true);

void heartbeatThread() {
    while (heartbeat_running) {
        sendCommand(ROBOT_IP, ROBOT_PORT, CMD_HEARTBEAT);
        Sleep(500);  // 500ms = 2Hz
    }
}

// ============ 自主充电函数 ============

void startAutoCharge() {
    std::cout << "[INFO] Starting auto charge task..." << std::endl;
    sendCommand(ROBOT_IP, ROBOT_PORT, CMD_AUTO_CHARGE_START, CHARGE_START);
}

void stopAutoCharge() {
    std::cout << "[INFO] Stopping auto charge task..." << std::endl;
    sendCommand(ROBOT_IP, ROBOT_PORT, CMD_AUTO_CHARGE_START, CHARGE_STOP);
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
    std::cout << "  Quadruped Robot Auto Charge Demo" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Target Robot: " << ROBOT_IP << ":" << ROBOT_PORT << std::endl;
    std::cout << std::endl;
    std::cout << "Note: Robot must be in Navigation mode to perform auto charge" << std::endl;
    std::cout << std::endl;

    // Start heartbeat thread
    std::thread hb_thread(heartbeatThread);
    std::cout << "[INFO] Heartbeat thread started (2Hz)" << std::endl;

    // Wait 1s to ensure heartbeat is running
    Sleep(1000);

    // Start auto charge
    startAutoCharge();
    std::cout << "[INFO] Charge task running, waiting 5 seconds..." << std::endl;
    Sleep(5000);

    // Stop auto charge
    /*
    stopAutoCharge();
    std::cout << "[INFO] Charge task stopped" << std::endl;
    Sleep(1000);
    */

    // Stop heartbeat thread
    heartbeat_running = false;
    hb_thread.join();

    // Cleanup Winsock
    WSACleanup();

    std::cout << "[INFO] Demo finished" << std::endl;
    return 0;
}

