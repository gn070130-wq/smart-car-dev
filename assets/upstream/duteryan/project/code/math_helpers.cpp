#include  "zf_common_headfile.hpp"
int myabs_Int(int my_value)//myabs_Int
{
    if (my_value < 0)
        my_value = -my_value;
    return my_value;
}

int Get_angle(uint8 ax, uint8 ay, uint8 bx, uint8 by, uint8 cx, uint8 cy)
{
    int16 abx = ax - bx;
    int16 aby = ay - by;
    int16 cbx = cx - bx;
    int16 cby = cy - by;
    double ab_muti_cb = abx * cbx + aby * cby;
    double dist_ab =  sqrtf(abx * abx + aby * aby);
    double dist_cb =  sqrtf(cbx * cbx + cby * cby);
    double cosvalue = ab_muti_cb / (dist_ab * dist_cb);
    return (int)(acosf(cosvalue) * 180 / 3.14);
}
