/* ==========================================================
   data.cpp  --  读取数据文件、各种查找、日期计算
   作者：张健维   24070213
   ========================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include "structs.h"

/* ---------------- 全局变量（在 structs.h 里 extern 过） ---------------- */
Dept        depts[MAX_DEPT];        int deptNum     = 0;
Doctor      doctors[MAX_DOCTOR];    int doctorNum   = 0;
Schedule    schedules[MAX_SCHEDULE];int scheduleNum = 0;
Patient     patients[MAX_PATIENT];  int patientNum  = 0;
Appointment appts[MAX_APPOINT];     int apptNum     = 0;
Evaluation  evals[MAX_EVAL];        int evalNum     = 0;
Symptom     symptoms[MAX_SYMPTOM];  int symptomNum  = 0;
MapNode     nodes[MAX_NODE];        int nodeNum     = 0;

int graph[MAX_NODE][MAX_NODE];
int edgeKind[MAX_NODE][MAX_NODE];

QueueItem   queueData[MAX_QUEUE];
int         queueNum = 0;

Notice      notices[MAX_NOTICE];
int         noticeNum = 0;

int  todayNum = 0;
char todayStr[LEN_ID] = "";

int  curPatient = -1;      /* 当前登录的患者，-1 表示没登录 */

#define LINE_LEN 512       /* 读一行用的缓冲区大小 */
#define MAXF     16        /* 一行最多几个字段 */
#define FLEN     160       /* 每个字段最长多少 */

/* ==========================================================
   一、小工具函数
   ========================================================== */

/* 数据文件是 UTF-8 存的，而程序里的中文是 GBK（编译参数 -fexec-charset=GBK），
   EasyX 显示中文也按 GBK 来。所以从文件读进来以后要转一下，不然中文全乱码。 */
void utf8ToGbk(char* s)
{
    wchar_t wbuf[600];
    char    tmp[600];
    MultiByteToWideChar(CP_UTF8, 0, s, -1, wbuf, 600);
    WideCharToMultiByte(CP_ACP, 0, wbuf, -1, tmp, 600, NULL, NULL);
    strcpy(s, tmp);
}

/* 去掉字符串末尾的换行和空格 */
void trim(char* s)
{
    int n = strlen(s);
    while (n > 0 && (s[n-1] == '\n' || s[n-1] == '\r' ||
                     s[n-1] == ' '  || s[n-1] == '\t'))
        s[--n] = 0;
}

/* 按 '|' 把一行拆成几个字段，返回字段个数 */
int split(char* line, char field[][FLEN], int maxField)
{
    int n = 0;
    char* p = line;
    while (*p && n < maxField) {
        int i = 0;
        while (*p && *p != '|' && i < FLEN - 1)
            field[n][i++] = *p++;
        field[n][i] = 0;
        n++;
        if (*p == '|') p++;
    }
    return n;
}

/* 读一行有效数据。自动跳过空行、以 # 开头的注释行和文件开头的 BOM */
int readLine(FILE* fp, char* buf)
{
    while (fgets(buf, LINE_LEN, fp))
    {
        /* 去掉 UTF-8 文件开头的 BOM（三个字节 EF BB BF） */
        if ((unsigned char)buf[0] == 0xEF &&
            (unsigned char)buf[1] == 0xBB &&
            (unsigned char)buf[2] == 0xBF)
            memmove(buf, buf + 3, strlen(buf));

        trim(buf);
        if (buf[0] == 0) continue;      /* 空行 */
        if (buf[0] == '#') continue;    /* 注释行 */

        utf8ToGbk(buf);                 /* 中文转码，避免乱码 */
        return 1;
    }
    return 0;
}

/* ==========================================================
   二、日期计算
   ========================================================== */

int isLeap(int y)
{
    return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}

