/**
 * test_cs.c — Cosmic Stars 功能与物理正确性验证
 *
 * 测试:
 *   1. 生命周期（create/free，重复 create 会被拒）
 *   2. GR Potential — 验证近日点进动率符合理论值
 *   3. J2 谐波     — 验证节点进动率符合理论值
 *   4. Radiation   — 验证 Poynting-Robertson 拖曳使半长轴衰减
 *   5. Tides       — 验证潮汐作用使偏心率衰减
 *   6. 多模块组合  — GR + Radiation 联合运行不崩溃
 *   7. disable / re-enable 状态切换
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "cs/cs.h"

static int passed = 0;
static int failed = 0;

#define T(cond, msg) do { \
    if (cond) { passed++; printf("  [PASS] %s\n", msg); } \
    else      { failed++; printf("  [FAIL] %s\n", msg); } \
} while(0)

/* ---- 工具函数 ---- */

/* Runge-Lenz 向量角度（近日点经度） */
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
    return atan2(ey, ex);
}

/* 升交点经度（从角动量向量 xy 投影） */
static double node_angle(const struct reb_particle* p) {
    double hx = p->y * p->vz - p->z * p->vy;
    double hy = p->z * p->vx - p->x * p->vz;
    return atan2(hy, hx);
}

/* 半长轴 */
static double semi_major(const struct reb_particle* p, const struct reb_particle* star,
                         double mu) {
    double dx = p->x - star->x, dy = p->y - star->y, dz = p->z - star->z;
    double dvx = p->vx - star->vx, dvy = p->vy - star->vy, dvz = p->vz - star->vz;
    double r = sqrt(dx*dx + dy*dy + dz*dz);
    double v2 = dvx*dvx + dvy*dvy + dvz*dvz;
    return 1.0 / (2.0/r - v2/mu);
}

/* 偏心率 */
static double eccentricity(const struct reb_particle* p, const struct reb_particle* star,
                           double mu) {
    double dx = p->x - star->x, dy = p->y - star->y, dz = p->z - star->z;
    double dvx = p->vx - star->vx, dvy = p->vy - star->vy, dvz = p->vz - star->vz;
    double r = sqrt(dx*dx + dy*dy + dz*dz);
    double v2 = dvx*dvx + dvy*dvy + dvz*dvz;
    double a = 1.0 / (2.0/r - v2/mu);
    double hx = dy*dvz - dz*dvy;
    double hy = dz*dvx - dx*dvz;
    double hz = dx*dvy - dy*dvx;
    double h2 = hx*hx + hy*hy + hz*hz;
    return sqrt(1.0 - h2/(mu*a));
}

/* =================================================================
 * Test 1: 生命周期与空指针保护
 * ================================================================= */
static void test_lifecycle(void) {
    printf("\n--- Test 1: Lifecycle ---\n");

    struct reb_simulation* sim = reb_simulation_create();
    T(sim != NULL, "reb_simulation_create");

    cs_simulation_t* cs = cs_simulation_create(sim);
    T(cs != NULL, "cs_simulation_create");
    T(sim->extras == cs, "sim->extras == cs");

    /* 重复 create 应该被拒绝 */
    cs_simulation_t* cs2 = cs_simulation_create(sim);
    T(cs2 == NULL, "duplicate cs_simulation_create rejected");

    reb_simulation_free(sim);  /* cs 随 sim 自动释放 */

    /* NULL 防护 */
    cs_simulation_create(NULL);
    cs_simulation_free(NULL);
    cs_enable_gr(NULL, CS_GR_POTENTIAL, 1.0);
    cs_enable_radiation(NULL, 1.0);
    T(1, "NULL guards don't crash");
}

/* =================================================================
 * Test 2: GR Potential — 近日点进动率
 *
 * 理论: Δω = 6π·GM / (a·c²·(1-e²))  per orbit
 * 使用小 c 放大相对论效应，N 圈后 LRL 向量旋转量应与理论一致。
 * ================================================================= */
