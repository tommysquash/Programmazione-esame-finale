

/* ── COMMIT 1 ── include, costanti, strutture ─────────────────────── */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define MAX_SENSORS  15
#define MAX_SAMPLES  10000
#define CSV_FILE     "log_monitoraggio_25-07-2022.csv"
#define HDR_LINES    4
#define MAX_COLS     12

typedef struct { time_t ts; double val; } Sample;

/* Stats: 5 grandezze — mean, std_dev, min, max, avg_variation */
typedef struct { double mean, std, mn, mx, avgv; } Stats;

typedef struct { char name[64]; Sample s[MAX_SAMPLES]; int n; Stats st; } Sensor;

static Sensor sensors[MAX_SENSORS];
static int    nsensors = 0;

/* split: tokenizer custom (strtok salta ',,' iniziali sfasando gli indici) */
static int split(char *line, char **tok, int max) {
    int c = 0; tok[c++] = line;
    for (char *p = line; *p && c < max; p++)
        if (*p == ',') { *p = '\0'; tok[c++] = p+1; }
    return c;
}

/* ts_parse: ISO8601 "2025-07-22T00:00:07.223108Z" -> time_t via sscanf
   sscanf si ferma al '.' ignorando microsecondi; struct tm: year-=1900, mon-=1 */
static time_t ts_parse(const char *s) {
    struct tm t = {0};
    if (sscanf(s,"%d-%d-%dT%d:%d:%d",&t.tm_year,&t.tm_mon,&t.tm_mday,
               &t.tm_hour,&t.tm_min,&t.tm_sec) != 6) return (time_t)-1;
    t.tm_year -= 1900; t.tm_mon -= 1; t.tm_isdst = -1;
    return mktime(&t);
}

/* find_sensor: cerca per nome; se assente lo crea in coda */
static int find_sensor(const char *name) {
    for (int i = 0; i < nsensors; i++)
        if (!strcmp(sensors[i].name, name)) return i;
    if (nsensors >= MAX_SENSORS) { fputs("[ERR] MAX_SENSORS\n",stderr); exit(1); }
    strncpy(sensors[nsensors].name, name, 63);
    sensors[nsensors].n = 0;
    return nsensors++;
}

/* load_csv: salta HDR_LINES header, per ogni riga estrae ts/val/sensore
   COMMIT 2 — FIX anomalia: accel_x con val>+5 ha segno invertito (glitch
   firmware); il modulo è corretto, si nega il valore invece di scartarlo */
static void load_csv(void) {
    FILE *fp = fopen(CSV_FILE,"r");
    if (!fp) { perror("fopen"); exit(1); }
    char line[512], *tok[MAX_COLS];
    for (int h = 0; h < HDR_LINES; h++) fgets(line, sizeof line, fp);
    int tot = HDR_LINES, skip = 0;
    while (fgets(line, sizeof line, fp)) {
        tot++;
        line[strcspn(line,"\r\n")] = '\0';
        if (!line[0] || line[0]=='#') { skip++; continue; }
        char row[512]; strncpy(row, line, sizeof row-1);
        int n = split(row, tok, MAX_COLS);
        if (n<11 || !strcmp(tok[5],"_time")) { skip++; continue; }
        time_t ts = ts_parse(tok[5]);
        double val = strtod(tok[6], NULL);
        if (ts==(time_t)-1) { skip++; continue; }
        int idx = find_sensor(tok[10]);
        if (sensors[idx].n >= MAX_SAMPLES) continue;
        /* FIX Problema 2: accel_x positivo > 5 m/s² è impossibile per un
           frigo fermo (gravità su asse X è ~-9.73 m/s²); inverte il segno */
        if (!strcmp(sensors[idx].name,"fridge_black_accel_x") && val>5.0) {
            printf("[FIX] %s: %.4f -> %.4f\n", sensors[idx].name, val, -val);
            val = -val;
        }
        sensors[idx].s[sensors[idx].n++] = (Sample){ts, val};
    }
    fclose(fp);
    printf("[INFO] righe:%d skip:%d sensori:%d\n\n", tot, skip, nsensors);
}

