/* ==========================================================
   graph.cpp  --  就诊楼平面图和最短路径
   平面图用邻接矩阵存，求最短路用 Dijkstra 算法
   作者：张健维   24070213
   ========================================================== */
#include <stdio.h>
#include <string.h>
#include "structs.h"

/* 找门诊大门那个节点 */
int gateNode()
{
    for (int i = 0; i < nodeNum; i++)
        if (nodes[i].type == 4) return nodes[i].id;
    return -1;
}

/* 找某个医生的诊室节点 */
int findRoomNode(const char* doctorId)
{
    int di = findDoctor(doctorId);
    if (di < 0) return -1;
    return doctors[di].roomNode;
}

/* 根据节点编号取名字 */
const char* nodeName(int id)
{
    for (int i = 0; i < nodeNum; i++)
        if (nodes[i].id == id) return nodes[i].name;
    return "未知地点";
}

/* 根据节点编号取楼层 */
int nodeFloor(int id)
{
    for (int i = 0; i < nodeNum; i++)
        if (nodes[i].id == id) return nodes[i].floor;
    return 0;
}

/* 根据节点编号取类型 */
int nodeType(int id)
{
    for (int i = 0; i < nodeNum; i++)
        if (nodes[i].id == id) return nodes[i].type;
    return 0;
}

/* ==========================================================
   Dijkstra 算法求最短路
   参数：起点、终点、用来存路径的数组、返回路径长度
   返回：总距离；找不到路返回 -1
   ========================================================== */
int shortestPath(int start, int end, int* path, int* len)
{
    int dist[MAX_NODE];      /* 起点到每个点的最短距离，-1 表示还没算出来 */
    int visited[MAX_NODE];   /* 是否已经确定 */
    int prev[MAX_NODE];      /* 记录是从哪个点走过来的，用来倒推路径 */
    int i, j;

    *len = 0;
    if (start <= 0 || end <= 0) return -1;

    for (i = 0; i < MAX_NODE; i++) {
        dist[i] = -1;
        visited[i] = 0;
        prev[i] = -1;
    }
    dist[start] = 0;

    for (int k = 0; k < nodeNum; k++)
    {
        /* 第一步：在所有还没确定的点里，找一个离起点最近的 */
        int u = -1, best = 0;
        for (i = 0; i < nodeNum; i++) {
            int id = nodes[i].id;
            if (visited[id]) continue;
            if (dist[id] < 0) continue;
            if (u < 0 || dist[id] < best) { best = dist[id]; u = id; }
        }
        if (u < 0) break;          /* 剩下的点都到不了 */
        if (u == end) break;       /* 终点已经确定，可以停了 */
        visited[u] = 1;

        /* 第二步：用这个点去更新它周围的点 */
        for (j = 0; j < nodeNum; j++) {
            int v = nodes[j].id;
            if (graph[u][v] == 0) continue;   /* 不通 */
            if (visited[v]) continue;
            if (dist[v] < 0 || dist[u] + graph[u][v] < dist[v]) {
                dist[v] = dist[u] + graph[u][v];
                prev[v] = u;
            }
        }
    }

    if (dist[end] < 0) return -1;    /* 走不到 */

    /* 从终点往回倒推，得到的是反的，再翻过来 */
    int tmp[MAX_NODE];
    int n = 0;
    int cur = end;
    while (cur != -1 && n < MAX_NODE) {
        tmp[n++] = cur;
        if (cur == start) break;
        cur = prev[cur];
    }
    for (i = 0; i < n; i++)
        path[i] = tmp[n - 1 - i];
    *len = n;

    return dist[end];
}

/* 把路径变成一段文字说明 */
void pathToText(int* path, int len, char* out, int outSize)
{
    char line[160];

    out[0] = 0;
    if (len <= 0) { strcpy(out, "没有找到路线"); return; }

    strcpy(out, "从 ");
    strcat(out, nodeName(path[0]));
    strcat(out, " 出发\n");

    for (int i = 1; i < len; i++)
    {
        int a = path[i - 1], b = path[i];
        const char* how = "沿走廊走到";

        if (edgeKind[a][b] == 1)      how = "坐电梯到";
        else if (edgeKind[a][b] == 2) how = "走楼梯到";
        else if (nodeType(b) == 1)    how = "走到";

        sprintf(line, "%d. %s%s（%d米）\n", i, how, nodeName(b), graph[a][b]);
        if ((int)(strlen(out) + strlen(line)) < outSize)
            strcat(out, line);
    }
}
