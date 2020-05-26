#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>

#define SIZE_OF_INFO 128
#define SIZE_OF_LOG 512
#define SIZE_OF_FILE 512
#define EXIT 0
#define HOST 1
#define TIME 2
#define DSG 3
#define MAX 4
#define MONTH 12
#define START 0
#define END 1

static void DesignationPeriod(int mode);                                                //期間指定したとき用の関数
static int GetCmd(void);                                                                //コマンド入力関数
static char* GetHostInfo(char *log, char* host);                                        //ログの中からリモートホスト情報を取得する関数
static int GetMonth(char month[4]);                                                     //文字列の月を数値に変換する関数
static char* GetTimeInfo(char *log, char* time);                                        //ログの中から時間帯情報を取得する関数
static void InitHeader(void);                                                           //連結リストの先頭（ヘッダ）をNULLにする関数
static struct info_list* MakeNewInfoList(char info[SIZE_OF_INFO], int data);            //ソート後の連結リストのセルを作成する関数
static int OpenDirectory(char *dirpath);                                                //指定したパスのディレクトリを開く関数
static void PrintData(int mode);                                                        //データをファイル出力する関数
static void PrintExplain(void);                                                         //コマンドオプションを出力する関数
static int SearchFileName(char* filename, char* dirpath);                               //指定したパス内の同一ファイル名を探索する関数
static struct info_list* SearchLine(int year, int month, int day, int mode);            //指定した期間のデータをもつポインタを探索する関数
static int SplitDayMonthYear(char* time, int year, int month, int day);                 //時間情報を年・月・日に分割して連結リストにデータを格納する関数
static void InfoCounter(char* log);                                                     //ログ情報から同じデータが見つかればインクリメントする関数
static void InsertSort();                                                               //降順ソートしながら挿入する関数
static void InsertSortForDsg(struct info_list* p, struct info_list* end);               //開始・終了期間を指定したときの挿入ソート関数
static void InitArray(char* info);                                                      //配列を初期化する関数
static struct info_list* SetData(char* log);                                            //連結リストのセルを作成する関数


/*リモートホスト名もしくは時間情報とカウンタを保持する構造体*/
struct info_list {
    struct info_list* next;
    char info[SIZE_OF_INFO];
    int count;
};
static struct info_list Head, sHead;

int main(void)
{

    FILE* fp;
    char filename[SIZE_OF_FILE];
    char host[SIZE_OF_INFO] = { '\0' };
    char time[SIZE_OF_INFO] = { '\0' };
    int num;
    char dirpath[] = "/var/log/httpd/";
    int i;
    char filepath[SIZE_OF_FILE];
    char log[SIZE_OF_LOG];
    int mode, mode2 = 0;
    int flag = 0;

    PrintExplain();
    switch (GetCmd()) {
    case EXIT:
        return -1;
        break;
    case HOST:
        mode = HOST;
        break;
    case TIME:
        mode = TIME;
        break;
    case DSG:
        mode = DSG;
        printf("%d : 各時間帯毎のアクセス件数を表示\n", TIME);
        printf("%d : リモートホスト別のアクセス件数を表示\n", HOST);
        printf("期間指定で表示したい情報を入力してください : ");
        scanf("%d", &mode2);
        break;
    default:
        return -1;
        break;
    }

    printf("ログファイルの取得件数 : ");
    scanf("%d", &num);
    if (num <= 0) {
        printf("入力エラー。\n");
        exit(1);
    }

    if (OpenDirectory(dirpath) == -1) {
        printf("パスエラー\n");
        exit(1);
    }

    //ヘッダの初期化
    InitHeader();

    
    for (i = 0; i < num; i++) {
        printf("読み込むログファイル名（%d個目）を入力 : ", i + 1);
        scanf("%s", filename);
        if (SearchFileName(filename, dirpath) == -1) {
            printf("そのような名前のファイルは存在しません\n");
            if (num == 1) {
                exit(1);
            }
            else {
                continue;
            }
        }
        sprintf(filepath, "%s%s", dirpath, filename);

        /* ファイルを開く */
        fp = fopen(filepath, "r");
        if (fp == NULL) {
            printf("ファイルが存在しません。プログラムを終了します。\n");
            exit(1);
        }

        if (mode == HOST) {
            while (fgets(log, SIZE_OF_LOG, fp) != NULL) {
                InfoCounter(GetHostInfo(log, host));
                InitArray(host);
            }
            InsertSort();
        }
        else if (mode == TIME) {
            while (fgets(log, SIZE_OF_LOG, fp) != NULL) {
                InfoCounter(GetTimeInfo(log, time));
                InitArray(time);
            }
            InsertSort();
        }
        else if (mode == DSG) {
            flag = 1;
            if (mode2 == HOST) {
                while (fgets(log, SIZE_OF_LOG, fp) != NULL) {
                    InfoCounter(GetTimeInfo(log, host));
                    InitArray(host);
                }
            }
            else if (mode2 == TIME) {
                while (fgets(log, SIZE_OF_LOG, fp) != NULL) {
                    InfoCounter(GetTimeInfo(log, time));
                    InitArray(time);
                }
            }
            else {
                printf("コマンドエラー\n");
                return -1;
            }
        }
        
        fclose(fp);
    }
    if (flag == 1) {
        
        DesignationPeriod(mode2);
        mode = mode2;
    }
    PrintData(mode);
   
   return 0;
}

