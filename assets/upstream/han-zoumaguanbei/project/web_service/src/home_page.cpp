#include "web_pages.h"

namespace web_pages {

std::string home_html() {
    return std::string(R"HTML(<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">)HTML") + common_style() + R"HTML(</head><body><div class="top"><nav class="nav"><span class="brand">Vision Console</span></nav></div><main class="wrap"><h1>选择服务</h1><p class="muted">进入需要使用的设备服务。</p><div class="services"><a class="service" href="/dataset-capture"><strong>数据集拍摄</strong><span>启动实时图像处理，拍摄并管理模型训练数据。</span><b>进入数据集拍摄 →</b></a><a class="service" href="/model-test"><strong>模型识别测试</strong><span>批量测试分类数据集，查看正确率、错误数量和结果分布。</span><b>进入模型测试 →</b></a><a class="service" href="/camera-settings"><strong>摄像头参数调整</strong><span>调整曝光模式、曝光时间和自动曝光优先策略，并实时预览原始画面。</span><b>进入摄像头设置 →</b></a></div><div style="margin-top:24px"><button class="danger" onclick="stopServer()">退出网页服务</button></div><script>async function stopServer(){await fetch('/api/stop',{method:'POST'});document.body.innerHTML='<main class="wrap"><section class="panel"><h2>网页服务已退出</h2><p class="muted">网页服务已停止，可以关闭此页面。</p></section></main>'}</script></main></body></html>)HTML";
}

} // namespace web_pages
