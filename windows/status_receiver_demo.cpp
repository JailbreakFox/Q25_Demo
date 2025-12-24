/**
 * @file status_receiver_demo.cpp
 * @brief 四足机器人状态接收Demo - 接收机器人上报的状态数据 (Windows版)
 *
 * 编译: 使用 CMake 或 Visual Studio
 * 运行: status_receiver_demo.exe
 *
 * ============================================================================
 *                              网络配置说明
 * ============================================================================
 *
 * 机器人会主动向指定IP和端口发送状态数据，使用本Demo前需要进行网络配置：
 *
 * 【配置方式一】将本机IP配置为 192.168.3.157，监听端口 43893
 *   - 修改本机网卡IP地址为 192.168.3.157
 *   - 确保本机与机器人在同一网段（192.168.3.x）
 *   - 运行本Demo即可接收数据
 *
 * 【配置方式二】修改机器人本体的目标IP/端口配置
 *   - 参考《天狼Q25 Ultra 软件接口规格说明书.docx》
 *   - 修改机器人端的网络配置，将目标IP/端口改为与本机一致
 *   - 修改后重启机器人相关服务使配置生效
 *
 * ============================================================================
 *
 * 接收的数据类型:
 *   - 电池信息: 电量、电压、电流、温度
 *   - IMU数据: 姿态角、角速度、加速度
 *   - 关节数据: 位置、速度、力矩、温度
 *   - 运动状态: 当前步态、运动模式、速度等
 *   - 系统信息: 各模块版本、运行时间、里程等
 */

#include <cstring>
#include <cstdint>
#include <thread>
#include <atomic>
#include <iostream>
#include <iomanip>
#include <algorithm>

// Windows 特定头文件
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

// ============ 配置 ============
// 本机监听配置（需与机器人端配置的目标地址一致）
const char* LOCAL_IP = "192.168.3.157";  // 本机IP
const int LOCAL_PORT = 43893;            // 监听端口

// 接收缓冲区大小
constexpr size_t RECV_BUFFER_SIZE = 65535;

// ============ 数据包类型标识 ============
// 根据实际协议定义，这里列出常见的数据类型
constexpr uint32_t DATA_TYPE_BATTERY  = 0x01;  // 电池数据
constexpr uint32_t DATA_TYPE_IMU      = 0x02;  // IMU数据
constexpr uint32_t DATA_TYPE_JOINT    = 0x03;  // 关节数据
constexpr uint32_t DATA_TYPE_MOTION   = 0x04;  // 运动状态
constexpr uint32_t DATA_TYPE_SYSTEM   = 0x05;  // 系统信息

// ============ 数据结构定义 ============
#pragma pack(push, 1)

// 通用数据包头
struct PacketHeader {
    uint32_t type;        // 数据类型
    uint32_t length;      // 数据长度
    uint64_t timestamp;   // 时间戳
};

// 电池数据
struct BatteryData {
    float voltage;        // 电压 (V)
    float current;        // 电流 (A)
    float percentage;     // 电量百分比 (0-100)
    float temperature;    // 温度 (℃)
};

// IMU数据
struct IMUData {
    // 姿态角 (rad)
    float roll;
    float pitch;
    float yaw;
    // 角速度 (rad/s)
    float gyro_x;
    float gyro_y;
    float gyro_z;
    // 加速度 (m/s^2)
    float acc_x;
    float acc_y;
    float acc_z;
};

// 单关节数据
struct JointData {
    float position;       // 位置 (rad)
    float velocity;       // 速度 (rad/s)
    float torque;         // 力矩 (Nm)
    float temperature;    // 温度 (℃)
};

#pragma pack(pop)

// ============ 全局变量 ============
std::atomic<bool> running(true);
std::atomic<uint64_t> packet_count(0);

// ============ 数据解析函数 ============

void parseBatteryData(const uint8_t* data, size_t len) {
    if (len < sizeof(BatteryData)) return;
    
    const BatteryData* battery = reinterpret_cast<const BatteryData*>(data);
    std::cout << "[电池] 电量: " << std::fixed << std::setprecision(1) 
              << battery->percentage << "%, "
              << "电压: " << battery->voltage << "V, "
              << "电流: " << battery->current << "A, "
              << "温度: " << battery->temperature << "℃" << std::endl;
}

void parseIMUData(const uint8_t* data, size_t len) {
    if (len < sizeof(IMUData)) return;
    
    const IMUData* imu = reinterpret_cast<const IMUData*>(data);
    std::cout << "[IMU] Roll: " << std::fixed << std::setprecision(3) 
              << imu->roll << ", Pitch: " << imu->pitch 
              << ", Yaw: " << imu->yaw << std::endl;
}

