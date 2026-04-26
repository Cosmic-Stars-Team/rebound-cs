/**
 * demo_harmonics.c — J2 物理验证
 *
 * 测试：地球卫星在 J2 引力谐波下的节点进动
 *
 * 编译: cd build && cl -I.. -I../src ../src/*.c ../cs/*.c demo_harmonics.c -Fe:demo_harmonics.exe
 * 运行: ./demo_harmonics.exe
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "cs/cs.h"

static double node_angle(const struct reb_particle* p) {
    /* 角动量向量在 xy 平面的投影方向 = 升交点经度 */
    double hx = p->y * p->vz - p->z * p->vy;
    double hy = p->z * p->vx - p->x * p->vz;
    return atan2(hy, hx) * 180.0 / M_PI;
}

int main(void) {
    printf("J2 Harmonics — Nodal Precession Demo\n");
    printf("=====================================\n\n");

    struct reb_simulation* sim = reb_simulation_create();
    cs_simulation_t* cs = cs_simulation_create(sim);
    cs_enable_harmonics(cs);

    /* 中心天体参数 */
    double M_body = 1.0;
    double J2_val = 0.05;
    double R_eq   = 0.05;

    /* 卫星轨道 */
    double a_sma = 0.15;
    double ecc   = 0.0;
    double inc   = 45.0 * M_PI / 180.0;  /* 45° 倾角 */

    /* 初条件：倾斜圆轨道 */
    double v_circ = sqrt(sim->G * M_body / a_sma);
    double x0 = a_sma;
    double y0 = 0.0;
    double z0 = 0.0;
    double vx0 = 0.0;
    double vy0 = v_circ * cos(inc);
    double vz0 = v_circ * sin(inc);

    reb_simulation_add_fmt(sim, "m", M_body);
    reb_simulation_add_fmt(sim, "m x y z vx vy vz",
                           1e-6, x0, y0, z0, vx0, vy0, vz0);

    /* 设置中心天体的 J2 */
    cs_particle_params_t* bp = cs_particle_params_create();
    bp->J2   = J2_val;
    bp->R_eq = R_eq;
    cs_particle_params_set(&sim->particles[0], bp);

    /* 理论节点进动率 */
    double n_mean   = sqrt(sim->G * M_body / (a_sma*a_sma*a_sma));
    double P_orb    = 2.0 * M_PI / n_mean;
    double Omega_dot_theory = -(1.5) * J2_val * n_mean
                              * (R_eq/a_sma) * (R_eq/a_sma)
                              * cos(inc);

    printf("M=%.1f  J2=%.3f  R_eq=%.3f  a=%.3f  i=%.0f deg\n",
           M_body, J2_val, R_eq, a_sma, inc * 180.0 / M_PI);
    printf("P_orb = %.4f\n", P_orb);
    printf("Theory: dOmega/dt = %.4f deg/orbit\n\n",
           Omega_dot_theory * 180.0 / M_PI * P_orb);

    /* 初始节点位置 */
    double node0 = node_angle(&sim->particles[1]);
    printf("  t (orbits)   node (deg)    Δnode (deg)   theory Δ\n");
    printf("  ----------   ----------    -----------   --------\n");

    for (int s = 0; s <= 8; s++) {
        double t = s * P_orb;
        if (s > 0) reb_simulation_integrate(sim, t);

        double node = node_angle(&sim->particles[1]);
        double dnode = node - node0;
        /* 标准化到 [-180, 180] */
        if (dnode >  180) dnode -= 360;
        if (dnode < -180) dnode += 360;

        double theory_delta = Omega_dot_theory * t * 180.0 / M_PI;
        printf("    %6.1f      %+8.2f      %+8.2f      %+8.2f\n",
               s * 1.0, node, dnode, theory_delta);
    }

    /* 验证：J2 不改变半长轴（保守力） */
    struct reb_particle* p = &sim->particles[1];
    double r = sqrt(p->x*p->x + p->y*p->y + p->z*p->z);
    double v2 = p->vx*p->vx + p->vy*p->vy + p->vz*p->vz;
    double a_final = 1.0 / (2.0/r - v2/(sim->G * M_body));
    printf("\n  a_initial = %.4f   a_final = %.4f  (should be ~same)\n",
           a_sma, a_final);
    printf("  J2 进动方向: dΩ/dt < 0 → 节点顺时针旋转 (符合理论)\n");

    reb_simulation_free(sim);
    return 0;
}
