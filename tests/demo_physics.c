/**
 * demo_physics.c — 物理验证
 *
 * Demo 1: GR 近日点进动 — 跟踪 Runge-Lenz 向量旋转
 * Demo 2: PR 拖曳 — 正确初条件，观测半长轴衰减
 * Demo 3: Newton vs GR vs Radiation 三方对比
 *
 * 编译: cd build && cl -I.. -I../src ../src/*.c ../cs/*.c demo_physics.c -Fe:demo.exe
 * 运行: ./demo.exe
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "cs/cs.h"

/* ----------------------------------------------------------------
 * 工具：计算 Runge-Lenz 向量的角度 (近日点方向)
 * ---------------------------------------------------------------- */
static double lrl_angle(const struct reb_particle* p, const struct reb_particle* star,
                        double mu) {
    double dx = p->x - star->x, dy = p->y - star->y, dz = p->z - star->z;
    double dvx = p->vx - star->vx, dvy = p->vy - star->vy, dvz = p->vz - star->vz;
    double r = sqrt(dx*dx + dy*dy + dz*dz);

    double hx = dy*dvz - dz*dvy;
    double hy = dz*dvx - dx*dvz;
    double hz = dx*dvy - dy*dvx;

    double ex = (dvy*hz - dvz*hy) / mu - dx/r;
    double ey = (dvz*hx - dvx*hz) / mu - dy/r;

    return atan2(ey, ex) * 180.0 / M_PI;
}

/* ----------------------------------------------------------------
 * 工具：从 x,y,vx,vy 计算 半长轴
 * ---------------------------------------------------------------- */
static double semi_major(const struct reb_particle* p, const struct reb_particle* star,
                         double mu) {
    double dx = p->x - star->x, dy = p->y - star->y, dz = p->z - star->z;
    double dvx = p->vx - star->vx, dvy = p->vy - star->vy, dvz = p->vz - star->vz;
    double r = sqrt(dx*dx + dy*dy + dz*dz);
    double v2 = dvx*dvx + dvy*dvy + dvz*dvz;
    return 1.0 / (2.0/r - v2/mu);
}

/* ================================================================
 * Demo 1: GR 近日点进动
 *
 * a=0.1, e=0.1, c=500, G=1, M=1
 * 理论: Δω = 6π*GM/(a*c^2*(1-e^2))
 *   = 18.85/(0.1*250000*0.99) = 18.85/24750 = 7.62e-4 rad/orbit
 *   = 0.0436 deg/orbit
 *
 * 积分 50 圈 → 累积约 2.18 deg，输出 LRL 角度进度。
 * ================================================================ */
static void demo_gr_precession(void) {
    printf("Demo 1: GR Perihelion Precession (LRL vector)\n");
    printf("----------------------------------------------\n");

    double c_test = 500.0, a0 = 0.1, e0 = 0.1;

    struct reb_simulation* sim = reb_simulation_create();
    cs_simulation_t* cs = cs_simulation_create(sim);
    cs_enable_gr(cs, CS_GR_POTENTIAL, c_test);

    double mu = sim->G * 1.0;
    double P  = 2.0 * M_PI * sqrt(a0*a0*a0 / mu);

    reb_simulation_add_fmt(sim, "m", 1.0);
    reb_simulation_add_fmt(sim, "m a e", 1e-6, a0, e0);

    double theory = 6.0 * M_PI * mu / (a0 * c_test * c_test * (1.0 - e0*e0));
    printf("a=%.2f  e=%.1f  c=%.0f  P=%.4f yr\n", a0, e0, c_test, P);
    printf("Theory: %.4f deg/orbit\n\n", theory * 180.0 / M_PI);

    /* 每 10 步记录一次 LRL 角度 */
    double ref_angle = lrl_angle(&sim->particles[1], &sim->particles[0], mu);
    printf("  t (yr)      LRL angle    Δ (deg)    predicted Δ\n");
    printf("  ------     ----------   ---------   -----------\n");

    for (int n = 0; n <= 5; n++) {
        double t_target = n * P * 10.0;
        if (n > 0) reb_simulation_integrate(sim, t_target);

        double ang = lrl_angle(&sim->particles[1], &sim->particles[0], mu);
        double delta = ang - ref_angle;
        /* 处理角度回绕 */
        if (delta < -180) delta += 360;
        if (delta >  180) delta -= 360;

        double predicted = theory * 180.0 / M_PI * 10.0 * n;
        printf("  %7.2f      %9.3f      %+7.3f      %+7.3f\n",
               t_target, ang, delta, predicted);
    }

    reb_simulation_free(sim);
}

