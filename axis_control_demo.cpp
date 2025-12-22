/**
 * @file axis_control_demo.cpp
 * @brief 四足机器人轴控制Demo - 心跳+运动控制
 *
 * 编译: g++ -std=c++11 -pthread axis_control_demo.cpp -o axis_control_demo
 * 运行: ./axis_control_demo
 *
 * 流程:
 *   1. 启动2Hz心跳（每500ms发送一次）
 *   2. 前进2秒
 *   3. 后退2秒
 *   4. 左转2秒
 *   5. 右转2秒
 *   6. 停止并退出
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
constexpr uint32_t CMD_HEARTBEAT     = 0x21040001;
constexpr uint32_t CMD_LEFT_YAXIS    = 0x21010130;  // 左摇杆Y轴（前后）
constexpr uint32_t CMD_LEFT_XAXIS    = 0x21010131;  // 左摇杆X轴（左右）
constexpr uint32_t CMD_RIGHT_XAXIS   = 0x21010135;  // 右摇杆X轴（旋转）

// ============ 轴值定义 ============
// 左摇杆Y轴（前后）死区: -6553 ~ 6553
constexpr int32_t AXIS_FORWARD  = 20000;   // 前进（超过6553即可）
constexpr int32_t AXIS_BACKWARD = -20000;  // 后退（小于-6553即可）

// 左摇杆X轴（左右平移）死区: -24576 ~ 24576
constexpr int32_t AXIS_MOVE_LEFT  = -30000;  // 左移（小于-24576即可）
constexpr int32_t AXIS_MOVE_RIGHT = 30000;   // 右移（超过24576即可）

// 右摇杆X轴（旋转）死区: -28212 ~ 28212
constexpr int32_t AXIS_TURN_LEFT  = -30000;  // 左转（小于-28212即可）
constexpr int32_t AXIS_TURN_RIGHT = 30000;   // 右转（超过28212即可）

constexpr int32_t AXIS_STOP = 0;  // 停止（所有轴）

// ============ UDP命令结构 ============
#pragma pack(push, 1)
struct UDPCommand {
    uint32_t code;
    uint32_t parameters_size;  // 对于轴指令，这个字段存储轴值
    uint32_t type;

    UDPCommand(uint32_t cmd, int32_t param = 0)
        : code(cmd)
        , parameters_size(static_cast<uint32_t>(param))  // 将int32_t转为uint32_t
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

// ============ 运动控制函数 ============

// 前进（左摇杆Y轴正向）
void moveForward(int duration_sec) {
    sendCommand(ROBOT_IP, ROBOT_PORT, CMD_LEFT_YAXIS, AXIS_FORWARD);
    sleep(duration_sec);
    sendCommand(ROBOT_IP, ROBOT_PORT, CMD_LEFT_YAXIS, AXIS_STOP);
}

// 后退（左摇杆Y轴负向）
void moveBackward(int duration_sec) {
    sendCommand(ROBOT_IP, ROBOT_PORT, CMD_LEFT_YAXIS, AXIS_BACKWARD);
    sleep(duration_sec);
    sendCommand(ROBOT_IP, ROBOT_PORT, CMD_LEFT_YAXIS, AXIS_STOP);
}

// 左转（右摇杆X轴负向）
void turnLeft(int duration_sec) {
    sendCommand(ROBOT_IP, ROBOT_PORT, CMD_RIGHT_XAXIS, AXIS_TURN_LEFT);
    sleep(duration_sec);
    sendCommand(ROBOT_IP, ROBOT_PORT, CMD_RIGHT_XAXIS, AXIS_STOP);
}

// 右转（右摇杆X轴正向）
void turnRight(int duration_sec) {
    sendCommand(ROBOT_IP, ROBOT_PORT, CMD_RIGHT_XAXIS, AXIS_TURN_RIGHT);
    sleep(duration_sec);
    sendCommand(ROBOT_IP, ROBOT_PORT, CMD_RIGHT_XAXIS, AXIS_STOP);
}

// 左移（左摇杆X轴负向）
void moveLeft(int duration_sec) {
    sendCommand(ROBOT_IP, ROBOT_PORT, CMD_LEFT_XAXIS, AXIS_MOVE_LEFT);
    sleep(duration_sec);
    sendCommand(ROBOT_IP, ROBOT_PORT, CMD_LEFT_XAXIS, AXIS_STOP);
}

// 右移（左摇杆X轴正向）
void moveRight(int duration_sec) {
    sendCommand(ROBOT_IP, ROBOT_PORT, CMD_LEFT_XAXIS, AXIS_MOVE_RIGHT);
    sleep(duration_sec);
    sendCommand(ROBOT_IP, ROBOT_PORT, CMD_LEFT_XAXIS, AXIS_STOP);
}

// ============ 主函数 ============
int main() {
    // 启动心跳线程
    std::thread hb_thread(heartbeatThread);

    // 等待500ms确保心跳已启动
    usleep(500000);

    // 前进2秒
    moveForward(2);
    sleep(1);

    // 后退2秒
    moveBackward(2);
    sleep(1);

    // 左转2秒
    turnLeft(2);
    sleep(1);

    // 右转2秒
    turnRight(2);
    sleep(1);

    // 左移2秒
    moveLeft(2);
    sleep(1);

    // 右移2秒
    moveRight(2);
    sleep(1);

    // 停止心跳线程
    heartbeat_running = false;
    hb_thread.join();

    return 0;
}