static void test_gr_precession(void) {
    printf("\n--- Test 2: GR Potential precession ---\n");

    double c_test = 500.0, a0 = 0.2, e0 = 0.1;
    int n_orbits = 5;

    struct reb_simulation* sim = reb_simulation_create();
    cs_simulation_t* cs = cs_simulation_create(sim);
    cs_enable_gr(cs, CS_GR_POTENTIAL, c_test);

    double mu = sim->G * 1.0;
    double P  = 2.0 * M_PI * sqrt(a0*a0*a0 / mu);

    reb_simulation_add_fmt(sim, "m", 1.0);
    reb_simulation_add_fmt(sim, "m a e", 1e-6, a0, e0);

    double theory_per_orbit = 6.0 * M_PI * mu / (a0 * c_test * c_test * (1.0 - e0*e0));
    double theory_total = theory_per_orbit * n_orbits;

    double ref_angle = lrl_angle(&sim->particles[1], &sim->particles[0], mu);
    reb_simulation_integrate(sim, P * n_orbits);

    double delta = lrl_angle(&sim->particles[1], &sim->particles[0], mu) - ref_angle;

    /* 角度标准化到 [-π, π] */
    delta = fmod(delta + M_PI, 2.0 * M_PI);
    if (delta < 0) delta += 2.0 * M_PI;
    delta -= M_PI;

    double err_frac = fabs(delta - theory_total) / theory_total;
    T(err_frac < 0.15, "GR precession rate within 15% of theory");

    printf("        theory: %+.6f rad  measured: %+.6f rad  err: %.2f%%\n",
           theory_total, delta, err_frac * 100.0);

    reb_simulation_free(sim);
}

/* =================================================================
 * Test 3: J2 谐波 — 节点进动
 *
 * 理论: dΩ/dt = -1.5·J2·n·(R_eq/a)²·cos(i)
 * 倾斜圆轨道，N 圈后验证节点漂移。
 * ================================================================= */
static void test_j2_precession(void) {
    printf("\n--- Test 3: J2 nodal precession ---\n");

    struct reb_simulation* sim = reb_simulation_create();
    cs_simulation_t* cs = cs_simulation_create(sim);
    cs_enable_harmonics(cs);

    double M_body = 1.0, J2_val = 0.05, R_eq = 0.05;
    double a_sma = 0.15, inc = 45.0 * M_PI / 180.0;
    double v_circ = sqrt(sim->G * M_body / a_sma);
    int n_orbits = 5;

    reb_simulation_add_fmt(sim, "m", M_body);
    reb_simulation_add_fmt(sim, "m x y z vx vy vz",
                           1e-6, a_sma, 0.0, 0.0, 0.0, v_circ * cos(inc), v_circ * sin(inc));

    cs_particle_params_t* bp = cs_particle_params_create();
    bp->J2   = J2_val;
    bp->R_eq = R_eq;
    cs_particle_params_set(&sim->particles[0], bp);

    double n_mean = sqrt(sim->G * M_body / (a_sma*a_sma*a_sma));
    double P_orb  = 2.0 * M_PI / n_mean;
    double omega_dot_theory = -1.5 * J2_val * n_mean
                              * (R_eq / a_sma) * (R_eq / a_sma) * cos(inc);

    double node0 = node_angle(&sim->particles[1]);
    reb_simulation_integrate(sim, P_orb * n_orbits);
    double dnode = node_angle(&sim->particles[1]) - node0;

    double theory_dnode = omega_dot_theory * P_orb * n_orbits;
    double err_frac = fabs(dnode - theory_dnode) / fabs(theory_dnode);

    T(err_frac < 0.10, "J2 nodal precession rate within 10% of theory");
    T(dnode < 0,       "J2 precession is retrograde (dOmega/dt < 0)");
    printf("        theory: %+.6f rad  measured: %+.6f rad  err: %.2f%%\n",
           theory_dnode, dnode, err_frac * 100.0);

    /* J2 是保守力，半长轴应守恒 */
    double a_final = semi_major(&sim->particles[1], &sim->particles[0], sim->G * M_body);
    T(fabs(a_final - a_sma) / a_sma < 1e-4, "J2 conserves semi-major axis");

    reb_simulation_free(sim);
}

