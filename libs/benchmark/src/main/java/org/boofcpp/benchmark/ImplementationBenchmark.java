package org.boofcpp.benchmark;

import boofcv.concurrency.BoofConcurrency;
import org.boofcpp.NativeConcurrency;
import org.openjdk.jmh.annotations.*;
import org.openjdk.jmh.infra.BenchmarkParams;

import java.util.concurrent.TimeUnit;

/// Base of the benchmarks. Each benchmark method is one op called through BoofCV's API, with a [Hook] naming the hook
/// field its native implementations are registered with. Its JMH settings apply unless a benchmark sets its own
@BenchmarkMode(Mode.AverageTime)
@OutputTimeUnit(TimeUnit.MILLISECONDS)
@Warmup(iterations = 2)
@Measurement(iterations = 5)
@Fork(value = 1)
@State(Scope.Benchmark)
public abstract class ImplementationBenchmark {
    /// Name of an [Implementation]. [BenchmarkRunner] only runs a variant on the ops it has
    @Param({"java", "strict", "fast"})
    public String impl;

    /// Threads of the native code and of BoofCV's Java code. 1 is single threaded, which is the speed target against
    /// Java. [BenchmarkRunner] also runs every core the JVM sees, so slower cores, e.g. big.LITTLE's, are included
    @Param({"1"})
    public int threads;

    /// Registers the implementation's hooks and sets the threads, then returns the name of the op being run. Call it
    /// before creating the op, since BoofCV picks hooks and concurrent code when an op is created
    protected String setupImplementation( BenchmarkParams params ) {
        Implementation.valueOf(impl).register();
        NativeConcurrency.setMaxThreads(threads);
        BoofConcurrency.USE_CONCURRENT = threads > 1;
        BoofConcurrency.setMaxThreads(threads);
        String benchmark = params.getBenchmark();
        return benchmark.substring(benchmark.lastIndexOf('.') + 1);
    }

    /// Throws unless a hook handled the call, when the implementation isn't java. 'handled' is what BoofCV's
    /// `invokeNative...()` returned for the op's arguments, which also runs the op once. For ops without an object to
    /// check with [#checked]
    protected void checkHandled( boolean handled ) {
        if (handled == impl.equals("java"))
            throw new IllegalStateException(impl + (handled ? " ran a hook" : " has no hook that handles this call"));
    }

    /// 'alg', after checking it's BoofCPP's unless the implementation is java. Otherwise a hook that declines the
    /// configuration would time BoofCV's Java code
    protected <T> T checked( T alg ) {
        boolean isNative = alg.getClass().getName().startsWith("org.boofcpp.");
        if (isNative == impl.equals("java"))
            throw new IllegalStateException(impl + " created " + alg.getClass().getName());
        return alg;
    }
}
