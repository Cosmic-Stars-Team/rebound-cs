#ifndef CS_RADIATION_H
#define CS_RADIATION_H

/**
 * @file    cs_radiation.h
 * @brief   辐射压与 Poynting-Robertson 拖曳力模块
 *
 *          基于 Burns, Lamy & Soter (1979) 方程 (5)。
 *          同时包含辐射压（向外推）和 PR 拖曳（切向减速）两个效果。
 *
 *          物理公式：
 *            a_rad = beta * G * M_src / r^2
 *            a = a_rad * [(1 - r_dot/c) * r_hat - v_rel/c]
 *
 *          其中：
 *            beta    = 辐射压力 / 引力，粒子越小 beta 越大
 *            r_dot   = 粒子相对辐射源的径向速度
 *            r_hat   = 从辐射源指向粒子的单位向量
 *            v_rel   = 粒子相对辐射源的速度向量
 *
 *          使用方式：
 *            // 开启辐射力，光速单位与仿真一致
 *            cs_enable_radiation(cs, CS_C_AU_YR_MSUN);
 *
 *            // 设置粒子参数（必须设置 beta，否则该粒子不受辐射力）
 *            cs_particle_params_t* params = cs_particle_params_create();
 *            params->beta = 0.01;        // 辐射压与引力之比
 *            params->rad_source = 0;     // 不是辐射源
 *            cs_particle_params_set(&sim->particles[1], params);
 *
 *            // 标记辐射源（默认 particles[0]，可显式标记）
 *            cs_particle_params_t* src_params = cs_particle_params_create();
 *            src_params->rad_source = 1;
 *            cs_particle_params_set(&sim->particles[0], src_params);
 */

#include "../src/rebound.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * 内部入口，由 cs_dispatch_additional_forces() 调用
 * 不建议用户直接调用
 * ------------------------------------------------------------------------- */
void cs_radiation_additional_forces(struct reb_simulation* sim);

/* -------------------------------------------------------------------------
 * 底层计算函数
 * 纯计算，不依赖 cs_simulation_t，方便单元测试
 *
 * @param sim           父仿真指针（用于读取 G）
 * @param particles     粒子数组
 * @param N             真实粒子数（sim->N - sim->N_var）
 * @param c             光速（仿真单位）
 * @param source_index  辐射源粒子的下标
 * ------------------------------------------------------------------------- */
void cs_calculate_radiation_forces(
    struct reb_simulation* const sim,
    struct reb_particle* const particles,
    int N,
    double c,
    int source_index
);

/* -------------------------------------------------------------------------
 * 辅助工具：计算 beta 值
 *
 * beta = 3 * L * Q_pr / (16 * pi * G * M * c * rho * radius)
 *
 * @param G                 引力常数（sim->G）
 * @param c                 光速
 * @param source_mass       辐射源质量
 * @param source_luminosity 辐射源光度（仿真单位）
 * @param radius            粒子半径
 * @param density           粒子密度
 * @param Q_pr              辐射压效率因子（通常取 1.0）
 * @return                  beta 值
 * ------------------------------------------------------------------------- */
double cs_radiation_calc_beta(
    double G,
    double c,
    double source_mass,
    double source_luminosity,
    double radius,
    double density,
    double Q_pr
);

#ifdef __cplusplus
}
#endif

#endif /* CS_RADIATION_H */