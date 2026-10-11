/* ==========================================================
   ui.cpp  --  图形界面（用 EasyX 做的）
   一共 8 个页面：首页、智能导诊、科室预约、排班日历、
                 我的预约、就诊路线、候诊队列、通知评价
   界面走极简风格：只用方框和文字，不做动画、渐变和圆角
   作者：张健维   24070213
   ========================================================== */
#include <graphics.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "structs.h"

/* 用到的颜色，一共就这几个 */
#define C_BG     RGB(245, 246, 248)      /* 背景 */
#define C_LINE   RGB(190, 195, 205)      /* 边框 */
#define C_TEXT   RGB(30, 30, 30)         /* 正文 */
#define C_GRAY   RGB(130, 130, 130)      /* 次要文字 */
#define C_BLUE   RGB(45, 105, 180)       /* 主色 */
#define C_RED    RGB(200, 60, 60)        /* 提醒 */
#define C_GREEN  RGB(40, 140, 80)        /* 正常 */

#define WIN_W 1024
#define WIN_H 700

/* 8 个页面的名字 */
const char* pageName[8] = {
    "首页", "智能导诊", "科室预约", "排班日历",
    "我的预约", "就诊路线", "候诊队列", "通知评价"
};

/* ---------------- 界面自己的数据 ---------------- */
int  page   = 0;
int  mouseX = 0, mouseY = 0;
char msgText[200] = "";
int  msgTimer = 0;

char symptomText[200] = "";
int  triDept[8];
double triMatch[8];
int  triCount = 0;

int  selDept = 0, selDoctor = 0, selSchedule = -1, selAppt = -1;
int  calDoctor = 0, calDay = 0;
int  routeFloor = 1, routeEnd = -1, routeLen = 0, routeDist = 0;
int  routePath[80];
int  evalAppt = -1, evalScore = 5;
char evalText[120] = "";

/* ==========================================================
   一、画图用的小函数
   ========================================================== */

/* 设字体。宋体系统自带，不用另外装 */
void setFont(int size)
{
    settextstyle(size, 0, "宋体");
    setbkmode(TRANSPARENT);
}

/* 实心矩形 */
void box(int x, int y, int w, int h, COLORREF c)
{
    setfillcolor(c);
    solidrectangle(x, y, x + w, y + h);
}

/* 边框 */
void frame(int x, int y, int w, int h, COLORREF c)
{
    setlinecolor(c);
    rectangle(x, y, x + w, y + h);
}

/* 写字。字符串里的 \n 会换行（EasyX 的 outtextxy 本身不认换行） */
void put(int x, int y, const char* s, COLORREF c, int size)
{
    char line[300];
    int i = 0, n = 0;

    setFont(size);
    settextcolor(c);

    while (1) {
        if (s[i] == '\n' || s[i] == 0) {
            line[n] = 0;
            outtextxy(x, y, line);
            y += size + 5;
            n = 0;
            if (s[i] == 0) break;
        } else {
            if (n < 299) line[n++] = s[i];
        }
        i++;
    }
}

/* 在方框里居中写一行字 */
void putMid(int x, int y, int w, const char* s, COLORREF c, int size)
{
    setFont(size);
    settextcolor(c);
    outtextxy(x + (w - textwidth(s)) / 2, y, s);
}

/* 鼠标是不是点在某个方框里面 */
int hit(int x, int y, int w, int h)
{
    return mouseX >= x && mouseX <= x + w && mouseY >= y && mouseY <= y + h;
}

/* 画按钮。main 为 1 是蓝色按钮，为 0 是灰色按钮 */
void btn(int x, int y, int w, int h, const char* s, int main)
{
    if (main) {
        box(x, y, w, h, C_BLUE);
        putMid(x, y + (h - 18) / 2, w, s, WHITE, 15);
    } else {
        box(x, y, w, h, RGB(236, 239, 243));
        frame(x, y, w, h, C_LINE);
        putMid(x, y + (h - 18) / 2, w, s, C_TEXT, 15);
    }
}

/* 底部提示，过一会儿自动消失 */
void tip(const char* s)
{
    strcpy(msgText, s);
    msgTimer = 150;
}

/* 页面标题 */
void pageTitle(const char* t, const char* sub)
{
    put(10, 84, t, C_TEXT, 19);
    put(10, 110, sub, C_GRAY, 13);
}

/* ==========================================================
   二、顶部导航条和底部提示条
   ========================================================== */
void drawTop()
{
    box(0, 0, WIN_W, 38, C_BLUE);
    put(10, 9, "医院门诊预约与智能导诊系统", WHITE, 17);

    for (int i = 0; i < 8; i++) {
        int x = 10 + i * 118;
        if (i == page) {
            box(x, 44, 110, 28, C_BLUE);
            putMid(x, 49, 110, pageName[i], WHITE, 14);
        } else {
            box(x, 44, 110, 28, RGB(236, 239, 243));
            frame(x, 44, 110, 28, C_LINE);
            putMid(x, 49, 110, pageName[i], C_TEXT, 14);
        }
    }
}

