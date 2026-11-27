package org.boofcpp.benchmark;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

/// The BoofCV hook field a benchmarked op's native implementations are registered with, e.g.
/// `@Hook(type = BOverrideFactoryThresholdBinary.class, field = "globalOtsu")`. [BenchmarkRunner] only runs a variant
/// on the ops whose field it sets
@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.METHOD)
public @interface Hook {
    /// BoofCV's class with the hook field
    Class<?> type();

    /// Name of the hook field
    String field();
}
