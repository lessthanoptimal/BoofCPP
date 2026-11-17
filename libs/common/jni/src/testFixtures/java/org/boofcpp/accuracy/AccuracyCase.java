package org.boofcpp.accuracy;

/// Measures the accuracy of one of a variant's ops against BoofCV's Java code
public interface AccuracyCase {
    void run( AccuracyRecord record );
}
