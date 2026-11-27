#/*********************************************************************************************************************
 * Wuwu 开源库（Wuwu Open Source Library） — 摄像头模块
 * 版权所有 (c) 2025 Blockingsys
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * 本文件是 Wuwu 开源库 的一部分。
 *
 * 本文件按照 GNU 通用公共许可证 第3版（GPLv3）或您选择的任何后续版本的条款授权。
 * 您可以在遵守 GPL-3.0 许可条款的前提下，自由地使用、复制、修改和分发本文件及其衍生作品。
 * 在分发本文件或其衍生作品时，必须以相同的许可证（GPL-3.0）对源代码进行授权并随附许可证副本。
 *
 * 本软件按“原样”提供，不对适销性、特定用途适用性或不侵权做任何明示或暗示的保证。
 * 有关更多细节，请参阅 GNU 官方许可证文本： https://www.gnu.org/licenses/gpl-3.0.html
 *
 * 注：本注释为 GPL-3.0 许可证的中文说明与摘要，不构成法律意见。正式许可以 GPL 原文为准。
 * LICENSE 副本通常位于项目根目录的 LICENSE 文件或 libraries 文件夹下；若未找到，请访问上方链接获取。
 *
 * 额外说明：
 * - 本项目可能包含第三方组件，各组件的版权与许可以其各自随附的 LICENSE 为准；
 * - 分发、修改本文件时请保留本版权与许可声明以尊重原作者权利；
 * - 本文档为中文译述/摘要，英文许可证文本为法律权威版。
 *
 * 文件名称：ww_camera_server.cc
 * 所属模块：wuwu_library
 * 功能描述：摄像头服务器，用于 MJPEG 流与 HTTP 接口
 * 版本信息：详见 libraries/doc/version
 * 开发环境：Linux，GCC / Clang，OpenCV，V4L2
 * 联系/主页：请参阅项目 README
 *
 * 修改记录：
 * 日期        作者              说明
 * 2025-12-16  Blockingsys    添加 GPL-3.0 中文许可头
 ********************************************************************************************************************/

#include "zf_common_headfile.hpp"
#include "ww_camera_server.h"
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <net/if.h>
#include <netdb.h>
#include <ifaddrs.h>

namespace
{
class DataStreamServer
{
public:
    DataStreamServer(void)
        : server_sock_fd(-1)
        , server_port(DATA_STREAM_DEFAULT_PORT)
        , running(false)
        , server_thread_id(0)
    {
        pthread_mutex_init(&sock_mutex, NULL);
        pthread_mutex_init(&client_mutex, NULL);
    }

    ~DataStreamServer(void)
    {
        stop_server();
        pthread_mutex_destroy(&sock_mutex);
        pthread_mutex_destroy(&client_mutex);
    }

    int start_server(int port)
    {
        if (running) {
            std::cout << "调试数据服务器已经在运行中" << std::endl;
            return 0;
        }

        server_port = port;
        running = true;
        close_all_clients();

        if (pthread_create(&server_thread_id, NULL, server_thread_func, this) != 0) {
            std::cerr << "创建调试数据服务器线程失败" << std::endl;
            running = false;
            return -1;
        }

        pthread_detach(server_thread_id);
        std::cout << "调试数据服务器启动中..." << std::endl;
        return 0;
    }

    void stop_server(void)
    {
        if (!running) return;

        std::cout << "正在停止调试数据服务器..." << std::endl;
        running = false;
        close_server_socket();
        close_all_clients();
        std::cout << "调试数据服务器已停止" << std::endl;
    }

    bool is_running(void) const
    {
        return running;
    }

    void upload_channels(const char* prefix, const float* channels, size_t channel_count)
    {
        if (!running || channels == NULL || channel_count == 0) return;

        std::string safe_prefix = (prefix == NULL) ? "" : prefix;
        if (safe_prefix == "image") {
            return;
        }

        std::ostringstream oss;
        if (!safe_prefix.empty()) {
            oss << safe_prefix << ":";
        }

        for (size_t i = 0; i < channel_count; ++i) {
            if (i > 0) {
                oss << ",";
            }
            oss << std::fixed << std::setprecision(6) << channels[i];
        }
        oss << "\n";

        const std::string payload = oss.str();

        pthread_mutex_lock(&client_mutex);
        for (std::vector<int>::iterator it = client_socks.begin(); it != client_socks.end();) {
            const ssize_t send_len = send(*it, payload.c_str(), payload.size(), MSG_NOSIGNAL);
            if (send_len < 0) {
                close(*it);
                it = client_socks.erase(it);
            } else {
                ++it;
            }
        }
        pthread_mutex_unlock(&client_mutex);
    }

private:
    int server_sock_fd;
    int server_port;
    bool running;
    pthread_t server_thread_id;
    pthread_mutex_t sock_mutex;
    pthread_mutex_t client_mutex;
    std::vector<int> client_socks;

    void close_server_socket(void)
    {
        pthread_mutex_lock(&sock_mutex);
        if (server_sock_fd >= 0) {
            shutdown(server_sock_fd, SHUT_RDWR);
            close(server_sock_fd);
            server_sock_fd = -1;
        }
        pthread_mutex_unlock(&sock_mutex);
    }

