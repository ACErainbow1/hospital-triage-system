/* ==========================================================
   main.cpp  --  程序入口
   先读数据文件，再打开图形界面
   作者：张健维   24070213
   ========================================================== */
#include <stdio.h>
#include <time.h>
#include "structs.h"

int main()
{
    printf("==========================================\n");
    printf("  医院门诊预约与智能导诊系统\n");
    printf("  数据结构课程设计   张健维  24070213\n");
    printf("==========================================\n\n");

    printf("正在读取数据文件 ...\n");
    if (!loadData()) {
        printf("数据读取失败，请检查 data 目录下的文件是否存在。\n");
        printf("按回车键退出 ...");
        getchar();
        return 1;
    }

    printf("读取完成：\n");
    printf("  科室 %d 个\n", deptNum);
    printf("  医生 %d 人\n", doctorNum);
    printf("  排班 %d 条\n", scheduleNum);
    printf("  患者 %d 人\n", patientNum);
    printf("  预约 %d 条\n", apptNum);
    printf("  评价 %d 条\n", evalNum);
    printf("  症状关联 %d 条\n", symptomNum);
    printf("  平面图节点 %d 个\n", nodeNum);
    printf("\n今天是 %s（%s）\n", todayStr, weekName(weekDay(todayNum)));

    /* 默认用第一个患者登录，方便演示 */
    if (patientNum > 0) {
        curPatient = 0;
        printf("默认登录患者：%s（%s）\n", patients[0].name, patients[0].id);
    }

    /* 把数据里"已签到"的预约放进候诊队列 */
    initQueue();
    printf("候诊队列里现在有 %d 人\n", countWaiting());

    printf("\n正在打开图形界面窗口 ...\n");
    uiMain();

    printf("程序结束。\n");
    return 0;
}
