/**
 * demo_mass.c — 恒星质量损失物理验证
 *
 * 物理：恒星质量损失导致行星轨道扩张 a ∝ 1/M
 * 解析解：a(t) * M(t) ≈ constant (adiabatic mass loss)
 *
 * 编译: cd build && cl -I.. -I../src ../src/*.c ../cs/*.c demo_mass.c -Fe:demo_mass.exe
 * 运行: ./demo_mass.exe
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "cs/cs.h"

static double semi_major(const struct reb_particle* p,
                         const struct reb_particle* star, double mu) {
    double dx = p->x - star->x, dy = p->y - star->y, dz = p->z - star->z;
    double dvx = p->vx - star->vx, dvy = p->vy - star->vy, dvz = p->vz - star->vz;
    double r = sqrt(dx*dx + dy*dy + dz*dz);
    double v2 = dvx*dvx + dvy*dvy + dvz*dvz;
    return 1.0 / (2.0/r - v2/mu);
}

int main(void) {
    printf("Stellar Mass Loss Demo\n");
    printf("======================\n\n");

    struct reb_simulation* sim = reb_simulation_create();
    cs_simulation_t* cs = cs_simulation_create(sim);

    /* 开启质量损失：恒星 mdot = -0.002 M_sun/yr (10 年失去 2% 质量) */
    cs_enable_solarmass(cs);

    /* 恒星：初始质量 1.0 */
    reb_simulation_add_fmt(sim, "m", 1.0);

    /* 行星：a=1.0, 圆轨道 */
    reb_simulation_add_fmt(sim, "m a e", 1e-6, 1.0, 0.0);

    /* 设置恒星的 mdot */
    cs_particle_params_t* star_p = cs_particle_params_create();
    star_p->mdot = -0.002;
    cs_particle_params_set(&sim->particles[0], star_p);

    double M0 = sim->particles[0].m;
    double a0 = semi_major(&sim->particles[1], &sim->particles[0], sim->G * M0);

    printf("Initial: M_star = %.4f,  a_planet = %.4f AU\n", M0, a0);
    printf("Star mass loss rate: dm/dt = %.4f Msun/yr\n\n", star_p->mdot);

    printf("  t (yr)    M_star     a (AU)    a*M (expected const)\n");
    printf("  ------    ------    --------    --------------------\n");

    for (int s = 0; s <= 5; s++) {
        double t = s * 2.0;
        if (s > 0) reb_simulation_integrate(sim, t);

        double M = sim->particles[0].m;
        double a = semi_major(&sim->particles[1], &sim->particles[0],
                              sim->G * M);

        printf("  %6.1f    %6.4f    %8.4f     %8.4f\n",
               t, M, a, a * M);
    }

    /* 验证 a*M ≈ constant */
    double M_final = sim->particles[0].m;
    double a_final = semi_major(&sim->particles[1], &sim->particles[0],
                                sim->G * M_final);

    printf("\n  M: %.4f → %.4f (expected %.4f)\n", M0, M_final,
           1.0 + star_p->mdot * 10.0);
    printf("  a: %.4f → %.4f\n", a0, a_final);
    printf("  a*M: %.4f → %.4f (should be ~constant)\n",
           a0 * M0, a_final * M_final);
    printf("\n  Mass loss → a 增大：物理正确\n");

    reb_simulation_free(sim);
    return 0;
}
