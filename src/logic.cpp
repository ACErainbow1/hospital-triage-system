/* ==========================================================
   logic.cpp  --  各种业务功能
   智能导诊、预约挂号、取消、签到、叫号、停诊通知、评价、转诊推荐
   作者：张健维   24070213
   ========================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "structs.h"

/* ==========================================================
   一、几个小工具
   ========================================================== */

/* 在 text 里找 word，找到返回 1，找不到返回 0。
   这就是自己写的一个简单字符串匹配（两层循环逐个比较） */
int findWord(const char* text, const char* word)
{
    int n = strlen(text);
    int m = strlen(word);
    if (m == 0) return 0;
    for (int i = 0; i + m <= n; i++) {
        int j = 0;
        while (j < m && text[i + j] == word[j]) j++;
        if (j == m) return 1;
    }
    return 0;
}

/* 某科室未来 3 天还剩多少个号 */
int deptRemain3(int dIdx)
{
    char limit[LEN_ID];
    int total = 0;

    if (dIdx < 0) return 0;
    dateAdd(todayNum, 3, limit);

    for (int i = 0; i < scheduleNum; i++) {
        if (schedules[i].status != 0) continue;             /* 停诊不算 */
        if (strcmp(schedules[i].date, todayStr) < 0) continue;   /* 已经过去的 */
        if (strcmp(schedules[i].date, limit) > 0) continue;      /* 太靠后的 */
        int di = findDoctor(schedules[i].doctorId);
        if (di < 0) continue;
        if (strcmp(doctors[di].deptId, depts[dIdx].id) != 0) continue;
        total += remainSeat(i);
    }
    return total;
}

/* 某医生未来几天的余号 */
int doctorRemain(int docIdx, int days)
{
    char limit[LEN_ID];
    int total = 0;

    if (docIdx < 0) return 0;
    dateAdd(todayNum, days, limit);

    for (int i = 0; i < scheduleNum; i++) {
        if (schedules[i].status != 0) continue;
        if (strcmp(schedules[i].doctorId, doctors[docIdx].id) != 0) continue;
        if (strcmp(schedules[i].date, todayStr) < 0) continue;
        if (strcmp(schedules[i].date, limit) > 0) continue;
        total += remainSeat(i);
    }
    return total;
}

/* ==========================================================
   二、智能导诊
   把"症状—科室"看成一张加权二部图：
   症状是一边的顶点，科室是另一边的顶点，边上的权就是关联强度。
   做法是：先在患者说的话里找出认识的症状词，然后沿边把权加到科室头上，
   再加上医生评分和号源的加成，最后换算成百分制，从高到低排序。
   ========================================================== */
int doTriage(const char* text, int* deptIdx, double* match, int maxOut)
{
    double score[MAX_DEPT];
    int i, j;

    if (text == NULL || text[0] == 0) return 0;
    for (i = 0; i < deptNum; i++) score[i] = 0;

    /* 第一步：找出描述里出现了哪些症状词 */
    for (i = 0; i < symptomNum; i++) {
        if (findWord(text, symptoms[i].word)) {
            int di = findDept(symptoms[i].deptId);
            if (di >= 0) score[di] += symptoms[i].weight;
        }
    }

    /* 第二步：一个症状词都没认出来时，拿输入和科室的"常见症状"做个简单比对 */
    int got = 0;
    for (i = 0; i < deptNum; i++)
        if (score[i] > 0) got = 1;

    if (!got) {
        for (i = 0; i < deptNum; i++) {
            char tmp[120];
            char* p;
            strcpy(tmp, depts[i].symptoms);
            p = strtok(tmp, ",");
            while (p != NULL) {
                if (findWord(text, p)) score[i] += 1.0;
                p = strtok(NULL, ",");
            }
        }
    }

    /* 第三步：加上评分加成（最多 1.5 分）和号源加成（最多 1 分） */
    for (i = 0; i < deptNum; i++) {
        if (score[i] <= 0) continue;
        score[i] += deptAvgScore(i) / 5.0 * 1.5;
        int r = deptRemain3(i);
        score[i] += (r > 30 ? 30 : r) / 30.0;
    }

    /* 第四步：找出最高分，换算成百分制的"匹配度" */
    double maxScore = 0;
    for (i = 0; i < deptNum; i++)
        if (score[i] > maxScore) maxScore = score[i];

    if (maxScore <= 0) {
        /* 还是全 0，就按科室评分来排，保证一定能给出推荐 */
        for (i = 0; i < deptNum; i++) score[i] = deptAvgScore(i);
        for (i = 0; i < deptNum; i++)
            if (score[i] > maxScore) maxScore = score[i];
    }
    if (maxScore <= 0) return 0;

    /* 第五步：选择排序，从大到小，取前 maxOut 个 */
    int idx[MAX_DEPT];
    for (i = 0; i < deptNum; i++) idx[i] = i;
    for (i = 0; i < deptNum - 1; i++) {
        int k = i;
        for (j = i + 1; j < deptNum; j++)
            if (score[idx[j]] > score[idx[k]]) k = j;
        if (k != i) { int t = idx[i]; idx[i] = idx[k]; idx[k] = t; }
    }

    int n = 0;
    for (i = 0; i < deptNum && n < maxOut; i++) {
        if (score[idx[i]] <= 0) continue;
        deptIdx[n] = idx[i];
        match[n] = score[idx[i]] / maxScore * 100.0;
        n++;
    }
    return n;
}