void drawBottom()
{
    box(0, WIN_H - 32, WIN_W, 32, RGB(236, 239, 243));

    if (msgTimer > 0) put(10, WIN_H - 25, msgText, C_BLUE, 14);
    else              put(10, WIN_H - 25, "提示信息会显示在这里，按 Esc 退出", C_GRAY, 14);

    if (curPatient >= 0) {
        char s[60];
        sprintf(s, "当前患者：%s", patients[curPatient].name);
        put(WIN_W - 140, WIN_H - 25, s, C_TEXT, 14);
    }
}

/* ==========================================================
   三、首页
   ========================================================== */
void drawHome()
{
    char s[120];

    pageTitle("欢迎使用", "数据结构课程设计 —— 医院门诊预约与智能导诊系统");

    sprintf(s, "科室 %d 个      医生 %d 人      排班 %d 条      正在候诊 %d 人",
            deptNum, doctorNum, scheduleNum, countWaiting());
    put(10, 140, s, C_BLUE, 17);

    put(10, 185, "系统功能：", C_TEXT, 15);
    put(20, 215, "1. 智能导诊：说出哪里不舒服，推荐该挂哪个科", C_TEXT, 14);
    put(20, 243, "2. 科室预约：按科室选医生，选时段挂号", C_TEXT, 14);
    put(20, 271, "3. 排班日历：一周七天显示医生的出诊安排", C_TEXT, 14);
    put(20, 299, "4. 我的预约：查看、取消、签到、看预约凭证", C_TEXT, 14);
    put(20, 327, "5. 就诊路线：算出从大门到诊室怎么走最近", C_TEXT, 14);
    put(20, 355, "6. 候诊队列：签到后排队，叫号、过号、完成就诊", C_TEXT, 14);

    btn(10, 400, 120, 34, "切换患者", 0);
    btn(140, 400, 120, 34, "开始导诊", 1);

    sprintf(s, "您有 %d 条通知消息", noticeNum);
    put(10, 455, s, C_RED, 14);
}

void clickHome(int mx, int my)
{
    if (hit(140, 400, 120, 34)) { page = 1; return; }

    if (hit(10, 400, 120, 34)) {
        char buf[20];
        buf[0] = 0;
        if (InputBox(buf, 18, "请输入患者编号（例如 P01）：", "切换患者", "", 320, 180)) {
            int i = findPatient(buf);
            if (i >= 0) { curPatient = i; tip("登录成功"); }
            else tip("没有找到这个患者");
        }
    }
}

/* ==========================================================
   四、智能导诊
   ========================================================== */
void drawTriage()
{
    char s[220];

    pageTitle("智能导诊", "输入症状，系统按「症状—科室」的关联程度推荐科室");

    /* 输入框 */
    box(10, 140, 700, 34, WHITE);
    frame(10, 140, 700, 34, C_LINE);
    if (symptomText[0] == 0)
        put(18, 149, "点这里输入症状，例如：最近胸闷心悸气短", C_GRAY, 14);
    else
        put(18, 149, symptomText, C_TEXT, 14);

    btn(720, 140, 100, 34, "开始导诊", 1);
    btn(830, 140, 80, 34, "清空", 0);

    /* 常用症状，点一下填进去。同一个词在文件里会出现多次，只画第一次 */
    put(10, 186, "常用症状：", C_GRAY, 13);
    int x = 90;
    for (int i = 0; i < symptomNum && x < 900; i++) {
        int repeat = 0;
        for (int j = 0; j < i; j++)
            if (strcmp(symptoms[j].word, symptoms[i].word) == 0) repeat = 1;
        if (repeat) continue;

        btn(x, 182, 76, 24, symptoms[i].word, 0);
        x += 84;
    }

    /* 推荐结果 */
    if (triCount == 0) {
        put(10, 250, "还没有推荐结果，输入症状后点「开始导诊」", C_GRAY, 15);
        return;
    }

    put(10, 236, "推荐结果（按匹配度从高到低）：", C_TEXT, 15);
    for (int i = 0; i < triCount; i++) {
        int d = triDept[i];
        sprintf(s, "%d. %s      匹配度 %.1f%%      %s      %d 位医生      未来 3 天余号 %d",
                i + 1, depts[d].name, triMatch[i], depts[d].place,
                deptDoctorNum(d), deptRemain3(d));
        put(20, 266 + i * 32, s, i == 0 ? C_BLUE : C_TEXT, 15);
    }

    btn(10, 540, 230, 34, "该科室没号？推荐别的科室", 0);
}

