/**
 * @file motion_mode_demo.cpp
 * @brief 四足机器人运动模式切换Demo
 *
 * 编译: g++ -std=c++11 -pthread motion_mode_demo.cpp -o motion_mode_demo
 * 运行: ./motion_mode_demo
 *
 * 流程:
 *   1. 启动2Hz心跳（每500ms发送一次）
 *   2. 切换到遥控模式
 *   3. 等待3秒
 *   4. 切换到导航模式
 *   5. 等待3秒
 *   6. 切换到辅助模式
 *   7. 等待1秒后退出
 *
 * 运动模式说明:
 *   - 遥控模式(MANUAL): 机器人响应手动控制指令
 *   - 导航模式(NAVI): 机器人执行自主导航任务
 *   - 辅助模式(ASSISTANT): 机器人进入辅助控制模式
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
constexpr uint32_t CMD_HEARTBEAT      = 0x21040001;
constexpr uint32_t CMD_MANUAL_MODE    = 0x21010C02;  // 遥控模式
constexpr uint32_t CMD_NAVI_MODE      = 0x21010C03;  // 导航模式
constexpr uint32_t CMD_ASSISTANT_MODE = 0x21010C04;  // 辅助模式

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

// ============ 模式切换函数 ============

// 切换到遥控模式
void switchToManualMode() {
    std::cout << "[INFO] 切换到遥控模式..." << std::endl;
    sendCommand(ROBOT_IP, ROBOT_PORT, CMD_MANUAL_MODE);
}

// 切换到导航模式
void switchToNaviMode() {
    std::cout << "[INFO] 切换到导航模式..." << std::endl;
    sendCommand(ROBOT_IP, ROBOT_PORT, CMD_NAVI_MODE);
}

// 切换到辅助模式
void switchToAssistantMode() {
    std::cout << "[INFO] 切换到辅助模式..." << std::endl;
    sendCommand(ROBOT_IP, ROBOT_PORT, CMD_ASSISTANT_MODE);
}

// ============ 主函数 ============
int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  四足机器人运动模式切换Demo" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "目标机器人: " << ROBOT_IP << ":" << ROBOT_PORT << std::endl;
    std::cout << std::endl;

    // 启动心跳线程
    std::thread hb_thread(heartbeatThread);
    std::cout << "[INFO] 心跳线程已启动 (2Hz)" << std::endl;

    // 等待500ms确保心跳已启动
    usleep(500000);

    // 切换到遥控模式
    switchToManualMode();
    std::cout << "[INFO] 等待3秒..." << std::endl;
    sleep(3);

    // 切换到导航模式
    switchToNaviMode();
    std::cout << "[INFO] 等待3秒..." << std::endl;
    sleep(3);

    // 切换到辅助模式
    switchToAssistantMode();
    std::cout << "[INFO] 等待1秒..." << std::endl;
    sleep(1);

    // 停止心跳线程
    heartbeat_running = false;
    hb_thread.join();

    std::cout << "[INFO] Demo结束" << std::endl;
    return 0;
}

