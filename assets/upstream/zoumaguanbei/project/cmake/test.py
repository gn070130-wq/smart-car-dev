import math

def generate_area_table():
    # 基础参数
    total_rows = 240
    # 摄像头底座与垂直方向夹角 40度，假设光轴垂直于底座
    # 那么光轴与垂直方向夹角也为 40度
    tilt_angle_deg = 40.0  
    vfov_deg = 120.0 # 垂直视场角，若已知具体参数请修改
    
    # 存储结果的数组
    # index i 代表图像中的 y = 239 - i
    table = [0] * total_rows
    
    # 计算 y 从 239 递减到 150 (对应 i 从 0 到 89)
    for i in range(240):
        y = 239 - i
        
        if y < 150:
            table[i] = 0
            continue
            
        # 计算该像素点相对于光轴的角度偏移 (弧度)
        # y=239 对应 offset = -VFOV/2, y=0 对应 +VFOV/2 (假设光轴在图像中心)
        # 图像中心 y=119.5
        angle_offset = ((119.5 - y) / 240.0) * math.radians(vfov_deg)
        
        # 该像素点在地面上的入射角 phi
        phi_y = math.radians(tilt_angle_deg) + angle_offset
        
        # 基准点 y = 239 的入射角
        phi_base = math.radians(tilt_angle_deg) + ((119.5 - 239) / 240.0) * math.radians(vfov_deg)
        
        # 相对面积比例公式 (1/cos^3)
        # 避免除以 0
        try:
            ratio = (math.cos(phi_base) / math.cos(phi_y))**3
            # 为了保证 int 精度，我们将比例结果四舍五入
            # 如果需要保留小数，可以考虑存储为 float
            table[i] = int(round(ratio))
        except:
            table[i] = 0

    # 输出 C 语言数组
    print(f"// 摄像头倾角: {tilt_angle_deg}°, VFOV: {vfov_deg}°")
    print(f"// 数组下标 i 对应图像坐标 y = 239 - i")
    print("const int area_weight_table[240] = {")
    for i in range(0, 240, 10):
        line = ", ".join(map(str, table[i:i+10]))
        print(f"    {line},")
    print("};")

if __name__ == "__main__":
    generate_area_table()
