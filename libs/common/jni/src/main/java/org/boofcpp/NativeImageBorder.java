package org.boofcpp;

import boofcv.struct.border.BorderType;

/// A BoofCV border in the form the native code takes: extended, reflected, wrapped or zero. Anything else throws a
/// RuntimeException, which causes BoofCV to fall back to its Java implementation
public class NativeImageBorder {
    /// Border type, or null if every pixel outside the image has 'value'
    public final BorderType type;
    public final double value;

    private NativeImageBorder( BorderType type, double value ) {
        this.type = type;
        this.value = value;
    }

    public static NativeImageBorder of( BorderType type ) {
        switch (type) {
            case EXTENDED:
            case REFLECT:
            case WRAP:
                return new NativeImageBorder(type, 0);
            case ZERO:
                return new NativeImageBorder(null, 0);
            default:
                throw new RuntimeException("Unsupported border " + type);
        }
    }
}