void clickTriage(int mx, int my)
{
    char buf[200];

    if (hit(10, 140, 700, 34)) {
        strcpy(buf, symptomText);
        if (InputBox(buf, 190, "请描述一下哪里不舒服：", "智能导诊", symptomText, 460, 200))
            strcpy(symptomText, buf);
        return;
    }
    if (hit(830, 140, 80, 34)) { symptomText[0] = 0; triCount = 0; return; }

    if (hit(720, 140, 100, 34)) {
        if (symptomText[0] == 0) { tip("请先输入症状"); return; }
        triCount = doTriage(symptomText, triDept, triMatch, 8);
        if (triCount == 0) tip("没能识别出症状，换一种说法试试");
        else { tip("导诊完成"); selDept = triDept[0]; }
        return;
    }

    /* 常用症状按钮 */
    int x = 90;
    for (int i = 0; i < symptomNum && x < 900; i++) {
        int repeat = 0;
        for (int j = 0; j < i; j++)
            if (strcmp(symptoms[j].word, symptoms[i].word) == 0) repeat = 1;
        if (repeat) continue;
        if (hit(x, 182, 76, 24)) {
            if (strlen(symptomText) + strlen(symptoms[i].word) + 3 < 190) {
                if (symptomText[0] != 0) strcat(symptomText, "，");
                strcat(symptomText, symptoms[i].word);
            }
            return;
        }
        x += 84;
    }

    /* 转诊推荐 */
    if (hit(10, 540, 230, 34) && triCount > 0) {
        int outDept[8];
        double outMatch[8];
        int n = recommendTransfer(triDept[0], outDept, outMatch, 3);
        if (n == 0) { tip("没有号源充足的相近科室"); return; }
        sprintf(msgText, "推荐去：%s（%s）", depts[outDept[0]].name, depts[outDept[0]].place);
        msgTimer = 150;
        selDept = outDept[0];
    }
}

/* ==========================================================
   五、科室预约
   ========================================================== */
/* 查某个医生某天某时段的排班，找不到返回 -1 */
int findScheduleOf(int docIdx, const char* date, int period)
{
    if (docIdx < 0) return -1;
    for (int i = 0; i < scheduleNum; i++)
        if (strcmp(schedules[i].doctorId, doctors[docIdx].id) == 0 &&
            strcmp(schedules[i].date, date) == 0 &&
            schedules[i].period == period)
            return i;
    return -1;
}

void drawBook()
{
    char s[220];

    pageTitle("科室预约", "先点科室，再点医生，然后点一个时段，最后挂号");

    /* 左边：科室 */
    put(10, 132, "科室", C_TEXT, 14);
    for (int i = 0; i < deptNum; i++) {
        int y = 158 + i * 28;
        if (i == selDept) box(10, y, 180, 26, RGB(215, 228, 245));
        put(16, y + 4, depts[i].name, i == selDept ? C_BLUE : C_TEXT, 14);
    }

    /* 右上：医生 */
    put(210, 132, "该科室的医生", C_TEXT, 14);
    int y = 158;
    for (int i = 0; i < doctorNum; i++) {
        if (strcmp(doctors[i].deptId, depts[selDept].id) != 0) continue;
        if (i == selDoctor) box(210, y, 800, 30, RGB(215, 228, 245));
        sprintf(s, "%s    %s    %s    挂号费 %.0f 元",
                doctors[i].name, doctors[i].title, doctors[i].good, doctors[i].fee);
        put(216, y + 7, s, i == selDoctor ? C_BLUE : C_TEXT, 14);
        y += 34;
    }

    /* 右下：可预约时段 */
    put(210, 280, "可以预约的时段（点一下选中）：", C_TEXT, 14);
    int x = 210, yy = 308;
    for (int d = 0; d < 7; d++) {
        char date[LEN_ID];
        dateAdd(todayNum, d, date);
        for (int p = 1; p <= 2; p++) {
            int si = findScheduleOf(selDoctor, date, p);
            if (si < 0) continue;
            if (schedules[si].status != 0) continue;
            if (yy > 505) break;

            sprintf(s, "%s %s  %d/%d", &date[5], (p == 1 ? "上午" : "下午"),
                    remainSeat(si), schedules[si].maxNum);
            box(x, yy, 180, 30, si == selSchedule ? RGB(215, 228, 245) : RGB(242, 244, 247));
            frame(x, yy, 180, 30, C_LINE);
            putMid(x, yy + 7, 180, s, remainSeat(si) == 0 ? C_RED : C_TEXT, 13);

            x += 190;
            if (x > 830) { x = 210; yy += 36; }
        }
    }

    btn(210, 560, 130, 34, "确认挂号", 1);
    btn(350, 560, 170, 34, "看这位医生排班", 0);
    put(545, 570, "要先选好医生和时段", C_GRAY, 13);
}

