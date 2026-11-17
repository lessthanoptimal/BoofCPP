package org.boofcpp.testing;

/// Tolerance a variant's floating point results must be within. Integer results must always be identical
public class Tolerance {
    /// Identical bits, as required of strict
    public static final Tolerance IDENTICAL = new Tolerance(0.0, 0.0);
    /// Floating point tolerance, as required of fast
    public static final Tolerance FLOAT = new Tolerance(1e-5, 1e-12);

    public final double f32;
    public final double f64;

    public Tolerance( double f32, double f64 ) {
        this.f32 = f32;
        this.f64 = f64;
    }
}
