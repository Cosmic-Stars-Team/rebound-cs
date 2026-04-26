"""
demo_cs.py — Cosmic Stars Python demo

Install first (one-time):  pip install -e .
Run from anywhere:           python demo_cs.py
"""
import rebound_cs
import math


def demo_tides():
    print("=== Tides (CTL) ===")
    sim = rebound_cs.Simulation()
    sim.G = 1.0

    sim.add(m=1.0)                       # star
    sim.add(m=1e-3, a=0.05, e=0.3)       # close-in planet

    cs = sim.attach_cs()
    cs.enable_tides()

    # both bodies need tide params
    rebound_cs.CsParticleParams(tides_k2=0.5, tides_tau=0.0, tides_R=0.001).attach_to(sim.particles[0])
    rebound_cs.CsParticleParams(tides_k2=0.5, tides_tau=0.3, tides_R=0.002).attach_to(sim.particles[1])

    r0 = math.hypot(sim.particles[1].x, sim.particles[1].y)
    sim.integrate(0.5)
    r1 = math.hypot(sim.particles[1].x, sim.particles[1].y)
    print(f"  r: {r0:.4f} -> {r1:.4f}  (delta={r1-r0:.1e})")
    cs.free()
    sim.free()
    print()


def demo_gr():
    print("=== GR (potential mode) ===")
    sim = rebound_cs.Simulation()
    sim.G = 1.0

    sim.add(m=1.0)                                      # star
    sim.add(m=1e-6, a=0.2, e=0.9)                       # test particle

    cs = sim.attach_cs()
    cs.enable_gr(rebound_cs.CS_GR_POTENTIAL, rebound_cs.CS_C_AU_YR_MSUN)

    sim.integrate(1.0)
    print(f"  t={sim.t:.1f}")
    cs.free()
    sim.free()
    print()


def demo_radiation():
    print("=== Radiation (PR drag) ===")
    sim = rebound_cs.Simulation()
    sim.G = 1.0

    sim.add(m=1.0)                                      # radiation source (star)
    sim.add(m=1e-5, a=1.5, e=0.1)                       # dust particle

    cs = sim.attach_cs()
    cs.enable_radiation(rebound_cs.CS_C_AU_YR_MSUN)

    rebound_cs.CsParticleParams(rad_source=1).attach_to(sim.particles[0])
    rebound_cs.CsParticleParams(beta=0.01).attach_to(sim.particles[1])

    a0 = math.hypot(sim.particles[1].x, sim.particles[1].y)
    sim.integrate(0.5)
    a1 = math.hypot(sim.particles[1].x, sim.particles[1].y)
    print(f"  a: {a0:.4f} -> {a1:.4f}")
    cs.free()
    sim.free()
    print()


def demo_harmonics():
    print("=== Harmonics (J2) ===")
    sim = rebound_cs.Simulation()
    sim.G = 1.0

    sim.add(m=1.0)                                      # oblate central body
    sim.add(m=1e-5, a=0.1, e=0.01, inc=0.3)             # orbiting body

    cs = sim.attach_cs()
    cs.enable_harmonics()

    rebound_cs.CsParticleParams(J2=0.014, R_eq=0.001).attach_to(sim.particles[0])

    sim.integrate(0.1)
    print(f"  t={sim.t:.1f}  ok")
    cs.free()
    sim.free()
    print()


def demo_solarmass():
    print("=== Solar Mass Loss ===")
    sim = rebound_cs.Simulation()
    sim.G = 1.0

    sim.add(m=1.0)                                      # evolving star

    cs = sim.attach_cs()
    cs.enable_solarmass()

    rebound_cs.CsParticleParams(mdot=-0.001).attach_to(sim.particles[0])

    m0 = sim.particles[0].m
    sim.integrate(0.5)
    m1 = sim.particles[0].m
    print(f"  m: {m0:.4f} -> {m1:.6f}  (dm={m1-m0:.1e})")
    cs.free()
    sim.free()
    print()


if __name__ == "__main__":
    demo_tides()
    demo_gr()
    demo_radiation()
    demo_harmonics()
    demo_solarmass()
    print("Done.")