/* ================================================================
 * Demo 2: PR 拖曳
 *
 * Particle 初始在修正后的圆轨道上 (v = sqrt(μ(1-β)/r))
 * 然后 PR 拖曳使半长轴缓慢下降。
 *
 * beta=0.02, c=2000, r0=0.5:
 * t_PR = c*r0^2/(4*μ*β) = 2000*0.25/(4*1*0.02) = 6250 yr
 * 积分 ~1000 yr 应见轻微衰减。
 * ================================================================ */
static void demo_pr_drag(void) {
    printf("\nDemo 2: Poynting-Robertson Drag\n");
    printf("--------------------------------\n");

    double c_test = 2000.0, beta = 0.02, r0 = 0.5;

    struct reb_simulation* sim = reb_simulation_create();
    cs_simulation_t* cs = cs_simulation_create(sim);
    cs_enable_radiation(cs, c_test);

    double mu_eff = sim->G * 1.0 * (1.0 - beta);
    double v_circ = sqrt(mu_eff / r0);
    double P_orb  = 2.0 * M_PI * r0 / v_circ;
    double t_pr   = c_test * r0 * r0 / (4.0 * sim->G * 1.0 * beta);

    reb_simulation_add_fmt(sim, "m", 1.0);
    reb_simulation_add_fmt(sim, "m x y z vx vy vz",
        1e-5, r0, 0.0, 0.0, 0.0, v_circ, 0.0);

    cs_particle_params_t* src = cs_particle_params_create();
    src->rad_source = 1;
    cs_particle_params_set(&sim->particles[0], src);

    cs_particle_params_t* dust = cs_particle_params_create();
    dust->beta = beta;
    cs_particle_params_set(&sim->particles[1], dust);

    double a0 = semi_major(&sim->particles[1], &sim->particles[0], sim->G);
    printf("beta=%.2f  c=%.0f  r0=%.1f  v_circ=%.4f  P=%.4f  t_PR=%.0f\n",
           beta, c_test, r0, v_circ, P_orb, t_pr);
    printf("a0=%.6f\n\n", a0);

    printf("  t (yr)      a (AU)      da/a0\n");
    printf("  ------     --------    --------\n");

    for (int s = 0; s <= 8; s++) {
        double t = s * P_orb * 3.0;
        if (s > 0) reb_simulation_integrate(sim, t);

        double a_s = semi_major(&sim->particles[1], &sim->particles[0], sim->G);
        printf("  %7.1f    %9.6f   %+9.2e\n", t, a_s, (a_s - a0)/a0);
    }

    printf("\n  da < 0 → PR drag 有效\n");

    reb_simulation_free(sim);
}

/* ================================================================
 * Demo 3: Newton vs GR(Full) vs Radiation — 3P 对比
 * ================================================================ */
static void demo_comparison(void) {
    printf("\nDemo 3: Newton vs GR-Full vs Radiation\n");
    printf("--------------------------------------\n");

    const char* labels[] = {"Newton only", "GR Full", "Radiation beta=0.2"};

    for (int mode = 0; mode < 3; mode++) {
        struct reb_simulation* sim = reb_simulation_create();
        cs_simulation_t* cs = cs_simulation_create(sim);

        if (mode == 1)
            cs_enable_gr(cs, CS_GR_FULL, 1000.0);
        if (mode == 2)
            cs_enable_radiation(cs, 1000.0);

        double a0 = 0.2, e0 = 0.5;
        reb_simulation_add_fmt(sim, "m", 1.0);
        reb_simulation_add_fmt(sim, "m a e", 1e-6, a0, e0);

        /* 加第二个小质量行星，测试 GR Full 的多体处理 */
        if (mode == 1) {
            reb_simulation_add_fmt(sim, "m a e", 1e-8, 0.5, 0.2);
        }

        if (mode == 2) {
            cs_particle_params_t* sp = cs_particle_params_create();
            sp->rad_source = 1;
            cs_particle_params_set(&sim->particles[0], sp);

            cs_particle_params_t* pp = cs_particle_params_create();
            pp->beta = 0.2;
            cs_particle_params_set(&sim->particles[1], pp);
        }

        double P = 2.0 * M_PI * sqrt(a0*a0*a0 / (sim->G * 1.0));
        reb_simulation_integrate(sim, P * 5.0);

        double mu = sim->G * sim->particles[0].m;
        double a_f = semi_major(&sim->particles[1], &sim->particles[0], mu);

        printf("  %-22s t=%.3f  a=%.4f AU  (start: %.2f)\n",
               labels[mode], sim->t, a_f, a0);

        reb_simulation_free(sim);
    }

    printf("\n  Newton    : a = 0.2 不变\n");
    printf("  GR Full   : a 基本不变，近日点进动 (且含多体 PN 效应)\n");
    printf("  Radiation : a 应缓慢减小\n");
}

int main(void) {
    printf("Cosmic Stars — Physics Demos\n");
    printf("=============================\n\n");
    demo_gr_precession();
    demo_pr_drag();
    demo_comparison();
    printf("\nDone.\n");
    return 0;
}