void clickBook(int mx, int my)
{
    /* 点科室 */
    for (int i = 0; i < deptNum; i++) {
        if (!hit(10, 158 + i * 28, 180, 26)) continue;
        selDept = i;
        selSchedule = -1;
        for (int k = 0; k < doctorNum; k++)
            if (strcmp(doctors[k].deptId, depts[i].id) == 0) { selDoctor = k; break; }
        return;
    }

    /* 点医生 */
    int y = 158;
    for (int i = 0; i < doctorNum; i++) {
        if (strcmp(doctors[i].deptId, depts[selDept].id) != 0) continue;
        if (hit(210, y, 800, 30)) { selDoctor = i; selSchedule = -1; return; }
        y += 34;
    }

    /* 点时段 */
    int x = 210, yy = 308;
    for (int d = 0; d < 7; d++) {
        char date[LEN_ID];
        dateAdd(todayNum, d, date);
        for (int p = 1; p <= 2; p++) {
            int si = findScheduleOf(selDoctor, date, p);
            if (si < 0) continue;
            if (schedules[si].status != 0) continue;
            if (yy > 505) break;

            if (hit(x, yy, 180, 30)) {
                selSchedule = si;
                if (remainSeat(si) == 0) tip("这个时段已经约满，换一个");
                return;
            }
            x += 190;
            if (x > 830) { x = 210; yy += 36; }
        }
    }

    /* 挂号 */
    if (hit(210, 560, 130, 34)) {
        if (curPatient < 0) { tip("请先到首页切换患者"); return; }
        if (selSchedule < 0) { tip("请先选一个时段"); return; }

        int r = bookAppoint(curPatient, selSchedule);
        if (r == OK)              tip("挂号成功，到「我的预约」可以看凭证");
        else if (r == ERR_STOP)   tip("这个时段医生停诊了");
        else if (r == ERR_FULL)   tip("号已经满了，换一个时段");
        else if (r == ERR_REPEAT) tip("您已经约过这个时段了");
        else if (r == ERR_OLD)    tip("这个日期已经过去了");
        else                      tip("挂号失败");
    }

    if (hit(350, 560, 170, 34)) { calDoctor = selDoctor; page = 3; }
}

/* ==========================================================
   六、排班日历
   ========================================================== */
void drawCalendar()
{
    char s[120];

    pageTitle("排班日历", "点格子可以直接挂号：绿色还有号，红色约满，灰色停诊");

    btn(10, 130, 70, 28, "上一位", 0);
    btn(88, 130, 70, 28, "下一位", 0);
    sprintf(s, "医生：%s（%s）", doctors[calDoctor].name, doctors[calDoctor].title);
    put(180, 137, s, C_TEXT, 15);
    btn(860, 130, 70, 28, "上一周", 0);
    btn(936, 130, 70, 28, "下一周", 0);

    /* 第一行：星期和日期 */
    int y0 = 178;
    put(20, y0 + 6, "时段", C_TEXT, 13);
    for (int d = 0; d < 7; d++) {
        char date[LEN_ID];
        dateAdd(todayNum, calDay * 7 + d, date);
        sprintf(s, "%s %s", weekName(weekDay(addDays(todayNum, calDay * 7 + d))), &date[5]);
        putMid(90 + d * 132, y0 + 6, 124, s, C_TEXT, 13);
    }

    /* 第二、三行：上午和下午 */
    const char* pr[2] = {"上午", "下午"};
    for (int p = 0; p < 2; p++) {
        int y = y0 + 30 + p * 110;
        put(20, y + 44, pr[p], C_TEXT, 15);

        for (int d = 0; d < 7; d++) {
            char date[LEN_ID];
            dateAdd(todayNum, calDay * 7 + d, date);

            int x = 90 + d * 132;
            int si = findScheduleOf(calDoctor, date, p + 1);

            if (si < 0) {                              /* 这天不上班 */
                box(x, y, 124, 100, RGB(240, 240, 240));
                frame(x, y, 124, 100, C_LINE);
                putMid(x, y + 40, 124, "休诊", C_GRAY, 14);
            } else if (schedules[si].status == 1) {    /* 医生停诊 */
                box(x, y, 124, 100, RGB(240, 240, 240));
                frame(x, y, 124, 100, C_LINE);
                putMid(x, y + 40, 124, "停诊", C_RED, 14);
            } else {
                int r = remainSeat(si);
                box(x, y, 124, 100, r == 0 ? RGB(250, 234, 234) : RGB(233, 245, 236));
                frame(x, y, 124, 100, C_LINE);
                sprintf(s, "%d/%d", r, schedules[si].maxNum);
                putMid(x, y + 22, 124, s, r == 0 ? C_RED : C_GREEN, 22);
                putMid(x, y + 58, 124, "余号", C_GRAY, 12);
            }
        }
    }
}