/* =================================================================
 * Test 4: 辐射 PR 拖曳 — 半长轴衰减
 *
 * beta > 0 的尘埃粒子在 PR 拖曳下半长轴应减小。验证 da < 0。
 * ================================================================= */
static void test_pr_drag(void) {
    printf("\n--- Test 4: Radiation PR drag ---\n");

    double c_test = 500.0, beta = 0.1, r0 = 0.5;

    struct reb_simulation* sim = reb_simulation_create();
    cs_simulation_t* cs = cs_simulation_create(sim);
    cs_enable_radiation(cs, c_test);

    double mu_eff = sim->G * 1.0 * (1.0 - beta);
    double v_circ = sqrt(mu_eff / r0);
    double P_orb  = 2.0 * M_PI * r0 / v_circ;

    reb_simulation_add_fmt(sim, "m", 1.0);
    reb_simulation_add_fmt(sim, "m x y z vx vy vz",
                           1e-5, r0, 0.0, 0.0, 0.0, v_circ, 0.0);

    cs_particle_params_t* src = cs_particle_params_create();
    src->rad_source = 1;
    cs_particle_params_set(&sim->particles[0], src);

    cs_particle_params_t* dust = cs_particle_params_create();
    dust->beta = beta;
    cs_particle_params_set(&sim->particles[1], dust);

    double a0 = semi_major(&sim->particles[1], &sim->particles[0], sim->G * 1.0);
    reb_simulation_integrate(sim, P_orb * 20.0);
    double a1 = semi_major(&sim->particles[1], &sim->particles[0], sim->G * 1.0);

    T(a1 < a0, "PR drag decreases semi-major axis");
    printf("        a0=%.6f  a1=%.6f  da=%.2e\n", a0, a1, a1 - a0);

    reb_simulation_free(sim);
}

/* =================================================================
 * Test 5: 潮汐 — 偏心率衰减
 *
 * Close-in 偏心轨道，潮汐应使 e 减小。
 * 与无潮汐的 baseline 对比。
 * ================================================================= */
static void test_tides(void) {
    printf("\n--- Test 5: Tidal eccentricity damping ---\n");

    double a0 = 0.08, e0 = 0.3;

    /* Baseline: 无潮汐 */
    struct reb_simulation* sim0 = reb_simulation_create();
    cs_simulation_create(sim0);
    reb_simulation_add_fmt(sim0, "m", 1.0);
    reb_simulation_add_fmt(sim0, "m a e", 0.002, a0, e0);

    double GM = sim0->G * 1.0;
    double P = 2.0 * M_PI * sqrt(a0*a0*a0 / GM);
    reb_simulation_integrate(sim0, P * 6.0);
    double e_baseline = eccentricity(&sim0->particles[1], &sim0->particles[0], GM);
    reb_simulation_free(sim0);

    /* Test: 有潮汐 */
    struct reb_simulation* sim1 = reb_simulation_create();
    cs_simulation_t* cs = cs_simulation_create(sim1);
    cs_enable_tides(cs);

    reb_simulation_add_fmt(sim1, "m", 1.0);
    reb_simulation_add_fmt(sim1, "m a e", 0.002, a0, e0);

    cs_particle_params_t* tp = cs_particle_params_create();
    tp->tides_k2  = 0.5;
    tp->tides_tau = 0.3;
    tp->tides_R   = 0.002;
    cs_particle_params_set(&sim1->particles[1], tp);

    reb_simulation_integrate(sim1, P * 6.0);
    double e_tides = eccentricity(&sim1->particles[1], &sim1->particles[0], GM);

    T(e_tides < e0,        "Tides reduce eccentricity (e < e0)");
    T(e_tides < e_baseline,"Tides reduce eccentricity more than Newton baseline");
    printf("        e0=%.4f  e_no_tides=%.4f  e_tides=%.4f\n", e0, e_baseline, e_tides);

    reb_simulation_free(sim1);
}

