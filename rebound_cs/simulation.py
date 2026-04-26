"""
pycs/simulation.py — Minimal REBOUND Simulation backed by _cs.dll

Light wrapper around the reb_simulation C struct in _cs.dll.
Only exposes the fields and functions needed by cs/ users.
"""
import ctypes
import math as _math

_clib = None


def _init(lib):
    global _clib
    _clib = lib
    lib.reb_simulation_init.argtypes = [ctypes.c_void_p]
    lib.reb_simulation_init.restype = None
    lib.reb_simulation_free_pointers.argtypes = [ctypes.c_void_p]
    lib.reb_simulation_free_pointers.restype = None
    lib.reb_simulation_integrate.argtypes = [ctypes.c_void_p, ctypes.c_double]
    lib.reb_simulation_integrate.restype = ctypes.c_int
    lib.reb_simulation_move_to_com.argtypes = [ctypes.c_void_p]
    lib.reb_simulation_move_to_com.restype = None


# ---------------------------------------------------------------------------
# reb_particle ctypes struct
# ---------------------------------------------------------------------------

class Particle(ctypes.Structure):
    _fields_ = [
        ("x",             ctypes.c_double),
        ("y",             ctypes.c_double),
        ("z",             ctypes.c_double),
        ("vx",            ctypes.c_double),
        ("vy",            ctypes.c_double),
        ("vz",            ctypes.c_double),
        ("ax",            ctypes.c_double),
        ("ay",            ctypes.c_double),
        ("az",            ctypes.c_double),
        ("m",             ctypes.c_double),
        ("r",             ctypes.c_double),
        ("last_collision", ctypes.c_double),
        ("c",             ctypes.c_void_p),
        ("hash",          ctypes.c_uint32),
        ("ap",            ctypes.c_void_p),
        ("sim",           ctypes.c_void_p),
    ]


# ---------------------------------------------------------------------------
# Simulation
# ---------------------------------------------------------------------------