void clickCalendar(int mx, int my)
{
    if (hit(10, 130, 70, 28))  { calDoctor--; if (calDoctor < 0) calDoctor = doctorNum - 1; return; }
    if (hit(88, 130, 70, 28))  { calDoctor++; if (calDoctor >= doctorNum) calDoctor = 0; return; }
    if (hit(860, 130, 70, 28)) { calDay--; return; }
    if (hit(936, 130, 70, 28)) { calDay++; return; }

    int y0 = 178;
    for (int p = 0; p < 2; p++) {
        int y = y0 + 30 + p * 110;
        for (int d = 0; d < 7; d++) {
            if (!hit(90 + d * 132, y, 124, 100)) continue;

            char date[LEN_ID];
            dateAdd(todayNum, calDay * 7 + d, date);
            int si = findScheduleOf(calDoctor, date, p + 1);

            if (si < 0)                     { tip("这天医生不上班"); return; }
            if (schedules[si].status == 1)  { tip("这个时段医生停诊"); return; }
            if (remainSeat(si) == 0)        { tip("这个时段已经约满"); return; }
            if (curPatient < 0)             { tip("请先到首页切换患者"); return; }

            selDoctor = calDoctor;
            selSchedule = si;
            int r = bookAppoint(curPatient, si);
            if (r == OK)              tip("挂号成功，到「我的预约」可以看凭证");
            else if (r == ERR_REPEAT) tip("您已经约过这个时段了");
            else                      tip("挂号失败，换一个时段");
            return;
        }
    }
}

/* ==========================================================
   七、我的预约
   ========================================================== */
/* 预约状态转成中文 */
const char* statusName(int st)
{
    if (st == 0) return "已预约";
    if (st == 1) return "已签到";
    if (st == 2) return "就诊中";
    if (st == 3) return "已完成";
    if (st == 4) return "已取消";
    if (st == 5) return "爽约";
    return "停诊退号";
}

void drawMyAppt()
{
    char s[220];

    pageTitle("我的预约", "选中一条记录，可以看凭证、签到、取消、评价");

    put(10, 132, "预约记录", C_TEXT, 14);
    if (curPatient < 0) {
        put(20, 190, "还没有登录患者，请到首页切换患者", C_GRAY, 15);
        return;
    }

    int y = 160, n = 0;
    for (int i = 0; i < apptNum && y < 530; i++) {
        if (strcmp(appts[i].patientId, patients[curPatient].id) != 0) continue;

        int si = findSchedule(appts[i].scheduleId);
        int di = findDoctor(appts[i].doctorId);

        if (i == selAppt) box(10, y, 610, 32, RGB(215, 228, 245));
        sprintf(s, "%s %s    %s    第 %d 号    %s",
                (si >= 0 ? schedules[si].date : "?"),
                (si >= 0 ? (schedules[si].period == 1 ? "上午" : "下午") : "?"),
                (di >= 0 ? doctors[di].name : "?"),
                appts[i].seq, statusName(appts[i].status));
        put(18, y + 7, s, i == selAppt ? C_BLUE : C_TEXT, 14);

        y += 38;
        n++;
    }
    if (n == 0) put(20, 200, "还没有预约记录，去「科室预约」挂个号吧", C_GRAY, 14);

    /* 右边显示凭证 */
    put(640, 132, "预约凭证", C_TEXT, 14);
    if (selAppt >= 0) {
        char buf[700];
        buildReceipt(selAppt, buf, 700);
        put(640, 160, buf, C_TEXT, 13);
    } else {
        put(640, 162, "选中左边一条记录", C_GRAY, 13);
    }

    btn(10, 560, 110, 34, "查看凭证", 0);
    btn(130, 560, 110, 34, "签到候诊", 1);
    btn(250, 560, 110, 34, "取消预约", 0);
    btn(370, 560, 110, 34, "去评价", 0);
}

void clickMyAppt(int mx, int my)
{
    /* 选中一条记录 */
    if (curPatient >= 0) {
        int y = 160;
        for (int i = 0; i < apptNum && y < 530; i++) {
            if (strcmp(appts[i].patientId, patients[curPatient].id) != 0) continue;
            if (hit(10, y, 610, 32)) { selAppt = i; return; }
            y += 38;
        }
    }

    if (hit(10, 560, 110, 34)) {
        if (selAppt < 0) tip("请先选中一条记录");
        else tip("凭证显示在右边");
        return;
    }

    if (hit(130, 560, 110, 34)) {
        if (selAppt < 0) { tip("请先选中一条记录"); return; }
        if (checkIn(selAppt) == 0) { tip("签到成功，已经进入候诊队列"); page = 6; }
        else tip("只有「已预约」的才能签到");
        return;
    }

    if (hit(250, 560, 110, 34)) {
        if (selAppt < 0) { tip("请先选中一条记录"); return; }
        if (cancelAppoint(selAppt) == 0) tip("已取消，号退回去了");
        else tip("这条预约现在不能取消");
        return;
    }

    if (hit(370, 560, 110, 34)) {
        int i = pendingEvalAppt(curPatient);
        if (i < 0) { tip("没有需要评价的就诊记录"); return; }
        evalAppt = i;
        page = 7;
        tip("请在右边打分并提交");
    }
}

/* ==========================================================
   八、就诊路线
   ========================================================== */
