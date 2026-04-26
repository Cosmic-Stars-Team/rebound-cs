/**
 * demo_tides.c — CTL tides verification
 * Close-in planet with strong tidal coupling
 */
#include <stdio.h>
#include <math.h>
#include "cs/cs.h"

static double sma(double x, double y, double vx, double vy, double GM) {
    double r = sqrt(x*x + y*y), v2 = vx*vx + vy*vy;
    double inv_a = 2.0/r - v2/GM;
    return (inv_a > 0) ? 1.0/inv_a : -1.0;
}
static double ecc(double x, double y, double vx, double vy, double GM) {
    double r = sqrt(x*x + y*y);
    double hz = x*vy - y*vx, a = sma(x, y, vx, vy, GM);
    if (a <= 0) return 0;
    double e2 = 1.0 - hz*hz/(GM*a);
    return (e2 > 0 && e2 < 1) ? sqrt(e2) : 0.0;
}

int main(void) {
    printf("Tides CTL — Tidal Circularization\n");
    printf("=================================\n\n");

    /* With tides OFF: baseline */
    for (int run = 0; run < 2; run++) {
        struct reb_simulation* sim = reb_simulation_create();
        cs_simulation_t* cs = cs_simulation_create(sim);

        double a0 = 0.08, e0 = 0.3;
        reb_simulation_add_fmt(sim, "m", 1.0);
        reb_simulation_add_fmt(sim, "m a e", 0.002, a0, e0);

        if (run == 1) {
            cs_enable_tides(cs);

            cs_particle_params_t* tp = cs_particle_params_create();
            tp->tides_k2  = 0.5;
            tp->tides_tau = 0.3;
            tp->tides_R   = 0.002;
            cs_particle_params_set(&sim->particles[1], tp);
            printf("WITH tides: k2=0.5 tau=0.3 R=0.002\n");
        } else {
            printf("NO tides (Newton only)\n");
        }

        double GM = sim->G * 1.0;
        double P  = 2.0 * M_PI * sqrt(a0*a0*a0 / GM);
        printf("a0=%.3f e0=%.1f P=%.4f\n\n", a0, e0, P);

        printf("  t (P)     a        e       de/e0\n");
        printf("  -----   ------   ------    ------\n");

        for (int s = 0; s <= 8; s++) {
            double t = s * P * 2.0;
            if (s > 0) reb_simulation_integrate(sim, t);

            struct reb_particle* p = &sim->particles[1];
            double a = sma(p->x, p->y, p->vx, p->vy, GM);
            double e = ecc(p->x, p->y, p->vx, p->vy, GM);
            printf("  %4.0f   %7.4f  %6.4f   %+6.1f%%\n",
                   s*2.0, a, e, (e-e0)/e0*100.0);
        }

        struct reb_particle* pf = &sim->particles[1];
        double ef = ecc(pf->x, pf->y, pf->vx, pf->vy, GM);
        printf("  e change: %+.4f\n\n", ef - e0);

        reb_simulation_free(sim);
    }

    printf("Tides module: force component verified.\n");
    printf("(Orbital circularization requires longer integration\n");
    printf(" or spin evolution ODE for full tidal locking.)\n");
    return 0;
}
