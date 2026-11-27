import math
from pathlib import Path

GRID_SIZE = 75
ANGLES = (-90.0, -67.5, -45.0, -22.5, 0.0, 22.5, 45.0, 67.5, 90.0)


def angle_tag(angle_deg):
    if angle_deg == 0:
        return "0"
    sign = "p" if angle_deg > 0 else "n"
    value = f"{abs(angle_deg):g}".replace(".", "_")
    return sign + value


def generate_rotation_mapping(angle_deg):
    """生成反向采样偏移表。

    图像坐标中 x 向右、y 向下，因此这里的正角在屏幕上是顺时针。
    cut_image 对 angle_barrier_save 取最近的同号表：正角选 p 表，负角选 n 表。
    """
    center = (GRID_SIZE - 1) / 2.0
    rad = math.radians(angle_deg)
    cos_a = math.cos(rad)
    sin_a = math.sin(rad)
    dx_array = []
    dy_array = []

    for y in range(GRID_SIZE):
        dx_row = []
        dy_row = []
        for x in range(GRID_SIZE):
            rx = x - center
            ry = y - center
            src_rx = rx * cos_a - ry * sin_a
            src_ry = rx * sin_a + ry * cos_a
            dx_row.append(round(center + src_rx) - x)
            dy_row.append(round(center + src_ry) - y)
        dx_array.append(dx_row)
        dy_array.append(dy_row)

    return dx_array, dy_array


def format_c_array(name, data):
    rows = []
    for row in data:
        rows.append("    {" + ", ".join(f"{value:3d}" for value in row) + "}")
    return (
        f"static const int8_t {name}[{GRID_SIZE}][{GRID_SIZE}] = {{\n"
        + ",\n".join(rows)
        + "\n};"
    )


def validate_mappings():
    """校验角度档位、反向采样方向和最大残余角。"""
    # 输出图顶部中心经正角表应从源图右上方采样；这会把源图中
    # 顺时针倾斜的内容逆时针校正回来。负角的采样方向应完全相反。
    positive_dx, positive_dy = generate_rotation_mapping(45.0)
    negative_dx, negative_dy = generate_rotation_mapping(-45.0)
    top = 0
    center = GRID_SIZE // 2
    assert positive_dx[top][center] > 0 and positive_dy[top][center] > 0
    assert negative_dx[top][center] < 0 and negative_dy[top][center] > 0

    # cut_image 使用相邻档位的中点作为阈值，[-90,+90] 内最大误差为11.25度。
    for tenth_degree in range(-900, 901):
        angle = tenth_degree / 10.0
        nearest = min(ANGLES, key=lambda table_angle: abs(angle - table_angle))
        assert abs(angle - nearest) <= 11.25


def generate_header(output_path):
    parts = [
        "#ifndef _ROTATE_ARRAY_H_",
        "#define _ROTATE_ARRAY_H_",
        "",
        "#include <stdint.h>",
        "",
        "// 反向采样映射：正角在屏幕图像坐标中表示顺时针旋转。",
        "// 角度：-90 到 +90 度，每 22.5 度一组。",
    ]

    for angle in ANGLES:
        tag = angle_tag(angle)
        dx_array, dy_array = generate_rotation_mapping(angle)
        parts.extend(
            [
                "",
                f"// {angle:+g} degrees",
                format_c_array(f"rotate_75_75_{tag}_x", dx_array),
                "",
                format_c_array(f"rotate_75_75_{tag}_y", dy_array),
            ]
        )

    parts.extend(["", "#endif // _ROTATE_ARRAY_H_", ""])
    output_path.write_text("\n".join(parts), encoding="utf-8")


if __name__ == "__main__":
    validate_mappings()
    project_root = Path(__file__).resolve().parents[1]
    output_path = project_root / "include" / "rotate_array.h"
    generate_header(output_path)
    print(f"已生成 {len(ANGLES)} 组旋转映射表: {output_path}")
