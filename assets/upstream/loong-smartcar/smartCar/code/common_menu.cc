#include "common_menu.h"

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     向父节点注册文件夹
// 参数说明		father		父节点	
// 参数说明		me          文件夹
// 参数说明		name        文件夹名称
// 返回参数     void
// 使用示例     create_Menu_Folder(&myMenu, &folder1, "folder1");
// 备注信息
//-------------------------------------------------------------------------------------------------------------------
void create_Menu_Folder(Folder_Menu *father, Folder_Menu *me, const char *name)
{
    //无法向非文件夹文件注册子文件
    if(father->kind != Normal_Folder) {
        return ;
    }
    //文件属性初始化
    me->name = name;
    me->sons_Count = 0;
    me->kind = Normal_Folder;
    //文件链表初始化
    me->next_brother = NULL;
    me->last_brother = NULL;
    me->son_first = NULL;
    me->father = father;
    father->sons_Count ++;

    uint8_t No = 1;
    //添加新节点
    if(father->sons_Count == 1)
    {
        father->son_first = me;
        me->No = No;
    }
    else
    {
        No++;
        Folder_Menu *p = father->son_first;
        while(p->next_brother != NULL) {
            p = p->next_brother;
            No++;
        }
        //双向添加
        me->last_brother = p;
        me->No = No;
        p->next_brother = me;
    }
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     向父节点注册复选框
// 参数说明		father		父节点	
// 参数说明		me          复选框
// 参数说明		name        复选框名称
// 参数说明		bool        复选框指向的变量
// 返回参数     void
// 使用示例     create_Mune_CheckBox(&myMenu, &box, "box1", &box1);
// 备注信息
//-------------------------------------------------------------------------------------------------------------------
void create_Menu_CheckBox(Folder_Menu *father, Folder_Menu *me, const char *name, void *check)
{
    //无法向非文件夹文件注册子文件
    if(father->kind != Normal_Folder) {
        return ;
    }
    //文件属性初始化
    me->name = name;
    me->sons_Count = 0;
    me->kind = bool_Box;
    me->private_data = check;
    //文件链表初始化
    me->next_brother = NULL;
    me->last_brother = NULL;
    me->son_first = NULL;
    me->father = father;
    father->sons_Count ++;

    uint8_t No = 1;
    //添加新节点
    if(father->sons_Count == 1)
    {
        father->son_first = me;
        me->No = No;
    }
    else
    {
        No++;
        Folder_Menu *p = father->son_first;
        while(p->next_brother != NULL) {
            p = p->next_brother;
            No++;
        }
        //双向添加
        me->last_brother = p;
        me->No = No;
        p->next_brother = me;
    }
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     向父节点注册数值框
// 参数说明		father		父节点	
// 参数说明		me          数值框
// 参数说明		name        数值框名称
// 参数说明		number      数值框指向的变量(必须定义为int32_t 不然读取时格式错误)
// 返回参数     void
// 使用示例     create_Menu_NumberBox(&myMenu, &numbox1, "numbox1", &num1);
// 备注信息
//-------------------------------------------------------------------------------------------------------------------
void create_Menu_NumberBox(Folder_Menu *father, Folder_Menu *me, const char *name, void *number)
{
    //无法向非文件夹文件注册子文件
    if(father->kind != Normal_Folder) {
        return ;
    }
    //文件属性初始化
    me->name = name;
    me->sons_Count = 0;
    me->kind = int_Box;
    me->private_data = number;
    me->number_box_select = 0;
    //文件链表初始化
    me->next_brother = NULL;
    me->last_brother = NULL;
    me->son_first = NULL;
    me->father = father;
    father->sons_Count ++;

    uint8_t No = 1;
    //添加新节点
    if(father->sons_Count == 1)
    {
        father->son_first = me;
        me->No = No;
    }
    else
    {
        No++;
        Folder_Menu *p = father->son_first;
        while(p->next_brother != NULL) {
            p = p->next_brother;
            No++;
        }
        //双向添加
        me->last_brother = p;
        me->No = No;
        p->next_brother = me;
    }
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     向父节点注册浮点型
// 参数说明		father		父节点	
// 参数说明		me          数值框
// 参数说明		name        数值框名称
// 参数说明		float_number数值框指向的变量
// 返回参数     void
// 使用示例     create_Menu_FloatBox(&myMenu, &numbox1, "numbox1", &num1);
// 备注信息
//-------------------------------------------------------------------------------------------------------------------
void create_Menu_FloatBox(Folder_Menu *father, Folder_Menu *me, const char *name, void *float_number)
{
    //无法向非文件夹文件注册子文件
    if(father->kind != Normal_Folder) {
        return ;
    }
    //文件属性初始化
    me->name = name;
    me->sons_Count = 0;
    me->kind = float_Box;
    me->private_data = float_number;
    me->number_box_select = 0;
    //文件链表初始化
    me->next_brother = NULL;
    me->last_brother = NULL;
    me->son_first = NULL;
    me->father = father;
    father->sons_Count ++;

    uint8_t No = 1;
    //添加新节点
    if(father->sons_Count == 1)
    {
        father->son_first = me;
        me->No = No;
    }
    else
    {
        No++;
        Folder_Menu *p = father->son_first;
        while(p->next_brother != NULL) {
            p = p->next_brother;
            No++;
        }
        //双向添加
        me->last_brother = p;
        me->No = No;
        p->next_brother = me;
    }
}

/*----------------------
/   所有子项目初始化
----------------------*/
void All_Folder_Menu_Init(Folder_Menu *Menu)
{
    if(Menu->son_first == NULL) {
        return ;
    }
    Folder_Menu *hp = Menu->son_first;
    Folder_Menu *p = Menu->son_first;

    if(hp->next_brother == NULL) {
        All_Folder_Menu_Init(p);
    }
    while(p->next_brother != NULL) {
        if(p->kind == Normal_Folder) {
            All_Folder_Menu_Init(p);
        }
        p = p->next_brother;
    }
    if(hp->next_brother != NULL) {
        All_Folder_Menu_Init(p);
    }

    p->next_brother = hp;
    hp->last_brother = p;
}