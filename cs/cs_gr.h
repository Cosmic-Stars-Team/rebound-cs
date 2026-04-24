#ifndef CS_GR_H
#define CS_GR_H

/**
 * @file    cs_gr.h
 * @brief   General Relativity corrections for Cosmic Stars
 *
 *          Three modes available, in order of increasing accuracy and cost:
 *
 *          CS_GR_POTENTIAL  (Nobili & Roxburgh 1986)
 *              Adds a simple -3(GM)^2 / (c^2 r^4) potential.
 *              Gets pericenter precession right, mean motion slightly off.
 *              NOT velocity-dependent -> safe with WHFast (symplectic).
 *              Cost: O(N).  Best for: single-star planet systems, fast runs.
 *
 *          CS_GR_SINGLE  (Anderson et al. 1975)
 *              Full first-order post-Newtonian correction from one dominant
 *              source body (particles[0]).  Velocity-dependent.
 *              Gets both mean motion and precession correct.
 *              Cost: O(N).  Best for: planetary systems around one star.
 *
 *          CS_GR_FULL  (Newhall et al. 1983)
 *              First-order post-Newtonian corrections between ALL pairs of
 *              bodies.  Required when multiple bodies have comparable masses
 *              (stellar binaries, multi-star systems).
 *              Velocity-dependent.  Solves implicit equation iteratively.
 *              Cost: O(N^3) x max_iter.  Best for: multi-star / binary systems.
 */

#include "../src/rebound.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Internal entry point called by cs_dispatch_additional_forces().
 * Reads cs_simulation_t from sim->extras and dispatches to the correct
 * gr variant based on cs->gr_mode.
 * Not intended to be called directly by users.
 * ------------------------------------------------------------------------- */
void cs_gr_additional_forces(struct reb_simulation* sim);

/* -------------------------------------------------------------------------
 * Low-level calculation functions.
 * These are pure computation — no dependency on cs_simulation_t.
 * Useful if you want to call them standalone or write unit tests.
 *
 * Parameters common to all three:
 *   particles   pointer to the simulation particle array
 *   N           number of real particles (sim->N - sim->N_var)
 *   C2          speed of light squared (c * c), in simulation units
 *   G           gravitational constant from sim->G
 * ------------------------------------------------------------------------- */

/**
 * CS_GR_POTENTIAL — simple post-Newtonian potential correction.
 * Assumes particles[0] is the dominant central body.
 * Adds acceleration corrections directly to particles[i].ax/ay/az.
 *
 * Physics:
 *   a_GR = -6 (G M)^2 / (c^2 r^4)  *  r_hat
 */
void cs_calculate_gr_potential(
    struct reb_particle* const particles,
    int N,
    double C2,
    double G
);

/**
 * CS_GR_SINGLE — single-source full post-Newtonian correction.
 * Assumes particles[0] is the dominant source.
 * Works in Jacobi coordinates internally.
 * Velocity-dependent: sim->force_is_velocity_dependent must be 1.
 *
 * @param sim           parent simulation (needed for Jacobi transform helpers)
 * @param particles     particle array
 * @param N             number of real particles
 * @param C2            c^2 in simulation units
 * @param G             gravitational constant
 * @param max_iter      max iterations for implicit velocity solver (typical: 10)
 */
void cs_calculate_gr(
    struct reb_simulation* const sim,
    struct reb_particle* const particles,
    int N,
    double C2,
    double G,
    int max_iter
);

/**
 * CS_GR_FULL — full post-Newtonian correction for all body pairs.
 * No assumption about a dominant body.
 * Velocity-dependent: sim->force_is_velocity_dependent must be 1.
 * VLA-free: uses heap allocation internally (MSVC compatible).
 *
 * @param sim           parent simulation
 * @param particles     particle array
 * @param N             number of real particles
 * @param C2            c^2 in simulation units
 * @param G             gravitational constant
 * @param max_iter      max iterations for implicit acceleration solver (typical: 10)
 */
void cs_calculate_gr_full(
    struct reb_simulation* const sim,
    struct reb_particle* const particles,
    int N,
    double C2,
    double G,
    int max_iter
);

#ifdef __cplusplus
}
#endif

#endif /* CS_GR_H */