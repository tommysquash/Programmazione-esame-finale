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