// ====================================================================
//          Created:    2025/12/25/ 15:33
//	         Author:	xuyanghe
//	        Company:
// ====================================================================

/**
 * @file power_control_demo.cpp
 * @brief 四足机器人设备电源控制Demo
 *
 * 编译: g++ -std=c++11 -pthread power_control_demo.cpp -o power_control_demo
 * 运行: ./power_control_demo
 *
 * 流程:
 *   1. 启动2Hz心跳（每500ms发送一次）
 *   2. 开启前上雷达电源
 *   3. 等待2秒
 *   4. 开启前下雷达电源
 *   5. 等待2秒
 *   6. 开启外挂电脑电源
 *   7. 等待2秒
 *   8. 关闭所有雷达电源
 *   9. 等待1秒后退出
 *
 * 设备电源说明:
 *   - LIDAR_FU: 前上雷达 (Front Upper)
 *   - LIDAR_FL: 前下雷达 (Front Lower)
 *   - LIDAR_BU: 后上雷达 (Back Upper)
 *   - LIDAR_BL: 后下雷达 (Back Lower)
 *   - UPLOAD: 外挂电脑/载荷电源
 *   - DRIVER_MOTOR: 驱动电机电源
 *
 * 参数说明:
 *   - 0: 关闭电源
 *   - 1: 开启电源
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
constexpr uint32_t CMD_HEARTBEAT           = 0x21040001;
constexpr uint32_t CMD_POWER_DRIVER_MOTOR  = 0x80110201;  // 驱动电机电源
constexpr uint32_t CMD_POWER_STATUS        = 0x80110202;  // 电源状态查询
constexpr uint32_t CMD_POWER_UPLOAD        = 0x80110801;  // 外挂电脑电源
constexpr uint32_t CMD_POWER_LIDAR_FU      = 0x80110501;  // 前上雷达电源
constexpr uint32_t CMD_POWER_LIDAR_FL      = 0x80110502;  // 前下雷达电源
constexpr uint32_t CMD_POWER_LIDAR_BU      = 0x80110503;  // 后上雷达电源
constexpr uint32_t CMD_POWER_LIDAR_BL      = 0x80110504;  // 后下雷达电源

// ============ 电源状态 ============
constexpr int32_t POWER_OFF = 0;  // 关闭
constexpr int32_t POWER_ON  = 1;  // 开启

// ============ UDP命令结构 ============
#pragma pack(push, 1)
struct UDPCommand {
    uint32_t code;
    uint32_t parameters_size;  // 对于电源指令，这个字段存储开关状态
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

// ============ 电源控制函数 ============

void setLidarFUPower(bool on) {
    std::cout << "[INFO] " << (on ? "开启" : "关闭") << "前上雷达电源..." << std::endl;
    sendCommand(ROBOT_IP, ROBOT_PORT, CMD_POWER_LIDAR_FU, on ? POWER_ON : POWER_OFF);
}

void setLidarFLPower(bool on) {
    std::cout << "[INFO] " << (on ? "开启" : "关闭") << "前下雷达电源..." << std::endl;
    sendCommand(ROBOT_IP, ROBOT_PORT, CMD_POWER_LIDAR_FL, on ? POWER_ON : POWER_OFF);
}

void setLidarBUPower(bool on) {
    std::cout << "[INFO] " << (on ? "开启" : "关闭") << "后上雷达电源..." << std::endl;
    sendCommand(ROBOT_IP, ROBOT_PORT, CMD_POWER_LIDAR_BU, on ? POWER_ON : POWER_OFF);
}

void setLidarBLPower(bool on) {
    std::cout << "[INFO] " << (on ? "开启" : "关闭") << "后下雷达电源..." << std::endl;
    sendCommand(ROBOT_IP, ROBOT_PORT, CMD_POWER_LIDAR_BL, on ? POWER_ON : POWER_OFF);
}

void setUploadPower(bool on) {
    std::cout << "[INFO] " << (on ? "开启" : "关闭") << "外挂电脑电源..." << std::endl;
    sendCommand(ROBOT_IP, ROBOT_PORT, CMD_POWER_UPLOAD, on ? POWER_ON : POWER_OFF);
}

void setDriverMotorPower(bool on) {
    std::cout << "[INFO] " << (on ? "开启" : "关闭") << "驱动电机电源..." << std::endl;
    sendCommand(ROBOT_IP, ROBOT_PORT, CMD_POWER_DRIVER_MOTOR, on ? POWER_ON : POWER_OFF);
}

// ============ 主函数 ============
int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  四足机器人设备电源控制Demo" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "目标机器人: " << ROBOT_IP << ":" << ROBOT_PORT << std::endl;
    std::cout << std::endl;

    // 启动心跳线程
    std::thread hb_thread(heartbeatThread);
    std::cout << "[INFO] 心跳线程已启动 (2Hz)" << std::endl;

    // 等待1s确保心跳已启动
    sleep(1);

    // 开启前上雷达
    setLidarFUPower(true);
    std::cout << "[INFO] 等待2秒..." << std::endl;
    sleep(2);

    // 开启前下雷达
    setLidarFLPower(true);
    std::cout << "[INFO] 等待2秒..." << std::endl;
    sleep(2);

    // 开启外挂电脑
    setUploadPower(true);
    std::cout << "[INFO] 等待2秒..." << std::endl;
    sleep(2);

    // 关闭所有雷达
    std::cout << "[INFO] 关闭所有雷达电源..." << std::endl;
    setLidarFUPower(false);
    setLidarFLPower(false);
    setLidarBUPower(false);
    setLidarBLPower(false);
    sleep(1);

    // 关闭外挂电脑
    setUploadPower(false);
    sleep(1);

    // 停止心跳线程
    heartbeat_running = false;
    hb_thread.join();

    std::cout << "[INFO] Demo结束" << std::endl;
    return 0;
}