void DesignationPeriod(int mode) 
{
    int sYear, eYear, sMonth, eMonth, sDay, eDay;
    char start[SIZE_OF_INFO] = { 0 }, end[SIZE_OF_INFO] = { 0 };
    struct info_list* p, * q;

    printf("開始期間 year : "); scanf("%d", &sYear);
    printf("開始期間 month : "); scanf("%d", &sMonth);
    printf("開始期間 day : "); scanf("%d", &sDay);
    printf("終了期間 year : "); scanf("%d", &eYear);
    printf("終了期間 month : "); scanf("%d", &eMonth);
    printf("終了期間 day : "); scanf("%d", &eDay);

    sprintf(start, "%d年%d月%d日", sYear, sMonth, sDay);
    sprintf(end, "%d年%d月%d日", eYear, eMonth, eDay);
    printf("%s -> %s\n", start, end);

    /* p : 開始期間ポインタ q : 終了期間ポインタ */
    /*指定した期間と同じデータをもつポインタを探索*/

    if ((p = SearchLine(sYear, sMonth, sDay, START)) == NULL) {
        printf("開始期間エラー\n");
        return;
    }
    if ((q = SearchLine(eYear, eMonth, eDay, END)) == NULL) {
        printf("終了期間エラー\n");
        return;
    }
    InsertSortForDsg(p, q);
}

int GetCmd(void)
{
    int cmd;
    do {
        printf("実行するコマンドを入力してください : ");
        scanf("%d", &cmd);
    } while (cmd < EXIT  ||  MAX <= cmd);
    
    return cmd;
}

char* GetHostInfo(char* log, char* host)
{

    int i, j = 0;

    for (i = 0; i < SIZE_OF_INFO; i++) {
        if (log[i] != ' ') {
            /*初めの空白文字が見つかるまで*/
            /*ログ情報をホスト用配列に代入*/
            host[j] = log[i];
            j++;
        }
        if (log[i] == ' ') {
            /*空白文字が見つかったら*/
            /*探索終了*/
            break;
        }
    }

    return host;
}

int GetMonth(char month[4]) 
{
    char cMonth[MONTH][4] = { {"Jan"},{"Feb"},{"Mar"}, {"Apr"},{"May"},{"Jun"}, {"Jul"},{"Aug"},{"Sep"},{"Oct"},{"Nov"}, {"Dec"} };
    
    int i;
    int monthNum;
    for (i = 0; i < MONTH; i++) {
        if (strcmp(month, cMonth[i]) == 0) {
            monthNum = i + 1;
        }
    }

    return monthNum;
}

char* GetTimeInfo(char* log, char* time)
{
    int i, j = 0;
    int flag = 0;

    for (i = 0; i < SIZE_OF_INFO; i++) {
        if (log[i] == ']') {
            /*']'が見つかったら*/
            /*代入終了*/
            flag = 0;
            break;
        }
        if (flag == 1) {
            time[j] = log[i];
            j++;
        }
        if (log[i] == '[') {
            /*'['が見つかったら*/
            /*代入開始*/
            flag = 1;
        }
    }

    return time;
}

struct info_list* SetData(char* log)
{
    /*連結リスト用のセルを作成*/
    struct info_list* p = (struct info_list*)malloc(sizeof(struct info_list));
    if (p == NULL) {
        fputs("メモリ割り当てに失敗しました。", stderr);
    }
    strcpy(p->info, log);
    p->count = 1;

    return p;
}

