// ====================================================================
//          Created:    2025/12/25/ 15:33
//	         Author:	xuyanghe
//	        Company:
// ====================================================================

/**
 * @file height_control_demo.cpp
 * @brief 四足机器人高度调节Demo (Windows版)
 *
 * 编译: 使用 CMake 或 Visual Studio
 * 运行: height_control_demo.exe
 *
 * 流程:
 *   1. 启动2Hz心跳（每500ms发送一次）
 *   2. 发送站立命令
 *   3. 设置匍匐
 *   4. 等待2秒
 *   5. 设置中高度
 *   6. 等待2秒
 *   7. 设置高高度
 *   8. 等待2秒
 *   9. 趴下并退出
 *
 * 高度说明:
 *   - 0: 匍匐 - 重心较低，更稳定
 *   - 1: 中高度 - 默认高度
 *   - 2: 高高度 - 视野更好，但稳定性略降
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
constexpr uint32_t CMD_LIE_DOWN      = 0x21010222;
constexpr uint32_t CMD_CHANGE_HEIGHT = 0x21010406;  // 高度调节

// ============ 高度值定义 ============
constexpr int32_t HEIGHT_LOW    = 0;  // 匍匐
constexpr int32_t HEIGHT_HIGH   = 2;  // 正常高度

// ============ UDP命令结构 ============
#pragma pack(push, 1)
struct UDPCommand {
    uint32_t code;
    uint32_t parameters_size;  // 对于高度指令，这个字段存储高度值
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

// ============ 高度调节函数 ============

void setHeightLow() {
    std::cout << "[INFO] Setting low height..." << std::endl;
    sendCommand(ROBOT_IP, ROBOT_PORT, CMD_CHANGE_HEIGHT, HEIGHT_LOW);
}

void setNormalHigh() {
    std::cout << "[INFO] Setting normal height..." << std::endl;
    sendCommand(ROBOT_IP, ROBOT_PORT, CMD_CHANGE_HEIGHT, HEIGHT_HIGH);
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
    std::cout << "  Quadruped Robot Height Control Demo" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Target Robot: " << ROBOT_IP << ":" << ROBOT_PORT << std::endl;
    std::cout << std::endl;

    // 启动心跳线程
    std::thread hb_thread(heartbeatThread);
    std::cout << "[INFO] Heartbeat thread started (2Hz)" << std::endl;

    // 等待1s确保心跳已启动
    Sleep(1000);

    // 站立
    std::cout << "[INFO] Sending stand up command..." << std::endl;
    sendCommand(ROBOT_IP, ROBOT_PORT, CMD_STAND_UP);
    Sleep(10000);

    // 设置匍匐
    setHeightLow();
    std::cout << "[INFO] Waiting 10 seconds..." << std::endl;
    Sleep(10000);

    // 设置正常高度
    setNormalHigh();
    std::cout << "[INFO] Waiting 10 seconds..." << std::endl;
    Sleep(10000);

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

