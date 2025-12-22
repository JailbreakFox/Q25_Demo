/**
 * @file stand_lie_demo.cpp
 * @brief 四足机器人完整Demo - 心跳+站立+趴下
 *
 * 编译: g++ -std=c++11 -pthread stand_lie_demo.cpp -o stand_lie_demo
 * 运行: ./stand_lie_demo
 *
 * 流程:
 *   1. 启动2Hz心跳（每500ms发送一次）
 *   2. 发送站立命令
 *   3. 等待3秒
 *   4. 发送趴下命令
 *   5. 等待1秒后退出
 */

#include <cstring>
#include <cstdint>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <thread>
#include <atomic>

// ============ 配置 ============
const char* ROBOT_IP = "192.168.3.20";
const int ROBOT_PORT = 43893;

// ============ 命令码 ============
constexpr uint32_t CMD_HEARTBEAT = 0x21040001;
constexpr uint32_t CMD_STAND_UP  = 0x21010202;
constexpr uint32_t CMD_LIE_DOWN  = 0x21010222;

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

// ============ 主函数 ============
int main() {
    // 启动心跳线程
    std::thread hb_thread(heartbeatThread);

    // 等待500ms确保心跳已启动
    usleep(500000);

    // 站立
    sendCommand(ROBOT_IP, ROBOT_PORT, CMD_STAND_UP);

    // 等待3秒
    sleep(3);

    // 趴下
    sendCommand(ROBOT_IP, ROBOT_PORT, CMD_LIE_DOWN);

    // 等待1秒
    sleep(1);

    // 停止心跳线程
    heartbeat_running = false;
    hb_thread.join();

    return 0;
}