void drawRoute()
{
    char s[200];

    pageTitle("就诊路线", "选医生，算出从门诊大门到诊室怎么走最近");

    btn(10, 130, 70, 28, "上一位", 0);
    btn(88, 130, 70, 28, "下一位", 0);
    btn(170, 130, 100, 28, "查询路线", 1);
    sprintf(s, "%s（%s）   诊室：%s", doctors[selDoctor].name,
            doctors[selDoctor].title, nodeName(doctors[selDoctor].roomNode));
    put(290, 137, s, C_TEXT, 14);

    /* 平面图。节点坐标是 0~620 / 0~300 这一套，这里按比例画到方框里 */
    box(10, 170, 500, 380, WHITE);
    frame(10, 170, 500, 380, C_LINE);

    for (int a = 0; a < nodeNum; a++) {
        if (nodes[a].floor != routeFloor) continue;
        for (int b = a + 1; b < nodeNum; b++) {
            if (nodes[b].floor != routeFloor) continue;
            if (graph[nodes[a].id][nodes[b].id] == 0) continue;
            setlinecolor(C_LINE);
            line(30 + nodes[a].x * 440 / 620, 200 + nodes[a].y * 320 / 300,
                 30 + nodes[b].x * 440 / 620, 200 + nodes[b].y * 320 / 300);
        }
    }

    for (int k = 0; k < nodeNum; k++) {
        if (nodes[k].floor != routeFloor) continue;
        int px = 30 + nodes[k].x * 440 / 620;
        int py = 200 + nodes[k].y * 320 / 300;

        if (nodes[k].type == 4)      setfillcolor(C_RED);    /* 大门 */
        else if (nodes[k].type == 1) setfillcolor(C_BLUE);   /* 诊室 */
        else                         setfillcolor(C_GRAY);
        solidcircle(px, py, 4);
        put(px + 6, py - 8, nodes[k].name, C_TEXT, 11);
    }

    /* 找到的路线用红线画粗一点 */
    setlinestyle(PS_SOLID, 3);
    setlinecolor(C_RED);
    for (int i = 1; i < routeLen; i++) {
        int pa = -1, pb = -1;
        for (int k = 0; k < nodeNum; k++) {
            if (nodes[k].id == routePath[i - 1]) pa = k;
            if (nodes[k].id == routePath[i])     pb = k;
        }
        if (pa < 0 || pb < 0) continue;
        if (nodes[pa].floor != nodes[pb].floor) continue;
        line(30 + nodes[pa].x * 440 / 620, 200 + nodes[pa].y * 320 / 300,
             30 + nodes[pb].x * 440 / 620, 200 + nodes[pb].y * 320 / 300);
    }
    setlinestyle(PS_SOLID, 1);

    /* 楼层切换按钮 */
    for (int f = 1; f <= 6; f++) {
        sprintf(s, "%dF", f);
        btn(10 + (f - 1) * 60, 560, 54, 28, s, f == routeFloor ? 1 : 0);
    }

    /* 右边的文字说明 */
    put(530, 178, "路线说明", C_TEXT, 15);
    if (routeLen == 0) {
        put(530, 210, "点上面的「查询路线」", C_GRAY, 14);
    } else {
        char buf[900];
        sprintf(buf, "全程 %d 米，一共 %d 步\n", routeDist, routeLen - 1);
        pathToText(routePath, routeLen, buf + strlen(buf), 800);
        put(530, 206, buf, C_TEXT, 13);
    }
}

void clickRoute(int mx, int my)
{
    if (hit(10, 130, 70, 28)) {
        selDoctor--; if (selDoctor < 0) selDoctor = doctorNum - 1;
        routeLen = 0; return;
    }
    if (hit(88, 130, 70, 28)) {
        selDoctor++; if (selDoctor >= doctorNum) selDoctor = 0;
        routeLen = 0; return;
    }
    if (hit(170, 130, 100, 28)) {
        int start = gateNode();
        int end   = doctors[selDoctor].roomNode;
        if (start < 0) { tip("没有找到门诊大门"); return; }
        if (end < 0)   { tip("这位医生还没有安排诊室"); return; }

        routeDist = shortestPath(start, end, routePath, &routeLen);
        if (routeDist < 0) { routeLen = 0; tip("没有找到路线"); }
        else {
            routeFloor = nodeFloor(start);
            sprintf(msgText, "找到路线，全程 %d 米", routeDist);
            msgTimer = 150;
        }
        return;
    }

    for (int f = 1; f <= 6; f++)
        if (hit(10 + (f - 1) * 60, 560, 54, 28)) { routeFloor = f; return; }
}

/* ==========================================================
   九、候诊队列
   ========================================================== */
/* 这位医生现在有几个人在等 */
int waitingOf(int docIdx)
{
    int n = 0;
    if (docIdx < 0) return 0;
    for (int i = 0; i < queueNum; i++) {
        int ai = queueData[i].apptId;
        if (ai < 0 || ai >= apptNum) continue;
        if (strcmp(appts[ai].doctorId, doctors[docIdx].id) == 0) n++;
    }
    return n;
}