/* ==========================================================
   三、科室之间的转诊推荐
   把每个科室看成"症状权重"组成的一个向量，两个科室的相似度
   用余弦相似度来算。号源紧张的科室就可以推荐最相似的科室。
   ========================================================== */
double deptSimilar(int a, int b)
{
    double dot = 0, lenA = 0, lenB = 0;

    if (a < 0 || b < 0) return 0;

    for (int i = 0; i < symptomNum; i++) {
        for (int j = 0; j < symptomNum; j++) {
            if (strcmp(symptoms[i].word, symptoms[j].word) != 0) continue;
            if (strcmp(symptoms[i].deptId, depts[a].id) == 0 &&
                strcmp(symptoms[j].deptId, depts[b].id) == 0)
                dot += symptoms[i].weight * symptoms[j].weight;
        }
    }
    for (int i = 0; i < symptomNum; i++) {
        if (strcmp(symptoms[i].deptId, depts[a].id) == 0) lenA += symptoms[i].weight * symptoms[i].weight;
        if (strcmp(symptoms[i].deptId, depts[b].id) == 0) lenB += symptoms[i].weight * symptoms[i].weight;
    }
    if (lenA <= 0 || lenB <= 0) return 0;
    return dot / (sqrt(lenA) * sqrt(lenB));
}

int recommendTransfer(int dIdx, int* outDept, double* outMatch, int maxOut)
{
    double score[MAX_DEPT];
    int i, j;

    if (dIdx < 0) return 0;
    for (i = 0; i < deptNum; i++) {
        if (i == dIdx) { score[i] = -1; continue; }
        int r = deptRemain3(i);
        if (r <= 0) { score[i] = -1; continue; }        /* 没号的科室不推荐 */
        double sim = deptSimilar(dIdx, i);
        score[i] = sim * 60.0 + (r > 40 ? 40 : r) / 40.0 * 25.0
                 + deptAvgScore(i) / 5.0 * 15.0;
    }

    int idx[MAX_DEPT];
    for (i = 0; i < deptNum; i++) idx[i] = i;
    for (i = 0; i < deptNum - 1; i++) {
        int k = i;
        for (j = i + 1; j < deptNum; j++)
            if (score[idx[j]] > score[idx[k]]) k = j;
        if (k != i) { int t = idx[i]; idx[i] = idx[k]; idx[k] = t; }
    }

    int n = 0;
    for (i = 0; i < deptNum && n < maxOut; i++) {
        if (score[idx[i]] <= 0) continue;
        outDept[n] = idx[i];
        outMatch[n] = score[idx[i]];
        n++;
    }
    return n;
}

/* ==========================================================
   四、预约挂号
   ========================================================== */
int bookAppoint(int pIdx, int sIdx)
{
    if (pIdx < 0) return ERR_PATIENT;
    if (sIdx < 0) return ERR_SCHEDULE;
    if (apptNum >= MAX_APPOINT) return ERR_FULLARR;

    Schedule* s = &schedules[sIdx];
    if (s->status != 0) return ERR_STOP;          /* 停诊 */
    if (remainSeat(sIdx) <= 0) return ERR_FULL;   /* 号已满 */
    if (strcmp(s->date, todayStr) < 0) return ERR_OLD;   /* 日期已经过了 */

    /* 同一个人同一个排班不能约两次 */
    for (int i = 0; i < apptNum; i++) {
        if (strcmp(appts[i].patientId, patients[pIdx].id) != 0) continue;
        if (appts[i].scheduleId != s->id) continue;
        if (appts[i].status == 0 || appts[i].status == 1 || appts[i].status == 2)
            return ERR_REPEAT;
    }

    s->booked++;                       /* 先扣号 */

    int maxId = 0;
    for (int i = 0; i < apptNum; i++)
        if (appts[i].id > maxId) maxId = appts[i].id;

    Appointment* a = &appts[apptNum];
    a->id = maxId + 1;
    strcpy(a->patientId, patients[pIdx].id);
    strcpy(a->doctorId, s->doctorId);
    a->scheduleId = s->id;
    a->seq = s->booked;                /* 第几号 */
    a->status = 0;
    sprintf(a->time, "%s 09:00", todayStr);
    apptNum++;

    return OK;
}

