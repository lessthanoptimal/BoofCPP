package org.boofcpp;

import boofcv.core.image.ImageBorderValue;
import boofcv.core.image.border.BorderIndex1D_Extend;
import boofcv.core.image.border.BorderIndex1D_Reflect;
import boofcv.core.image.border.BorderIndex1D_Wrap;
import boofcv.struct.border.BorderType;
import boofcv.struct.border.ImageBorder;
import boofcv.struct.border.ImageBorder1D;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageBase;

/// A BoofCV border in the form the native code takes. Native supports borders that are extended, reflected or wrapped,
/// the same for rows and columns, and a constant value. Anything else throws a RuntimeException, which causes BoofCV to
/// fall back to its Java implementation
public class NativeImageBorder {
    /// Border type, or null if every pixel outside the image has 'value'
    public final BorderType type;
    public final double value;

    private NativeImageBorder( BorderType type, double value ) {
        this.type = type;
        this.value = value;
    }

    /// @param image Image the border is for. An integer value must fit in its pixels, GrayU8 or GrayU16
    public static NativeImageBorder of( ImageBorder border, ImageBase image ) {
        if (border instanceof ImageBorder1D)
            return new NativeImageBorder(indexType((ImageBorder1D)border), 0);
        if (border instanceof ImageBorderValue.Value_I) {
            int value = ((ImageBorderValue.Value_I)border).value;
            int max = image instanceof GrayU8 ? 0xFF : 0xFFFF;
            if (value < 0 || value > max)
                throw new RuntimeException("Border value can't be stored in the image's pixel type");
            return new NativeImageBorder(null, value);
        }
        if (border instanceof ImageBorderValue.Value_F32)
            return new NativeImageBorder(null, ((ImageBorderValue.Value_F32)border).value);
        if (border instanceof ImageBorderValue.Value_F64)
            return new NativeImageBorder(null, ((ImageBorderValue.Value_F64)border).value);
        String name = border == null ? "null" : border.getClass().getSimpleName();
        throw new RuntimeException("Unsupported border " + name);
    }

    /// The border type of a border that changes the index of pixels outside the image
    private static BorderType indexType( ImageBorder1D border ) {
        Class<?> rowType = border.getRowWrap().getClass();
        if (rowType != border.getColWrap().getClass())
            throw new RuntimeException("Rows and columns must have the same border");
        if (rowType == BorderIndex1D_Extend.class)
            return BorderType.EXTENDED;
        if (rowType == BorderIndex1D_Reflect.class)
            return BorderType.REFLECT;
        if (rowType == BorderIndex1D_Wrap.class)
            return BorderType.WRAP;
        throw new RuntimeException("Unsupported border " + rowType.getSimpleName());
    }
}