/* 如果当前医生没人在等，就自动换一个有人的医生，免得页面看起来是空的 */
void pickQueueDoctor()
{
    if (waitingOf(selDoctor) > 0) return;
    for (int i = 0; i < queueNum; i++) {
        int ai = queueData[i].apptId;
        if (ai < 0 || ai >= apptNum) continue;
        int di = findDoctor(appts[ai].doctorId);
        if (di >= 0) { selDoctor = di; return; }
    }
}

void drawQueue()
{
    char s[200];

    pageTitle("候诊队列", "签到后进入队列，老年人自动排前面，可以叫号、过号、完成就诊");

    btn(10, 130, 70, 28, "上一位", 0);
    btn(88, 130, 70, 28, "下一位", 0);
    sprintf(s, "医生：%s（%s）    等待中 %d 人",
            doctors[selDoctor].name, doctors[selDoctor].title, waitingOf(selDoctor));
    put(180, 137, s, C_TEXT, 15);

    box(10, 178, 1000, 340, WHITE);
    frame(10, 178, 1000, 340, C_LINE);
    put(22, 188, "等候名单（已经按优先级排好顺序）", C_TEXT, 14);

    int y = 216, n = 0;
    for (int i = 0; i < queueNum && n < 8; i++) {
        int ai = queueData[i].apptId;
        if (ai < 0 || ai >= apptNum) continue;
        if (strcmp(appts[ai].doctorId, doctors[selDoctor].id) != 0) continue;

        int pi = findPatient(appts[ai].patientId);
        sprintf(s, "%d.  %d 号    %s    %s    状态：%s",
                n + 1, appts[ai].seq,
                (pi >= 0 ? patients[pi].name : "患者"),
                (pi >= 0 && patients[pi].age >= 65 ? "老年优先" : "普通"),
                statusName(appts[ai].status));
        put(24, y, s, C_TEXT, 15);

        y += 30;
        n++;
    }
    if (n == 0) put(24, 300, "目前没有人在等这位医生", C_GRAY, 15);

    btn(24, 540, 130, 34, "叫下一位", 1);
    btn(164, 540, 130, 34, "过号", 0);
    btn(304, 540, 130, 34, "完成就诊", 0);
}

void clickQueue(int mx, int my)
{
    if (hit(10, 130, 70, 28)) {
        selDoctor--; if (selDoctor < 0) selDoctor = doctorNum - 1; return;
    }
    if (hit(88, 130, 70, 28)) {
        selDoctor++; if (selDoctor >= doctorNum) selDoctor = 0; return;
    }

    if (hit(24, 540, 130, 34)) {
        char msg[120];
        int ai = callNext(selDoctor, msg, 120);
        tip(msg);
        if (ai >= 0) selAppt = ai;
        return;
    }
    if (hit(164, 540, 130, 34)) {
        if (selAppt < 0) { tip("还没有叫号"); return; }
        if (passNumber(selAppt) == 0) tip("已过号");
        else tip("这条记录现在不能过号");
        return;
    }
    if (hit(304, 540, 130, 34)) {
        if (selAppt < 0) { tip("还没有叫号"); return; }
        if (finishVisit(selAppt) == 0) tip("就诊完成，可以到「通知评价」打分");
        else tip("只有正在就诊的才能点完成");
        return;
    }
}

/* ==========================================================
   十、通知与评价
   ========================================================== */
void drawNotice()
{
    char s[220];

    pageTitle("通知与评价", "医生停诊会自动发通知；看完病可以给医生打分");

    /* 左边：通知 */
    box(10, 132, 600, 400, WHITE);
    frame(10, 132, 600, 400, C_LINE);
    put(22, 142, "我的通知", C_TEXT, 14);

    int y = 172, n = 0;
    if (curPatient >= 0) {
        for (int i = noticeNum - 1; i >= 0 && n < 8; i--) {
            if (strcmp(notices[i].patientId, patients[curPatient].id) != 0) continue;
            put(24, y, notices[i].content, C_TEXT, 13);
            y += 44;
            n++;
        }
    }
    if (n == 0) put(24, 300, "暂时没有通知（医生停诊时才会有）", C_GRAY, 14);

    btn(24, 480, 230, 34, "模拟医生停诊（发通知）", 0);

    /* 右边：评价 */
    box(630, 132, 380, 400, WHITE);
    frame(630, 132, 380, 400, C_LINE);
    put(642, 142, "给医生评价", C_TEXT, 14);

    if (evalAppt < 0) {
        put(642, 300, "先到「我的预约」点「去评价」", C_GRAY, 14);
        return;
    }

    Appointment* a = &appts[evalAppt];
    int di = findDoctor(a->doctorId);

    sprintf(s, "就诊医生：%s（%s）", (di >= 0 ? doctors[di].name : "?"),
            (di >= 0 ? doctors[di].title : "?"));
    put(642, 178, s, C_TEXT, 15);

    put(642, 214, "打分（点星星）：", C_GRAY, 14);
    for (int i = 1; i <= 5; i++)
        put(642 + (i - 1) * 36, 238, "★", i <= evalScore ? C_RED : RGB(200, 200, 200), 26);

    sprintf(s, "%d 分", evalScore);
    put(830, 244, s, C_RED, 16);

    btn(642, 288, 150, 32, "写评价内容", 0);
    btn(806, 288, 150, 32, "提交评价", 1);

    if (evalText[0] != 0) {
        sprintf(s, "内容：%s", evalText);
        put(642, 334, s, C_GREEN, 12);
    }

    put(642, 366, "这位医生收到的评价：", C_TEXT, 13);
    int y2 = 392, m = 0;
    for (int i = 0; i < evalNum && m < 5; i++) {
        if (strcmp(evals[i].doctorId, a->doctorId) != 0) continue;
        sprintf(s, "%d 分  %s", evals[i].score, evals[i].content);
        put(654, y2, s, C_GRAY, 12);
        y2 += 22;
        m++;
    }
    if (m == 0) put(654, y2, "还没有人评价过", C_GRAY, 12);
}