    void close_all_clients(void)
    {
        pthread_mutex_lock(&client_mutex);
        for (size_t i = 0; i < client_socks.size(); ++i) {
            shutdown(client_socks[i], SHUT_RDWR);
            close(client_socks[i]);
        }
        client_socks.clear();
        pthread_mutex_unlock(&client_mutex);
    }

    std::string get_local_ip(void) const
    {
        struct ifaddrs *ifaddr, *ifa;
        char host[NI_MAXHOST];

        if (getifaddrs(&ifaddr) == -1) {
            return "127.0.0.1";
        }

        std::string result = "127.0.0.1";
        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_INET) continue;
            if (strcmp(ifa->ifa_name, "lo") == 0) continue;

            const int s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                                      host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s == 0) {
                result = host;
                break;
            }
        }

        freeifaddrs(ifaddr);
        return result;
    }

    static void* server_thread_func(void* arg)
    {
        DataStreamServer* server = static_cast<DataStreamServer*>(arg);
        if (!server) return NULL;

        const int server_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (server_sock < 0) {
            std::cerr << "创建调试数据服务器socket失败" << std::endl;
            server->running = false;
            return NULL;
        }

        pthread_mutex_lock(&server->sock_mutex);
        server->server_sock_fd = server_sock;
        pthread_mutex_unlock(&server->sock_mutex);

        int opt = 1;
        setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(server->server_port);

        if (bind(server_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            std::cerr << "调试数据服务器绑定端口失败" << std::endl;
            close(server_sock);
            server->running = false;
            return NULL;
        }

        if (listen(server_sock, 5) < 0) {
            std::cerr << "调试数据服务器监听失败" << std::endl;
            close(server_sock);
            server->running = false;
            return NULL;
        }

        std::cout << "\n======================================" << std::endl;
        std::cout << "📈 调试数据服务器启动成功!" << std::endl;
        std::cout << "======================================" << std::endl;
        std::cout << "监听端口: " << server->server_port << std::endl;
        std::cout << "本机IP: " << server->get_local_ip() << std::endl;
        std::cout << "数据格式: <any>:ch0,ch1,...,chN\\n" << std::endl;
        std::cout << "======================================\n" << std::endl;

        while (server->running) {
            struct sockaddr_in client_addr;
            socklen_t len = sizeof(client_addr);
            const int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &len);

            if (client_sock < 0) {
                if (!server->running) break;
                continue;
            }

            setsockopt(client_sock, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));

            pthread_mutex_lock(&server->client_mutex);
            server->client_socks.push_back(client_sock);
            pthread_mutex_unlock(&server->client_mutex);
        }

        server->close_server_socket();
        server->close_all_clients();
        return NULL;
    }
};

DataStreamServer g_data_server;
}


// 静态成员初始化
CameraStreamServer* CameraStreamServer::instance = nullptr;