int monthDays(int y, int m)
{
    int a[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (m == 2 && isLeap(y)) return 29;
    return a[m];
}

/* 20261011 -> "2026-10-11" */
void numToDate(int n, char* out)
{
    int y = n / 10000, m = n / 100 % 100, d = n % 100;
    sprintf(out, "%04d-%02d-%02d", y, m, d);
}

/* 日期加 n 天，n 可以是负数 */
int addDays(int dateNum, int n)
{
    int y = dateNum / 10000, m = dateNum / 100 % 100, d = dateNum % 100;
    while (n > 0) {
        d++;
        if (d > monthDays(y, m)) { d = 1; m++; if (m > 12) { m = 1; y++; } }
        n--;
    }
    while (n < 0) {
        d--;
        if (d < 1) { m--; if (m < 1) { m = 12; y--; } d = monthDays(y, m); }
        n++;
    }
    return y * 10000 + m * 100 + d;
}

/* 日期加减后直接得到字符串 */
void dateAdd(int dateNum, int n, char* out)
{
    numToDate(addDays(dateNum, n), out);
}

/* 用基姆拉尔森公式求星期几。返回 0=周一 1=周二 … 5=周六 6=周日 */
int weekDay(int dateNum)
{
    int y = dateNum / 10000, m = dateNum / 100 % 100, d = dateNum % 100;
    if (m == 1 || m == 2) { m += 12; y--; }
    int w = (d + 2 * m + 3 * (m + 1) / 5 + y + y / 4 - y / 100 + y / 400) % 7;
    return w;
}

const char* weekName(int w)
{
    const char* name[7] = {"周一", "周二", "周三", "周四", "周五", "周六", "周日"};
    if (w < 0 || w > 6) return "";
    return name[w];
}

/* 数据文件里日期写成 TODAY、TODAY+1、TODAY-1 这种，
   这里换成真正的日期，这样测试数据不会过期 */
void realDate(const char* src, char* out)
{
    if (strncmp(src, "TODAY", 5) != 0) { strcpy(out, src); return; }
    int off = 0;
    if (src[5] == '+') off = atoi(src + 6);
    else if (src[5] == '-') off = -atoi(src + 6);
    numToDate(addDays(todayNum, off), out);
}

/* 取今天的日期 */
void initToday()
{
    time_t t = time(0);
    struct tm* p = localtime(&t);
    todayNum = (p->tm_year + 1900) * 10000 + (p->tm_mon + 1) * 100 + p->tm_mday;
    numToDate(todayNum, todayStr);
}

/* ==========================================================
   三、读各个数据文件
   ========================================================== */

int loadDept()
{
    FILE* fp = fopen("data/department.txt", "r");
    if (fp == NULL) { printf("打不开 data/department.txt\n"); return 0; }
    char buf[LINE_LEN], f[MAXF][FLEN];
    while (readLine(fp, buf) && deptNum < MAX_DEPT)
    {
        if (split(buf, f, MAXF) < 6) continue;
        Dept* d = &depts[deptNum];
        strcpy(d->id, f[0]);
        strcpy(d->name, f[1]);
        strcpy(d->place, f[2]);
        d->floor = atoi(f[3]);
        strcpy(d->desc, f[4]);
        strcpy(d->symptoms, f[5]);
        deptNum++;
    }
    fclose(fp);
    return 1;
}

int loadDoctor()
{
    FILE* fp = fopen("data/doctor.txt", "r");
    if (fp == NULL) { printf("打不开 data/doctor.txt\n"); return 0; }
    char buf[LINE_LEN], f[MAXF][FLEN];
    while (readLine(fp, buf) && doctorNum < MAX_DOCTOR)
    {
        if (split(buf, f, MAXF) < 7) continue;
        Doctor* d = &doctors[doctorNum];
        strcpy(d->id, f[0]);
        strcpy(d->name, f[1]);
        strcpy(d->deptId, f[2]);
        strcpy(d->title, f[3]);
        strcpy(d->good, f[4]);
        d->fee = atof(f[5]);
        d->score = atof(f[6]);
        d->roomNode = -1;
        doctorNum++;
    }
    fclose(fp);
    return 1;
}

int loadSchedule()
{
    FILE* fp = fopen("data/schedule.txt", "r");
    if (fp == NULL) { printf("打不开 data/schedule.txt\n"); return 0; }
    char buf[LINE_LEN], f[MAXF][FLEN];
    while (readLine(fp, buf) && scheduleNum < MAX_SCHEDULE)
    {
        if (split(buf, f, MAXF) < 7) continue;
        Schedule* s = &schedules[scheduleNum];
        s->id = atoi(f[0]);
        strcpy(s->doctorId, f[1]);
        realDate(f[2], s->date);        /* TODAY+1 -> 真实日期 */
        s->period  = atoi(f[3]);
        s->maxNum  = atoi(f[4]);
        s->booked  = atoi(f[5]);
        s->status  = atoi(f[6]);
        scheduleNum++;
    }
    fclose(fp);
    return 1;
}

int loadPatient()
{
    FILE* fp = fopen("data/patient.txt", "r");
    if (fp == NULL) { printf("打不开 data/patient.txt\n"); return 0; }
    char buf[LINE_LEN], f[MAXF][FLEN];
    while (readLine(fp, buf) && patientNum < MAX_PATIENT)
    {
        if (split(buf, f, MAXF) < 6) continue;
        Patient* p = &patients[patientNum];
        strcpy(p->id, f[0]);
        strcpy(p->name, f[1]);
        p->sex = atoi(f[2]);
        p->age = atoi(f[3]);
        strcpy(p->phone, f[4]);
        strcpy(p->pwd, f[5]);
        patientNum++;
    }
    fclose(fp);
    return 1;
}

/* 预约文件里只写了排班编号，医生编号是通过排班推出来的 */
int loadAppointment()
{
    FILE* fp = fopen("data/appointment.txt", "r");
    if (fp == NULL) { printf("打不开 data/appointment.txt\n"); return 0; }
    char buf[LINE_LEN], f[MAXF][FLEN];
    while (readLine(fp, buf) && apptNum < MAX_APPOINT)
    {
        if (split(buf, f, MAXF) < 6) continue;
        int si = findSchedule(atoi(f[2]));
        if (si < 0) continue;           /* 排班不存在就跳过 */
        Appointment* a = &appts[apptNum];
        a->id = atoi(f[0]);
        strcpy(a->patientId, f[1]);
        a->scheduleId = atoi(f[2]);
        strcpy(a->doctorId, schedules[si].doctorId);
        a->seq    = atoi(f[3]);
        a->status = atoi(f[4]);
        realDate(f[5], a->time);        /* 只取日期部分也可以 */
        apptNum++;
    }
    fclose(fp);
    return 1;
}

int loadEvaluation()
{
    FILE* fp = fopen("data/evaluation.txt", "r");
    if (fp == NULL) return 1;           /* 评价文件可以没有 */
    char buf[LINE_LEN], f[MAXF][FLEN];
    while (readLine(fp, buf) && evalNum < MAX_EVAL)
    {
        if (split(buf, f, MAXF) < 5) continue;
        Evaluation* e = &evals[evalNum];
        e->id = atoi(f[0]);
        strcpy(e->patientId, f[1]);
        strcpy(e->doctorId, f[2]);
        e->score = atoi(f[3]);
        strcpy(e->content, f[4]);
        evalNum++;
    }
    fclose(fp);
    return 1;
}

/* 症状文件里一行就是"症状 + 科室 + 权重"一条边
   同一个症状会出现好几行（对应不同科室） */
int loadSymptom()
{
    FILE* fp = fopen("data/symptom.txt", "r");
    if (fp == NULL) { printf("打不开 data/symptom.txt\n"); return 0; }
    char buf[LINE_LEN], f[MAXF][FLEN];
    while (readLine(fp, buf) && symptomNum < MAX_SYMPTOM)
    {
        if (split(buf, f, MAXF) < 3) continue;
        Symptom* s = &symptoms[symptomNum];
        strcpy(s->word, f[0]);
        strcpy(s->deptId, f[1]);
        s->weight = atof(f[2]);
        symptomNum++;
    }
    fclose(fp);
    return 1;
}

/* 平面图：一行是一个节点(N)或一条边(L)，用邻接矩阵存 */
int loadMap()
{
    FILE* fp = fopen("data/map.txt", "r");
    if (fp == NULL) { printf("打不开 data/map.txt\n"); return 0; }
    char buf[LINE_LEN], f[MAXF][FLEN];

    memset(graph, 0, sizeof(graph));
    memset(edgeKind, 0, sizeof(edgeKind));

    while (readLine(fp, buf))
    {
        int n = split(buf, f, MAXF);
        if (n < 5) continue;

        if (strcmp(f[0], "N") == 0 && n >= 9 && nodeNum < MAX_NODE)
        {
            MapNode* m = &nodes[nodeNum];
            m->id = atoi(f[1]);
            m->floor = atoi(f[2]);
            strcpy(m->name, f[3]);
            m->type = atoi(f[4]);
            strcpy(m->deptId, f[5]);
            strcpy(m->doctorId, f[6]);
            m->x = atoi(f[7]);
            m->y = atoi(f[8]);
            nodeNum++;
        }
        else if (strcmp(f[0], "L") == 0)
        {
            int a = atoi(f[1]), b = atoi(f[2]);
            if (a <= 0 || b <= 0 || a >= MAX_NODE || b >= MAX_NODE) continue;
            graph[a][b] = graph[b][a] = atoi(f[3]);      /* 无向图，两个方向都存 */
            edgeKind[a][b] = edgeKind[b][a] = atoi(f[4]);
        }
    }
    fclose(fp);

    /* 每个医生的诊室在平面图里是哪个节点 */
    for (int i = 0; i < doctorNum; i++)
        for (int j = 0; j < nodeNum; j++)
            if (strcmp(nodes[j].doctorId, doctors[i].id) == 0)
                doctors[i].roomNode = nodes[j].id;

    return 1;
}

/* 用评价里的分数重新算一遍医生的评分。没有评价的医生就保留文件里的初始分 */
void updateScore()
{
    for (int i = 0; i < doctorNum; i++)
    {
        double sum = 0;
        int n = 0;
        for (int j = 0; j < evalNum; j++)
            if (strcmp(evals[j].doctorId, doctors[i].id) == 0) {
                sum += evals[j].score;
                n++;
            }
        if (n > 0) doctors[i].score = sum / n;
    }
}

/* 读入全部数据 */
int loadData()
{
    initToday();
    if (!loadDept())       return 0;
    if (!loadDoctor())     return 0;
    if (!loadSchedule())   return 0;
    if (!loadPatient())    return 0;
    if (!loadAppointment()) return 0;
    loadEvaluation();
    if (!loadSymptom())    return 0;
    if (!loadMap())        return 0;
    updateScore();
    return 1;
}

/* ==========================================================
   四、查找函数（数据量不大，直接用顺序查找）
   ========================================================== */

int findDept(const char* id)
{
    for (int i = 0; i < deptNum; i++)
        if (strcmp(depts[i].id, id) == 0) return i;
    return -1;
}

int findDeptByName(const char* name)
{
    for (int i = 0; i < deptNum; i++)
        if (strcmp(depts[i].name, name) == 0) return i;
    return -1;
}

int findDoctor(const char* id)
{
    for (int i = 0; i < doctorNum; i++)
        if (strcmp(doctors[i].id, id) == 0) return i;
    return -1;
}

int findPatient(const char* id)
{
    for (int i = 0; i < patientNum; i++)
        if (strcmp(patients[i].id, id) == 0) return i;
    return -1;
}

int findSchedule(int id)
{
    for (int i = 0; i < scheduleNum; i++)
        if (schedules[i].id == id) return i;
    return -1;
}

int findAppointment(int id)
{
    for (int i = 0; i < apptNum; i++)
        if (appts[i].id == id) return i;
    return -1;
}

/* ==========================================================
   五、统计函数
   ========================================================== */

/* 某条排班还剩几个号 */
int remainSeat(int sIdx)
{
    if (sIdx < 0 || sIdx >= scheduleNum) return 0;
    int r = schedules[sIdx].maxNum - schedules[sIdx].booked;
    return r < 0 ? 0 : r;
}

/* 某个科室有几个医生 */
int deptDoctorNum(int dIdx)
{
    if (dIdx < 0) return 0;
    int n = 0;
    for (int i = 0; i < doctorNum; i++)
        if (strcmp(doctors[i].deptId, depts[dIdx].id) == 0) n++;
    return n;
}

/* 某个科室所有医生的平均分 */
double deptAvgScore(int dIdx)
{
    if (dIdx < 0) return 0;
    double sum = 0;
    int n = 0;
    for (int i = 0; i < doctorNum; i++)
        if (strcmp(doctors[i].deptId, depts[dIdx].id) == 0) {
            sum += doctors[i].score;
            n++;
        }
    if (n == 0) return 0;
    return sum / n;
}
