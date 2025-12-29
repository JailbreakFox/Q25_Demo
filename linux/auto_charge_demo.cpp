// ====================================================================
//          Created:    2025/12/25/ 15:33
//	         Author:	xuyanghe
//	        Company:
// ====================================================================

/**
 * @file auto_charge_demo.cpp
 * @brief 四足机器人自主充电Demo
 *
 * 编译: g++ -std=c++11 -pthread auto_charge_demo.cpp -o auto_charge_demo
 * 运行: ./auto_charge_demo
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
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    UDPCommand cmd(cmd_code, param);
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

// ============ 自主充电函数 ============

void startAutoCharge() {
    std::cout << "[INFO] 启动自主充电任务..." << std::endl;
    sendCommand(ROBOT_IP, ROBOT_PORT, CMD_AUTO_CHARGE_START, CHARGE_START);
}

void stopAutoCharge() {
    std::cout << "[INFO] 停止自主充电任务..." << std::endl;
    sendCommand(ROBOT_IP, ROBOT_PORT, CMD_AUTO_CHARGE_START, CHARGE_STOP);
}

// ============ 主函数 ============
int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  四足机器人自主充电Demo" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "目标机器人: " << ROBOT_IP << ":" << ROBOT_PORT << std::endl;
    std::cout << std::endl;
    std::cout << "注意: 机器人需要先处于导航模式才能执行自主充电" << std::endl;
    std::cout << std::endl;

    // 启动心跳线程
    std::thread hb_thread(heartbeatThread);
    std::cout << "[INFO] 心跳线程已启动 (2Hz)" << std::endl;

    // 等待1s确保心跳已启动
    sleep(1);

    // 启动自主充电
    startAutoCharge();
    std::cout << "[INFO] 充电任务执行中，等待5秒..." << std::endl;
    sleep(5);

    // 停止自主充电
    /*
    stopAutoCharge();
    std::cout << "[INFO] 充电任务已停止" << std::endl;
    sleep(1);
    */

    // 停止心跳线程
    heartbeat_running = false;
    hb_thread.join();

    std::cout << "[INFO] Demo结束" << std::endl;
    return 0;
}

