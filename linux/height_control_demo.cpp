// ====================================================================
//          Created:    2025/12/25/ 15:33
//	         Author:	xuyanghe
//	        Company:
// ====================================================================

/**
 * @file height_control_demo.cpp
 * @brief 四足机器人高度调节Demo
 *
 * 编译: g++ -std=c++11 -pthread height_control_demo.cpp -o height_control_demo
 * 运行: ./height_control_demo
 *
 * 流程:
 *   1. 启动2Hz心跳（每500ms发送一次）
 *   2. 发送站立命令
 *   3. 设置低高度
 *   4. 等待2秒
 *   5. 设置中高度
 *   6. 等待2秒
 *   7. 设置高高度
 *   8. 等待2秒
 *   9. 趴下并退出
 *
 * 高度说明:
 *   - 0: 低高度 - 重心较低，更稳定
 *   - 1: 中高度 - 默认高度
 *   - 2: 高高度 - 视野更好，但稳定性略降
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
constexpr uint32_t CMD_HEARTBEAT     = 0x21040001;
constexpr uint32_t CMD_STAND_UP      = 0x21010202;
constexpr uint32_t CMD_LIE_DOWN      = 0x21010222;
constexpr uint32_t CMD_CHANGE_HEIGHT = 0x21010406;  // 高度调节

// ============ 高度值定义 ============
constexpr int32_t HEIGHT_LOW    = 0;  // 低高度
constexpr int32_t HEIGHT_MEDIUM = 1;  // 中高度
constexpr int32_t HEIGHT_HIGH   = 2;  // 高高度

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

// ============ 高度调节函数 ============

void setHeightLow() {
    std::cout << "[INFO] 设置低高度..." << std::endl;
    sendCommand(ROBOT_IP, ROBOT_PORT, CMD_CHANGE_HEIGHT, HEIGHT_LOW);
}

void setHeightMedium() {
    std::cout << "[INFO] 设置中高度..." << std::endl;
    sendCommand(ROBOT_IP, ROBOT_PORT, CMD_CHANGE_HEIGHT, HEIGHT_MEDIUM);
}

void setHeightHigh() {
    std::cout << "[INFO] 设置高高度..." << std::endl;
    sendCommand(ROBOT_IP, ROBOT_PORT, CMD_CHANGE_HEIGHT, HEIGHT_HIGH);
}

// ============ 主函数 ============
int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  四足机器人高度调节Demo" << std::endl;
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

    // 设置低高度
    setHeightLow();
    std::cout << "[INFO] 等待2秒..." << std::endl;
    sleep(2);

    // 设置中高度
    setHeightMedium();
    std::cout << "[INFO] 等待2秒..." << std::endl;
    sleep(2);

    // 设置高高度
    setHeightHigh();
    std::cout << "[INFO] 等待2秒..." << std::endl;
    sleep(2);

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

