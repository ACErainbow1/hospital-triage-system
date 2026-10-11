/* ==========================================================
   structs.h  --  医院门诊预约与智能导诊系统
   所有结构体、常量、全局变量和函数声明都放在这里
   作者：张健维   24070213
   ========================================================== */
#ifndef _STRUCTS_H
#define _STRUCTS_H

/* ---------------- 各种数组的最大长度 ---------------- */
#define MAX_DEPT     20     /* 科室最多 20 个 */
#define MAX_DOCTOR   60     /* 医生最多 60 个 */
#define MAX_SCHEDULE 400    /* 排班最多 400 条 */
#define MAX_PATIENT  20     /* 患者最多 20 个 */
#define MAX_APPOINT  200    /* 预约最多 200 条 */
#define MAX_EVAL     100    /* 评价最多 100 条 */
#define MAX_SYMPTOM  200    /* 症状和科室的关联最多 200 条 */
#define MAX_NODE     120    /* 平面图节点最多 120 个 */
#define MAX_QUEUE    60     /* 候诊队列最多 60 人 */
#define MAX_NOTICE   100    /* 通知最多 100 条 */

/* 字符串长度 */
#define LEN_ID    12
#define LEN_NAME  32
#define LEN_TEXT  200

/* 预约成功/失败的返回值 */
#define OK           0     /* 成功 */
#define ERR_PATIENT  1     /* 患者不存在 */
#define ERR_SCHEDULE 2     /* 排班不存在 */
#define ERR_STOP     3     /* 医生停诊 */
#define ERR_FULL     4     /* 号已经约满 */
#define ERR_REPEAT   5     /* 重复预约同一个时段 */
#define ERR_OLD      6     /* 日期已经过去 */
#define ERR_FULLARR  7     /* 数组满了 */
#define ERR_STATE    8     /* 状态不对，不能做这个操作 */

/* ---------------- 结构体定义 ---------------- */

/* 科室 */
struct Dept {
    char id[LEN_ID];        /* 科室编号，如 D01 */
    char name[LEN_NAME];    /* 科室名称 */
    char place[LEN_NAME];   /* 位置，如 3F-A区 */
    int  floor;             /* 楼层 */
    char desc[120];         /* 科室简介 */
    char symptoms[120];     /* 常见症状 */
};

/* 医生 */
struct Doctor {
    char id[LEN_ID];        /* 医生编号，如 DR01 */
    char name[LEN_NAME];    /* 姓名 */
    char deptId[LEN_ID];    /* 所属科室编号 */
    char title[LEN_NAME];   /* 职称 */
    char good[120];         /* 擅长领域 */
    double fee;             /* 挂号费 */
    double score;           /* 评分 */
    int  roomNode;          /* 诊室在平面图里对应的节点编号 */
};

/* 排班：一个医生在某天的某个时段出诊 */
struct Schedule {
    int  id;                /* 排班编号 */
    char doctorId[LEN_ID];  /* 医生编号 */
    char date[LEN_ID];      /* 日期 */
    int  period;            /* 1 上午  2 下午 */
    int  maxNum;            /* 最多看几个病人 */
    int  booked;            /* 已经约了几个 */
    int  status;            /* 0 正常  1 停诊 */
};

/* 患者 */
struct Patient {
    char id[LEN_ID];
    char name[LEN_NAME];
    int  sex;               /* 1 男  2 女 */
    int  age;
    char phone[20];
    char pwd[LEN_ID];
};

/* 预约记录 */
struct Appointment {
    int  id;                /* 预约编号 */
    char patientId[LEN_ID];
    char doctorId[LEN_ID];
    int  scheduleId;        /* 对应的排班编号 */
    int  seq;               /* 第几号 */
    int  status;            /* 0已预约 1已签到 2就诊中 3已完成 4已取消 5爽约 6停诊退号 */
    char time[24];          /* 预约时间 */
};

/* 评价 */
struct Evaluation {
    int  id;
    char patientId[LEN_ID];
    char doctorId[LEN_ID];
    int  score;             /* 1~5 分 */
    char content[120];
};

/* 症状和科室的关联（加权二部图的边） */
struct Symptom {
    char word[LEN_NAME];    /* 症状关键词，如 胸闷 */
    char deptId[LEN_ID];    /* 对应的科室 */
    double weight;          /* 关联强度 0~1 */
};

/* 平面图上的一个节点（走廊、诊室、电梯……） */
struct MapNode {
    int  id;
    int  floor;
    char name[LEN_NAME];
    int  type;              /* 0走廊 1诊室 2电梯 3楼梯 4大门 5服务点 */
    char deptId[LEN_ID];
    char doctorId[LEN_ID];
    int  x, y;              /* 画图用的坐标 */
};

