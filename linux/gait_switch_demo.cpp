// ====================================================================
//          Created:    2025/12/25/ 15:33
//	         Author:	xuyanghe
//	        Company:
// ====================================================================

/**
 * @file gait_switch_demo.cpp
 * @brief 四足机器人步态切换Demo
 *
 * 编译: g++ -std=c++11 -pthread gait_switch_demo.cpp -o gait_switch_demo
 * 运行: ./gait_switch_demo
 *
 * 流程:
 *   1. 启动2Hz心跳（每500ms发送一次）
 *   2. 发送站立命令
 *   3. 切换到行走步态(Walk)
 *   4. 等待3秒
 *   5. 切换到小跑步态(Trot/Run)
 *   6. 等待3秒
 *   7. 趴下并退出
 *
 * 步态说明:
 *   - Walk步态: 稳定的四足行走，适用于平坦地面
 *   - Trot/Run步态: 对角步态，速度更快
 */

#include <cstring>
#include <cstdint>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <thread>
#include <atomic>
#include <iostream>

// ============ 配置 ============
const char* ROBOT_IP = "192.168.3.20";
const int ROBOT_PORT = 43893;

// ============ 命令码 ============
constexpr uint32_t CMD_HEARTBEAT  = 0x21040001;
constexpr uint32_t CMD_STAND_UP   = 0x21010202;
constexpr uint32_t CMD_LIE_DOWN   = 0x21010222;
constexpr uint32_t CMD_WALK_STATE = 0x21010300;  // 行走步态(Walk)
constexpr uint32_t CMD_RUN_STATE  = 0x21010423;  // 小跑步态(Trot/Run)

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
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    UDPCommand cmd(cmd_code);
    sendto(sock, &cmd, sizeof(cmd), 0, (struct sockaddr*)&addr, sizeof(addr));

    close(sock);
}

// ============ 心跳线程 ============
std::atomic<bool> heartbeat_running(true);

void heartbeatThread() {
    while (heartbeat_running) {
        sendCommand(ROBOT_IP, ROBOT_PORT, CMD_HEARTBEAT);
        usleep(500000);  // 500ms = 2Hz
    }
}

// ============ 步态切换函数 ============

// 切换到Walk步态
void switchToWalkGait() {
    std::cout << "[INFO] 切换到Walk步态..." << std::endl;
    sendCommand(ROBOT_IP, ROBOT_PORT, CMD_WALK_STATE);
}

// 切换到Run/Trot步态
void switchToRunGait() {
    std::cout << "[INFO] 切换到Run/Trot步态..." << std::endl;
    sendCommand(ROBOT_IP, ROBOT_PORT, CMD_RUN_STATE);
}

// ============ 主函数 ============
int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  四足机器人步态切换Demo" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "目标机器人: " << ROBOT_IP << ":" << ROBOT_PORT << std::endl;
    std::cout << std::endl;

    // 启动心跳线程
    std::thread hb_thread(heartbeatThread);
    std::cout << "[INFO] 心跳线程已启动 (2Hz)" << std::endl;

    // 等待1s确保心跳已启动
    sleep(1);

    // 站立
    std::cout << "[INFO] 发送站立命令..." << std::endl;
    sendCommand(ROBOT_IP, ROBOT_PORT, CMD_STAND_UP);
    sleep(2);

    // 切换到Walk步态
    switchToWalkGait();
    std::cout << "[INFO] 等待3秒..." << std::endl;
    sleep(3);

    // 切换到Run步态
    switchToRunGait();
    std::cout << "[INFO] 等待3秒..." << std::endl;
    sleep(3);

    // 趴下
    std::cout << "[INFO] 发送趴下命令..." << std::endl;
    sendCommand(ROBOT_IP, ROBOT_PORT, CMD_LIE_DOWN);
    sleep(1);

    // 停止心跳线程
    heartbeat_running = false;
    hb_thread.join();

    std::cout << "[INFO] Demo结束" << std::endl;
    return 0;
}