/* ── COMMIT 2 ── funzioni statistiche ────────────────────────────── */

/* mean = (1/N)*SUM(x_i) */
static double mean(Sensor *s) {
    double r=0; for(int i=0;i<s->n;i++) r+=s->s[i].val; return r/s->n;
}

/* std_dev POPOLAZIONE = SQRT((1/N)*SUM((x_i-mean)^2))
   Divisore N (non N-1): si dispone dell'intera serie, non di un campione */
static double stddev(Sensor *s, double m) {
    double r=0; for(int i=0;i<s->n;i++){double d=s->s[i].val-m;r+=d*d;}
    return sqrt(r/s->n);
}

/* min/max: scansione lineare con init dal primo campione */
static void minmax(Sensor *s, double *mn, double *mx) {
    *mn=*mx=s->s[0].val;
    for(int i=1;i<s->n;i++){
        if(s->s[i].val<*mn)*mn=s->s[i].val;
        if(s->s[i].val>*mx)*mx=s->s[i].val;
    }
}

/* avg_variation = (1/(N-1))*SUM(|x_{i+1}-x_i|)
   fabs() necessario: senza di esso variazioni ± si cancellerebbero */
static double avgvar(Sensor *s) {
    if(s->n<2) return 0;
    double r=0; for(int i=0;i<s->n-1;i++) r+=fabs(s->s[i+1].val-s->s[i].val);
    return r/(s->n-1);
}

static void compute_stats(void) {
    for(int i=0;i<nsensors;i++){
        Sensor *s=&sensors[i]; if(!s->n) continue;
        s->st.mean=mean(s); s->st.std=stddev(s,s->st.mean);
        minmax(s,&s->st.mn,&s->st.mx); s->st.avgv=avgvar(s);
    }
}

/* ── COMMIT 3 ── output formattato ───────────────────────────────── */

/* print_results: per ogni sensore stampa intervallo temporale e 5 stat.
   %-28s/%14.6f: larghezze fisse per allineare verticalmente le colonne.
   localtime() puntatore statico: si copia la prima struct tm prima
   della seconda chiamata, altrimenti la seconda sovrascrive la prima */
static void print_results(void) {
    for(int i=0;i<70;i++) putchar('='); puts("");
    puts("  ANALISI STATISTICA SENSORI — log_monitoraggio_25-07-2022.csv");
    for(int i=0;i<70;i++) putchar('='); puts("\n");
    for(int i=0;i<nsensors;i++){
        Sensor *s=&sensors[i]; if(!s->n) continue;
        printf("  SENSORE [%d/%d]: %s  (%d campioni)\n",i+1,nsensors,s->name,s->n);
        char t0[32],t1[32]; struct tm c,*p;
        p=localtime(&s->s[0].ts);     c=*p; strftime(t0,sizeof t0,"%Y-%m-%d %H:%M:%S",&c);
        p=localtime(&s->s[s->n-1].ts);      strftime(t1,sizeof t1,"%Y-%m-%d %H:%M:%S",p);
        printf("  Intervallo: %s  -->  %s\n\n",t0,t1);
        printf("    %-28s %14.6f\n","1. Media aritmetica:",   s->st.mean);
        printf("    %-28s %14.6f\n","2. Deviazione standard:",s->st.std);
        printf("    %-28s %14.6f\n","3. Valore minimo:",      s->st.mn);
        printf("    %-28s %14.6f\n","4. Valore massimo:",     s->st.mx);
        printf("    %-28s %14.6f\n","5. Variazione media:",   s->st.avgv);
        puts(""); for(int j=0;j<70;j++) putchar('='); puts("\n");
    }
}

int main(void) {
    load_csv();        /* Commit 1+2: import + fix anomalia */
    compute_stats();   /* Commit 2:   calcolo 5 statistiche */
    print_results();   /* Commit 3:   output formattato     */
    return 0;
}