/* 取消预约，把号退回去 */
int cancelAppoint(int aIdx)
{
    if (aIdx < 0) return ERR_STATE;
    if (appts[aIdx].status != 0 && appts[aIdx].status != 1) return ERR_STATE;

    int si = findSchedule(appts[aIdx].scheduleId);
    if (si >= 0 && schedules[si].booked > 0) schedules[si].booked--;

    appts[aIdx].status = 4;
    return OK;
}

/* ==========================================================
   五、候诊队列
   用一个数组存，每次插入的时候按优先级插到合适的位置，
   这样数组最前面的就是最该先看的人（有序插入）
   ========================================================== */
void queueInsert(int apptId, int priority)
{
    if (queueNum >= MAX_QUEUE) return;
    int i = queueNum;
    while (i > 0 && queueData[i - 1].priority > priority) {
        queueData[i] = queueData[i - 1];
        i--;
    }
    queueData[i].apptId = apptId;
    queueData[i].priority = priority;
    queueNum++;
}

void queueRemoveAt(int idx)
{
    for (int i = idx; i < queueNum - 1; i++)
        queueData[i] = queueData[i + 1];
    queueNum--;
}

/* 老年人的号优先看 */
int queuePriority(int pIdx)
{
    if (pIdx < 0) return 3;
    if (patients[pIdx].age >= 65) return 2;
    return 3;
}

/* 签到，进入候诊队列 */
int checkIn(int aIdx)
{
    if (aIdx < 0) return 1;
    if (appts[aIdx].status != 0) return 2;      /* 只有"已预约"的才能签到 */

    appts[aIdx].status = 1;
    queueInsert(aIdx, queuePriority(findPatient(appts[aIdx].patientId)));
    return 0;
}

/* 叫下一位。返回被叫到的预约下标，没人等返回 -1 */
int callNext(int doctorIdx, char* msg, int msgSize)
{
    if (doctorIdx < 0) { strcpy(msg, "请先选择医生"); return -1; }

    for (int i = 0; i < queueNum; i++) {
        int ai = queueData[i].apptId;
        if (ai < 0 || ai >= apptNum) continue;
        if (strcmp(appts[ai].doctorId, doctors[doctorIdx].id) != 0) continue;

        queueRemoveAt(i);
        appts[ai].status = 2;                    /* 变成"就诊中" */

        int pi = findPatient(appts[ai].patientId);
        sprintf(msg, "请 %d 号 %s 到 %s 医生处就诊",
                appts[ai].seq,
                (pi >= 0 ? patients[pi].name : "患者"),
                doctors[doctorIdx].name);
        return ai;
    }
    strcpy(msg, "这个医生现在没有患者在等");
    return -1;
}

/* 看完病 */
int finishVisit(int aIdx)
{
    if (aIdx < 0) return 1;
    if (appts[aIdx].status != 2) return 2;
    appts[aIdx].status = 3;
    return 0;
}

/* 过号：把状态改成爽约，并从队列里去掉 */
int passNumber(int aIdx)
{
    if (aIdx < 0) return 1;
    if (appts[aIdx].status != 1 && appts[aIdx].status != 2) return 2;

    appts[aIdx].status = 5;
    for (int i = 0; i < queueNum; i++)
        if (queueData[i].apptId == aIdx) { queueRemoveAt(i); break; }
    return 0;
}

int countWaiting()
{
    return queueNum;
}

/* 程序刚启动时，把数据文件里状态是"已签到"的预约先放进队列，
   这样候诊队列页一打开就有内容，方便演示 */
void initQueue()
{
    for (int i = 0; i < apptNum; i++) {
        if (appts[i].status != 1) continue;
        queueInsert(i, queuePriority(findPatient(appts[i].patientId)));
    }
}

/* ==========================================================
   六、医生停诊 / 换班通知
   ========================================================== */
void addNotice(const char* patientId, const char* content)
{
    if (noticeNum >= MAX_NOTICE) return;
    strcpy(notices[noticeNum].patientId, patientId);
    strcpy(notices[noticeNum].content, content);
    noticeNum++;
}

