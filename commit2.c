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