// HTML查看器内容
const char* viewer_html = R"HTML(
<!DOCTYPE html> 
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>摄像头实时查看</title>
    <style>
        body { margin: 0; padding: 20px; background: #1a1a1a; font-family: Arial, sans-serif; }
        .container { max-width: 1200px; margin: 0 auto; background: #2d2d2d; border-radius: 10px; padding: 20px; box-shadow: 0 5px 20px rgba(0,0,0,0.5); }
        h1 { color: #fff; text-align: center; margin-bottom: 20px; }
        #stream { width: 100%; border-radius: 8px; background: #000; }
        .controls { margin-top: 20px; text-align: center; }
        button { background: #4CAF50; color: white; border: none; padding: 12px 24px; margin: 5px; border-radius: 5px; cursor: pointer; font-size: 16px; }
        button:hover { background: #45a049; }
        .snapshot-btn { background: #2196F3; }
        .snapshot-btn:hover { background: #0b7dda; }
        .record-btn { background: #e53935; }
        .record-btn:hover { background: #c62828; }
        .record-btn.recording { background: #ff9800; }
        .record-btn.recording:hover { background: #fb8c00; }
        .info { color: #aaa; margin-top: 15px; font-size: 14px; line-height: 1.6; }
        .hint { color: #f5a623; font-size: 13px; margin-top: 8px; }
        .recording-text { color: #ff9800; font-weight: bold; }
        .status { display: inline-block; width: 10px; height: 10px; border-radius: 50%; background: #4CAF50; margin-right: 8px; animation: pulse 2s infinite; }
        @keyframes pulse { 0%, 100% { opacity: 1; } 50% { opacity: 0.5; } }
        .filename-config { margin-top: 15px; text-align: center; }
        .filename-config label { color: #aaa; font-size: 14px; margin-right: 10px; }
        .filename-config input { background: #1a1a1a; color: #fff; border: 1px solid #555; padding: 8px 12px; border-radius: 5px; font-size: 14px; width: 200px; }
        .filename-config input:focus { outline: none; border-color: #4CAF50; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🎥 摄像头实时查看 <span class="status"></span></h1>
        <img id="stream" src="/stream" alt="摄像头画面">
        <div class="controls">
            <button class="snapshot-btn" onclick="takeSnapshot()">📸 拍照保存</button>
            <button id="recordButton" class="record-btn" onclick="toggleRecording()">⏺ 开始录制</button>
            <button onclick="reconnect()">🔄 重新连接</button>
            <button onclick="toggleFullscreen()">⛶ 全屏</button>
        </div>
        <div class="filename-config">
            <label>文件名前缀:</label>
            <input type="text" id="filenamePrefix" value="snapshot" placeholder="snapshot" />
            <span style="color: #777; font-size: 12px; margin-left: 10px;">格式: 前缀_年月日_时分秒.(png/webm)</span>
        </div>
        <div class="info">
            <p>• 点击"拍照保存"下载原始无损图片(PNG格式) • 支持全屏查看 • 视频流: <span id="url"></span></p>
            <p>• 点击"开始录制"可保存当前图传视频(WebM格式) • <span id="recordingStatus">当前未录制</span></p>
            <p>• 快捷键: 按 <strong>K</strong> 键快速拍照 • 按 <strong>V</strong> 键开始/停止录制 • 按 <strong>F</strong> 键全屏</p>
            <p>• 延迟: <strong><span id="latency">--</span></strong> • 实时帧率: <strong><span id="fps">--</span></strong></p>
            <p id="clock-hint" class="hint"></p>
        </div>
    </div>
    <script>
        document.getElementById('url').textContent = window.location.origin + '/stream';
        const img = document.getElementById('stream');
        const recordButton = document.getElementById('recordButton');
        const recordingStatus = document.getElementById('recordingStatus');
        const recordCanvas = document.createElement('canvas');
        const recordContext = recordCanvas.getContext('2d');
        let mediaRecorder = null;
        let recordedChunks = [];
        let recordingMimeType = '';
        let drawFrameRequestId = null;
        let isRecording = false;
        
        // 加载保存的文件名前缀
        const savedPrefix = localStorage.getItem('filenamePrefix') || 'snapshot';
        document.getElementById('filenamePrefix').value = savedPrefix;
        
        // 监听文件名前缀变化，自动保存
        document.getElementById('filenamePrefix').addEventListener('change', function() {
            const prefix = this.value.trim() || 'snapshot';
            localStorage.setItem('filenamePrefix', prefix);
            console.log('文件名前缀已保存:', prefix);
        });
        
        function showNotification(message, backgroundColor = '#4CAF50') {
            const notification = document.createElement('div');
            notification.textContent = message;
            notification.style.cssText = `position:fixed;top:20px;right:20px;background:${backgroundColor};color:white;padding:15px 25px;border-radius:5px;box-shadow:0 2px 10px rgba(0,0,0,0.3);z-index:9999;`;
            document.body.appendChild(notification);
            setTimeout(() => {
                if (notification.parentNode) {
                    notification.parentNode.removeChild(notification);
                }
            }, 3000);
        }

        function takeSnapshot() {
            // 获取用户自定义的文件名前缀
            const prefix = document.getElementById('filenamePrefix').value.trim() || 'snapshot';
            
            // 生成带时间戳的文件名
            const now = new Date();
            const year = now.getFullYear();
            const month = String(now.getMonth() + 1).padStart(2, '0');
            const day = String(now.getDate()).padStart(2, '0');
            const hour = String(now.getHours()).padStart(2, '0');
            const minute = String(now.getMinutes()).padStart(2, '0');
            const second = String(now.getSeconds()).padStart(2, '0');
            const filename = `${prefix}_${year}${month}${day}_${hour}${minute}${second}.png`;
            
            // 创建隐藏的下载链接，将前缀通过URL参数传递给后端
            const a = document.createElement('a');
            a.href = `/snapshot?prefix=${encodeURIComponent(prefix)}`;
            a.download = filename;
            a.style.display = 'none';
            document.body.appendChild(a);
            a.click();
            document.body.removeChild(a);
            
            // 显示提示（可选）
            showNotification('✓ 正在下载: ' + filename);
        }
        
        function reconnect() {
            img.src = '/stream?t=' + new Date().getTime();
        }
        
        function toggleFullscreen() {
            if (!document.fullscreenElement) {
                img.requestFullscreen();
            } else {
                document.exitFullscreen();
            }
        }

        function buildTimestamp() {
            const now = new Date();
            const year = now.getFullYear();
            const month = String(now.getMonth() + 1).padStart(2, '0');
            const day = String(now.getDate()).padStart(2, '0');
            const hour = String(now.getHours()).padStart(2, '0');
            const minute = String(now.getMinutes()).padStart(2, '0');
            const second = String(now.getSeconds()).padStart(2, '0');
            return `${year}${month}${day}_${hour}${minute}${second}`;
        }

        function getSupportedRecordingMimeType() {
            const candidates = [
                'video/webm;codecs=vp9',
                'video/webm;codecs=vp8',
                'video/webm'
            ];
            for (const mimeType of candidates) {
                if (window.MediaRecorder && MediaRecorder.isTypeSupported(mimeType)) {
                    return mimeType;
                }
            }
            return '';
        }

        function updateRecordingUi(recording) {
            isRecording = recording;
            recordButton.classList.toggle('recording', recording);
            recordButton.textContent = recording ? '■ 停止录制' : '⏺ 开始录制';
            recordingStatus.textContent = recording ? '正在录制中...' : '当前未录制';
            recordingStatus.classList.toggle('recording-text', recording);
        }

        function drawRecordingFrame() {
            if (!isRecording) {
                drawFrameRequestId = null;
                return;
            }
            if (img.naturalWidth > 0 && img.naturalHeight > 0) {
                if (recordCanvas.width !== img.naturalWidth || recordCanvas.height !== img.naturalHeight) {
                    recordCanvas.width = img.naturalWidth;
                    recordCanvas.height = img.naturalHeight;
                }
                recordContext.drawImage(img, 0, 0, recordCanvas.width, recordCanvas.height);
            }
            drawFrameRequestId = requestAnimationFrame(drawRecordingFrame);
        }

        function stopDrawingRecordingFrame() {
            if (drawFrameRequestId !== null) {
                cancelAnimationFrame(drawFrameRequestId);
                drawFrameRequestId = null;
            }
        }

        function saveRecordedVideo() {
            if (!recordedChunks.length) {
                showNotification('✗ 没有录到有效视频数据', '#f44336');
                return;
            }
            const prefix = document.getElementById('filenamePrefix').value.trim() || 'record';
            const filename = `${prefix}_${buildTimestamp()}.webm`;
            const blob = new Blob(recordedChunks, { type: recordingMimeType || 'video/webm' });
            const url = URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url;
            a.download = filename;
            a.style.display = 'none';
            document.body.appendChild(a);
            a.click();
            document.body.removeChild(a);
            setTimeout(() => URL.revokeObjectURL(url), 1000);
            showNotification('✓ 录制完成: ' + filename);
        }

        function startRecording() {
            if (!window.MediaRecorder) {
                showNotification('✗ 当前浏览器不支持视频录制', '#f44336');
                return;
            }
            if (img.naturalWidth <= 0 || img.naturalHeight <= 0) {
                showNotification('✗ 请等待图像加载后再开始录制', '#f44336');
                return;
            }

            recordingMimeType = getSupportedRecordingMimeType();
            if (!recordingMimeType) {
                showNotification('✗ 当前浏览器不支持 WebM 录制', '#f44336');
                return;
            }

            recordCanvas.width = img.naturalWidth;
            recordCanvas.height = img.naturalHeight;
            recordedChunks = [];

            try {
                const stream = recordCanvas.captureStream(25);
                mediaRecorder = new MediaRecorder(stream, { mimeType: recordingMimeType });
            } catch (error) {
                showNotification('✗ 初始化录制失败: ' + error.message, '#f44336');
                return;
            }

            mediaRecorder.ondataavailable = function(event) {
                if (event.data && event.data.size > 0) {
                    recordedChunks.push(event.data);
                }
            };

            mediaRecorder.onstop = function() {
                stopDrawingRecordingFrame();
                updateRecordingUi(false);
                saveRecordedVideo();
                mediaRecorder = null;
            };

            mediaRecorder.onerror = function(event) {
                stopDrawingRecordingFrame();
                updateRecordingUi(false);
                const message = event.error && event.error.message ? event.error.message : '未知错误';
                showNotification('✗ 录制失败: ' + message, '#f44336');
                mediaRecorder = null;
            };

            updateRecordingUi(true);
            drawRecordingFrame();
            mediaRecorder.start(1000);
            showNotification('● 已开始录制视频', '#e53935');
        }

        function stopRecording() {
            if (mediaRecorder && mediaRecorder.state !== 'inactive') {
                mediaRecorder.stop();
            }
        }

        function toggleRecording() {
            if (isRecording) {
                stopRecording();
            } else {
                startRecording();
            }
        }

        // 键盘快捷键
        document.addEventListener('keydown', function(event) {
            // 如果焦点在输入框上，不触发快捷键
            const activeElement = document.activeElement;
            if (activeElement && (activeElement.tagName === 'INPUT' || activeElement.tagName === 'TEXTAREA')) {
                return;
            }
            
            // K键 - 拍照保存
            if (event.key === 'k' || event.key === 'K') {
                event.preventDefault();
                takeSnapshot();
            }
            // F键 - 全屏切换
            else if (event.key === 'f' || event.key === 'F') {
                event.preventDefault();
                toggleFullscreen();
            }
            // R键 - 重新连接
            else if (event.key === 'r' || event.key === 'R') {
                event.preventDefault();
                reconnect();
            }
            // V键 - 开始/停止录制
            else if (event.key === 'v' || event.key === 'V') {
                event.preventDefault();
                toggleRecording();
            }
        });

        window.addEventListener('beforeunload', function() {
            if (isRecording) {
                stopRecording();
            }
        });

        async function updateStats() {
            try {
                const response = await fetch('/stats');
                if (!response.ok) throw new Error('stats fetch failed');
                const data = await response.json();
                const latencyEl = document.getElementById('latency');
                const fpsEl = document.getElementById('fps');
                const hintEl = document.getElementById('clock-hint');
                hintEl.textContent = '';

                const captureTs = Number(data.latestCaptureTsMs) || 0;
                const serverTs = Number(data.serverTsMs) || 0;
                const browserNow = Date.now();

                if (captureTs && serverTs) {
                    const internalLatency = Math.max(0, serverTs - captureTs);
                    const clockOffset = browserNow - serverTs;
                    const networkLatency = Math.max(0, clockOffset);
                    if (Math.abs(clockOffset) > 2000) {
                        latencyEl.textContent = internalLatency + ' ms (板载)';
                        hintEl.textContent = '⚠️ 开发板时钟未和电脑同步，浏览器显示的总延迟会偏大。';
                    } else {
                        const endToEnd = internalLatency + networkLatency;
                        latencyEl.textContent = endToEnd + ' ms';
                    }
                } else {
                    latencyEl.textContent = '--';
                }

                if (data.estimatedFps && data.estimatedFps > 0) {
                    fpsEl.textContent = Number(data.estimatedFps).toFixed(1) + ' FPS';
                } else {
                    fpsEl.textContent = '--';
                }
            } catch (err) {
                document.getElementById('latency').textContent = 'N/A';
                document.getElementById('fps').textContent = 'N/A';
                document.getElementById('clock-hint').textContent = '';
            }
        }

        setInterval(updateStats, 1000);
        updateStats();
    </script>
</body>
</html>
)HTML";

CameraStreamServer::CameraStreamServer(void)
    : server_sock_fd(-1)
    , server_port(CAMERA_STREAM_DEFAULT_PORT)
    , running(false)
    , server_thread_id(0)
    , latest_frame_id(0)
    , latest_capture_ts_ms(0)
    , ema_fps(0.0)
{
    pthread_mutex_init(&frame_mutex, NULL);
    pthread_cond_init(&frame_cond, NULL);
    pthread_mutex_init(&sock_mutex, NULL);
    pthread_mutex_init(&original_frame_mutex, NULL);
}

CameraStreamServer::~CameraStreamServer(void)
{
    stop_server();
    pthread_mutex_destroy(&frame_mutex);
    pthread_cond_destroy(&frame_cond);
    pthread_mutex_destroy(&sock_mutex);
    pthread_mutex_destroy(&original_frame_mutex);
}


/*******************************************************************
 * @brief       获取本机IP地址
 * 
 * @return      返回本机IP地址字符串
 * 
 * @note        自动选择优先级最高的网络接口IP
 ******************************************************************/
std::string CameraStreamServer::get_local_ip(void)
{
    struct ifaddrs *ifaddr, *ifa;
    char host[NI_MAXHOST];
    
    if (getifaddrs(&ifaddr) == -1) {
        return "127.0.0.1";
    }
    
    std::string result = "127.0.0.1";
    std::string fallback_ip = "";
    int best_priority = -1;
    
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;
        
        int family = ifa->ifa_addr->sa_family;
        
        // 只处理IPv4地址
        if (family == AF_INET) {
            // 跳过回环地址
            if (strcmp(ifa->ifa_name, "lo") == 0) continue;
            
            int s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                              host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0) continue;
            
            // 计算接口优先级
            int priority = 0;
            bool is_up = (ifa->ifa_flags & IFF_UP) != 0;
            bool is_running = (ifa->ifa_flags & IFF_RUNNING) != 0;
            
            // UP 且 RUNNING 的接口优先级最高
            if (is_up && is_running) {
                priority = 100;
                // wlan/eth 接口额外加分
                if (strncmp(ifa->ifa_name, "wlan", 4) == 0) priority += 20;
                else if (strncmp(ifa->ifa_name, "eth", 3) == 0) priority += 15;
                else if (strncmp(ifa->ifa_name, "en", 2) == 0) priority += 15; // macOS/BSD
                else priority += 5; // 其他接口
            } 
            // 只有 UP 没有 RUNNING 的接口作为备选
            else if (is_up) {
                priority = 50;
                if (strncmp(ifa->ifa_name, "wlan", 4) == 0) priority += 10;
                else if (strncmp(ifa->ifa_name, "eth", 3) == 0) priority += 8;
                else if (strncmp(ifa->ifa_name, "en", 2) == 0) priority += 8;
            }
            // 其他情况优先级很低
            else {
                priority = 10;
            }
            
            // 选择优先级最高的接口
            if (priority > best_priority) {
                best_priority = priority;
                result = host;
            }
            
            // 保存第一个有效IP作为最终备选
            if (fallback_ip.empty() && strcmp(host, "127.0.0.1") != 0) {
                fallback_ip = host;
            }
        }
    }
    
    freeifaddrs(ifaddr);
    
    // 如果没找到合适的，使用备选IP
    if (result == "127.0.0.1" && !fallback_ip.empty()) {
        result = fallback_ip;
    }
    
    return result;
}

/*******************************************************************
 * @brief       关闭服务器socket
 * 
 * @note        线程安全的关闭操作
 ******************************************************************/
void CameraStreamServer::close_server_socket(void)
{
    pthread_mutex_lock(&sock_mutex);
    if (server_sock_fd >= 0) {
        shutdown(server_sock_fd, SHUT_RDWR);
        close(server_sock_fd);
        server_sock_fd = -1;
    }
    pthread_mutex_unlock(&sock_mutex);
}

/*******************************************************************
 * @brief       获取当前时间戳(毫秒)
 * 
 * @return      返回当前时间戳(毫秒)
 ******************************************************************/
uint64_t CameraStreamServer::now_ms(void)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

/*******************************************************************
 * @brief       格式化时间戳
 * 
 * @param       ts_ms           时间戳(毫秒)
 * 
 * @return      返回格式化后的时间字符串
 ******************************************************************/
std::string CameraStreamServer::format_timestamp(uint64_t ts_ms)
{
    if (ts_ms == 0) return "--";
    time_t seconds = static_cast<time_t>(ts_ms / 1000);
    int ms = static_cast<int>(ts_ms % 1000);
    struct tm tm_time;
    localtime_r(&seconds, &tm_time);
    char date_buf[64];
    strftime(date_buf, sizeof(date_buf), "%Y-%m-%d %H:%M:%S", &tm_time);
    char buf[80];
    snprintf(buf, sizeof(buf), "%s.%03d", date_buf, ms);
    return std::string(buf);
}

/*******************************************************************
 * @brief       发送HTTP响应
 * 
 * @param       sock            客户端socket
 * @param       content_type    内容类型
 * @param       body            响应体
 * @param       body_len        响应体长度
 ******************************************************************/
void CameraStreamServer::send_response(int sock, const char* content_type, const char* body, size_t body_len)
{
    std::ostringstream header;
    header << "HTTP/1.1 200 OK\r\n";
    header << "Content-Type: " << content_type << "\r\n";
    header << "Content-Length: " << body_len << "\r\n";
    header << "Connection: close\r\n\r\n";
    std::string h = header.str();
    send(sock, h.c_str(), h.length(), 0);
    send(sock, body, body_len, 0);
}

/*******************************************************************
 * @brief       发送统计信息响应
 * 
 * @param       sock            客户端socket
 ******************************************************************/
void CameraStreamServer::send_stats_response(int sock)
{
    uint64_t capture_ts = latest_capture_ts_ms;
    uint64_t frame_id = 0;
    pthread_mutex_lock(&frame_mutex);
    frame_id = latest_frame_id;
    pthread_mutex_unlock(&frame_mutex);

    uint64_t server_ts = now_ms();
    double fps = ema_fps;
    std::ostringstream body;
    body << std::fixed << std::setprecision(2)
         << "{\"latestFrameId\":" << frame_id
         << ",\"latestCaptureTsMs\":" << capture_ts
         << ",\"serverTsMs\":" << server_ts
         << ",\"estimatedFps\":" << fps << "}";
    std::string json = body.str();
    send_response(sock, "application/json; charset=utf-8", json.c_str(), json.size());
}

/*******************************************************************
 * @brief       发送MJPEG流
 * 
 * @param       sock            客户端socket
 ******************************************************************/
void CameraStreamServer::send_mjpeg_stream(int sock)
{
    const char* header = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
        "Cache-Control: no-cache\r\n"
        "Connection: close\r\n\r\n";
    send(sock, header, strlen(header), 0);
    
    uint64_t last_frame_sent = 0;

    while (running) {
        std::vector<unsigned char> jpeg_copy;

        pthread_mutex_lock(&frame_mutex);
        while (running && latest_frame_id == last_frame_sent) {
            pthread_cond_wait(&frame_cond, &frame_mutex);
        }

        if (!running) {
            pthread_mutex_unlock(&frame_mutex);
            break;
        }

        if (current_jpeg.empty()) {
            pthread_mutex_unlock(&frame_mutex);
            continue;
        }

        jpeg_copy = current_jpeg;
        last_frame_sent = latest_frame_id;
        pthread_mutex_unlock(&frame_mutex);
        
        char boundary[256];
        snprintf(boundary, sizeof(boundary),
                "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %zu\r\n\r\n",
                jpeg_copy.size());
        
        if (send(sock, boundary, strlen(boundary), 0) < 0) break;
        if (send(sock, jpeg_copy.data(), jpeg_copy.size(), 0) < 0) break;
        if (send(sock, "\r\n", 2, 0) < 0) break;
    }
}

/*******************************************************************
 * @brief       处理拍照请求(发送原始高质量图片到客户端)
 * 
 * @param       sock            客户端socket
 * @param       prefix          文件名前缀
 ******************************************************************/
void CameraStreamServer::handle_snapshot_request(int sock, const std::string& prefix)
{
    pthread_mutex_lock(&original_frame_mutex);
    cv::Mat frame_copy = original_frame.clone();
    pthread_mutex_unlock(&original_frame_mutex);
    
    if (frame_copy.empty()) {
        const char* error_html = "<h1>Error</h1><p>没有可用的图像帧</p>";
        send_response(sock, "text/html; charset=utf-8", error_html, strlen(error_html));
        return;
    }
    
    // 编码为PNG无损格式（用于数据集采集）
    std::vector<unsigned char> image_buffer;
    std::vector<int> params;
    params.push_back(cv::IMWRITE_PNG_COMPRESSION);
    params.push_back(3);
    
    if (!cv::imencode(".png", frame_copy, image_buffer, params)) {
        const char* error_html = "<h1>Error</h1><p>图像编码失败</p>";
        send_response(sock, "text/html; charset=utf-8", error_html, strlen(error_html));
        std::cerr << "✗ 图像编码失败" << std::endl;
        return;
    }
    
    // 生成文件名（使用自定义前缀）
    time_t now = time(NULL);
    struct tm tm_time;
    localtime_r(&now, &tm_time);
    char filename[256];
    snprintf(filename, sizeof(filename), "%s_%04d%02d%02d_%02d%02d%02d.png",
             prefix.c_str(),
             tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
             tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
    
    // 发送HTTP响应头
    std::ostringstream header;
    header << "HTTP/1.1 200 OK\r\n";
    header << "Content-Type: image/png\r\n";
    header << "Content-Length: " << image_buffer.size() << "\r\n";
    header << "Content-Disposition: attachment; filename=\"" << filename << "\"\r\n";
    header << "Cache-Control: no-cache\r\n";
    header << "Connection: close\r\n\r\n";
    
    std::string h = header.str();
    send(sock, h.c_str(), h.length(), 0);
    send(sock, image_buffer.data(), image_buffer.size(), 0);
    
    std::cout << "✓ 已发送原始无损图片到客户端: " << filename 
              << " (大小: " << image_buffer.size() / 1024 << " KB, PNG无损)" << std::endl;
}

/*******************************************************************
 * @brief       处理客户端HTTP请求
 * 
 * @param       sock            客户端socket
 ******************************************************************/
void CameraStreamServer::handle_client_request(int sock)
{
    char buffer[4096];
    ssize_t n = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (n <= 0) {
        close(sock);
        return;
    }
    buffer[n] = '\0';
    
    // 解析请求路径
    std::string request(buffer);
    size_t path_start = request.find(" ") + 1;
    size_t path_end = request.find(" ", path_start);
    std::string path = request.substr(path_start, path_end - path_start);
    
    if (path == "/" || path.find("/viewer") == 0 || path.find("/?") == 0) {
        // 返回HTML查看器
        send_response(sock, "text/html; charset=utf-8", viewer_html, strlen(viewer_html));
    } else if (path.find("/stream") == 0) {
        // 返回视频流
        send_mjpeg_stream(sock);
    } else if (path.find("/stats") == 0) {
        send_stats_response(sock);
    } else if (path.find("/snapshot") == 0) {
        // 解析文件名前缀参数
        std::string prefix = "snapshot";  // 默认前缀
        size_t query_pos = path.find("?prefix=");
        if (query_pos != std::string::npos) {
            size_t prefix_start = query_pos + 8;  // "?prefix=" 长度为8
            size_t prefix_end = path.find("&", prefix_start);
            if (prefix_end == std::string::npos) {
                prefix_end = path.length();
            }
            prefix = path.substr(prefix_start, prefix_end - prefix_start);
            
            // URL解码（简单处理，只处理常见字符）
            size_t pos = 0;
            while ((pos = prefix.find("%20", pos)) != std::string::npos) {
                prefix.replace(pos, 3, " ");
                pos += 1;
            }
            
            // 安全检查：只允许字母、数字、下划线、中划线
            bool valid = true;
            for (char c : prefix) {
                if (!isalnum(c) && c != '_' && c != '-') {
                    valid = false;
                    break;
                }
            }
            if (!valid || prefix.empty()) {
                prefix = "snapshot";
            }
        }
        
        // 处理拍照请求
        handle_snapshot_request(sock, prefix);
    } else {
        // 404
        const char* not_found = "<h1>404 Not Found</h1>";
        send_response(sock, "text/html", not_found, strlen(not_found));
    }
    
    close(sock);
}

/*******************************************************************
 * @brief       客户端处理线程函数
 * 
 * @param       arg             客户端socket指针
 * 
 * @return      返回NULL
 ******************************************************************/
void* CameraStreamServer::client_thread_func(void* arg)
{
    int sock = *(int*)arg;
    delete (int*)arg;
    
    if (instance) {
        instance->handle_client_request(sock);
    } else {
        close(sock);
    }
    
    return NULL;
}

/*******************************************************************
 * @brief       服务器线程函数
 * 
 * @param       arg             CameraStreamServer实例指针
 * 
 * @return      返回NULL
 ******************************************************************/
void* CameraStreamServer::server_thread_func(void* arg)
{
    CameraStreamServer* server = static_cast<CameraStreamServer*>(arg);
    if (!server) return NULL;
    
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        std::cerr << "创建socket失败" << std::endl;
        return NULL;
    }

    pthread_mutex_lock(&server->sock_mutex);
    server->server_sock_fd = server_sock;
    pthread_mutex_unlock(&server->sock_mutex);
    
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(server->server_port);
    
    if (bind(server_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "绑定端口失败" << std::endl;
        close(server_sock);
        return NULL;
    }
    
    if (listen(server_sock, 10) < 0) {
        std::cerr << "监听失败" << std::endl;
        close(server_sock);
        return NULL;
    }
    
    // 获取本机IP地址
    std::string local_ip = server->get_local_ip();
    
    std::cout << "\n======================================" << std::endl;
    std::cout << "📡 MJPEG摄像头图传服务器启动成功!" << std::endl;
    std::cout << "======================================" << std::endl;
    std::cout << "监听端口: " << server->server_port << std::endl;
    std::cout << "本机IP: " << local_ip << std::endl;
    std::cout << "请在浏览器访问: http://" << local_ip << ":" << server->server_port << std::endl;
    std::cout << "======================================\n" << std::endl;
    
    while (server->running) {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &len);
        
        if (client_sock < 0) {
            if (!server->running) break;
            continue;
        }
        
        // 优化socket选项
        int flag = 1;
        setsockopt(client_sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
        
        // 设置发送缓冲区大小
        int sndbuf = 65536;
        setsockopt(client_sock, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf));

        pthread_t tid;
        int* sock_ptr = new int(client_sock);
        pthread_create(&tid, NULL, client_thread_func, sock_ptr);
        pthread_detach(tid);
    }
    
    server->close_server_socket();
    return NULL;
}

/*******************************************************************
 * @brief       信号处理函数
 * 
 * @param       sig             信号值
 ******************************************************************/
void CameraStreamServer::signal_handler(int sig)
{
    std::cout << "\n正在关闭服务器..." << std::endl;
    if (instance) {
        instance->stop_server();
    }
}

/*******************************************************************
 * @brief       启动摄像头图传服务器
 * 
 * @param       port            服务器监听端口(默认8080)
 * 
 * @return      返回启动状态
 * @retval      0               启动成功
 * @retval      -1              启动失败
 * 
 * @example     //启动摄像头图传服务器
 *              if(camera_server.start_server(8080) < 0) {
 *                  return -1;
 *              }
 * 
 * @note        在后台线程中启动HTTP服务器，支持浏览器访问
 *              访问 http://<开发板IP>:<port> 即可查看实时画面
 ******************************************************************/
int CameraStreamServer::start_server(int port)
{
    if (running) {
        std::cout << "服务器已经在运行中" << std::endl;
        return 0;
    }
    
    server_port = port;
    running = true;
    latest_frame_id = 0;
    current_jpeg.clear();
    
    // 设置全局实例指针(用于信号处理)
    instance = this;
    
    // 注册信号处理函数
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    if (pthread_create(&server_thread_id, NULL, server_thread_func, this) != 0) {
        std::cerr << "创建服务器线程失败" << std::endl;
        running = false;
        return -1;
    }
    pthread_detach(server_thread_id);
    
    std::cout << "摄像头图传服务器启动中..." << std::endl;
    return 0;
}

/*******************************************************************
 * @brief       更新摄像头帧数据
 * 
 * @param       frame           OpenCV Mat格式的图像帧
 * 
 * @example     camera_server.update_frame(frame);
 * 
 * @note        将最新的摄像头帧推送到服务器，供客户端获取
 *              自动编码为JPEG格式并计算帧率
 ******************************************************************/
void CameraStreamServer::update_frame(const cv::Mat& frame)
{
    if (frame.empty()) return;

    uint64_t capture_ts_ms = now_ms();
    
    // 保存原始帧（用于高质量拍照）
    pthread_mutex_lock(&original_frame_mutex);
    original_frame = frame.clone();
    pthread_mutex_unlock(&original_frame_mutex);
    
    // 计算FPS
    static uint64_t last_capture_ts_local = 0;
    static double local_fps_estimate = 0.0;
    if (last_capture_ts_local != 0) {
        uint64_t delta = capture_ts_ms - last_capture_ts_local;
        if (delta > 0) {
            double instant_fps = 1000.0 / static_cast<double>(delta);
            if (local_fps_estimate <= 0.0) {
                local_fps_estimate = instant_fps;
            } else {
                local_fps_estimate = 0.85 * local_fps_estimate + 0.15 * instant_fps;
            }
            ema_fps = local_fps_estimate;
        }
    }
    last_capture_ts_local = capture_ts_ms;

    // 编码为JPEG（低质量，用于图传）
    std::vector<unsigned char> jpeg_buffer;
    std::vector<int> params;
    params.push_back(cv::IMWRITE_JPEG_QUALITY);
    params.push_back(90); // 质量30(低质量，适合图传)
    
    if (cv::imencode(".jpg", frame, jpeg_buffer, params)) {
        latest_capture_ts_ms = capture_ts_ms;
        pthread_mutex_lock(&frame_mutex);
        current_jpeg.swap(jpeg_buffer);
        ++latest_frame_id;
        pthread_cond_broadcast(&frame_cond);
        pthread_mutex_unlock(&frame_mutex);
    }
}

/*******************************************************************
 * @brief       停止摄像头图传服务器
 * 
 * @example     camera_server.stop_server();
 * 
 * @note        停止服务器并释放所有资源
 ******************************************************************/
void CameraStreamServer::stop_server(void)
{
    if (!running) return;
    
    std::cout << "正在停止摄像头图传服务器..." << std::endl;
    running = false;
    close_server_socket();
    pthread_cond_broadcast(&frame_cond);
    
    // 清空实例指针
    if (instance == this) {
        instance = nullptr;
    }
    
    std::cout << "摄像头图传服务器已停止" << std::endl;
}

/*******************************************************************
 * @brief       检查服务器是否正在运行
 * 
 * @return      返回服务器运行状态
 * @retval      true            服务器正在运行
 * @retval      false           服务器已停止
 * 
 * @example     if(camera_server.is_running()) {
 *                  //服务器正在运行
 *              }
 ******************************************************************/
bool CameraStreamServer::is_running(void)
{
    return running;
}

int start_data_server(int port)
{
    return g_data_server.start_server(port);
}

void stop_data_server(void)
{
    g_data_server.stop_server();
}

bool is_data_server_running(void)
{
    return g_data_server.is_running();
}

void upload_data_channels(const char* prefix, const float* channels, size_t channel_count)
{
    g_data_server.upload_channels(prefix, channels, channel_count);
}