/* 停诊：把这条排班下已经约好的患者全部退号，并给他们发通知 */
int suspendSchedule(int sIdx, const char* reason)
{
    if (sIdx < 0) return 0;
    if (schedules[sIdx].status == 1) return 0;

    schedules[sIdx].status = 1;

    char text[LEN_TEXT];
    int count = 0;

    for (int i = 0; i < apptNum; i++) {
        if (appts[i].scheduleId != schedules[sIdx].id) continue;
        if (appts[i].status != 0 && appts[i].status != 1) continue;

        appts[i].status = 6;                    /* 6 表示因停诊退号 */
        count++;

        int di = findDoctor(appts[i].doctorId);
        sprintf(text, "您预约的 %s %s %s 门诊因为「%s」停诊，已经为您退号，挂号费会原路退回。",
                schedules[sIdx].date,
                (schedules[sIdx].period == 1 ? "上午" : "下午"),
                (di >= 0 ? doctors[di].name : "医生"),
                (reason == NULL ? "医生临时停诊" : reason));
        addNotice(appts[i].patientId, text);
    }

    if (schedules[sIdx].booked >= count)
        schedules[sIdx].booked -= count;
    else
        schedules[sIdx].booked = 0;

    return count;      /* 返回通知了几个人 */
}

/* ==========================================================
   七、评价
   ========================================================== */
int addEvaluation(int aIdx, int score, const char* content)
{
    if (aIdx < 0) return 1;
    if (appts[aIdx].status != 3) return 2;          /* 只有看完病才能评价 */
    if (evalNum >= MAX_EVAL) return 3;

    /* 同一个患者对同一个医生同一天只评价一次 */
    for (int i = 0; i < evalNum; i++) {
        if (strcmp(evals[i].patientId, appts[aIdx].patientId) != 0) continue;
        if (strcmp(evals[i].doctorId, appts[aIdx].doctorId) != 0) continue;
        return 4;
    }

    Evaluation* e = &evals[evalNum];
    e->id = evalNum + 1;
    strcpy(e->patientId, appts[aIdx].patientId);
    strcpy(e->doctorId, appts[aIdx].doctorId);
    e->score = score;
    strcpy(e->content, (content == NULL ? "" : content));
    evalNum++;

    /* 重新算一遍这个医生的平均分 */
    int di = findDoctor(appts[aIdx].doctorId);
    if (di >= 0) {
        double sum = 0;
        int n = 0;
        for (int i = 0; i < evalNum; i++)
            if (strcmp(evals[i].doctorId, doctors[di].id) == 0) { sum += evals[i].score; n++; }
        if (n > 0) doctors[di].score = sum / n;
    }
    return 0;
}

/* 找一个这个患者看过病但还没评价的预约，没有返回 -1 */
int pendingEvalAppt(int pIdx)
{
    if (pIdx < 0) return -1;
    for (int i = 0; i < apptNum; i++) {
        if (strcmp(appts[i].patientId, patients[pIdx].id) != 0) continue;
        if (appts[i].status != 3) continue;

        int done = 0;
        for (int j = 0; j < evalNum; j++)
            if (strcmp(evals[j].patientId, appts[i].patientId) == 0 &&
                strcmp(evals[j].doctorId, appts[i].doctorId) == 0) done = 1;
        if (!done) return i;
    }
    return -1;
}

/* ==========================================================
   八、生成预约凭证
   ========================================================== */
void buildReceipt(int aIdx, char* out, int outSize)
{
    char tmp[700];

    if (aIdx < 0) { strcpy(out, "没有这条预约记录"); return; }

    Appointment* a = &appts[aIdx];
    int di = findDoctor(a->doctorId);
    int si = findSchedule(a->scheduleId);
    int pi = findPatient(a->patientId);
    int depi = (di >= 0 ? findDept(doctors[di].deptId) : -1);

    sprintf(tmp,
        "========== 预 约 凭 证 ==========\n"
        "预约编号：%d\n"
        "患 者 名：%s\n"
        "就 诊 科 室：%s\n"
        "就 诊 医 生：%s\n"
        "医 生 职 称：%s\n"
        "就 诊 日 期：%s\n"
        "就 诊 时 段：%s\n"
        "挂 号 序 号：第 %d 号\n"
        "诊 室 位 置：%s\n"
        "挂 号 费 用：%.2f 元\n"
        "当 前 状 态：%s\n",
        a->id,
        (pi >= 0 ? patients[pi].name : "-"),
        (depi >= 0 ? depts[depi].name : "-"),
        (di >= 0 ? doctors[di].name : "-"),
        (di >= 0 ? doctors[di].title : "-"),
        (si >= 0 ? schedules[si].date : "-"),
        (si >= 0 ? (schedules[si].period == 1 ? "上午 08:00-12:00" : "下午 13:30-17:30") : "-"),
        a->seq,
        (di >= 0 ? nodeName(doctors[di].roomNode) : "-"),
        (di >= 0 ? doctors[di].fee : 0.0),
        (a->status == 0 ? "已预约" : a->status == 1 ? "已签到" :
         a->status == 2 ? "就诊中" : a->status == 3 ? "已完成" :
         a->status == 4 ? "已取消" : a->status == 5 ? "爽约" : "停诊退号"));

    if ((int)strlen(tmp) >= outSize) tmp[outSize - 1] = 0;
    strcpy(out, tmp);
}
