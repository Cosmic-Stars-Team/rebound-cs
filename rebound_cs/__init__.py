"""
pycs — Cosmic Stars Physics Extension for Python
=================================================

Self-contained physics extension: uses its own compiled _cs.dll
which bundles REBOUND + cs modules together.  No dependency on
the ``rebound`` pip package — guarantees struct layout consistency.

Usage::

    import pycs

    sim = pycs.Simulation()
    sim.G = 1.0
    sim.add(m=1.0)          # star
    sim.add(m=1e-3, a=0.08, e=0.3)

    cs = sim.attach_cs()
    cs.enable_tides()

    params = pycs.CsParticleParams(tides_k2=0.5, tides_tau=0.3, tides_R=0.002)
    params.attach_to(sim.particles[1])

    sim.integrate(100.0)
"""
import ctypes
import os

# ---------------------------------------------------------------------------
# Library loading
# ---------------------------------------------------------------------------

def _find_cs_lib():
    mod_dir = os.path.dirname(os.path.abspath(__file__))
    import sys as _sys
    if _sys.platform == "win32":
        candidates = ["_cs.dll", "_cs.pyd"]
    elif _sys.platform == "darwin":
        candidates = ["_cs.dylib", "_cs.so"]
    else:
        candidates = ["_cs.so"]
    for name in candidates:
        path = os.path.join(mod_dir, name)
        if os.path.exists(path):
            return path
    raise ImportError(
        "Cannot find CS shared library (_cs.dll / _cs.so).\n"
        "  Build it with:  make pycs   (top-level Makefile)"
    )

_clib = ctypes.cdll.LoadLibrary(_find_cs_lib())

# ---------------------------------------------------------------------------
# C function signatures — FFI path
# ---------------------------------------------------------------------------

_clib.cs_ffi_create.argtypes = []
_clib.cs_ffi_create.restype = ctypes.c_void_p
_clib.cs_ffi_free.argtypes = [ctypes.c_void_p]
_clib.cs_ffi_free.restype = None
_clib.cs_ffi_dispatch_af.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
_clib.cs_ffi_dispatch_af.restype = None
_clib.cs_ffi_dispatch_post_timestep.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
_clib.cs_ffi_dispatch_post_timestep.restype = None

_clib.cs_enable_gr.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_double]
_clib.cs_enable_gr.restype = None
_clib.cs_disable_gr.argtypes = [ctypes.c_void_p]
_clib.cs_disable_gr.restype = None
_clib.cs_enable_radiation.argtypes = [ctypes.c_void_p, ctypes.c_double]
_clib.cs_enable_radiation.restype = None
_clib.cs_disable_radiation.argtypes = [ctypes.c_void_p]
_clib.cs_disable_radiation.restype = None
_clib.cs_enable_harmonics.argtypes = [ctypes.c_void_p]
_clib.cs_enable_harmonics.restype = None
_clib.cs_enable_tides.argtypes = [ctypes.c_void_p]
_clib.cs_enable_tides.restype = None
_clib.cs_enable_solarmass.argtypes = [ctypes.c_void_p]
_clib.cs_enable_solarmass.restype = None

_clib.cs_particle_params_create.argtypes = []
_clib.cs_particle_params_create.restype = ctypes.c_void_p
_clib.cs_particle_params_set.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
_clib.cs_particle_params_set.restype = None

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

# Mirror of C enums from cs/cs_simulation.h — keep in sync
CS_GR_POTENTIAL = 0
CS_GR_SINGLE = 1
CS_GR_FULL = 2
# Mirror of C defines from cs/cs.h — keep in sync
CS_C_SI = 299792458.0
CS_C_AU_YR_MSUN = 63241.077
CS_C_AU_DAY_MSUN = 173.14463269

# ---------------------------------------------------------------------------
# cs_particle_params_t — ctypes Structure
# ---------------------------------------------------------------------------

class _CsParticleParamsC(ctypes.Structure):
    _fields_ = [
        ("gr_source",       ctypes.c_int),
        ("beta",            ctypes.c_double),
        ("rad_source",      ctypes.c_int),
        ("tides_k2",        ctypes.c_double),
        ("tides_Q",         ctypes.c_double),
        ("tides_tau",       ctypes.c_double),
        ("tides_R",         ctypes.c_double),
        ("spin_Omega_x",    ctypes.c_double),
        ("spin_Omega_y",    ctypes.c_double),
        ("spin_Omega_z",    ctypes.c_double),
        ("spin_I",          ctypes.c_double),
        ("J2",              ctypes.c_double),
        ("J4",              ctypes.c_double),
        ("J6",              ctypes.c_double),
        ("R_eq",            ctypes.c_double),
        ("tau_a",           ctypes.c_double),
        ("tau_e",           ctypes.c_double),
        ("tau_inc",         ctypes.c_double),
        ("stochastic_k",    ctypes.c_double),
        ("central_force_A", ctypes.c_double),
        ("central_force_n", ctypes.c_double),
        ("mdot",            ctypes.c_double),
    ]


