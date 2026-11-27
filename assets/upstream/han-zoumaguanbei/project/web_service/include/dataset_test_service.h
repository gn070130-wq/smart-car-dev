#ifndef DATASET_TEST_SERVICE_H
#define DATASET_TEST_SERVICE_H

#include <opencv2/core.hpp>
#include <string>
#include <vector>

// 数据集全量测试：目录结构为 root/<类别名>/图片。
bool start_dataset_test(const char* dir_path);

// 未分类图片识别：递归扫描指定目录，逐张输出预测结果。
bool start_folder_identification(const char* dir_path);

// 两种任务共用的状态和结果 JSON。
std::string dataset_test_status_json();
bool dataset_test_running();
// 请求停止当前测试，并等待测试线程退出；应在 HTTP 服务线程之外调用。
void stop_dataset_test();
void request_stop_dataset_test();

// 自动扫描模型目录、查询当前模型并切换模型。
std::string model_catalog_json();
bool switch_test_model(const char* model_name, std::string& message);

// 按状态 JSON 中的结果 ID 读取预览图，避免向 HTTP 接口暴露任意文件路径。
bool dataset_test_result_image(int result_id, std::vector<uchar>& encoded_jpeg);

#endif // DATASET_TEST_SERVICE_H