void clickNotice(int mx, int my)
{
    /* 模拟停诊：停掉一条已经有人预约的排班，系统会自动发通知 */
    if (hit(24, 480, 230, 34)) {
        int target = -1;
        for (int i = 0; i < scheduleNum; i++) {
            if (schedules[i].status != 0) continue;
            if (strcmp(schedules[i].doctorId, doctors[selDoctor].id) != 0) continue;
            if (schedules[i].booked <= 0) continue;
            target = i;
            break;
        }
        if (target < 0) { tip("这位医生没有已预约的排班，换一位医生"); return; }

        int n = suspendSchedule(target, "医生临时参加紧急会诊");
        sprintf(msgText, "已停诊，自动通知了 %d 位患者", n);
        msgTimer = 150;
        return;
    }

    if (evalAppt < 0) return;

    /* 点星星 */
    for (int i = 1; i <= 5; i++)
        if (hit(642 + (i - 1) * 36, 234, 32, 34)) { evalScore = i; return; }

    if (hit(642, 288, 150, 32)) {
        char buf[120];
        strcpy(buf, evalText);
        if (InputBox(buf, 110, "说点什么吧（可以留空）：", "评价内容", evalText, 420, 200))
            strcpy(evalText, buf);
        return;
    }

    if (hit(806, 288, 150, 32)) {
        int r = addEvaluation(evalAppt, evalScore, evalText);
        if (r == 0)      { tip("评价提交成功，谢谢反馈"); evalAppt = -1; evalText[0] = 0; }
        else if (r == 2) tip("这次就诊还没完成，不能评价");
        else if (r == 4) tip("这次就诊已经评价过了");
        else             tip("提交失败");
        return;
    }
}

/* ==========================================================
   十一、主循环
   ========================================================== */
void drawPage()
{
    box(0, 38, WIN_W, WIN_H - 38, C_BG);

    if      (page == 0) drawHome();
    else if (page == 1) drawTriage();
    else if (page == 2) drawBook();
    else if (page == 3) drawCalendar();
    else if (page == 4) drawMyAppt();
    else if (page == 5) drawRoute();
    else if (page == 6) drawQueue();
    else                drawNotice();

    drawTop();
    drawBottom();
}

void onClick(int mx, int my)
{
    /* 先看是不是点了顶上的导航 */
    for (int i = 0; i < 8; i++) {
        if (hit(10 + i * 118, 44, 110, 28)) {
            page = i;
            if (i == 6) pickQueueDoctor();
            return;
        }
    }

    if      (page == 0) clickHome(mx, my);
    else if (page == 1) clickTriage(mx, my);
    else if (page == 2) clickBook(mx, my);
    else if (page == 3) clickCalendar(mx, my);
    else if (page == 4) clickMyAppt(mx, my);
    else if (page == 5) clickRoute(mx, my);
    else if (page == 6) clickQueue(mx, my);
    else                clickNotice(mx, my);
}

void uiMain()
{
    initgraph(WIN_W, WIN_H);
    BeginBatchDraw();

    ExMessage m;
    while (1) {
        while (peekmessage(&m)) {
            if (m.message == WM_MOUSEMOVE)
                { mouseX = m.x; mouseY = m.y; }
            else if (m.message == WM_LBUTTONDOWN)
                onClick(m.x, m.y);
            else if (m.message == WM_KEYDOWN && m.vkcode == VK_ESCAPE)
                { EndBatchDraw(); closegraph(); return; }
            else if (m.message == WM_CLOSE)
                { EndBatchDraw(); closegraph(); return; }
        }

        if (msgTimer > 0) msgTimer--;
        drawPage();
        FlushBatchDraw();
        Sleep(20);
    }
}