class CsParticleParams:
    def __init__(self, **kwargs):
        self._ptr = _clib.cs_particle_params_create()
        if not self._ptr:
            raise MemoryError("cs_particle_params_create returned NULL")
        self._cstruct = _CsParticleParamsC.from_address(self._ptr)
        for k, v in kwargs.items():
            setattr(self._cstruct, k, v)

    def __getattr__(self, name):
        if name.startswith("_"): return super().__getattribute__(name)
        return getattr(self._cstruct, name)

    def __setattr__(self, name, value):
        if name.startswith("_"): super().__setattr__(name, value)
        else: setattr(self._cstruct, name, value)

    def __repr__(self):
        fields = {f[0]: getattr(self._cstruct, f[0]) for f in self._cstruct._fields_}
        kv = ", ".join(f"{k}={v}" for k, v in fields.items() if v != 0)
        return f"<CsParticleParams {kv}>"

    def attach_to(self, particle):
        _clib.cs_particle_params_set(ctypes.byref(particle), self._ptr)


# ---------------------------------------------------------------------------
# Simulation — import from submodule
# ---------------------------------------------------------------------------

from .simulation import Simulation
from .simulation import _init as _sim_init
_sim_init(_clib)


# ---------------------------------------------------------------------------
# CsSimulation — physics extension handle
# ---------------------------------------------------------------------------

class CsSimulation:
    def __init__(self, sim):
        self._sim = sim
        self._ptr = _clib.cs_ffi_create()
        if not self._ptr:
            raise RuntimeError("cs_ffi_create returned NULL")

        cs_ptr = self._ptr

        # additional_forces dispatch
        AFF = ctypes.CFUNCTYPE(None, ctypes.c_void_p)
        @AFF
        def _af_dispatch(raw_sim_ptr):
            _clib.cs_ffi_dispatch_af(raw_sim_ptr, cs_ptr)

        self._afp = _af_dispatch
        sim._additional_forces = ctypes.cast(self._afp, ctypes.c_void_p).value

        # post_timestep dispatch (for solarmass)
        @AFF
        def _pt_dispatch(raw_sim_ptr):
            _clib.cs_ffi_dispatch_post_timestep(raw_sim_ptr, cs_ptr)

        self._ptp = _pt_dispatch
        sim._post_timestep_modifications = ctypes.cast(self._ptp, ctypes.c_void_p).value

    # -- enable / disable -----------------------------------------------
    # NOTE: force_is_velocity_dependent mirrors C logic in cs_simulation.c
    # VEL_DEP_MASK there must match the modules that set it here.

    def enable_gr(self, mode=CS_GR_POTENTIAL, c=CS_C_AU_YR_MSUN):
        _clib.cs_enable_gr(self._ptr, mode, c)
        if mode in (CS_GR_SINGLE, CS_GR_FULL):
            self._sim.force_is_velocity_dependent = 1

    def disable_gr(self):
        _clib.cs_disable_gr(self._ptr)

    def enable_radiation(self, c=CS_C_AU_YR_MSUN):
        _clib.cs_enable_radiation(self._ptr, c)
        self._sim.force_is_velocity_dependent = 1

    def disable_radiation(self):
        _clib.cs_disable_radiation(self._ptr)

    def enable_harmonics(self):
        _clib.cs_enable_harmonics(self._ptr)

    def enable_tides(self):
        _clib.cs_enable_tides(self._ptr)
        self._sim.force_is_velocity_dependent = 1

    def enable_solarmass(self):
        _clib.cs_enable_solarmass(self._ptr)

    # -- lifecycle ------------------------------------------------------

    def free(self):
        if self._ptr is not None:
            _clib.cs_ffi_free(self._ptr)
            self._ptr = None

    def __del__(self):
        self.free()

    def __repr__(self):
        return f"<CsSimulation at {hex(self._ptr) if self._ptr else 'freed'}>"


# Attach helper on Simulation
def _sim_attach_cs(self):
    """Attach CS physics extension to this simulation."""
    return CsSimulation(self)

Simulation.attach_cs = _sim_attach_cs