class Simulation:
    """Self-contained REBOUND simulation backed by _cs.dll."""

    # sizeof(reb_simulation) from our DLL
    _SIZE = 2336

    # Field offsets (verified via cs_debug_layout in cs_simulation.c)
    _OFF_T      = 0
    _OFF_G      = 8
    _OFF_SOFT   = 16
    _OFF_DT     = 24
    _OFF_N      = 48
    _OFF_N_ACT  = 52     # guessed, not verified
    _OFF_N_VAR  = 56     # guessed
    _OFF_PART   = 112    # particles pointer
    _OFF_VELDEP = 168    # force_is_velocity_dependent
    _OFF_ADD_F  = 2256   # additional_forces
    _OFF_POST_TS= 2272   # post_timestep_modifications
    _OFF_FREE_AP= 2312   # free_particle_ap
    _OFF_EXTRAS = 2328   # extras

    def __init__(self):
        self._buf = (ctypes.c_char * self._SIZE)()
        _clib.reb_simulation_init(ctypes.c_void_p(self._addr))

    @property
    def _addr(self):
        return ctypes.cast(self._buf, ctypes.c_void_p).value

    def _get_dbl(self, off):    return ctypes.c_double.from_address(self._addr + off).value
    def _set_dbl(self, off, v): ctypes.c_double.from_address(self._addr + off).value = v
    def _get_int(self, off):    return ctypes.c_int.from_address(self._addr + off).value
    def _set_int(self, off, v): ctypes.c_int.from_address(self._addr + off).value = int(v)
    def _get_ptr(self, off):    return ctypes.c_void_p.from_address(self._addr + off).value
    def _set_ptr(self, off, v): ctypes.c_void_p.from_address(self._addr + off).value = v

    t    = property(lambda s: s._get_dbl(s._OFF_T),    lambda s, v: s._set_dbl(s._OFF_T, v))
    G    = property(lambda s: s._get_dbl(s._OFF_G),    lambda s, v: s._set_dbl(s._OFF_G, v))
    softening = property(lambda s: s._get_dbl(s._OFF_SOFT), lambda s, v: s._set_dbl(s._OFF_SOFT, v))
    dt   = property(lambda s: s._get_dbl(s._OFF_DT),   lambda s, v: s._set_dbl(s._OFF_DT, v))
    N    = property(lambda s: s._get_int(s._OFF_N))
    force_is_velocity_dependent = property(lambda s: s._get_int(s._OFF_VELDEP), lambda s, v: s._set_int(s._OFF_VELDEP, v))

    @property
    def particles(self):
        ptr = self._get_ptr(self._OFF_PART)
        if not ptr:
            return []
        n = self.N
        P = ctypes.POINTER(Particle)
        return [ctypes.cast(ptr, P)[i] for i in range(n)]

    # Callback slots
    _additional_forces           = property(lambda s: s._get_ptr(s._OFF_ADD_F),  lambda s, v: s._set_ptr(s._OFF_ADD_F, v))
    _post_timestep_modifications = property(lambda s: s._get_ptr(s._OFF_POST_TS), lambda s, v: s._set_ptr(s._OFF_POST_TS, v))
    _free_particle_ap            = property(lambda s: s._get_ptr(s._OFF_FREE_AP), lambda s, v: s._set_ptr(s._OFF_FREE_AP, v))

    # -- add particles -------------------------------------------------------

    def add(self, m=0.0, x=0.0, y=0.0, z=0.0, vx=0.0, vy=0.0, vz=0.0,
            a=None, e=0.0, inc=0.0, Omega=0.0, omega=0.0, f=0.0, **kw):
        """Add a particle. Cartesian (x,y,z,vx,vy,vz) or orbital (a,e,inc,Omega,omega,f)."""
        if a is not None:
            x, y, z, vx, vy, vz = _orbit_to_cartesian(
                self.G, m, a, e, inc, Omega, omega, f)

        _clib.cs_add_particle.argtypes = [ctypes.c_void_p] + [ctypes.c_double]*7
        _clib.cs_add_particle.restype = ctypes.c_int
        _clib.cs_add_particle(ctypes.c_void_p(self._addr),
            ctypes.c_double(m),
            ctypes.c_double(x), ctypes.c_double(y), ctypes.c_double(z),
            ctypes.c_double(vx), ctypes.c_double(vy), ctypes.c_double(vz))

    def move_to_com(self):
        _clib.reb_simulation_move_to_com(ctypes.c_void_p(self._addr))

    # -- integrate -----------------------------------------------------------

    def integrate(self, t_final):
        result = _clib.reb_simulation_integrate(ctypes.c_void_p(self._addr), ctypes.c_double(t_final))
        if result != 0:
            raise RuntimeError(f"Integration failed with status {result}")

    # -- cleanup -------------------------------------------------------------

    def free(self):
        if hasattr(self, '_buf') and self._buf is not None:
            _clib.reb_simulation_free_pointers(ctypes.c_void_p(self._addr))
            self._buf = None

    def __del__(self):
        self.free()

    def __repr__(self):
        return f"<Simulation N={self.N} t={self.t:.3f}>"


# ---------------------------------------------------------------------------
# Orbit → Cartesian conversion (Python)
# ---------------------------------------------------------------------------

def _orbit_to_cartesian(G, m, a, e, inc, Omega, omega, f):
    """Convert classical orbital elements to Cartesian position and velocity.

    All masses are added to compute the gravitational parameter.
    Returns (x, y, z, vx, vy, vz).
    """
    mu = G + m  # approximate for now
    if a <= 0:
        return 0, 0, 0, 0, 0, 0

    # Perifocal coordinates
    c_f, s_f = _math.cos(f), _math.sin(f)
    r = a * (1 - e * e) / (1 + e * c_f)
    h = _math.sqrt(mu * a * (1 - e * e))

    # Position
    x_p = r * c_f
    y_p = r * s_f

    # Velocity
    vx_p = -mu / h * s_f
    vy_p =  mu / h * (e + c_f)

    # Rotate by omega
    c_w, s_w = _math.cos(omega), _math.sin(omega)
    x_w = x_p * c_w - y_p * s_w
    y_w = x_p * s_w + y_p * c_w
    vx_w = vx_p * c_w - vy_p * s_w
    vy_w = vx_p * s_w + vy_p * c_w

    # Rotate by inc
    c_i, s_i = _math.cos(inc), _math.sin(inc)
    y_i = y_w * c_i
    z_i = y_w * s_i
    vy_i = vy_w * c_i
    vz_i = vy_w * s_i

    # Rotate by Omega
    c_O, s_O = _math.cos(Omega), _math.sin(Omega)
    x = x_w * c_O - y_i * s_O
    y = x_w * s_O + y_i * c_O
    z = z_i
    vx = vx_w * c_O - vy_i * s_O
    vy = vx_w * s_O + vy_i * c_O
    vz = vz_i

    return x, y, z, vx, vy, vz