void InfoCounter(char* log)
{
    /*ログ情報をもとに連結リストを作成*/
    /*同じ情報があれば、カウンタをインクリメントしメモリを開放する*/
    struct info_list* p = &Head;
    struct info_list* q = (struct info_list*)malloc(sizeof(struct info_list));
    if (q == NULL) {
        fputs("メモリ割り当てに失敗しました。", stderr);
        exit(1);
    }
    q = SetData(log);

    if (p->next == NULL) {
        p->next = q;
        
    }
    else {
        while (p != NULL) {
            if (strcmp(p->info, log) == 0) {
                p->count++;
                free(q);
                break;
            }
            else if (p->next == NULL) {
                p->next = q;
                break;
            }
            p = p->next;
        }
        
    }
}

void InsertSort(void)
{
    /*InfoCounterで作成した連結リストを基に、挿入ソートする*/
    struct info_list* p = (struct info_list*)malloc(sizeof(struct info_list));
    if (p == NULL) {
        fputs("メモリ割り当てに失敗しました。", stderr);
        exit(1);
    }
    struct info_list* q = (struct info_list*)malloc(sizeof(struct info_list));
    if (q == NULL) {
        fputs("メモリ割り当てに失敗しました。", stderr);
        exit(1);
    }
    struct info_list* pNew = (struct info_list*)malloc(sizeof(struct info_list));
    if (pNew == NULL) {
        fputs("メモリ割り当てに失敗しました。", stderr);
        exit(1);
    }

    p = Head.next;
    
    while (p != NULL) {
        pNew = MakeNewInfoList(p->info, p->count);
        q = &sHead;
        if (q->next == NULL) {
            /*リストが空の場合*/
            /*データを先頭に格納する*/
            q->next = pNew;
        }
        else if (pNew->count > q->next->count) {
            /*今回のデータが先頭のデータより値が大きい場合*/
            /*今回のデータを先頭のデータにする*/
            pNew->next = q->next;
            q->next = pNew;
        }
        else {
            /*それ以外の場合*/
            /*挿入位置を探索する*/
            while (q != NULL) {
                if (q->next != NULL) {
                    /*次のポインタが空でない場合*/
                    /*今回のデータを、現在のポインタと次のポインタのデータと比較する*/
                    if ((q->next->count < pNew->count) && (pNew->count <= q->count)) {
                        /*挿入処理*/
                        pNew->next = q->next;
                        q->next = pNew;
                        break;
                    }
                }
                else if (q->next == NULL) {
                    /*次のポインタが空（＝今回のデータはどのデータよりも小さい）場合*/
                    /*データを末尾に追加する*/
                    q->next = pNew;
                    break;
                }
                q = q->next;
            }
        }
        p = p->next;
    }

}

static void InsertSortForDsg(struct info_list* p, struct info_list* end)
{
    struct info_list* q = (struct info_list*)malloc(sizeof(struct info_list));
    if (q == NULL) {
        fputs("メモリ割り当てに失敗しました。", stderr);
        exit(1);
    }
    struct info_list* pNew = (struct info_list*)malloc(sizeof(struct info_list));
    if (pNew == NULL) {
        fputs("メモリ割り当てに失敗しました。", stderr);
        exit(1);
    }

    while (p != end->next) {
        pNew = MakeNewInfoList(p->info, p->count);
        q = &sHead;
        if (q->next == NULL) {
            /*リストが空の場合*/
            /*データを先頭に格納する*/
            q->next = pNew;
        }
        else if (pNew->count > q->next->count) {
            /*今回のデータが先頭のデータより値が大きい場合*/
            /*今回のデータを先頭のデータにする*/
            pNew->next = q->next;
            q->next = pNew;
        }
        else {
            /*それ以外の場合*/
            /*挿入位置を探索する*/
            while (q != NULL) {
                if (q->next != NULL) {
                    /*次のポインタが空でない場合*/
                    /*今回のデータを、現在のポインタと次のポインタのデータと比較する*/
                    if ((q->next->count < pNew->count) && (pNew->count <= q->count)) {
                        /*挿入処理*/
                        pNew->next = q->next;
                        q->next = pNew;
                        break;
                    }
                }
                else if (q->next == NULL) {
                    /*次のポインタが空（＝今回のデータはどのデータよりも小さい）場合*/
                    /*データを末尾に追加する*/
                    q->next = pNew;
                    break;
                }
                q = q->next;
            }
        }
        p = p->next;
    }
}

void InitArray(char* info)
{
    int i;
    for (i = 0; i < SIZE_OF_INFO; i++) {
        info[i] = '\0';
    }
}