/* 候诊队列里的一项 */
struct QueueItem {
    int apptId;             /* 对应哪条预约 */
    int priority;           /* 优先级，数字越小越靠前 */
};

/* 通知（医生停诊时发给患者） */
struct Notice {
    char patientId[LEN_ID];
    char content[LEN_TEXT];
};

/* ---------------- 全局变量 ---------------- */
extern Dept        depts[MAX_DEPT];
extern int         deptNum;

extern Doctor      doctors[MAX_DOCTOR];
extern int         doctorNum;

extern Schedule    schedules[MAX_SCHEDULE];
extern int         scheduleNum;

extern Patient     patients[MAX_PATIENT];
extern int         patientNum;

extern Appointment appts[MAX_APPOINT];
extern int         apptNum;

extern Evaluation  evals[MAX_EVAL];
extern int         evalNum;

extern Symptom     symptoms[MAX_SYMPTOM];
extern int         symptomNum;

extern MapNode     nodes[MAX_NODE];
extern int         nodeNum;

/* 平面图用邻接矩阵存，graph[i][j] 是距离，0 表示不通 */
extern int         graph[MAX_NODE][MAX_NODE];
extern int         edgeKind[MAX_NODE][MAX_NODE];

/* 候诊队列（按优先级插入的有序数组） */
extern QueueItem   queueData[MAX_QUEUE];
extern int         queueNum;

extern Notice      notices[MAX_NOTICE];
extern int         noticeNum;

/* 今天 */
extern int         todayNum;            /* 20261011 这种形式 */
extern char        todayStr[LEN_ID];    /* 2026-10-11 */

/* 当前登录的患者下标，-1 表示没登录 */
extern int         curPatient;

/* ---------------- 函数声明 ---------------- */

/* ---------- data.cpp : 读文件、查找 ---------- */
int  loadData();                                /* 读取全部数据文件，成功返回 1 */
void realDate(const char* src, char* out);      /* 把 TODAY+1 换成真实日期 */
int  addDays(int dateNum, int n);               /* 日期加减 */
int  weekDay(int dateNum);                      /* 0 星期日 … 6 星期六 */
const char* weekName(int w);                    /* "周一" … "周日" */
void dateAdd(int dateNum, int n, char* out);    /* 加减后转成字符串 */

int  findDept(const char* id);
int  findDoctor(const char* id);
int  findPatient(const char* id);
int  findSchedule(int id);
int  findAppointment(int id);
int  findDeptByName(const char* name);

int  remainSeat(int sIdx);                      /* 某条排班还剩几个号 */
int  deptDoctorNum(int dIdx);                   /* 某科室有几个医生 */
double deptAvgScore(int dIdx);                  /* 某科室的平均分 */

/* ---------- graph.cpp : 平面图和最短路 ---------- */
int  gateNode();                                /* 门诊大门节点编号 */
int  findRoomNode(const char* doctorId);        /* 医生诊室对应的节点 */
const char* nodeName(int id);                   /* 节点名字 */
int  nodeFloor(int id);                         /* 节点在第几层 */
int  nodeType(int id);                          /* 节点类型 */
int  shortestPath(int start, int end, int* path, int* len);   /* Dijkstra，返回总距离 */
void pathToText(int* path, int len, char* out, int outSize);  /* 把路径变成文字说明 */

/* ---------- logic.cpp : 业务功能 ---------- */
int  findWord(const char* text, const char* word);
int  deptRemain3(int dIdx);                     /* 某科室未来 3 天的余号 */
int  doctorRemain(int docIdx, int days);        /* 某医生未来几天的余号 */
double deptSimilar(int a, int b);               /* 两个科室的相似度 */
int  doTriage(const char* text, int* deptIdx, double* match, int maxOut);
int  bookAppoint(int pIdx, int sIdx);           /* 预约，返回 0 成功，其它是错误码 */
int  cancelAppoint(int aIdx);
int  checkIn(int aIdx);                         /* 签到进候诊队列 */
int  callNext(int doctorIdx, char* msg, int msgSize);
int  finishVisit(int aIdx);
int  passNumber(int aIdx);                      /* 过号 */
int  countWaiting();
void initQueue();                               /* 把数据里"已签到"的预约放进队列 */
int  suspendSchedule(int sIdx, const char* reason);
int  addEvaluation(int aIdx, int score, const char* content);
int  recommendTransfer(int dIdx, int* outDept, double* outMatch, int maxOut);
int  pendingEvalAppt(int pIdx);                 /* 找一个还没评价的已完成预约，没有返回 -1 */
void addNotice(const char* patientId, const char* content);
void buildReceipt(int aIdx, char* out, int outSize);   /* 生成预约凭证文字 */

/* ---------- ui.cpp : 图形界面 ---------- */
void uiMain();

#endif