void parseJointData(const uint8_t* data, size_t len) {
    size_t joint_count = len / sizeof(JointData);
    std::cout << "[关节] 共 " << joint_count << " 个关节数据" << std::endl;
    
    // 仅打印前4个关节的简要信息
    const JointData* joints = reinterpret_cast<const JointData*>(data);
    for (size_t i = 0; i < (std::min)(joint_count, (size_t)4); i++) {
        std::cout << "  关节" << i << ": pos=" << std::fixed << std::setprecision(2)
                  << joints[i].position << ", vel=" << joints[i].velocity << std::endl;
    }
}

void parsePacket(const uint8_t* buffer, size_t len) {
    if (len < sizeof(PacketHeader)) {
        std::cout << "[WARNING] 数据包过短: " << len << " bytes" << std::endl;
        return;
    }
    
    const PacketHeader* header = reinterpret_cast<const PacketHeader*>(buffer);
    const uint8_t* payload = buffer + sizeof(PacketHeader);
    size_t payload_len = len - sizeof(PacketHeader);
    
    switch (header->type) {
        case DATA_TYPE_BATTERY:
            parseBatteryData(payload, payload_len);
            break;
        case DATA_TYPE_IMU:
            parseIMUData(payload, payload_len);
            break;
        case DATA_TYPE_JOINT:
            parseJointData(payload, payload_len);
            break;
        default:
            std::cout << "[INFO] 收到数据类型: 0x" << std::hex << header->type 
                      << std::dec << ", 长度: " << len << " bytes" << std::endl;
            break;
    }
}

// ============ 接收线程 ============
void receiverThread(SOCKET sock) {
    uint8_t buffer[RECV_BUFFER_SIZE];
    struct sockaddr_in sender_addr;
    int sender_len = sizeof(sender_addr);
    
    while (running) {
        int recv_len = recvfrom(sock, reinterpret_cast<char*>(buffer), RECV_BUFFER_SIZE, 0,
                                (struct sockaddr*)&sender_addr, &sender_len);
        
        if (recv_len > 0) {
            packet_count++;
            
            // 每100个包打印一次统计
            if (packet_count % 100 == 0) {
                char sender_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &sender_addr.sin_addr, sender_ip, INET_ADDRSTRLEN);
                std::cout << "----------------------------------------" << std::endl;
                std::cout << "[统计] 已接收 " << packet_count << " 个数据包, "
                          << "来源: " << sender_ip << ":" << ntohs(sender_addr.sin_port) << std::endl;
                std::cout << "----------------------------------------" << std::endl;
            }
            
            // 解析数据包
            parsePacket(buffer, recv_len);
        }
    }
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
    std::cout << "  四足机器人状态接收Demo (Windows)" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    std::cout << "============ 网络配置说明 ==============" << std::endl;
    std::cout << std::endl;
    std::cout << "【配置方式一】将本机IP配置为 192.168.3.157" << std::endl;
    std::cout << "              监听端口 43893" << std::endl;
    std::cout << std::endl;
    std::cout << "【配置方式二】参考《天狼Q25 Ultra 软件接口规格说明书.docx》" << std::endl;
    std::cout << "              修改机器人本体的目标IP/端口，使其与本机一致" << std::endl;
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // 创建UDP socket
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "[ERROR] 创建socket失败: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return -1;
    }

    // 设置socket选项：允许地址复用
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));

    // 绑定本地地址
    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(LOCAL_PORT);
    local_addr.sin_addr.s_addr = INADDR_ANY;  // 监听所有网卡

    if (bind(sock, (struct sockaddr*)&local_addr, sizeof(local_addr)) == SOCKET_ERROR) {
        std::cerr << "[ERROR] 绑定端口 " << LOCAL_PORT << " 失败: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return -1;
    }

    std::cout << "[INFO] UDP Server 已启动" << std::endl;
    std::cout << "[INFO] 监听端口: " << LOCAL_PORT << std::endl;
    std::cout << "[INFO] 等待接收机器人状态数据..." << std::endl;
    std::cout << "[INFO] 按 Ctrl+C 退出" << std::endl;
    std::cout << std::endl;

    // 启动接收线程
    std::thread recv_thread(receiverThread, sock);

    // 主线程等待（实际应用中可以添加信号处理）
    while (running) {
        Sleep(1000);
    }

    // 清理
    running = false;
    closesocket(sock);
    recv_thread.join();

    // 清理 Winsock
    WSACleanup();

    std::cout << std::endl;
    std::cout << "[INFO] 共接收 " << packet_count << " 个数据包" << std::endl;
    std::cout << "[INFO] Demo结束" << std::endl;
    return 0;
}