/* =================================================================
 * Test 6: GR + Radiation 组合
 * ================================================================= */
static void test_multi_module(void) {
    printf("\n--- Test 6: GR + Radiation combined ---\n");

    struct reb_simulation* sim = reb_simulation_create();
    cs_simulation_t* cs = cs_simulation_create(sim);

    cs_enable_gr(cs, CS_GR_POTENTIAL, CS_C_AU_YR_MSUN);
    cs_enable_radiation(cs, CS_C_AU_YR_MSUN);
    T((cs->modules & CS_MODULE_GR_POTENTIAL) && (cs->modules & CS_MODULE_RADIATION),
      "both modules active");

    reb_simulation_add_fmt(sim, "m", 1.0);
    reb_simulation_add_fmt(sim, "m a e", 1e-10, 2.0, 0.1);

    cs_particle_params_t* src = cs_particle_params_create();
    src->rad_source = 1;
    cs_particle_params_set(&sim->particles[0], src);

    cs_particle_params_t* dust = cs_particle_params_create();
    dust->beta = 0.05;
    cs_particle_params_set(&sim->particles[1], dust);

    reb_simulation_integrate(sim, 0.5);
    double r = sqrt(sim->particles[1].x * sim->particles[1].x
                  + sim->particles[1].y * sim->particles[1].y);
    T(r > 0.5 && r < 5.0, "particle survives combined forces");

    reb_simulation_free(sim);
}

/* =================================================================
 * Test 7: disable / re-enable 状态切换
 * ================================================================= */
static void test_disable(void) {
    printf("\n--- Test 7: Disable / Re-enable ---\n");

    struct reb_simulation* sim = reb_simulation_create();
    cs_simulation_t* cs = cs_simulation_create(sim);

    cs_enable_radiation(cs, CS_C_AU_YR_MSUN);
    T(sim->force_is_velocity_dependent == 1, "vel_dep = 1");

    cs_disable_radiation(cs);
    T(!(cs->modules & CS_MODULE_RADIATION), "radiation bit cleared");
    T(sim->force_is_velocity_dependent == 0, "vel_dep back to 0");

    /* 含另一个速度依赖模块时不应清零 */
    cs_enable_gr(cs, CS_GR_SINGLE, CS_C_AU_YR_MSUN);
    cs_enable_radiation(cs, CS_C_AU_YR_MSUN);
    cs_disable_radiation(cs);
    T(sim->force_is_velocity_dependent == 1, "vel_dep stays 1 (GR still active)");

    /* GR mode 切换 */
    cs_enable_gr(cs, CS_GR_POTENTIAL, CS_C_AU_YR_MSUN);
    T(cs->modules & CS_MODULE_GR_POTENTIAL, "switched to POTENTIAL");
    T(!(cs->modules & CS_MODULE_GR), "SINGLE cleared");

    cs_disable_gr(cs);
    T(!(cs->modules & (CS_MODULE_GR_POTENTIAL | CS_MODULE_GR | CS_MODULE_GR_FULL)),
      "all GR bits cleared");

    reb_simulation_free(sim);
}

/* =================================================================
 * main
 * ================================================================= */
int main(void) {
    printf("Cosmic Stars — Test Suite\n"
           "========================\n");

    test_lifecycle();
    test_gr_precession();
    test_j2_precession();
    test_pr_drag();
    test_tides();
    test_multi_module();
    test_disable();

    printf("\n========================\n");
    printf("%d passed, %d failed\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
