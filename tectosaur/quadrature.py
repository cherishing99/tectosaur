import numpy as np
import scipy.linalg
from math import factorial

# Derives the n-point gauss quadrature rule
def gaussxw(n):
    k = np.arange(1.0, n)
    a_band = np.zeros((2, n))
    a_band[1,0:n-1] = k / np.sqrt(4 * k * k - 1)
    x, V = scipy.linalg.eig_banded(a_band, lower=True)
    w = 2*np.real(np.power(V[0,:], 2))
    return x, w

# Change the domain of integration for a quadrature rule
def map_to(qr, interval):
    x01 = (qr[0] + 1) / 2
    outx = interval[0] + (interval[1] - interval[0]) * x01
    outw = (qr[1] / 2) * (interval[1] - interval[0])
    return outx, outw

# Integrate!
def quadrature(f, qr):
    return sum(f(qr[0]) * qr[1])

# Perform richardson extrapolation, lim h->0 of y(h)
def richardson(h_vals, y_vals):
    rich = [y_vals]
    h_vals_ratio = h_vals[:-1] / h_vals[1:]
    error_order = 1
    for m in range(1, len(y_vals)):
        prev_rich = rich[m - 1]
        mult = h_vals_ratio[(m - 1):] ** error_order
        error_order += 1
        factor = (1.0 / (mult - 1.0))
        next_rich = factor * (mult * prev_rich[1:] - prev_rich[:-1])
        rich.append(next_rich)
    return rich

# Sinh transform for integrals of the form \int_{-1}^1 f(x)<F12>
def sinh_transform(quad_rule, a, b, iterated = False):
    n_q = len(quad_rule[0])
    mu_0 = 0.5 * (np.arcsinh((1.0 + a) / b) + np.arcsinh((1.0 - a) / b))
    eta_0 = 0.5 * (np.arcsinh((1.0 + a) / b) - np.arcsinh((1.0 - a) / b))

    start_q = quad_rule
    if iterated:
        xs = np.empty(n_q)
        ws = np.empty(n_q)
        a_1 = eta_0 / mu_0
        b_1 = np.pi / (2 * mu_0)
        mu_1 = 0.5 * (np.arcsinh((1.0 + a_1) / b_1) + np.arcsinh((1.0 - a_1) / b_1))
        eta_1 = 0.5 * (np.arcsinh((1.0 + a_1) / b_1) - np.arcsinh((1.0 - a_1) / b_1));
        for i in range(n_q):
            u = quad_rule[0][i]
            xs[i] = a_1 + b_1 * np.sinh(mu_1 * u - eta_1);
            jacobian = b_1 * mu_1 * np.cosh(mu_1 * u - eta_1);
            ws[i] = quad_rule[1][i] * jacobian;
        start_q = (xs, ws)

    x = np.empty(n_q)
    w = np.empty(n_q)
    for i in range(n_q):
        s = start_q[0][i]
        x[i] = a + b * np.sinh(mu_0 * s - eta_0)
        jacobian = b * mu_0 * np.cosh(mu_0 * s - eta_0)
        w[i] = start_q[1][i] * jacobian
    return np.array(x), np.array(w)

def aimi_diligenti(quad_rule, p, q):
    n_q = len(quad_rule[0])
    x = np.zeros(n_q)
    w = np.zeros(n_q)
    n_transform_quad = int(np.floor((p + q) / 2.0)) ** 2
    transform_quad = gaussxw(n_transform_quad)
    for i in range(n_q):
        t = (quad_rule[0][i] + 1.0) / 2.0
        F = factorial(p + q - 1) / (factorial(p - 1) * factorial(q - 1))
        x[i] = 2 * F * quadrature(
            lambda us: [u ** (p - 1) * (1 - u) ** (q - 1) for u in us],
            map_to(transform_quad, [0, t])
        ) - 1.0
        w[i] = quad_rule[1][i] * F * t ** (p - 1) * (1 - t) ** (q - 1)
    return x, w
