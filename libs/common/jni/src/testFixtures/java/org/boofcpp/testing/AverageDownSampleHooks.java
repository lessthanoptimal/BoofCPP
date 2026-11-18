package org.boofcpp.testing;

import boofcv.alg.filter.misc.BOverrideAverageDownSampleOps;

/// Runs code with chosen BOverrideAverageDownSampleOps hooks, e.g. null for BoofCV's Java code
public class AverageDownSampleHooks {
    /// Sets the hooks while 'call' runs, then restores the registered ones
    public static void run( BOverrideAverageDownSampleOps.Square square,
                            BOverrideAverageDownSampleOps.General general,
                            BOverrideAverageDownSampleOps.Horizontal horizontal,
                            BOverrideAverageDownSampleOps.Vertical vertical, Runnable call ) {
        BOverrideAverageDownSampleOps.Square previousSquare = BOverrideAverageDownSampleOps.square;
        BOverrideAverageDownSampleOps.General previousGeneral = BOverrideAverageDownSampleOps.general;
        BOverrideAverageDownSampleOps.Horizontal previousHorizontal = BOverrideAverageDownSampleOps.horizontal;
        BOverrideAverageDownSampleOps.Vertical previousVertical = BOverrideAverageDownSampleOps.vertical;
        try {
            BOverrideAverageDownSampleOps.square = square;
            BOverrideAverageDownSampleOps.general = general;
            BOverrideAverageDownSampleOps.horizontal = horizontal;
            BOverrideAverageDownSampleOps.vertical = vertical;
            call.run();
        } finally {
            BOverrideAverageDownSampleOps.square = previousSquare;
            BOverrideAverageDownSampleOps.general = previousGeneral;
            BOverrideAverageDownSampleOps.horizontal = previousHorizontal;
            BOverrideAverageDownSampleOps.vertical = previousVertical;
        }
    }

    /// Runs BoofCV's Java code
    public static void withoutHooks( Runnable call ) {
        run(null, null, null, null, call);
    }
}