void InitHeader(void)
{
    /*先頭にNULLを格納*/
    Head.next = NULL;
    sHead.next = NULL;
}

struct info_list* MakeNewInfoList(char info[SIZE_OF_INFO], int count) {
    struct info_list* p = (struct info_list*)malloc(sizeof(struct info_list));

    if (p == NULL) {
        fputs("メモリ割り当てに失敗しました。", stderr);
        exit(1);
    }
    strcpy(p->info, info);
    p->count = count;
    p->next = NULL;

    return p;
}

int OpenDirectory(char *dirpath) 
{
    /*ディレクトリの探索*/
    DIR* dir;
    struct dirent* dp;
    dir = opendir(dirpath);
    if (dir == NULL) {
        /*ディレクトリが存在しなかったら*/
        return -1;
    }
    printf("ファイル一覧\n");
    dp = readdir(dir);
    while (dp != NULL) {
        printf("%s\n", dp->d_name);
        dp = readdir(dir);
    }
    if (dir != NULL) {
        /*探索終了したら*/
        closedir(dir);
    }

    return 0;
}

void PrintData(int mode)
{
    FILE* fp;
    fp = fopen("output.log", "w");
    if (fp == NULL) {
        printf("出力ファイルを開けません\n");
        exit(1);
    }

    struct info_list* p = (struct info_list*)malloc(sizeof(struct info_list));
    if (p == NULL) {
        fputs("メモリ割り当てに失敗しました。", stderr);
        exit(1);
    }
    p = sHead.next;
    printf("\n");
    
    if (mode == TIME) {
        while (p != NULL) {
            fprintf(fp, "時間帯情報 : [%s] アクセス件数 : [%d]\n", p->info, p->count);
            p = p->next;
        }
    }
    else if (mode == HOST) {
        while (p != NULL) {
            fprintf(fp, "リモートホスト名 : [%s] アクセス件数 : [%d]\n", p->info, p->count);
            p = p->next;
        }
    }
    
}

void PrintExplain(void)
{
    printf("\nコマンドオプション\n");
    printf("%d : 期間指定してアクセス集計\n", DSG);
    printf("%d : 各時間帯毎のアクセス件数を表示\n", TIME);
    printf("%d : リモートホスト別のアクセス件数を表示\n", HOST);
    printf("%d : 終了\n", EXIT);
}

static int SearchFileName(char* filename, char* dirpath)
{
    /*指定したパスのディレクトリからファイル名を探索する関数*/
    /*見つかれば1を、見つからなければ-1を返す*/
    DIR* dir;
    struct dirent* dp;
    dir = opendir(dirpath);
    if (dir == NULL) {
        return -1;
    }
    dp = readdir(dir);
    while (dp != NULL) {
        if (strcmp(filename, dp->d_name) == 0) {
            return 1;
        }
        dp = readdir(dir);
    }
    if (dir != NULL) {
        closedir(dir);
    }
    return -1;
}

struct info_list* SearchLine(int year, int month, int day, int mode)
{
    struct info_list* p = Head.next;

    /*引数と同じデータ（year, month ,day）をもったリストのポインタを返す*/
    /*（year, month, day）すべての情報と一致するリストがなければNULLを返す*/
    while (p != NULL) {
        if (SplitDayMonthYear(p->info, year, month, day) == 1) {
            if (mode == END) {
                if (p->next != NULL) {
                    while (SplitDayMonthYear(p->info, year, month, day) == 1) {
                        p = p->next;
                    }
                }
                else {
                    return p;
                }
            }
            return p;
        }
        else {
            return NULL;
        }
        p = p->next;
    }
    return NULL;
}

int SplitDayMonthYear(char* time, int year, int month, int day)
{
    int i;
    int count = 0;
    int j = 0;
    char Day[3] = { 0 }, Month[4] = { 0 }, Year[5] = { 0 };

    /*時間帯情報の形式に着目してday, month, yearのデータを抽出*/
    for (i = 0; i < SIZE_OF_INFO; i++) {
        if (time[i] == '/') {
            j = 0;
            count++;
            continue;
        }
        else if (time[i] == ':') {
            break;
        }
        if (count == 0) {
            Day[j] = time[i];
            j++;
        }
        else if (count == 1) {
            Month[j] = time[i];
            j++;
        }
        else if (count == 2) {
            Year[j] = time[i];
            j++;
        }
    }

    /*文字変換して一致すれば1を、そうでなければ0を返す*/
    if (atoi(Day) == day && GetMonth(Month) == month && atoi(Year) == year) {
        return 1;
    }

    return 0;
}